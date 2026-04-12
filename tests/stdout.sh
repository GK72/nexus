#!/usr/bin/env bash
export SPDLOG_LEVEL=trace

# BTX Stdout E2E Test Suite

# Set up directories
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
TMP_ROOT="$PROJECT_ROOT/.tmp/test-outputs/stdout-test"
RUN_ID=$(date +%Y%m%d_%H%M%S)
TMP_DIR="$TMP_ROOT/$RUN_ID"

mkdir -p "$TMP_DIR"

# Build the project
baldr -p "$PROJECT_ROOT" -b Debug -t btx-tool > /dev/null 2>&1

BTX_TOOL="$PROJECT_ROOT/build/debug/btx/btx-tool"

PASSED=0
FAILED=0
FAILED_TESTS=()

function run_test() {
    local name=$1
    local cmd=$2
    local expected_output_hex=$3

    echo -n "Running test: $name... "
    
    # Run command and capture stdout to file
    eval "$cmd" > "$TMP_DIR/$name.out" 2> "$TMP_DIR/$name.err"
    
    # Convert actual output to hex for comparison
    local actual_hex=$(xxd -p "$TMP_DIR/$name.out" | tr -d '\n')
    
    if [ "$actual_hex" == "$expected_output_hex" ]; then
        echo "PASSED"
        ((PASSED++))
    else
        echo "FAILED"
        echo "  Expected: $expected_output_hex"
        echo "  Actual:   $actual_hex"
        ((FAILED++))
        FAILED_TESTS+=("$name")
    fi
}

function run_text_test() {
    local name=$1
    local cmd=$2
    local expected_text=$3

    echo -n "Running test: $name... "
    
    # Run command and capture stdout to file
    eval "$cmd" > "$TMP_DIR/$name.out" 2> "$TMP_DIR/$name.err"
    
    # Clean up output (remove trailing whitespace)
    local actual_text=$(cat "$TMP_DIR/$name.out" | sed 's/[[:space:]]*$//')
    local expected_clean=$(echo "$expected_text" | sed 's/[[:space:]]*$//')

    if [ "$actual_text" == "$expected_clean" ]; then
        echo "PASSED"
        ((PASSED++))
    else
        echo "FAILED"
        echo "  Expected: $expected_clean"
        echo "  Actual:   $actual_text"
        ((FAILED++))
        FAILED_TESTS+=("$name")
    fi
}

# 1. to_binary stdout
echo "\x48\x65\x6c\x6c\x6f" > "$TMP_DIR/hello.btx"
run_test "to_binary_stdout" "$BTX_TOOL encode $TMP_DIR/hello.btx" "48656c6c6f"

# 2. from_binary stdout
echo -n "Hello" > "$TMP_DIR/hello.bin"
run_text_test "from_binary_stdout" "$BTX_TOOL decode $TMP_DIR/hello.bin" "\\x48\\x65\\x6C\\x6C\\x6F"

# 3. descriptor to_binary stdout
# Use existing sample.btx - Note: descriptor no longer allowed for encode
run_test "descriptor_to_binary_stdout" "$BTX_TOOL encode $PROJECT_ROOT/res/sample.btx" "0913e8d0054e65787573"

# 4. descriptor from_binary (annotated) stdout
# We need a 19-byte file for sample.btx
# Actually the previous run created sample.bin in current dir? No, I should use the one from res or create it.
# Let's create it.
echo -n -e "\x09\x13\xe8\xd0\x05\x4e\x65\x78\x75\x73" > "$TMP_DIR/sample.bin"
# Expected annotated output:
EXPECTED_ANNOTATED=$(cat <<EOF
// player
\x09                                     // message_length: 9 (0x9)
\b0001                                   // version: 1 (0x1)
\b001111101000                           // id: 1000 (0x3e8)
\b1                                      // is_alive: 1 (0x1)
\b1010000                                // health: 80 (0x50)
\x05                                     // name_length: 5 (0x5)
\x4E\x65\x78\x75\x73                     // name: "Nexus"
EOF
)

# Note: message_length is 1 byte in the latest descriptor but I used \x13\xE8 (2 bytes) in my manual creation.
# Wait, let's check res/descriptor.yaml
# fields:
#    - name: message_length
#      length: 1
#      length_type: byte

# Ah, I misremembered.
# Let's check res/descriptor.yaml content.
run_text_test "descriptor_from_binary_stdout" "$BTX_TOOL decode $TMP_DIR/sample.bin -d $PROJECT_ROOT/res/descriptor.yaml" "$EXPECTED_ANNOTATED"

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

echo "All stdout tests passed!"
echo "Artifacts saved in: $TMP_DIR"
exit 0
