#pragma once

#include "../core/operation.hpp"
#include "../core/topology.hpp" // [NEW]
#include "resp_parser.hpp"
#include <cstddef>
#include <functional> // [NEW]
#include <memory>
#include <string>
#include <vector>

// Forward decl
namespace quine {
namespace core {
class IoContext;
}
} // namespace quine

namespace quine {
namespace network {

/// @brief Represents a single client TCP connection.
/// Manages the file descriptor and the read/write data buffers.
class Connection {
public:
  explicit Connection(int fd, core::Topology &topology, size_t core_id);
  ~Connection();

  Connection(const Connection &) = delete;
  Connection &operator=(const Connection &) = delete;

  int get_fd() const { return fd_; }
  uint32_t get_id() const { return id_; }

  void set_on_disconnect(std::function<void(uint32_t)> cb) {
    on_disconnect_ = cb;
  } // [NEW] // [NEW]

  // Start processing (post initial read)
  void start(core::IoContext &ctx);

  // Buffer management
  void resize_buffer(size_t size);

  // Process incoming data
  // Returns response bytes to write back
  std::vector<char> handle_data(const char *data, size_t len);

  // Async Operations
  struct ReadOp;
  struct WriteOp;

  std::unique_ptr<ReadOp> read_op_;
  std::unique_ptr<WriteOp> write_op_;

  void submit_read(core::IoContext &ctx);
  void submit_write(core::IoContext &ctx, std::vector<char> data);

  void handle_read(int res, core::IoContext &ctx);
  void handle_write(int res, core::IoContext &ctx);

private:
  int fd_;
  uint32_t id_; // [NEW]
  std::vector<char> read_buffer_;
  core::Topology &topology_;
  size_t core_id_;
  RespParser parser_;
  std::function<void(uint32_t)> on_disconnect_; // [NEW]

  // Helper to execute parsed command
  std::string execute_command(const std::vector<std::string> &args);
};

} // namespace network
} // namespace quine
