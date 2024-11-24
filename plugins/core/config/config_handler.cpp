#include "config_handler.hpp"
#include <format>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>
#include <source/source.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

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

std::vector<Source *> ConfigHandler::getSourceFromTag(std::string_view TAG) {
  std::string_view COMM_TYPE = Source::COMM_TYPE;
  if (!configData.contains(TAG)) {
    throw std::runtime_error("No input sources!");
  }

  if (!configData[TAG].is_array()) {
    throw std::runtime_error("Input should be an array!");
  }

  std::vector<Source *> result;
  std::set<std::string> sawTag;
  for (auto &source : configData[TAG]) {
    if (!source.contains(COMM_TYPE)) {
      throw std::runtime_error("Source doesn't define comm_type");
    }
    if (!source[COMM_TYPE].is_string()) {
      throw std::runtime_error(std::format("{} is not string type", COMM_TYPE));
    }
    std::string comm_type = source[COMM_TYPE].get<std::string>();

    if (comm_type == Source::UNIX_STRING) {
      UnixSource unix_src = UnixSource(source);
      unix_src.isInput = true;

      if (sawTag.contains(unix_src.tag)) {
        throw std::runtime_error(
            std::format("Duplicate tags {}", unix_src.tag));
      } else {
        sawTag.insert(unix_src.tag);
      }

      result.emplace_back(unix_src.clone());
    } else if (comm_type == Source::IPV4_STRING) {

      IPv4Source ipv4source = IPv4Source(source);
      ipv4source.isInput = true;

      if (sawTag.contains(ipv4source.tag)) {
        throw std::runtime_error(
            std::format("Duplicate tags {}", ipv4source.tag));
      } else {
        sawTag.insert(ipv4source.tag);
      }

      result.emplace_back(ipv4source.clone());
    } else {
      result.push_back(UndefinedSource().clone());
    }
  }
  return result;
}

Source *ConfigHandler::getCommandServer() {
  std::vector<Source *> ret = getSourceFromTag(Source::CMD_SERVER);
  if(ret.size() != 1) {
    throw std::runtime_error("Only one command server in array");
  }
  return ret[0];
}


Source *ConfigHandler::getListenServer() {
  std::vector<Source *> ret = getSourceFromTag(Source::CONN_SERVER);
  if(ret.size() != 1) {
    throw std::runtime_error("Only one command server in array");
  }
  return ret[0];
}

std::vector<Source *> ConfigHandler::getSourceFromInputs() {
  std::string INPUT = "input";
  return getSourceFromTag(INPUT);
}

