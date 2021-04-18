cmake_minimum_required(VERSION 3.15)
project(chat)
include_directories(./)
set(CMAKE_CXX_STANDARD 14)

add_executable(client client.cpp)
add_executable(server server.cpp)

