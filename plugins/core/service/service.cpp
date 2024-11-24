#include <cerrno>
#include <cstddef>
#include <exception>
#include <fcntl.h>
#include <format>
#include <iostream>
#include <source/source.hpp>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <thread>
#include <unordered_map>

#include "service.hpp"

// int ServiceClient::listen_source(Source *inputSource) {
//   struct sockaddr_storage sock_out;
//   int socklen = inputSource->constructSock(&sock_out);
//   int sockfd = socket(inputSource->getTypeOfSocket(), SOCK_STREAM, 0);
//
//   if (sockfd < 0) {
//     std::cerr << std::format("Couldn't create a socket for {}\n",
//                              inputSource->tag);
//     return -1;
//   }
//
//   // Set it to be nonblocking
//   set_nonblocking(sockfd);
//
//   if (bind(sockfd, (struct sockaddr *)&sock_out, socklen) < 0) {
//     std::cerr << std::format("Binding failed for input source {}\n",
//                              inputSource->tag);
//     return -1;
//   }
//
//   if (listen(sockfd, SOMAXCONN) < 0) {
//     std::cerr << std::format("Listening failed for input source {}\n",
//                              inputSource->tag);
//     return -1;
//   }
//
//   std::cout << "Server started on" << inputSource->getLocation() <<
//   std::endl;
//
//   int epollfd = epoll_create1(0);
//   if (epollfd < 0) {
//     std::cerr << "Failed to create epoll: " << std::strerror(errno) << '\n';
//     close(sockfd);
//     return -1;
//   }
//
//   struct epoll_event ev{};
//   ev.events = EPOLLIN;
//   ev.data.fd = sockfd;
//   if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev) < 0) {
//     std::cerr << "Failed to add listening socket to epoll: "
//               << std::strerror(errno) << '\n';
//     close(epollfd);
//     close(sockfd);
//     return -1;
//   }
//
//   std::vector<epoll_event> events(1024);
//
//   while (true) {
//     int nfds = epoll_wait(epollfd, events.data(), events.size(), -1);
//     if (nfds < 0) {
//       if (errno == EINTR)
//         continue; // Interrupted by signal
//       std::cerr << "epoll_wait failed: " << std::strerror(errno) << '\n';
//       break;
//     }
//
//     for (int n = 0; n < nfds; ++n) {
//       if (events[n].data.fd == sockfd) {
//         // New connection
//         int connfd = accept(sockfd, nullptr, nullptr);
//         if (connfd < 0) {
//           std::cerr << "Accept failed: " << std::strerror(errno) << '\n';
//           continue;
//         }
//
//         ev.events = EPOLLIN | EPOLLRDHUP; // Detect disconnection
//         ev.data.fd = connfd;
//         if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev) < 0) {
//           std::cerr << "Failed to add client to epoll: " <<
//           std::strerror(errno)
//                     << '\n';
//           close(connfd);
//           continue;
//         }
//       } else {
//         // Handle existing connection
//         if (events[n].events & (EPOLLRDHUP | EPOLLERR | EPOLLHUP)) {
//           // Client disconnected or error occurred
//           epoll_ctl(epollfd, EPOLL_CTL_DEL, events[n].data.fd, nullptr);
//           close(events[n].data.fd);
//           std::cerr << "Client disconnected\n";
//         } else {
//           handle_conn();
//         }
//       }
//     }
//   }
//
//   // Cleanup
//   close(epollfd);
//   close(sockfd);
//   inputSource->cleanUp();
//   return 0;
// }

/**
 * @brief It writes to a specific fd
 *
 * @param[in] write_fd The fd to which to write
 * @param[in] buf The data we need to write
 * @param[in] bytes_to_write How many bytes to write
 */
void ServiceClient::write_to_conn(int write_fd, char *buf,
                                  size_t bytes_to_write) {
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
      std::cerr << "Write error: \n" << std::strerror(errno) << '\n';
      std::terminate();
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

void ServiceClient::handle_conn() {
  char buf[1024];
  ssize_t bytes_read;

  while (!haveInput)
    ;

  while (true) {
    bytes_read = read(inputfd, buf, sizeof(buf) - 1);

    if (bytes_read < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // No more data available right now
        return;
      }
      // Real error occurred
      std::cerr << "Read error: " << std::strerror(errno) << '\n';
      return;
    } else if (bytes_read == 0) {
      // Connection closed
      std::cout << clientfd << "closed the connection" << "\n";
      return;
    }

    write_to_conn(clientfd, buf, bytes_read);
  }
}

void ServiceClient::run(){
  std::thread(&ServiceClient::handle_conn, this).detach();
}
