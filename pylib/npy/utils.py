import logging
import os
import shlex
import subprocess

from typing import List
from npy.types import AnsiColors


class ColorLogFormatter(logging.Formatter):
    formats = {
        logging.DEBUG:    "%(asctime)s [%(name)s] {}%(levelname)s:{} %(message)s".format(AnsiColors.BLUE.value, AnsiColors.DEFAULT.value),
        logging.INFO:     "%(asctime)s [%(name)s] {}%(levelname)s:{} %(message)s".format(AnsiColors.GREEN.value, AnsiColors.DEFAULT.value),
        logging.WARNING:  "%(asctime)s [%(name)s] {}%(levelname)s:{} %(message)s".format(AnsiColors.YELLOW.value, AnsiColors.DEFAULT.value),
        logging.ERROR:    "%(asctime)s [%(name)s] {}%(levelname)s:{} %(message)s".format(AnsiColors.RED.value, AnsiColors.DEFAULT.value),
        logging.CRITICAL: "%(asctime)s [%(name)s] {}%(levelname)s:{} %(message)s".format(AnsiColors.BOLD_RED.value, AnsiColors.DEFAULT.value)
    }

    def format(self, record):
        log_fmt = self.formats.get(record.levelno)
        formatter = logging.Formatter(log_fmt)
        return formatter.format(record)


def create_symlink(src: str, dst: str):
    """ Create symlink if it does not exist, else check if it points to a valid
    path and unlink if it is a broken link """

    if os.path.exists(dst):
        return

    if os.path.lexists(dst):
        os.unlink(dst)
    os.symlink(src, dst)


def run_command(command: str, shell=False) -> int:
    """ Run a command

    Returns with the code returned by the process.
    Raises exception if the `command` is not valid.
    """

    if command in ["", None]:
        raise Exception("Command cannot be empty or `None`")

    try:
        process = subprocess.Popen(shlex.split(command), shell=shell)
        process.wait()
    except KeyboardInterrupt:
        process.terminate()

    return process.returncode


def find_files(what: str, where: str) -> List[str]:
    """ Find files recursively in a given path """

    found_files = []

    for root, dir, files in os.walk(where):
        if what in files:
            found_files.append(os.path.join(root, what))

    return found_files
