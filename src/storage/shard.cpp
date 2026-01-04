#include "shard.hpp"

namespace quine {
namespace storage {

Shard::Shard() : data_store_(10000) {}

void Shard::set(std::string_view key, Value value) {
  data_store_.put(key, std::move(value));
}

Value *Shard::get(std::string_view key) { return data_store_.get(key); }

const Value *Shard::get(std::string_view key) const {
  return data_store_.get(key);
}

bool Shard::del(std::string_view key) { return data_store_.del(key); }

} // namespace storage
} // namespace quine
