add_executable(input 
  config/config_handler.cpp
  input/input.cpp 
  main.cpp)

target_link_libraries(input
  PRIVATE
    nlohmann_json::nlohmann_json
)
