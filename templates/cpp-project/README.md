# @name@

@description@

The manual: [`doc/@name@.adoc`](doc/@name@.adoc).

## Build and test

Nexus must be checked out next to this repository (`../nexus`), or set `NEXUS_DIR`.

```sh
ci/pipeline.sh                          # lint, build (build/debug), unit and function tests, the book
ci/pipeline.sh --only function-test
```

## Layout

```
lib@name@/lib@name@/   the library: modules with their *.test.cpp unit tests
@name@/@name@/         the program
tests/function/        function tests (TAP, nci-test)
ci/                    the pipeline (nci)
doc/                   the book (tools/doc.sh doc)
tools/                 configure.sh (Conan + CMake), doc.sh (Nexus doc.sh)
```
