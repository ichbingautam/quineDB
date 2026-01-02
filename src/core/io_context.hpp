#pragma once

#include <cstdint>
#include <functional> // [NEW]
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

  /// @brief Register a callback to be invoked when the event_fd is signaled.
  /// Used for integrating ITC/Messaging.
  void set_notification_handler(std::function<void()> handler);

  // Accessors
  struct io_uring *get_ring() { return &ring_; }

  /// @brief Helper for cross-thread wakeups.
  /// On Linux: eventfd. On macOS: pipe.
  /// Returns the read-end FD.
  int get_event_fd() const { return event_fd_; }

  /// @brief Helper for cross-thread wakeups.
  /// On Linux: eventfd. On macOS: pipe.
  /// Returns the write-end FD (to be used by other threads to wake this one
  /// up).
  int get_notify_fd() const { return notify_fd_; }

  /// @brief Notify the event loop (wake up from wait).
  void notify();

  /// @brief Submit a read request for the eventfd/pipe (internal use).
  void submit_notification_read(); // [NEW]

private:
  struct io_uring ring_;
  int event_fd_ = -1;  // Read end
  int notify_fd_ = -1; // Write end

  // Notification handling
  std::function<void()> notification_handler_; // [NEW]

  struct NotificationOp; // [NEW] Forward decl
  friend struct NotificationOp;
  std::unique_ptr<NotificationOp> notification_op_; // [NEW]

  // Setup notification mechanism
  void setup_event_fd();

  // Check ring status
  void check_error(int result, const char *msg);
};

} // namespace core
} // namespace quine
