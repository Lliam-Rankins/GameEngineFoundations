cmake_minimum_required(VERSION 3.10)
project(FeelingLoopy)

set(CMAKE_CXX_STANDARD 17)

find_package(SDL3 REQUIRED)

add_executable(render ./src/engine/components/render.cpp)

target_link_libraries(render PRIVATE SDL3::SDL3)
