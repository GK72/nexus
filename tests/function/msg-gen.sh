#!/usr/bin/env bash
#
# Part of BTX Toolset
#
# Function tests of msg-gen: a randomly generated message of a descriptor decodes with btx-tool.

set -euo pipefail
# shellcheck source=../common.sh
source "$(dirname "$(realpath "$0")")/../common.sh"

tap::require "${MSG_GEN}" "build first: ci/pipeline.sh --only build"

generate_and_parse() {
    local message="${TAP_CASE_DIR}/random.bin"
    tap::run "${MSG_GEN}" --descriptor "${DATA}/descriptor.yaml" --output "${message}"
    tap::expect_status 0
    [[ -s "${message}" ]] || tap::_fail "no message generated at ${message}"

    tap::run "${BTX_TOOL}" decode --descriptor "${DATA}/descriptor.yaml" --input "${message}"
    tap::expect_status 0
    tap::expect_contains "${TAP_STDOUT}" \
        'message_length:' 'version:' 'id:' 'is_alive:' 'health:' 'name_length:' 'name:'
}

tap::case "generate a message and parse it"     generate_and_parse

tap::done
