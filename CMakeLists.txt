cmake_minimum_required(VERSION 3.10)
project(sock2wav)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(sock2wav
        sock2wav.cpp)
