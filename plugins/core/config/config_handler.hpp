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

  /**
   * @brief Return `Source` objects from a particular array
   *
   */
  std::vector<Source *> getSourceFromTag(std::string_view TAG);

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
  std::vector<Source* > getSourceFromInputs();

  // std::vector<Source* > getSourceForOutputs();


  /**
   * @brief Get information about command server
   *
   */
  Source * getCommandServer();


  /**
   * @brief Get information about where to listen
   *
   */
  Source * getListenServer();
};
