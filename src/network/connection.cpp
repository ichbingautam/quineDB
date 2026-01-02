#include "connection.hpp"
#include <unistd.h>

namespace quine {
namespace network {

Connection::Connection(int fd, storage::Shard *shard) : fd_(fd), shard_(shard) {
  // Pre-allocate decent buffer
  read_buffer_.reserve(4096);
}

Connection::~Connection() {
  if (fd_ >= 0) {
    close(fd_);
  }
}

void Connection::resize_buffer(size_t size) { read_buffer_.resize(size); }

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
    shard_->set(args[1], args[2]);
    return "+OK\r\n";
  } else if (cmd == "GET") {
    if (args.size() != 2)
      return "-ERR wrong number of arguments for 'get'\r\n";
    auto val = shard_->get(args[1]);
    if (val) {
      return "$" + std::to_string(val->size()) + "\r\n" + *val + "\r\n";
    } else {
      return "$-1\r\n"; // Null bulk string
    }
  } else if (cmd == "DEL") {
    if (args.size() != 2)
      return "-ERR wrong number of arguments for 'del'\r\n";
    bool deleted = shard_->del(args[1]);
    return ":" + std::to_string(deleted ? 1 : 0) + "\r\n";
  } else if (cmd == "PING") {
    return "+PONG\r\n";
  }

  return "-ERR unknown command '" + cmd + "'\r\n";
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
    parser_.reset(); // or close connection?
  }

  return response;
}

std::string Connection::execute_command(const std::vector<std::string> &args) {
  if (args.empty())
    return "-ERR empty command\r\n";

  std::string cmd = args[0];
  // poor man's to_upper
  for (auto &c : cmd)
    c = toupper(c);

  if (cmd == "SET") {
    if (args.size() != 3)
      return "-ERR wrong number of arguments for 'set'\r\n";
    shard_->set(args[1], args[2]);
    return "+OK\r\n";
  } else if (cmd == "GET") {
    if (args.size() != 2)
      return "-ERR wrong number of arguments for 'get'\r\n";
    auto val = shard_->get(args[1]);
    if (val) {
      return "$" + std::to_string(val->size()) + "\r\n" + *val + "\r\n";
    } else {
      return "$-1\r\n"; // Null bulk string
    }
  } else if (cmd == "DEL") {
    if (args.size() != 2)
      return "-ERR wrong number of arguments for 'del'\r\n";
    bool deleted = shard_->del(args[1]);
    return ":" + std::to_string(deleted ? 1 : 0) + "\r\n";
  } else if (cmd == "PING") {
    return "+PONG\r\n";
  }

  return "-ERR unknown command '" + cmd + "'\r\n";
}

} // namespace network
} // namespace quine
