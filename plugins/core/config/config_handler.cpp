#include "config_handler.hpp"
#include <format>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <source/source.hpp>
#include <string>

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
    exit(EXIT_FAILURE);
  }
}

std::vector<Source> ConfigHandler::getSourceFromInputs() {
  std::string_view INPUT = "input";
  std::string_view COMM_TYPE = Source::COMM_TYPE;
  if (!configData.contains(INPUT)) {
    throw std::runtime_error("No input sources!");
  }

  if (!configData[INPUT].is_array()) {
    throw std::runtime_error("Input should be an array!");
  }

  std::vector<Source> result;
  for (auto &source : configData[INPUT]) {
    if (!source.contains(COMM_TYPE)) {
      throw std::runtime_error("Source doesn't define comm_type");
    }
    if (!source[COMM_TYPE].is_string()) {
      throw std::runtime_error(std::format("{} is not string type", COMM_TYPE));
    }
    std::string comm_type = source[COMM_TYPE].get<std::string>();
    if (comm_type == Source::UNIX_STRING) {
      result.push_back(UnixSource(source));
    } else if (comm_type == Source::IPV4_STRING) {
      result.push_back(IPv4Source(source));
    }else{
      result.push_back(UndefinedSource());
    }
  }
  return result;
}
