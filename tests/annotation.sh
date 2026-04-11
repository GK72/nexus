#!/bin/bash
export SPDLOG_LEVEL=trace

# End-to-end test for BTX annotation feature.

set -u

# Configuration
BTX_TOOL="./build/debug/btx/btx-tool"
RES_DIR="./res"
TMP_DIR="./.tmp/test-outputs/annotation-test/$(date +%Y%m%d_%H%M%S)"

mkdir -p "$TMP_DIR"

echo "Using temporary directory: $TMP_DIR"

# 1. BTX -> Binary with Descriptor
echo "Testing: BTX -> Binary with Descriptor"
$BTX_TOOL "$RES_DIR/sample.btx" -d "$RES_DIR/descriptor.yaml" -o "$TMP_DIR/sample.bin"
if [ $? -ne 0 ]; then
    echo "FAILED: BTX -> Binary conversion failed"
    exit 1
fi

# Verify binary output
EXPECTED_HEX="0913e8d0054e65787573"
ACTUAL_HEX=$(xxd -p "$TMP_DIR/sample.bin" | tr -d '\n')
if [ "$ACTUAL_HEX" != "$EXPECTED_HEX" ]; then
    echo "FAILED: Binary output mismatch"
    echo "  Expected: $EXPECTED_HEX"
    echo "  Actual:   $ACTUAL_HEX"
    exit 1
fi
echo "PASSED: BTX -> Binary conversion"

# 2. Binary -> Annotated BTX with Descriptor
echo "Testing: Binary -> Annotated BTX with Descriptor"
$BTX_TOOL "$TMP_DIR/sample.bin" -f -d "$RES_DIR/descriptor.yaml" -o "$TMP_DIR/sample_annotated.btx"
if [ $? -ne 0 ]; then
    echo "FAILED: Binary -> Annotated BTX conversion failed"
    exit 1
fi

# Verify annotated BTX output
# We check if some expected comments and tokens are present
grep -F -q "// player" "$TMP_DIR/sample_annotated.btx" && \
grep -F -q "// message_length: 9" "$TMP_DIR/sample_annotated.btx" && \
grep -F -q "\\x09" "$TMP_DIR/sample_annotated.btx" && \
grep -F -q "// version: 1" "$TMP_DIR/sample_annotated.btx" && \
grep -F -q "\\b0001" "$TMP_DIR/sample_annotated.btx" && \
grep -F -q "// name: \"Nexus\"" "$TMP_DIR/sample_annotated.btx"

if [ $? -ne 0 ]; then
    echo "FAILED: Annotated BTX does not contain expected metadata"
    cat "$TMP_DIR/sample_annotated.btx"
    exit 1
fi
echo "PASSED: Binary -> Annotated BTX conversion"

echo "All annotation tests PASSED!"
echo "Artifacts saved in: $TMP_DIR"
