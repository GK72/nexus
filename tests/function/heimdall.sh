#!/usr/bin/env bash
#
# Part of Nexus Library
#
# Function tests of heimdall on a fixture: a bare remote and a clone with every state the git
# report shows. Hermetic: no global git config, no network, its own configuration.
#
#   BUILD=build/release tests/function/heimdall.sh

# shellcheck disable=SC2329  # the cases are called through tap::case
set -euo pipefail
# shellcheck source=../../tools/ci/nci-test.sh
source "$(dirname "$(realpath "$0")")/../../tools/ci/nci-test.sh"

BUILD="$(realpath -m "${BUILD:-${TAP_ROOT}/build/debug}")"
HEIMDALL="${HEIMDALL:-${BUILD}/heimdall/heimdall}"
tap::require "${HEIMDALL}" "build first: ci/pipeline.sh --only build"

export GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_NOSYSTEM=1
export XDG_CONFIG_HOME="${TAP_DIR}/config" HOME="${TAP_DIR}/home"
export GIT_AUTHOR_NAME=test GIT_AUTHOR_EMAIL=test@example.com
export GIT_COMMITTER_NAME=test GIT_COMMITTER_EMAIL=test@example.com
mkdir -p "${HOME}"

commit() {  # commit REPO FILE [TEXT]
    printf '%s\n' "${3:-$2}" >> "$1/$2"
    git -C "$1" add "$2"
    git -C "$1" commit -qm "$2"
}

configure() {  # configure TOML-LINE...: the configuration file, one key per argument
    mkdir -p "${XDG_CONFIG_HOME}/nexus"
    printf '%s\n' "$@" > "${XDG_CONFIG_HOME}/nexus/heimdall.toml"
}

# repos/
#   app       clone of remote.git: main (dirty), feature (2 unpushed), gone (upstream deleted, so
#             its commit is unpushed), 1 stash, a detached worktree and a deleted (prunable) one
#   solo      no remote, clean
make_fixture() {
    local root="${TAP_DIR}/fixture"
    rm -rf "${root}"
    mkdir -p "${root}/repos"
    git init -q --bare -b main "${root}/remote.git"
    git clone -q "${root}/remote.git" "${root}/repos/app" 2>/dev/null
    local app="${root}/repos/app"
    commit "${app}" README
    git -C "${app}" push -q -u origin main

    git -C "${app}" switch -qc gone
    commit "${app}" gone
    git -C "${app}" push -q -u origin gone
    git -C "${app}" push -q origin --delete gone

    git -C "${app}" switch -qc feature main
    commit "${app}" f1
    commit "${app}" f2

    git -C "${app}" switch -q main
    commit "${app}" stashed
    printf 'change\n' >> "${app}/stashed"
    git -C "${app}" stash -q

    git -C "${app}" worktree add -q --detach "${root}/wt-detached" main
    git -C "${app}" worktree add -q -b scratch "${root}/wt-missing" main
    rm -rf "${root}/wt-missing"

    printf 'staged\n' > "${app}/staged"
    git -C "${app}" add staged
    printf 'more\n' >> "${app}/README"
    printf 'new\n' > "${app}/untracked"

    git init -q -b main "${root}/repos/solo"
    commit "${root}/repos/solo" README
    printf '%s\n' "${root}"
}

FIXTURE="$(make_fixture)"

report() {
    configure "roots = [\"${FIXTURE}/repos\"]"
    tap::run "${HEIMDALL}" git --color never
    tap::expect_status 0
    tap::expect_contains "${TAP_STDOUT}" '2 repos · 1 need attention' '▌app' '⌂ main' '+1 ~1 ?1 ↑1' \
        'detached @' '• feature' '↑2' '• gone' 'upstream gone' 'stash@{0}'
    tap::expect_eq "$(grep -c -e solo -e wt-missing "${TAP_STDOUT}" || true)" 0 "clean and prunable left out"
}

brief() {
    configure "roots = [\"${FIXTURE}/repos\"]"
    tap::run "${HEIMDALL}" git -b --color never
    tap::expect_status 0
    tap::expect_contains "${TAP_STDOUT}" 'Heimdall git brief:' 'app' 'main' '+1 ~1 ?1 ↑4 ≡1' '+1 worktree' \
        '1 upstream gone'
}

check() {
    configure "roots = [\"${FIXTURE}/repos\"]"
    tap::run "${HEIMDALL}" git --check
    tap::expect_status 1
    configure "roots = []" "repositories = [\"${FIXTURE}/repos/solo\"]"
    tap::run "${HEIMDALL}" git --check
    tap::expect_status 0
}

conflict() {
    local repo="${TAP_CASE_DIR}/conflict"
    git init -q -b main "${repo}"
    commit "${repo}" file base
    git -C "${repo}" switch -qc other
    commit "${repo}" file theirs
    git -C "${repo}" switch -q main
    commit "${repo}" file ours
    git -C "${repo}" merge -q other >/dev/null 2>&1 || true
    configure "roots = []"
    tap::run "${HEIMDALL}" git --color never "${repo}"
    tap::expect_contains "${TAP_STDOUT}" '!1'
}

fetches() {
    local other="${TAP_CASE_DIR}/other"
    git clone -q "${FIXTURE}/remote.git" "${other}"
    commit "${other}" upstream-change
    git -C "${other}" push -q origin main
    configure "roots = [\"${FIXTURE}/repos\"]"
    tap::run "${HEIMDALL}" git -b -f --color never
    tap::expect_status 0
    tap::expect_contains "${TAP_STDOUT}" 'fetched just now' '↓1'
}

fetch_failure() {
    local repo="${TAP_CASE_DIR}/broken"
    git init -q -b main "${repo}"
    commit "${repo}" README
    git -C "${repo}" remote add origin "${TAP_CASE_DIR}/missing.git"
    configure "roots = []" "repositories = [\"${repo}\"]"
    tap::run "${HEIMDALL}" git -f --color never
    tap::expect_contains "${TAP_STDOUT}" 'fetch failed' '1 failed'
}

config() {
    configure "roots = [\"${FIXTURE}/repos\"]" 'exclude-from-roots = ["ap*"]'
    tap::run "${HEIMDALL}" git -b --color never
    tap::expect_contains "${TAP_STDOUT}" '1 repo · all clear'
    configure "roots = [\"${FIXTURE}/repos\"]"
    tap::run "${HEIMDALL}" git -b --color never --exclude-from-roots 'ap*'
    tap::expect_contains "${TAP_STDOUT}" '1 repo · all clear'
    configure 'roots = "not a list"'
    tap::run "${HEIMDALL}" git
    tap::expect_status 2
}

usage() {
    tap::run "${HEIMDALL}" --brief
    tap::expect_status 2
    tap::run "${HEIMDALL}" nonsense
    tap::expect_status 2
    tap::run "${HEIMDALL}" --help
    tap::expect_status 0
    tap::expect_contains "${TAP_STDOUT}" 'usage: heimdall'
}

dashboard() {
    configure "roots = [\"${FIXTURE}/repos\"]"
    tap::run "${HEIMDALL}" --color never --width 80
    tap::expect_status 0
    tap::expect_contains "${TAP_STDOUT}" 'Dashboard' 'weather: not yet available' '── system' 'Disks: /' \
        '── repos' '2 repos · 1 need attention' 'app' ' ms'
}

tap::case "the git report"                          report
tap::case "the brief report"                        brief
tap::case "--check"                                 check
tap::case "conflicts"                               conflict
tap::case "fetches (-f)"                            fetches
tap::case "a failed fetch"                          fetch_failure
tap::case "roots and exclude-from-roots"            config
tap::case "usage errors"                            usage
tap::case "the dashboard"                           dashboard

tap::done
