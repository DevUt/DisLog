#include "input.hpp"
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

  const std::string file_path = "/var/log/syslog";
  std::string address = "/tmp/input.sock";
  int port = 8080;

  struct sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  int ret = 0;
  ret = inet_pton(AF_UNIX, file_path.c_str(), &(addr.sun_path));

  if (ret < 0)
    return ret;

  ret = connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr));
  if (ret < 0)
    return ret;

  Input syslog(file_path, sock_fd);
  bool finished = false;
  syslog.stream_from_source(finished);

  if (finished){
    std::cout<<"Done!";
  }else{
    std::cout<<"Why not done";
  }
}
