---
name: new-project
description: Stamp out a new C++ project (repository) of the author's from the Nexus template with `tools/new-project.sh`, or align an existing project with it. Use when asked to create, start, scaffold or bootstrap a new project or repo, migrate code into a new repo, or bring a repo in line with current practice. Not for adding a module or a library to an existing project.
---

# New Project

C++ projects start from `~/repos/nexus/templates/cpp-project`, which `~/repos/nexus/tools/new-project.sh`
copies under a new name. The New Projects book, `~/repos/nexus/doc/new-project/`, explains the
layout and why it is so (`pages/anatomy.adoc`). Read it before deviating from the template.

## Stamping

1. **Agree on the name and the one-line description.** The name (`[a-z][a-z0-9_-]*`) is the CMake
   project, `lib<name>` and the program `<name>`. The folder (`--dir`), namespace (`--namespace`),
   header project (`--part-of`) and author (`--author`) default from it or from Nexus and may
   differ (`baldr` in `baldr-cpp`, "Part of Freyja Project"). Ask which parts the project needs
   beyond the skeleton (a service, a client, a systemd unit, a book with figures) only when the
   request leaves it open.
2. **Stamp.** The author usually creates the GitHub repository first and clones it to
   `~/repos/<name>`; the script accepts a directory holding only `.git`.
   `~/repos/nexus/tools/new-project.sh <name> --description "..."` (`--dir` elsewhere).
3. **Run the pipeline before changing anything**: `ci/pipeline.sh` in the new project. It must pass.
   Suggest committing the untouched skeleton as `Feat: Project skeleton`; commit only when asked.
4. **Make it the project's own.** Replace the placeholder module (`lib<name>/lib<name>/greeting.*`),
   the program's `main.cpp` and `tests/function/cli.sh`; write `doc/pages/overview.adoc`; add
   dependencies to `conandata.yml` and `dependencies.cmake`. Keep the pipeline green after each step.
5. **Record it.** Tasks go to norn as `<name>/<area>/<task>`. Save a project memory in Nexus's
   memory directory (where it lives, its status, the decisions made), linked from `MEMORY.md`.

## Migrating code into a new project

Stamp first, then move the code in over the skeleton, adapting it to the template's layout
(`lib<name>/lib<name>/`, `<name>/<name>/`, `version.txt`, TAP function tests, `code-analysis`
through `link_libraries`). Agree first on what moves and what is left behind, and whether the
history comes along. Leave the old copy in place until the new one runs and the author has
confirmed; removing it is the author's call.

## Rules

- The template is the best practice. When a project needs something every project would need,
  change the template (and the book), not only the project, and check it by stamping a throwaway
  project into the scratchpad: `tools/new-project.sh demo --dir <scratch>/demo`, then
  `NEXUS_DIR=~/repos/nexus <scratch>/demo/ci/pipeline.sh`.
- Shared tooling (nci, nci-test, `doc.sh`, `file_headers.py`) stays in Nexus and is reached via
  `NEXUS_DIR` (default `../nexus`); never copy it into a project.
- Programs start through Nova's `NOVA_MAIN` and parse arguments with Boost.Program_options. Boost
  modules are all off in `conanfile.py` but those in `BOOST_MODULES_USED`.
- New projects are private: no LICENSE, `license = ""` in `.file-headers.toml`.
- In a pipeline, `NCI_ARTIFACTS` and `NCI_STAGE_DIR` exist only inside a running stage: read them
  in a function, not in the `nci::stage` line.
