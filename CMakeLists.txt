set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON) # Ensure it fails if 17 isn't supported
set(CMAKE_CXX_EXTENSIONS OFF)
cmake_minimum_required(VERSION 3.10)
project(Project)

find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(cppzmq REQUIRED)

# add_executable(client
#     ./src/game/main.cpp
#     ./src/engine/components/render.cpp
#     ./src/engine/components/input.cpp
#     ./src/engine/components/entities.cpp
#     ./src/engine/components/physics.cpp
#     ./src/engine/components/collisions.cpp
#     ./src/engine/components/network.cpp
#     ./src/engine/components/timeline.cpp
#     ./src/engine/components/GameObject.cpp
#     ./src/engine/components/EventManager.cpp
#     ./src/engine/components/recording.cpp
#     ./src/engine/components/GameObjectPool.cpp
#     ./src/engine/components/CustomAllocator.cpp
# )

add_executable(heck
    ./src/game/projectileHeck.cpp
    ./src/engine/components/render.cpp
    ./src/engine/components/input.cpp
    ./src/engine/components/entities.cpp
    ./src/engine/components/physics.cpp
    ./src/engine/components/mathEngine.cpp
    ./src/engine/components/collisions.cpp
    ./src/engine/components/network.cpp
    ./src/engine/components/timeline.cpp
    ./src/engine/components/GameObject.cpp
    ./src/engine/components/EventManager.cpp
    ./src/engine/components/recording.cpp
    ./src/engine/components/GameObjectPool.cpp
    ./src/engine/components/CustomAllocator.cpp
)

# target_compile_definitions(client PRIVATE IS_CLIENT)

add_executable(server
    ./src/game/server.cpp
    ./src/engine/components/network.cpp
    ./src/engine/components/entities.cpp
    ./src/engine/components/timeline.cpp
    ./src/engine/components/mathEngine.cpp
    ./src/engine/components/GameObject.cpp
    ./src/engine/components/EventManager.cpp
    ./src/engine/components/collisions.cpp
)

set(MEDIA_SOURCE_DIR "${CMAKE_SOURCE_DIR}/media")
set(MEDIA_DEST_DIR "${CMAKE_BINARY_DIR}/media")

# add_custom_command(
#     TARGET client POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#     "${MEDIA_SOURCE_DIR}"
#     "${MEDIA_DEST_DIR}"
# )

# target_link_libraries(client PRIVATE SDL3::SDL3 SDL3_image::SDL3_image cppzmq)
target_link_libraries(heck PRIVATE SDL3::SDL3 SDL3_image::SDL3_image cppzmq)
target_link_libraries(server PRIVATE cppzmq)