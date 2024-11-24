#include "../common.hpp"
#include "config/config_handler.hpp"
#include "service/service.hpp"
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <thread>

using json = nlohmann::json;

// Constants
constexpr int MAX_EVENTS = 32;
constexpr int BUFFER_SIZE = 1024;

// Global state
std::unordered_map<int, std::string> client_buffers;
std::unordered_map<int, ServiceClient *> client_services;
std::vector<Source *> inputs;
std::unordered_map<std::string, int> input_tag_fd_map;

void set_nonblocking(int sockfd) {
  int flags = fcntl(sockfd, F_GETFL, 0);
  fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

void add_to_epoll(int epoll_fd, int fd, uint32_t events) {
  epoll_event ev{};
  ev.events = events;
  ev.data.fd = fd;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
    throw std::runtime_error("Failed to add to epoll: " +
                             std::string(strerror(errno)));
  }
}

void close_connection(int epoll_fd, int client_fd) {
  epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
  close(client_fd);
  client_buffers.erase(client_fd);
  std::cout << "Client disconnected: " << client_fd << std::endl;
}

void handle_json_message(int epoll_fd, int client_fd, const json &j) {
  if (j.contains(Common::inputStringComand) && inputs.size()) {
    std::string msg;
    for (const auto &[tag, _] : input_tag_fd_map) {
      msg += (tag + "\n");
    }
    send(client_fd, msg.data(), msg.size(), 0);
  } else if (j.contains(Common::selectedInput) && inputs.size()) {
    if (client_services.contains(client_fd)) {
      if (j.contains(Common::selectedInput) &&
          j[Common::selectedInput].is_string()) {
        if (input_tag_fd_map.contains(j[Common::selectedInput])) {
          client_services[client_fd]->inputfd =
              input_tag_fd_map[j[Common::selectedInput].get<std::string>()];
          client_services[client_fd]->haveInput = true;
          std::cout << "Success in changing fd for " << client_fd << "\n";
        }
      }
    }
  }
}

void process_client_buffer(int epoll_fd, int client_fd) {
  std::string &buffer = client_buffers[client_fd];

  try {
    auto j = json::parse(buffer);
    handle_json_message(epoll_fd, client_fd, j);
  } catch (const json::parse_error &e) {
    std::cerr << "Parse failed for client " << client_fd << std::endl;
  }

  buffer.clear();
}

void handle_client_data(int epoll_fd, int client_fd) {
  char buffer[BUFFER_SIZE];
  while (true) {
    ssize_t bytes_read = read(client_fd, buffer, BUFFER_SIZE);
    if (bytes_read == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      }
      close_connection(epoll_fd, client_fd);
      return;
    }
    if (bytes_read == 0) {
      close_connection(epoll_fd, client_fd);
      return;
    }

    client_buffers[client_fd].append(buffer, bytes_read);
  }

  process_client_buffer(epoll_fd, client_fd);
}

void handle_new_connection(int epoll_fd, int server_fd) {
  sockaddr_in client_addr{};
  socklen_t client_len = sizeof(client_addr);

  int client_fd =
      accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
  if (client_fd == -1) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      std::cerr << "Accept failed: " << strerror(errno) << std::endl;
    }
    return;
  }

  set_nonblocking(client_fd);
  add_to_epoll(epoll_fd, client_fd, EPOLLIN | EPOLLET);
  client_buffers[client_fd] = "";
  ServiceClient *new_client = new ServiceClient(client_fd);
  client_services[client_fd] = new_client;
  new_client->run();

  std::cout << "New client connected: " << client_fd << std::endl;
}
void run_server(Source *client) {
  struct sockaddr_storage server_addr;
  int socklen = client->constructSock(&server_addr);

  if (socklen <= 0) {
    throw std::runtime_error("Couldn't construct socket for conn server");
  }

  int server_fd = socket(client->getTypeOfSocket(), SOCK_STREAM, 0);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
      -1) {
    close(server_fd);
    throw std::runtime_error("Failed to bind");
  }

  if (listen(server_fd, SOMAXCONN) == -1) {
    close(server_fd);
    throw std::runtime_error("Failed to listen");
  }

  std::cout << "Core Server started at " << client->getLocation() << '\n';
  int epoll_fd = epoll_create1(0);
  if (epoll_fd == -1) {
    close(server_fd);
    throw std::runtime_error("Failed to create epoll instance");
  }

  set_nonblocking(server_fd);
  add_to_epoll(epoll_fd, server_fd, EPOLLIN | EPOLLET);

  std::vector<epoll_event> events(MAX_EVENTS);
  while (true) {
    int nfds = epoll_wait(epoll_fd, events.data(), MAX_EVENTS, -1);

    for (int i = 0; i < nfds; i++) {
      if (events[i].data.fd == server_fd) {
        handle_new_connection(epoll_fd, server_fd);
      } else {
        handle_client_data(epoll_fd, events[i].data.fd);
      }
    }
  }

  for (const auto &[fd, _] : client_buffers) {
    close(fd);
  }
  close(epoll_fd);
  close(server_fd);
}

void accept_input(int sockfd, Source *inp) {
  int connfd = accept(sockfd, NULL, NULL);
  std::cout << "Input on board at " << connfd << '\n';
  input_tag_fd_map[inp->tag] = connfd;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "Unexpected number of arguments\n";
    exit(EXIT_FAILURE);
  }

  std::string filePath(argv[1]);
  ConfigHandler Config(filePath);

  inputs = Config.getSourceFromInputs();

  for (const auto &inp : inputs) {
    struct sockaddr_storage out;
    int socklen = inp->constructSock(&out);
    int sockfd = socket(inp->getTypeOfSocket(), SOCK_STREAM, 0);

    if (bind(sockfd, (struct sockaddr *)&out, socklen) == 0) {
      listen(sockfd, SOMAXCONN);
      std::thread(accept_input, sockfd, inp).detach();
    } else {
      std::cerr << "Binding failed for " << inp->tag << '\n';
    }
  }

  Source *conn_server = Config.getListenServer();

  run_server(conn_server->clone());
}
