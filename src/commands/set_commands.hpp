#pragma once

#include "../core/command.hpp"
#include "../core/message.hpp"
#include "../core/topology.hpp"
#include "../storage/value.hpp"
#include <string>
#include <vector>

namespace quine {
namespace commands {

class SAddCommand : public core::Command {
public:
  std::string name() const override { return "SADD"; }

  std::string execute(quine::core::Topology &topology, size_t core_id,
                      uint32_t conn_id,
                      const std::vector<std::string> &args) override {
    if (args.size() < 3)
      return "-ERR wrong number of arguments for 'sadd'\r\n";

    const std::string &key = args[1];

    if (topology.is_local(core_id, key)) {
      auto *shard = topology.get_shard(core_id);
      storage::Value *val = shard->get(key);
      storage::Set *set_ptr = nullptr;

      if (!val) {
        shard->set(key, storage::Set{});
        val = shard->get(key);
        set_ptr = std::get_if<storage::Set>(val);
      } else {
        set_ptr = std::get_if<storage::Set>(val);
        if (!set_ptr) {
          return "-ERR WRONGTYPE Operation against a key holding the wrong "
                 "kind of value\r\n";
        }
      }

      int added = 0;
      for (size_t i = 2; i < args.size(); ++i) {
        if (set_ptr->insert(args[i]).second) {
          added++;
        }
      }
      return ":" + std::to_string(added) + "\r\n";

    } else {
      // Forwarding... (simplified duplicate logic, ideally should be shared)
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

class SMembersCommand : public core::Command {
public:
  std::string name() const override { return "SMEMBERS"; }

  std::string execute(quine::core::Topology &topology, size_t core_id,
                      uint32_t conn_id,
                      const std::vector<std::string> &args) override {
    if (args.size() != 2)
      return "-ERR wrong number of arguments for 'smembers'\r\n";

    const std::string &key = args[1];

    if (topology.is_local(core_id, key)) {
      auto *shard = topology.get_shard(core_id);
      storage::Value *val = shard->get(key);

      if (!val) {
        return "*0\r\n";
      }

      auto *set_ptr = std::get_if<storage::Set>(val);
      if (!set_ptr) {
        return "-ERR WRONGTYPE Operation against a key holding the wrong kind "
               "of value\r\n";
      }

      std::string resp = "*" + std::to_string(set_ptr->size()) + "\r\n";
      for (const auto &member : *set_ptr) {
        resp += "$" + std::to_string(member.size()) + "\r\n" + member + "\r\n";
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

class SRemCommand : public core::Command {
public:
  std::string name() const override { return "SREM"; }

  std::string execute(quine::core::Topology &topology, size_t core_id,
                      uint32_t conn_id,
                      const std::vector<std::string> &args) override {
    if (args.size() < 3)
      return "-ERR wrong number of arguments for 'srem'\r\n";

    const std::string &key = args[1];

    if (topology.is_local(core_id, key)) {
      auto *shard = topology.get_shard(core_id);
      storage::Value *val = shard->get(key);

      if (!val) {
        return ":0\r\n";
      }

      auto *set_ptr = std::get_if<storage::Set>(val);
      if (!set_ptr) {
        return "-ERR WRONGTYPE Operation against a key holding the wrong kind "
               "of value\r\n";
      }

      int removed = 0;
      for (size_t i = 2; i < args.size(); ++i) {
        if (set_ptr->erase(args[i])) {
          removed++;
        }
      }
      return ":" + std::to_string(removed) + "\r\n";

    } else {
      return forward_request(topology, core_id, conn_id, args);
    }
  }

private:
  std::string forward_request(core::Topology &topology, size_t core_id,
                              uint32_t conn_id,
                              const std::vector<std::string> &args) {
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
};

class SCardCommand : public core::Command {
public:
  std::string name() const override { return "SCARD"; }

  std::string execute(quine::core::Topology &topology, size_t core_id,
                      uint32_t conn_id,
                      const std::vector<std::string> &args) override {
    if (args.size() != 2)
      return "-ERR wrong number of arguments for 'scard'\r\n";

    const std::string &key = args[1];

    if (topology.is_local(core_id, key)) {
      auto *shard = topology.get_shard(core_id);
      storage::Value *val = shard->get(key);

      if (!val) {
        return ":0\r\n";
      }

      auto *set_ptr = std::get_if<storage::Set>(val);
      if (!set_ptr) {
        return "-ERR WRONGTYPE Operation against a key holding the wrong "
               "kind of value\r\n";
      }

      return ":" + std::to_string(set_ptr->size()) + "\r\n";

    } else {
      return forward_request(topology, core_id, conn_id, args);
    }
  }

private:
  std::string forward_request(core::Topology &topology, size_t core_id,
                              uint32_t conn_id,
                              const std::vector<std::string> &args) {
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
};

} // namespace commands
} // namespace quine
