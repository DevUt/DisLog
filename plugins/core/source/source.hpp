#pragma once

#include <arpa/inet.h>
#include <format>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>

/**
 * @brief Base Class for different type of sources
 * @note Currently this only supports different types of socket but
 *        want to extend this in future
 */
class Source {
public:
  constexpr static std::string_view UNIX_STRING = "UNIX_SOCK";
  constexpr static std::string_view COMM_TYPE = "comm_type";
  constexpr static std::string_view IPV4_STRING = "IPv4";
  constexpr static std::string_view CMD_SERVER = "cmd_server";
  constexpr static std::string_view CONN_SERVER = "conn_server";

  /// Tag for the source
  std::string tag;

  /**
   * @brief It constructs a socket address and returns
   *
   * @param[out] out `sockaddr_storage` setup with the socket
   * @return 0 on error else sizeof socket
   */
  virtual socklen_t constructSock(struct sockaddr_storage *out) const {
    return 0;
  };

  /**
   * @brief Get the type of Socket. Currently 'AF_UNIX' or 'AF_INET'
   *
   * @return return the domain. or -1 if Invalid instantiation
   */
  virtual int getTypeOfSocket() const { return -1; };

  /**
   * @brief Gets you information on where the socket is running
   *
   * @return string containing information on the socket
   */
  virtual std::string getLocation() const { return "Invalid ACCESS HOW??"; }

  /**
   * @brief If this source requires any cleanup this does it
   *
   */
  virtual void cleanUp() const {}

  /**
   * @brief Create a clone
   *
   * @return [TODO:description]
   */
  virtual Source *clone() { return new Source; }

  /**
   * @isInput Is the source block a input source
   *
   */
  bool isInput = false;

  /**
   * @isOutput Is the source block an output source
   *
   */
  bool isOutput = false;
};

/**
 * @brief Defining a Unix Socket source
 */
class UnixSource : public Source {
private:
  using json = nlohmann::json;

  std::string_view UNIX_FILEPATH = "sock_file_path";

public:
  std::string socket_file_path;

  UnixSource(nlohmann::basic_json<> sourceBlock) : Source() {
    if (!sourceBlock.contains("tag")) {
      throw std::runtime_error("Tag not provided for the block\n");
    }

    if (!sourceBlock["tag"].is_string()) {
      throw std::runtime_error("Tag type is not string");
    }

    tag = sourceBlock["tag"].get<std::string>();

    if (!sourceBlock.contains(COMM_TYPE)) {
      throw std::runtime_error(
          "How did we end up here? communication type is not defined!");
    }

    if (!sourceBlock[COMM_TYPE].is_string() &&
        sourceBlock[COMM_TYPE].get<std::string>() != UNIX_STRING) {
      throw std::runtime_error(
          "How did we end up here? communication type is not valid!");
    }

    if (!sourceBlock.contains(UNIX_STRING)) {
      throw std::runtime_error(
          "No Unix socket info found! Check if UNIX_SOCK defined");
    }

    auto unix_detail_j = sourceBlock[UNIX_STRING];

    if (!unix_detail_j.contains(UNIX_FILEPATH)) {
      throw std::runtime_error("No Unix File Path Found!");
    }

    if (!unix_detail_j[UNIX_FILEPATH].is_string()) {
      throw std::runtime_error("Socket filepath is not a string!");
    }

    socket_file_path = unix_detail_j[UNIX_FILEPATH].get<std::string>();
  }

  /**
   * @brief Setup socket for UNIX sockets
   *
   * @param[out] out `sockaddr_storage`
   * @return sizeof the socket:w
   *
   */
  socklen_t constructSock(struct sockaddr_storage *out) const override {
    struct sockaddr_un *unix_addr = (struct sockaddr_un *)(out);
    unix_addr->sun_family = AF_UNIX;
    std::strncpy(unix_addr->sun_path, socket_file_path.c_str(), 100);
    return sizeof(*unix_addr);
  }

  /**
   * @brief Inform that this is a UNIX Socket
   *
   * @return `AF_UNIX`
   */
  int getTypeOfSocket() const override { return AF_UNIX; }

  /**
   * @brief Gets you the socket file path
   *
   * @return The socket file path
   */
  std::string getLocation() const override { return socket_file_path; }

  /**
   * @brief Remove the socket file
   *
   */
  void cleanUp() const override { std::filesystem::remove(socket_file_path); }

  Source *clone() override { return new UnixSource(*this); }
};

/**
 * @brief Class for modelling an IPv4 Source
 */
class IPv4Source : public Source {
private:
  using json = nlohmann::json;
  std::string_view URI = "uri";
  std::string_view PORT = "port";

public:
  std::string uri;
  int port;

  IPv4Source(nlohmann::basic_json<> sourceBlock) : Source() {

    if (!sourceBlock.contains("tag")) {
      throw std::runtime_error("Tag not provided for the block\n");
    }

    if (!sourceBlock["tag"].is_string()) {
      throw std::runtime_error("Tag type is not string");
    }

    tag = sourceBlock["tag"].get<std::string>();
    if (!sourceBlock.contains(COMM_TYPE)) {
      throw std::runtime_error(
          "How did we end up here? communication type is not defined!");
    }

    if (!sourceBlock[COMM_TYPE].is_string() &&
        sourceBlock[COMM_TYPE].get<std::string>() != IPV4_STRING) {
      throw std::runtime_error(
          "How did we end up here? communication type is not valid!");
    }

    if (!sourceBlock.contains(IPV4_STRING)) {
      throw std::runtime_error(
          "No Unix socket info found! Check if UNIX_SOCK defined");
    }

    auto ipv4_details_j = sourceBlock[IPV4_STRING];

    if (!ipv4_details_j.contains(URI)) {
      throw std::runtime_error("URI not found in config");
    }

    if (!ipv4_details_j[URI].is_string()) {
      throw std::runtime_error("URI not a string in config");
    }

    if (!ipv4_details_j.contains(PORT)) {
      throw std::runtime_error("PORT not found in config");
    }

    if (!ipv4_details_j[PORT].is_number()) {
      throw std::runtime_error("PORT not a number in config");
    }

    uri = ipv4_details_j[URI].get<std::string>();
    port = ipv4_details_j[PORT].get<int>();
  }

  /**
   * @brief Construct a IPv4 socket
   *
   * @param[out] out The constructed socket
   * @return size of the socket
   */
  socklen_t constructSock(struct sockaddr_storage *out) const override {
    struct sockaddr_in *ipv4_addr = (struct sockaddr_in *)(out);
    memset(ipv4_addr, 0, sizeof(*ipv4_addr));
    ipv4_addr->sin_family = AF_INET;
    ipv4_addr->sin_port = htons(port);

    char *temp_addr = (char *)malloc(sizeof(char) * 2000);
    strcpy(temp_addr, uri.c_str());
    inet_pton(AF_INET, temp_addr, &(ipv4_addr->sin_addr));
    return sizeof(*ipv4_addr);
  }

  /**
   * @brief Inform that this is a IPv4 Socket
   *
   * @return `AF_INET`
   */
  int getTypeOfSocket() const override { return AF_INET; }

  /**
   * @brief Gets uri and port number
   *
   * @return A string having {uri} : {port}
   */
  std::string getLocation() const override {
    return std::format("{} : {}", uri, port);
  }

  Source * clone() override {
    return  new IPv4Source(*this);
  }
};

/**
 * @brief It is an invalid Source
 */
class UndefinedSource : public Source {
public:
  UndefinedSource() : Source() {};

  /**
   * @brief It indicates that it is a invalid source.
   *
   * @return -1
   */
  int getTypeOfSocket() const override { return -1; }

  /**
   * @brief Return 0 to indicate invalid source
   *
   * @param[out] out Won't be modified
   * @return 0
   */
  socklen_t constructSock(struct sockaddr_storage *out) const override {
    return 0;
  }

  /**
   * @brief Indicates its an invalid source it should never reach this stage
   * @return String saying its invalid
   */
  std::string getLocation() const override { return "Invalid source"; }
};
