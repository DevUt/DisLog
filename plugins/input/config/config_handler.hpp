#include <nlohmann/json.hpp>
#include <sys/socket.h>

class ConfigHandler {
public:
  /**
   * @brief Construct a configHandler with a path to config file
   *
   * @param[in] confilePath path to the config file
   */
  ConfigHandler(std::string confilePath);

  /**
   * @brief Construct a socket on where the core is already listening
   * @details Expects json entry "comm_type : <json_string>"
   *            The valid options are `UNIX_SOCKET` | `IPv4`
   *            Support for Ipv6 is underway
   *
   * @param[out] socket_out This storage will be populated
   *
   * @returns 0 if error else size of socket created.
   */
  socklen_t getForwardedSocketStore(struct sockaddr_storage *socket_out);

  /**
   * @brief What type of socket has been given in configuration file
   * @note It assumes that `typeof(AF_*)` is int
   *
   * @return Type of socket which can be directly plugged in `socket()` on success
   *          else -1
   */
  int getSocketType();

private:
  using json = nlohmann::json;
  json configData;

  /**
   * @brief Construct a Unix Socket according to the configuration file
   *
   * @param[out] socket_out Socket being populated
   * @return 0 on error else size of socket
   */
  socklen_t getUnixSocket(struct sockaddr_storage *socket_out);

  /**
   * @brief Construct a Ipv4 socket according to the configuration file
   *
   * @param[out] socket_out This is the socket that will be populated.
   * @return 0 on error else size of socket
   */
  socklen_t getIPv4Socket(struct sockaddr_storage *socket_out);
};
