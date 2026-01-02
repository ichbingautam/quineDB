#pragma once

#include "hash_map.hpp"
#include <optional>
#include <string_view>

namespace quine {
namespace storage {

/// @brief Represents a thread-local partition of the database.
/// Wraps a HashMap and provides high-level storage operations.
class Shard {
public:
  Shard() : data_store_(10000) {} // Default small capacity for testing

  void set(std::string_view key, std::string_view value) {
    data_store_.put(key, value);
  }

  std::optional<std::string> get(std::string_view key) const {
    return data_store_.get(key);
  }

  bool del(std::string_view key) { return data_store_.del(key); }

private:
  HashMap data_store_;
};

} // namespace storage
} // namespace quine
