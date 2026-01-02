#include "connection.hpp"
#include <cctype> // for std::toupper
#include <cstring>
#include <iostream>
#include <unistd.h>

namespace quine {
namespace network {

// --- Nested Operation Definitions ---

struct Connection::ReadOp : public core::Operation {
  Connection *conn;
  core::IoContext &ctx;
  explicit ReadOp(Connection *c, core::IoContext &io) : conn(c), ctx(io) {}
  void complete(int res) override { conn->handle_read(res, ctx); }
};

struct Connection::WriteOp : public core::Operation {
  Connection *conn;
  core::IoContext &ctx;
  explicit WriteOp(Connection *c, core::IoContext &io) : conn(c), ctx(io) {}
  void complete(int res) override { conn->handle_write(res, ctx); }
};

// --- Connection Implementation ---

Connection::Connection(int fd, core::Topology &topology, size_t core_id)
    : fd_(fd), topology_(topology), core_id_(core_id) {
  // Pre-allocate decent buffer
  read_buffer_.reserve(4096);
}

Connection::~Connection() {
  if (fd_ >= 0) {
    close(fd_);
  }
}

void Connection::resize_buffer(size_t size) { read_buffer_.resize(size); }

void Connection::start(core::IoContext &ctx) {
  read_op_ = std::make_unique<ReadOp>(this, ctx);
  // write_op_ lazily created or pooled if needed

  submit_read(ctx);
}

void Connection::submit_read(core::IoContext &ctx) {
  struct io_uring_sqe *sqe = ctx.get_sqe();
  io_uring_prep_read(sqe, fd_, read_buffer_.data(), read_buffer_.capacity(), 0);
  io_uring_sqe_set_data(sqe, read_op_.get());
}

void Connection::submit_write(core::IoContext &ctx, std::vector<char> data) {
  // TODO: Implement proper async write with queuing.
  // For V0, we might just write synchronously or fire-and-forget if possible,
  // but io_uring write is preferred.
  // Since we don't have a WriteOp ready to manage the lifetime of 'data',
  // we will stick to synchronous write for the response to ensure correctness
  // for now.

  ::write(fd_, data.data(), data.size());
}

void Connection::handle_read(int res, core::IoContext &ctx) {
  if (res <= 0) {
    // EOF or Error
    // std::cerr << "Connection " << fd_ << " closed or error: " << res <<
    // std::endl; In a real server, we would notify the server to destroy this
    // connection. For now, we effectively leak/stop processing.
    return;
  }

  // Process data
  auto response = handle_data(read_buffer_.data(), res);

  if (!response.empty()) {
    submit_write(ctx, std::move(response));
  }

  // Re-submit read to keep listening
  submit_read(ctx);
}

void Connection::handle_write(int res, core::IoContext &ctx) {
  // Check write result
  if (res < 0) {
    std::cerr << "Write error: " << res << std::endl;
  }
}

std::vector<char> Connection::handle_data(const char *data, size_t len) {
  size_t consumed = 0;
  auto result =
      parser_.consume(reinterpret_cast<const uint8_t *>(data), len, consumed);

  std::vector<char> response;

  if (result == RespParser::Result::Complete) {
    // Execute
    std::string resp_str = execute_command(parser_.get_args());
    // Reset for next command
    parser_.reset();

    // Convert to char vector
    response.assign(resp_str.begin(), resp_str.end());
  } else if (result == RespParser::Result::Error) {
    std::string err = "-ERR Protocol Error\r\n";
    response.assign(err.begin(), err.end());
    parser_.reset();
  }

  return response;
}

std::string Connection::execute_command(const std::vector<std::string> &args) {
  if (args.empty())
    return "-ERR empty command\r\n";

  std::string cmd = args[0];
  // poor man's to_upper
  for (auto &c : cmd)
    c = std::toupper(c);

  if (cmd == "SET") {
    if (args.size() != 3)
      return "-ERR wrong number of arguments for 'set'\r\n";

    // Check routing
    if (topology_.is_local(core_id_, args[1])) {
      topology_.get_shard(core_id_)->set(args[1], args[2]);
      return "+OK\r\n";
    } else {
      // TODO: Implement ITC forwarding
      return "-ERR redirection not implemented yet\r\n";
    }
  } else if (cmd == "GET") {
    if (args.size() != 2)
      return "-ERR wrong number of arguments for 'get'\r\n";

    if (topology_.is_local(core_id_, args[1])) {
      auto val = topology_.get_shard(core_id_)->get(args[1]);
      if (val) {
        return "$" + std::to_string(val->size()) + "\r\n" + *val + "\r\n";
      } else {
        return "$-1\r\n"; // Null bulk string
      }
    } else {
      // TODO: Implement ITC forwarding
      return "-ERR redirection not implemented yet\r\n";
    }
  } else if (cmd == "DEL") {
    if (args.size() != 2)
      return "-ERR wrong number of arguments for 'del'\r\n";

    if (topology_.is_local(core_id_, args[1])) {
      bool deleted = topology_.get_shard(core_id_)->del(args[1]);
      return ":" + std::to_string(deleted ? 1 : 0) + "\r\n";
    } else {
      return "-ERR redirection not implemented yet\r\n";
    }
  } else if (cmd == "PING") {
    return "+PONG\r\n";
  }

  return "-ERR unknown command '" + cmd + "'\r\n";
}

} // namespace network
} // namespace quine
