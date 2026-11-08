# shellcheck shell=bash
#
# Part of Nexus Library
#
# nci-test: a TAP library for shell test scripts, run by nci or by hand.
#
#     source "<nexus>/tools/ci/nci-test.sh"
#     greets() { tap::run "${TOOL}" hello "$1"; tap::expect_stdout "Hello, $1"; }
#     tap::case "greets the world" greets world
#     tap::done
#
# Documentation: the test library chapter of the nci book, `doc/nci`.

[[ -n "${TAP_VERSION:-}" ]] && return 0
readonly TAP_VERSION="0.1.0"

TAP_COUNT=0
TAP_FAILED=0
TAP_SKIPPED=0
TAP_ROOT="$(git -C "$(dirname "$(realpath "$0")")" rev-parse --show-toplevel 2>/dev/null || pwd)"

if [[ -z "${TAP_DIR:-}" ]]; then
    if [[ -n "${NCI_STAGE_DIR:-}" ]]; then
        TAP_DIR="${NCI_STAGE_DIR}/cases"
    else
        TAP_DIR="${TAP_ROOT}/build/test-out/$(basename "${0%.*}")"
    fi
    rm -rf -- "${TAP_DIR:?}"
fi
mkdir -p "${TAP_DIR}"

# ------------------------------------------------------------------------------------ Cases ----

# tap::case NAME FUNCTION [ARG...]: runs one test case.
tap::case() {
    local name="$1"; shift
    TAP_COUNT=$(( TAP_COUNT + 1 ))
    local slug
    slug="$(printf '%02d-%s' "${TAP_COUNT}" "${name}" | tr -c '[:alnum:]._-' '-')"
    TAP_CASE_DIR="${TAP_DIR}/${slug}"
    mkdir -p "${TAP_CASE_DIR}"
    local log="${TAP_CASE_DIR}/case.log"

    local errexit=0 rc
    [[ $- == *e* ]] && errexit=1
    set +e
    ( set -euo pipefail; "$@" ) > "${log}" 2>&1
    rc=$?
    (( errexit )) && set -e

    if (( rc == 0 )); then
        printf 'ok %d - %s\n' "${TAP_COUNT}" "${name}"
    else
        TAP_FAILED=$(( TAP_FAILED + 1 ))
        printf 'not ok %d - %s\n' "${TAP_COUNT}" "${name}"
        tail -n "${TAP_DIAG_LINES:-15}" "${log}" | sed 's/^/#   /'
        printf '#   case log: %s\n' "${log}"
    fi
    return 0
}

# tap::skip NAME REASON: a case that is not run.
tap::skip() {
    TAP_COUNT=$(( TAP_COUNT + 1 ))
    TAP_SKIPPED=$(( TAP_SKIPPED + 1 ))
    printf 'ok %d - %s # SKIP %s\n' "${TAP_COUNT}" "$1" "${2:-}"
}

# tap::require FILE [HINT]: stops the script (TAP "Bail out!") unless FILE is an executable.
tap::require() {
    [[ -x "$1" ]] && return 0
    printf 'Bail out! %s not found or not executable%s\n' "$1" "${2:+ (${2})}"
    exit 1
}

# tap::done: the plan and a summary; exits 1 if a case failed.
tap::done() {
    printf '1..%d\n' "${TAP_COUNT}"
    printf '# %d passed, %d failed, %d skipped\n' \
        "$(( TAP_COUNT - TAP_FAILED - TAP_SKIPPED ))" "${TAP_FAILED}" "${TAP_SKIPPED}"
    (( TAP_FAILED == 0 )) || exit 1
    exit 0
}

# ------------------------------------------------------------------------------- Inside a case ----

# tap::run COMMAND [ARG...]: runs the command; its stdout, stderr and both go to the files
# TAP_STDOUT, TAP_STDERR and TAP_OUTPUT, its exit status to TAP_STATUS. Never fails.
tap::run() {
    TAP_STDOUT="${TAP_CASE_DIR}/stdout"
    TAP_STDERR="${TAP_CASE_DIR}/stderr"
    TAP_OUTPUT="${TAP_CASE_DIR}/output"
    printf '$ %s\n' "$*"
    local errexit=0
    [[ $- == *e* ]] && errexit=1
    set +e
    "$@" > "${TAP_STDOUT}" 2> "${TAP_STDERR}" < /dev/null
    TAP_STATUS=$?
    (( errexit )) && set -e
    cat "${TAP_STDOUT}" "${TAP_STDERR}" > "${TAP_OUTPUT}"
    printf '  exit status %d, stdout %d bytes, stderr %d lines\n' "${TAP_STATUS}" \
        "$(wc -c < "${TAP_STDOUT}")" "$(wc -l < "${TAP_STDERR}")"
    return 0
}

tap::_fail() {
    printf 'FAIL: %s\n' "$1" >&2
    shift
    (( $# )) && printf '%s\n' "$@" | sed 's/^/      /' >&2
    return 1
}

tap::_stderr_tail() {
    [[ -s "${TAP_STDERR:-}" ]] || return 0
    printf '  stderr (last lines):\n'
    tail -n 5 "${TAP_STDERR}" | sed 's/^/    /'
}

# tap::expect_status [CODE]: the last tap::run exited with CODE (default 0).
tap::expect_status() {
    local want="${1:-0}"
    (( TAP_STATUS == want )) && return 0
    tap::_fail "exit status ${TAP_STATUS}, expected ${want}" "$(tap::_stderr_tail)"
}

# tap::expect_fail: the last tap::run exited with a non-zero status.
tap::expect_fail() {
    (( TAP_STATUS != 0 )) && return 0
    tap::_fail "exit status 0, expected a failure"
}

# tap::expect_eq ACTUAL EXPECTED [WHAT]
tap::expect_eq() {
    [[ "$1" == "$2" ]] && return 0
    tap::_fail "${3:-value} differs" "expected: $2" "actual:   $1"
}

# tap::expect_stdout TEXT: the stdout of the last tap::run is TEXT (trailing whitespace of the lines
# and trailing newlines ignored).
tap::expect_stdout() {
    local actual expected
    actual="$(sed 's/[[:space:]]*$//' "${TAP_STDOUT}")"
    expected="$(printf '%s\n' "$1" | sed 's/[[:space:]]*$//')"
    tap::expect_eq "${actual}" "${expected}" "stdout"
}

# tap::expect_hex FILE HEX: the bytes of FILE, as lowercase hex without separators.
tap::expect_hex() {
    local actual
    actual="$(od -An -v -tx1 "$1" | tr -d ' \n')"
    tap::expect_eq "${actual}" "${2,,}" "content of $(basename "$1")"
}

# tap::expect_contains FILE TEXT...: FILE contains every TEXT (fixed strings).
tap::expect_contains() {
    local file="$1" text
    shift
    local -a missing=()
    for text in "$@"; do
        grep -qF -- "${text}" "${file}" || missing+=("${text}")
    done
    (( ${#missing[@]} == 0 )) && return 0
    tap::_fail "$(basename "${file}") misses ${#missing[@]} of $# expected text(s)" "${missing[@]/#/missing: }"
}

# tap::expect_match FILE REGEX: a line of FILE matches the extended regex.
tap::expect_match() {
    grep -qE -- "$2" "$1" && return 0
    tap::_fail "no line of $(basename "$1") matches '$2'" "$(tail -n 5 "$1")"
}

# tap::write_hex FILE HEX: writes the bytes given as hex (no separators).
tap::write_hex() {
    [[ "$2" =~ ^([0-9a-fA-F]{2})*$ ]] || { tap::_fail "not hex: '$2'"; return 1; }
    # shellcheck disable=SC2001,SC2059  # the format is the bytes, built from validated hex
    printf "$(sed 's/../\\x&/g' <<< "$2")" > "$1"
}
