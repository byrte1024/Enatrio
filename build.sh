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
    OS_NAME="linux"
else
    # Windows / MSYS2
    export PATH="/ucrt64/bin:$PATH"
    GCC="gcc"
    OS_NAME="windows"
fi

BUILD_TYPE="${1:-debug}"
SKIP_LINT=0
if [ "$2" = "--no-lint" ] || [ "$1" = "--no-lint" ]; then
    SKIP_LINT=1
    if [ "$1" = "--no-lint" ]; then BUILD_TYPE="${2:-debug}"; fi
fi
PROJECT_NAME="Enatrio"
SRC_DIR="src"
ASSETS_DIR="assets"

# Pick C23 flag (GCC < 14 uses -std=c2x)
if $GCC -std=c23 -x c -E /dev/null > /dev/null 2>&1; then
    C_STD="-std=c23"
else
    C_STD="-std=c2x"
fi

# Set build configuration
if [ "$BUILD_TYPE" = "release" ]; then
    BUILD_DIR="build/release/$OS_NAME"
    CFLAGS="$C_STD -O2 -DNDEBUG -DPROJECT_NAME=$PROJECT_NAME"
    echo "Building RELEASE ($OS_NAME)..."
elif [ "$BUILD_TYPE" = "tests" ]; then
    BUILD_DIR="build/tests/$OS_NAME"
    CFLAGS="$C_STD -g3 -O0 -DDEBUG -DINTESTING -DPROJECT_NAME=$PROJECT_NAME"
    echo "Building TESTS ($OS_NAME)..."
else
    BUILD_DIR="build/debug/$OS_NAME"
    CFLAGS="$C_STD -g3 -O0 -DDEBUG -DPROJECT_NAME=$PROJECT_NAME"
    echo "Building DEBUG ($OS_NAME)..."
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

# Git info
GIT_HASH_FULL=$(git rev-parse HEAD 2>/dev/null || echo "unknown")
GIT_HASH_SHORT=$(git rev-parse --short HEAD 2>/dev/null || echo "unknown")
GIT_BRANCH=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unknown")
GIT_DIRTY=$(git diff --quiet 2>/dev/null && echo 0 || echo 1)

CFLAGS="$CFLAGS -DGIT_HASH_FULL=\"${GIT_HASH_FULL}\""
CFLAGS="$CFLAGS -DGIT_HASH=\"${GIT_HASH_SHORT}\""
CFLAGS="$CFLAGS -DGIT_BRANCH=\"${GIT_BRANCH}\""
CFLAGS="$CFLAGS -DGIT_DIRTY=${GIT_DIRTY}"

# Build metadata
SRC_COUNT=$(find "$SRC_DIR" -name '*.c' -o -name '*.h' | wc -l)
SRC_LINES=$(find "$SRC_DIR" -name '*.c' -o -name '*.h' -exec cat {} + | wc -l)

CFLAGS="$CFLAGS -DBUILD_TYPE_STR=\"${BUILD_TYPE}\""
CFLAGS="$CFLAGS -DBUILD_PLATFORM=\"${OS_NAME}\""
CFLAGS="$CFLAGS -DSRC_FILE_COUNT=${SRC_COUNT}"
CFLAGS="$CFLAGS -DSRC_LINE_COUNT=${SRC_LINES}"

# Run linter (requires uv + tree-sitter)
if [ "$SKIP_LINT" -eq 0 ] && command -v uv > /dev/null 2>&1 && [ -f "tools/lint.py" ]; then
    echo "Running linter..."
    if ! uv run --with tree-sitter --with tree-sitter-c python3 tools/lint.py "$SRC_DIR" 2>&1; then
        echo "Lint failed -- fix errors before building"
        exit 1
    fi
fi

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
    RAYLIB_CFLAGS="$(pkg-config --cflags raylib 2>/dev/null || echo -Ilib/raylib/include)"
    RAYLIB_LIBS="$(pkg-config --libs raylib 2>/dev/null || echo '-Llib/raylib/lib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11')"
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

if [ $COMPILE_STATUS -ne 0 ]; then
    echo "Build failed with exit code $COMPILE_STATUS"
    exit 1
fi

echo "Build successful: $OUTPUT_FILE"

# Generate info.txt
INFO_FILE="$BUILD_DIR/info.txt"

EXE_SIZE=$(stat -c%s "$OUTPUT_FILE" 2>/dev/null || stat -f%z "$OUTPUT_FILE" 2>/dev/null || echo "0")
ASSETS_SIZE=$(du -sb "$BUILD_DIR/assets" 2>/dev/null | cut -f1 || echo "0")
TOTAL_SIZE=$(du -sb "$BUILD_DIR" 2>/dev/null | cut -f1 || echo "0")
COMPILER_VERSION=$($GCC --version | head -1)
GIT_STATE=$([ "$GIT_DIRTY" -eq 0 ] && echo "clean" || echo "dirty")

cat > "$INFO_FILE" <<INFOEOF
========================================
  $PROJECT_NAME Build Info
========================================

Build Type:       $BUILD_TYPE
Platform:         $OS_NAME
Date:             $(date '+%Y-%m-%d %H:%M:%S')
C Standard:       $C_STD

Git Branch:       $GIT_BRANCH
Git Commit:       $GIT_HASH_FULL
Git Short Hash:   $GIT_HASH_SHORT
Git State:        $GIT_STATE

Source Hash:      $SRC_HASH_INT
Compiler:         $COMPILER_VERSION

Executable:       $(basename "$OUTPUT_FILE")
Executable Size:  $EXE_SIZE bytes ($(( EXE_SIZE / 1024 )) KB)
Assets Size:      $ASSETS_SIZE bytes ($(( ASSETS_SIZE / 1024 )) KB)
Total Build Size: $TOTAL_SIZE bytes ($(( TOTAL_SIZE / 1024 )) KB)

Source Files:     $SRC_COUNT
Source Lines:     $SRC_LINES
INFOEOF

echo "Build info written to $INFO_FILE"
