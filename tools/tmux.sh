#!/usr/bin/env bash
#
# Launch a tmux session with the standard window layout, rooted at a directory.
#
# Usage: tmux.sh [--no-sandbox] [path]   (default: the Nexus repository)
#
# Windows: 1 nvim, 2 shell (with Heimdall), 3 ai (claude), 9 btop
#
# Every window starts with the default shell and the application is typed into
# it via send-keys, so exiting the application drops back to the shell instead
# of closing the window.
#
# By default the session lives on a separate tmux server that runs inside
# tools/sandbox.sh, so every window inherits the sandbox. The server's socket
# is in $XDG_RUNTIME_DIR/nexus-sandbox, reachable from both sides: clients
# outside can attach, processes inside reach only this sandboxed server.
# --no-sandbox uses the default tmux server instead.

set -euo pipefail

NEXUS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

SANDBOX=1
if [[ "${1:-}" == "--no-sandbox" ]]; then
    SANDBOX=0
    shift
fi

WORK_DIR="$(realpath "${1:-${NEXUS_DIR}}")"

if [[ ! -d "${WORK_DIR}" ]]; then
    echo "Not a directory: ${WORK_DIR}" >&2
    exit 1
fi

# tmux session names cannot contain '.' or ':'
SESSION="$(basename "${WORK_DIR}" | tr '.:' '__')"

SOCKET_DIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}/nexus-sandbox"
SOCKET="${SOCKET_DIR}/tmux"

TMUX_CMD=(tmux)
if [[ "${SANDBOX}" == 1 ]]; then
    TMUX_CMD=(tmux -S "${SOCKET}")
fi

tmx() {
    "${TMUX_CMD[@]}" "$@"
}

server_running() {
    tmx show-options -s exit-empty > /dev/null 2>&1
}

# Starts the tmux server in the foreground of a detached sandbox and waits
# until it answers.
start_sandbox_server() {
    mkdir -p "${SOCKET_DIR}"
    chmod 700 "${SOCKET_DIR}"
    rm -f "${SOCKET}"

    NEXUS_SANDBOX_SHARE="${SOCKET_DIR}" setsid -f \
        "${NEXUS_DIR}/tools/sandbox.sh" tmux -S "${SOCKET}" -D \
        < /dev/null > /dev/null 2>&1

    for _ in {1..50}; do
        if server_running; then
            return 0
        fi
        sleep 0.1
    done

    echo "The sandboxed tmux server did not start; try: tools/sandbox.sh true" >&2
    exit 1
}

attach() {
    if [[ -z "${TMUX:-}" ]]; then
        exec "${TMUX_CMD[@]}" attach-session -t "=${SESSION}"
    elif [[ "${SANDBOX}" == 0 ]]; then
        tmux switch-client -t "=${SESSION}"
    else
        # switch-client cannot cross servers: the current client detaches and
        # its terminal attaches to the sandboxed server instead.
        tmux detach-client -E "tmux -S '${SOCKET}' attach-session -t '=${SESSION}'"
    fi
}

if [[ "${SANDBOX}" == 1 ]] && ! server_running; then
    start_sandbox_server
fi

if tmx has-session -t "=${SESSION}" 2> /dev/null; then
    attach
    exit 0
fi

# new_window <index> <name> [command]
new_window() {
    local index="$1" name="$2" command="${3:-}"
    local target="${SESSION}:${index}"

    if [[ "${index}" == 1 ]]; then
        tmx new-session -d -s "${SESSION}" -n "${name}" -c "${WORK_DIR}"
    else
        tmx new-window -d -t "${target}" -n "${name}" -c "${WORK_DIR}"
    fi

    if [[ -n "${command}" ]]; then
        tmx send-keys -t "${target}" "${command}" Enter
    fi
}

new_window 1 nvim  nvim
new_window 2 shell "heimdall -f"
new_window 3 ai    claude
new_window 9 btop  btop

# -D turns exit-empty off; turn it back on so the server, and with it the
# sandbox, ends with the last session.
if [[ "${SANDBOX}" == 1 ]]; then
    tmx set-option -s exit-empty on
fi

tmx select-window -t "${SESSION}:1"
attach
