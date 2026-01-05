#pragma once

#include <chrono>
#include <optional>
#include <string_view>
#include <unordered_map>

#include "hash_map.hpp"
#include "value.hpp"

namespace quine {
namespace storage {

/// @brief Represents a thread-local partition of the database.
/// Wraps a HashMap and provides high-level storage operations.
class Shard {
 public:
  Shard();

  void set(std::string_view key, Value value);
  Value* get(std::string_view key);
  const Value* get(std::string_view key) const;
  bool del(std::string_view key);

  template <typename F>
  void for_each(F callback) const {
    data_store_.for_each(callback);
  }

  // Expiration support
  void set_expiry(std::string_view key, long long milliseconds_timestamp);
  long long get_expiry(std::string_view key) const;  // Returns timestamp or -1

 private:
  HashMap data_store_;
  // Stores absolute timestamp in milliseconds for expiration
  std::unordered_map<std::string, long long> expires_;
};

}  // namespace storage
}  // namespace quine
