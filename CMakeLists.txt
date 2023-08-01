cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED True)
#LIBGPIOD requires extensions over C90
#set(CMAKE_C_EXTENSIONS OFF)

project(network_monitor VERSION 0.1.0)
set(CMAKE_C_FLAGS "-Wall -Wextra -Werror -pedantic-errors")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -g -O0")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O2")
add_executable(network_monitor ./src/main.c)

add_library(serial STATIC ./src/serial.c ./include/serial.h)

add_library(bg96 STATIC ./src/bg96.c ./include/bg96.h)
target_link_libraries(bg96 serial gpiod m utils)
target_include_directories(bg96 PUBLIC ./include)

add_library(utils STATIC ./src/utils.c ./include/utils.h)
target_link_libraries(utils gpiod)
target_include_directories(utils PUBLIC ./include)

target_include_directories(network_monitor PUBLIC ./include)
target_link_libraries(network_monitor serial bg96 utils)

install(TARGETS network_monitor DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/bin/)
