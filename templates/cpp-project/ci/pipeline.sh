#!/usr/bin/env bash
#
# The CI pipeline of @name@, run with Nexus's nci (`tools/ci/nci.sh`, documented in Nexus's `doc/nci`).
# Nexus is expected next to this repository; override with NEXUS_DIR.
#
#   ci/pipeline.sh                          everything
#   ci/pipeline.sh --changed                only what the changes since main affect
#   ci/pipeline.sh --only function-test     a group of stages (and what they need)
#   ci/pipeline.sh --list                   the stages, and what would run
#   ci/pipeline.sh --help                   all options

PROJECT_DIR="$(realpath "$(dirname "$(realpath "$0")")/..")"
NEXUS_DIR="${NEXUS_DIR:-$(realpath -m "${PROJECT_DIR}/../nexus")}"
# shellcheck source=/dev/null
source "${NEXUS_DIR}/tools/ci/nci.sh"

BUILD_DIR="${BUILD_DIR:-build/debug}"
CPP_PATHS='CMakeLists.txt version.txt *.cmake cmake/ conanfile.py conandata.yml tools/configure.sh lib@name@/ @name@/'

lint_shell() {
    command -v shellcheck >/dev/null || nci::skip "shellcheck not installed"
    shellcheck --external-sources ci/*.sh tools/*.sh tests/common.sh tests/function/*.sh
}

build() {
    if [[ ! -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
        nci::step configure tools/configure.sh "${BUILD_DIR}" Debug
    fi
    nci::step compile cmake --build "${BUILD_DIR}"
}

unit_test() {
    local out="${NCI_STAGE_DIR}/ctest.txt"
    ctest --test-dir "${BUILD_DIR}" --output-on-failure --no-tests=error | tee "${out}"
    nci::note "$(grep -Eo '[0-9]+% tests passed.*' "${out}")"
}

# NCI_ARTIFACTS exists only once the run started: read it in the stage, not in its declaration.
docs() {
    tools/doc.sh --out "${NCI_ARTIFACTS}/doc" doc
}

nci::init @name@
nci::stage  lint-shell    --paths 'ci/ tools/ tests/'                          -- lint_shell
nci::stage  file-headers  --paths '*.cpp *.hpp .file-headers.toml' --allow-failure \
                                                                               -- python3 "${NEXUS_DIR}/tools/file_headers.py" --check
nci::stage  build         --paths "${CPP_PATHS}"                                -- build
nci::stage  unit-test     --paths "${CPP_PATHS}"              --needs build     -- unit_test
nci::stages function-test --paths "${CPP_PATHS} tests/"       --needs build     -- tests/function/*.sh
nci::stage  docs          --paths 'doc/ tools/doc.sh'                           -- docs
nci::main "$@"
