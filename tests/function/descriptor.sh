#!/usr/bin/env bash
#
# Part of BTX Toolset
#
# Function tests of the descriptor parser: a message decoded with a descriptor is logged field by
# field.

set -euo pipefail
# shellcheck source=../common.sh
source "$(dirname "$(realpath "$0")")/../common.sh"

parse_sample() {
    tap::run "${BTX_TOOL}" encode "${DATA}/sample.btx" "${TAP_CASE_DIR}/sample.bin"
    tap::expect_status 0
    tap::run "${BTX_TOOL}" decode "${TAP_CASE_DIR}/sample.bin" -d "${DATA}/descriptor.yaml"
    tap::expect_status 0
    tap::expect_contains "${TAP_OUTPUT}" \
        'Parsed message: player' \
        'message_length: 9 (0x9)' \
        'version: 1 (0x1)' \
        'id: 1000 (0x3e8)' \
        'is_alive: true' \
        'health: 80 (0x50)' \
        'name_length: 5 (0x5)' \
        'name: Nexus'
}

tap::case "parse the sample"                    parse_sample

tap::done
