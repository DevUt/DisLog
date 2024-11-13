#include <cerrno>
#include <cstddef>
#include <fcntl.h>
#include <format>
#include <iostream>
#include <source/source.hpp>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <thread>
#include <unordered_map>

#include "service.hpp"

int set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    return -1;
  }
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    return -1;
  }
  return 0;
}

// Connect to out sources and map their ids
std::unordered_map<std::string, int> outSrc_to_fd;
std::unordered_map<int, std::string> fd_to_outSrc;
int service(Source *inputSource, std::vector<Source *> outputSources) {

  for (auto &out : outputSources) {
    // Create socket
    int domain = out->getTypeOfSocket();
    int sockfd = socket(domain, SOCK_STREAM, 0);
    if (sockfd < 0) {
      std::cerr << std::format("Couldn't create a socket for {}", out->tag)
                << std::strerror(errno) << '\n';
    } else {
      outSrc_to_fd[out->tag] = sockfd;
      fd_to_outSrc[sockfd] = out->tag;
    }

    set_nonblocking(sockfd);

    // Connect to it
    struct sockaddr_storage sock_out;
    int socklen = out->constructSock(&sock_out);
    if (socklen == 0) {
      std::cerr << std::format("Couldn't connect to tag {} {}\n", out->tag,
                               std::strerror(errno));
    } else {
      int ret = connect(sockfd, (struct sockaddr *)(&sock_out), socklen);
      if (ret < 0) {
        if (errno != EINPROGRESS) {

          std::cerr << std::format("Couldn't connect to tag {} {}\n", out->tag,
                                   std::strerror(errno));
        }
      }
    }
  }

  // Start a server to listen at client side
  return listen_source(inputSource->clone());
}

/**
 * @brief It writes to a specific fd
 *
 * @param[in] write_fd The fd to which to write
 * @param[in] buf The data we need to write
 * @param[in] bytes_to_write How many bytes to write
 */
void write_to_conn(int write_fd, char *buf, size_t bytes_to_write) {
  if (bytes_to_write <= 0)
    return;

  // Non-blocking write
  size_t bytes_written = 0;
  while (bytes_written < bytes_to_write) {
    ssize_t result =
        write(write_fd, buf + bytes_written, bytes_to_write - bytes_written);

    if (result < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // TODO: Make a queue out of this data
        return;
      }
      std::cerr << "Write error: \n"
                << std::strerror(errno) << "for " << fd_to_outSrc[write_fd]
                << '\n';
      return;
    }
    bytes_written += result;
  }
  return;
}

/**
 * @brief Receives data from client and forwards it to many of the output fds
 *
 * @param[in] connfd The client side fd from which the data would be read
 * @return True on success;
 */

bool handle_conn(int connfd) {
  char buf[1024];
  ssize_t bytes_read;

  while (true) {
    bytes_read = read(connfd, buf, sizeof(buf) - 1);

    if (bytes_read < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // No more data available right now
        return true;
      }
      // Real error occurred
      std::cerr << "Read error: " << std::strerror(errno) << '\n';
      return false;
    } else if (bytes_read == 0) {
      // Connection closed
      std::cout << fd_to_outSrc[connfd] << "closed the connection" << "\n";
      return false;
    }

    for (auto &[tag, fd] : outSrc_to_fd) {
      // TODO: Imporve this.
      std::thread(write_to_conn, fd, buf, bytes_read).detach();
    }
  }
}

int listen_source(Source *inputSource) {
  struct sockaddr_storage sock_out;
  int socklen = inputSource->constructSock(&sock_out);
  int sockfd = socket(inputSource->getTypeOfSocket(), SOCK_STREAM, 0);

  if (sockfd < 0) {
    std::cerr << std::format("Couldn't create a socket for {}\n",
                             inputSource->tag);
    return -1;
  }

  // Set it to be nonblocking
  set_nonblocking(sockfd);

  if (bind(sockfd, (struct sockaddr *)&sock_out, socklen) < 0) {
    std::cerr << std::format("Binding failed for input source {}\n",
                             inputSource->tag);
    return -1;
  }

  if (listen(sockfd, SOMAXCONN) < 0) {
    std::cerr << std::format("Listening failed for input source {}\n",
                             inputSource->tag);
    return -1;
  }

  std::cout << "Server started on" << inputSource->getLocation() << std::endl;

  int epollfd = epoll_create1(0);
  if (epollfd < 0) {
    std::cerr << "Failed to create epoll: " << std::strerror(errno) << '\n';
    close(sockfd);
    return -1;
  }

  struct epoll_event ev{};
  ev.events = EPOLLIN;
  ev.data.fd = sockfd;
  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev) < 0) {
    std::cerr << "Failed to add listening socket to epoll: "
              << std::strerror(errno) << '\n';
    close(epollfd);
    close(sockfd);
    return -1;
  }

  std::vector<epoll_event> events(fd_to_outSrc.size());

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

        ev.events = EPOLLIN | EPOLLRDHUP; // Detect disconnection
        ev.data.fd = connfd;
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev) < 0) {
          std::cerr << "Failed to add client to epoll: " << std::strerror(errno)
                    << '\n';
          close(connfd);
          continue;
        }
      } else {
        // Handle existing connection
        if (events[n].events & (EPOLLRDHUP | EPOLLERR | EPOLLHUP)) {
          // Client disconnected or error occurred
          epoll_ctl(epollfd, EPOLL_CTL_DEL, events[n].data.fd, nullptr);
          close(events[n].data.fd);
          std::cerr << "Client disconnected\n";
        } else {
          handle_conn(events[n].data.fd);
        }
      }
    }
  }

  // Cleanup
  close(epollfd);
  close(sockfd);
  inputSource->cleanUp();
  return 0;
}
