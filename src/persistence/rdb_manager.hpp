#pragma once

#include "../core/topology.hpp"
#include "../storage/shard.hpp"
#include "../storage/value.hpp"
#include "rdb.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>

namespace quine {
namespace persistence {

class RdbManager {
public:
  static bool save(const core::Topology &topology,
                   const std::string &filename) {
    std::ofstream ofs(filename, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open())
      return false;

    // Header
    ofs.write(RDB_MAGIC, 7);
    ofs.write(reinterpret_cast<const char *>(&RDB_VERSION),
              sizeof(RDB_VERSION));

    // Iterate over all shards
    for (size_t i = 0; i < topology.shard_count(); ++i) {
      auto *shard = topology.get_shard(i);
      // We assume save is called when system is quiescent or lock-safe.
      // In strict thread-per-core, accessing other core's shard is UNSAFE
      // unless paused. For V1, we will assume this is triggered synchronously
      // or we just do it unsafely for now (NOT PRODUCTION READY). Proper way:
      // Send SAVE signal to all cores, they snapshot their data, then merge or
      // write to separate files. Or use a global lock that pauses all workers.
      // Given shared-nothing, 'save' should conceptually be distributed.
      // BUT, for simplicity in this task, we assume non-concurrent access or
      // 'good enough' for demo.

      shard->for_each([&](const std::string &key, const storage::Value &val) {
        // Write expiry first if present (Redis standard puts EXPIRE before
        // key/value) Here, to simplify, we can put it as a separate entry type
        // associated with key BUT strict Redis RDB usually prefixes the value
        // type with expiry opcode. Let's adopt: [OPCODE_EXPIRE_MS] [timestamp]
        // [OPCODE_TYPE] [key] [value]
        long long expiry = shard->get_expiry(key);
        if (expiry != -1) {
          uint8_t expire_opcode = static_cast<uint8_t>(RdbType::EXPIRE_MS);
          ofs.write(reinterpret_cast<const char *>(&expire_opcode), 1);
          ofs.write(reinterpret_cast<const char *>(&expiry), sizeof(expiry));
        }

        write_entry(ofs, key, val);
      });
    }

    // EOF
    uint8_t type = static_cast<uint8_t>(RdbType::END_OF_FILE);
    ofs.write(reinterpret_cast<const char *>(&type), 1);

    return true;
  }

  static bool load(core::Topology &topology, const std::string &filename) {
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs.is_open())
      return false;

    char magic[8] = {0};
    ifs.read(magic, 7);
    if (std::string(magic) != RDB_MAGIC)
      return false;

    uint32_t version;
    ifs.read(reinterpret_cast<char *>(&version), sizeof(version));
    if (version != RDB_VERSION)
      return false;

    while (ifs.peek() != EOF) {
      uint8_t type_byte;
      if (!ifs.read(reinterpret_cast<char *>(&type_byte), 1))
        break;

      if (static_cast<RdbType>(type_byte) == RdbType::END_OF_FILE)
        break;

      std::string key = read_string(ifs);
      storage::Value val;

      switch (static_cast<RdbType>(type_byte)) {
      case RdbType::EXPIRE_MS:
        long long expiry;
        ifs.read(reinterpret_cast<char *>(&expiry), sizeof(expiry));
        // After expire, the next byte MUST be value type
        ifs.read(reinterpret_cast<char *>(&type_byte), 1);
        key = read_string(ifs);

        // Determine value type
        switch (static_cast<RdbType>(type_byte)) {
        case RdbType::STRING:
          val = read_string(ifs);
          break;
        case RdbType::LIST:
          val = read_list(ifs);
          break;
        case RdbType::SET:
          val = read_set(ifs);
          break;
        case RdbType::HASH:
          val = read_hash(ifs);
          break;
        case RdbType::ZSET:
          val = read_zset(ifs);
          break;
        default:
          return false;
        }

        {
          size_t target_core = topology.get_target_core(key);
          auto *shard = topology.get_shard(target_core);
          shard->set(key, std::move(val));
          shard->set_expiry(key, expiry);
        }
        continue; // Skip the standard insert below

      case RdbType::STRING:
        val = read_string(ifs);
        break;
      case RdbType::LIST:
        val = read_list(ifs);
        break;
      case RdbType::SET:
        val = read_set(ifs);
        break;
      case RdbType::HASH:
        val = read_hash(ifs);
        break;
      case RdbType::ZSET:
        val = read_zset(ifs);
        break;
      default:
        return false; // Corrupt
      }

      // Route key to correct shard
      // Since we are loading, we can directly insert into the shard.
      // The Router logic is stateless/static so we can use existing method or
      // just replicate logic. Topology::get_target_core requires hashing.
      size_t target_core = topology.get_target_core(key);
      topology.get_shard(target_core)->set(key, std::move(val));
    }

    return true;
  }

private:
  static void write_string(std::ofstream &ofs, const std::string &s) {
    uint32_t len = s.size();
    ofs.write(reinterpret_cast<const char *>(&len), sizeof(len));
    ofs.write(s.data(), len);
  }

  static std::string read_string(std::ifstream &ifs) {
    uint32_t len;
    ifs.read(reinterpret_cast<char *>(&len), sizeof(len));
    std::string s(len, '\0');
    ifs.read(s.data(), len);
    return s;
  }

  static void write_entry(std::ofstream &ofs, const std::string &key,
                          const storage::Value &val) {
    using namespace storage;
    if (std::holds_alternative<String>(val)) {
      uint8_t type = static_cast<uint8_t>(RdbType::STRING);
      ofs.write(reinterpret_cast<const char *>(&type), 1);
      write_string(ofs, key);
      write_string(ofs, std::get<String>(val));
    } else if (std::holds_alternative<List>(val)) {
      uint8_t type = static_cast<uint8_t>(RdbType::LIST);
      ofs.write(reinterpret_cast<const char *>(&type), 1);
      write_string(ofs, key);
      const auto &list = std::get<List>(val);
      uint32_t count = list.size();
      ofs.write(reinterpret_cast<const char *>(&count), sizeof(count));
      for (const auto &item : list)
        write_string(ofs, item);
    } else if (std::holds_alternative<Set>(val)) {
      uint8_t type = static_cast<uint8_t>(RdbType::SET);
      ofs.write(reinterpret_cast<const char *>(&type), 1);
      write_string(ofs, key);
      const auto &set = std::get<Set>(val);
      uint32_t count = set.size();
      ofs.write(reinterpret_cast<const char *>(&count), sizeof(count));
      for (const auto &item : set)
        write_string(ofs, item);
    } else if (std::holds_alternative<Hash>(val)) {
      uint8_t type = static_cast<uint8_t>(RdbType::HASH);
      ofs.write(reinterpret_cast<const char *>(&type), 1);
      write_string(ofs, key);
      const auto &hash = std::get<Hash>(val);
      uint32_t count = hash.size();
      ofs.write(reinterpret_cast<const char *>(&count), sizeof(count));
      for (const auto &pair : hash) {
        write_string(ofs, pair.first);
        write_string(ofs, pair.second);
      }
    } else if (std::holds_alternative<ZSet>(val)) {
      uint8_t type = static_cast<uint8_t>(RdbType::ZSET);
      ofs.write(reinterpret_cast<const char *>(&type), 1);
      write_string(ofs, key);
      const auto &zset = std::get<ZSet>(val);
      uint32_t count = zset.size();
      ofs.write(reinterpret_cast<const char *>(&count), sizeof(count));
      for (const auto &entry : zset) {
        ofs.write(reinterpret_cast<const char *>(&entry.score), sizeof(double));
        write_string(ofs, entry.member);
      }
    }
  }

  static storage::List read_list(std::ifstream &ifs) {
    storage::List list;
    uint32_t count;
    ifs.read(reinterpret_cast<char *>(&count), sizeof(count));
    for (uint32_t i = 0; i < count; ++i) {
      list.push_back(read_string(ifs));
    }
    return list;
  }

  static storage::Set read_set(std::ifstream &ifs) {
    storage::Set set;
    uint32_t count;
    ifs.read(reinterpret_cast<char *>(&count), sizeof(count));
    for (uint32_t i = 0; i < count; ++i) {
      set.insert(read_string(ifs));
    }
    return set;
  }

  static storage::Hash read_hash(std::ifstream &ifs) {
    storage::Hash hash;
    uint32_t count;
    ifs.read(reinterpret_cast<char *>(&count), sizeof(count));
    for (uint32_t i = 0; i < count; ++i) {
      std::string field = read_string(ifs);
      std::string val = read_string(ifs);
      hash[field] = val;
    }
    return hash;
  }

  static storage::ZSet read_zset(std::ifstream &ifs) {
    storage::ZSet zset;
    uint32_t count;
    ifs.read(reinterpret_cast<char *>(&count), sizeof(count));
    for (uint32_t i = 0; i < count; ++i) {
      double score;
      ifs.read(reinterpret_cast<char *>(&score), sizeof(double));
      std::string member = read_string(ifs);
      zset.insert({score, member});
    }
    return zset;
  }
};

} // namespace persistence
} // namespace quine
