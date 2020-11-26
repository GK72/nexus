from npx.ui import UI
from npx.ui import Cursor

import pandas as pd


def example_df():
    return pd.DataFrame({
        "Date": ["2020.01.01", "2020.02.01", "2020.02.02"],
        "Price": [100, 200, 300]
    })


def main(ui):
    df = example_df()
    cur = Cursor((0,0), df.shape)

    while True:
        ch = ui.scr.getkey()
        ui.scr.clear()

        if   ch == 'j':     cur.down()
        elif ch == 'k':     cur.up()
        elif ch == 'h':     cur.left()
        elif ch == 'l':     cur.right()
        elif ch == 'o':     ui.set(df, cur)
        elif ch == 'q':     break

        ui.display_df(2, 0, df, cur)
        ui.scr.refresh()


if __name__ == "__main__":
    ui = UI(main)


# --=[ Sandbox ]=--

if False:
    df = example_df()
    df.shape
    df.iloc[1,0]
