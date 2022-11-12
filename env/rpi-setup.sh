# Download the Imager from `https://www.raspberrypi.com/software/` and install
# the Manjaro Linux from other OS menu
#
# Connect it on ethernet, login with `ssh root@IP` and follow the first-time setup
#
# Assumption: this repository is setup in the following way
#   git clone https://github.com/GK72/nexus --bare ~/repos/nexus.git
#   git -C ~/repos/nexus.git worktree add master

net_packages=(
    rsync
    tcpdump
    traceroute
    wireshark-cli
)

zsh_ext=(
    zsh
    zsh-autosuggestions
    zsh-syntax-highlighting
    zsh-theme-powerlevel10k
)

tools=(
    bat                 # `cat` but better
    bpytop              # `top` but better
    exa                 # `ls` but better
    fzf                 # Fuzzy finder
    git
    git-delta           # `diff` but better
    lf                  # File manager
    neovim
    ripgrep             # Dependency for Neovim
    stow
    the_silver_searcher # `ag`; `grep` but better
    tmux
)

lsps=(
    bash-language-server
    jedi-language-server
    lua-language-server
    yaml-language-server
)

compilers=(
    clang
    rust
)

sudo pacman -Sy \
    ${net_packages[@]} \
    ${zsh_ext[@]} \
    ${tools[@]} \
    ${compilers[@]} \
    ${lsps[@]}

ln -sf ~/repos/nexus.git/master ~/nxs
stow home -d ~/nxs/env -t ~/

curl -fLo \
    ~/.local/share/nvim/site/autoload/plug.vim --create-dirs \
    https://raw.githubusercontent.com/junegunn/vim-plug/master/plug.vim

nvim --headless \
    -u <(sed -n '1,/call plug#end()/p' ~/${XDG_CONFIG_HOME:-nxs/env/vim/.config/}/nvim/init.vim) \
    +PlugInstall \
    +qa

py_ver=$(python3 --version | cut -f2 -d' ' | cut -f1,2 -d'.')

sudo cp \
    ~/nxs/env/tmux/tmux.json \
    /usr/lib/python${py_ver}/site-packages/powerline/config_files/themes/tmux/default.json

git clone https://github.com/marlonrichert/zsh-autocomplete.git ~/repos/zsh-autocomplete

touch ~/.cenv       # Custom environment settings
