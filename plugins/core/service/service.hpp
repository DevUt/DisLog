#pragma once

#include <fcntl.h>
#include <source/source.hpp>
#include <vector>

/**
 * @brief Service information about the client
 *        What does the client want.
 */
class ServiceClient {
private:
  /**
   * @clientfd This is the client side
   */
  int clientfd;

  int write_to_conn(int write_fd, char *buf, size_t bytes_to_write);

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

public:
  /**
   * @inputfd This is the input source.
   *    The client would be forwarded this
   */
  int inputfd;

  bool haveInput = false;
  /**
   * @brief Service a client who know what is required
   *
   * @param[in] clientfd Output filedescriptor
   * @param[in] inputfd File descriptor for the input file stream
   */
  ServiceClient(int clientfd, int inputfd)
      : inputfd(inputfd), clientfd(clientfd), haveInput(true) {};

  /**
   * @brief Service a client which don't know what they want
   *
   * @param[in] clientfd File descriptor to talk to the  client
   */
  ServiceClient(int clientfd) : clientfd(clientfd) {};

  void handle_conn();

  void run();
};
