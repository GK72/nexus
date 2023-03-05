# Nexus Library

Personal project with all kinds of developer configuration, tools and a modern
C++ library; a collection of knowledge.

# Coding and styleguide

The commits are following the [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/).

# Project structure

```
├── ci                      CI-like workflow
├── cmake                   CMake files and library (only library files are installed for the public)
├── conanfile.py            Conan recipe for Nexus Library
├── conanfile.txt           Dependencies (CMake input)
├── env                     Environment related files
├── example-project         Demo usage
├── include                 Public headers
├── src                     Implementation detail (static/shared libraries)
├── test                    Unit tests
└── tools                   Developer tools
```
