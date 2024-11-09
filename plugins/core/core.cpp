#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>

void handle_conn(int connfd) {
  char buf[1024];
  ssize_t bytes_read;

  while ((bytes_read = read(connfd, buf, sizeof(buf) - 1)) > 0) {
    buf[bytes_read] = '\0';
    std::cout << "Received: " << buf << '\n';

    // Echo back to client
    ssize_t bytes_written = write(connfd, buf, bytes_read);
    if (bytes_written != bytes_read) {
      std::cerr << "Failed to echo data: " << std::strerror(errno) << '\n';
      break;
    }
  }

  if (bytes_read == 0) {
    std::cout << "Client disconnected\n";
  } else if (bytes_read < 0) {
    std::cerr << "Read error: " << std::strerror(errno) << '\n';
  }
}

int main() {
  const std::string socket_path = "/tmp/input.sock";

  // Remove existing socket file if it exists
  // Else it goes crazy
  std::filesystem::remove(socket_path);

  int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sockfd < 0) {
    std::cerr << "Failed to create socket: " << std::strerror(errno) << '\n';
    return 1;
  }

  struct sockaddr_un serv_addr{};
  serv_addr.sun_family = AF_UNIX;
  strncpy(serv_addr.sun_path, socket_path.c_str(),
          sizeof(serv_addr.sun_path) - 1);

  if (bind(sockfd, (struct sockaddr *)(&serv_addr),
           sizeof(serv_addr)) < 0) {
    std::cerr << "Failed to bind: " << std::strerror(errno) << '\n';
    close(sockfd);
    return 1;
  }

  if (listen(sockfd, SOMAXCONN) < 0) {
    std::cerr << "Failed to listen: " << std::strerror(errno) << '\n';
    close(sockfd);
    return 1;
  }

  std::cout << "Server started on " << socket_path << '\n';

  int epollfd = epoll_create1(0);
  if (epollfd < 0) {
    std::cerr << "Failed to create epoll: " << std::strerror(errno) << '\n';
    close(sockfd);
    return 1;
  }

  struct epoll_event ev{};
  ev.events = EPOLLIN;
  ev.data.fd = sockfd;
  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev) < 0) {
    std::cerr << "Failed to add listening socket to epoll: "
              << std::strerror(errno) << '\n';
    close(epollfd);
    close(sockfd);
    return 1;
  }

  std::vector<epoll_event> events(50);

  while (true) {
    int nfds = epoll_wait(epollfd, events.data(), events.size(), -1);
    if (nfds < 0) {
      if (errno == EINTR)
        continue; // Interrupted by signal
      std::cerr << "epoll_wait failed: " << std::strerror(errno) << '\n';
      break;
    }

    for (int n = 0; n < nfds; ++n) {
      if (events[n].data.fd == sockfd) {
        // New connection
        int connfd = accept(sockfd, nullptr, nullptr);
        if (connfd < 0) {
          std::cerr << "Accept failed: " << std::strerror(errno) << '\n';
          continue;
        }

        ev.events = EPOLLIN | EPOLLRDHUP; // Detect client disconnection
        ev.data.fd = connfd;
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev) < 0) {
          std::cerr << "Failed to add client to epoll: " << std::strerror(errno)
                    << '\n';
          close(connfd);
          continue;
        }
        std::cout << "New client connected\n";
      } else {
        // Handle existing connection
        if (events[n].events & (EPOLLRDHUP | EPOLLERR | EPOLLHUP)) {
          // Client disconnected or error occurred
          epoll_ctl(epollfd, EPOLL_CTL_DEL, events[n].data.fd, nullptr);
          close(events[n].data.fd);
          std::cout << "Client disconnected\n";
        } else {
          handle_conn(events[n].data.fd);
        }
      }
    }
  }

  // Cleanup
  close(epollfd);
  close(sockfd);
  std::filesystem::remove(socket_path);
  return 0;
}
