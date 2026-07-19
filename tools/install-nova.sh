#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
PROJECT_DIR=$(git -C "${SCRIPT_DIR}" rev-parse --show-toplevel)

BUILD_DIR="${PROJECT_DIR}/.tmp/build/libnova"
INSTALL_DIR="${PROJECT_DIR}/.tmp/install"

if [[ "${1:-}" == "--prefix" ]]; then
    INSTALL_DIR="${2}"
fi

mkdir --parent "${BUILD_DIR}"

CMAKE_BUILD_PARALLEL_LEVEL=$(($(nproc)/2))

PROJECT_NOVA_DIR="${PROJECT_DIR}/deps/nova-cpp"

cmake -S "${PROJECT_NOVA_DIR}/libnova" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES="${PROJECT_NOVA_DIR}/conan_provider.cmake" \
    -DBUILD_TESTING=OFF

cmake --build "${BUILD_DIR}"
cmake --install "${BUILD_DIR}" --prefix "${INSTALL_DIR}"
