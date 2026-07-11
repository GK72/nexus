#!/bin/bash
export SPDLOG_LEVEL=info

# Baldr Logging Mode E2E Test Suite
#
# Demonstrates that `baldr::log` selects between the plain `nova::log` sink
# ("standard" mode) and the in-place `progress`-rendering sink ("progress"
# mode) at runtime, and that failure (not success) is the special case that
# dumps the buffered log history.

# Configuration
PROJECT_ROOT=$(pwd)
BALDR_PROJECT_DIR="${PROJECT_ROOT}/baldr-cpp"
BALDR_BIN="${BALDR_PROJECT_DIR}/build/debug/baldr/baldr"
RUN_ID=$(date +%Y%m%d_%H%M%S)
OUTPUT_DIR="${PROJECT_ROOT}/.tmp/test-outputs/e2e-log-mode/${RUN_ID}"
mkdir -p "${OUTPUT_DIR}"

# Build the project
echo "Building project..."
if ! baldr -p "${BALDR_PROJECT_DIR}" -b Debug -t baldr --no-confirm > "${OUTPUT_DIR}/build.log" 2>&1; then
    echo "Build failed! Check ${OUTPUT_DIR}/build.log"
    exit 1
fi

# An empty "project" with no build directory, used to trigger Builder::debug's
# "No executables found" failure path without needing a real build.
EMPTY_PROJECT_DIR="${OUTPUT_DIR}/empty-project"
mkdir -p "${EMPTY_PROJECT_DIR}"

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

# Detects the raw ANSI cursor-control bytes (clear-line / cursor-up|down) that
# `baldr::progress` emits for its in-place rendering.
has_progress_escapes() {
    grep -Pq '\x1b\[2K|\x1b\[[0-9]+[AB]' "$1"
}

# Detects the red buffer-dump escape codes that `progress::failure` uses.
has_failure_dump_escapes() {
    grep -Pq '\x1b\[0;31m|\x1b\[1;31m' "$1"
}

# run_happy_path_test <name> <expect_progress: yes|no> [env assignments...] -- <args...>
#
# Only tokens matching NAME=value (no leading dash) preceding "--" are treated
# as environment assignments; everything else is forwarded verbatim as an
# argument to `baldr`.
run_happy_path_test() {
    local name=$1
    local expect_progress=$2
    shift 2

    local env_args=()
    local pre_args=()
    while [ "$1" != "--" ]; do
        if [[ "$1" =~ ^[A-Za-z_][A-Za-z0-9_]*=.*$ ]]; then
            env_args+=("$1")
        else
            pre_args+=("$1")
        fi
        shift
    done
    shift # drop the "--"

    local out_file="${OUTPUT_DIR}/${name}.out"
    echo -n "Running test: ${name}... "
    env "${env_args[@]}" "${BALDR_BIN}" "${pre_args[@]}" "$@" > "${out_file}" 2>&1

    if ! grep -q "Testing logging" "${out_file}" || ! grep -q "Test finished" "${out_file}"; then
        record_fail "${name}" "expected 'Testing logging...'/'Test finished' lines not found"
        return
    fi

    if [ "${expect_progress}" == "yes" ]; then
        if has_progress_escapes "${out_file}"; then
            record_pass
        else
            record_fail "${name}" "expected progress-mode cursor-control escapes, none found"
        fi
    else
        if has_progress_escapes "${out_file}"; then
            record_fail "${name}" "unexpected progress-mode cursor-control escapes found"
        else
            record_pass
        fi
    fi
}

# run_failure_path_test <name> <expect_dump: yes|no> [env assignments...] -- <args...>
run_failure_path_test() {
    local name=$1
    local expect_dump=$2
    shift 2

    local env_args=()
    local pre_args=()
    while [ "$1" != "--" ]; do
        if [[ "$1" =~ ^[A-Za-z_][A-Za-z0-9_]*=.*$ ]]; then
            env_args+=("$1")
        else
            pre_args+=("$1")
        fi
        shift
    done
    shift # drop the "--"

    local out_file="${OUTPUT_DIR}/${name}.out"
    echo -n "Running test: ${name}... "
    env "${env_args[@]}" "${BALDR_BIN}" "${pre_args[@]}" "$@" -p "${EMPTY_PROJECT_DIR}" debug > "${out_file}" 2>&1

    if ! grep -q "No executables found" "${out_file}"; then
        record_fail "${name}" "expected 'No executables found' error line not found"
        return
    fi

    if [ "${expect_dump}" == "yes" ]; then
        if has_failure_dump_escapes "${out_file}"; then
            record_pass
        else
            record_fail "${name}" "expected red buffer-dump escape codes, none found"
        fi
    else
        if has_failure_dump_escapes "${out_file}"; then
            record_fail "${name}" "unexpected red buffer-dump escape codes found"
        else
            record_pass
        fi
    fi
}

# run_negative_test <name> [env assignments...] -- <args...> <error_regex>
run_negative_test() {
    local name=$1
    shift

    local env_args=()
    while [ "$1" != "--" ]; do
        env_args+=("$1")
        shift
    done
    shift # drop the "--"

    local error_regex="${@: -1}"
    local args=("${@:1:$#-1}")

    local out_file="${OUTPUT_DIR}/${name}.out"
    echo -n "Running negative test: ${name}... "
    if env "${env_args[@]}" "${BALDR_BIN}" "${args[@]}" > "${out_file}" 2>&1; then
        record_fail "${name}" "expected failure, but it succeeded"
        return
    fi

    if grep -qP "${error_regex}" "${out_file}"; then
        record_pass
    else
        record_fail "${name}" "error message did not match '${error_regex}'"
        echo "  Actual output: $(cat "${out_file}")"
    fi
}

test_cases() {
    # --- Happy path: standard vs. progress, both explicit and via env/baseline ---
    run_happy_path_test "standard_mode_happy"   "no"  --log-mode=standard -- test
    run_happy_path_test "progress_mode_happy"   "yes" --log-mode=progress -- test
    run_happy_path_test "baseline_no_override"  "no"  -- test

    # --- Failure path: only progress mode dumps the buffered history ---
    run_failure_path_test "standard_mode_failure" "no"  --log-mode=standard --
    run_failure_path_test "progress_mode_failure" "yes" --log-mode=progress --

    # --- Mode resolution precedence ---
    run_happy_path_test "env_alone_selects_progress"     "yes" BALDR_LOG_MODE=progress -- test
    run_happy_path_test "cli_overrides_env_to_standard"  "no"  BALDR_LOG_MODE=progress -- --log-mode=standard test

    # --- Invalid CLI value ---
    run_negative_test "invalid_log_mode" -- --log-mode=bogus test "Invalid value for --log-mode: 'bogus'"
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
        echo "Log mode E2E tests failed!"
        echo "Test artifacts are saved in: ${OUTPUT_DIR}"
        exit 1
    fi

    echo "----------------------------------------"
    echo "All log mode E2E tests passed successfully!"
    echo "Test artifacts are saved in: ${OUTPUT_DIR}"
    exit 0
}

test_cases
summary
