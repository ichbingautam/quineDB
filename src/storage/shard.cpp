#include "shard.hpp"

namespace quine {
namespace storage {

Shard::Shard() : data_store_(10000) {}

void Shard::set(std::string_view key, Value value) {
  data_store_.put(key, std::move(value));
  // SET clears any existing expiration
  expires_.erase(std::string(key));
}

Value *Shard::get(std::string_view key) {
  // Check expiration
  auto it = expires_.find(std::string(key));
  if (it != expires_.end()) {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
                   .count();
    if (now > it->second) {
      // Expired
      data_store_.del(key);
      expires_.erase(it);
      return nullptr;
    }
  }
  return data_store_.get(key);
}

const Value *Shard::get(std::string_view key) const {
  // Const get cannot lazy expire, but should simulate it
  // Cast away constness to check expires map? Or just check map?
  // In a read-only context (like RDB save), we might return expired data
  // or we should check expiration and return nullptr.
  // For now, let's just return key if it exists, RDB might save expired keys
  // which is fine (they will expire on load).
  // Actually, let's check validation to be clean.
  auto it = expires_.find(std::string(key));
  if (it != expires_.end()) {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
                   .count();
    if (now > it->second) {
      return nullptr;
    }
  }
  return data_store_.get(key);
}

bool Shard::del(std::string_view key) {
  expires_.erase(std::string(key));
  return data_store_.del(key);
}

void Shard::set_expiry(std::string_view key, long long milliseconds_timestamp) {
  expires_[std::string(key)] = milliseconds_timestamp;
}

long long Shard::get_expiry(std::string_view key) const {
  auto it = expires_.find(std::string(key));
  if (it != expires_.end()) {
    return it->second;
  }
  return -1;
}

} // namespace storage
} // namespace quine
