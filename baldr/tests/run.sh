#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(dirname "$(realpath "$0")")"
PROJECT_DIR=$(git -C "${SCRIPT_DIR}" rev-parse --show-toplevel)
BALDR="${PROJECT_DIR}/build/debug/baldr/baldr"

SPDLOG_MODE=standard $BALDR -p "${PROJECT_DIR}/baldr/tests/make-project" run -t hello --build
SPDLOG_MODE=standard $BALDR -p "${PROJECT_DIR}/baldr/tests/cmake-project" run -t hello --build
SPDLOG_MODE=standard $BALDR -p "${PROJECT_DIR}/baldr/tests/cmake-project" run -t hello --build -b Release
