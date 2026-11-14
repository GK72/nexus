#!/usr/bin/env bash
#
# Run a command inside a bubblewrap sandbox that keeps accidents (our own or a
# script's) away from everything outside the work areas.
#
# Usage: sandbox.sh [command [args...]]   (default: $SHELL)
#
# Nothing is visible unless listed here: the sandbox starts from an empty root
# and an empty, read-only $HOME.
#
# System:    /usr, /etc, /opt, /sys read-only; private /tmp, /var/tmp and /run.
#            The real /run and /tmp hold the sockets of processes outside the
#            sandbox (D-Bus, tmux, X11, Wayland, keyring), and talking to them
#            would act outside it.
# Read-only: the paths in NEXUS_SANDBOX_RO
# Writable:  the paths in NEXUS_SANDBOX_RW
#
# Both are colon-separated lists of absolute paths, kept outside the repository
# (e.g. in ~/.cenv) because they describe one machine. A listed path that is a
# symlink is recreated as the same symlink, so its
# target must be visible too (most dotfiles point into ~/repos/nexus).
# Paths that do not exist are skipped.
#
# The network is shared. Inside, NEXUS_SANDBOX=1 is set.
#
# NEXUS_SANDBOX_SHARE: a host directory to bind into the sandbox read-write at
# the same path, even under the private /run (tmux.sh uses it for its socket).

set -euo pipefail

if [[ -n "${NEXUS_SANDBOX:-}" ]]; then
    echo "Already inside the sandbox" >&2
    exit 1
fi

RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"

if [[ -z "${NEXUS_SANDBOX_RO:-}" && -z "${NEXUS_SANDBOX_RW:-}" ]]; then
    echo "Set NEXUS_SANDBOX_RO and NEXUS_SANDBOX_RW (e.g. in ~/.cenv)" >&2
    exit 1
fi

IFS=: read -ra READ_ONLY <<< "${NEXUS_SANDBOX_RO:-}"
IFS=: read -ra WRITABLE <<< "${NEXUS_SANDBOX_RW:-}"

args=(
    --ro-bind /usr /usr
    --symlink usr/bin /bin
    --symlink usr/bin /sbin
    --symlink usr/lib /lib
    --symlink usr/lib /lib64
    --ro-bind /etc /etc
    --ro-bind-try /opt /opt
    --ro-bind /sys /sys
    --dev /dev
    --proc /proc
    --tmpfs /tmp
    --tmpfs /var/tmp
    --tmpfs /run
    --perms 0700 --dir "${RUNTIME_DIR}"
    --tmpfs "${HOME}"
    --unshare-pid
    --unshare-ipc
    --die-with-parent
    --setenv NEXUS_SANDBOX 1
    --setenv XDG_RUNTIME_DIR "${RUNTIME_DIR}"
    --unsetenv DISPLAY
    --unsetenv WAYLAND_DISPLAY
    --unsetenv XAUTHORITY
    --unsetenv DBUS_SESSION_BUS_ADDRESS
    --unsetenv SSH_AUTH_SOCK
    --unsetenv TMUX
    --unsetenv TMUX_PANE
)

# add_mount <bind option> <path>
add_mount() {
    local option="$1" path="$2"

    if [[ -L "${path}" ]]; then
        args+=(--symlink "$(readlink "${path}")" "${path}")
    elif [[ -e "${path}" ]]; then
        args+=("${option}" "${path}" "${path}")
    fi
}

for path in "${READ_ONLY[@]}"; do
    add_mount --ro-bind "${path}"
done

for path in "${WRITABLE[@]}"; do
    add_mount --bind "${path}"
done

if [[ -n "${NEXUS_SANDBOX_SHARE:-}" ]]; then
    args+=(--bind "${NEXUS_SANDBOX_SHARE}" "${NEXUS_SANDBOX_SHARE}")
fi

# Last, once every mount point exists: writes to anything unlisted fail loudly
# instead of vanishing with the tmpfs.
args+=(--remount-ro "${HOME}" --remount-ro /)

if [[ $# -eq 0 ]]; then
    set -- "${SHELL:-/bin/zsh}"
fi

exec bwrap "${args[@]}" -- "$@"
