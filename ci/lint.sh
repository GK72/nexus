#!/bin/bash

set -o pipefail

clang-tidy -p "${WORKSPACE}" --warnings-as-errors='*' src/ncx/**.*{h,cpp} \
    | tee "${WORKSPACE}/build/artifacts/clang-tidy.log"
