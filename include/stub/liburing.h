#pragma once

#if defined(__APPLE__) || defined(__MACH__)

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <deque>
#include <fcntl.h>
#include <iostream>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

// Mock constants
#define IORING_OP_ACCEPT 0
#define IORING_OP_READ 1
#define IORING_OP_WRITE 2

struct io_uring_sqe {
  uint64_t user_data;
  int opcode;
  int fd;
  uint64_t addr;
  uint32_t len;
  bool active = false; // Internal: is this slot being used?
};

struct io_uring_cqe {
  uint64_t user_data;
  int32_t res;
};

struct io_uring {
  std::vector<io_uring_sqe> sqes;
  std::deque<io_uring_sqe> pending_sqes; // Submitted but not completed
  std::deque<io_uring_cqe> cqes;         // Completed
  unsigned entries;
};

inline int io_uring_queue_init(unsigned entries, struct io_uring *ring,
                               unsigned) {
  ring->entries = entries;
  ring->sqes.resize(entries);
  return 0;
}

inline void io_uring_queue_exit(struct io_uring *) {}

inline struct io_uring_sqe *io_uring_get_sqe(struct io_uring *ring) {
  // Find a free slot in sqes (simple linear search for V0 stub)
  for (auto &sqe : ring->sqes) {
    if (!sqe.active) {
      sqe.active = true;
      return &sqe;
    }
  }
  return nullptr; // Ring full
}

inline void io_uring_sqe_set_data(struct io_uring_sqe *sqe, void *data) {
  sqe->user_data = (uint64_t)data;
}

// Prep functions
inline void io_uring_prep_accept(struct io_uring_sqe *sqe, int fd,
                                 struct sockaddr *addr, socklen_t *addrlen,
                                 int flags) {
  sqe->opcode = IORING_OP_ACCEPT;
  sqe->fd = fd;
  sqe->addr =
      (uint64_t)addr; // We ignore addr return in this simple stub/reactor
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

inline int io_uring_submit(struct io_uring *ring) {
  // Move active SQEs to pending_sqes
  int submitted = 0;
  for (auto &sqe : ring->sqes) {
    if (sqe.active) {
      ring->pending_sqes.push_back(sqe);
      sqe.active = false; // Clear slot for reuse
      submitted++;
    }
  }
  return submitted;
}

inline int io_uring_submit_and_wait(struct io_uring *ring, unsigned wait_nr) {
  io_uring_submit(ring);

  // If we already have enough completions, return immediately
  // (Not strictly required by API, but good optimization)
  if (ring->cqes.size() >= wait_nr && wait_nr > 0)
    return 0;

  // Simple reactor loop using select
  // Note: This is inefficient but functional for small scale
  // Loop until we have at least wait_nr completions

  while (ring->cqes.size() < wait_nr) {
    fd_set readfds, writefds;
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    int max_fd = -1;

    if (ring->pending_sqes.empty()) {
      // Nothing to wait for?
      if (wait_nr == 0)
        return 0;
      // Deadlock prevention if logic is flawed, theoretically shouldn't happen
      // if wait_nr > 0 implies pending I/O
      usleep(1000);
      // Check for manual break or signal handling in real app
      if (wait_nr > 0 && ring->cqes.size() >= wait_nr)
        break; // Check if we gathered enough
      continue;
    }

    for (const auto &sqe : ring->pending_sqes) {
      if (sqe.opcode == IORING_OP_READ || sqe.opcode == IORING_OP_ACCEPT) {
        FD_SET(sqe.fd, &readfds);
      } else if (sqe.opcode == IORING_OP_WRITE) {
        FD_SET(sqe.fd, &writefds);
      }
      if (sqe.fd > max_fd)
        max_fd = sqe.fd;
    }

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000; // 10ms timeout

    int sel_res = select(max_fd + 1, &readfds, &writefds, nullptr, &timeout);

    if (sel_res < 0) {
      if (errno == EINTR)
        continue;
      break;
    }

    // Check results
    auto it = ring->pending_sqes.begin();
    while (it != ring->pending_sqes.end()) {
      bool passed = false;
      int res = -1;

      if (it->opcode == IORING_OP_READ || it->opcode == IORING_OP_ACCEPT) {
        if (FD_ISSET(it->fd, &readfds))
          passed = true;
      } else if (it->opcode == IORING_OP_WRITE) {
        if (FD_ISSET(it->fd, &writefds))
          passed = true;
      }

      if (passed) {
        if (it->opcode == IORING_OP_ACCEPT) {
          res = ::accept(it->fd, nullptr, nullptr);
        } else if (it->opcode == IORING_OP_READ) {
          res = ::read(it->fd, (void *)it->addr, it->len);
        } else if (it->opcode == IORING_OP_WRITE) {
          res = ::write(it->fd, (void *)it->addr, it->len);
        }

        if (res < 0) {
          if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // Spurious wakeup? Keep pending
            passed = false;
          } else {
            res = -errno; // Use negative errno
          }
        }
      }

      if (passed) {
        // Create CQE
        io_uring_cqe cqe;
        cqe.user_data = it->user_data;
        cqe.res = res;
        ring->cqes.push_back(cqe);

        // Remove from pending
        it = ring->pending_sqes.erase(it);
      } else {
        ++it;
      }
    }

    if (wait_nr == 0)
      break;
  }
  return 0;
}

// Iterator helpers
#define io_uring_for_each_cqe(ring, head, cqe)                                 \
  for (head = 0;                                                               \
       (head < (ring)->cqes.size() ? (cqe = &(ring)->cqes[head], true)         \
                                   : (cqe = nullptr, false));                  \
       head++)

inline void io_uring_cq_advance(struct io_uring *ring, unsigned count) {
  // Remove 'count' items from front of deque
  for (unsigned i = 0; i < count && !ring->cqes.empty(); ++i) {
    ring->cqes.pop_front();
  }
}

inline void *io_uring_cqe_get_data(const struct io_uring_cqe *cqe) {
  return (void *)cqe->user_data;
}

#endif
