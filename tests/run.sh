#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(dirname "$(realpath "$0")")"

"${SCRIPT_DIR}/annotation.sh"
"${SCRIPT_DIR}/btx.sh"
"${SCRIPT_DIR}/descriptor.sh"
"${SCRIPT_DIR}/msg-gen.sh"
"${SCRIPT_DIR}/sample.sh"
"${SCRIPT_DIR}/stdout.sh"
