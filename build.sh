#!/bin/bash

# Build script for Enatrio project
# Usage: ./build.sh [debug|release]

# Set temp directory to avoid Windows permission issues
export TMP=/tmp
export TEMP=/tmp
export TMPDIR=/tmp

# Detect platform
OS_TYPE="$(uname -s)"

if [ "$OS_TYPE" = "Linux" ]; then
    GCC="gcc"
else
    # Windows / MSYS2
    export PATH="/ucrt64/bin:$PATH"
    GCC="gcc"
fi

BUILD_TYPE="${1:-debug}"
PROJECT_NAME="Enatrio"
SRC_DIR="src"
ASSETS_DIR="assets"

# Set build configuration
if [ "$BUILD_TYPE" = "release" ]; then
    BUILD_DIR="build/release"
    CFLAGS="-std=c23 -O2 -DNDEBUG -DPROJECT_NAME=$PROJECT_NAME"
    echo "Building RELEASE configuration..."
elif [ "$BUILD_TYPE" = "tests" ]; then
    BUILD_DIR="build/tests"
    CFLAGS="-std=c23 -g3 -O0 -DDEBUG -DINTESTING -DPROJECT_NAME=$PROJECT_NAME"
    echo "Building TESTS configuration..."
else
    BUILD_DIR="build/debug"
    CFLAGS="-std=c23 -g3 -O0 -DDEBUG -DPROJECT_NAME=$PROJECT_NAME"
    echo "Building DEBUG configuration..."
fi

if [ "$OS_TYPE" = "Linux" ]; then
    OUTPUT_FILE="$BUILD_DIR/$PROJECT_NAME"
else
    OUTPUT_FILE="$BUILD_DIR/$PROJECT_NAME.exe"
fi

# Create build directory
mkdir -p "$BUILD_DIR"

# Copy assets directory to build directory
if [ -d "$ASSETS_DIR" ]; then
    echo "Copying assets..."
    cp -r "$ASSETS_DIR" "$BUILD_DIR/"
fi

# Calculate source checksum
SRC_HASH=$(find "$SRC_DIR" -type f \( -name "*.c" -o -name "*.h" \) -print0 | sort -z | xargs -0 md5sum | md5sum | cut -d' ' -f1)
SRC_HASH_INT=$(( 16#${SRC_HASH:0:8} ))
echo "Source checksum: $SRC_HASH_INT"
CFLAGS="$CFLAGS -DHASH=${SRC_HASH_INT}UL"

# Record build timestamp
CFLAGS="$CFLAGS -DBUILD_DAY=$(date +%-d) -DBUILD_MONTH=$(date +%-m) -DBUILD_YEAR=$(date +%Y)"
CFLAGS="$CFLAGS -DBUILD_HOUR=$(date +%-H) -DBUILD_MINUTE=$(date +%-M) -DBUILD_SECOND=$(date +%-S)"

# Find all .c files in src directory
echo "Scanning for source files..."
C_FILES=$(find "$SRC_DIR" -name "*.c")

if [ -z "$C_FILES" ]; then
    echo "Error: No .c files found in $SRC_DIR directory"
    exit 1
fi

echo "Found source files:"
echo "$C_FILES"

# Compile all C files
echo "Compiling..."

# Raylib flags (platform-specific linking)
RAYLIB_CFLAGS="-Ilib/raylib/include"
if [ "$OS_TYPE" = "Linux" ]; then
    RAYLIB_LIBS="-Llib/raylib/lib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11"
else
    RAYLIB_LIBS="-Llib/raylib/lib -lraylib -lopengl32 -lgdi32 -lwinmm"
fi

# Run GCC and capture output (both stdout and stderr)
COMPILE_OUTPUT=$($GCC $CFLAGS -Wall -Wextra $RAYLIB_CFLAGS $C_FILES -o "$OUTPUT_FILE" $RAYLIB_LIBS 2>&1)
COMPILE_STATUS=$?

# Display all compiler output (warnings and errors)
if [ -n "$COMPILE_OUTPUT" ]; then
    echo "$COMPILE_OUTPUT"
fi

if [ $COMPILE_STATUS -eq 0 ]; then
    echo "Build successful: $OUTPUT_FILE"
    exit 0
else
    echo "Build failed with exit code $COMPILE_STATUS"
    exit 1
fi
