import curses


class UI:
    def __init__(self, func, *args, **kwargs):
        self.scr = curses.initscr()
        curses.noecho()
        curses.cbreak()
        self.scr.keypad(True)
        self.scr.clear()

        try:
            func(self, *args, **kwargs)
        except Exception as e:
            raise e
        finally:
            curses.nocbreak()
            self.scr.keypad(False)
            curses.echo()
            curses.endwin()

    def display(self, row: int, col: int, string: str):
        self.scr.addstr(row, col, string)

    def display_df(self, row: int, col: int, df, cur):
        new_col = col
        new_row = row

        for col_name in df.columns.to_list():
            self.scr.addstr(new_row, new_col, col_name)
            new_col += 15

        for x, r in enumerate(df.itertuples()):
            new_col = col
            new_row += 1
            for y, e in enumerate(r[1:]):
                if y == cur.y and x == cur.x:
                    attr = curses.A_REVERSE
                else:
                    attr = curses.A_NORMAL

                self.scr.addstr(new_row, new_col, str(e), attr)
                new_col += 15

    def set(self, df, cur):
        df.iloc[cur.x, cur.y] = ""
        self.scr.addstr(10, 0, str(df.iloc[cur.x, cur.y]))


class Cursor:
    def __init__(self, mins: (int, int), maxs: (int, int)):
        self.mins = mins
        self.maxs = maxs
        self.x = 0
        self.y = 0

    def up(self):
        if self.x == self.mins[0]:
            self.x = self.maxs[0] - 1
        else:
            self.x -= 1

    def down(self):
        if self.x == self.maxs[0] - 1:
            self.x = self.mins[0]
        else:
            self.x += 1

    def left(self):
        if self.y == self.mins[1]:
            self.y = self.maxs[1] - 1
        else:
            self.y -= 1

    def right(self):
        if self.y == self.maxs[1] - 1:
            self.y = self.mins[1]
        else:
            self.y += 1



