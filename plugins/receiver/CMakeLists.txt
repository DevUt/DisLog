add_executable(receiver
  receiver.cpp)

target_link_libraries(receiver
  PRIVATE
    nlohmann_json::nlohmann_json
)
