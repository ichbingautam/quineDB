#include "core/io_context.hpp"
#include "core/topology.hpp"
#include "network/tcp_server.hpp"
#include <iostream>
#include <thread>
#include <vector>

/// @file main.cpp
/// @brief Entry point for the QuineDB Server.
///
/// This file bootstraps the Thread-per-Core architecture.
/// it detects the number of available hardware contexts and spawns
/// a dedicated worker thread for each. Each worker runs its own
/// isolated Event Loop (IoContext), adhering to the Shared-Nothing design.

// Worker thread function
void worker_main(size_t core_id, int port, quine::core::Topology &topology) {
  try {
    // 1. Initialize Thread-Local Event Loop
    quine::core::IoContext ctx;

    // 2. Initialize TCP Server (Shared Port via SO_REUSEPORT)
    quine::network::TcpServer server(ctx, port, topology, core_id);
    server.start();

    // 3. Register ITC Notification Handler
    auto *my_channel = topology.get_channel(core_id);
    ctx.set_notification_handler([&]() {
      // Process all pending messages in the inbox
      my_channel->consume_all([&](quine::core::Message &&msg) {
        // HACK: for V0 logging
        // std::cout << "[Core " << core_id << "] Recv Message: type="
        //           << (int)msg.type << " key=" << msg.key << std::endl;

        if (msg.type == quine::core::MessageType::REQUEST) {
          // TODO: Execute on local shard and reply
          auto shard = topology.get_shard(core_id);
          // shard->op(...)
          // Then send RESPONSE back to origin
        }
      });
    });

    std::cout << "[Core " << core_id << "] Started on thread "
              << std::this_thread::get_id() << std::endl;

    // 4. Run Event Loop
    ctx.run();
  } catch (const std::exception &e) {
    std::cerr << "[Core " << core_id << "] Error: " << e.what() << std::endl;
  }
}

int main() {
  unsigned int n_threads = std::thread::hardware_concurrency();
  // Default port 6379 (Redis standard)
  int port = 6379;

  std::cout << "QuineDB Server starting on " << n_threads << " cores, port "
            << port << std::endl;

  // 1. Create Static Topology (Shared state resource container)
  // The Topology itself is thread-safe or read-only (Router),
  // Shards are single-threaded accessed by owner core.
  quine::core::Topology topology(n_threads);

  std::vector<std::thread> threads;
  threads.reserve(n_threads);

  // 2. Launch pinned worker threads
  for (unsigned int i = 0; i < n_threads; ++i) {
    threads.emplace_back(worker_main, i, port, std::ref(topology));
  }

  // 3. Wait for threads
  for (auto &t : threads) {
    if (t.joinable())
      t.join();
  }

  return 0;
}
