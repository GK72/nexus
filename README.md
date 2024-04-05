# Nexus Library

Personal project with all kinds of developer configuration, tools, experimental
codes; a collection of knowledge.

# Coding and styleguide

The commits are following the [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/).

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
├── include                 Public headers
├── rust                    Rust project
├── src                     Implementation detail (static/shared libraries)
├── unit-tests
└── tools                   Developer tools
```
