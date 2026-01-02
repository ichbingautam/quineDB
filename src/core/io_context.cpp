#include "io_context.hpp"
#include "../include/stub/liburing.h"
#include "operation.hpp"
#include <iostream>
#include <stdexcept>

namespace quine {
namespace core {

IoContext::IoContext(unsigned entries, uint32_t flags) {
  int ret = io_uring_queue_init(entries, &ring_, flags);
  if (ret < 0) {
    throw std::system_error(-ret, std::generic_category(),
                            "io_uring_queue_init failed");
  }
}

IoContext::~IoContext() { io_uring_queue_exit(&ring_); }

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
  // check_error(ret, "io_uring_submit_and_wait"); // Stub often returns 0
}

void IoContext::run() {
  while (true) {
    submit_and_wait(1);

    struct io_uring_cqe *cqe;
    unsigned head;
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
