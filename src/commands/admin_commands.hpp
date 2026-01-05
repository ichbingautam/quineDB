#pragma once

#include "../core/command.hpp"
#include "../core/topology.hpp"
#include "../persistence/rdb_manager.hpp"

namespace quine {
namespace commands {

class SaveCommand : public core::Command {
 public:
  std::string name() const override {
    return "SAVE";
  }

  std::string execute(quine::core::Topology& topology, size_t core_id, uint32_t conn_id,
                      const std::vector<std::string>& args) override {
    (void)core_id;
    (void)conn_id;
    (void)args;
    // WARNING: Blocking SAVE. In production, use BGSAVE (fork).
    // Also, this iterates over ALL shards, which might race with modifications
    // from other cores if we don't have a global lock or pause mechanism. For
    // V1 Demo purposes, we assume light load or acceptable risk.
    if (persistence::RdbManager::save(topology, "data/dump.rdb")) {
      return "+OK\r\n";
    } else {
      return "-ERR failed to save\r\n";
    }
  }
};

}  // namespace commands
}  // namespace quine
