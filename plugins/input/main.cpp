#include "config/config_handler.hpp"
#include "input/input.hpp"
#include <arpa/inet.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>

int main() {
  auto Config = ConfigHandler("/home/devut/Projects/DisLog/plugins/examples/input.json");
  int domain = Config.getSocketType();
  if(domain < 0){
    std::cerr<< "Invalid domain\n";
    exit(EXIT_FAILURE);
  }

  int sock_fd = socket(domain, SOCK_STREAM, 0);

  if (sock_fd < 0) {
    return -1; // Failed creating socket
  }
  const std::string file_path = "/var/log/syslog";

  struct sockaddr_storage addr;
  int socklen = Config.getForwardedSocketStore(&addr);

  int ret = connect(sock_fd, (struct sockaddr *)&addr, socklen);

  if (ret < 0)
    return ret;

  Input syslog(file_path, sock_fd);
  syslog.stream_from_source();

}
