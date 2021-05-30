import dash_html_components as html
import dash_core_components as dcc
import plotly.express as px
import pandas as pd

from dash import Dash
from dash.dependencies import Input, Output

css = ["https://codepen.io/chriddyp/pen/bWLwP.css"]
app = Dash(__name__, external_stylesheets=css)


class Colors:
    BACKGROUND = "#FFFFFF"
    TEXT = "#000000"


df = pd.DataFrame({
    "Fruit": ["Apples", "Oranges"],
    "Amount": [3, 5],
    "City": ["Bp", "Deb"]
})

fig = px.bar(
    df,
    x="Fruit",
    y="Amount",
    color="City",
    barmode="group"
)

fig.update_layout(
    plot_bgcolor=Colors.BACKGROUND,
    paper_bgcolor=Colors.BACKGROUND,
    font_color=Colors.TEXT
)

mdtext = """
# Title

Some text.
Some text.
Some text.

- list1
- list2
- list3

## Subtitle

Something
"""

app.layout = html.Div(
        style={
            "backgroundColor": Colors.BACKGROUND,
            "columnCount": 1
        },

        children=[

    html.Label("Dropdown"),
    dcc.Dropdown(
        options=[
            { "label": "Label1", "value": "L1" },
            { "label": "Label2", "value": "L2" },
            { "label": "Label3", "value": "L3" }
        ],
        value="L2"
    ),

    html.Label("Multi Dropdown"),
    dcc.Dropdown(
        options=[
            { "label": "Label1", "value": "L1" },
            { "label": "Label2", "value": "L2" },
            { "label": "Label3", "value": "L3" }
        ],
        value="L2",
        multi=True
    ),

    html.Label("Radio"),
    dcc.RadioItems(
        options=[
            { "label": "Label1", "value": "L1" },
            { "label": "Label2", "value": "L2" },
            { "label": "Label3", "value": "L3" }
        ],
        value="L2",
    ),

    html.Label("Checklist"),
    dcc.Checklist(
        options=[
            { "label": "Label1", "value": "L1" },
            { "label": "Label2", "value": "L2" },
            { "label": "Label3", "value": "L3" }
        ],
        value="L2",
    ),

    html.Label("Checklist"),
    dcc.Input(value="Some default input", type="text"),

    html.Label("Slider"),
    dcc.Slider(
        min=3,
        max=9,
        marks={ i: f"Label {i}" if i == 1 else str(i) for i in range (4,6)},
        value=4
    ),

    html.H1(children="Hello",
        style={
            "textAlign": "center",
            "color": "#AAAAFF"
        }
    ),
    dcc.Graph(
        id="example-graph",
        figure=fig
    ),
    dcc.Markdown(children=mdtext),

    html.Div([
        "Input: ",
        dcc.Input(id="input1", value="init", type="text")
    ]),

    html.Br(),

    html.Div(id="output")
])


@app.callback(
    Output(component_id="output", component_property="children"),
    Input(component_id="input1", component_property="value")
)
def func(x):
    return f"Output: {x}"


if __name__ == "__main__":
    app.server.run(debug=True)
