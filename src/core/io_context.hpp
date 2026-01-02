#pragma once

#include <cstdint>
#include <liburing.h>
#include <memory>
#include <system_error>
#include <vector>

namespace quine {
namespace core {

class IoContext {
public:
  /// @brief Initializes the io_uring instance.
  /// @param entries Size of the submission/completion queue rings.
  /// @param flags Configuration flags for io_uring setup.
  explicit IoContext(unsigned entries = 4096, uint32_t flags = 0);
  ~IoContext();

  // Delete copy/move to prevent double-free of ring for now
  IoContext(const IoContext &) = delete;
  IoContext &operator=(const IoContext &) = delete;

  // Core Ring Operations

  /// @brief Get a fresh Submission Queue Entry (SQE) to prepare an I/O request.
  /// @return Pointer to a cleared io_uring_sqe, or throws if ring is full.
  struct io_uring_sqe *get_sqe();

  /// @brief Submit queued requests and wait for at least wait_nr completion
  /// events.
  /// @param wait_nr Minimum number of completions to wait for (default 1).
  void submit_and_wait(int wait_nr = 1);

  /// @brief Run the event loop indefinitely.
  /// Dispatches completions to the Operation* stored in user_data.
  void run();

  /// @brief Run the event loop indefinitely.
  /// Dispatches completions to the Operation* stored in user_data.
  void run();

  // Accessors
  struct io_uring *get_ring() { return &ring_; }

private:
  struct io_uring ring_;

  // Check ring status
  void check_error(int result, const char *msg);
};

} // namespace core
} // namespace quine
