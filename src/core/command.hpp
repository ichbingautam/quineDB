#pragma once

#include "topology.hpp"
#include <string>
#include <vector>

namespace quine {
namespace core {

class Topology; // Forward declaration

/// @brief Abstract base class for all Redis commands.
class Command {
public:
  virtual ~Command() = default;

  /// @brief Execute the command.
  /// @param topology Access to the cluster topology and shards.
  /// @param core_id The ID of the current core executing the command.
  /// @param args The command arguments (including the command name).
  /// @return The RESP-formatted response string.
  virtual std::string execute(Topology &topology, size_t core_id,
                              uint32_t conn_id,
                              const std::vector<std::string> &args) = 0;

  /// @brief Get the command name (e.g., "SET").
  virtual std::string name() const = 0;
};

} // namespace core
} // namespace quine
