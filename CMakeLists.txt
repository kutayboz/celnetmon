cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED True)
#LIBGPIOD requires extensions over C90
#set(CMAKE_C_EXTENSIONS OFF)

project(celnetmon VERSION 1.0.0)
set(CMAKE_C_FLAGS "-Wall -Wextra -Werror -pedantic-errors")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -g -O0")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O2")
add_executable(celnetmon ./src/main.c)

add_library(serial STATIC ./src/serial.c ./include/serial.h)

add_library(bg96 STATIC ./src/bg96.c ./include/bg96.h)
target_link_libraries(bg96 serial gpiod m utils)
target_include_directories(bg96 PUBLIC ./include)

add_library(utils STATIC ./src/utils.c ./include/utils.h)
target_link_libraries(utils gpiod)
target_include_directories(utils PUBLIC ./include)

target_include_directories(celnetmon PUBLIC ./include)
target_link_libraries(celnetmon serial bg96 utils)

install(TARGETS celnetmon DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/bin/)
