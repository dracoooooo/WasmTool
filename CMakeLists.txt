cmake_minimum_required(VERSION 3.16)
project(wasmtool)

set(CMAKE_CXX_STANDARD 11)

add_executable(wasmtool src/wasmtool.cpp)
target_link_libraries(wasmtool -lwasmedge)
