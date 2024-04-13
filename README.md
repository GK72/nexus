# Nexus Project

Personal project with all kinds of developer configuration, tools, experimental
codes; a collection of knowledge.

# Coding and styleguide

The commits are following the [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/).

# Building GFX application (Rust)

There is a dependency version mismatch in `winit`.

`glium` version needs to be updated in `imgui-glium-renderer/Cargo.toml`

* https://github.com/imgui-rs/imgui-rs/issues/759
* https://github.com/imgui-rs/imgui-rs/pull/765/commits/1a3e7ca748584c561ff0fb395a215f23b9c0e4da#diff-e74620692fda6a49120226a2c1c4bfe2080cf2e0d9066e8159fe9fc6906c054a

# Building Rust code for Alpine

Use Rustup and build for musl as Alpine does not use glibc.

https://musl.libc.org/

```sh
rustup target add aarch64-unknown-linux-musl
rustup target add x86_64-unknown-linux-musl
```

To build a fully statically linked binary:

```sh
cargo build --release --target=aarch64-unknown-linux-musl
```

# CppFront

`cppfront` is automatically called for `cpp2` sources by using
`https://github.com/modern-cmake/cppfront`.

Install `cppfront` via the following command:

    build -p deps/cx2cmake/ -b Release -t install -- -DCMAKE_INSTALL_PREFIX=$HOME/.local/

Enable `cppfront` in subdirectories with:

    cppfront_enable(TARGETS ${target_name})

C++ syntax 2 is referred as `cx2` in this repo.

# Project structure

```
├── ci                      CI-like workflow
├── cmake
├── env                     Environment related files (Shell, NeoVim, Tmux, etc...)
├── nxs                     Nexus library
├── nrs-meta                Nexus Rust meta library
├── nrs-app                 Nexus Rust applications
├── tests                   Integration tests
├── include                 Public headers (legacy C++)
├── src                     Implementation detail (static/shared libraries) (legacy C++)
├── unit-tests              (legacy C++)
└── tools                   Developer tools
```
