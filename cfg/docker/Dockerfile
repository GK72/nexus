FROM manjarolinux/base

RUN pacman -Sy --noconfirm sudo
RUN pacman -Sy --noconfirm awk
RUN pacman -Sy --noconfirm grep
RUN pacman -Sy --noconfirm zsh
RUN pacman -Sy --noconfirm git
RUN pacman -Sy --noconfirm neovim
RUN pacman -Sy --noconfirm the_silver_searcher
RUN pacman -Sy --noconfirm cmake
RUN pacman -Sy --noconfirm ninja
RUN pacman -Sy --noconfirm clang

RUN git clone https://aur.archlinux.org/lf.git /tmp/lf
WORKDIR /tmp/lf
# RUN makepkg --clean --syncdeps --install --noconfirm

# cd ~/repos/llvm-project
# mkdir build
# cd build
# cmake -G Ninja -DLLVM_ENABLE_PROJECTS=clang;clang-tools-extra;libcxx;libcxxabi;lldb;lld -DCMAKE_BUILD_TYPE=Release ../llvm
# cmake --build . -j2
