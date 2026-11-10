#!/bin/bash
#
# Shows how a draft's prose was reworded, word by word, to learn the writing style from a
# review: the author fixes a draft in a commit, the diff of the two shows the rules.
#
# Usage:
#   style-diff.sh FIXUP              The changes of the commit FIXUP.
#   style-diff.sh DRAFT FIXUP        The changes from DRAFT to FIXUP.
#   style-diff.sh ... -- PATH...     Other files than *.adoc and *.md.
#
# Whitespace-only changes, such as rewrapping, are left out. Off a terminal, the
# changes are marked [-removed-]{+added+} instead of coloured.

set -euo pipefail

usage() {
    sed -n '3,12p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
    exit "${1:-0}"
}

revs=()
paths=()
while (( $# )); do
    case "$1" in
        -h|--help) usage ;;
        --)        shift; paths=("$@"); break ;;
        -*)        echo "style-diff: unknown option: $1" >&2; usage 2 ;;
        *)         revs+=("$1") ;;
    esac
    shift
done

case ${#revs[@]} in
    1) range=("${revs[0]}^" "${revs[0]}") ;;
    2) range=("${revs[0]}" "${revs[1]}") ;;
    *) usage 2 ;;
esac
(( ${#paths[@]} )) || paths=('*.adoc' '*.md')

mode=plain
[[ -t 1 ]] && mode=color

git diff --word-diff="${mode}" --word-diff-regex='[^[:space:]]+' --ignore-all-space \
    --stat --patch "${range[@]}" -- "${paths[@]}"
