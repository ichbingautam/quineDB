# QuineDB

**QuineDB** is a high-performance, distributed, in-memory key-value store built with modern C++. It is designed to demonstrate advanced systems programming concepts, including thread-per-core architecture, asynchronous I/O with `io_uring`, and shared-nothing sharding.

## Features
*   **High Performance Architecture**:
    *   **Thread-per-core**: Each core manages its own shard of data, eliminating lock contention on the data path.
    *   **Shared-Nothing**: No shared memory between cores; strict partitioning.
    *   **Asynchronous I/O**: Built on `io_uring` (Linux) for high-throughput non-blocking I/O (with fallback to `kqueue` mock for macOS/BSD compatibility).

*   **Redis-Compatible Protocol (RESP)**:
    *   Connect using any standard Redis client (e.g., `redis-cli`, `redis-py`).

*   **Rich Data Structures**:
    *   **Strings**: `SET`, `GET`, `INCR`, `DECR`, `DEL`
    *   **Lists**: `LPUSH`, `RPUSH`, `LPOP`, `RPOP`, `LRANGE`
    *   **Sets**: `SADD`, `SREM`, `SMEMBERS`, `SISMEMBER`
    *   **Hashes**: `HSET`, `HGET`, `HGETALL`, `HDEL`
    *   **Sorted Sets**: `ZADD`, `ZREM`, `ZSCORE`, `ZCARD`, `ZRANGE` (with `WITHSCORES`)

*   **Advanced Features**:
    *   **Persistence**: RDB-compatible snapshotting (save/load) to disk.
    *   **TTL**: Key expiration (`EXPIRE`, `TTL`) with lazy expiration.
    *   **Scalability**: Consistent Hashing with connection forwarding for seamless horizontal scaling.

## Building QuineDB

### Prerequisites
*   C++17 compliant compiler (GCC 9+, Clang 10+)
*   CMake 3.10+
*   Linux (recommended for `io_uring`) or macOS.

### Build Instructions

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

## Running the Server

Start the server using the compiled binary. By default, it runs on port `6379`.

```bash
./build/bin/quine-server
```

You should see startup logs indicating the server has initialized on available cores.

## Usage

Connect using `redis-cli` or any compatible client:

```bash
redis-cli -p 6379
```

### Examples

**Strings & TTL**
```redis
SET mykey "Hello QuineDB"
EXPIRE mykey 10
GET mykey
TTL mykey
```

**Sorted Sets**
```redis
ZADD leaderboard 100 "PlayerA"
ZADD leaderboard 200 "PlayerB"
ZRANGE leaderboard 0 -1 WITHSCORES
```

**Persistence**
```redis
SAVE
# Data saved to data/dump.rdb
```

## Architecture

### Thread-per-Core
QuineDB spawns one worker thread per available CPU core. Keys are mapped to specific shards using **Consistent Hashing** (Ring architecture with virtual nodes).

### Request Routing
When a client connects to any core, QuineDB's internal Router determines which shard owns the requested key.
*   If the key belongs to the current core, it is processed immediately.
*   If it belongs to another core, the request is forwarded internally via lock-free message passing channels.

### Persistence
The `RdbManager` handles snapshotting the in-memory state to disk in a format compatible with Redis RDB (v1), ensuring data durability across restarts.

## Roadmap

*   **Consensus**: Full Raft implementation for distributed fault tolerance (Skeleton currently in `src/consensus`).
*   **Active Expiration**: Background eviction of expired keys.
*   **Cluster Mode**: Multi-node clustering support.

## License

MIT License. See `LICENSE` for details.
