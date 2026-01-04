#pragma once

#include "../core/command.hpp"
#include "../core/message.hpp"
#include "../core/topology.hpp"
#include "../storage/value.hpp"
#include <algorithm>
#include <string>
#include <vector>

namespace quine {
namespace commands {

class ZAddCommand : public core::Command {
public:
  std::string name() const override { return "ZADD"; }

  std::string execute(quine::core::Topology &topology, size_t core_id,
                      uint32_t conn_id,
                      const std::vector<std::string> &args) override {
    // ZADD key score member [score member ...]
    if (args.size() < 4 || (args.size() % 2 != 0))
      return "-ERR wrong number of arguments for 'zadd'\r\n";

    const std::string &key = args[1];

    if (topology.is_local(core_id, key)) {
      auto *shard = topology.get_shard(core_id);
      storage::Value *val = shard->get(key);
      storage::ZSet *zset_ptr = nullptr;

      if (!val) {
        shard->set(key, storage::ZSet{});
        val = shard->get(key);
        zset_ptr = std::get_if<storage::ZSet>(val);
      } else {
        zset_ptr = std::get_if<storage::ZSet>(val);
        if (!zset_ptr) {
          return "-ERR WRONGTYPE Operation against a key holding the wrong "
                 "kind of value\r\n";
        }
      }

      int added = 0;
      for (size_t i = 2; i < args.size(); i += 2) {
        double score;
        try {
          score = std::stod(args[i]);
        } catch (...) {
          return "-ERR value is not a valid float\r\n";
        }
        const std::string &member = args[i + 1];

        // Optimized lookup using internal dictionary
        auto it = zset_ptr->dict.find(member);
        if (it != zset_ptr->dict.end()) {
          if (it->second != score) {
            // Update existing
            zset_ptr->insert({score, member});
          }
        } else {
          // New insert
          zset_ptr->insert({score, member});
          added++;
        }
      }
      return ":" + std::to_string(added) + "\r\n";

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

class ZRangeCommand : public core::Command {
public:
  std::string name() const override { return "ZRANGE"; }

  std::string execute(quine::core::Topology &topology, size_t core_id,
                      uint32_t conn_id,
                      const std::vector<std::string> &args) override {
    if (args.size() < 4)
      return "-ERR wrong number of arguments for 'zrange'\r\n";

    const std::string &key = args[1];

    if (topology.is_local(core_id, key)) {
      auto *shard = topology.get_shard(core_id);
      storage::Value *val = shard->get(key);

      if (!val)
        return "*0\r\n";

      auto *zset_ptr = std::get_if<storage::ZSet>(val);
      if (!zset_ptr)
        return "-ERR WRONGTYPE Operation against a key holding the wrong kind "
               "of value\r\n";

      try {
        int start = std::stoi(args[2]);
        int stop = std::stoi(args[3]);
        int size = static_cast<int>(zset_ptr->size());

        if (start < 0)
          start = size + start;
        if (stop < 0)
          stop = size + stop;
        if (start < 0)
          start = 0;
        if (stop < 0)
          stop = 0;
        if (start >= size)
          return "*0\r\n";
        if (stop >= size)
          stop = size - 1;
        if (start > stop)
          return "*0\r\n";

        bool withscores = false;
        if (args.size() > 4) {
          std::string opt = args[4];
          std::transform(opt.begin(), opt.end(), opt.begin(), ::toupper);
          if (opt == "WITHSCORES")
            withscores = true;
        }

        std::string resp =
            "*" + std::to_string((stop - start + 1) * (withscores ? 2 : 1)) +
            "\r\n";

        auto it = zset_ptr->begin();
        std::advance(it, start);
        for (int i = start; i <= stop; ++i) {
          const auto &entry = *it;
          resp += "$" + std::to_string(entry.member.size()) + "\r\n" +
                  entry.member + "\r\n";
          if (withscores) {
            // Clean formatting for float? std::to_string gives trailing zeros.
            // Redis removes trailing zeros usually.
            std::string score_str = std::to_string(entry.score);
            // Strip trailing zeros
            score_str.erase(score_str.find_last_not_of('0') + 1,
                            std::string::npos);
            if (score_str.back() == '.')
              score_str.pop_back();

            resp += "$" + std::to_string(score_str.size()) + "\r\n" +
                    score_str + "\r\n";
          }
          it++;
        }
        return resp;

      } catch (...) {
        return "-ERR value is not an integer or out of range\r\n";
      }

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

class ZRemCommand : public core::Command {
public:
  std::string name() const override { return "ZREM"; }

  std::string execute(quine::core::Topology &topology, size_t core_id,
                      uint32_t conn_id,
                      const std::vector<std::string> &args) override {
    if (args.size() < 3)
      return "-ERR wrong number of arguments for 'zrem'\r\n";

    const std::string &key = args[1];

    if (topology.is_local(core_id, key)) {
      auto *shard = topology.get_shard(core_id);
      storage::Value *val = shard->get(key);

      if (!val)
        return ":0\r\n";

      auto *zset_ptr = std::get_if<storage::ZSet>(val);
      if (!zset_ptr)
        return "-ERR WRONGTYPE Operation against a key holding the wrong kind "
               "of value\r\n";

      int removed = 0;
      for (size_t i = 2; i < args.size(); ++i) {
        if (zset_ptr->erase(args[i])) {
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

class ZCardCommand : public core::Command {
public:
  std::string name() const override { return "ZCARD"; }

  std::string execute(quine::core::Topology &topology, size_t core_id,
                      uint32_t conn_id,
                      const std::vector<std::string> &args) override {
    if (args.size() != 2)
      return "-ERR wrong number of arguments for 'zcard'\r\n";

    const std::string &key = args[1];

    if (topology.is_local(core_id, key)) {
      auto *shard = topology.get_shard(core_id);
      storage::Value *val = shard->get(key);

      if (!val)
        return ":0\r\n";

      auto *zset_ptr = std::get_if<storage::ZSet>(val);
      if (!zset_ptr)
        return "-ERR WRONGTYPE Operation against a key holding the wrong kind "
               "of value\r\n";

      return ":" + std::to_string(zset_ptr->size()) + "\r\n";

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

class ZScoreCommand : public core::Command {
public:
  std::string name() const override { return "ZSCORE"; }

  std::string execute(quine::core::Topology &topology, size_t core_id,
                      uint32_t conn_id,
                      const std::vector<std::string> &args) override {
    if (args.size() != 3)
      return "-ERR wrong number of arguments for 'zscore'\r\n";

    const std::string &key = args[1];
    const std::string &member = args[2];

    if (topology.is_local(core_id, key)) {
      auto *shard = topology.get_shard(core_id);
      storage::Value *val = shard->get(key);

      if (!val)
        return "$-1\r\n";

      auto *zset_ptr = std::get_if<storage::ZSet>(val);
      if (!zset_ptr)
        return "-ERR WRONGTYPE Operation against a key holding the wrong kind "
               "of value\r\n";

      auto it = zset_ptr->dict.find(member);
      if (it == zset_ptr->dict.end()) {
        return "$-1\r\n";
      }

      std::string score_str = std::to_string(it->second);
      // Strip trailing zeros
      score_str.erase(score_str.find_last_not_of('0') + 1, std::string::npos);
      if (score_str.back() == '.')
        score_str.pop_back();

      return "$" + std::to_string(score_str.size()) + "\r\n" + score_str +
             "\r\n";

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
