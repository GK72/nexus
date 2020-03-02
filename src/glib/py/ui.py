#!/usr/bin/python3

import curses
from curses import wrapper

class UI():
    def __init__(self):
        self.stdscr = curses.initscr()
        curses.noecho()
        curses.cbreak()                     # Non-buffered input mode

        self.curpos_x = 0
        self.curpos_y = 0
        self.n_options = 0
        self.selection = 0
        self._cond_terminate = False

    def printc(self, s: str, y, x):
        self.stdscr.addstr(y, x, s)

    def printn(self, s: str, offset_y=0, offset_x=0):
        y = self.curpos_y + offset_y
        x = self.curpos_x + offset_x
        self.stdscr.addstr(y, x, s)
        self.curpos_y += 1

    def footer(self, s: str):
        y, x = self.stdscr.getmaxyx()
        self.stdscr.addstr(y - 2, 0, "-" * (x - 1))
        self.stdscr.addstr(y - 1, 0, s)

    def run(self):
        c = ""
        while not self._cond_terminate:
            self.reset()
            self._create_main_menu()
            self.printc("#", self.selection + 1, 0)
            self.footer("Version 0.01")

            self._get_user_input()
            self.stdscr.refresh()

    def close(self):
        curses.nocbreak()
        # curses.nl()
        self.stdscr.keypad(False)
        curses.echo()
        curses.endwin()

    def reset(self):
        self.curpos_x = 0
        self.curpos_y = 0
        self.stdscr.clear()

    def _create_main_menu(self):
        self.n_options = 3
        self.printn("Python Curses UI")
        self.printn("1.", offset_x=2)
        self.printn("2.", offset_x=2)
        self.printn("3.", offset_x=2)

    def _get_user_input(self) -> None:
        c = self.stdscr.getch()
        if c == ord('h'):
            pass
        elif c == ord('j'):
            self.selection = (self.selection + 1) % self.n_options
        elif c == ord('k'):
            self.selection = (self.selection - 1) % self.n_options
        elif c == ord('l'):
            pass
        elif c == ord('o'):
            self.printc(str(self.selection) + " was selected", 8, 2)
        elif c == ord('q'):
            self._cond_terminate = True


if __name__ == "__main__":
    try:
        ui = UI()
        ui.run()
    except Exception as e:
        print("Exception happened:", e)
    finally:
        ui.close()