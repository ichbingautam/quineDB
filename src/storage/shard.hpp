#pragma once

#include "hash_map.hpp"
#include "value.hpp"
#include <optional>
#include <string_view>

namespace quine {
namespace storage {

/// @brief Represents a thread-local partition of the database.
/// Wraps a HashMap and provides high-level storage operations.
class Shard {
public:
  Shard();

  void set(std::string_view key, Value value);
  Value *get(std::string_view key);
  const Value *get(std::string_view key) const;
  bool del(std::string_view key);

private:
  HashMap data_store_;
};

} // namespace storage
} // namespace quine
