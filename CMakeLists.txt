cmake_minimum_required(VERSION 3.16)

project(camera_asio)

set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR} ${CMAKE_MODULE_PATH})
set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR} ${CMAKE_PREFIX_PATH})

add_subdirectory(common)
add_subdirectory(client)
add_subdirectory(server)