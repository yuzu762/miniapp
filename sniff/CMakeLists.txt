cmake_minimum_required(VERSION 3.15)
project(sniff C)

set(CMAKE_C_STANDARD 11)

add_executable(sniff main.c sniffer.c)