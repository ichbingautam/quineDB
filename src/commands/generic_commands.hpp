#pragma once

#include "../core/command.hpp"
#include "../core/message.hpp"
#include "../core/topology.hpp"
#include <chrono>

namespace quine {
namespace commands {

class ExpireCommand : public core::Command {
public:
  std::string name() const override { return "EXPIRE"; }

  std::string execute(quine::core::Topology &topology, size_t core_id,
                      uint32_t conn_id,
                      const std::vector<std::string> &args) override {
    if (args.size() != 3)
      return "-ERR wrong number of arguments for 'expire'\r\n";

    // EXPIRE key seconds
    const std::string &key = args[1];
    long seconds = 0;
    try {
      seconds = std::stol(args[2]);
    } catch (...) {
      return "-ERR value is not an integer or out of range\r\n";
    }

    if (topology.is_local(core_id, key)) {
      auto *shard = topology.get_shard(core_id);
      // We must check if key exists first!
      // Note: shard->get() also checks expiry, so if it returns nullptr, it's
      // already expired or missing.
      auto *val = shard->get(key);
      if (!val) {
        return ":0\r\n";
      }

      auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::system_clock::now().time_since_epoch())
                     .count();
      long long expiry = now + (seconds * 1000);
      shard->set_expiry(key, expiry);
      return ":1\r\n";
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

class TtlCommand : public core::Command {
public:
  std::string name() const override { return "TTL"; }

  std::string execute(quine::core::Topology &topology, size_t core_id,
                      uint32_t conn_id,
                      const std::vector<std::string> &args) override {
    if (args.size() != 2)
      return "-ERR wrong number of arguments for 'ttl'\r\n";

    const std::string &key = args[1];

    if (topology.is_local(core_id, key)) {
      auto *shard = topology.get_shard(core_id);

      // Use get() to filter out already expired keys
      auto *val = shard->get(key);
      if (!val) {
        return ":-2\r\n"; // Key does not exist
      }

      long long expiry = shard->get_expiry(key);
      if (expiry == -1) {
        return ":-1\r\n"; // No expiry
      }

      auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::system_clock::now().time_since_epoch())
                     .count();

      long long diff = expiry - now;
      if (diff < 0) {
        // Should have been caught by get(), but race is possible?
        // Or get() cleaned it up. access flow: get() -> clean -> return
        // nullptr.
        return ":-2\r\n";
      }

      return ":" + std::to_string(diff / 1000) + "\r\n";

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
