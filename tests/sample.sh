#!/bin/bash
export SPDLOG_LEVEL=trace
# tests/sample.sh

set -u

PROJECT_ROOT=$(pwd)
BTX_TOOL="${PROJECT_ROOT}/build/debug/btx/btx-tool"
INPUT_FILE="${PROJECT_ROOT}/res/sample.btx"
RUN_ID=$(date +%Y%m%d_%H%M%S)
OUTPUT_DIR="${PROJECT_ROOT}/.tmp/test-outputs/sample-test/${RUN_ID}"
OUTPUT_FILE="${OUTPUT_DIR}/sample.bin"

mkdir -p "${OUTPUT_DIR}"

echo "Testing sample.btx..."

if [ ! -f "${BTX_TOOL}" ]; then
    echo "btx-tool not found at ${BTX_TOOL}. Building..."
    baldr -p "${PROJECT_ROOT}" -b Debug -t btx-tool --no-configure
fi

if ! "${BTX_TOOL}" encode "${INPUT_FILE}" "${OUTPUT_FILE}"; then
    echo "btx-tool failed!"
    exit 1
fi

ACTUAL=$(xxd -p "${OUTPUT_FILE}" | tr -d '\n')
EXPECTED="0913e8d0054e65787573"

echo "Actual:   ${ACTUAL}"
echo "Expected: ${EXPECTED}"

if [ "${ACTUAL}" == "${EXPECTED}" ]; then
    echo "SUCCESS: sample.btx converted correctly."
    echo "Artifacts saved in ${OUTPUT_DIR}"
    exit 0
else
    echo "FAILURE: Output mismatch."
    echo "Artifacts saved in ${OUTPUT_DIR}"
    exit 1
fi
