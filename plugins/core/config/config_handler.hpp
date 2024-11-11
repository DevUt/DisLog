#pragma once

#include <nlohmann/json.hpp>
#include <source/source.hpp>

/**
 * @brief ConfigHandling duties for the Core
 */
class ConfigHandler {
private:
  using json = nlohmann::json;
  json configData;

public:
  /**
   * @brief Construct ConfigHandler
   *
   * @param[in] filePath Path of the configuration file
   */
  ConfigHandler(std::string filePath);

  /**
   * @brief Return `Source` objects for valid sources
   *
   */
  std::vector<Source> getSourceFromInputs();
};
