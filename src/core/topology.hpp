#pragma once

#include "../storage/shard.hpp"
#include "itc_channel.hpp"
#include "message.hpp"
#include "router.hpp"
#include <memory>
#include <stdexcept>
#include <unistd.h> // for write
#include <vector>

namespace quine {
namespace core {

/// @brief Holds the static topology of the node/cluster.
/// Contains the Router, Shards, and ITC Channels for all cores.
class Topology {
public:
  Topology(size_t num_cores) : router_(num_cores), num_cores_(num_cores) {

    // Initialize resources for each core
    for (size_t i = 0; i < num_cores; ++i) {
      shards_.push_back(std::make_unique<storage::Shard>());
      channels_.push_back(std::make_unique<ItcChannel<Message>>());
      notify_fds_.push_back(-1); // Init with invalid FD
    }
  }

  // Register the write-end of the eventfd/pipe for a core
  void register_notify_fd(size_t core_id, int fd) {
    if (core_id >= notify_fds_.size())
      throw std::out_of_range("Invalid core_id");
    notify_fds_[core_id] = fd;
  }

  // Wake up a specific core
  void notify_core(size_t core_id) {
    if (core_id >= notify_fds_.size())
      return;
    int fd = notify_fds_[core_id];
    if (fd >= 0) {
      uint64_t u = 1;
      if (::write(fd, &u, sizeof(u)) < 0) {
        // Ignore error for now, purely signaling
      }
    }
  }

  // -- Accessors --

  size_t get_num_cores() const { return num_cores_; }
  size_t shard_count() const { return num_cores_; }

  Router &get_router() { return router_; }

  storage::Shard *get_shard(size_t core_id) {
    if (core_id >= shards_.size())
      throw std::out_of_range("Invalid core_id");
    return shards_[core_id].get();
  }

  const storage::Shard *get_shard(size_t core_id) const {
    if (core_id >= shards_.size())
      throw std::out_of_range("Invalid core_id");
    return shards_[core_id].get();
  }

  ItcChannel<Message> *get_channel(size_t core_id) {
    if (core_id >= channels_.size())
      throw std::out_of_range("Invalid core_id");
    return channels_[core_id].get();
  }

  // Helper to check if a key belongs to a specific core
  bool is_local(size_t core_id, const std::string &key) {
    return router_.get_shard_id(key) == core_id;
  }

  // Helper to get target core
  size_t get_target_core(const std::string &key) {
    return router_.get_shard_id(key);
  }

private:
  Router router_;
  size_t num_cores_;

  // Per-core resources
  std::vector<std::unique_ptr<storage::Shard>> shards_;
  std::vector<std::unique_ptr<ItcChannel<Message>>> channels_;
  std::vector<int> notify_fds_;
};

} // namespace core
} // namespace quine
