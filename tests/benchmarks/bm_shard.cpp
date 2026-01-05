#include <benchmark/benchmark.h>

#include <string>
#include <vector>

#include "storage/shard.hpp"

using namespace quine::storage;

static void BM_HashMapPut(benchmark::State& state) {
  HashMap map(100000);
  int i = 0;
  for (auto _ : state) {
    std::string key = "key" + std::to_string(i++);
    map.put(key, "value");
    if (i >= 80000) {
      state.PauseTiming();
      map = HashMap(100000);  // Reset
      i = 0;
      state.ResumeTiming();
    }
  }
}
BENCHMARK(BM_HashMapPut);

static void BM_ShardSet(benchmark::State& state) {
  Shard shard;
  // Shard uses default capacity (1024), so it will fill up quickly.
  // Ideally Shard should be configurable or resizeable.
  // For V1 benchmark, we reuse keys to avoid crash.

  std::vector<std::string> keys;
  for (int i = 0; i < 1000; ++i) keys.push_back("key" + std::to_string(i));

  int i = 0;
  for (auto _ : state) {
    shard.set(keys[i % 1000], "value");
    i++;
  }
}
BENCHMARK(BM_ShardSet);

static void BM_ShardGet(benchmark::State& state) {
  Shard shard;
  std::vector<std::string> keys;
  for (int i = 0; i < 1000; ++i) {
    std::string k = "key" + std::to_string(i);
    keys.push_back(k);
    shard.set(k, "value");
  }

  int i = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(shard.get(keys[i % 1000]));
    i++;
  }
}
BENCHMARK(BM_ShardGet);

BENCHMARK_MAIN();
