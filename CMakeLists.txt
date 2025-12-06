cmake_minimum_required(VERSION 3.14)
project(Project VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL3 REQUIRED)

set(ENGINE_SOURCES
    src/engine/components/CustomAllocator.cpp
    src/engine/components/EventManager.cpp
    src/engine/components/GameObject.cpp
    src/engine/components/GameObjectPool.cpp
    src/engine/components/JobSystem.cpp
    src/engine/components/collisions.cpp
    src/engine/components/entities.cpp
    src/engine/components/input.cpp
    src/engine/components/mathEngine.cpp
    src/engine/components/physics.cpp
    src/engine/components/render.cpp
    src/engine/components/timeline.cpp
)

set(SPACE_INVADERS_SOURCES
    src/game/SpaceInvaders/SpaceInvadersGame.cpp
)

set(BUBBLE_SHOOTER_SOURCES
    src/game/BubbleShooter/BubbleShooter.cpp
)

# Tells the compiler where to look for .h files
include_directories(
    headers
    SpaceInvaders
    BubbleShooter
)

add_executable(${PROJECT_NAME}
    ${ENGINE_SOURCES}
    ${SPACE_INVADERS_SOURCES}
    ${BUBBLE_SHOOTER_SOURCES}
    src/game/main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)