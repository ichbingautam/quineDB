# Release Notes

## v0.1.0 - Initial Release: "Genesis"

**Date:** 2026-01-05

This is the first official release of **QuineDB**, a high-performance, distributed, thread-per-core key-value store built with C++20 and `io_uring`.

### 🚀 Key Features

*   **Core Architecture**:
    *   **Thread-per-Core**: Shared-nothing architecture for maximum throughput and cache locality.
    *   **Asynchronous I/O**: leveraging `io_uring` for non-blocking I/O operations (Linux).
    *   **Inter-Core Communication**: Lock-free message passing between cores using `eventfd` (Linux) and pipes (macOS).

*   **Data Structures**:
    *   **Strings**: Basic `SET`, `GET`, `DEL` operations.
    *   **Sets**: `SADD`, `SMEMBERS`, `SREM`.
    *   **Sorted Sets (ZSets)**: `ZADD`, `ZRANGE`, `ZREM`, `ZSCORE`, `ZCARD`.
    *   **TTL Support**: `EXPIRE` and `TTL` commands for automatic key expiration.

*   **Networking & Protocol**:
    *   **RESP Compatibility**: Fully compatible with the Redis Serialization Protocol (RESP), allowing usage with standard Redis clients.
    *   **Pipelining**: Support for request pipelining.

*   **Partitioning & Distribution**:
    *   **Sharding**: Automatic data partitioning across cores using consistent hashing.
    *   **Request Routing**: Intelligent routing of commands to the owner core.

### 🛠 Infrastructure & Quality

*   **CI/CD Pipeline**:
    *   Automated GitHub Actions workflow.
    *   **Sanitizers**: Continuous testing with AddressSanitizer (ASan), ThreadSanitizer (TSan), and UndefinedBehaviorSanitizer (UBSan).
    *   **Docker**: Multi-stage Docker builds and automated publishing to GitHub Container Registry (GHCR).
    *   **Linting**: strict code formatting with `clang-format`.

*   **Testing**:
    *   **Unit Tests**: GoogleTest framework integration.
    *   **Benchmarks**: Google Benchmark integration for performance regression testing.
    *   **Integration Tests**: Python-based functional tests.

### 🐛 Bug Fixes

*   Fixed critical CI deadlocks caused by missing `eventfd` initialization on Linux.
*   Resolved race conditions during core startup using topology barriers.
*   Fixed macOS/Linux compatibility issues for development environments.

### 📦 Deployment

*   **Docker**: `ghcr.io/ichbingautam/quinedb:v0.1.0`
*   **Kubernetes**: Example manifests provided for StatefulSet deployments.
