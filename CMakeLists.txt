cmake_minimum_required(VERSION 3.16)
project(FlightSim VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

file(GLOB_RECURSE SOURCES
    "src/app/*.cpp"
    "src/managers/*.cpp"
    "src/aircraft/*.cpp"
    "src/graphics/*.cpp"
    "src/graphics/*.c"
    "src/math/*.cpp"
    "src/world/*.cpp"
)

add_executable(flightsim ${SOURCES})

# Copy assets to the build directory
add_custom_command(TARGET flightsim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:flightsim>/assets
)

# Add the root directory to includes so "math/vec3.hpp" works (as math/ is now at root)
# Also add specific directories if needed, but root is most important for "folder/file.hpp" includes
target_include_directories(flightsim PRIVATE
    ${CMAKE_SOURCE_DIR}/src # Allow includes like "app/simulator.hpp"
    ${CMAKE_SOURCE_DIR}/src/app
    ${CMAKE_SOURCE_DIR}/src/managers
    ${CMAKE_SOURCE_DIR}/src/aircraft
    ${CMAKE_SOURCE_DIR}/src/graphics
    ${CMAKE_SOURCE_DIR}/src/math
    ${CMAKE_SOURCE_DIR}/src/world
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