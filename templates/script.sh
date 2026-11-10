#!/usr/bin/env bash
#
# Part of Nexus Library
#
# One-line summary of what the script does.
#
# A longer description: the inputs, the outputs and the conventions the script relies on.
#
# Run with `--help` for the usage. Dependencies: list the external tools here.

set -euo pipefail

SCRIPT_NAME="$(basename "$0")"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
readonly SCRIPT_NAME SCRIPT_DIR

usage() {
    cat <<EOF
Usage: ${SCRIPT_NAME} [OPTION]... INPUT...

One-line summary of what the script does.

Options:
  -o, --out DIR             output directory (default: .)
  -v, --verbose             report each step on stderr
  -h, --help                show this help and exit
  --                        end of the options, the rest are inputs

Environment:
  EXAMPLE_VAR  what it changes (default: ...)
EOF
}

die() {
    printf '%s: %s\n' "${SCRIPT_NAME}" "$*" >&2
    exit 1
}

warn() {
    printf '%s: %s\n' "${SCRIPT_NAME}" "$*" >&2
}

# Fails unless option `$1` has a value: `$2` is the number of arguments left, the option included.
need_value() {
    (($2 >= 2)) || die "option $1 needs a value (see --help)"
}

out_dir="."
verbose=false
inputs=()

# Accepts `-o DIR`, `--out DIR` and `--out=DIR`; options and inputs in any order, `--` ends the options.
parse_args() {
    while (($# > 0)); do
        case "$1" in
            -h | --help)
                usage
                exit 0
                ;;
            -o | --out)
                need_value "$1" "$#"
                out_dir="$2"
                shift 2
                ;;
            --out=*)
                out_dir="${1#*=}"
                shift
                ;;
            -v | --verbose)
                verbose=true
                shift
                ;;
            --)
                shift
                inputs+=("$@")
                break
                ;;
            -?*)
                die "unknown option: $1 (see --help)"
                ;;
            *)
                inputs+=("$1")
                shift
                ;;
        esac
    done

    ((${#inputs[@]} > 0)) || die "nothing given (see --help)"
}

# ---------------------------------------------------------------------------------------- Section

# Processes input `$1`.
process() {
    local -r input="$1"
    ! ${verbose} || warn "processing: ${input}"
}

main() {
    parse_args "$@"

    # Check the dependencies up front, e.g.:
    # command -v tool >/dev/null || die "tool not found"

    mkdir -p "${out_dir}"
    local input
    for input in "${inputs[@]}"; do
        process "${input}"
    done
}

main "$@"
