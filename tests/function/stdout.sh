#!/usr/bin/env bash
#
# Part of BTX Toolset
#
# Function tests of btx-tool writing to stdout when no output file is given.

set -euo pipefail
# shellcheck source=../common.sh
source "$(dirname "$(realpath "$0")")/../common.sh"

encode_to_stdout() {
    printf '%s\n' '\x48\x65\x6c\x6c\x6f' > "${TAP_CASE_DIR}/hello.btx"
    tap::run "${BTX_TOOL}" encode "${TAP_CASE_DIR}/hello.btx"
    tap::expect_status 0
    tap::expect_hex "${TAP_STDOUT}" 48656c6c6f
}

decode_to_stdout() {
    printf 'Hello' > "${TAP_CASE_DIR}/hello.bin"
    tap::run "${BTX_TOOL}" decode "${TAP_CASE_DIR}/hello.bin"
    tap::expect_status 0
    tap::expect_stdout '\x48\x65\x6C\x6C\x6F'
}

encode_sample_to_stdout() {
    tap::run "${BTX_TOOL}" encode "${DATA}/sample.btx"
    tap::expect_status 0
    tap::expect_hex "${TAP_STDOUT}" 0913e8d0054e65787573
}

decode_annotated_to_stdout() {
    tap::write_hex "${TAP_CASE_DIR}/sample.bin" 0913e8d0054e65787573
    tap::run "${BTX_TOOL}" decode "${TAP_CASE_DIR}/sample.bin" -d "${DATA}/descriptor.yaml"
    tap::expect_status 0
    tap::expect_stdout "$(cat <<'EOF'
// player
\x09                                     // message_length: 9 (0x9)
\b0001                                   // version: 1 (0x1)
\b001111101000                           // id: 1000 (0x3e8)
\b1                                      // is_alive: 1 (0x1)
\b1010000                                // health: 80 (0x50)
\x05                                     // name_length: 5 (0x5)
\x4E\x65\x78\x75\x73                     // name: "Nexus"
EOF
)"
}

tap::case "encode to stdout"                    encode_to_stdout
tap::case "decode to stdout"                    decode_to_stdout
tap::case "encode the sample to stdout"         encode_sample_to_stdout
tap::case "decode annotated to stdout"          decode_annotated_to_stdout

tap::done
