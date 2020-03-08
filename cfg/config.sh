#!/bin/bash

# Environment setup script
# gkpro @ 2020-02-25

function printPad() {
    # Move the cursor forward N columns:    \033[<N>C
    # Restore cursor position:              \033[u
    printf "\r\033[$2C$1\n\033[u"
}

PATH_REPO_GLIB="$( cd "$( dirname "${BASH_SOURCE[0]}"  )" >/dev/null 2>&1 && pwd  )"
if [[ $(basename "${PATH_REPO_GLIB}") != "glib" ]]; then
    printf "Script is not in gLib repository\n"
    exit 1
fi

command -v apt >/dev/null 2>&1 && {
    printf "Using apt package manager\n"
    INSTALL_ZSH="sudo apt-get install zsh"
    INSTALL_GCC="sudo apt-get install gcc"
    INSTALL_CMAKE="sudo apt-get install ccmake"
    INSTALL_CCMAKE="sudo apt-get install cmake-curses-gui"
    INSTALL_CCACHE="sudo apt-get install ccache"
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
command -v timew >/dev/null 2>&1 && printpad OK 30 || {
    "${INSTALL_TIMEWARRIOR}"
}

printf "\nEnvironment setup has finished.\n"

###############################################################################
#                         Copying configuration files                         #
###############################################################################

read -p "Do you want to copy configurations? (y/n) " confirm && \
    [[ $confirm == [yY] || $confirm == [yY][eE][sS] ]] || exit 1

printf "Copying NeoVim configuration...\n"
mv ~/.config/nvim/init.vim ~/.config/nvim/init.vim.bak
cp ${PATH_REPO_GLIB}/init.vim ~/.config/nvim

printf "Copying ZSH configuration...\n"
mv ~/.zshrc ~/.zshrc.bak
cp ${PATH_REPO_GLIB}/.zshrc ~
