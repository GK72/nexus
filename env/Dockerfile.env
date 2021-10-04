FROM archlinux

ARG DOCKER_GID
ARG USER_ID
ARG USERNAME=dockerdev

RUN groupadd -g $DOCKER_GID docker

RUN pacman -Sy --noconfirm \
    autoconf \
    automake \
    bat \
    cmake \
    clang \
    docker \
    fakeroot  \
    fd \
    fzf \
    git \
    go \
    jq \
    m4 \
    make \
    man \
    ninja \
    npm \
    perl \
    pkgconfig \
    powerline \
    python-pip \
    ripgrep \
    stow \
    sudo \
    the_silver_searcher \
    tmux \
    unzip \
    yarn \
    yq \
    zsh \
    zsh-autosuggestions \
    zsh-completions \
    zsh-syntax-highlighting \
    zsh-theme-powerlevel10k

RUN useradd \
        --create-home \
        -u $USER_ID \
        -G docker \
        -s /usr/bin/zsh \
        $USERNAME && \
    echo "$USERNAME ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers

USER $USERNAME

RUN git clone https://aur.archlinux.org/lf.git /tmp/lf && \
    (cd /tmp/lf && makepkg) && \
    sudo pacman -U --noconfirm /tmp/lf/*.zst && \
    rm -rf /tmp/lf

RUN git clone https://github.com/neovim/neovim ~/repos/neovim && \
    curl -fLo \
        ~/.local/share/nvim/site/autoload/plug.vim --create-dirs \
        https://raw.githubusercontent.com/junegunn/vim-plug/master/plug.vim && \
    (cd ~/repos/neovim && \
        make -j CMAKE_BUILD_TYPE=RelWithDebInfo && \
        sudo make install)

RUN git clone --depth 1 -- https://github.com/marlonrichert/zsh-autocomplete.git ~/repos/zsh-autocomplete

ADD cfgenv.sh /tmp

RUN /tmp/cfgenv.sh && \
    echo "export NXS_P10K_MSG=dev-env" >> ~/.cenv

RUN echo "Configuring Neovim..." && \
    nvim \
    --headless \
    -u <(sed -n '1,/call plug#end()/p' ~/.config/nvim/init.vim) \
    +PlugInstall \
    "+TSInstall bash cpp cmake dockerfile json python rst rust toml vim yaml" \
    +qa

RUN ~/nxs/env/install-lsps.sh

WORKDIR /home/$USERNAME
