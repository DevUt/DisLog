#include "config/config_handler.hpp"
#include "service/service.hpp"
#include <cstdlib>
#include <format>
#include <iostream>
#include <nlohmann/json.hpp>
#include <thread>

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "Unexpected number of arguments\n";
    exit(EXIT_FAILURE);
  }

  std::string filePath(argv[1]);
  ConfigHandler Config(filePath);

  std::vector<Source *> inputs = Config.getSourceFromInputs();
  std::vector<Source *> outputs = Config.getSourceForOutputs();

  std::unordered_map<std::string, Source *> tag_output_match;
  for (auto &source : outputs) {
    if (!source->tag.empty())
      tag_output_match[source->tag] = source;
  }

  std::vector<std::thread> service_able;
  for (auto &input : inputs) {
    std::vector<Source *> outputSources;
    for (const std::string &out_tags : input->output) {
      if (tag_output_match.contains(out_tags)) {
        outputSources.emplace_back(tag_output_match[out_tags]);
      } else {
        std::cerr << std::format(
            "Output source for input tag {} is unaavailable!\n", input->tag);
      }
    }
    std::thread servicing(service, input->clone(), outputSources);
    service_able.emplace_back(std::move(servicing));
  }

  for (auto &th : service_able) {
    th.join();
  }
}
