from enum import Enum


class AnsiColors(Enum):
    DARK_GREY = "\x1b[30;1m"
    RED       = "\x1b[31;20m"
    BOLD_RED  = "\x1b[31;1m"
    GREEN     = "\x1b[32;20m"
    YELLOW    = "\x1b[33;20m"
    BLUE      = "\x1b[34;20m"
    PURPLE    = "\x1b[35;20m"
    CYAN      = "\x1b[36;20m"
    WHITE     = "\x1b[37;1m"
    GREY      = "\x1b[38;20m"
    DEFAULT   = "\x1b[0m"
