# shellcheck shell=bash
#
# Sourced by the function tests (`tests/function/*.sh`): the test library (nci-test, from Nexus)
# and the program under test. Override from the environment, e.g.
# `BUILD=build/release tests/function/cli.sh`.

PROJECT_DIR="$(realpath "$(dirname "$(realpath "${BASH_SOURCE[0]}")")/..")"
NEXUS_DIR="${NEXUS_DIR:-$(realpath -m "${PROJECT_DIR}/../nexus")}"
# shellcheck source=/dev/null
source "${NEXUS_DIR}/tools/ci/nci-test.sh"

BUILD="$(realpath -m "${BUILD:-${PROJECT_DIR}/build/debug}")"
@NAME@_BIN="${@NAME@_BIN:-${BUILD}/bin/@name@}"

tap::require "${@NAME@_BIN}" "build first: ci/pipeline.sh --only build"
