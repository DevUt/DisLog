#include "input/input.hpp"
#include "config/config_handler.hpp"
#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>

int main() {
  int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);

  if (sock_fd < 0) {
    return -1; // Failed creating socket
  }

  auto C = ConfigHandler("/tmp/nonexistts.json");

  const std::string file_path = "/var/log/syslog";
  std::string address = "/tmp/input.sock";

  struct sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, address.c_str(), sizeof(addr.sun_path) - 1);

  int ret = connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr));

  if (ret < 0)
    return ret;

  Input syslog(file_path, sock_fd);
  bool finished = false;
  syslog.stream_from_source(finished);

  if (finished) {
    std::cout << "Done!";
  } else {
    std::cout << "Why not done";
  }
}
