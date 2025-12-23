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
    src/core/simulator.cpp
    src/modules/input_module.cpp
    src/modules/render_module.cpp
    src/modules/aircraft_module.cpp
    src/modules/terrain_module.cpp
    src/modules/camera_module.cpp
    src/simulation/flight_model.cpp
    src/simulation/flight_params.cpp
    src/graphics/mesh_builder.cpp
    src/assets/asset_store.cpp
)

add_executable(flightsim ${SOURCES})

# Copy assets to the build directory
add_custom_command(TARGET flightsim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:flightsim>/assets
)

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