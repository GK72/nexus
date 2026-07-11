#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(dirname "$(realpath "$0")")"
PROJECT_DIR=$(git -C "${SCRIPT_DIR}" rev-parse --show-toplevel)

"${PROJECT_DIR}/install.sh"
LD_LIBRARY_PATH="${PROJECT_DIR}/.tmp/install/lib" "${PROJECT_DIR}/pylib/btx-encode.py"
