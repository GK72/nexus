#!/usr/bin/env bash
#
# Part of Nexus Library
#
# Builds the documentation site of a project: renders its books and documents with `tools/doc.sh`
# from a clean export of a commit (so nothing uncommitted or untracked gets in) and adds an index
# page linking them. The output is a folder of static files, ready for GitHub Pages; the workflow
# `.github/workflows/docs.yml` deploys it on demand.
#
# Works for any project: run it from inside the project's repository. Run with `--help` for the
# usage. Dependencies: git, and those of `tools/doc.sh`.

set -euo pipefail

SCRIPT_NAME="$(basename "$0")"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
readonly SCRIPT_NAME SCRIPT_DIR
readonly DOC_SH="${SCRIPT_DIR}/doc.sh"

usage() {
    cat <<EOF
Usage: ${SCRIPT_NAME} [OPTION]... [BOOK_DIR | FILE.adoc]...

Builds the documentation site of the project at REF into OUT: every book and document, and an
index.html linking them. Paths are relative to the project root. Without paths, everything under
DIR: each book (a folder with one .adoc and a pages/ folder), and every other .adoc not starting
with '_'.

Options:
  -o, --out OUT        the site folder, emptied first (default: build/site of the project root)
  --ref REF            the commit to render (default: HEAD)
  --working-tree       render the files as they are on disk instead, untracked ones included
  --dir DIR            where to look for documents without paths (default: doc)
  --title TITLE        the title of the index page (default: "<repository> Documentation", by
                       the name of the repository on origin)
  -h, --help           show this help and exit

Examples:
  ${SCRIPT_NAME}                                    # the site of HEAD, into build/site
  ${SCRIPT_NAME} --working-tree                     # with the uncommitted changes
  ${SCRIPT_NAME} doc/documenting
EOF
}

die() {
    printf '%s: %s\n' "${SCRIPT_NAME}" "$*" >&2
    exit 1
}

info() {
    printf '%s: %s\n' "${SCRIPT_NAME}" "$*" >&2
}

need_value() {
    (($2 >= 2)) || die "option $1 needs a value (see --help)"
}

out=""
ref="HEAD"
working_tree=false
doc_dir="doc"
title=""
inputs=()
source_label=""

parse_args() {
    while (($# > 0)); do
        case "$1" in
            -h | --help) usage; exit 0 ;;
            -o | --out) need_value "$1" "$#"; out="$2"; shift 2 ;;
            --out=*) out="${1#*=}"; shift ;;
            --ref) need_value "$1" "$#"; ref="$2"; shift 2 ;;
            --ref=*) ref="${1#*=}"; shift ;;
            --working-tree) working_tree=true; shift ;;
            --dir) need_value "$1" "$#"; doc_dir="$2"; shift 2 ;;
            --dir=*) doc_dir="${1#*=}"; shift ;;
            --title) need_value "$1" "$#"; title="$2"; shift 2 ;;
            --title=*) title="${1#*=}"; shift ;;
            --) shift; inputs+=("$@"); break ;;
            -?*) die "unknown option: $1 (see --help)" ;;
            *) inputs+=("$1"); shift ;;
        esac
    done
    ${working_tree} && [[ "${ref}" != "HEAD" ]] && die "--ref and --working-tree exclude each other"
    return 0
}

# ------------------------------------------------------------------------------------- Documents

# Succeeds if folder `$1` is a book: one `.adoc` and a `pages/` folder (as in `tools/doc.sh`).
is_book() {
    local spines=()
    [[ -d "$1/pages" ]] || return 1
    mapfile -t spines < <(find "$1" -maxdepth 1 -name '*.adoc' -type f)
    ((${#spines[@]} == 1))
}

# Prints the inputs found under folder `$1`, relative to the current directory: the books, and the
# other documents that are not partials, not descending into books.
discover() {
    local -r dir="$1"
    if is_book "${dir}"; then
        printf '%s\n' "${dir}"
        return
    fi
    local entry
    for entry in "${dir}"/*; do
        if [[ -d "${entry}" ]]; then
            discover "${entry}"
        elif [[ "${entry}" == *.adoc && "$(basename "${entry}")" != _* ]]; then
            printf '%s\n' "${entry}"
        fi
    done
}

# Prints the level-0 title of document `$1`, or its name without one.
doc_title() {
    local line
    while IFS= read -r line; do
        if [[ "${line}" == "= "* ]]; then
            printf '%s\n' "${line#= }"
            return
        fi
    done <"$1"
    basename "$1" .adoc
}

# Writes the index page, `$1/index.adoc`, linking the inputs: a book by the title of its spine, a
# document by its own.
write_index() {
    local -r dir="$1"
    local books=() docs=() input spine
    for input in "${inputs[@]}"; do
        input="${input%/}"
        if [[ -d "${input}" ]]; then
            spine="$(find "${input}" -maxdepth 1 -name '*.adoc' -type f)"
            books+=("link:$(basename "${spine}" .adoc)/index.html[$(doc_title "${spine}")]")
        elif is_book "$(dirname "${input}")"; then
            books+=("link:$(basename "${input}" .adoc)/index.html[$(doc_title "${input}")]")
        else
            docs+=("link:$(basename "${input}" .adoc).html[$(doc_title "${input}")]")
        fi
    done

    {
        printf '= %s\n:nofooter:\n\n' "${title}"
        # shellcheck disable=SC2016  # AsciiDoc backticks, not a command substitution
        printf 'Generated from `%s`.\n\n' "${source_label}"
        if ((${#books[@]} > 0)); then
            printf '== Books\n\n'
            printf '* %s\n' "${books[@]}"
            printf '\n'
        fi
        if ((${#docs[@]} > 0)); then
            printf '== Documents\n\n'
            printf '* %s\n' "${docs[@]}"
        fi
    } >"${dir}/index.adoc"
}

main() {
    parse_args "$@"
    [[ -x "${DOC_SH}" ]] || die "not found: ${DOC_SH}"

    local root commit
    root="$(git rev-parse --show-toplevel 2>/dev/null)" || die "not in a git repository"
    cd "${root}"
    commit="$(git rev-parse --verify -q "${ref}^{commit}")" || die "no such commit: ${ref}"
    out="$(realpath -m "${out:-${root}/build/site}")"
    # OUT is emptied: never the root, home, or a folder holding the project
    if [[ "${out}" == / || "${out}" == "${HOME}" || "${root}/" == "${out}"/* ]]; then
        die "--out: refusing to empty ${out}"
    fi

    local -r tmp="$(mktemp -d)"
    # shellcheck disable=SC2064  # expand now: `tmp` is local
    trap "rm -rf '${tmp}'" EXIT

    # The source: an export of the commit, or the files on disk
    local src="${root}"
    if ${working_tree}; then
        source_label="the working tree at $(git rev-parse --short HEAD)"
        [[ -z "$(git status --porcelain)" ]] || info "the working tree has changes; they are in the site too"
    else
        src="${tmp}/src"
        mkdir -p "${src}"
        git archive "${commit}" | tar -x -C "${src}"
        source_label="$(git rev-parse --short "${commit}")"
    fi
    if [[ -z "${title}" ]]; then
        # The name of the repository on origin, else of its folder
        local name
        name="$(git remote get-url origin 2>/dev/null || printf '%s' "${root}")"
        name="$(basename "${name%/}" .git)"
        title="${name} Documentation"
    fi

    cd "${src}"
    if ((${#inputs[@]} == 0)); then
        [[ -d "${doc_dir}" ]] || die "no ${doc_dir}/ folder at ${source_label}; pass the documents or --dir"
        mapfile -t inputs < <(discover "${doc_dir%/}")
        ((${#inputs[@]} > 0)) || die "no documents under ${doc_dir}/"
    fi
    local input
    for input in "${inputs[@]}"; do
        [[ -e "${input}" ]] || die "not found at ${source_label}: ${input}"
        [[ "$(basename "${input%/}")" != index.adoc ]] || die "${input}: index.html is the site's own index"
    done

    rm -rf "${out}"
    info "rendering ${#inputs[@]} book(s) and document(s) of ${source_label}"
    ADOC_ROOT="${src}" "${DOC_SH}" -o "${out}" "${inputs[@]}" >/dev/null
    write_index "${tmp}"
    ADOC_ROOT="${src}" "${DOC_SH}" -o "${out}" "${tmp}/index.adoc" >/dev/null
    # Pages are served as they are, not built by Jekyll
    touch "${out}/.nojekyll"
    info "the site is in ${out}: ${out}/index.html"
}

main "$@"
