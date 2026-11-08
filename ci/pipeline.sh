#!/usr/bin/env bash
#
# Part of Nexus Library
#
# The CI pipeline of Nexus, run with `tools/ci/nci.sh` (see the nci book, `doc/nci`). By
# default in place, on the working tree, where the C++ build is the developer build (`build/debug`).
# The file headers are allowed to fail until the old headers are migrated (`--strict` to enforce).
#
#   ci/pipeline.sh                          everything
#   ci/pipeline.sh --changed                only what the changes since master affect
#   ci/pipeline.sh --only function-test     a group of stages (and what they need)
#   ci/pipeline.sh --clean                  on a fresh clone of HEAD in /tmp/workspace/nexus
#   ci/pipeline.sh --list                   the stages, and what would run
#   ci/pipeline.sh --help                   all options

# shellcheck source=../tools/ci/nci.sh
source "$(dirname "$(realpath "$0")")/../tools/ci/nci.sh"

CPP_PATHS='CMakeLists.txt CMakeUserPresets.json *.cmake cmake/ conanfile.txt deps/ libbtx/ liblexy/ libnxs/ btx/ msg-gen/'
TEST_PATHS='tests/common.sh tests/data/ tools/ci/nci-test.sh'
DOC_PATHS='doc/ res/adoc/ tools/doc.sh tools/doc-site.sh tools/asciidoc/'

lint_shell() {
    command -v shellcheck >/dev/null || nci::skip "shellcheck not installed"
    shellcheck --external-sources tools/ci/*.sh ci/*.sh tests/common.sh tests/function/*.sh
}

unit_test() {
    local out="${NCI_STAGE_DIR}/ctest.txt"
    ctest --test-dir build/debug --output-on-failure | tee "${out}"
    nci::note "$(grep -Eo '[0-9]+% tests passed.*' "${out}")"
}

docs() {
    tools/doc-site.sh --working-tree --out "${NCI_ARTIFACTS}/site"
    nci::note "$(find "${NCI_ARTIFACTS}/site" -name '*.html' | wc -l) pages"
}

nci::init nexus
nci::stage  lint-shell    --paths 'ci/ tools/ci/ tests/'                     -- lint_shell
nci::stage  file-headers  --paths '*.cpp *.hpp *.h .file-headers.toml tools/file_headers.py' --allow-failure \
                                                                              -- python3 tools/file_headers.py --check
nci::stage  tools-test    --paths 'tools/*.py'                                -- python3 tools/file_headers_test.py
nci::stage  build         --paths "${CPP_PATHS}"                              -- baldr build -b Debug
nci::stage  unit-test     --paths "${CPP_PATHS}"              --needs build   -- unit_test
nci::stages function-test --paths "${CPP_PATHS} ${TEST_PATHS}" --needs build  -- tests/function/*.sh
nci::stage  docs          --paths "${DOC_PATHS}"                              -- docs
nci::main "$@"
