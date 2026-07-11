#!/usr/bin/env bash

set -e

SCRIPT_DIR="$(dirname "$(realpath "$0")")"
BUILD_DIR="${SCRIPT_DIR}/build/release"
INSTALL_PREFIX="${1:-${SCRIPT_DIR}/.tmp/install}"
JOBS=$(($(nproc) / 2))

baldr -p "${SCRIPT_DIR}" -b Release -j $JOBS

echo "Installing Nexus to ${INSTALL_PREFIX}..."
cmake --install "${BUILD_DIR}" --prefix "${INSTALL_PREFIX}"

echo "Installation complete!"
