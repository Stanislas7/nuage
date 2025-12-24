cmake_minimum_required(VERSION 3.16)
project(nuage VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

file(GLOB_RECURSE SOURCES
    "src/core/*.cpp"
    "src/environment/*.cpp"
    "src/input/*.cpp"
    "src/aircraft/*.cpp"
    "src/aircraft/systems/**/*.cpp"
    "src/graphics/*.cpp"
    "src/graphics/*.c"
    "src/math/*.cpp"
    "src/ui/*.cpp"
    "src/ui/font/*.cpp"
    "src/world/*.cpp"
)

add_executable(nuage ${SOURCES})

# Copy assets to the build directory
add_custom_command(TARGET nuage POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:nuage>/assets
)

target_include_directories(nuage PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/environment
    ${CMAKE_SOURCE_DIR}/src/input
    ${CMAKE_SOURCE_DIR}/src/aircraft
    ${CMAKE_SOURCE_DIR}/src/aircraft/systems
    ${CMAKE_SOURCE_DIR}/src/graphics
    ${CMAKE_SOURCE_DIR}/src/math
    ${CMAKE_SOURCE_DIR}/src/ui
    ${CMAKE_SOURCE_DIR}/src/ui/font
    ${CMAKE_SOURCE_DIR}/src/world
    ${CMAKE_SOURCE_DIR}/src/utils
)

target_link_libraries(nuage
    glfw
    OpenGL::GL
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(flightsim PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
endif()