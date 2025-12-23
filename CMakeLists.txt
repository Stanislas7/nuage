cmake_minimum_required(VERSION 3.16)
project(FlightSim VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

set(SOURCES
    src/main.cpp
    src/glad.c
    src/graphics/shader.cpp
    src/graphics/mesh.cpp
    src/game/terrain.cpp
    src/game/aircraft.cpp
)

add_executable(flightsim ${SOURCES})

target_include_directories(flightsim PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(flightsim
    glfw
    OpenGL::GL
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(flightsim PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
endif()
