#!/usr/bin/env bash

if [[ ! -d "~/repos/nexus.git" ]]; then
    git clone --bare https://github.com/GK72/nexus.git ~/repos/nexus.git
    (cd ~/repos/nexus.git && git worktree add master master)
else
    echo "Nexus repo already exists"
fi

ln -sf ~/repos/nexus.git/master ~/nxs

stow home -d ~/nxs/env -t ~/
stow vim -d ~/nxs/env -t ~/

touch ~/.cenv       # Custom environment settings
