#include "input.hpp"

#include <array>
#include <fcntl.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/// This reads from the source file
/// Currently this version doesn't provide much version checking
int Input::stream_from_source(int start_offset) {
  int fd = open(file_path.c_str(), O_RDONLY);
  if (fd < 0)
    return fd; // Return the error back to the caller.

  // Tell the kernel how I will be accessing the memory
  posix_fadvise(fd, 0, start_offset, POSIX_FADV_SEQUENTIAL);
  lseek(fd, start_offset, SEEK_SET);

  int ret = 0;
  while (true) {
    size_t b_read = read(fd, buf.data(), BUF_SIZE);
    if (b_read == -1)
      return -1;       // We failed again

    ret = stream(b_read);
    if (ret < 0)
      return ret; // If we fail
  }
  return 0; // We are sucessful
}

/// Attempt to send all the data that has been read
int Input::stream(int b_read) {
  size_t tot_sent = 0;
  while (tot_sent < b_read) {
    ssize_t sent = send(socket_fd, buf.data() + tot_sent, b_read - tot_sent, 0);

    if (sent < 0)
      return sent;
    tot_sent += sent;
  }
  return 0;
}
