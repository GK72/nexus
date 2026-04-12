#!/bin/bash
export SPDLOG_LEVEL=trace

# Part of Nexus project.
# E2E tests for BTX Descriptor Parser.

set -u # Exit on undefined variables

# --- Configuration ---
PROJECT_ROOT="$(pwd)"
BUILD_DIR="${PROJECT_ROOT}/build/debug"
BTX_TOOL="${BUILD_DIR}/btx/btx-tool"
TEST_OUTPUT_BASE="${PROJECT_ROOT}/.tmp/test-outputs/e2e-descriptor"
RUN_ID=$(date +%Y%m%d_%H%M%S)
TEST_DIR="${TEST_OUTPUT_BASE}/${RUN_ID}"

# --- Setup ---
mkdir -p "${TEST_DIR}"

PASSED=0
FAILED=0
FAILED_TESTS=""

echo "Starting E2E Descriptor Tests (Run: ${RUN_ID})"
echo "------------------------------------------------"

# --- Helper Functions ---

run_test_parse() {
    local name=$1
    local btx_file=$2
    local desc_file=$3
    local expected_log=$4

    echo -n "Test [${name}]: "
    
    local output_log="${TEST_DIR}/${name}.log"
    local bin_file="${TEST_DIR}/${name}.bin"
    
    # First encode to binary
    "${BTX_TOOL}" encode "${btx_file}" "${bin_file}" > /dev/null 2>&1
    
    # Then decode with descriptor
    "${BTX_TOOL}" decode "${bin_file}" -d "${desc_file}" > "${output_log}" 2>&1
    local exit_code=$?

    if [ ${exit_code} -ne 0 ]; then
        echo "FAILED (exit code ${exit_code})"
        FAILED=$((FAILED + 1))
        FAILED_TESTS="${FAILED_TESTS}\n - ${name} (exit code ${exit_code})"
        return
    fi

    # Check if expected strings are in the log
    local missing=0
    while IFS= read -r line; do
        if [[ -z "${line}" ]]; then continue; fi
        if ! grep -q "${line}" "${output_log}"; then
            missing=$((missing + 1))
            echo -e "\n   Missing expected line: ${line}"
        fi
    done <<< "${expected_log}"

    if [ ${missing} -eq 0 ]; then
        echo "PASSED"
        PASSED=$((PASSED + 1))
    else
        echo "FAILED (${missing} lines missing)"
        FAILED=$((FAILED + 1))
        FAILED_TESTS="${FAILED_TESTS}\n - ${name} (output mismatch)"
    fi
}

# --- Test Cases ---

# 1. Basic Sample Parse
EXPECTED_SAMPLE="Parsed message: player
message_length: 9 (0x9)
version: 1 (0x1)
id: 1000 (0x3e8)
is_alive: true
health: 80 (0x50)
name_length: 5 (0x5)
name: Nexus"

run_test_parse "sample_parse" "res/sample.btx" "res/descriptor.yaml" "${EXPECTED_SAMPLE}"

# --- Summary ---
echo "------------------------------------------------"
echo "Tests finished."
echo "PASSED: ${PASSED}"
echo "FAILED: ${FAILED}"

if [ ${FAILED} -ne 0 ]; then
    echo -e "Failed tests:${FAILED_TESTS}"
    echo "Artifacts saved in: ${TEST_DIR}"
    exit 1
fi

echo "Artifacts saved in: ${TEST_DIR}"
exit 0
