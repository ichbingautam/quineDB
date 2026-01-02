#include "tcp_server.hpp"
#include "../core/operation.hpp"
#include "../network/connection.hpp"
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

namespace quine {
namespace network {

struct TcpServer::AcceptOp : public core::Operation {
  TcpServer *server;

  explicit AcceptOp(TcpServer *s) : server(s) {}

  void complete(int res) override { server->handle_accept(res); }
};

TcpServer::TcpServer(core::IoContext &io, int port, core::Topology &top,
                     size_t core_id)
    : io_(io), port_(port), topology_(top), core_id_(core_id), server_fd_(-1) {
  accept_op_ = std::make_unique<AcceptOp>(this);
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
  struct io_uring_sqe *sqe = io_.get_sqe();

  // We need to pass the address structure to accept if we want client info
  // For now, pass nullptr/0 if we don't care, or add members to AcceptOp if we
  // do.
  io_uring_prep_accept(sqe, server_fd_, nullptr, nullptr, 0);
  io_uring_sqe_set_data(sqe, accept_op_.get());
}

void TcpServer::handle_accept(int fd) {
  if (fd < 0) {
    std::cerr << "Accept error: " << -fd << std::endl;
    // Resubmit accept to keep server alive
    submit_accept();
    return;
  }

  // std::cout << "Accepted connection fd: " << fd << std::endl;

  // Create a new Connection
  auto conn = std::make_unique<Connection>(fd, topology_, core_id_);

  if (on_connect_) {
    on_connect_(conn.get());
  }

  // START reading from the connection
  conn->start(io_);

  // Keeping it alive (hacky for now, need a container in TcpServer)
  conn.release(); // Leaking for V0 proof of concept to avoid immediate
                  // destruction

  // Accept next
  submit_accept();
}

} // namespace network
} // namespace quine
