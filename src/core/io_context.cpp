#include "io_context.hpp"
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
  if (ret < 0) {
    // -EAGAIN or -EINTR are usually fine, but strictly logging here
    std::cerr << "io_uring_submit_and_wait error: " << -ret << std::endl;
  }
}

} // namespace core
} // namespace quine
