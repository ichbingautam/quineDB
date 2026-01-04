#pragma once

#include "../core/command.hpp"
#include "../core/message.hpp"
#include "../core/topology.hpp"
#include "../storage/value.hpp"
#include <string>
#include <vector>

namespace quine {
namespace commands {

class HSetCommand : public core::Command {
public:
  std::string name() const override { return "HSET"; }

  std::string execute(quine::core::Topology &topology, size_t core_id,
                      uint32_t conn_id,
                      const std::vector<std::string> &args) override {
    // HSET key field value [field value ...]
    if (args.size() < 4 || (args.size() % 2 != 0))
      return "-ERR wrong number of arguments for 'hset'\r\n";

    const std::string &key = args[1];

    if (topology.is_local(core_id, key)) {
      auto *shard = topology.get_shard(core_id);
      storage::Value *val = shard->get(key);
      storage::Hash *hash_ptr = nullptr;

      if (!val) {
        shard->set(key, storage::Hash{});
        val = shard->get(key);
        hash_ptr = std::get_if<storage::Hash>(val);
      } else {
        hash_ptr = std::get_if<storage::Hash>(val);
        if (!hash_ptr) {
          return "-ERR WRONGTYPE Operation against a key holding the wrong "
                 "kind of value\r\n";
        }
      }

      int created_fields = 0;
      for (size_t i = 2; i < args.size(); i += 2) {
        const std::string &field = args[i];
        const std::string &value = args[i + 1];
        // std::map::insert returns pair<iterator, bool>, bool is true if
        // inserted using insert_or_assign in C++17 would be better to detect
        // creation vs update But strict generic Redis HSET returns number of
        // fields *added*.
        auto it = hash_ptr->find(field);
        if (it == hash_ptr->end()) {
          created_fields++;
          (*hash_ptr)[field] = value;
        } else {
          (*hash_ptr)[field] = value;
        }
      }
      return ":" + std::to_string(created_fields) + "\r\n";

    } else {
      size_t target_core = topology.get_target_core(args[1]);
      core::Message msg;
      msg.type = core::MessageType::REQUEST;
      msg.origin_core_id = core_id;
      msg.conn_id = conn_id;
      msg.key = args[1];
      msg.args = args;
      topology.get_channel(target_core)->push(msg);
      topology.notify_core(target_core);
      return "";
    }
  }
};

class HGetCommand : public core::Command {
public:
  std::string name() const override { return "HGET"; }

  std::string execute(quine::core::Topology &topology, size_t core_id,
                      uint32_t conn_id,
                      const std::vector<std::string> &args) override {
    if (args.size() != 3)
      return "-ERR wrong number of arguments for 'hget'\r\n";

    const std::string &key = args[1];
    const std::string &field = args[2];

    if (topology.is_local(core_id, key)) {
      auto *shard = topology.get_shard(core_id);
      storage::Value *val = shard->get(key);

      if (!val)
        return "$-1\r\n";

      auto *hash_ptr = std::get_if<storage::Hash>(val);
      if (!hash_ptr)
        return "-ERR WRONGTYPE Operation against a key holding the wrong kind "
               "of value\r\n";

      auto it = hash_ptr->find(field);
      if (it == hash_ptr->end()) {
        return "$-1\r\n";
      }

      return "$" + std::to_string(it->second.size()) + "\r\n" + it->second +
             "\r\n";

    } else {
      size_t target_core = topology.get_target_core(args[1]);
      core::Message msg;
      msg.type = core::MessageType::REQUEST;
      msg.origin_core_id = core_id;
      msg.conn_id = conn_id;
      msg.key = args[1];
      msg.args = args;
      topology.get_channel(target_core)->push(msg);
      topology.notify_core(target_core);
      return "";
    }
  }
};

class HGetAllCommand : public core::Command {
public:
  std::string name() const override { return "HGETALL"; }

  std::string execute(quine::core::Topology &topology, size_t core_id,
                      uint32_t conn_id,
                      const std::vector<std::string> &args) override {
    if (args.size() != 2)
      return "-ERR wrong number of arguments for 'hgetall'\r\n";

    const std::string &key = args[1];

    if (topology.is_local(core_id, key)) {
      auto *shard = topology.get_shard(core_id);
      storage::Value *val = shard->get(key);

      if (!val)
        return "*0\r\n";

      auto *hash_ptr = std::get_if<storage::Hash>(val);
      if (!hash_ptr)
        return "-ERR WRONGTYPE Operation against a key holding the wrong kind "
               "of value\r\n";

      // Result is array of field, value, field, value...
      std::string resp = "*" + std::to_string(hash_ptr->size() * 2) + "\r\n";
      for (const auto &pair : *hash_ptr) {
        resp += "$" + std::to_string(pair.first.size()) + "\r\n" + pair.first +
                "\r\n";
        resp += "$" + std::to_string(pair.second.size()) + "\r\n" +
                pair.second + "\r\n";
      }
      return resp;

    } else {
      size_t target_core = topology.get_target_core(args[1]);
      core::Message msg;
      msg.type = core::MessageType::REQUEST;
      msg.origin_core_id = core_id;
      msg.conn_id = conn_id;
      msg.key = args[1];
      msg.args = args;
      topology.get_channel(target_core)->push(msg);
      topology.notify_core(target_core);
      return "";
    }
  }
};

} // namespace commands
} // namespace quine
