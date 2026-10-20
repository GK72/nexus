#!/usr/bin/env bash

if [[ ! -d "${HOME}/repos/nexus.git" ]]; then
    git clone --bare https://github.com/GK72/nexus.git ~/repos/nexus.git
    (cd ~/repos/nexus.git && git worktree add master master)
else
    echo "Nexus repo already exists"
fi

ln -sf ~/repos/nexus.git/master ~/nxs

mkdir -p ~/.config
stow home -d ~/nxs/env -t ~/
stow config -d ~/nxs/env -t ~/.config
sudo cp \
    ~/nxs/env/tmux/tmux.json \
    /usr/lib/python3.9/site-packages/powerline/config_files/themes/tmux/default.json

touch ~/.cenv       # Custom environment settings
