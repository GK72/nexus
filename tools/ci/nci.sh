# shellcheck shell=bash
#
# Part of Nexus Library
#
# nci: a CI pipeline runner, sourced by a project's pipeline script (e.g. `ci/pipeline.sh`).
#
#     source "<nexus>/tools/ci/nci.sh"
#     nci::init myproject
#     nci::stage  build     --paths 'src/' -- baldr -p . -b Debug build
#     nci::stages func-test --needs build  -- tests/function/*.sh
#     nci::main "$@"
#
# Documentation (options, stage helpers, conventions, architecture): the nci book, `doc/nci`.
# Requires bash 4.4+ (5.0+ for precise timing).

[[ -n "${NCI_VERSION:-}" ]] && return 0
readonly NCI_VERSION="0.2.0"

# Lines that look like errors: compiler and tool errors, test frameworks (gtest, ctest, TAP), asserts.
NCI_ERRORS="${NCI_ERRORS:-(^|[^[:alnum:]_])(error|Error|ERROR|FAIL|FAILED|FAILURE|Failed|fatal|Fatal|FATAL|panic|Assertion|Traceback|Bail out!)([^[:alnum:]_]|$)|^not ok }"

# ---------------------------------------------------------------------------------- Registry ----

NCI_PROJECT=""
NCI_STAGE_NAMES=()
NCI_STAGE_CMDS=()
NCI_STAGE_PATHS=()
NCI_STAGE_NEEDS=()
NCI_STAGE_SOFT=()
NCI_STAGE_ERRORS=()
NCI_CALLER_DIR="${PWD}"

# nci::init [PROJECT]: names the project (default: the name of the repository's directory) and
# changes to its root.
nci::init() {
    NCI_ROOT="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
    NCI_PROJECT="${1:-$(basename "${NCI_ROOT}")}"
    cd "${NCI_ROOT}" || nci::_die "cannot change to ${NCI_ROOT}"
}

# nci::stage NAME [--paths 'GLOB...'] [--needs 'STAGE...'] [--allow-failure] [--errors REGEX]
#            -- COMMAND [ARG...]
nci::stage() {
    local name="$1"; shift
    local paths="" needs="" soft=0 errors=""
    while (( $# )); do
        case "$1" in
            --paths)         paths="$2"; shift 2 ;;
            --needs)         needs="$2"; shift 2 ;;
            --allow-failure) soft=1; shift ;;
            --errors)        errors="$2"; shift 2 ;;
            --)              shift; break ;;
            *)               nci::_die "stage '${name}': unknown option '$1' (missing '--' before the command?)" ;;
        esac
    done
    (( $# )) || nci::_die "stage '${name}': no command"
    [[ -z "$(nci::_index "${name}")" ]] || nci::_die "stage '${name}' declared twice"

    local need resolved="" expanded
    for need in ${needs}; do
        expanded="$(nci::_expand "${need}")"
        [[ -n "${expanded}" ]] || nci::_die "stage '${name}' needs '${need}', which is not declared before it"
        resolved+="${resolved:+ }${expanded}"
    done

    NCI_STAGE_NAMES+=("${name}")
    NCI_STAGE_CMDS+=("$(printf '%q ' "$@")")
    NCI_STAGE_PATHS+=("${paths}")
    NCI_STAGE_NEEDS+=("${resolved}")
    NCI_STAGE_SOFT+=("${soft}")
    NCI_STAGE_ERRORS+=("${errors}")
}

# nci::stages GROUP [STAGE OPTION...] -- FILE...: one stage per file, GROUP/<name>, running the file.
nci::stages() {
    local group="$1"; shift
    local -a opts=()
    local paths=""
    while (( $# )) && [[ "$1" != -- ]]; do
        case "$1" in
            --paths)         paths="$2"; shift 2 ;;
            --allow-failure) opts+=("$1"); shift ;;
            *)               opts+=("$1" "${2:-}"); shift 2 ;;
        esac
    done
    [[ "${1:-}" == -- ]] || nci::_die "stages '${group}': missing '--' before the files"
    shift
    (( $# )) || nci::_die "stages '${group}': no files"

    local file base
    for file in "$@"; do
        [[ -f "${file}" ]] || nci::_die "stages '${group}': no such file '${file}'"
        base="$(basename "${file}")"
        nci::stage "${group}/${base%.*}" "${opts[@]}" --paths "${paths:+${paths} }${file}" -- "${file}"
    done
}

# ------------------------------------------------------------------------- Stage-side helpers ----

nci::note() {
    [[ -n "${NCI_STAGE_DIR:-}" ]] || return 0
    printf '%s\n' "$*" >> "${NCI_STAGE_DIR}/notes"
}

nci::skip() {
    printf '%s\n' "${*:-skipped}" > "${NCI_STAGE_DIR}/skipped"
    exit 0
}

nci::attach() {
    local file
    for file in "$@"; do
        realpath -m "${file}" >> "${NCI_STAGE_DIR}/attachments"
    done
}

# nci::step NAME COMMAND [ARG...]: runs the command as a step of the stage, in a subshell with
# `set -euo pipefail`; returns its exit code.
nci::step() {
    local name="$1"; shift
    local first_line start rc secs
    first_line=$(( $(wc -l < "${NCI_STAGE_LOG}") + 1 ))
    printf '#> step %s\n' "${name}"
    start="$(nci::_now)"
    set +e
    ( set -euo pipefail; "$@" )
    rc=$?
    set -e
    secs="$(nci::_elapsed "${start}" "$(nci::_now)")"
    if (( rc )); then
        printf '#> step %s: exit code %d after %ss\n' "${name}" "${rc}" "${secs}"
    else
        printf '#> step %s: ok after %ss\n' "${name}" "${secs}"
    fi
    printf '%s\t%s\t%s\t%s\n' "${name}" "${rc}" "${secs}" "${first_line}" >> "${NCI_STAGE_DIR}/steps"
    return "${rc}"
}

# ----------------------------------------------------------------------------------- Output ----

nci::_colors() {
    if [[ -t 1 && -z "${NO_COLOR:-}" ]]; then
        C_RESET=$'\e[0m' C_BOLD=$'\e[1m' C_DIM=$'\e[2m'
        C_RED=$'\e[31m' C_GREEN=$'\e[32m' C_YELLOW=$'\e[33m' C_BLUE=$'\e[34m'
    else
        C_RESET="" C_BOLD="" C_DIM="" C_RED="" C_GREEN="" C_YELLOW="" C_BLUE=""
    fi
}

nci::_die() {
    printf 'nci: error: %s\n' "$*" >&2
    exit 2
}

nci::_now() {
    if [[ -n "${EPOCHREALTIME:-}" ]]; then
        printf '%s' "${EPOCHREALTIME/,/.}"
    else
        date +%s
    fi
}

# nci::_elapsed START END: seconds with one decimal.
nci::_elapsed() {
    awk -v start="$1" -v end="$2" 'BEGIN { printf "%.1f", end - start }'
}

nci::_duration() {
    awk -v secs="$1" 'BEGIN {
        if (secs < 60) printf "%.1fs", secs
        else printf "%dm%02ds", int(secs / 60), int(secs) % 60
    }'
}

nci::_status_text() {
    case "$1" in
        ok)          printf '%s%-7s%s' "${C_GREEN}" "ok" "${C_RESET}" ;;
        fail)        printf '%s%-7s%s' "${C_RED}${C_BOLD}" "FAIL" "${C_RESET}" ;;
        warn)        printf '%s%-7s%s' "${C_YELLOW}${C_BOLD}" "warn" "${C_RESET}" ;;
        skip)        printf '%s%-7s%s' "${C_YELLOW}" "skipped" "${C_RESET}" ;;
        blocked)     printf '%s%-7s%s' "${C_YELLOW}" "blocked" "${C_RESET}" ;;
        interrupted) printf '%s%-7s%s' "${C_RED}" "interrupted" "${C_RESET}" ;;
        *)           printf '%s%-7s%s' "${C_DIM}" "-" "${C_RESET}" ;;
    esac
}

# nci::_stage_line POS TOTAL NAME: "  [ 2/5] build ........... "
nci::_stage_line() {
    local label dots
    label="$(printf '[%*d/%d] %s ' "${#2}" "$1" "$2" "$3")"
    dots="$(printf '%*s' $(( NCI_WIDTH - ${#label} > 2 ? NCI_WIDTH - ${#label} : 2 )) '' | tr ' ' '.')"
    printf '  %s%s%s%s ' "${label}" "${C_DIM}" "${dots}" "${C_RESET}"
}

nci::_json_escape() {
    local s="$1"
    s="${s//\\/\\\\}"; s="${s//\"/\\\"}"; s="${s//$'\n'/\\n}"; s="${s//$'\t'/\\t}"; s="${s//$'\r'/}"
    printf '%s' "${s}"
}

# ---------------------------------------------------------------------------------- Selection ----

nci::_index() {
    local i
    for i in "${!NCI_STAGE_NAMES[@]}"; do
        [[ "${NCI_STAGE_NAMES[i]}" == "$1" ]] && { printf '%s' "${i}"; return 0; }
    done
}

# nci::_expand NAME: the stage NAME, or the stages of the group NAME, space-separated.
nci::_expand() {
    local stage out=""
    for stage in "${NCI_STAGE_NAMES[@]}"; do
        [[ "${stage}" == "$1" || "${stage}" == "$1"/* ]] && out+="${out:+ }${stage}"
    done
    printf '%s' "${out}"
}

# nci::_in_list LIST NAME: true if the space-separated LIST names NAME or its group.
nci::_in_list() {
    local item
    for item in $1; do
        [[ "$2" == "${item}" || "$2" == "${item}"/* ]] && return 0
    done
    return 1
}

# nci::_matches PATTERNS FILE: true if FILE matches one of the patterns.
nci::_matches() {
    local -a patterns
    local pattern
    read -r -a patterns <<< "$1"
    for pattern in "${patterns[@]}"; do
        [[ "${pattern}" == */ ]] && pattern+="*"
        # shellcheck disable=SC2053  # the pattern is a glob on purpose
        [[ "$2" == ${pattern} ]] && return 0
    done
    return 1
}

nci::_default_base() {
    local ref
    for ref in "${NCI_BASE:-}" "${GITHUB_BASE_REF:+origin/${GITHUB_BASE_REF}}" master main origin/HEAD; do
        [[ -n "${ref}" ]] && git -C "${NCI_ROOT}" rev-parse --verify --quiet "${ref}" >/dev/null && {
            printf '%s' "${ref}"; return 0
        }
    done
    return 1
}

# Files changed since the merge base with NCI_SEL_BASE: committed, staged, unstaged and untracked;
# committed only in clean runs.
nci::_changed_files() {
    local merge_base
    merge_base="$(git -C "${NCI_ROOT}" merge-base "${NCI_SEL_BASE}" HEAD)" || nci::_die "no merge base with '${NCI_SEL_BASE}'"
    if (( NCI_OPT_CLEAN )); then
        git -C "${NCI_ROOT}" diff --name-only "${merge_base}" HEAD
    else
        {
            git -C "${NCI_ROOT}" diff --name-only "${merge_base}"
            git -C "${NCI_ROOT}" ls-files --others --exclude-standard
        } | sort -u
    fi
}

# Fills NCI_SELECTED[i] (1/0) and NCI_REASON[i] from the options.
nci::_select() {
    local count=${#NCI_STAGE_NAMES[@]} i name
    NCI_SELECTED=() NCI_REASON=()

    local -a changed=()
    if [[ "${NCI_SEL_MODE}" == changed ]]; then
        mapfile -t changed < <(nci::_changed_files)
        NCI_CHANGED_COUNT=${#changed[@]}
    fi

    for (( i = 0; i < count; i++ )); do
        name="${NCI_STAGE_NAMES[i]}"
        NCI_SELECTED[i]=0
        case "${NCI_SEL_MODE}" in
            all)
                NCI_SELECTED[i]=1 NCI_REASON[i]="" ;;
            only)
                if nci::_in_list "${NCI_SEL_ONLY}" "${name}"; then
                    NCI_SELECTED[i]=1 NCI_REASON[i]="--only"
                else
                    NCI_REASON[i]="not in --only"
                fi ;;
            changed)
                if [[ -z "${NCI_STAGE_PATHS[i]}" ]]; then
                    NCI_SELECTED[i]=1 NCI_REASON[i]="no paths: always"
                else
                    NCI_REASON[i]="no changes"
                    local file hits=0 first=""
                    for file in "${changed[@]}"; do
                        if nci::_matches "${NCI_STAGE_PATHS[i]}" "${file}"; then
                            (( hits++ )) || first="${file}"
                        fi
                    done
                    if (( hits )); then
                        NCI_SELECTED[i]=1
                        NCI_REASON[i]="changed: ${first}"
                        (( hits > 1 )) && NCI_REASON[i]+=" (+$(( hits - 1 )))"
                    fi
                fi ;;
        esac
    done

    # Needs: walk backwards, so that needs of needs are pulled in too (needs are declared earlier).
    local need j
    for (( i = count - 1; i >= 0; i-- )); do
        (( NCI_SELECTED[i] )) || continue
        for need in ${NCI_STAGE_NEEDS[i]}; do
            j="$(nci::_index "${need}")"
            if (( ! NCI_SELECTED[j] )); then
                NCI_SELECTED[j]=1
                NCI_REASON[j]="needed by ${NCI_STAGE_NAMES[i]}"
            fi
        done
    done

    for (( i = 0; i < count; i++ )); do
        if nci::_in_list "${NCI_SEL_SKIP}" "${NCI_STAGE_NAMES[i]}"; then
            NCI_SELECTED[i]=0 NCI_REASON[i]="--skip"
        fi
    done

    if [[ -n "${NCI_SEL_FROM}" ]]; then
        local from first
        first="$(nci::_expand "${NCI_SEL_FROM}")"
        from="$(nci::_index "${first%% *}")"
        for (( i = 0; i < from; i++ )); do
            NCI_SELECTED[i]=0 NCI_REASON[i]="before --from"
        done
    fi
}

nci::_check_names() {
    local name
    for name in "$@"; do
        [[ -n "$(nci::_expand "${name}")" ]] || nci::_die "unknown stage or group '${name}' (see --list)"
    done
}

nci::_list() {
    local i count=${#NCI_STAGE_NAMES[@]} paths
    printf '%s%-3s %-*s %-4s %-34s %s%s\n' "${C_BOLD}" "#" "${NCI_NAME_WIDTH}" "stage" "run" "reason" "paths" "${C_RESET}"
    for (( i = 0; i < count; i++ )); do
        local mark="${C_DIM}no ${C_RESET}"
        (( NCI_SELECTED[i] )) && mark="${C_GREEN}yes${C_RESET}"
        paths="${NCI_STAGE_PATHS[i]:-(always)}"
        (( ${#paths} > 48 )) && paths="${paths:0:47}…"
        printf '%-3s %-*s %s  %-34s %s%s%s\n' "$(( i + 1 ))" "${NCI_NAME_WIDTH}" "${NCI_STAGE_NAMES[i]}" "${mark}" \
            "${NCI_REASON[i]:-}" "${C_DIM}" "${paths}" "${C_RESET}"
    done
}

# ---------------------------------------------------------------------------------- Execution ----

nci::_spinner() {
    local start="$1" line="$2" n=0
    local -a frames=('⠋' '⠙' '⠹' '⠸' '⠼' '⠴' '⠦' '⠧' '⠇' '⠏')
    while :; do
        printf '\r%s%s%s %ss%s' "${line}" "${C_BLUE}" "${frames[n++ % ${#frames[@]}]}" \
            "$(( ${EPOCHSECONDS:-$(date +%s)} - ${start%.*} ))" "${C_RESET}"
        sleep 0.2
    done
}

# The command to rerun a stage, for the failure report.
nci::_rerun_hint() {
    local hint="${NCI_SCRIPT} --only $1"
    (( NCI_OPT_CLEAN )) && hint+=" --clean --skip checkout"
    printf '%s -v' "${hint}"
}

nci::_log_header() {
    local i="$1"
    printf '#> nci stage   %s\n' "${NCI_STAGE_NAMES[i]}"
    printf '#> command     %s\n' "${NCI_STAGE_CMDS[i]% }"
    printf '#> directory   %s\n' "${NCI_ROOT}"
    printf '#> run         %s %s (%s)\n' "${NCI_PROJECT}" "${NCI_RUN_ID}" "${NCI_SELECTION_TEXT}"
    printf '#> commit      %s\n' "$(git -C "${NCI_ROOT}" log -1 --format='%h %s' 2>/dev/null)"
    printf '#> started     %s\n' "$(date '+%Y-%m-%d %H:%M:%S')"
    printf '#> rerun       %s\n' "$(nci::_rerun_hint "${NCI_STAGE_NAMES[i]}")"
    printf '#>\n'
}

# nci::_report LOG FIRST_LINE ERRORS TAIL: the lines of LOG from FIRST_LINE on that match ERRORS
# (when the part is longer than TAIL; none for an empty ERRORS), then its last TAIL lines.
nci::_report() {
    local log="$1" first="$2" errors="$3" tail="$4"
    awk -v first="${first}" -v re="${errors}" -v tail="${tail}" -v max="${NCI_ERROR_LINES:-10}" '
        NR < first || /^#>( |$)/ { next }
        {
            lines[++n] = $0; numbers[n] = NR
            if (re != "" && $0 ~ re) { hits[++h] = n }
        }
        END {
            if (n > tail && h > 0) {
                printf "      errors:\n"
                for (k = 1; k <= h && k <= max; k++)
                    printf "      %6d │ %s\n", numbers[hits[k]], lines[hits[k]]
                if (h > max) printf "             │ … %d more\n", h - max
                printf "      last lines:\n"
            }
            for (k = (n > tail ? n - tail + 1 : 1); k <= n; k++)
                printf "      %6d │ %s\n", numbers[k], lines[k]
        }' "${log}"
}

# nci::_run_stage INDEX POS TOTAL: runs one stage, sets NCI_RESULT[INDEX], NCI_TIME[INDEX].
nci::_run_stage() {
    local i="$1" pos="$2" total="$3"
    local name="${NCI_STAGE_NAMES[i]}"
    local slug="${name//\//_}"
    local log
    log="${NCI_RUN_DIR}/logs/$(printf '%02d' "${pos}")-${slug}.log"
    local stage_dir="${NCI_RUN_DIR}/stages/${slug}"
    mkdir -p "${stage_dir}"
    NCI_LOG[i]="${log}"
    nci::_log_header "${i}" > "${log}"

    local line start end rc spinner=""
    line="$(nci::_stage_line "${pos}" "${total}" "${name}")"
    start="$(nci::_now)"

    if (( NCI_OPT_VERBOSE )); then
        printf '%s%s\n' "${line}" "${C_BLUE}running${C_RESET}"
        [[ "${GITHUB_ACTIONS:-}" == true ]] && printf '::group::%s\n' "${name}"
    elif [[ -t 1 ]]; then
        nci::_spinner "${start}" "${line}" &
        spinner=$!
    else
        printf '%s' "${line}"
    fi

    if (( NCI_OPT_VERBOSE )); then
        NCI_STAGE_NAME="${name}" NCI_STAGE_DIR="${stage_dir}" NCI_STAGE_LOG="${log}" \
            nci::_exec_stage "${i}" 2>&1 | tee -a "${log}"
        rc=${PIPESTATUS[0]}
        [[ "${GITHUB_ACTIONS:-}" == true ]] && printf '::endgroup::\n'
    else
        # shellcheck disable=SC2094  # the stage only counts the lines of its log (nci::step)
        NCI_STAGE_NAME="${name}" NCI_STAGE_DIR="${stage_dir}" NCI_STAGE_LOG="${log}" \
            nci::_exec_stage "${i}" >> "${log}" 2>&1
        rc=$?
    fi

    end="$(nci::_now)"
    if [[ -n "${spinner}" ]]; then
        kill "${spinner}" 2>/dev/null
        wait "${spinner}" 2>/dev/null
        printf '\r\e[K%s' "${line}"
    elif (( NCI_OPT_VERBOSE )); then
        printf '%s' "${line}"
    fi
    NCI_TIME[i]="$(nci::_elapsed "${start}" "${end}")"

    # TAP: counts as a note; `not ok` fails the stage whatever its exit code.
    local tap_pass tap_fail tap_skip
    tap_pass="$(grep -cE '^ok [0-9]+' "${log}")"
    tap_fail="$(grep -cE '^not ok [0-9]+' "${log}")"
    tap_skip="$(grep -ciE '^ok [0-9]+.*# skip' "${log}")"
    if (( tap_pass + tap_fail )); then
        local tap_note="$(( tap_pass - tap_skip ))/$(( tap_pass + tap_fail )) passed"
        (( tap_skip )) && tap_note+=", ${tap_skip} skipped"
        printf '%s\n' "${tap_note}" >> "${stage_dir}/notes"
        if (( tap_fail && rc == 0 )); then
            rc=1
            printf 'not ok lines in the TAP output, but exit code 0\n' >> "${stage_dir}/notes"
        fi
    fi
    printf '#> finished    exit code %d after %ss\n' "${rc}" "${NCI_TIME[i]}" >> "${log}"

    # Attachments: copied into the run.
    local -a attached=()
    if [[ -f "${stage_dir}/attachments" ]]; then
        local file
        mkdir -p "${NCI_RUN_DIR}/attachments/${slug}"
        while IFS= read -r file; do
            [[ -f "${file}" ]] || continue
            cp -- "${file}" "${NCI_RUN_DIR}/attachments/${slug}/"
            attached+=("${NCI_RUN_DIR}/attachments/${slug}/$(basename "${file}")")
        done < "${stage_dir}/attachments"
    fi

    # Steps: the failed one, and the timings for the summary.
    local failed_step="" first_line=1
    NCI_STEPS[i]=""
    if [[ -f "${stage_dir}/steps" ]]; then
        local step_name step_rc step_secs step_line
        while IFS=$'\t' read -r step_name step_rc step_secs step_line; do
            NCI_STEPS[i]+="${NCI_STEPS[i]:+, }{\"name\": \"$(nci::_json_escape "${step_name}")\", \"exit_code\": ${step_rc}, \"seconds\": ${step_secs}}"
            if (( step_rc )) && [[ -z "${failed_step}" ]]; then
                failed_step="${step_name}" first_line="${step_line}"
            fi
        done < "${stage_dir}/steps"
    fi

    if (( NCI_INTERRUPTED )); then
        NCI_RESULT[i]=interrupted
    elif (( rc != 0 )) && nci::_is_soft "${i}"; then
        NCI_RESULT[i]=warn
    elif (( rc != 0 )); then
        NCI_RESULT[i]=fail
    elif [[ -f "${stage_dir}/skipped" ]]; then
        NCI_RESULT[i]=skip
        NCI_NOTES[i]="$(< "${stage_dir}/skipped")"
    else
        NCI_RESULT[i]=ok
    fi
    [[ -f "${stage_dir}/notes" ]] && NCI_NOTES[i]+="${NCI_NOTES[i]:+; }$(paste -sd ';' "${stage_dir}/notes" | sed 's/;/; /g')"

    printf '%s %8s' "$(nci::_status_text "${NCI_RESULT[i]}")" "$(nci::_duration "${NCI_TIME[i]}")"
    [[ -n "${NCI_NOTES[i]:-}" ]] && printf '  %s%s%s' "${C_DIM}" "${NCI_NOTES[i]}" "${C_RESET}"
    printf '\n'

    local result="${NCI_RESULT[i]}"
    [[ "${result}" == fail || "${result}" == warn ]] || return 0

    # A warning is known and allowed: its tail only, no error lines.
    local color="${C_RED}" tail="${NCI_TAIL:-20}" what="failed" errors="${NCI_STAGE_ERRORS[i]:-${NCI_ERRORS}}"
    if [[ "${result}" == warn ]]; then
        color="${C_YELLOW}" tail="${NCI_WARN_TAIL:-5}" what="failed (allowed)" errors=""
    fi
    [[ -n "${failed_step}" ]] && printf '      %sin step %s%s\n' "${color}" "${failed_step}" "${C_RESET}"
    if (( ! NCI_OPT_VERBOSE )); then
        printf '%s' "${C_DIM}"
        nci::_report "${log}" "${first_line}" "${errors}" "${tail}"
        printf '%s' "${C_RESET}"
    fi
    local file
    for file in "${attached[@]}"; do
        printf '      %sattached %s:%s\n%s' "${color}" "${file}" "${C_RESET}" "${C_DIM}"
        nci::_report "${file}" 1 "${errors}" "${NCI_WARN_TAIL:-5}"
        printf '%s' "${C_RESET}"
    done
    printf '      %sexit code %d, log: %s%s\n' "${color}" "${rc}" "${log}" "${C_RESET}"
    [[ "${result}" == fail ]] && printf '      %srerun: %s%s\n' "${color}" "$(nci::_rerun_hint "${name}")" "${C_RESET}"

    if [[ "${GITHUB_ACTIONS:-}" == true ]]; then
        if [[ "${result}" == fail ]] && (( ! NCI_OPT_VERBOSE )); then
            printf '::group::Log of %s\n' "${name}"; cat "${log}"; printf '::endgroup::\n'
        fi
        local level=error
        [[ "${result}" == warn ]] && level=warning
        printf '::%s title=Stage %s %s::exit code %d\n' "${level}" "${name}" "${what}" "${rc}"
    fi
}

# nci::_is_soft INDEX: true if a failure of the stage is a warning.
nci::_is_soft() {
    nci::_in_list "${NCI_SEL_WARN}" "${NCI_STAGE_NAMES[$1]}" && return 0
    (( NCI_STAGE_SOFT[$1] && ! NCI_OPT_STRICT ))
}

# nci::_failed_need INDEX: the first need of the stage that ran and did not succeed, if any.
nci::_failed_need() {
    local need j
    for need in ${NCI_STAGE_NEEDS[$1]}; do
        j="$(nci::_index "${need}")"
        case "${NCI_RESULT[j]:-}" in
            fail|blocked|interrupted) printf '%s' "${need}"; return 0 ;;
        esac
    done
}

# nci::_exec_stage INDEX: the stage itself, in a subshell; NCI_STAGE_* come from the caller.
nci::_exec_stage() (
    set -euo pipefail
    cd "${NCI_ROOT}"
    eval "${NCI_STAGE_CMDS[$1]}"
) < /dev/null

nci::_prune_runs() {
    local keep="${NCI_KEEP_RUNS:-20}"
    local -a runs
    mapfile -t runs < <(find "${NCI_RUNS_DIR}" -mindepth 1 -maxdepth 1 -type d -name '[0-9]*' -printf '%f\n' | sort)
    local excess=$(( ${#runs[@]} - keep ))
    local k
    for (( k = 0; k < excess; k++ )); do
        rm -rf -- "${NCI_RUNS_DIR:?}/${runs[k]}"
    done
}

# ------------------------------------------------------------------------------- Clean runs ----

# Refuses workspaces that could take something valuable with them when wiped.
nci::_check_workspace() {
    NCI_WORKSPACE="$(realpath -m "${NCI_WORKSPACE}")"
    local source
    source="$(realpath "${NCI_SOURCE}")"
    if [[ "${NCI_WORKSPACE}" == / || "${NCI_WORKSPACE}" == "$(realpath -m "${HOME}")" \
          || "${source}" == "${NCI_WORKSPACE}" || "${source}" == "${NCI_WORKSPACE}"/* ]]; then
        nci::_die "refusing workspace '${NCI_WORKSPACE}' (it would wipe the repository or more)"
    fi
    if [[ -d "${NCI_WORKSPACE}" && ! -e "${NCI_WORKSPACE}/.git" && -n "$(ls -A "${NCI_WORKSPACE}")" ]]; then
        nci::_die "'${NCI_WORKSPACE}' exists and is not a workspace (no .git); remove it or set NCI_WORKSPACE"
    fi
}

# The built-in stage of clean runs: a fresh clone of the repository's HEAD in the workspace.
nci::_checkout() {
    local sha
    sha="$(git -C "${NCI_SOURCE}" rev-parse HEAD)"
    cd /
    rm -rf -- "${NCI_WORKSPACE:?}"
    git clone --shared --no-checkout "${NCI_SOURCE}" "${NCI_WORKSPACE}"
    cd "${NCI_WORKSPACE}"
    git checkout --detach "${sha}"

    if [[ -f .gitmodules ]]; then
        local key path name
        while read -r key path; do
            name="${key#submodule.}"
            name="${name%.path}"
            if [[ -e "${NCI_SOURCE}/${path}/.git" ]]; then
                git config "submodule.${name}.url" "${NCI_SOURCE}/${path}"
            fi
        done < <(git config -f .gitmodules --get-regexp '^submodule\..*\.path$')
        git -c protocol.file.allow=always submodule update --init --recursive
    fi
    nci::note "$(git log -1 --format='%h %s' | cut -c1-60)"
}

# ---------------------------------------------------------------------------------- Summary ----

nci::_summary() {
    local total_time="$1" count=${#NCI_STAGE_NAMES[@]} i

    printf '\n%sSummary%s %s, run %s, %s\n' "${C_BOLD}" "${C_RESET}" "${NCI_PROJECT}" "${NCI_RUN_ID}" \
        "$(nci::_duration "${total_time}")"
    for (( i = 0; i < count; i++ )); do
        local result="${NCI_RESULT[i]:-}" time="" notes="${NCI_NOTES[i]:-}"
        [[ -n "${NCI_TIME[i]:-}" ]] && time="$(nci::_duration "${NCI_TIME[i]}")"
        if [[ -z "${result}" ]]; then
            notes="${NCI_REASON[i]:-}"
            (( NCI_SELECTED[i] )) && notes="not reached"
        fi
        printf '  %-*s %s %8s  %s%s%s\n' "${NCI_NAME_WIDTH}" "${NCI_STAGE_NAMES[i]}" "$(nci::_status_text "${result}")" \
            "${time}" "${C_DIM}" "${notes}" "${C_RESET}"
    done
    (( NCI_OPT_CLEAN )) && printf '  %sWorkspace: %s%s\n' "${C_DIM}" "${NCI_WORKSPACE}" "${C_RESET}"
    printf '  %sLogs: %s%s\n' "${C_DIM}" "${NCI_RUN_DIR}/logs" "${C_RESET}"
    [[ -d "${NCI_ARTIFACTS}" && -n "$(ls -A "${NCI_ARTIFACTS}")" ]] && printf '  %sArtifacts: %s%s\n' "${C_DIM}" "${NCI_ARTIFACTS}" "${C_RESET}"

    {
        printf '{\n  "project": "%s",\n  "run_id": "%s",\n  "commit": "%s",\n  "selection": "%s",\n  "root": "%s",\n' \
            "$(nci::_json_escape "${NCI_PROJECT}")" "${NCI_RUN_ID}" \
            "$(git -C "${NCI_ROOT}" rev-parse HEAD 2>/dev/null)" "$(nci::_json_escape "${NCI_SELECTION_TEXT}")" \
            "$(nci::_json_escape "${NCI_ROOT}")"
        printf '  "status": "%s",\n  "seconds": %s,\n  "stages": [\n' "${NCI_STATUS}" "${total_time}"
        for (( i = 0; i < count; i++ )); do
            printf '    {"name": "%s", "status": "%s", "seconds": %s, "log": "%s", "notes": "%s", "steps": [%s]}' \
                "$(nci::_json_escape "${NCI_STAGE_NAMES[i]}")" "${NCI_RESULT[i]:-not-run}" "${NCI_TIME[i]:-null}" \
                "$(nci::_json_escape "${NCI_LOG[i]:-}")" "$(nci::_json_escape "${NCI_NOTES[i]:-${NCI_REASON[i]:-}}")" \
                "${NCI_STEPS[i]:-}"
            (( i < count - 1 )) && printf ','
            printf '\n'
        done
        printf '  ]\n}\n'
    } > "${NCI_RUN_DIR}/summary.json"

    if [[ "${GITHUB_ACTIONS:-}" == true && -n "${GITHUB_STEP_SUMMARY:-}" ]]; then
        {
            printf '### %s pipeline: %s\n\n| Stage | Status | Time | Notes |\n|---|---|---|---|\n' "${NCI_PROJECT}" "${NCI_STATUS}"
            for (( i = 0; i < count; i++ )); do
                printf '| %s | %s | %s | %s |\n' "${NCI_STAGE_NAMES[i]}" "${NCI_RESULT[i]:--}" \
                    "${NCI_TIME[i]:+${NCI_TIME[i]}s}" "${NCI_NOTES[i]:-${NCI_REASON[i]:-}}"
            done
        } >> "${GITHUB_STEP_SUMMARY}"
    fi
}

nci::_usage() {
    cat <<EOF
Usage: ${NCI_SCRIPT} [OPTION]...

Runs the ${NCI_PROJECT} pipeline (nci ${NCI_VERSION}). Stage output goes to log files of the run;
the console shows one line per stage and a summary. A group name (the part before '/') stands for
all of its stages wherever a STAGE is expected.

Selection:
  -l, --list            show the stages and which would run, run nothing
  -o, --only STAGE,...  run only these stages (and the stages they need)
  -s, --skip STAGE,...  do not run these stages
  -f, --from STAGE      start at this stage (resume a failed run)
  -c, --changed         run the stages affected by the changes since the merge base with --base
                        (committed, uncommitted and untracked); stages without paths always run
  -b, --base REF        base of --changed (default: \$NCI_BASE, then master, main, origin/HEAD)

Results:
  -w, --warn STAGE,...  a failure of these stages is a warning, not an error
      --strict          stages declared with --allow-failure fail the pipeline too

Execution:
  -C, --clean           run on a fresh clone of HEAD in \$NCI_WORKSPACE (default: /tmp/workspace/${NCI_PROJECT});
                        with --skip checkout (or --from), in the existing one
  -k, --keep-going      continue after a failed stage
  -v, --verbose         stream the stage output to the console as well
  -h, --help            show this help and exit

Runs go to ${NCI_RUNS_DIR}/<run id> (\$NCI_RUNS_DIR), the last ${NCI_KEEP_RUNS:-20} are kept;
'latest' links to the newest.
EOF
}

# ----------------------------------------------------------------------------------- Main ----

# nci::main "$@": parses the options, runs the selected stages, prints the summary, exits.
nci::main() {
    set +e
    [[ -n "${NCI_PROJECT}" ]] || nci::init
    (( ${#NCI_STAGE_NAMES[@]} )) || nci::_die "no stages declared"
    nci::_colors
    export NCI_ROOT
    NCI_RUNS_DIR="${NCI_RUNS_DIR:-${NCI_ROOT}/build/ci}"
    NCI_SCRIPT="$(realpath --relative-to="${NCI_CALLER_DIR}" "$0")"
    [[ "${NCI_SCRIPT}" == */* ]] || NCI_SCRIPT="./${NCI_SCRIPT}"

    NCI_SEL_MODE=all NCI_SEL_ONLY="" NCI_SEL_SKIP="" NCI_SEL_FROM="" NCI_SEL_BASE=""
    NCI_SEL_WARN="" NCI_OPT_STRICT=0 NCI_OPT_CLEAN=0
    NCI_OPT_LIST=0 NCI_OPT_KEEP_GOING=0 NCI_OPT_VERBOSE="${NCI_VERBOSE:-0}"
    while (( $# )); do
        case "$1" in
            -l|--list)       NCI_OPT_LIST=1; shift ;;
            -o|--only)       NCI_SEL_MODE=only; NCI_SEL_ONLY="${2//,/ }"; shift 2 ;;
            -s|--skip)       NCI_SEL_SKIP="${2//,/ }"; shift 2 ;;
            -f|--from)       NCI_SEL_FROM="$2"; shift 2 ;;
            -c|--changed)    NCI_SEL_MODE=changed; shift ;;
            -b|--base)       NCI_SEL_BASE="$2"; shift 2 ;;
            -w|--warn)       NCI_SEL_WARN="${2//,/ }"; shift 2 ;;
            --strict)        NCI_OPT_STRICT=1; shift ;;
            -C|--clean)      NCI_OPT_CLEAN=1; shift ;;
            -k|--keep-going) NCI_OPT_KEEP_GOING=1; shift ;;
            -v|--verbose)    NCI_OPT_VERBOSE=1; shift ;;
            -h|--help)       nci::_usage; exit 0 ;;
            *)               nci::_die "unknown option '$1' (see --help)" ;;
        esac
    done

    NCI_SOURCE="${NCI_ROOT}"
    export NCI_SOURCE
    if (( NCI_OPT_CLEAN )); then
        NCI_WORKSPACE="${NCI_WORKSPACE:-/tmp/workspace/${NCI_PROJECT}}"
        nci::_check_workspace
        export NCI_WORKSPACE
        local k
        for k in "${!NCI_STAGE_NAMES[@]}"; do
            NCI_STAGE_NEEDS[k]="checkout${NCI_STAGE_NEEDS[k]:+ ${NCI_STAGE_NEEDS[k]}}"
        done
        NCI_STAGE_NAMES=(checkout "${NCI_STAGE_NAMES[@]}")
        NCI_STAGE_CMDS=(nci::_checkout "${NCI_STAGE_CMDS[@]}")
        NCI_STAGE_PATHS=("" "${NCI_STAGE_PATHS[@]}")
        NCI_STAGE_NEEDS=("" "${NCI_STAGE_NEEDS[@]}")
        NCI_STAGE_SOFT=(0 "${NCI_STAGE_SOFT[@]}")
        NCI_STAGE_ERRORS=("" "${NCI_STAGE_ERRORS[@]}")
    fi

    # shellcheck disable=SC2086  # word splitting on purpose
    nci::_check_names ${NCI_SEL_ONLY} ${NCI_SEL_SKIP} ${NCI_SEL_FROM} ${NCI_SEL_WARN}
    if [[ "${NCI_SEL_MODE}" == changed ]]; then
        [[ -n "${NCI_SEL_BASE}" ]] || NCI_SEL_BASE="$(nci::_default_base)" || nci::_die "no base for --changed, use --base"
    fi

    NCI_NAME_WIDTH=5
    local name
    for name in "${NCI_STAGE_NAMES[@]}"; do
        (( ${#name} > NCI_NAME_WIDTH )) && NCI_NAME_WIDTH=${#name}
    done
    NCI_WIDTH=$(( NCI_NAME_WIDTH + 16 ))

    NCI_CHANGED_COUNT=0
    nci::_select
    case "${NCI_SEL_MODE}" in
        all)     NCI_SELECTION_TEXT="all stages" ;;
        only)    NCI_SELECTION_TEXT="only: ${NCI_SEL_ONLY}" ;;
        changed) NCI_SELECTION_TEXT="changed since ${NCI_SEL_BASE} (${NCI_CHANGED_COUNT} files)" ;;
    esac
    [[ -n "${NCI_SEL_SKIP}" ]] && NCI_SELECTION_TEXT+=", skip: ${NCI_SEL_SKIP}"
    [[ -n "${NCI_SEL_FROM}" ]] && NCI_SELECTION_TEXT+=", from: ${NCI_SEL_FROM}"
    [[ -n "${NCI_SEL_WARN}" ]] && NCI_SELECTION_TEXT+=", warn: ${NCI_SEL_WARN}"
    (( NCI_OPT_STRICT )) && NCI_SELECTION_TEXT+=", strict"
    (( NCI_OPT_CLEAN )) && NCI_SELECTION_TEXT+=", clean"

    if (( NCI_OPT_LIST )); then
        printf '%s%s%s pipeline, %s\n\n' "${C_BOLD}" "${NCI_PROJECT}" "${C_RESET}" "${NCI_SELECTION_TEXT}"
        nci::_list
        exit 0
    fi

    NCI_RUN_ID="$(date +%Y%m%d-%H%M%S)"
    NCI_RUN_DIR="${NCI_RUNS_DIR}/${NCI_RUN_ID}"
    NCI_ARTIFACTS="${NCI_RUN_DIR}/artifacts"
    mkdir -p "${NCI_RUN_DIR}/logs" "${NCI_RUN_DIR}/stages" "${NCI_ARTIFACTS}" || nci::_die "cannot create ${NCI_RUN_DIR}"
    ln -sfn "${NCI_RUN_ID}" "${NCI_RUNS_DIR}/latest"
    export NCI_RUN_ID NCI_RUN_DIR NCI_ARTIFACTS

    if (( NCI_OPT_CLEAN )); then
        if (( NCI_SELECTED[0] )); then
            mkdir -p "${NCI_WORKSPACE}" || nci::_die "cannot create ${NCI_WORKSPACE}"
        elif [[ ! -e "${NCI_WORKSPACE}/.git" ]]; then
            nci::_die "no workspace to resume in at ${NCI_WORKSPACE}: run with checkout first"
        fi
        NCI_ROOT="${NCI_WORKSPACE}"
    fi

    NCI_RESULT=() NCI_TIME=() NCI_NOTES=() NCI_LOG=() NCI_STEPS=()
    NCI_INTERRUPTED=0
    trap 'NCI_INTERRUPTED=1' INT TERM

    local -a order=()
    local i
    for i in "${!NCI_STAGE_NAMES[@]}"; do
        (( NCI_SELECTED[i] )) && order+=("${i}")
    done

    printf '%s%s%s pipeline, %s, %d of %d stages, run %s\n' "${C_BOLD}" "${NCI_PROJECT}" "${C_RESET}" \
        "${NCI_SELECTION_TEXT}" "${#order[@]}" "${#NCI_STAGE_NAMES[@]}" "${NCI_RUN_ID}"
    if (( NCI_OPT_CLEAN )); then
        local dirty="" changes
        changes="$(git -C "${NCI_SOURCE}" status --porcelain | wc -l)"
        (( changes )) && dirty=", ${changes} uncommitted/untracked file(s) not included"
        printf '%sworkspace %s, HEAD %s%s%s\n' "${C_DIM}" "${NCI_WORKSPACE}" \
            "$(git -C "${NCI_SOURCE}" rev-parse --short HEAD)" "${dirty}" "${C_RESET}"
    fi

    local start pos=0 failed=0
    start="$(nci::_now)"
    for i in "${order[@]}"; do
        (( pos++ ))
        local blocker
        blocker="$(nci::_failed_need "${i}")"
        if [[ -n "${blocker}" ]]; then
            NCI_RESULT[i]=blocked NCI_NOTES[i]="${blocker} failed"
            printf '%s%s %8s  %s%s%s\n' "$(nci::_stage_line "${pos}" "${#order[@]}" "${NCI_STAGE_NAMES[i]}")" \
                "$(nci::_status_text blocked)" "" "${C_DIM}" "${NCI_NOTES[i]}" "${C_RESET}"
            continue
        fi
        nci::_run_stage "${i}" "${pos}" "${#order[@]}"
        (( NCI_INTERRUPTED )) && break
        if [[ "${NCI_RESULT[i]}" == fail ]]; then
            failed=1
            (( NCI_OPT_KEEP_GOING )) || break
        fi
    done

    NCI_WARNINGS=0
    for i in "${order[@]}"; do
        [[ "${NCI_RESULT[i]:-}" == warn ]] && (( NCI_WARNINGS++ ))
    done
    NCI_STATUS=passed
    (( NCI_WARNINGS )) && NCI_STATUS=warnings
    (( failed )) && NCI_STATUS=failed
    (( NCI_INTERRUPTED )) && NCI_STATUS=interrupted

    nci::_summary "$(nci::_elapsed "${start}" "$(nci::_now)")"
    nci::_prune_runs

    case "${NCI_STATUS}" in
        passed)      printf '\n%sPipeline passed%s\n' "${C_GREEN}${C_BOLD}" "${C_RESET}"; exit 0 ;;
        warnings)    printf '\n%sPipeline passed with %d warning(s)%s\n' "${C_YELLOW}${C_BOLD}" "${NCI_WARNINGS}" "${C_RESET}"; exit 0 ;;
        failed)      printf '\n%sPipeline failed%s\n' "${C_RED}${C_BOLD}" "${C_RESET}"; exit 1 ;;
        interrupted) printf '\n%sPipeline interrupted%s\n' "${C_RED}${C_BOLD}" "${C_RESET}"; exit 130 ;;
    esac
}
