#!/usr/bin/env bash
#
# Installs the Conan dependencies and configures a CMake build directory.
#
#   tools/configure.sh <build-dir> <Debug|Release|RelWithDebInfo> [cmake args...]
#   tools/configure.sh build/debug Debug -DSANITIZERS=asan
#
# Conan's output goes to <build-dir>/conan, so several build directories can coexist. The first
# one configured provides compile_commands.json at the root, for clangd.

set -euo pipefail

PROJECT_DIR="$(realpath "$(dirname "$(realpath "$0")")/..")"

BUILD_DIR="${1:?usage: configure.sh <build-dir> <build-type> [cmake args...]}"
BUILD_TYPE="${2:?usage: configure.sh <build-dir> <build-type> [cmake args...]}"
shift 2

CONAN_DIR="${BUILD_DIR}/conan"

conan install "${PROJECT_DIR}" \
    --output-folder "${CONAN_DIR}" \
    --build missing \
    -s build_type="${BUILD_TYPE}" \
    -s compiler.cppstd=26 \
    ${CONAN_PROFILE:+--profile "${CONAN_PROFILE}"}

cmake -S "${PROJECT_DIR}" -B "${BUILD_DIR}" -G Ninja \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCMAKE_TOOLCHAIN_FILE="$(realpath "${CONAN_DIR}")/conan_toolchain.cmake" \
    "$@"

if [[ ! -e "${PROJECT_DIR}/compile_commands.json" ]]; then
    ln -s "$(realpath "${BUILD_DIR}")/compile_commands.json" "${PROJECT_DIR}/compile_commands.json"
fi
