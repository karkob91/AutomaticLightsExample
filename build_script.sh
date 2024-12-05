#!/bin/bash

set -e

# Detect the environment
echo "=== Detecting Compiler Environment ==="
if command -v gcc >/dev/null 2>&1; then
    GENERATOR="Unix Makefiles"
    COMPILER="GCC"
    echo "Detected GCC environment."
else
    echo "Error: GCC not found! Please install GCC."
    exit 1
fi

# Get the number of processors for parallel builds
NUM_PROCESSORS=$(nproc)
echo "WARNING: This script will use ${NUM_PROCESSORS} processors for the build."
echo "If this is not desired, modify the script to adjust the processor count."

# Step 1: Run CMake to generate build system files
echo "=== Running CMake configuration ==="
cmake -G "${GENERATOR}" -B build -S .
if [ $? -ne 0 ]; then
    echo "Error: CMake configuration failed."
    exit 1
fi

# Step 2: Build the code generation target
echo "=== Building Dezyne code generation target ==="
cmake --build build --target generate_dezyne_code -- -j "${NUM_PROCESSORS}"
if [ $? -ne 0 ]; then
    echo "Error: Dezyne code generation failed."
    exit 1
fi

# Step 3: Run CMake again to configure with the generated files included
echo "=== Reconfiguring CMake with generated files ==="
cmake -B build -S .
if [ $? -ne 0 ]; then
    echo "Error: Reconfiguration failed."
    exit 1
fi

# Step 4: Build the final project
echo "=== Building the final project ==="
cmake --build build -- -j "${NUM_PROCESSORS}"
if [ $? -ne 0 ]; then
    echo "Error: Final build failed."
    exit 1
fi

echo "=== Build completed successfully ==="
exit 0
