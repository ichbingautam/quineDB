#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace quine {
namespace network {

/// @brief A simple streaming RESP (Redis Serialization Protocol) parser.
/// Maintains internal state to handle partial reads.
class RespParser {
public:
  enum class State {
    WaitType,    // Expecting '*' (Array)
    WaitSize,    // Expecting array size integer
    WaitArgSize, // Expecting '$' then length of bulk string
    WaitArgData, // Expecting actual string data
    WaitCRLF     // Expecting \r\n after data
  };

  enum class Result {
    Partial,  // Need more data
    Complete, // Command parsed successfully
    Error     // Invalid protocol
  };

  RespParser();

  /// @brief Consume data from a buffer.
  /// @param data Pointer to input data
  /// @param len Length of input data
  /// @param consumed Output: bytes processed
  /// @return Result status
  Result consume(const uint8_t *data, size_t len, size_t &consumed);

  /// @brief Get the parsed command arguments (e.g., {"SET", "key", "val"})
  const std::vector<std::string> &get_args() const { return args_; }

  /// @brief Reset parser for the next command
  void reset();

private:
  State state_;
  std::vector<std::string> args_;

  int expected_args_ = 0;   // Number of items in array (*N)
  int current_arg_len_ = 0; // Length of current bulk string ($N)
  std::string current_arg_; // Accumulating current argument

  // Process a single line (up to \r\n)
  bool process_line(const uint8_t *data, size_t len, size_t &pos);
};

} // namespace network
} // namespace quine
