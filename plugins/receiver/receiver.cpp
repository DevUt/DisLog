#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <sys/socket.h>

#include "../common.hpp"

#include "receiver.hpp"

using json = nlohmann::json;
std::string input_string() {
  json gInp = json::parse(R"(
                          {
                            "getInputS" : true
                          }
                            )");
  return gInp.dump();
}

std::string getStream() {
  json gStr = json::parse(R"(
                          {
                            "selectedInput" : "SYSLOG"
                          }
                          )");
  return gStr.dump();
}

socklen_t constructSock(struct sockaddr_storage *out) {
  struct sockaddr_in *serv_addr = (struct sockaddr_in *)out;
  serv_addr->sin_family = AF_INET;
  int ret = inet_pton(AF_INET, "127.0.0.1", &serv_addr->sin_addr);
  if (ret <= 0) {
    return 0;
  }
  serv_addr->sin_port = htons(3001);
  return sizeof(*serv_addr);
}

int main() {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_storage serv_addr;
  int socklen = constructSock(&serv_addr);
  if (socklen == 0) {
    exit(EXIT_FAILURE);
  }
  int ret = connect(sockfd, (struct sockaddr *)&serv_addr, socklen);
  if (ret < 0) {
    std::cerr << "Connection failed: " << std::strerror(errno) << "\n";
    exit(EXIT_FAILURE);
  }

  std::cout << "Connection successful\n";
  ret = send(sockfd, input_string().data(), input_string().size(), 0);

  if (ret != input_string().size()) {
    std::cerr << "Failed to send input\n";
    exit(EXIT_FAILURE);
  }

  std::array<char, 1024> read_buf;
  int len = read(sockfd, read_buf.data(), 1024);

  if (len <= 0) {
    std::cerr << "Reading in failed\n";
    exit(EXIT_FAILURE);
  }

  read_buf[len] = '\0';
  std::cout << "Available logs:\n";
  std::cout << read_buf.data() << '\n';

  std::cout << "Press any key to start streaming \n";
  getchar();
  ret = send(sockfd, getStream().data(), getStream().size(), 0);
  if (ret != getStream().size()) {
    std::cerr << "Failed to send input\n";
    exit(EXIT_FAILURE);
  }

  while (true) {

    len = read(sockfd, read_buf.data(), 1024);

    if (len <= 0) {
      std::cerr << "Reading in failed\n";
      exit(EXIT_FAILURE);
    }

    read_buf[len] = '\0';
    std::cout << read_buf.data();
  }
}
