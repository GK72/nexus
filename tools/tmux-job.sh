#!/usr/bin/env bash

TMUX_SESSION=$(tmux display-message -p "#S")
TMUX_WINDOW=job
TMUX_MESSAGE="Job finished."
TMUX_TARGET="${TMUX_SESSION}:${TMUX_WINDOW}"

if ! tmux has-session -t "${TMUX_TARGET}" 2> /dev/null; then
    tmux new-window -dn "${TMUX_WINDOW}"
fi

tmux send-keys -t "${TMUX_TARGET}" " $*; tmux display-message -N -d 2000 \"${TMUX_MESSAGE}\""
