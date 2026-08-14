#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(dirname "$(realpath "$0")")"
PROJECT_DIR=$(git -C "${SCRIPT_DIR}" rev-parse --show-toplevel)
BALDR="${PROJECT_DIR}/build/debug/baldr/baldr"

$BALDR -p "${PROJECT_DIR}/baldr/tests/make-project" run -t hello --build
$BALDR -p "${PROJECT_DIR}/baldr/tests/cmake-project" run -t hello --build
$BALDR -p "${PROJECT_DIR}/baldr/tests/cmake-project" run -t hello --build -b Release
$BALDR -p "${PROJECT_DIR}/baldr/tests/cmake-project" run -t hello --build -b Release --clean
$BALDR -p "${PROJECT_DIR}/baldr/tests/cmake-project" run -t hello --build -- --foo bar
$BALDR -p "${PROJECT_DIR}/baldr/tests/cmake-project" run -t hello --build -b Release -D BALDR_TEST_DEFINE=1
$BALDR -p "${PROJECT_DIR}/baldr/tests/cmake-project" run -t hello --build -b Release -D BALDR_TEST_DEFINE=2
$BALDR -p "${PROJECT_DIR}/baldr/tests/cmake-project" run -t hello --build -b Release -DBALDR_TEST_DEFINE=3
