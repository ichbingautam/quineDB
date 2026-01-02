#include "tcp_server.hpp"
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

namespace quine {
namespace network {

TcpServer::TcpServer(core::IoContext &io, int port, storage::Shard *shard)
    : io_(io), port_(port), shard_(shard), server_fd_(-1) {
  setup_listener();
}

TcpServer::~TcpServer() {
  if (server_fd_ >= 0) {
    close(server_fd_);
  }
}

void TcpServer::setup_listener() {
  server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd_ < 0) {
    throw std::system_error(errno, std::generic_category(), "socket failed");
  }

  int opt = 1;
  // SO_REUSEPORT is crucial for thread-per-core scalability
  if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt)) < 0) {
    throw std::system_error(errno, std::generic_category(),
                            "setsockopt failed");
  }

  struct sockaddr_in addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port_);

  if (bind(server_fd_, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    throw std::system_error(errno, std::generic_category(), "bind failed");
  }

  if (listen(server_fd_, 1024) < 0) {
    throw std::system_error(errno, std::generic_category(), "listen failed");
  }
}

void TcpServer::start() {
  // Initial accept submission
  submit_accept();
}

void TcpServer::submit_accept() {
  // TODO: We need a struct to track this detailed operation context (Token)
  // For now, using raw io_uring_prep_accept

  struct io_uring_sqe *sqe = io_.get_sqe();

  // In a real impl, we'd pass a pointer to a struct holding the client_addr/len
  // buffers io_uring_prep_accept(sqe, server_fd_, ...);
  // io_uring_sqe_set_data(sqe, this); // or a special AcceptToken

  // Placeholder to make it compile with logic
  (void)sqe;
}

} // namespace network
} // namespace quine
