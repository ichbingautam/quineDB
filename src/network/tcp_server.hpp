#pragma once

#include "../core/io_context.hpp"
#include "../core/topology.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

// Forward decl
namespace quine {
namespace core {
struct Operation;
}
} // namespace quine
namespace quine {
namespace network {
class Connection;
}
} // namespace quine

namespace quine {
namespace network {

/// @brief Handles listening for incoming TCP connections using io_uring.
/// Designed for a Thread-per-Core architecture where multiple instances
/// can listen on the same port via SO_REUSEPORT.
class TcpServer {
public:
  TcpServer(core::IoContext &io, int port, core::Topology &topology,
            size_t core_id);
  ~TcpServer();

  // Delete copy/move
  TcpServer(const TcpServer &) = delete;
  TcpServer &operator=(const TcpServer &) = delete;

  /// @brief Starts the asynchronous accept loop.
  /// Submits the initial accept request to the io_uring.
  void start();

  /// @brief Set callback for when a new connection is established
  void set_on_connect(std::function<void(Connection *)> cb) {
    on_connect_ = cb;
  }

  /// @brief Set callback for connection disconnection
  void set_on_disconnect(std::function<void(uint32_t)> cb) {
    on_disconnect_ = cb;
  }

private:
  core::IoContext &io_;
  core::Topology &topology_;
  size_t core_id_;
  int port_;
  int server_fd_;
  std::function<void(Connection *)> on_connect_;
  std::function<void(uint32_t)> on_disconnect_;

  void setup_listener();
  void submit_accept();

  // The operation object that handles the completion of the accept call
  struct AcceptOp;
  std::unique_ptr<AcceptOp> accept_op_;

  void handle_accept(int fd);
};

} // namespace network
} // namespace quine
