#pragma once

#include "value.hpp"
#include <algorithm>
#include <functional>
#include <optional>
#include <stdexcept>
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
    Value value; // [CHANGED]
    bool occupied = false;
    bool deleted = false;
  };

  explicit HashMap(size_t capacity = 1024) : capacity_(capacity), size_(0) {
    entries_.resize(capacity_);
  }

  /// @brief Insert or Update a key-value pair.
  /// @return true if inserted, false if updated
  bool put(std::string_view key, Value value) {
    size_t idx = hash(key);
    size_t start_idx = idx;

    while (entries_[idx].occupied) {
      if (!entries_[idx].deleted && entries_[idx].key == key) {
        // Update existing
        entries_[idx].value = std::move(value);
        return false;
      }
      idx = (idx + 1) % capacity_;
      if (idx == start_idx) {
        throw std::runtime_error("HashMap is full");
      }
    }

    // Insert new
    entries_[idx].key = std::string(key);
    entries_[idx].value = std::move(value);
    entries_[idx].occupied = true;
    entries_[idx].deleted = false;
    size_++;
    return true;
  }

  /// @brief Retrieve a value by key.
  Value *get(std::string_view key) {
    size_t idx = hash(key);
    size_t start_idx = idx;

    while (entries_[idx].occupied) {
      if (!entries_[idx].deleted && entries_[idx].key == key) {
        return &entries_[idx].value;
      }
      idx = (idx + 1) % capacity_;
      if (idx == start_idx)
        return nullptr;
    }
    return nullptr;
  }

  // Const overflow for get
  const Value *get(std::string_view key) const {
    return const_cast<HashMap *>(this)->get(key);
  }

  /// @brief Remove a key.
  bool del(std::string_view key) {
    size_t idx = hash(key);
    size_t start_idx = idx;

    while (entries_[idx].occupied) {
      if (!entries_[idx].deleted && entries_[idx].key == key) {
        entries_[idx].deleted = true;
        entries_[idx].value = std::monostate{}; // Clear memory
        size_--;
        return true;
      }
      idx = (idx + 1) % capacity_;
      if (idx == start_idx)
        return false;
    }
    return false;
  }

  /// @brief Iterate over all valid entries
  template <typename F> void for_each(F callback) const {
    for (const auto &entry : entries_) {
      if (entry.occupied && !entry.deleted) {
        callback(entry.key, entry.value);
      }
    }
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
Dummy change
