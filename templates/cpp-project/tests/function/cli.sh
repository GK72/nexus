#!/usr/bin/env bash
#
# Function tests of the command line.

set -euo pipefail
# shellcheck source=../common.sh
source "$(dirname "$(realpath "$0")")/../common.sh"

greets_by_name() {
    tap::run "${@NAME@_BIN}" Nexus
    tap::expect_status 0
    tap::expect_stdout "Hello, Nexus!"
}

greets_the_world() {
    tap::run "${@NAME@_BIN}"
    tap::expect_status 0
    tap::expect_stdout "Hello, world!"
}

version() {
    tap::run "${@NAME@_BIN}" --version
    tap::expect_status 0
    tap::expect_match "${TAP_STDOUT}" "^@name@ $(cat "${PROJECT_DIR}/version.txt")\+"
}

unknown_option() {
    tap::run "${@NAME@_BIN}" --frobnicate
    tap::expect_status 1
    tap::expect_match "${TAP_STDERR}" "unrecognised option '--frobnicate'"
    tap::expect_match "${TAP_STDERR}" '^Usage: @name@'
}

two_names() {
    tap::run "${@NAME@_BIN}" Nexus Norn
    tap::expect_status 1
    tap::expect_match "${TAP_STDERR}" 'too many positional options'
}

tap::case "greets by name"      greets_by_name
tap::case "greets the world"    greets_the_world
tap::case "version"             version
tap::case "unknown option"      unknown_option
tap::case "two names"           two_names

tap::done
