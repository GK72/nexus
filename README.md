# Nexus Project

Personal project with all kinds of developer configuration, tools, experimental
codes; a collection of knowledge.

## Coding and styleguide

The commits are following the [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/).

## Building

The project uses `baldr` for building.

```bash
baldr -p . -b Debug -j 8 -t <target> [--run]
```

## End-to-end Testing

```bash
./tests/btx.sh
```
