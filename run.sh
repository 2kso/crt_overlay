#!/bin/bash

# Break on any error
set -e

echo "Compiling the CRT overlay project..."

# Create the build directory if it doesn't already exist
mkdir -p build

# Generate CMake build files in the build directory
cmake -S . -B build

# Compile the project using the generated build files
cmake --build build

echo "Compilation successful! Executable is located at: build/crt_overlay"
echo "Starting the CRT overlay..."

# Execute the newly built binary
./build/crt_overlay
