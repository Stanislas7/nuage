cmake_minimum_required(VERSION 3.16)
project(nuage VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

# JSBSim options (keep the dependency lean)
set(BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(BUILD_PYTHON_MODULE OFF CACHE BOOL "" FORCE)
set(BUILD_JULIA_PACKAGE OFF CACHE BOOL "" FORCE)
set(BUILD_MATLAB_SFUNCTION OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(external/jsbsim EXCLUDE_FROM_ALL)

file(GLOB_RECURSE SOURCES
    "src/core/*.cpp"
    "src/environment/*.cpp"
    "src/input/*.cpp"
    "src/scenery/*.cpp"
    "src/aircraft/*.cpp"
    "src/aircraft/systems/**/*.cpp"
    "src/graphics/*.cpp"
    "src/graphics/*.c"
    "src/math/*.cpp"
    "src/ui/*.cpp"
    "src/ui/**/*.cpp"
)

add_executable(nuage ${SOURCES})

add_executable(terrainc
    tools/terrainc.cpp
    tools/terrainc/heightmap.cpp
    tools/terrainc/color_ramp.cpp
    tools/terrainc/mask_smoothing.cpp
)
target_include_directories(terrainc PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/math
    ${CMAKE_SOURCE_DIR}/src/utils
)

# Copy assets to the build directory
set(JSBSIM_MODELS c172p)

set(copy_assets_commands
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    ${CMAKE_BINARY_DIR}/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/external/jsbsim/engine
    ${CMAKE_BINARY_DIR}/assets/jsbsim/engine
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/external/jsbsim/systems
    ${CMAKE_BINARY_DIR}/assets/jsbsim/systems
)

foreach(model ${JSBSIM_MODELS})
    list(APPEND copy_assets_commands
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/external/jsbsim/aircraft/${model}
        ${CMAKE_BINARY_DIR}/assets/jsbsim/aircraft/${model}
    )
endforeach()

add_custom_target(copy_assets ALL ${copy_assets_commands})
add_dependencies(nuage copy_assets)

target_include_directories(nuage PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/environment
    ${CMAKE_SOURCE_DIR}/src/input
    ${CMAKE_SOURCE_DIR}/src/scenery
    ${CMAKE_SOURCE_DIR}/src/aircraft
    ${CMAKE_SOURCE_DIR}/src/aircraft/systems
    ${CMAKE_SOURCE_DIR}/src/graphics
    ${CMAKE_SOURCE_DIR}/src/math
    ${CMAKE_SOURCE_DIR}/src/ui
    ${CMAKE_SOURCE_DIR}/src/ui/font
    ${CMAKE_SOURCE_DIR}/src/utils
    ${CMAKE_SOURCE_DIR}/external/jsbsim/src
)

target_link_libraries(nuage
    glfw
    OpenGL::GL
    libJSBSim
)

# Avoid building JSBSim utilities/binaries on every build.
if (TARGET JSBSim)
    set_target_properties(JSBSim PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()
if (TARGET Aeromatic++)
    set_target_properties(Aeromatic++ PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()
if (TARGET aeromatic)
    set_target_properties(aeromatic PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(flightsim PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
endif()
