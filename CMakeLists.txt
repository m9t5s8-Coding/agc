cmake_minimum_required(VERSION 3.20)

project(agc)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE  APP_SRC "${CMAKE_SOURCE_DIR}/src/*cpp")
add_executable(agc ${APP_SRC})

target_include_directories(agc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

install(TARGETS agc DESTINATION /usr/local/bin)
