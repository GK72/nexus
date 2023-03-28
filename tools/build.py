#!/usr/bin/env python3

import argparse
import logging

import os
import shlex
import subprocess
import shutil

from enum import Enum


def shell(command: str, verbose=False) -> int:
    """ Run a shell command

    Returns with the code returned by the process
    """

    if verbose:
        print(command)      # TODO: use logging module

    result = subprocess.run(shlex.split(command))
    return result.returncode


def configure(project: str,
              build_dir: str,
              build_type: str,
              generator: str,
              sanitizers: str,
              verbose: bool,
              forward_args=None):
    """ Configure a CMake a project

    project: full path to the project
    """

    cmake_cmd = " ".join([
        f"cmake -S {project} -B {build_dir}",
        f"-DCMAKE_BUILD_TYPE={build_type}",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        f"-G{generator}"
    ])

    if sanitizers != "nosan":
        cmake_cmd += f" -DSANITIZERS={sanitizers}"

    if forward_args:
        cmake_cmd += f" {forward_args}"

    shell(cmake_cmd, verbose=verbose)


def build(build_dir: str,
          target: str,
          jobs: int,
          verbose: bool):
    """ Build a configured CMake project """

    shell(f"cmake --build {build_dir} --target {target} -- -j{jobs}", verbose=verbose)


def create_build_path(project: str, build_type: str, compiler: str, sanitizers: str) -> str:
    if sanitizers == "nosan":
        san = ""
    else:
        san = f"-{sanitizers}"

    return os.path.join(
        os.path.realpath(project),
        "build",
        f"{build_type.lower()}-{os.path.basename(compiler)}{san}"
    )


def parse_arguments():
    args = argparse.ArgumentParser()
    args.add_argument("-p", "--project")
    args.add_argument("-b", "--build-type", default="Debug")
    args.add_argument("-d", "--delete", action="store_true", help="Clean build")
    args.add_argument("-g", "--generator", default="'Unix Makefiles'", choices=["'Unix Makefiles'", "Ninja"])
    args.add_argument("-j", "--jobs", default=1)
    args.add_argument("-s", "--sanitizers", default="nosan", choices=["asan", "tsan"])
    args.add_argument("-t", "--target", default="all")
    args.add_argument("-v", "--verbose", action="store_true")

    return args.parse_args()


def main():
    args = parse_arguments()

    build_dir = create_build_path(args.project, args.build_type, os.getenv("CXX"), args.sanitizers)

    if args.delete:
        if input(f"Are you sure to remove `{build_dir}` (press 'y' to proceed): ") == "y":
            shutil.rmtree(build_dir)

    configure(args.project, build_dir, args.build_type, args.generator, args.sanitizers, args.verbose)
    build(build_dir, args.target, args.jobs, args.verbose)


if __name__ == "__main__":
    main()
