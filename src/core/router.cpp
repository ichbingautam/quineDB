#include "router.hpp"

namespace quine {
namespace core {

// Basic wrapper around std::hash or similar.
static uint32_t hash_key(const std::string &key) {
  // Use FNV-1a or similar simple hash
  uint32_t hash = 2166136261u;
  for (char c : key) {
    hash ^= (uint8_t)c;
    hash *= 16777619;
  }
  return hash;
}

Router::Router(size_t num_shards) : num_shards_(num_shards) {
  initialize_ring(num_shards);
}

void Router::initialize_ring(size_t num_shards) {
  if (num_shards == 0)
    return;
  ring_.clear();
  for (size_t shard_id = 0; shard_id < num_shards; ++shard_id) {
    for (size_t v = 0; v < VIRTUAL_NODES_PER_SHARD; ++v) {
      std::string virtual_node_key =
          "SHARD-" + std::to_string(shard_id) + "-VN-" + std::to_string(v);
      uint32_t hash = hash_key(virtual_node_key);
      ring_[hash] = shard_id;
    }
  }
}

size_t Router::get_shard_id(const std::string &key) const {
  if (num_shards_ == 0)
    return 0;

  if (ring_.empty()) {
    // Fallback
    return crc16(key) % num_shards_;
  }

  uint32_t hash = hash_key(key);
  auto it = ring_.lower_bound(hash);
  if (it == ring_.end()) {
    // Wrap around
    it = ring_.begin();
  }
  return it->second;
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
