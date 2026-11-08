#!/usr/bin/env bash
#
# Part of Nexus Library
#
# Renders AsciiDoc documents to standalone, themed HTML with the plugins of `tools/asciidoc/`.
#
# A book is a folder with one `.adoc` (the spine, any name) and a `pages/` folder of pages, each of
# which also renders alone; pages named `_*.adoc` are partials, only included. A book renders into
# `OUT/<spine>/`: the spine as `index.html`, each page as `<page>.html` with navigation (the book,
# previous and next, in the order of the spine's includes). Any other `.adoc` renders as
# `OUT/<name>.html`.
#
# A document may have a companion extension next to it, `FILE.rb` for `FILE.adoc`, loaded for that
# document only (e.g. the model of an interactive block).
#
# Run with `--help` for the usage. Dependencies: asciidoctor, optionally rouge (gems); python3 for
# `--serve`, inotifywait (inotify-tools) for its file watching; a Chromium-based browser for `--shot`.

set -euo pipefail

SCRIPT_NAME="$(basename "$0")"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
NEXUS_DIR="$(dirname "${SCRIPT_DIR}")"
readonly SCRIPT_NAME SCRIPT_DIR NEXUS_DIR

usage() {
    cat <<EOF
Usage: ${SCRIPT_NAME} [OPTION]... (BOOK_DIR | FILE.adoc)...
       ${SCRIPT_NAME} --shot [--dark] FILE.html...

Renders AsciiDoc to standalone HTML. A BOOK_DIR (one .adoc, the spine, and a pages/ folder), or
its spine, renders the whole book into OUT/<spine>/; any other FILE.adoc renders to
OUT/FILE.html. For documents outside the book layout, pass the files: ${SCRIPT_NAME} doc/*.adoc

Options:
  -o, --out DIR             output directory (default: build/doc of the project root)
  -r, --require FILE.rb     extra Asciidoctor extension, registered on require; repeatable
  -a, --attribute NAME=VAL  document attribute, over the one in the document; repeatable
  --serve[=PORT]            render, serve OUT on 127.0.0.1:PORT (default: 8000), re-render on
                            changes; open pages reload themselves
  --interval SEC            seconds between checks when watching without inotifywait (0.5)
  --shot                    screenshot the given HTML files to FILE.png (no rendering)
  --dark                    with --shot: the dark theme, to FILE-dark.png
  --size WxH                with --shot: the window (default: 1300x2400)
  -h, --help                show this help and exit
  --                        end of the options, the rest are files

Environment:
  ADOC_STYLE   style level: plain, compact or full (default: full; the :theme-style: of a
               document wins)
  ADOC_SIZE    base size: small (15px), normal (16px) or large (17px); default: the
               tokens' --text (the :theme-size: of a document wins)
  ADOC_TOKENS  tokens of the theme (default: res/adoc/tokens.css of Nexus), e.g. a file
               exported from the Color or Font Studio
  ADOC_CSS     a stylesheet of the project, instead of the tokens and the style levels
  ADOC_ICONS   directories of project icons (NAME.svg, separated by ':'), over the icons
               of Nexus (res/adoc/icons/); used by i:NAME[] and as --icon-NAME in CSS
  ADOC_ROOT    project root: the default output is under it, extensions may resolve paths
               against it (default: the git root of the working directory)
  ADOC_BROWSER the browser of --shot (default: the first of chromium, google-chrome, brave, ...)
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

out_dir=""
requires=()
attributes=()
inputs=()
serve_port=""
interval="0.5"
shot=false
shot_dark=false
shot_size="1300,2400"

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
            --serve)
                serve_port=8000
                shift
                ;;
            --serve=*)
                serve_port="${1#*=}"
                [[ "${serve_port}" =~ ^[0-9]+$ ]] || die "--serve: not a port: ${serve_port}"
                shift
                ;;
            --interval)
                need_value "$1" "$#"
                interval="$2"
                shift 2
                ;;
            --interval=*)
                interval="${1#*=}"
                shift
                ;;
            --shot)
                shot=true
                shift
                ;;
            --dark)
                shot_dark=true
                shift
                ;;
            --size)
                need_value "$1" "$#"
                shot_size="${2/x/,}"
                shift 2
                ;;
            --size=*)
                shot_size="${1#*=}"
                shot_size="${shot_size/x/,}"
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
    [[ "${interval}" =~ ^[0-9]*\.?[0-9]+$ ]] || die "--interval: not a number of seconds: ${interval}"
}

# --------------------------------------------------------------------------------------- Planning

# The render jobs: source, output and, for the pages of a book, the spine
job_src=()
job_out=()
job_book=()

# Prints the spine of the book in folder `$1`: its only `.adoc`, with a `pages/` folder beside it.
book_spine() {
    local -r dir="$1"
    local spines=()
    [[ -d "${dir}/pages" ]] || return 1
    mapfile -t spines < <(find "${dir}" -maxdepth 1 -name '*.adoc' -type f)
    ((${#spines[@]} == 1)) || return 1
    realpath "${spines[0]}"
}

add_job() {
    job_src+=("$1")
    job_out+=("$2")
    job_book+=("$3")
}

declare -A books=()

add_book() {
    local -r spine="$1"
    [[ -z "${books[${spine}]:-}" ]] || return 0
    books[${spine}]=1
    local -r dest="${out_dir}/$(basename "${spine}" .adoc)"
    add_job "${spine}" "${dest}/index.html" ""
    local page
    while IFS= read -r page; do
        [[ "$(basename "${page}")" == _* ]] && continue
        add_job "$(realpath "${page}")" "${dest}/$(basename "${page}" .adoc).html" "${spine}"
    done < <(find "$(dirname "${spine}")/pages" -maxdepth 1 -name '*.adoc' -type f | sort)
}

plan() {
    local input spine
    for input in "${inputs[@]}"; do
        if [[ -d "${input}" ]]; then
            spine="$(book_spine "${input}")" ||
                die "${input}: not a book (one .adoc and a pages/ folder); pass its files: ${input%/}/*.adoc"
            add_book "${spine}"
        elif [[ -f "${input}" && "${input}" == *.adoc ]]; then
            if spine="$(book_spine "$(dirname "${input}")")" && [[ "${spine}" == "$(realpath "${input}")" ]]; then
                add_book "${spine}"
            else
                add_job "$(realpath "${input}")" "${out_dir}/$(basename "${input}" .adoc).html" ""
            fi
        else
            die "no such book or .adoc file: ${input}"
        fi
    done

    local -A outputs=()
    local i
    for i in "${!job_out[@]}"; do
        [[ -z "${outputs[${job_out[i]}]:-}" ]] ||
            die "${job_src[i]} and ${outputs[${job_out[i]}]} would both be ${job_out[i]}"
        outputs[${job_out[i]}]="${job_src[i]}"
    done
}

# -------------------------------------------------------------------------------------- Rendering

# Renders job `$1`; prints the output path.
render() {
    local -r src="${job_src[$1]}" out="${job_out[$1]}" book="${job_book[$1]}"
    local -r companion="${src%.adoc}.rb"
    local args=(--safe-mode safe -r "${SCRIPT_DIR}/asciidoc/extensions.rb")

    local require
    for require in "${requires[@]}"; do
        args+=(-r "$(realpath "${require}")")
    done
    if [[ -f "${companion}" ]]; then
        args+=(-r "${companion}")
    fi
    # Images go into the page as data URIs, so it stays one file; `-a data-uri!` links them instead
    args+=(-a data-uri)
    [[ -z "${book}" ]] || args+=(-a "nxs-book=${book}")
    [[ -z "${serve_port}" ]] || args+=(-a nxs-live)
    local attribute
    for attribute in "${attributes[@]}"; do
        args+=(-a "${attribute}")
    done

    mkdir -p "$(dirname "${out}")"
    # Safe mode jails `-D` to the document's directory, so the document goes through stdout.
    asciidoctor "${args[@]}" -o - "${src}" >"${out}"
    printf '%s\n' "${out}"
}

# Renders the jobs given by index; a failure is reported, the others still render.
render_jobs() {
    local i failed=0
    for i in "$@"; do
        render "${i}" || { warn "failed: ${job_src[i]}"; failed=1; }
    done
    return "${failed}"
}

# -------------------------------------------------------------------------------------- Watching

# The folders whose changes re-render: the books and the folders of single documents, the theme
# and the plugins of Nexus, and the project's stylesheet, tokens and extensions.
watch_dirs() {
    local i path
    {
        for i in "${!job_src[@]}"; do
            if [[ -n "${job_book[i]}" ]]; then dirname "$(dirname "${job_src[i]}")"; else dirname "${job_src[i]}"; fi
        done
        printf '%s\n' "${NEXUS_DIR}/res/adoc" "${SCRIPT_DIR}/asciidoc"
        for path in "${ADOC_CSS:-}" "${ADOC_TOKENS:-}" "${requires[@]}"; do
            [[ -z "${path}" ]] || dirname "$(realpath "${path}")"
        done
    } | sort -u
}

# The jobs that a change of the files `$@` re-renders, by index, one per line.
affected() {
    local path i spine
    local -A hit=()
    for path in "$@"; do
        [[ "${path}" == "${out_dir}"/* ]] && continue
        if [[ "${path}" == "${NEXUS_DIR}/res/adoc/"* || "${path}" == "${SCRIPT_DIR}/asciidoc/"* ||
              "${path}" == "${ADOC_CSS:-//}" || "${path}" == "${ADOC_TOKENS:-//}" ]]; then
            for i in "${!job_src[@]}"; do hit[${i}]=1; done
            continue
        fi
        for i in "${!job_src[@]}"; do
            spine="${job_book[i]:-${job_src[i]}}"
            if [[ -n "${books[${spine}]:-}" ]]; then
                # A book: a page re-renders itself and the spine; anything else, the whole book
                [[ "${path}" == "$(dirname "${spine}")/"* ]] || continue
                if [[ "${path}" == *.adoc && "$(basename "${path}")" != _* && "${path}" != "${spine}" ]]; then
                    [[ "${job_src[i]}" == "${path}" || "${job_src[i]}" == "${spine}" ]] && hit[${i}]=1
                else
                    hit[${i}]=1
                fi
            elif [[ "${path}" == "$(dirname "${job_src[i]}")/"* ]]; then
                # A single document: itself, its companion, or a file it may include
                if [[ "${path}" == "${job_src[i]}" || "${path}" == "${job_src[i]%.adoc}.rb" ]] ||
                   [[ "${path}" != *.adoc && "${path}" != *.rb ]]; then
                    hit[${i}]=1
                fi
            fi
        done
    done
    printf '%s\n' "${!hit[@]}" | sort -n | grep . || true
}

on_change() {
    local jobs=()
    mapfile -t jobs < <(affected "$@")
    ((${#jobs[@]} > 0)) || return 0
    printf '%s: changed: %s\n' "${SCRIPT_NAME}" "$*" >&2
    render_jobs "${jobs[@]}" || true
}

# Calls `on_change` with each batch of changed files, through inotify or, without it, by polling.
watch() {
    local dirs=()
    mapfile -t dirs < <(watch_dirs)
    if command -v inotifywait >/dev/null; then
        local path batch=()
        local fd
        exec {fd}< <(inotifywait -m -r -q -e close_write,moved_to,create,delete --format '%w%f' "${dirs[@]}")
        while IFS= read -r -u "${fd}" path; do
            batch=("${path}")
            # Editors write a file in several steps: collect what follows closely
            while IFS= read -r -t 0.2 -u "${fd}" path; do batch+=("${path}"); done
            on_change "${batch[@]}"
        done
    else
        warn "inotifywait not found (inotify-tools); checking for changes every ${interval} s"
        local before after changed=()
        # One line per file, `path<TAB>mtime`: a file that changed, appeared or went differs
        before="$(find "${dirs[@]}" -type f -printf '%p\t%T@\n' | LC_ALL=C sort)"
        while sleep "${interval}"; do
            after="$(find "${dirs[@]}" -type f -printf '%p\t%T@\n' | LC_ALL=C sort)"
            [[ "${after}" != "${before}" ]] || continue
            mapfile -t changed < <(LC_ALL=C comm -3 <(printf '%s\n' "${before}") <(printf '%s\n' "${after}") |
                sed 's/^\t//' | cut -f1 | sort -u)
            before="${after}"
            on_change "${changed[@]}"
        done
    fi
}

serve() {
    command -v python3 >/dev/null || die "--serve needs python3"
    python3 -m http.server "${serve_port}" --bind 127.0.0.1 --directory "${out_dir}" >/dev/null 2>&1 &
    local -r pid=$!
    trap 'kill '"${pid}"' 2>/dev/null' EXIT
    sleep 0.3
    kill -0 "${pid}" 2>/dev/null || die "--serve: port ${serve_port} is taken"
    local i
    for i in "${!job_out[@]}"; do
        [[ -n "${job_book[i]}" ]] && continue
        printf 'http://127.0.0.1:%s/%s\n' "${serve_port}" "${job_out[i]#"${out_dir}"/}"
    done
    printf '%s: serving %s; ctrl-c stops\n' "${SCRIPT_NAME}" "${out_dir}" >&2
    watch
}

# ----------------------------------------------------------------------------------- Screenshots

browser() {
    if [[ -n "${ADOC_BROWSER:-}" ]]; then
        command -v "${ADOC_BROWSER}" || die "ADOC_BROWSER not found: ${ADOC_BROWSER}"
        return
    fi
    local b
    for b in chromium chromium-browser google-chrome google-chrome-stable brave brave-browser microsoft-edge; do
        command -v "${b}" && return
    done
    die "--shot needs a Chromium-based browser (or ADOC_BROWSER)"
}

# Screenshots each HTML file to FILE.png, or FILE-dark.png in the dark theme (from a copy beside
# it with `data-theme="dark"`, so relative links still resolve).
shots() {
    local -r exe="$(browser)"
    local file page png
    for file in "${inputs[@]}"; do
        [[ -f "${file}" && "${file}" == *.html ]] || die "--shot: not an HTML file: ${file}"
        page="$(realpath "${file}")"
        png="${page%.html}.png"
        if ${shot_dark}; then
            png="${page%.html}-dark.png"
            page="${page%.html}.dark-shot.html"
            sed 's/<html lang="\([^"]*\)">/<html lang="\1" data-theme="dark">/' "${file}" >"${page}"
        fi
        "${exe}" --headless=new --hide-scrollbars --window-size="${shot_size}" --virtual-time-budget=3000 \
            --screenshot="${png}" "file://${page}" >/dev/null 2>&1 || warn "--shot failed: ${file}"
        ${shot_dark} && rm -f "${page}"
        printf '%s\n' "${png}"
    done
}

main() {
    parse_args "$@"

    if ${shot}; then
        shots
        return
    fi

    command -v asciidoctor >/dev/null || die "asciidoctor not found (gem install asciidoctor)"

    ADOC_ROOT="${ADOC_ROOT:-$(git rev-parse --show-toplevel 2>/dev/null || pwd)}"
    export ADOC_ROOT
    local var
    for var in ADOC_CSS ADOC_TOKENS; do
        if [[ -n "${!var:-}" ]]; then
            export "${var}=$(realpath "${!var}")"
        fi
    done
    if [[ -n "${ADOC_ICONS:-}" ]]; then
        local dir dirs=()
        IFS=: read -ra dirs <<< "${ADOC_ICONS}"
        for dir in "${dirs[@]}"; do
            [[ -d "${dir}" ]] || die "ADOC_ICONS: no such directory: ${dir}"
        done
        ADOC_ICONS="$(for dir in "${dirs[@]}"; do realpath "${dir}"; done | paste -sd:)"
        export ADOC_ICONS
    fi
    out_dir="$(realpath -m "${out_dir:-${ADOC_ROOT}/build/doc}")"

    if ruby -e 'require "rouge"' 2>/dev/null; then
        attributes=(source-highlighter=rouge rouge-css=class "${attributes[@]}")
    fi

    plan
    mkdir -p "${out_dir}"
    if [[ -n "${serve_port}" ]]; then
        render_jobs "${!job_src[@]}" || true
        serve
    else
        render_jobs "${!job_src[@]}"
    fi
}

main "$@"
