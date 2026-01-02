#include "router.hpp"

namespace quine {
namespace core {

Router::Router(size_t num_shards) : num_shards_(num_shards) {}

size_t Router::get_shard_id(const std::string &key) const {
  if (num_shards_ == 0)
    return 0;

  // Redis-style sharding (CRC16 % N)
  // This is simple and effective for fixed-size clusters (or cores).
  // For distributed nodes, we would use a Ring (Consistent Hashing).
  uint16_t hash = crc16(key);
  return hash % num_shards_;
}

// Standard CRC16 implementation (XMODEM)
uint16_t Router::crc16(const std::string &key) {
  uint16_t crc = 0;
  for (char c : key) {
    crc = crc ^ ((uint16_t)c << 8);
    for (int i = 0; i < 8; i++) {
      if (crc & 0x8000)
        crc = (crc << 1) ^ 0x1021;
      else
        crc = crc << 1;
    }
  }
  return crc;
}

} // namespace core
} // namespace quine
