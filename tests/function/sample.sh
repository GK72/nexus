#!/usr/bin/env bash
#
# Part of BTX Toolset
#
# Function test of the sample message: `tests/data/sample.btx` encodes to its known bytes.

set -euo pipefail
# shellcheck source=../common.sh
source "$(dirname "$(realpath "$0")")/../common.sh"

encode_sample() {
    tap::run "${BTX_TOOL}" encode "${DATA}/sample.btx" "${TAP_CASE_DIR}/sample.bin"
    tap::expect_status 0
    tap::expect_hex "${TAP_CASE_DIR}/sample.bin" 0913e8d0054e65787573
}

tap::case "encode the sample"                   encode_sample

tap::done
