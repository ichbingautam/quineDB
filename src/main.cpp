#include <iostream>
#include <thread>
#include <unordered_map>
#include <vector>

#include "core/config.hpp"
#include "core/io_context.hpp"
#include "core/topology.hpp"
#include "network/connection.hpp"
#include "network/tcp_server.hpp"

/// @file main.cpp
/// @brief Entry point for the QuineDB Server.
///
/// This file bootstraps the Thread-per-Core architecture.
/// it detects the number of available hardware contexts and spawns
/// a dedicated worker thread for each. Each worker runs its own
/// isolated Event Loop (IoContext), adhering to the Shared-Nothing design.

#include "commands/list_commands.hpp"
#include "commands/registry.hpp"
#include "commands/string_commands.hpp"

// Worker thread function
void worker_main(size_t core_id, int port, quine::core::Topology& topology) {
  try {
    // 1. Initialize Thread-Local Event Loop
    quine::core::IoContext ctx;

    // Registry for local connections (ID -> Ptr)
    std::unordered_map<uint32_t, quine::network::Connection*> local_connections;

    topology.register_notify_fd(core_id, ctx.get_notify_fd());

    // 2.5 Wait for all cores to initialize their FDs
    // This prevents a race condition where a core receives a request (via
    // another core) before it has registered its notification FD.
    topology.wait_for_all_cores();

    // 3. Initialize TCP Server (Shared Port via SO_REUSEPORT)
    quine::network::TcpServer server(ctx, port, topology, core_id);

    // Track new connections
    server.set_on_connect(
        [&](quine::network::Connection* conn) { local_connections[conn->get_id()] = conn; });

    server.set_on_disconnect([&](uint32_t conn_id) { local_connections.erase(conn_id); });

    server.start();

    // 4. Register ITC Notification Handler
    auto* my_channel = topology.get_channel(core_id);
    ctx.set_notification_handler([&]() {
      // Process all pending messages in the inbox
      my_channel->consume_all([&](quine::core::Message&& msg) {
        if (msg.type == quine::core::MessageType::REQUEST) {
          // Execute on local shard (Remote Request)
          std::string cmd_name = msg.args[0];
          std::string response_str;

          // Use Registry to execute command
          auto* cmd = quine::commands::CommandRegistry::instance().get_command(cmd_name);
          if (cmd) {
            // Execute the command directly on this core
            // Note: msg.args contains the full command [SET, key, value]
            response_str = cmd->execute(topology, core_id, msg.conn_id, msg.args);
            // Since we are on the target core, execute() should return the
            // result string and NOT perform forwarding (as is_local will be
            // true). However, if the command returns empty string (which
            // implies "forwarded"), that would be a bug here since we ARE the
            // destination. But execute() logic checks is_local(core_id, key).
            // Since we routed it here, it MUST be local.
          } else {
            response_str = "-ERR unknown command '" + cmd_name + "'\r\n";
          }

          // Send RESPONSE back to origin core
          if (!response_str.empty()) {
            quine::core::Message reply;
            reply.type = quine::core::MessageType::RESPONSE;
            reply.origin_core_id = core_id;  // Sender (us)
            reply.conn_id = msg.conn_id;     // Route to original connection
            reply.payload = response_str;
            reply.success = true;

            auto* origin_channel = topology.get_channel(msg.origin_core_id);
            if (origin_channel) {
              origin_channel->push(reply);
              topology.notify_core(msg.origin_core_id);
            }
          }

        } else if (msg.type == quine::core::MessageType::RESPONSE) {
          // Received result from another core for one of our connections
          auto it = local_connections.find(msg.conn_id);
          if (it != local_connections.end()) {
            auto* conn = it->second;
            // Convert string to vector<char>
            std::vector<char> resp_data(msg.payload.begin(), msg.payload.end());
            conn->submit_write(ctx, std::move(resp_data));
          }
        }
      });
    });

    std::cout << "[Core " << core_id << "] Started on thread " << std::this_thread::get_id()
              << std::endl;

    // 4. Run Event Loop
    ctx.run();
  } catch (const std::exception& e) {
    std::cerr << "[Core " << core_id << "] Error: " << e.what() << std::endl;
  }
}

#include "commands/admin_commands.hpp"
#include "commands/generic_commands.hpp"
#include "commands/hash_commands.hpp"
#include "commands/set_commands.hpp"
#include "commands/zset_commands.hpp"
#include "persistence/rdb_manager.hpp"

// ... (worker_main stays same, skipping lines 23-113) ...

// Start of main()
int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;
  // 1. Load Configuration
  quine::core::Config config;

  unsigned int n_threads =
      config.worker_threads > 0 ? config.worker_threads : std::thread::hardware_concurrency();

  // Initialize Topology FIRST because RDB loader needs it
  quine::core::Topology topology(n_threads);

  std::cout << "QuineDB Server starting on " << n_threads << " cores, port " << config.port
            << std::endl;
  std::cout << "RDB Persistence: " << config.rdb_filename << " (" << config.save_params.size()
            << " save points)" << std::endl;

  // Try loading RDB
  if (quine::persistence::RdbManager::load(topology, config.rdb_filename)) {
    std::cout << "[RDB] Loaded successfully from " << config.rdb_filename << std::endl;
  } else {
    std::cout << "[RDB] No valid RDB file found, starting empty." << std::endl;
  }

  // 1. Initialize Registry
  auto& registry = quine::commands::CommandRegistry::instance();
  registry.register_command(std::make_unique<quine::commands::SetCommand>());
  registry.register_command(std::make_unique<quine::commands::GetCommand>());
  registry.register_command(std::make_unique<quine::commands::DelCommand>());

  registry.register_command(std::make_unique<quine::commands::LPushCommand>());
  registry.register_command(std::make_unique<quine::commands::LPopCommand>());
  registry.register_command(std::make_unique<quine::commands::LRangeCommand>());
  registry.register_command(std::make_unique<quine::commands::RPushCommand>());
  registry.register_command(std::make_unique<quine::commands::RPopCommand>());
  registry.register_command(std::make_unique<quine::commands::LLenCommand>());

  registry.register_command(std::make_unique<quine::commands::SAddCommand>());
  registry.register_command(std::make_unique<quine::commands::SMembersCommand>());
  registry.register_command(std::make_unique<quine::commands::SCardCommand>());
  registry.register_command(std::make_unique<quine::commands::SRemCommand>());

  registry.register_command(std::make_unique<quine::commands::HSetCommand>());
  registry.register_command(std::make_unique<quine::commands::HGetCommand>());
  registry.register_command(std::make_unique<quine::commands::HGetAllCommand>());
  registry.register_command(std::make_unique<quine::commands::HDelCommand>());
  registry.register_command(std::make_unique<quine::commands::HLenCommand>());

  registry.register_command(std::make_unique<quine::commands::ZAddCommand>());
  registry.register_command(std::make_unique<quine::commands::ZRangeCommand>());
  registry.register_command(std::make_unique<quine::commands::ZRemCommand>());
  registry.register_command(std::make_unique<quine::commands::ZCardCommand>());
  registry.register_command(std::make_unique<quine::commands::ZScoreCommand>());
  registry.register_command(std::make_unique<quine::commands::ExpireCommand>());
  registry.register_command(std::make_unique<quine::commands::TtlCommand>());
  registry.register_command(std::make_unique<quine::commands::SaveCommand>());

  std::vector<std::thread> threads;
  threads.reserve(n_threads);

  // 2. Launch pinned worker threads
  for (unsigned int i = 0; i < n_threads; ++i) {
    threads.emplace_back(worker_main, i, config.port, std::ref(topology));
  }

  // 3. Wait for threads
  for (auto& t : threads) {
    if (t.joinable()) t.join();
  }

  return 0;
}
