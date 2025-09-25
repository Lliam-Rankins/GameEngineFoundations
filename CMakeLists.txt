cmake_minimum_required(VERSION 3.10)
project(Project)

set(CMAKE_CXX_STANDARD 17)

find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(cppzmq REQUIRED)

add_executable(client
    ./src/game/main.cpp
    ./src/engine/components/render.cpp
    ./src/engine/components/input.cpp
    ./src/engine/components/entities.cpp
    ./src/engine/components/physics.cpp
    ./src/engine/components/collisions.cpp
    ./src/engine/components/network.cpp
    ./src/engine/components/timeline.cpp
)

target_compile_definitions(client PRIVATE IS_CLIENT)

add_executable(server
    ./src/engine/components/server.cpp
    ./src/engine/components/network.cpp
)

set(MEDIA_SOURCE_DIR "${CMAKE_SOURCE_DIR}/media")
set(MEDIA_DEST_DIR "${CMAKE_BINARY_DIR}/media")

add_custom_command(
    TARGET client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${MEDIA_SOURCE_DIR}"
    "${MEDIA_DEST_DIR}"
)

target_link_libraries(client PRIVATE SDL3::SDL3 SDL3_image::SDL3_image cppzmq)
target_link_libraries(server PRIVATE cppzmq)

