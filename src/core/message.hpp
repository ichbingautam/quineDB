#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace quine {
namespace core {

enum class MessageType { REQUEST, RESPONSE };

struct Message {
  MessageType type;
  size_t origin_core_id;  // [NEW] To route response back to the correct core
  uint32_t conn_id;       // To route response back to the correct connection
  std::string key;
  std::vector<std::string> args;  // For the command (e.g. SET key value)

  // For Response
  std::string payload;
  bool success;
};

}  // namespace core
}  // namespace quine
