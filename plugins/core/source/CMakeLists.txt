add_library(source
  source.hpp
)

target_include_directories(source
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

set_target_properties(source PROPERTIES
  PUBLIC_HEADER "source.hpp"
  LINKER_LANGUAGE CXX
)

target_link_libraries(source
  PUBLIC
    nlohmann_json::nlohmann_json
)

install(TARGETS source
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  PUBLIC_HEADER DESTINATION include/source
)
