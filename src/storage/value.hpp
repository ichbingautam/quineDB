#pragma once

#include <deque>
#include <map>
#include <set>
#include <string>
#include <variant>
#include <vector>

namespace quine {
namespace storage {

// Data Structures mirroring ZephyraDB functionality
using String = std::string;
using List = std::deque<std::string>;
using Set = std::set<std::string>;
using Hash = std::map<std::string, std::string>;

// Score-Value pair for Sorted Sets
struct ZSetEntry {
  double score;
  std::string member;

  bool operator<(const ZSetEntry &other) const {
    if (score != other.score)
      return score < other.score;
    return member < other.member;
  }
};
using ZSet = std::set<ZSetEntry>;

// Polymorphic container
using Value = std::variant<std::monostate, // Empty/Null
                           String, List, Set, Hash, ZSet>;

enum class ValueType { NONE = 0, STRING, LIST, SET, HASH, ZSET };

inline ValueType get_type(const Value &v) {
  switch (v.index()) {
  case 1:
    return ValueType::STRING;
  case 2:
    return ValueType::LIST;
  case 3:
    return ValueType::SET;
  case 4:
    return ValueType::HASH;
  case 5:
    return ValueType::ZSET;
  default:
    return ValueType::NONE;
  }
}

} // namespace storage
} // namespace quine
