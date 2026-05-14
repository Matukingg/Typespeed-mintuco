#!/usr/bin/env bash
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
echo "=== 1. Generating Makefiles ==="
premake4 --file="$PROJECT_ROOT/premake4.lua" gmake
echo "=== 2. Compiling (Debug) ==="
make -k -C "$BUILD_DIR" config=debug
