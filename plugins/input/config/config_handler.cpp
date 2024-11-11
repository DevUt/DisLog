#include "config_handler.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <exception>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>

/// Create the configuration Handler class
ConfigHandler::ConfigHandler(std::string filePath) {
  std::ifstream configStream(filePath);

  if (configStream.bad()) {
    throw std::runtime_error("The configuration file is not accessible");
  }

  try {
    // Allow for comments
    configData = json::parse(configStream);
  } catch (std::exception &e) {
    std::cerr << e.what() << '\n';
    exit(1);
  }
}

int ConfigHandler::getSocketType() {
  constexpr std::string_view COMM_TYPE = "comm_type";
  if (!configData.contains(COMM_TYPE)) {
    std::cerr << std::format("Config does not contain {} ", COMM_TYPE);
    return -1;
  }

  auto comm_type_j = configData[COMM_TYPE];
  if (!comm_type_j.is_string()) {
    std::cerr << std::format("{} value is not string", COMM_TYPE);
    return -1;
  }

  if (comm_type_j == "UNIX_SOCK")
    return AF_UNIX;
  else if (comm_type_j == "IPv4")
    return AF_INET;
  else
    // Unsupported
    return -1;
}

/// Return a socket on which input module can communicate
/// with the module for forwarding.
/// Configuration contains a json `comm_type : <json_strin>`
/// `UNIX_SOCK` for Unix socket
/// `IPv4` for Ipv4 socket
socklen_t
ConfigHandler::getForwardedSocketStore(struct sockaddr_storage *socket_out) {
  constexpr std::string_view COMM_TYPE = "comm_type";
  if (!configData.contains(COMM_TYPE)) {
    std::cerr << std::format("Config does not contain {} ", COMM_TYPE);
    return 0;
  }

  auto comm_type_j = configData[COMM_TYPE];
  if (!comm_type_j.is_string()) {
    std::cerr << std::format("{} value is not string", COMM_TYPE);
    return 0;
  }

  std::string comm_type = comm_type_j.get<std::string>();
  // I could use hash based switch here
  // Or enum tactics what to do?
  if (comm_type == "UNIX_SOCK") {
    return getUnixSocket(socket_out);
  } else if (comm_type == "IPv4") {
    return getIPv4Socket(socket_out);
  } else {
    std::cerr << "Unsupported communication type";
    return 0;
  }
}

/// Create a unix socket from a path
/// `"UNIX_SOCK" :` Unix socket object
/// `"sock_file_path"` : Socket file path
socklen_t ConfigHandler::getUnixSocket(struct sockaddr_storage *socket_out) {
  constexpr std::string_view UNIX_STRING = "UNIX_SOCK";
  constexpr std::string_view UNIX_FILEPATH = "sock_file_path";

  if (!configData.contains(UNIX_STRING)) {
    std::cerr << std::format("Config does not contain unix config `{}`",
                             UNIX_STRING);
    return 0;
  }

  auto unix_obj_j = configData[UNIX_STRING];
  if (!unix_obj_j.is_object()) {
    std::cerr << std::format("{} is not a valid object!", UNIX_STRING);
    return 0;
  }

  if (!unix_obj_j.contains(UNIX_FILEPATH)) {
    std::cerr << "Unix socket file path not defined";
    return 0;
  }

  auto unix_sock_file_j = unix_obj_j[UNIX_FILEPATH];
  if (!unix_sock_file_j.is_string()) {
    std::cerr << std::format("{} is not a string", UNIX_FILEPATH);
    return 0;
  }

  std::string unix_sock_file = unix_sock_file_j.get<std::string>();
  if (unix_sock_file.length() >= 100) {
    std::cerr << "FilePath too long can be at max 100 characters";
    return 0;
  }

  struct sockaddr_un *unix_addr = (struct sockaddr_un *)(socket_out);
  unix_addr->sun_family = AF_UNIX;
  std::strncpy(unix_addr->sun_path, unix_sock_file.c_str(), 100);
  return sizeof(*unix_addr);
}

/// Create an IPv4 Socket
socklen_t ConfigHandler::getIPv4Socket(struct sockaddr_storage *socket_out) {
  constexpr std::string_view IPV4_STRING = "IPv4";
  constexpr std::string_view URI = "uri";
  constexpr std::string_view PORT = "port";

  if (!configData.contains(IPV4_STRING)) {
    std::cerr << std::format("Config does not contain IPv4 config `{}`",
                             IPV4_STRING);
    return 0;
  }

  auto ipv4_obj_j = configData[IPV4_STRING];
  if (!ipv4_obj_j.is_object()) {
    std::cerr << std::format("{} is not a valid object!", IPV4_STRING);
    return 0;
  }

  if (!ipv4_obj_j.contains(URI)) {
    std::cerr << "Ipv4 URI config not found!";
    return 0;
  }

  if (!ipv4_obj_j.contains(PORT)) {
    std::cerr << "IPv4 port config not found";
    return 0;
  }

  auto ipv4_uri_j = ipv4_obj_j[URI];
  if (!ipv4_uri_j.is_string()) {
    std::cerr << "IPv4 URI is not a string!";
    return 0;
  }

  std::string ipv4_uri = ipv4_uri_j.get<std::string>();

  auto ipv4_port_j = ipv4_obj_j[PORT];
  if (!ipv4_port_j.is_number()) {
    std::cerr << "IPv4 port is not number!";
    return 0;
  }

  unsigned int port_no = ipv4_port_j.get<unsigned int>();

  struct sockaddr_in *ipv4_addr = (struct sockaddr_in *)(socket_out);
  memset(ipv4_addr, 0, sizeof(*ipv4_addr));
  ipv4_addr->sin_family = AF_INET;
  ipv4_addr->sin_port = htons(port_no);

  char *temp_addr = (char *)malloc(sizeof(char) * 2000);
  strcpy(temp_addr, ipv4_uri.c_str());
  inet_pton(AF_INET, temp_addr, &(ipv4_addr->sin_addr));
  return sizeof(*ipv4_addr);
}
