#include "config_handler.hpp"
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

  if (configStream.fail()) {
    throw std::runtime_error("The configuration file is not accessible");
  }

  try {
    // Allow for comments
    configData = json::parse(filePath, nullptr, true, true);
  } catch (std::exception &e) {
    std::cerr << e.what() << '\n';
    exit(1);
  }
}

/// Return a socket on which input module can communicate
/// with the module for forwarding.
/// The Socket could be an
/// AF_UNIX for Unix socket
/// AF_INET for Ipv4 socket
/// AF_INET6 for IPv6 socket
socklen_t
ConfigHandler::getForwardedSocket(struct sockaddr_storage *socket_out) {
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

/**
 * @brief Construct a unix socket from a configuration file
 *
 * @param[out] socket_out The socket being populated``
 *
 * @return 0 on error `socklen_t` on success
 */
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

  auto unix_sock_file_j = configData[UNIX_STRING][UNIX_FILEPATH];
  if (!unix_sock_file_j.is_string()) {
    std::cerr << std::format("{} is not a string", UNIX_FILEPATH);
    return 0;
  }

  std::string unix_sock_file = unix_sock_file_j.get<std::string>();
  if(unix_sock_file.length() >= 100){
    std::cerr << "FilePath too long can be at max 100 characters";
    return 0;
  }

  struct sockaddr_un *unix_addr = (struct sockaddr_un *)(socket_out);
  unix_addr->sun_family = AF_UNIX;
  std::strncpy(unix_addr->sun_path, unix_sock_file.c_str(), 100);
  return sizeof(*unix_addr);
}
