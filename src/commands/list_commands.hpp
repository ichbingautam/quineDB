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

class LPushCommand : public core::Command {
public:
  std::string name() const override { return "LPUSH"; }

  std::string execute(core::Topology &topology, size_t core_id,
                      uint32_t conn_id,
                      const std::vector<std::string> &args) override {
    if (args.size() < 3)
      return "-ERR wrong number of arguments for 'lpush'\r\n";

    const std::string &key = args[1];

    if (topology.is_local(core_id, key)) {
      auto *shard = topology.get_shard(core_id);
      storage::Value *val = shard->get(key);
      storage::List *list_ptr = nullptr;

      if (!val) {
        storage::List new_list;
        shard->set(key, std::move(new_list));
        val = shard->get(key);
        list_ptr = std::get_if<storage::List>(val);
      } else {
        list_ptr = std::get_if<storage::List>(val);
        if (!list_ptr) {
          return "-ERR WRONGTYPE Operation against a key holding the wrong "
                 "kind of value\r\n";
        }
      }

      for (size_t i = 2; i < args.size(); ++i) {
        list_ptr->push_front(args[i]);
      }

      return ":" + std::to_string(list_ptr->size()) + "\r\n";

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

class LPopCommand : public core::Command {
public:
  std::string name() const override { return "LPOP"; }

  std::string execute(core::Topology &topology, size_t core_id,
                      uint32_t conn_id,
                      const std::vector<std::string> &args) override {
    if (args.size() != 2)
      return "-ERR wrong number of arguments for 'lpop'\r\n";

    const std::string &key = args[1];

    if (topology.is_local(core_id, key)) {
      auto *shard = topology.get_shard(core_id);
      storage::Value *val = shard->get(key);

      if (!val) {
        return "$-1\r\n";
      }

      auto *list_ptr = std::get_if<storage::List>(val);
      if (!list_ptr) {
        return "-ERR WRONGTYPE Operation against a key holding the wrong kind "
               "of value\r\n";
      }

      if (list_ptr->empty()) {
        return "$-1\r\n";
      }

      std::string element = list_ptr->front();
      list_ptr->pop_front();

      return "$" + std::to_string(element.size()) + "\r\n" + element + "\r\n";

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

class LRangeCommand : public core::Command {
public:
  std::string name() const override { return "LRANGE"; }

  std::string execute(core::Topology &topology, size_t core_id,
                      uint32_t conn_id,
                      const std::vector<std::string> &args) override {
    if (args.size() != 4)
      return "-ERR wrong number of arguments for 'lrange'\r\n";

    const std::string &key = args[1];

    if (topology.is_local(core_id, key)) {
      auto *shard = topology.get_shard(core_id);
      storage::Value *val = shard->get(key);

      if (!val) {
        return "*0\r\n";
      }

      auto *list_ptr = std::get_if<storage::List>(val);
      if (!list_ptr) {
        return "-ERR WRONGTYPE Operation against a key holding the wrong kind "
               "of value\r\n";
      }

      try {
        int start = std::stoi(args[2]);
        int stop = std::stoi(args[3]);
        int size = static_cast<int>(list_ptr->size());

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

        std::string resp = "*" + std::to_string(stop - start + 1) + "\r\n";
        for (int i = start; i <= stop; ++i) {
          const auto &elem = (*list_ptr)[i];
          resp += "$" + std::to_string(elem.size()) + "\r\n" + elem + "\r\n";
        }
        return resp;

      } catch (...) {
        return "-ERR value is not an integer or out of range\r\n";
      }

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

class RPushCommand : public core::Command {
public:
  std::string name() const override { return "RPUSH"; }

  std::string execute(core::Topology &topology, size_t core_id,
                      uint32_t conn_id,
                      const std::vector<std::string> &args) override {
    if (args.size() < 3)
      return "-ERR wrong number of arguments for 'rpush'\r\n";

    const std::string &key = args[1];

    if (topology.is_local(core_id, key)) {
      auto *shard = topology.get_shard(core_id);
      storage::Value *val = shard->get(key);
      storage::List *list_ptr = nullptr;

      if (!val) {
        shard->set(key, storage::List{});
        val = shard->get(key);
        list_ptr = std::get_if<storage::List>(val);
      } else {
        list_ptr = std::get_if<storage::List>(val);
        if (!list_ptr)
          return "-ERR WRONGTYPE Operation against a key holding the wrong "
                 "kind of value\r\n";
      }

      for (size_t i = 2; i < args.size(); ++i) {
        list_ptr->push_back(args[i]);
      }
      return ":" + std::to_string(list_ptr->size()) + "\r\n";
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

class RPopCommand : public core::Command {
public:
  std::string name() const override { return "RPOP"; }

  std::string execute(core::Topology &topology, size_t core_id,
                      uint32_t conn_id,
                      const std::vector<std::string> &args) override {
    if (args.size() != 2)
      return "-ERR wrong number of arguments for 'rpop'\r\n";

    const std::string &key = args[1];

    if (topology.is_local(core_id, key)) {
      auto *shard = topology.get_shard(core_id);
      storage::Value *val = shard->get(key);
      if (!val)
        return "$-1\r\n";

      auto *list_ptr = std::get_if<storage::List>(val);
      if (!list_ptr)
        return "-ERR WRONGTYPE Operation against a key holding the wrong kind "
               "of value\r\n";

      if (list_ptr->empty())
        return "$-1\r\n";

      std::string element = list_ptr->back();
      list_ptr->pop_back();
      return "$" + std::to_string(element.size()) + "\r\n" + element + "\r\n";
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

class LLenCommand : public core::Command {
public:
  std::string name() const override { return "LLEN"; }

  std::string execute(core::Topology &topology, size_t core_id,
                      uint32_t conn_id,
                      const std::vector<std::string> &args) override {
    if (args.size() != 2)
      return "-ERR wrong number of arguments for 'llen'\r\n";

    const std::string &key = args[1];

    if (topology.is_local(core_id, key)) {
      auto *shard = topology.get_shard(core_id);
      storage::Value *val = shard->get(key);
      if (!val)
        return ":0\r\n";

      auto *list_ptr = std::get_if<storage::List>(val);
      if (!list_ptr)
        return "-ERR WRONGTYPE Operation against a key holding the wrong kind "
               "of value\r\n";

      return ":" + std::to_string(list_ptr->size()) + "\r\n";
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
