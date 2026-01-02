#pragma once

#include "../storage/shard.hpp"
#include "resp_parser.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace quine {
namespace network {

/// @brief Represents a single client TCP connection.
/// Manages the file descriptor and the read/write data buffers.
class Connection {
public:
  explicit Connection(int fd, storage::Shard *shard);
  ~Connection();

  Connection(const Connection &) = delete;
  Connection &operator=(const Connection &) = delete;

  int get_fd() const { return fd_; }

  // Buffer management
  void resize_buffer(size_t size);

  // Process incoming data
  // Returns response bytes to write back
  std::vector<char> handle_data(const char *data, size_t len);

private:
  int fd_;
  std::vector<char> read_buffer_;
  storage::Shard *shard_;
  RespParser parser_;

  // Helper to execute parsed command
  std::string execute_command(const std::vector<std::string> &args);
};

} // namespace network
} // namespace quine
