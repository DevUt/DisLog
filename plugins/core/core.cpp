#include "config/config_handler.hpp"
#include <cstdlib>
#include <iostream>
#include <nlohmann/json.hpp>

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "Unexpected number of arguments\n";
    exit(EXIT_FAILURE);
  }

  std::string filePath(argv[0]);
  ConfigHandler Config(filePath);
}
