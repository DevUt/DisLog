#pragma once

#include <array>
#include <string>

/**
 * @brief Class supporting to take input
 */
class Input {
public:
  /**
   * @brief Stream a file.
   *
   * @param[out] finished Indicate if reading has finished
   * @param[in] start_offset From where in file should we stream
   * @return 0 on sucess and non zero on failure
   */
  int stream_from_source(bool &finished, int start_offset = 0);

  /**
   * @brief Construct Input Class
   *
   * @param[in] file_path File path of the log
   * @param[int] socket_fd Socket over which the data would be sent
   */
  Input(const std::string file_path, const int socket_fd)
      : file_path(file_path), socket_fd(socket_fd) {};

private:
  const std::string file_path;
  static const int BUF_SIZE = 1024;
  const int socket_fd;
  std::array<char, BUF_SIZE> buf;

  /**
   * @brief Stream data to socket
   *
   * @note The data it sends is in `buf`
   *
   * @param[in] bytes_read How many bytes to send
   * @return < 0 if error 0 if successful
   */
  int stream(int bytes_read);
};
