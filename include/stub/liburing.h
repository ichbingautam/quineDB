#pragma once

#if defined(__APPLE__) || defined(__MACH__)

#include <cstdint>
#include <sys/socket.h> // [NEW] for sockaddr
#include <unistd.h>

// Mock structures to satisfy compiler on macOS
struct io_uring {
  int ring_fd;
};
struct io_uring_sqe {
  uint64_t user_data;
  int opcode;
  int fd;
  uint64_t addr;
  uint32_t len;
};
struct io_uring_cqe {
  uint64_t user_data;
  int32_t res;
  uint32_t flags;
};

// Mock constants
#define IORING_OP_ACCEPT 0
#define IORING_OP_READ 1
#define IORING_OP_WRITE 2

// Mock functions
inline int io_uring_queue_init(unsigned, struct io_uring *, unsigned) {
  return 0;
}
inline void io_uring_queue_exit(struct io_uring *) {}
inline struct io_uring_sqe *io_uring_get_sqe(struct io_uring *) {
  static struct io_uring_sqe sqe;
  return &sqe;
}
inline int io_uring_submit(struct io_uring *) { return 0; }
inline int io_uring_submit_and_wait(struct io_uring *, unsigned) {
  sleep(1);
  return 0;
} // Sleep to simulate blocking
inline void io_uring_sqe_set_data(struct io_uring_sqe *sqe, void *data) {
  sqe->user_data = (uint64_t)data;
}
inline void *io_uring_cqe_get_data(const struct io_uring_cqe *cqe) {
  return (void *)cqe->user_data;
}

// [NEW] Prep functions
inline void io_uring_prep_accept(struct io_uring_sqe *sqe, int fd,
                                 struct sockaddr *addr, socklen_t *addrlen,
                                 int flags) {
  sqe->opcode = IORING_OP_ACCEPT;
  sqe->fd = fd;
  sqe->addr = (uint64_t)addr;
  (void)addrlen;
  (void)flags;
}

inline void io_uring_prep_read(struct io_uring_sqe *sqe, int fd, void *buf,
                               unsigned nbytes, uint64_t offset) {
  sqe->opcode = IORING_OP_READ;
  sqe->fd = fd;
  sqe->addr = (uint64_t)buf;
  sqe->len = nbytes;
  (void)offset;
}

inline void io_uring_prep_write(struct io_uring_sqe *sqe, int fd,
                                const void *buf, unsigned nbytes,
                                uint64_t offset) {
  sqe->opcode = IORING_OP_WRITE;
  sqe->fd = fd;
  sqe->addr = (uint64_t)buf;
  sqe->len = nbytes;
  (void)offset;
}

// Iterators stub - hard to mock perfectly with macros/inline, typically handled
// by loops
#define io_uring_for_each_cqe(ring, head, cqe)                                 \
  for (head = 0; (cqe = nullptr, false); head++)
inline void io_uring_cq_advance(struct io_uring *, unsigned) {}

#endif
