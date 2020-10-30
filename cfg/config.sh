#!/bin/bash

# Environment setup script
# gkpro @ 2020-05-24

set -e

function printPad() {
    # Move the cursor forward N columns:    \033[<N>C
    # Save cursor position:                 \033[s
    # Restore cursor position:              \033[u
    printf "\r\033[s\033[$2C$1\n\033[u"
}

PATH_DIR_SCRIPT="$( cd "$( dirname "${BASH_SOURCE[0]}"  )" >/dev/null 2>&1 && pwd  )"
PATH_REPO_NEXUS="$(basename $(dirname ${PATH_DIR_SCRIPT}))"

if [[ $(basename "${PATH_REPO_NEXUS}") != "nexus" ]]; then
    printf "Script is not in Nexus repository\n"
    exit 1
fi

function usage() {
    cat <<EOH
Usage: ${0##*/} [install] [install-zshExt] [update] [backup]
  install           Checking environment and installs tools if not available
  install-zshExt    Installing ZSH extensions (Oh-My-Zsh, Powerlevel)
  update            Copies config files from nexus/cfg
  backup            Copies config files into nexus/cfg
EOH
}

function installZshExt() {
    sh -c "$(curl -fsSL https://raw.github.com/robbyrussell/oh-my-zsh/master/tools/install.sh)"
    git clone https://github.com/romkatv/powerlevel10k.git $ZSH_CUSTOM/themes/powerlevel10k

    git clone https://github.com/zsh-users/zsh-syntax-highlighting.git
    source ./zsh-syntax-highlighting/zsh-syntax-highlighting.zsh

    git clone https://github.com/zsh-users/zsh-autosuggestions \
        ${ZSH_CUSTOM:-~/.oh-my-zsh/custom}/plugins/zsh-autosuggestions
}

function install() {
    command -v apt >/dev/null 2>&1 && {
        printf "Using apt package manager\n"
        INSTALL_ZSH="sudo apt-get install zsh"
        INSTALL_GCC="sudo apt-get install gcc"
        INSTALL_AG="sudo apt-get install silversearcher-ag"
        INSTALL_CMAKE="sudo apt-get install cmake"
        INSTALL_CCMAKE="sudo apt-get install cmake-curses-gui"
        INSTALL_CCACHE="sudo apt-get install ccache"
        INSTALL_CPPCHECK="sudo apt-get install cppcheck"
        INSTALL_DOXYGEN="sudo apt-get install doxygen"
        INSTALL_GRAPHVIZ="sudo apt-get install graphviz"
        INSTALL_TIMEWARRIOR="sudo apt-get install timewarrior"
        INSTALL_PERF="sudo apt-get install linux-tools-generic"
    }

    command -v pacman >/dev/null 2>&1 && {
        printf "Using pacman package manager\n"
        INSTALL_ZSH="sudo pacman -S zsh"
        INSTALL_GCC="sudo pacman -S gcc"
        INSTALL_AG="sudo pacman -S silversearcher-ag"
        INSTALL_CMAKE="sudo pacman -S cmake"
        INSTALL_CCMAKE="sudo pacman -S cmake-curses-gui"
        INSTALL_CCACHE="sudo pacman -S ccache"
        INSTALL_CPPCHECK="sudo pacman -S cppcheck"
        INSTALL_DOXYGEN="sudo pacman -S doxygen"
        INSTALL_GRAPHVIZ="sudo pacman -S graphviz"
        INSTALL_TIMEWARRIOR="sudo pacman -S timewarrior"
        INSTALL_PERF="sudo pacman -S perf"
    }

    # ---------------------------------==[ Checking environment ]==---------------------------------

    # Shell
    printf "Checking ZSH..."
    command -v zsh >/dev/null 2>&1 && printPad OK 30 || {
        ${INSTALL_ZSH} && {
            zsh
            installZshExt
       } || printPad "Failed!" 30
    }

    # Compiler and build automation
    printf "Checking gcc..."
    command -v g++ >/dev/null 2>&1 && printPad OK 30 || eval "${INSTALL_GCC}"

    printf "Checking cmake..."
    command -v cmake >/dev/null 2>&1 && printPad OK 30 || eval "${INSTALL_CMAKE}"

    printf "Checking ccmake..."
    command -v ccmake >/dev/null 2>&1 && printPad OK 30 || eval "${INSTALL_CCMAKE}"

    printf "Checking ccache..."
    command -v ccache >/dev/null 2>&1 && printPad OK 30 || eval "${INSTALL_CCACHE}"

    printf "Checking cppcheck..."
    command -v cppcheck >/dev/null 2>&1 && printPad OK 30 || eval "${INSTALL_CPPCHECK}"

    printf "Checking perf..."
    command -v perf >/dev/null 2>&1 && printPad OK 30 || eval "${PERF}"

    # Doxygen
    printf "Checking doxygen..."
    command -v doxygen >/dev/null 2>&1 && printPad OK 30 || {
        eval "${INSTALL_DOXYGEN}"
        eval "${INSTALL_GRAPHVIZ}"
    }

    # LF file manager
    printf "Checking lf..."
    command -v lf >/dev/null 2>&1 && printPad OK 30 || {
    mkdir -p ~/.local/bin
        curl -L https://github.com/gokcehan/lf/releases/download/r13/lf-linux-amd64.tar.gz | tar xzC ~/.local/bin
        chmod +x ~/.local/bin/lf
        sudo mv ~/.local/bin/lf /usr/local/bin
    }

    # NeoVim
    printf "Checking nvim..."
    command -v nvim >/dev/null 2>&1 && printPad OK 30 || {
        curl -fSL https://github.com/neovim/neovim/releases/download/v0.4.3/nvim.appimage -o ~/nvim.appimage
        sudo chmod +x ~/nvim.appimage
        ~/nvim.appimage --appimage-extract
        sudo cp -r ~/squashfs-root/usr/* /usr/

        mkdir -p ~/.config/nvim
        touch ~/.config/nvim/init.vim

        # Plugin Manager
        curl -fLo ~/.local/share/nvim/site/autoload/plug.vim --create-dirs \
            https://raw.githubusercontent.com/junegunn/vim-plug/master/plug.vim

        sudo find /usr/share/nvim -type d -exec chmod 755 {} +

        echo "Use the following to install YCM"
        echo " ~/.local/share/nvim/plugged/YouCompleteMe/install.py --clang-completer"

        # Cleaning up
        printf "Cleaning up temporary files...\n"
        rm -rf ~/squashfs-root
        rm -rf ~/nvim.appimage
    }

    # Shell extensions
    printf "Checking shell extensions..."
    command -v timew >/dev/null 2>&1 && printPad OK 30 || {
        eval "${INSTALL_TIMEWARRIOR}" || echo "Package not found"
    }

    printf "\nEnvironment setup has finished.\n"
}

function backup() {
    printf "Backing up config files to ${PATH_DIR_SCRIPT}\n"
    cp ~/.p10k.zsh "${PATH_DIR_SCRIPT}"
    cp ~/.zshrc "${PATH_DIR_SCRIPT}"
    cp ~/.tmux.conf "${PATH_DIR_SCRIPT}"
    cp ~/.config/powerline/config_files/themes/tmux/default.json "${PATH_DIR_SCRIPT}"/tmux.json
    cp ~/.config/nvim/init.vim "${PATH_DIR_SCRIPT}"
    cp ~/.oh-my-zsh/custom/alias.zsh "${PATH_DIR_SCRIPT}"
    cp ~/.oh-my-zsh/custom/functions.zsh "${PATH_DIR_SCRIPT}"
    cp ~/.oh-my-zsh/custom/variables.zsh "${PATH_DIR_SCRIPT}"
    cp -r ~/.local/share/nvim/plugin/* "${PATH_DIR_SCRIPT}/vimfiles/plugin"
    cp -r ~/.local/share/nvim/templates/* "${PATH_DIR_SCRIPT}/vimfiles/templates"
}

function update() {
    printf "Updating config files from ${PATH_DIR_SCRIPT}\n"
    cp -iu "${PATH_DIR_SCRIPT}"/.p10k.zsh ~
    cp -iu "${PATH_DIR_SCRIPT}"/.zshrc ~
    cp -iu "${PATH_DIR_SCRIPT}"/.tmux.conf ~
    cp -iu "${PATH_DIR_SCRIPT}"/tmux.json ~/.config/powerline/config_files/themes/tmux/default.json
    cp -iu "${PATH_DIR_SCRIPT}"/init.vim ~/.config/nvim
    cp -iu "${PATH_DIR_SCRIPT}"/alias.zsh ~/.oh-my-zsh/custom
    cp -iu "${PATH_DIR_SCRIPT}"/functions.zsh ~/.oh-my-zsh/custom
    cp -iu "${PATH_DIR_SCRIPT}"/variables.zsh ~/.oh-my-zsh/custom

    mkdir -p ~/.local/share/nvim/plugin
    mkdir -p ~/.local/share/nvim/templates
    cp -iur "${PATH_DIR_SCRIPT}"/vimfiles/* ~/.local/share/nvim/
}

if [[ $# -eq 0 ]]; then
    usage
fi

while [[ $# -gt 0 ]]; do
    case $1 in
        install)            install;;
        install-zshExt)     installZshExt;;
        backup)             backup;;
        update)             update;;
        *)                  usage;;
    esac
    shift
done

