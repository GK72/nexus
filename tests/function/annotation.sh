#!/usr/bin/env bash
#
# Part of BTX Toolset
#
# Function tests of btx-tool's annotation: decoding with a descriptor writes the field names and
# values as comments next to the tokens.

set -euo pipefail
# shellcheck source=../common.sh
source "$(dirname "$(realpath "$0")")/../common.sh"

encode_sample() {
    tap::run "${BTX_TOOL}" encode "${DATA}/sample.btx" -o "${TAP_CASE_DIR}/sample.bin"
    tap::expect_status 0
    tap::expect_hex "${TAP_CASE_DIR}/sample.bin" 0913e8d0054e65787573
}

decode_annotated() {
    tap::write_hex "${TAP_CASE_DIR}/sample.bin" 0913e8d0054e65787573
    tap::run "${BTX_TOOL}" decode "${TAP_CASE_DIR}/sample.bin" -d "${DATA}/descriptor.yaml" \
        -o "${TAP_CASE_DIR}/annotated.btx"
    tap::expect_status 0
    tap::expect_contains "${TAP_CASE_DIR}/annotated.btx" \
        '// player' '// message_length: 9' '\x09' '// version: 1' '\b0001' '// name: "Nexus"'
}

tap::case "encode the sample (-o)"              encode_sample
tap::case "decode annotated (-d, -o)"           decode_annotated

tap::done
