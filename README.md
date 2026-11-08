# Nexus Project

Personal project with all kinds of developer configuration, tools, experimental
codes; a collection of knowledge.

## Coding and styleguide

The commits are following the [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/).

## Building

The project uses `baldr` for building.

```bash
baldr build -b Debug
baldr run -b Debug -t <target>
```

## Pipeline and Tests

The CI pipeline runs locally: lint, file headers, build, unit tests, function tests and the
documentation. See the nci book, [`doc/nci`](doc/nci/nci.adoc) (`tools/doc.sh doc/nci` renders it).

```bash
ci/pipeline.sh                      # everything
ci/pipeline.sh --changed            # what the changes since master affect
ci/pipeline.sh --only function-test # the function tests (`tests/function/*.sh`), after a build
ci/pipeline.sh --clean              # on a fresh clone of HEAD in /tmp/workspace/nexus
```

A function test also runs alone, against `build/debug` by default: `tests/function/btx.sh`.
