#!/usr/bin/env bash
#
# Stamps out a new C++ project from templates/cpp-project (documented in doc/new-project).
#
#   tools/new-project.sh <name> [--dir <dir>] [--namespace <ns>] [--part-of <text>]
#                        [--author <name>] [--description <text>]
#
# <name>        the project ([a-z][a-z0-9_-]*): the CMake project, the program <name>, the
#               library lib<name> and the book
# --dir         where to create it (default: next to Nexus, ../<name>); it may exist when it holds
#               nothing but a .git, e.g. a freshly cloned empty repository
# --namespace   the C++ namespace (default: <name>, with '-' as '_')
# --part-of     the project in the file headers, "Part of <text>" (default: <name>)
# --author      the files' @author (default: the author in Nexus's .file-headers.toml)
# --description the project's one-line summary for the README and the book
#
# The result is a git repository on `main` with nothing committed: review, then commit.

set -euo pipefail
shopt -s dotglob nullglob globstar

NEXUS_DIR="$(realpath "$(dirname "$(realpath "$0")")/..")"
TEMPLATE_DIR="${NEXUS_DIR}/templates/cpp-project"

usage() {
    sed -n '3,18s/^# \{0,1\}//p' "$0"
    exit "${1:-0}"
}

name=""
dir=""
namespace=""
part_of=""
author="$(sed -n 's/^author = "\(.*\)"$/\1/p' "${NEXUS_DIR}/.file-headers.toml")"
description="TODO: what this project is, in one sentence."

while (( $# )); do
    case "$1" in
        -h|--help)      usage ;;
        --dir)          dir="${2:?--dir needs a value}"; shift 2 ;;
        --namespace)    namespace="${2:?--namespace needs a value}"; shift 2 ;;
        --part-of)      part_of="${2:?--part-of needs a value}"; shift 2 ;;
        --author)       author="${2:?--author needs a value}"; shift 2 ;;
        --description)  description="${2:?--description needs a value}"; shift 2 ;;
        -*)             echo "unknown option: $1" >&2; usage 1 >&2 ;;
        *)              [[ -z "${name}" ]] || { echo "one name only" >&2; exit 1; }
                        name="$1"; shift ;;
    esac
done

[[ -n "${name}" ]] || usage 1 >&2
if [[ ! "${name}" =~ ^[a-z][a-z0-9_-]*$ ]]; then
    echo "not a lowercase name ([a-z][a-z0-9_-]*): ${name}" >&2
    exit 1
fi
namespace="${namespace:-${name//-/_}}"
if [[ ! "${namespace}" =~ ^[a-z_][a-z0-9_]*(::[a-z_][a-z0-9_]*)*$ ]]; then
    echo "not a C++ namespace: ${namespace}" >&2
    exit 1
fi
part_of="${part_of:-${name}}"
[[ -n "${author}" ]] || { echo "no author: pass --author" >&2; exit 1; }

dir="$(realpath -m "${dir:-${NEXUS_DIR}/../${name}}")"
if [[ -e "${dir}" ]]; then
    for entry in "${dir}"/*; do
        if [[ "$(basename "${entry}")" != .git ]]; then
            echo "${dir} is not empty: ${entry}" >&2
            exit 1
        fi
    done
fi

# Placeholders, in file contents: @name@, @NAME@, @namespace@, @part_of@, @author@, @description@,
# @date@; in paths: __name__. @NAME@ is the name for CMake and shell variables: capitals, '-' as '_'.
upper="${name^^}"
upper="${upper//-/_}"
today="$(date +%F)"

fill() {
    local text="$1"
    text="${text//@name@/"${name}"}"
    text="${text//@NAME@/"${upper}"}"
    text="${text//@namespace@/"${namespace}"}"
    text="${text//@part_of@/"${part_of}"}"
    text="${text//@author@/"${author}"}"
    text="${text//@description@/"${description}"}"
    text="${text//@date@/"${today}"}"
    printf '%s' "${text}"
}

mkdir -p "${dir}"
for source in "${TEMPLATE_DIR}"/**; do
    [[ -f "${source}" ]] || continue
    relative="${source#"${TEMPLATE_DIR}"/}"
    target="${dir}/${relative//__name__/${name}}"
    mkdir -p "$(dirname "${target}")"
    # Command substitution drops trailing newlines; every template file ends with exactly one.
    fill "$(cat "${source}")" > "${target}"
    printf '\n' >> "${target}"
    chmod --reference="${source}" "${target}"
done

if [[ ! -d "${dir}/.git" ]]; then
    git -C "${dir}" init --quiet --initial-branch main
fi

cat <<EOF
Created ${name} in ${dir}

Next:
  cd ${dir}
  ci/pipeline.sh                   # configure, build, test, render the book
  replace lib${name}/lib${name}/greeting.* with the first real module
  git add -A && git commit -m "Feat: Project skeleton"
EOF
