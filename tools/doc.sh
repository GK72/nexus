#!/usr/bin/env bash
#
# Part of Nexus Library
#
# Renders AsciiDoc documents to standalone, themed HTML with the extensions of `tools/asciidoc/`.
# A document may have a companion extension next to it, `FILE.rb` for `FILE.adoc`, which is loaded
# for that document only (e.g. the model of an interactive block).
#
# Run with `--help` for the usage. Dependencies: asciidoctor, optionally rouge (gems).

set -euo pipefail

SCRIPT_NAME="$(basename "$0")"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
readonly SCRIPT_NAME SCRIPT_DIR

usage() {
    cat <<EOF
Usage: ${SCRIPT_NAME} [OPTION]... FILE.adoc...

Renders AsciiDoc documents to standalone HTML, one FILE.html each.

Options:
  -o, --out DIR             output directory (default: build/doc of the project root)
  -r, --require FILE.rb     extra Asciidoctor extension, registered on require; repeatable
  -a, --attribute NAME=VAL  document attribute, over the one in the document; repeatable
  -h, --help                show this help and exit
  --                        end of the options, the rest are files

Environment:
  ADOC_CSS   theme stylesheet (default: res/adoc/theme.css of Nexus)
  ADOC_ROOT  project root: the default output is under it, extensions may resolve paths
             against it (default: the git root of the working directory)
EOF
}

die() {
    printf '%s: %s\n' "${SCRIPT_NAME}" "$*" >&2
    exit 1
}

# Fails unless option `$1` has a value: `$2` is the number of arguments left, the option included.
need_value() {
    (($2 >= 2)) || die "option $1 needs a value (see --help)"
}

out_dir=""
requires=()
attributes=()
files=()

# Accepts `-o DIR`, `--out DIR` and `--out=DIR`; options and files in any order, `--` ends the options.
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
            -r | --require)
                need_value "$1" "$#"
                requires+=("$2")
                shift 2
                ;;
            --require=*)
                requires+=("${1#*=}")
                shift
                ;;
            -a | --attribute)
                need_value "$1" "$#"
                attributes+=("$2")
                shift 2
                ;;
            --attribute=*)
                attributes+=("${1#*=}")
                shift
                ;;
            --)
                shift
                files+=("$@")
                break
                ;;
            -?*)
                die "unknown option: $1 (see --help)"
                ;;
            *)
                files+=("$1")
                shift
                ;;
        esac
    done

    ((${#files[@]} > 0)) || die "no documents given (see --help)"
}

# Renders one document into `out_dir`; prints the output path.
render() {
    local -r file="$1"
    local -r out="${out_dir}/$(basename "${file}" .adoc).html"
    local -r companion="${file%.adoc}.rb"
    local args=(--safe-mode safe -r "${SCRIPT_DIR}/asciidoc/extensions.rb")

    local require
    for require in "${requires[@]}"; do
        args+=(-r "$(realpath "${require}")")
    done
    if [[ -f "${companion}" ]]; then
        args+=(-r "$(realpath "${companion}")")
    fi
    local attribute
    for attribute in "${attributes[@]}"; do
        args+=(-a "${attribute}")
    done

    # Safe mode jails `-D` to the document's directory, so the document goes through stdout.
    asciidoctor "${args[@]}" -o - "${file}" >"${out}"
    printf '%s\n' "${out}"
}

main() {
    parse_args "$@"

    command -v asciidoctor >/dev/null || die "asciidoctor not found (gem install asciidoctor)"
    local file
    for file in "${files[@]}"; do
        [[ -f "${file}" ]] || die "no such file: ${file}"
    done

    ADOC_ROOT="${ADOC_ROOT:-$(git rev-parse --show-toplevel 2>/dev/null || pwd)}"
    export ADOC_ROOT
    if [[ -n "${ADOC_CSS:-}" ]]; then
        ADOC_CSS="$(realpath "${ADOC_CSS}")"
        export ADOC_CSS
    fi
    out_dir="${out_dir:-${ADOC_ROOT}/build/doc}"

    if ruby -e 'require "rouge"' 2>/dev/null; then
        attributes=(source-highlighter=rouge rouge-css=class "${attributes[@]}")
    fi

    mkdir -p "${out_dir}"
    for file in "${files[@]}"; do
        render "${file}"
    done
}

main "$@"
