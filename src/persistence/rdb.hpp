#pragma once

#include <cstdint>
#include <string>

namespace quine {
namespace persistence {

static const char *RDB_MAGIC = "QUINEDB";
static const uint32_t RDB_VERSION = 1;

enum class RdbType : uint8_t {
  STRING = 0,
  LIST = 1,
  SET = 2,
  HASH = 3,
  ZSET = 4,
  EXPIRE_MS = 252,
  END_OF_FILE = 255
};

} // namespace persistence
} // namespace quine
