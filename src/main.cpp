#include "core/config.hpp" // [NEW]
#include "core/io_context.hpp"
#include "core/topology.hpp"
#include "network/connection.hpp"
#include "network/tcp_server.hpp"
#include <iostream>
#include <thread>
#include <unordered_map>
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

    // Registry for local connections (ID -> Ptr)
    std::unordered_map<uint32_t, quine::network::Connection *>
        local_connections;

    // 2. Initialize TCP Server (Shared Port via SO_REUSEPORT)
    quine::network::TcpServer server(ctx, port, topology, core_id);

    // Track new connections
    server.set_on_connect([&](quine::network::Connection *conn) {
      local_connections[conn->get_id()] = conn;
    });

    server.set_on_disconnect(
        [&](uint32_t conn_id) { local_connections.erase(conn_id); });

    server.start();

    topology.register_notify_fd(core_id, ctx.get_notify_fd());

    // 3. Register ITC Notification Handler
    auto *my_channel = topology.get_channel(core_id);
    ctx.set_notification_handler([&]() {
      // Process all pending messages in the inbox
      my_channel->consume_all([&](quine::core::Message &&msg) {
        if (msg.type == quine::core::MessageType::REQUEST) {
          // Execute on local shard (Remote Request)
          auto *shard = topology.get_shard(core_id);
          std::string cmd = msg.args[0];
          std::string response_str;

          if (cmd == "SET" && msg.args.size() == 3) {
            shard->set(msg.args[1], msg.args[2]);
            response_str = "+OK\r\n";
          } else if (cmd == "DEL" && msg.args.size() == 2) {
            bool res = shard->del(msg.args[1]);
            response_str = ":" + std::to_string(res ? 1 : 0) + "\r\n";
          } else if (cmd == "GET" && msg.args.size() == 2) {
            auto val = shard->get(msg.args[1]);
            if (val)
              response_str =
                  "$" + std::to_string(val->size()) + "\r\n" + *val + "\r\n";
            else
              response_str = "$-1\r\n";
          } else {
            response_str = "-ERR unsupported forwarded command\r\n";
          }

          // Send RESPONSE back to origin core
          quine::core::Message reply;
          reply.type = quine::core::MessageType::RESPONSE;
          reply.origin_core_id = core_id; // Sender (us)
          reply.conn_id = msg.conn_id;    // Route to original connection
          reply.payload = response_str;
          reply.success = true;

          topology.get_channel(msg.origin_core_id)->push(reply);
          topology.notify_core(msg.origin_core_id);

        } else if (msg.type == quine::core::MessageType::RESPONSE) {
          // Received result from another core for one of our connections
          auto it = local_connections.find(msg.conn_id);
          if (it != local_connections.end()) {
            auto *conn = it->second;
            // Convert string to vector<char>
            std::vector<char> resp_data(msg.payload.begin(), msg.payload.end());
            conn->submit_write(ctx, std::move(resp_data));
          }
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

#include "commands/registry.hpp"
#include "commands/string_commands.hpp"

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  // 1. Load Configuration
  quine::core::Config config;
  // TODO: Load from config file or CLI args (argc/argv)

  // 1. Initialize Registry
  auto &registry = quine::commands::CommandRegistry::instance();
  registry.register_command(std::make_unique<quine::commands::SetCommand>());
  registry.register_command(std::make_unique<quine::commands::GetCommand>());

  unsigned int n_threads = config.worker_threads > 0
                               ? config.worker_threads
                               : std::thread::hardware_concurrency();

  std::cout << "QuineDB Server starting on " << n_threads << " cores, port "
            << config.port << std::endl;
  std::cout << "RDB Persistence: " << config.rdb_filename << " ("
            << config.save_params.size() << " save points)" << std::endl;

  // 1. Create Static Topology (Shared state resource container)
  // The Topology itself is thread-safe or read-only (Router),
  // Shards are single-threaded accessed by owner core.
  quine::core::Topology topology(n_threads);

  std::vector<std::thread> threads;
  threads.reserve(n_threads);

  // 2. Launch pinned worker threads
  for (unsigned int i = 0; i < n_threads; ++i) {
    threads.emplace_back(worker_main, i, config.port, std::ref(topology));
  }

  // 3. Wait for threads
  for (auto &t : threads) {
    if (t.joinable())
      t.join();
  }

  return 0;
}
