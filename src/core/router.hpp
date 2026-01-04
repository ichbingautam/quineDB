#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace quine {
namespace core {

/// @brief Handles key-to-shard mapping (Routing).
/// Currently supports static partitioning based on modulo hashing.
/// Flexible enough to be extended to Consistent Hashing (Ring) later.
class Router {
public:
  /// @brief Initialize the router with the number of shards (cores).
  /// @param num_shards Total number of available shards.
  explicit Router(size_t num_shards);

  /// @brief Determines which shard owns the given key.
  /// @param key The key to look up.
  /// @return The Shard ID (0 to num_shards - 1).
  size_t get_shard_id(const std::string &key) const;

  /// @brief Calculates CRC16 hash of a string.
  /// Used internally but exposed for testing/debug.
  static uint16_t crc16(const std::string &key);

private:
  // Maps Hash -> ShardID
  std::map<uint32_t, size_t> ring_;

  // Number of virtual nodes per shard
  static const size_t VIRTUAL_NODES_PER_SHARD = 100;

  size_t num_shards_; // Kept for fallback or reloading

  void initialize_ring(size_t num_shards);
};

} // namespace core
} // namespace quine
