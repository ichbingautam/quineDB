# QuineDB Tests

This directory contains unit tests and benchmarks for QuineDB.

## Building Tests

The tests are integrated into the main CMake build system but are optional (though enabled by default in our setup).

```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make unit_tests db_benchmarks
```

## Running Unit Tests

We use GoogleTest.

```bash
./tests/unit_tests
```

This runs tests for:
- `HashMap` (Put, Get, Del, Collision)
- `Shard` (Set, Get, TTL, Data Structures)

## Running Benchmarks

We use Google Benchmark.

```bash
./tests/db_benchmarks
```

This measures the throughput (ops/sec) and latency of the storage engine directly (bypassing network).

## Adding New Tests

1. Create a new `.cpp` file in `tests/unit/`.
2. Add it to `add_executable(unit_tests ...)` in `tests/CMakeLists.txt`.
