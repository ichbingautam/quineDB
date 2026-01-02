#include "storage/engine.hpp" // [NEW] Link Storage Layer
#include <iostream>
#include <thread>
#include <vector>

/// @file main.cpp
/// @brief Entry point for the QuineDB Server.
///
/// This file bootstraps the Thread-per-Core architecture.
/// It detects the number of available hardware contexts and spawns
/// a dedicated worker thread for each. Each worker runs its own
/// isolated Event Loop (IoContext), adhering to the Shared-Nothing design.

/// @brief Worker function that runs on each core.
/// @param id The logical core ID (0 to N-1).
void worker(int id) {
  // In a real implementation, we would:
  // 1. Initialize the thread-local IoContext (io_uring).
  // 2. Instantiate the TcpServer listening on the shared port (SO_REUSEPORT).
  // 3. Enter the Event Loop (ctx.submit_and_wait).

  // For now, we just print initialization status.
  std::cout << "[Core " << id << "] Initialized on thread "
            << std::this_thread::get_id() << std::endl;

  // Simulation of work
  // quine::core::IoContext ctx;
  // quine::network::TcpServer server(ctx, 6379);
  // server.start();
  // ctx.run();
}

int main() {
  // 1. Detect hardware concurrency
  // We want 1 thread per physical core to minimize context switching
  unsigned int n_threads = std::thread::hardware_concurrency();
  std::cout << "QuineDB Server starting on " << n_threads << " cores."
            << std::endl;

  std::vector<std::thread> threads;
  threads.reserve(n_threads);

  // 2. Launch pinned worker threads
  for (unsigned int i = 0; i < n_threads; ++i) {
    threads.emplace_back(worker, i);
    // Optional: Pin thread to CPU core 'i' using pthread_setaffinity_np here
  }

  // 3. Wait for threads (Server runs until SIGINT/SIGTERM)
  for (auto &t : threads) {
    if (t.joinable()) {
      t.join();
    }
  }

  return 0;
}
