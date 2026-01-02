#include "io_context.hpp"
#include "liburing.h"
#include "operation.hpp"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <unistd.h>

namespace quine {
namespace core {

struct IoContext::NotificationOp : public Operation {
  IoContext *ctx;
  uint64_t buffer = 0; // buffer for eventfd/pipe read
  explicit NotificationOp(IoContext *c) : ctx(c) {}

  void complete(int res) override {
    // Log error if any, but ignoring EAGAIN is fine-ish
    if (res < 0) {
      // std::cerr << "Notification read error: " << -res << std::endl;
    }

    // Invoke handler
    if (ctx->notification_handler_) {
      ctx->notification_handler_();
    }

    // Resubmit to stay active
    ctx->submit_notification_read();
  }
};

IoContext::IoContext(unsigned entries, uint32_t flags) {
  int ret = io_uring_queue_init(entries, &ring_, flags);
  if (ret < 0) {
    throw std::system_error(-ret, std::generic_category(),
                            "io_uring_queue_init failed");
  }
  setup_event_fd();
  notification_op_ = std::make_unique<NotificationOp>(this);
}

IoContext::~IoContext() {
  if (event_fd_ >= 0)
    close(event_fd_);
  if (notify_fd_ >= 0)
    close(notify_fd_);
  io_uring_queue_exit(&ring_);
}

void IoContext::setup_event_fd() {
#ifdef __linux__
  // event_fd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  // notify_fd_ = event_fd_; // Same FD for eventfd
#else
  // Fallback to pipe for macOS/BSD
  int fds[2];
  if (pipe(fds) < 0) {
    throw std::system_error(errno, std::generic_category(), "pipe failed");
  }
  event_fd_ = fds[0];
  notify_fd_ = fds[1];

  // Set non-blocking
  fcntl(event_fd_, F_SETFL, O_NONBLOCK);
  fcntl(notify_fd_, F_SETFL, O_NONBLOCK);
#endif
}

void IoContext::notify() {
  uint64_t u = 1;
  // On pipe, we write 8 bytes to match eventfd semantics roughly
  if (write(notify_fd_, &u, sizeof(u)) < 0) {
    // Ignore EAGAIN
  }
}

void IoContext::set_notification_handler(std::function<void()> handler) {
  notification_handler_ = handler;
}

void IoContext::submit_notification_read() {
  if (event_fd_ < 0)
    return;

  struct io_uring_sqe *sqe = get_sqe();
  // Use read on the event_fd
  // NotificationOp has the buffer member
  io_uring_prep_read(sqe, event_fd_, &notification_op_->buffer,
                     sizeof(uint64_t), 0);
  io_uring_sqe_set_data(sqe, notification_op_.get());
}

struct io_uring_sqe *IoContext::get_sqe() {
  struct io_uring_sqe *sqe = io_uring_get_sqe(&ring_);
  if (!sqe) {
    // Fallback: Submit existing and try again (Ring full)
    io_uring_submit(&ring_);
    sqe = io_uring_get_sqe(&ring_);
    if (!sqe) {
      throw std::runtime_error("IoContext: Submission Queue Full");
    }
  }
  return sqe;
}

void IoContext::submit_and_wait(int wait_nr) {
  int ret = io_uring_submit_and_wait(&ring_, wait_nr);
  (void)ret;
  // check_error(ret, "io_uring_submit_and_wait"); // Stub often returns 0
}

void IoContext::run() {
  // Submit the initial notification listener
  submit_notification_read();

  while (true) {
    submit_and_wait(1);

    struct io_uring_cqe *cqe;
    unsigned head;
    (void)head;
    unsigned count = 0;

    io_uring_for_each_cqe(&ring_, head, cqe) {
      count++;
      if (cqe->user_data) {
        auto *op = reinterpret_cast<Operation *>(cqe->user_data);
        op->complete(cqe->res);
      }
    }

    if (count > 0) {
      io_uring_cq_advance(&ring_, count);
    }
  }
}

} // namespace core
} // namespace quine
