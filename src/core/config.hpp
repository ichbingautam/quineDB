#pragma once

#include <string>
#include <vector>

namespace quine {
namespace core {

struct RdbSavePoint {
  long seconds;
  long changes;
};

struct Config {
  // RDB Persistence Configuration
  std::string rdb_filename = "dump.rdb";
  bool rdb_compression = true;
  std::string dir = "./";

  // Default Redis-like save points
  std::vector<RdbSavePoint> save_params = {
      {3600, 1},  // Save after 3600 seconds if at least 1 change
      {300, 100}, // Save after 300 seconds if at least 100 changes
      {60, 10000} // Save after 60 seconds if at least 10000 changes
  };

  // Network Configuration
  int port = 6379;
  int worker_threads = 0; // 0 = auto-detect
};

} // namespace core
} // namespace quine
