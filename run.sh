#!/bin/bash

# Delete build directory if it exists
rm -rf build

# Create build directory
mkdir -p build

# Go into build directory
cd build

# Configure with CMake
echo "Configuring..."
cmake ..

# Build
echo "Building..."
if cmake --build .; then
    echo "Build successful! Starting simulator..."
    echo "----------------------------------------"
    # Run the executable
    ./nuage
else
    echo "Build failed."
    exit 1
fi
