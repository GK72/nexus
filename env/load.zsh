#!/usr/bin/env zsh

source /usr/share/zsh-theme-powerlevel10k/powerlevel10k.zsh-theme
source /usr/share/zsh/plugins/zsh-autosuggestions/zsh-autosuggestions.zsh
source /usr/share/zsh/plugins/zsh-syntax-highlighting/zsh-syntax-highlighting.zsh
source /usr/share/fzf/key-bindings.zsh
source /usr/share/fzf/completion.zsh

source ~/nxs/env/omz/alias.zsh
source ~/nxs/env/omz/variables.zsh
source ~/nxs/env/omz/functions.zsh

# Lock the history file with fcntl instead of a lock file next to it, the same
# way inside and outside tools/sandbox.sh, where $HOME is read-only and only
# the history file itself is writable. Saving by copy would need a new file in
# $HOME too.
setopt hist_fcntl_lock
if [[ -n "${NEXUS_SANDBOX:-}" ]]; then
    unsetopt hist_save_by_copy
fi
