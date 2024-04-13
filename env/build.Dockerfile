FROM manjarolinux/base

RUN pacman -Sy --noconfirm  \
    zsh \
    python-pip

# Build tools
RUN pacman -Sy --noconfirm \
    clang \
    ccache \
    ninja

RUN pacman -Sy --noconfirm \
    benchmark \
    gcovr \
    valgrind

RUN python3 -m pip install conan
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
