#!/usr/bin/env bash
#
# Launch a tmux session with the standard window layout, rooted at a directory.
#
# Usage: tmux.sh [path]   (default: the Nexus repository)
#
# Windows: 1 nvim, 2 shell (with Heimdall), 3 ai (claude), 9 btop
#
# Every window starts with the default shell and the application is typed into
# it via send-keys, so exiting the application drops back to the shell instead
# of closing the window.

set -euo pipefail

NEXUS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORK_DIR="$(realpath "${1:-${NEXUS_DIR}}")"

if [[ ! -d "${WORK_DIR}" ]]; then
    echo "Not a directory: ${WORK_DIR}" >&2
    exit 1
fi

# tmux session names cannot contain '.' or ':'
SESSION="$(basename "${WORK_DIR}" | tr '.:' '__')"

attach() {
    if [[ -n "${TMUX:-}" ]]; then
        tmux switch-client -t "${SESSION}"
    else
        tmux attach-session -t "${SESSION}"
    fi
}

if tmux has-session -t "=${SESSION}" 2> /dev/null; then
    attach
    exit 0
fi

# new_window <index> <name> [command]
new_window() {
    local index="$1" name="$2" command="${3:-}"
    local target="${SESSION}:${index}"

    if [[ "${index}" == 1 ]]; then
        tmux new-session -d -s "${SESSION}" -n "${name}" -c "${WORK_DIR}"
    else
        tmux new-window -d -t "${target}" -n "${name}" -c "${WORK_DIR}"
    fi

    if [[ -n "${command}" ]]; then
        tmux send-keys -t "${target}" "${command}" Enter
    fi
}

new_window 1 nvim  nvim
new_window 2 shell "heimdall -f"
new_window 3 ai    claude
new_window 9 btop  btop

tmux select-window -t "${SESSION}:1"
attach
