cmake_minimum_required(VERSION 3.23)

project(DisLog)

set(CMAKE_EXPORT_COMPILE_COMMANDS True)
execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
)

add_subdirectory(plugins/input)
