cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED True)

project(network_monitor VERSION 0.1.0)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror -pedantic-errors")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g -O0")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2")
add_executable(network_monitor ./src/main.c)

install(TARGETS network_monitor DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/bin/)
