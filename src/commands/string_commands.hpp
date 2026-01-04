#pragma once

#include "../core/command.hpp"
#include "../core/message.hpp"
#include "../storage/value.hpp"
#include <iostream>

namespace quine {
namespace commands {

class SetCommand : public core::Command {
public:
  std::string name() const override { return "SET"; }

  std::string execute(core::Topology &topology, size_t core_id,
                      uint32_t conn_id,
                      const std::vector<std::string> &args) override {
    if (args.size() != 3)
      return "-ERR wrong number of arguments for 'set'\r\n";

    if (topology.is_local(core_id, args[1])) {
      // Construct Value variant from string
      storage::Value val = std::string(args[2]);
      topology.get_shard(core_id)->set(args[1], std::move(val));
      return "+OK\r\n";
    } else {
      // Forwarding
      size_t target_core = topology.get_target_core(args[1]);
      core::Message msg;
      msg.type = core::MessageType::REQUEST;
      msg.origin_core_id = core_id;
      msg.conn_id = conn_id;
      msg.key = args[1];
      msg.args = args;

      topology.get_channel(target_core)->push(msg);
      topology.notify_core(target_core);

      return ""; // Async response
    }
  }
};

class GetCommand : public core::Command {
public:
  std::string name() const override { return "GET"; }

  std::string execute(core::Topology &topology, size_t core_id,
                      uint32_t conn_id,
                      const std::vector<std::string> &args) override {
    if (args.size() != 2)
      return "-ERR wrong number of arguments for 'get'\r\n";

    if (topology.is_local(core_id, args[1])) {
      storage::Value *val = topology.get_shard(core_id)->get(args[1]);
      if (val) {
        if (auto str_val = std::get_if<std::string>(val)) {
          return "$" + std::to_string(str_val->size()) + "\r\n" + *str_val +
                 "\r\n";
        } else {
          return "-ERR WRONGTYPE Operation against a key holding the wrong "
                 "kind of value\r\n";
        }
      } else {
        return "$-1\r\n";
      }
    } else {
      // Forwarding
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
