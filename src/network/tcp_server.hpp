#pragma once

#include "../core/io_context.hpp"
#include "../storage/shard.hpp"
#include <cstdint>
#include <string>

namespace quine {
namespace network {

/// @brief Handles listening for incoming TCP connections using io_uring.
/// Designed for a Thread-per-Core architecture where multiple instances
/// can listen on the same port via SO_REUSEPORT.
class TcpServer {
public:
  TcpServer(core::IoContext &io, int port, storage::Shard *shard);
  ~TcpServer();

  // Delete copy/move
  TcpServer(const TcpServer &) = delete;
  TcpServer &operator=(const TcpServer &) = delete;

  /// @brief Starts the asynchronous accept loop.
  /// Submits the initial accept request to the io_uring.
  void start();

private:
  core::IoContext &io_;
  storage::Shard *shard_;
  int port_;
  int server_fd_;

  void setup_listener();
  void submit_accept();
};

} // namespace network
} // namespace quine
