#!/usr/bin/env bash

# Rolling Window Regression + Demo Suite
#
# Drives the real, compiled baldr-cpp `baldr` binary end-to-end through its
# `run` subcommand, reproducing two previously fixed bugs in the rolling
# progress window:
#   (a)/(b) command::poll() chunk boundaries not aligning with terminal lines
#           (multiple lines in one write, one line split across two writes)
#   (c)     progress::msg() letting an oversized line wrap onto multiple rows
#           instead of being capped to a single row with a trailing "..."
#
# Each scenario is run under a pseudo-TTY (script -qc) so baldr::progress
# takes its interactive/rolling-window code path, matching how a human
# actually runs `baldr build`/`baldr run` in a real terminal. Besides acting
# as an automated regression check, this script doubles as a narrated,
# human-runnable demo of the fix.

# Set up directories
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BALDR_CPP_ROOT="$PROJECT_ROOT/baldr-cpp"
TMP_ROOT="$PROJECT_ROOT/.tmp/test-outputs/rolling-window-test"
RUN_ID=$(date +%Y%m%d_%H%M%S)
TMP_DIR="$TMP_ROOT/$RUN_ID"

mkdir -p "$TMP_DIR"

# Build the baldr-cpp `baldr` binary itself
baldr -p "$BALDR_CPP_ROOT" -b Debug -t baldr > /dev/null 2>&1

BALDR_BIN="$BALDR_CPP_ROOT/build/debug/baldr/baldr"

if [ ! -x "$BALDR_BIN" ]; then
    echo "FAILED to build baldr binary at $BALDR_BIN"
    exit 1
fi

PASSED=0
FAILED=0
FAILED_TESTS=()

function strip_ansi() {
    sed -E 's/\x1b\[[0-9;]*[A-Za-z]//g'
}

function run_scenario() {
    local name=$1
    local description=$2
    local shell_cmd=$3

    echo
    echo "----------------------------------------"
    echo "Scenario: $description"
    echo -n "Running test: $name... "

    local raw_log="$TMP_DIR/$name.raw.log"
    local plain_log="$TMP_DIR/$name.plain.log"

    script -qc "\"$BALDR_BIN\" run bash -c '$shell_cmd'" "$raw_log" > /dev/null 2>&1
    strip_ansi < "$raw_log" > "$plain_log"

    echo
    echo "Captured raw terminal bytes:"
    cat -v "$raw_log"
    echo
}

function assert_contains() {
    local name=$1
    local file=$2
    local needle=$3

    if grep -qF -- "$needle" "$file"; then
        echo "PASSED: $name"
        ((PASSED++))
    else
        echo "FAILED: $name"
        echo "  Expected to find: $needle"
        echo "  In file:          $file"
        ((FAILED++))
        FAILED_TESTS+=("$name")
    fi
}

function assert_not_contains() {
    local name=$1
    local file=$2
    local needle=$3

    if grep -qF -- "$needle" "$file"; then
        echo "FAILED: $name"
        echo "  Did not expect to find: $needle"
        echo "  In file:                $file"
        ((FAILED++))
        FAILED_TESTS+=("$name")
    else
        echo "PASSED: $name"
        ((PASSED++))
    fi
}

# (a) Two log lines emitted in a single subprocess write
# Marker text is kept short so it survives the rolling window's per-line
# terminal-width cap (the timestamp/logger prefix already consumes most of
# an 80-column row).
run_scenario "two_lines_one_write" \
    "two lines are written by a single subprocess printf call, previously mis-split by command::poll()" \
    'printf "RW-A1\nRW-A2\n"'

assert_contains "two_lines_one_write_first_line" "$TMP_DIR/two_lines_one_write.plain.log" "RW-A1"
assert_contains "two_lines_one_write_second_line" "$TMP_DIR/two_lines_one_write.plain.log" "RW-A2"

# (b) One line split across two subprocess writes
run_scenario "line_split_across_writes" \
    "one line is emitted across two separate subprocess writes with a short sleep in between, previously reassembled incorrectly" \
    'printf "RW-SPL"; sleep 0.2; printf "IT-OK\n"'

assert_contains "line_split_across_writes_reassembled" "$TMP_DIR/line_split_across_writes.plain.log" "RW-SPLIT-OK"

# (c) One line far longer than a typical terminal width
LONG_LINE="rolling-window-C-$(printf 'x%.0s' $(seq 1 200))"
run_scenario "oversized_line_truncated" \
    "one line is deliberately ~200 characters long, previously causing the rolling window's cursor math to desync via multi-row wrapping" \
    "printf '${LONG_LINE}\n'"

assert_contains "oversized_line_truncated_with_ellipsis" "$TMP_DIR/oversized_line_truncated.plain.log" "..."
assert_not_contains "oversized_line_never_appears_in_full" "$TMP_DIR/oversized_line_truncated.plain.log" "[info] $LONG_LINE"

# Summary
echo
echo "----------------------------------------"
echo "Test Summary:"
echo "  Passed: $PASSED"
echo "  Failed: $FAILED"
echo "----------------------------------------"

if [ $FAILED -ne 0 ]; then
    echo "Failed tests: ${FAILED_TESTS[*]}"
    echo "Artifacts saved in: $TMP_DIR"
    exit 1
fi

echo "All rolling-window tests passed!"
echo "Artifacts saved in: $TMP_DIR"
exit 0
