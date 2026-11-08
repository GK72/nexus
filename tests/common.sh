# shellcheck shell=bash
#
# Part of BTX Toolset
#
# Sourced by the function tests (`tests/function/*.sh`): the test library, the tools under test and
# the test data. The tools can be overridden from the environment, e.g. to test a release build:
#
#   BUILD=build/release tests/function/btx.sh

# shellcheck source=../tools/ci/nci-test.sh
source "$(dirname "$(realpath "${BASH_SOURCE[0]}")")/../tools/ci/nci-test.sh"

# Always trace, whatever the caller has: the descriptor test checks the trace log.
export SPDLOG_LEVEL=trace

# shellcheck disable=SC2034  # used by the test scripts
DATA="${TAP_ROOT}/tests/data"
BUILD="$(realpath -m "${BUILD:-${TAP_ROOT}/build/debug}")"
BTX_TOOL="${BTX_TOOL:-${BUILD}/btx/btx-tool}"
# shellcheck disable=SC2034  # used by the test scripts
MSG_GEN="${MSG_GEN:-${BUILD}/msg-gen/msg-gen}"

tap::require "${BTX_TOOL}" "build first: ci/pipeline.sh --only build"
