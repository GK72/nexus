# DGM: Diagram as Code

DGM is a minimal, line-based language for drawing box-and-arrow diagrams. A
`.dgm` file is turned into an SVG by `dgm-tool`.

The MVP focuses on one thing: **precise control over where boxes go**. There
is no automatic graph layout; every box is either placed explicitly, placed
relative to another box, or follows the previous box in a row.

```bash
dgm-tool res/sample.dgm -o sample.svg
```

DGM is experimental and off by default; nothing else in Nexus depends on it.
Enable it with the `NEXUS_DGM` CMake option, e.g. in `.baldr.yaml`:

```yaml
cmake:
  definitions:
    NEXUS_DGM: "ON"
```

Then `baldr -p . -t dgm-tool -r -- res/sample.dgm -o sample.svg`.

## Example

```
// A request path, laid out by hand.
box client "Client"       at 0,0
box api    "API Gateway"  right-of client
box auth   "Auth"         below api gap 60   size 100x40
box db     "Database"     right-of api gap 120

arrow client -> api "HTTPS"
arrow api    -> auth
arrow api    -> db   "SQL"
```

## Syntax

One statement per line. Blank lines are ignored. Comments start with `//` and
run to the end of the line (a `//` inside a quoted label is not a comment).

Tokens are separated by whitespace. A label is a double-quoted string; `\"`
and `\\` are the only escapes.

### Box

```
box <id> ["label"] [<placement>] [size <W>x<H>]
```

| Part        | Meaning                                                     |
|-------------|-------------------------------------------------------------|
| `id`        | Unique name, used to refer to the box. `[A-Za-z0-9_-]+`.    |
| `"label"`   | Text shown in the box. Defaults to the id.                  |
| `placement` | Where the box goes; see below. Optional.                    |
| `size WxH`  | Box size in pixels, both positive. Default `120x50`.        |

`placement` and `size` may appear in either order, each at most once.

### Arrow

```
arrow <from> -> <to> ["label"]
```

Draws a straight arrow from the edge of box `from` to the edge of box `to`,
along the line between their centres. The optional label sits at the
midpoint. Arrows may refer to boxes declared later in the file.

## Positioning

Coordinates are in pixels. `x` grows to the right, `y` grows downwards, and a
box's position is its **top-left corner**. Negative coordinates are allowed;
the canvas grows to fit every box with a 20px margin.

| Placement                   | Result                                                        |
|-----------------------------|---------------------------------------------------------------|
| `at X,Y`                    | Top-left corner at `(X, Y)`. Integers, no spaces around `,`.   |
| `right-of <ref> [gap N]`    | `N` px to the right of `ref`, vertically centred on it.        |
| `left-of <ref> [gap N]`     | `N` px to the left of `ref`, vertically centred on it.         |
| `below <ref> [gap N]`       | `N` px below `ref`, horizontally centred on it.                |
| `above <ref> [gap N]`       | `N` px above `ref`, horizontally centred on it.                |
| *(none)*                    | `right-of` the previous box; the first box goes to `0,0`.      |

- The default `gap` is `40`. A gap may be `0` but not negative.
- `ref` must be a box declared **earlier** in the file. This keeps layout a
  single top-to-bottom pass and rules out cycles.
- Centring uses integer division, so odd size differences round towards the
  top/left.
- Overlapping boxes are allowed; DGM draws exactly what it is told.

## Errors

The tool stops at the first error and reports the line number:

| Situation                                | Message                                          |
|------------------------------------------|--------------------------------------------------|
| Unknown statement keyword                | `line N: unknown statement 'X'`                  |
| Missing or malformed box id              | `line N: invalid box id 'X'`                     |
| Same id declared twice                   | `line N: duplicate box id 'X'`                   |
| `at` value not `X,Y`                     | `line N: invalid position 'X', expected X,Y`     |
| `size` value not `WxH` with W, H > 0     | `line N: invalid size 'X', expected WxH`         |
| Bad or negative gap                      | `line N: invalid gap 'X'`                        |
| Relative placement to undeclared box     | `line N: unknown box 'X' (must be declared before use)` |
| Second placement or size on one box      | `line N: placement given more than once` / `size given more than once` |
| Arrow without `->`                       | `line N: expected '->' in arrow`                 |
| Arrow to an undeclared box               | `line N: arrow refers to unknown box 'X'`        |
| Arrow from a box to itself               | `line N: arrow from 'X' to itself is not supported` |
| Unterminated quoted label                | `line N: unterminated string`                    |
| Anything left over on a line             | `line N: unexpected token 'X'`                   |

## Pipeline

```
text --parse--> spec --layout--> diagram --to_svg--> SVG
```

- `dgm::parse` turns text into a `spec`: boxes with *unresolved* placements.
- `dgm::layout` resolves every placement to an absolute rectangle and clips
  arrows to box edges. This is the one place that decides positions, so a
  future automatic layout plugs in here without touching the parser or the
  renderer.
- `dgm::to_svg` writes plain SVG with no dependencies on fonts or scripts.

## Out of scope for the MVP

Styling (colours, shapes), routed or curved arrows, automatic layout, groups,
and text wrapping.
