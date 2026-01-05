#pragma once

#include <functional>
#include <memory>
#include <string_view>
#include <vector>

#include "shard.hpp"

namespace quine {
namespace storage {

/// @brief The global container for the storage layer.
/// Manages N shards, where N is typically the number of CPU cores.
class StorageEngine {
 public:
  explicit StorageEngine(size_t num_shards) : num_shards_(num_shards) {
    shards_.reserve(num_shards_);
    for (size_t i = 0; i < num_shards_; ++i) {
      shards_.emplace_back(std::make_unique<Shard>());
    }
  }

  /// @brief Get the specific shard responsible for a key.
  /// This ensures we route the request to the correct thread's data.
  Shard& get_shard(std::string_view key) {
    size_t shard_id = std::hash<std::string_view>{}(key) % num_shards_;
    return *shards_[shard_id];
  }

  /// @brief Direct access to a shard by ID (for Thread-per-Core
  /// initialization).
  Shard& get_shard_by_id(size_t id) {
    return *shards_[id];
  }

 private:
  size_t num_shards_;
  // We use unique_ptr to ensure pointer stability if vector resizes (though we
  // reserve)
  std::vector<std::unique_ptr<Shard>> shards_;
};

}  // namespace storage
}  // namespace quine
