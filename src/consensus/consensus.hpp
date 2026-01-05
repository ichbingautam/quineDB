#pragma once

#include <memory>
#include <string>
#include <vector>

namespace quine {
namespace consensus {

/// @brief Abstract base class for Consensus implementations (e.g. Raft, Paxos).
/// In a sharded system, each shard typically has its own consensus group
/// (Multi-Raft).
class ConsensusModule {
 public:
  virtual ~ConsensusModule() = default;

  /// @brief Start the consensus module (e.g. start election timer).
  virtual void start() = 0;

  /// @brief Stop the consensus module.
  virtual void stop() = 0;

  /// @brief Replicate a log entry/command to followers.
  /// @param command The command to replicate.
  /// @return True if committed, false otherwise (e.g. not leader).
  virtual bool replicate(const std::string& command) = 0;

  /// @brief Check if this node is the leader.
  virtual bool is_leader() const = 0;
};

}  // namespace consensus
}  // namespace quine
