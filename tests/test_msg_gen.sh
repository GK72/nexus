#!/bin/bash
export SPDLOG_LEVEL=trace
set -e

# Setup temporary directory for test outputs
mkdir -p ./.tmp/test-outputs/msg-gen-test
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
TEST_DIR="./.tmp/test-outputs/msg-gen-test/$TIMESTAMP"
mkdir -p "$TEST_DIR"

DESC_FILE="./res/descriptor.yaml"
OUTPUT_FILE="$TEST_DIR/random.bin"
PARSED_FILE="$TEST_DIR/parsed.txt"

# Run msg-gen
echo "Running msg-gen..."
./build/debug/msg-gen/msg-gen --descriptor "$DESC_FILE" --output "$OUTPUT_FILE"

if [ ! -f "$OUTPUT_FILE" ]; then
    echo "FAILURE: Output file not created."
    exit 1
fi

echo "Generated file size: $(stat -c%s "$OUTPUT_FILE") bytes"

# Run btx-tool to parse it
echo "Parsing with btx-tool..."
./build/debug/btx/btx-tool --descriptor "$DESC_FILE" --from-binary --input "$OUTPUT_FILE" > "$PARSED_FILE"

echo "Parsed output:"
cat "$PARSED_FILE"

# Check if it contains some expected fields
# Based on res/descriptor.yaml
echo "Verifying parsed output..."
grep -q "message_length:" "$PARSED_FILE"
grep -q "version:" "$PARSED_FILE"
grep -q "id:" "$PARSED_FILE"
grep -q "is_alive:" "$PARSED_FILE"
grep -q "health:" "$PARSED_FILE"
grep -q "name_length:" "$PARSED_FILE"
grep -q "name:" "$PARSED_FILE"

echo "SUCCESS: Randomly generated message was successfully parsed."
rm -rf "$TEST_DIR"
rmdir ./.tmp/test-outputs/msg-gen-test --ignore-fail-on-non-empty
