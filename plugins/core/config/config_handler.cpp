#include "config_handler.hpp"
#include <format>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>
#include <source/source.hpp>
#include <stdexcept>
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

std::vector<Source *> ConfigHandler::getSourceFromInputs() {
  std::string_view INPUT = "input";
  std::string_view COMM_TYPE = Source::COMM_TYPE;
  if (!configData.contains(INPUT)) {
    throw std::runtime_error("No input sources!");
  }

  if (!configData[INPUT].is_array()) {
    throw std::runtime_error("Input should be an array!");
  }

  std::vector<Source *> result;
  std::set<std::string> sawTag;
  for (auto &source : configData[INPUT]) {
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
      if (!source.contains("output_to") || !source["output_to"].is_array()) {
        throw std::runtime_error(
            std::format("Output is not well defined for {}", unix_src.tag));
      }

      std::vector<std::string> outputs;
      for (auto &outs : source["output_to"]) {
        if (outs.is_string()) {
          outputs.push_back(outs);
        } else {
          std::cerr << std::format(
              "Skipping invalid out config due to type mismatch for {}",
              unix_src.tag);
        }
      }

      unix_src.output = outputs;

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

      if (!source.contains("output_to") || !source["output_to"].is_array()) {
        throw std::runtime_error(
            std::format("Output is not well defined for {}", ipv4source.tag));
      }

      std::vector<std::string> outputs;
      for (auto &outs : source["output_to"]) {
        if (outs.is_string()) {
          outputs.push_back(outs);
        } else {
          std::cerr << std::format(
              "Skipping invalid out config due to type mismatch for {}",
              ipv4source.tag);
        }
      }

      ipv4source.output = outputs;
      result.emplace_back(ipv4source.clone());
    } else {
      result.push_back(UndefinedSource().clone());
    }
  }
  return result;
}

std::vector<Source *> ConfigHandler::getSourceForOutputs() {
  std::string_view OUTPUT = "output";
  std::string_view COMM_TYPE = Source::COMM_TYPE;
  if (!configData.contains(OUTPUT)) {
    throw std::runtime_error("No output sources!");
  }

  if (!configData[OUTPUT].is_array()) {
    throw std::runtime_error("Output should be an array!");
  }

  std::vector<Source *> result;
  std::set<std::string> sawTag;
  for (auto &source : configData[OUTPUT]) {
    if (!source.contains(COMM_TYPE)) {
      throw std::runtime_error("Source doesn't define comm_type");
    }
    if (!source[COMM_TYPE].is_string()) {
      throw std::runtime_error(std::format("{} is not string type", COMM_TYPE));
    }
    std::string comm_type = source[COMM_TYPE].get<std::string>();
    if (comm_type == Source::UNIX_STRING) {
      UnixSource unix_src = UnixSource(source);
      unix_src.isOutput = true;

      if (sawTag.contains(unix_src.tag)) {
        throw std::runtime_error(
            std::format("Duplicate tags {}\n", unix_src.tag));
      } else {
        sawTag.insert(unix_src.tag);
      }
      result.emplace_back(unix_src.clone());
    } else if (comm_type == Source::IPV4_STRING) {
      IPv4Source ipv4_source = IPv4Source(source);
      ipv4_source.isOutput = true;
      if (sawTag.contains(ipv4_source.tag)) {
        throw std::runtime_error(
            std::format("Duplicate tags {}\n", ipv4_source.tag));
      } else {
        sawTag.insert(ipv4_source.tag);
      }
      result.emplace_back(ipv4_source.clone());
    } else {
      std::cout << std::format("Undefined {}\n", COMM_TYPE);
      // result.push_back(UndefinedSource());
    }
  }
  return result;
}
