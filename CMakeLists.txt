cmake_minimum_required(VERSION 3.10)
project(DeadCellsClone)

set(CMAKE_CXX_STANDARD 17)

find_package(raylib REQUIRED)

include_directories(include)


# Find all source files
file(GLOB_RECURSE SOURCES 
    "src/**/*.cpp"
    "src/*.cpp"
    "main.cpp"
)

message(STATUS "Source files found: ${SOURCES}")
# Add shaders
file(GLOB SHADERS "shader/*.fs")

# Create executable
add_executable(SideScroller ${SOURCES})

# Include resources
file(COPY ${SHADERS} DESTINATION ${CMAKE_BINARY_DIR}/shader)
file(COPY resources DESTINATION ${CMAKE_BINARY_DIR})

target_link_libraries(SideScroller raylib)