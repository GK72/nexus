#!/bin/bash
export SPDLOG_LEVEL=trace

# Configuration
PROJECT_ROOT=$(pwd)
BTX_TOOL="${PROJECT_ROOT}/build/debug/btx/btx-tool"
RUN_ID=$(date +%Y%m%d_%H%M%S)
OUTPUT_DIR="${PROJECT_ROOT}/.tmp/test-outputs/e2e-btx/${RUN_ID}"
mkdir -p "${OUTPUT_DIR}"

# Build the project
echo "Building project..."
if ! baldr -p "${PROJECT_ROOT}" -b Debug -t btx-tool --no-configure > "${OUTPUT_DIR}/build.log" 2>&1; then
    echo "Build failed! Check ${OUTPUT_DIR}/build.log"
    exit 1
fi

PASSED=0
FAILED=0
FAILED_TESTS=()

record_pass() {
    PASSED=$((PASSED + 1))
    echo "PASS"
}

record_fail() {
    local name=$1
    local message=$2
    FAILED=$((FAILED + 1))
    FAILED_TESTS+=("${name}")
    echo "FAIL: ${message}"
}

# Helper to run tests
# run_test_to_bin <name> <input_content> <expected_hex_dump>
run_test_to_bin() {
    local name=$1
    local input=$2
    local expected=$3
    local input_file="${OUTPUT_DIR}/${name}.btx"
    local output_file="${OUTPUT_DIR}/${name}.bin"

    echo -n "Running test: ${name} (to_binary)... "
    printf "%s" "$input" > "${input_file}"
    if ! "${BTX_TOOL}" "${input_file}" "${output_file}" > "${OUTPUT_DIR}/${name}.out" 2> "${OUTPUT_DIR}/${name}.err"; then
        record_fail "${name}" "btx-tool failed with exit code $?"
        return
    fi

    local actual=$(xxd -p "${output_file}" | tr -d '\n')
    if [ "${actual}" == "${expected}" ]; then
        record_pass
    else
        record_fail "${name}" "expected ${expected}, got ${actual}"
    fi
}

# run_test_from_bin <name> <binary_hex> <expected_btx>
run_test_from_bin() {
    local name=$1
    local bin_hex=$2
    local expected=$3
    local input_file="${OUTPUT_DIR}/${name}.bin"
    local output_file="${OUTPUT_DIR}/${name}.btx"

    echo -n "Running test: ${name} (from_binary)... "
    echo "${bin_hex}" | xxd -r -p > "${input_file}"
    if ! "${BTX_TOOL}" "${input_file}" "${output_file}" -f > "${OUTPUT_DIR}/${name}.out" 2> "${OUTPUT_DIR}/${name}.err"; then
        record_fail "${name}" "btx-tool failed with exit code $?"
        return
    fi

    local actual=$(cat "${output_file}")
    if [ "${actual}" == "${expected}" ]; then
        record_pass
    else
        record_fail "${name}" "expected '${expected}', got '${actual}'"
    fi
}

# run_negative_test <name> <input_content> <expected_error_regex>
run_negative_test() {
    local name=$1
    local input=$2
    local error_regex=$3
    local input_file="${OUTPUT_DIR}/${name}.btx"
    local output_file="${OUTPUT_DIR}/${name}.bin"

    echo -n "Running negative test: ${name}... "
    printf "%s" "$input" > "${input_file}"
    if "${BTX_TOOL}" "${input_file}" "${output_file}" > "${OUTPUT_DIR}/${name}.out" 2> "${OUTPUT_DIR}/${name}.err"; then
        record_fail "${name}" "expected failure, but it succeeded"
    else
        if grep -q "${error_regex}" "${OUTPUT_DIR}/${name}.err"; then
            record_pass
        else
            record_fail "${name}" "error message did not match '${error_regex}'"
            echo "  Actual error: $(cat "${OUTPUT_DIR}/${name}.err")"
        fi
    fi
}

test_cases() {
    run_test_to_bin   "basic_hex"           '\x48\x65\x6c\x6c\x6f'          "48656c6c6f"
    run_test_to_bin   "basic_bits"          '\b10101010'                    "aa"
    run_test_to_bin   "bit_packing"         '\b1111____ \b____0000'         "f0"
    run_test_to_bin   "separators"          '\b1100'"'"'0011 \xDE \xAD'     "c3dead"
    run_test_from_bin "from_bin"            "48656c6c6f"                    '\x48\x65\x6C\x6C\x6F'
    run_negative_test "invalid_hex"         '\xGG'                          "Invalid hex character: G"
    run_negative_test "truncated_hex"       '\x4'                           "Unexpected end of input"
    run_negative_test "truncated_bits"      '\b101'                         "Incomplete .*b token"
    run_negative_test "malformed_comment"   "/ Invalid"                     "Invalid comment: expected '//'"

    run_test_from_bin "multiple_line_output" "0102030405060708090A"         '\x01\x02\x03\x04\x05\x06\x07\x08
\x09\x0A'

    run_test_to_bin "comments_whitespace" "// header
  \xAA  // comment

\xBB" "aabb"

    run_negative_test "mixed_with_b"    "\b1111 0xAA"                   "Incomplete .*b token"
    run_negative_test "no_alt_hex"      "0xAA"                          "Unexpected character: '0'"
    run_negative_test "no_alt_bits"     "0b10"                          "Unexpected character: '0'"
}

summary() {
    echo "----------------------------------------"
    echo "Test Summary:"
    echo "  Passed: ${PASSED}"
    echo "  Failed: ${FAILED}"

    if [ ${FAILED} -ne 0 ]; then
        echo "Failed tests:"
        for test_name in "${FAILED_TESTS[@]}"; do
            echo "  - ${test_name}"
        done
        echo "----------------------------------------"
        echo "E2E tests failed!"
        echo "Test artifacts are saved in: ${OUTPUT_DIR}"
        exit 1
    fi

    echo "----------------------------------------"
    echo "All E2E tests passed successfully!"
    echo "Test artifacts are saved in: ${OUTPUT_DIR}"
    exit 0
}

test_cases
summary
