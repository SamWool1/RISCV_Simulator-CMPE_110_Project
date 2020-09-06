cmake_minimum_required(VERSION 3.12)
project(riscvsim C)

set(CMAKE_C_STANDARD 11)

include_directories(src)

file(GLOB primary_src src/*.h src/*.c)

add_executable(riscvsim ${primary_src})

target_link_libraries(riscvsim m)