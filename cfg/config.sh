#!/bin/bash

# Environment setup script
# gkpro @ 2020-03-23

function printPad() {
    # Move the cursor forward N columns:    \033[<N>C
    # Save cursor position:                 \033[s
    # Restore cursor position:              \033[u
    printf "\r\033[s\033[$2C$1\n\033[u\n"
}

PATH_DIR_SCRIPT="$( cd "$( dirname "${BASH_SOURCE[0]}"  )" >/dev/null 2>&1 && pwd  )"
PATH_REPO_GLIB="$(basename $(dirname ${PATH_DIR_SCRIPT}))"

if [[ $(basename "${PATH_REPO_GLIB}") != "glib" ]]; then
    printf "Script is not in gLib repository\n"
    exit 1
fi

function usage() {
    cat <<EOH
Usage: ${0##*/} [install] [update] [backup]
  install           Checking environment and installs tools if not available
  update            Copies config files from glib/cfg
  backup            Copies config files into glib/cfg
EOH
}

function install() {
    command -v apt >/dev/null 2>&1 && {
        printf "Using apt package manager\n"
        INSTALL_ZSH="sudo apt-get install zsh"
        INSTALL_GCC="sudo apt-get install gcc"
        INSTALL_CMAKE="sudo apt-get install ccmake"
        INSTALL_CCMAKE="sudo apt-get install cmake-curses-gui"
        INSTALL_CCACHE="sudo apt-get install ccache"
        INSTALL_CPPCHECK="sudo apt-get install cppcheck"
        INSTALL_DOXYGEN="sudo apt-get install doxygen"
        INSTALL_GRAPHVIZ="sudo apt-get install graphviz"
        INSTALL_TIMEWARRIOR="sudo apt-get install timewarrior"
    }

    ###############################################################################
    #                             Checking environment                            #
    ###############################################################################

    # Shell
    printf "Checking ZSH..."
    command -v zsh >/dev/null 2>&1 && printPad OK 30 || {
        ${INSTALL_ZSH} && {
            zsh
            sh -c "$(curl -fsSL https://raw.github.com/robbyrussell/oh-my-zsh/master/tools/install.sh)"
            git clone https://github.com/romkatv/powerlevel10k.git $ZSH_CUSTOM/themes/powerlevel10k

            git clone https://github.com/zsh-users/zsh-syntax-highlighting.git
            echo "source ${(q-)PWD}/zsh-syntax-highlighting/zsh-syntax-highlighting.zsh" >> ${ZDOTDIR:-$HOME}/.zshrc
            source ./zsh-syntax-highlighting/zsh-syntax-highlighting.zsh

            git clone https://github.com/zsh-users/zsh-autosuggestions \
                ${ZSH_CUSTOM:-~/.oh-my-zsh/custom}/plugins/zsh-autosuggestions

        } || printPad "Failed!" 30
    }

    # Compiler and build automation
    printf "Checking gcc..."
    command -v g++ >/dev/null 2>&1 && printPad OK 30 || "${INSTALL_GCC}"

    printf "Checking cmake..."
    command -v cmake >/dev/null 2>&1 && printPad OK 30 || "${INSTALL_CMAKE}"

    printf "Checking ccmake..."
    command -v ccmake >/dev/null 2>&1 && printPad OK 30 || "${INSTALL_CCMAKE}"

    printf "Checking ccache..."
    command -v ccache >/dev/null 2>&1 && printPad OK 30 || "${INSTALL_CCACHE}"

    printf "Checking cppcheck..."
    command -v cppcheck >/dev/null 2>&1 && printPad OK 30 || "${INSTALL_CPPCHECK}"

    # Doxygen
    printf "Checking doxygen..."
    command -v doxygen >/dev/null 2>&1 && printPad OK 30 || {
        "${INSTALL_DOXYGEN}"
        "${INSTALL_GRAPHVIZ}"
    }

    # Testing environment
    printf "Checking Catch2..."
    if [[ -e /usr/local/lib/cmake/Catch2/Catch2Targets.cmake ]]; then
        printPad OK 30
    else
        cd ~/repos
        git clone https://github.com/catchorg/Catch2.git
        cmake -Bbuild -H. -DBUILD_TESTING=OFF
        sudo cmake --build build/ --target install
    fi

    # LF file manager
    printf "Checking lf..."
    command -v lf >/dev/null 2>&1 && printPad OK 30 || {
        curl -L https://github.com/gokcehan/lf/releases/download/r13/lf-linux-amd64.tar.gz | tar xzC ~/.local/bin
        chmod +x ~/.local/bin/lf
        sudo mv ~/.local/bin/lf /usr/local/bin
    }

    # NeoVim
    printf "Checking nvim..."
    command -v nvim >/dev/null 2>&1 && printPad OK 30 || {
        curl -fsSL https://github.com/neovim/neovim/releases/download/v0.4.3/nvim.appimage -o ~/nvim.appimage
        ./nvim.appimage --appimage-extract
        sudo cp -r ~/squashfs-root/usr/* /usr/

        mkdir ~/.config/nvim
        touch ~/.config/nvim/init.vim

        # Plugin Manager
        curl -fLo ~/.local/share/nvim/site/autoload/plug.vim --create-dirs \
            https://raw.githubusercontent.com/junegunn/vim-plug/master/plug.vim

        sudo find /usr/share/nvim -type d -exec chmod 755 {} +

        ~/.local/share/nvim/plugged/YouCompleteMe/install.py --clang-completer

        # Cleaning up
        printf "Cleaning up temporary files...\n"
        rm -rf ~/squashfs-root
        rm -rf ~/nvim.appimage
    }

    # Shell extensions
    printf "Checking shell extensions..."
    command -v timew >/dev/null 2>&1 && printPad OK 30 || {
        "${INSTALL_TIMEWARRIOR}"
    }

    printf "\nEnvironment setup has finished.\n"
}

function backup() {
    printf "Backing up config files to $PATH_DIR_SCRIPT\n"
    cp ~/.p10k.zsh "$PATH_DIR_SCRIPT"
    cp ~/.zshrc "$PATH_DIR_SCRIPT"
    cp ~/.config/nvim/init.vim "$PATH_DIR_SCRIPT"
    cp ~/.oh-my-zsh/custom/alias.zsh "$PATH_DIR_SCRIPT"
    cp ~/.oh-my-zsh/custom/functions.zsh "$PATH_DIR_SCRIPT"
    cp ~/.oh-my-zsh/custom/variables.zsh "$PATH_DIR_SCRIPT"
}

function update() {
    printf "Updating config files from $PATH_DIR_SCRIPT\n"
    cp "$PATH_DIR_SCRIPT"/.p10k.zsh ~
    cp "$PATH_DIR_SCRIPT"/.zshrc ~
    cp "$PATH_DIR_SCRIPT"/init.vim ~/.config/nvim
    cp "$PATH_DIR_SCRIPT"/alias.zsh ~/.oh-my-zsh/custom
    cp "$PATH_DIR_SCRIPT"/functions.zsh ~/.oh-my-zsh/custom
    cp "$PATH_DIR_SCRIPT"/variables.zsh ~/.oh-my-zsh/custom
}

if [[ $# -eq 0 ]]; then
    usage
fi

while [[ $# -gt 0 ]]; do
    case $1 in
        install)        install;;
        backup)         backup;;
        update)         update;;
        *)              usage;;
    esac
    shift
done

