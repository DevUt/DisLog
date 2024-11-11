add_executable(core
  core.cpp 
  config/config_handler.cpp
  service/service.cpp
)

add_subdirectory(source)

target_link_libraries(core
 PUBLIC 
    nlohmann_json::nlohmann_json
    source
)

