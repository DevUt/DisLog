#include <nlohmann/json.hpp>
#include <sys/socket.h>

using json = nlohmann::json;

/**
 * @brief String used to getting all the available inputs
 *
 * @return Json formatted string used to get all inputs of a core
 */
std::string input_string();

/**
 * @brief Create a socket for the client
 *
 * @return Length of the socket on success 0 on failure
 */
socklen_t constructSock(struct sockaddr_storage *out);

/**
 * @brief Register this output to the socket
 * @detail This would connect to out socket
 *
 * @param[in] sockfd The socket on where a client is listening
 * @return 0 on success else -1
 */
int register_out(int sockfd);
