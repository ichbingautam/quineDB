#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "../core/command.hpp"

namespace quine {
namespace commands {

class CommandRegistry {
 public:
  static CommandRegistry& instance() {
    static CommandRegistry instance;
    return instance;
  }

  void register_command(std::unique_ptr<core::Command> cmd) {
    commands_[cmd->name()] = std::move(cmd);
  }

  core::Command* get_command(const std::string& name) {
    auto it = commands_.find(name);
    if (it != commands_.end()) {
      return it->second.get();
    }
    return nullptr;
  }

 private:
  std::unordered_map<std::string, std::unique_ptr<core::Command>> commands_;
};

}  // namespace commands
}  // namespace quine
