cmake_minimum_required(VERSION 3.10)
project(DeadCellsClone)

set(CMAKE_CXX_STANDARD 17)

find_package(raylib REQUIRED)

include_directories(include)

add_executable(SideScroller
    main.cpp
    src/Game.cpp
    src/Player.cpp
    src/Map.cpp
    src/MapGeneration.cpp
    src/MapRendering.cpp
    src/MapAutomata.cpp
    src/MapCollisionSpawn.cpp
    src/Camera.cpp
    src/FishEyeGradient.cpp
)

target_link_libraries(SideScroller raylib)
