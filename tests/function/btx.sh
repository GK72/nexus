#!/usr/bin/env bash
#
# Part of BTX Toolset
#
# Function tests of btx-tool without a descriptor: encoding BTX to binary, decoding binary to BTX,
# and rejecting malformed BTX.

set -euo pipefail
# shellcheck source=../common.sh
source "$(dirname "$(realpath "$0")")/../common.sh"

# encode BTX HEX: BTX encodes to the bytes HEX.
encode() {
    printf '%s' "$1" > "${TAP_CASE_DIR}/input.btx"
    tap::run "${BTX_TOOL}" encode "${TAP_CASE_DIR}/input.btx" "${TAP_CASE_DIR}/output.bin"
    tap::expect_status 0
    tap::expect_hex "${TAP_CASE_DIR}/output.bin" "$2"
}

# decode HEX BTX: the bytes HEX decode to BTX.
decode() {
    tap::write_hex "${TAP_CASE_DIR}/input.bin" "$1"
    tap::run "${BTX_TOOL}" decode "${TAP_CASE_DIR}/input.bin" "${TAP_CASE_DIR}/output.btx"
    tap::expect_status 0
    tap::expect_eq "$(< "${TAP_CASE_DIR}/output.btx")" "$2" "decoded BTX"
}

# reject BTX REGEX: encoding BTX fails with an error matching REGEX.
reject() {
    printf '%s' "$1" > "${TAP_CASE_DIR}/input.btx"
    tap::run "${BTX_TOOL}" encode "${TAP_CASE_DIR}/input.btx" "${TAP_CASE_DIR}/output.bin"
    tap::expect_fail
    tap::expect_match "${TAP_STDERR}" "$2"
}

tap::case "encode: hex"                 encode '\x48\x65\x6c\x6c\x6f'           48656c6c6f
tap::case "encode: bits"                encode '\b10101010'                     aa
tap::case "encode: bit packing"         encode '\b1111____ \b____0000'          f0
tap::case "encode: separators"          encode '\b1100'"'"'0011 \xDE \xAD'      c3dead
tap::case "encode: comments, blanks"    encode '// header
  \xAA  // comment

\xBB' aabb

tap::case "decode: hex"                 decode 48656c6c6f                       '\x48\x65\x6C\x6C\x6F'
tap::case "decode: multiple lines"      decode 0102030405060708090A             '\x01\x02\x03\x04\x05\x06\x07\x08
\x09\x0A'

tap::case "reject: invalid hex"         reject '\xGG'                           "Invalid hex character: G"
tap::case "reject: truncated hex"       reject '\x4'                            "Unexpected end of input"
tap::case "reject: truncated bits"      reject '\b101'                          "Incomplete .*b token"
tap::case "reject: malformed comment"   reject '/ Invalid'                      "Invalid comment: expected '//'"
tap::case "reject: hex after bits"      reject '\b1111 0xAA'                    "Incomplete .*b token"
tap::case "reject: 0x hex"              reject '0xAA'                           "Unexpected character: '0'"
tap::case "reject: 0b bits"             reject '0b10'                           "Unexpected character: '0'"

tap::done
