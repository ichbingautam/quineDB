#pragma once

#include <algorithm>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace quine {
namespace storage {

/// @brief A simple Open Addressing Hash Map with Linear Probing.
/// Designed for high cache locality.
///
/// Current limitation: Does not resize automatically (fixed size for V1).
/// TODO: Implement resizing and Robin Hood hashing.
class HashMap {
public:
  struct Entry {
    std::string key;
    std::string value;
    bool occupied = false;
    bool deleted = false; // Tombstone support for deletions
  };

  explicit HashMap(size_t capacity = 1024) : capacity_(capacity), size_(0) {
    entries_.resize(capacity_);
  }

  /// @brief Insert or Update a key-value pair.
  void put(std::string_view key, std::string_view value) {
    size_t idx = hash(key);
    size_t start_idx = idx;

    while (entries_[idx].occupied) {
      if (!entries_[idx].deleted && entries_[idx].key == key) {
        // Update existing
        entries_[idx].value = std::string(value);
        return;
      }
      idx = (idx + 1) % capacity_;
      if (idx == start_idx) {
        // Full - in a real DB we would resize or evict.
        // For V1 proof-of-concept, we just throw or ignore.
        // Throwing is safer for debugging.
        throw std::runtime_error("HashMap is full");
      }
    }

    // Insert new
    entries_[idx].key = std::string(key);
    entries_[idx].value = std::string(value);
    entries_[idx].occupied = true;
    entries_[idx].deleted = false;
    size_++;
  }

  /// @brief Retrieve a value by key.
  std::optional<std::string> get(std::string_view key) const {
    size_t idx = hash(key);
    size_t start_idx = idx;

    while (entries_[idx].occupied) {
      if (!entries_[idx].deleted && entries_[idx].key == key) {
        return entries_[idx].value;
      }
      idx = (idx + 1) % capacity_;
      if (idx == start_idx)
        return std::nullopt;
    }
    return std::nullopt;
  }

  /// @brief Remove a key.
  bool del(std::string_view key) {
    size_t idx = hash(key);
    size_t start_idx = idx;

    while (entries_[idx].occupied) {
      if (!entries_[idx].deleted && entries_[idx].key == key) {
        entries_[idx].deleted = true; // Mark as tombstone
        // We keep 'occupied' true to maintain probe chain
        size_--;
        return true;
      }
      idx = (idx + 1) % capacity_;
      if (idx == start_idx)
        return false;
    }
    return false;
  }

private:
  std::vector<Entry> entries_;
  size_t capacity_;
  size_t size_;

  size_t hash(std::string_view key) const {
    return std::hash<std::string_view>{}(key) % capacity_;
  }
};

} // namespace storage
} // namespace quine
