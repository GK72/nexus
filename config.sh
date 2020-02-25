#!/bin/bash

# Environment setup script
# gkpro @ 2020-02-25

# TODO:
# Check if the system is Ubuntu
# Parameterize script


# Checking environment

# ---------- SHELL ----------
# ZSH
command -v zsh >/dev/null 2?&1 && echo "   OK" || sudo apt-get install zsh

# Theme: Powerlevel10k
git clone https://github.com/romkatv/powerlevel10k.git $ZSH_CUSTOM/themes/powerlevel10k

# Zsh syntax highlighting
git clone https://github.com/zsh-users/zsh-syntax-highlighting.git
echo "source ${(q-)PWD}/zsh-syntax-highlighting/zsh-syntax-highlighting.zsh" >> ${ZDOTDIR:-$HOME}/.zshrc
source ./zsh-syntax-highlighting/zsh-syntax-highlighting.zsh

# ---------- CMAKE ----------
echo -n "Checking cmake..."
command -v cmake >/dev/null 2>&1 && echo "   OK" || sudo apt-get install ccmake

# ---------- CCMAKE ---------
echo -n "Checking ccmake..."
command -v ccmake >/dev/null 2>&1 && echo "   OK" || sudo apt-get install cmake-curses-gui

# ---------- CCACHE ---------
echo -n "Checking ccache..."
command -v ccache >/dev/null 2>&1 && echo "   OK" || sudo apt-get install ccache

# --------- DOXYGEN ---------
echo -n "Checking doxygen..."
command -v doxygen >/dev/null 2>&1 && echo "   OK" || {
    sudo apt-get install doxygen
    sudo apt-get install graphviz
}

# ---------- CATCH2 ---------
echo -n "Checking Catch2..."
if [[ -e /usr/local/lib/cmake/Catch2/Catch2Targets.cmake ]]; then
    echo "   OK"
else
    cd ~/repos
    git clone https://github.com/catchorg/Catch2.git
    cmake -Bbuild -H. -DBUILD_TESTING=OFF
    sudo cmake --build build/ --target install
fi

# ------------ LF -----------
echo -n "Checking lf..."
command -v lf >/dev/null 2>&1 && echo "   OK" || {
    curl -L https://github.com/gokcehan/lf/releases/download/r13/lf-linux-amd64.tar.gz | tar xzC ~/.local/bin
    chmod +x ~/.local/bin/lf
    sudo mv ~/.local/bin/lf /usr/local/bin
}

# ---------- NEOVIM ---------
echo -n "Checking nvim..."
command -v nvim >/dev/null 2>&1 && echo "   OK" || {
    curl -fsSL https://github.com/neovim/neovim/releases/download/v0.4.3/nvim.appimage -o ~/nvim.appimage
    ./nvim.appimage --appimage-extract
    sudo cp -r ~/squashfs-root/usr/* /usr/

    mkdir ~/.config/nvim
    touch ~/.config/nvim/init.vim

    # Plugin Manager
    curl -fLo ~/.local/share/nvim/site/autoload/plug.vim --create-dirs \
        https://raw.githubusercontent.com/junegunn/vim-plug/master/plug.vim

    sudo find /usr/share/nvim -type d -exec chmod 755 {} +

    # Cleaning up
    echo "Cleaning up temporary files..."
    rm -rf ~/squashfs-root
    rm -rf ~/nvim.appimage
}

 Oh My Zsh
 sh -c "$(curl -fsSL https://raw.github.com/robbyrussell/oh-my-zsh/master/tools/install.sh)"
