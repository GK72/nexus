---
name: documenting
description: Write, restructure or review AsciiDoc documentation, rendered to HTML by Nexus `tools/doc.sh`. Use for manuals, guides, chapters and pages (`.adoc`), `[fig]` figures, interactive charts, the doc theme and its plugins, or when asked to document a feature. Not for code comments or README files.
---

# Documenting

The author's manuals are AsciiDoc, rendered by `~/repos/nexus/tools/doc.sh` into standalone HTML
with the Nexus theme and plugins. The full guide is the documenting book,
`~/repos/nexus/doc/documenting/` (`documenting.adoc` is the spine, `pages/` the chapters). Read the
page that covers the task before writing; this skill is the workflow and the traps.

## Workflow

1. **Find the book.** A book is a folder with one `.adoc` at its top (the spine, any name) and a
   `pages/` folder. The spine holds settings and `include::pages/...[leveloffset=+1]`; content goes
   into a page, never into the spine. A file that is only included is a partial, `_name.adoc`. A
   project may wrap `tools/doc.sh` (e.g. with `ADOC_CSS` and `-r` extensions): use the wrapper.
2. **Write prose first**, then reference tables. Every figure and table gets a `.Title` (numbered
   caption). Link to the page that owns a rule instead of repeating it: `link:page.html[]` within a
   book, `link:../other/index.html[]` to another.
3. **Render the book**: `tools/doc.sh doc/<book>` writes `build/doc/<spine>/index.html` and one
   `<page>.html` per page. Warnings and errors print to stderr; a clean run prints only the output
   paths. While writing with the user watching, `tools/doc.sh --serve doc/<book>` re-renders on
   every change and the open pages reload themselves.
4. **Look at it.** Render problems are visual: `tools/doc.sh --shot [--dark] build/doc/<spine>/<page>.html`
   writes a PNG beside the page (`--size WxH` for the window); read it. Check the dark theme too.
   Scripted parts (charts, studios) are in the screenshot; a missing one means a script failed.

## What to use

| Need | Use | Page |
|---|---|---|
| Box-and-arrow diagram | `[fig,name,WxH]` + `....` block, notation | `figures.adoc` |
| Drawing from formulas | `[fig,name,WxH,ruby]`, `f.rect/line/arrow/path/text/bracket/grid` | `figures.adoc` |
| Chart with inputs | `interactive::<model>[]` + companion `FILE.rb` and a JS model | `interactive.adoc` |
| Show markup and its result | `[demo]` around an open block | `writing.adoc` |
| Icon in text | `i:name[]`, `i:name[label="..."]` | `style.adoc` |
| Code from sources | `include::path[tag=x]` (plain AsciiDoc) | |
| Project-specific generated content | a plugin with `-r`, using `Nxs::Source` | `plugins.adoc` |

Colors in figures are roles (`:green`, `:muted`, ...), never values. Roles name hues, not
purposes. A page that renders alone wraps its settings in `ifeval::["{docname}" == "<page>"]`.

## Traps

- **Macros fire inside backticks.** `` `i:copy[]` `` renders an icon; write `` `+i:copy[]+` ``.
  Same for `*`: `` `st-*` (stroke), `t-*` `` turns bold; use `` `+st-*+` ``.
- **Adjacent lists merge.** Separate two lists with a `//-` line.
- **`include::` in a listing runs.** Escape it as `\include::` to show it.
- **Figure labels on steep diagonals** can sit on the line; lay boxes out so arrows run straight,
  or move the label with `@x,y`.
- **A companion belongs to the rendered file.** A page with an interactive block included into a
  spine needs the spine's companion to `require_relative` the page's one.
- **Documents outside the book layout** are passed as files (`tools/doc.sh doc/*.adoc`) and render
  flat into `build/doc/`; two with the same name in one run stop it.

## Changing Nexus itself

Plugins are `tools/asciidoc/plugins/*.rb` (each registers its processors; assets in a folder of
the same name), library code is `tools/asciidoc/lib/*.rb` (`Nxs::*`, registers nothing). Theme:
`res/adoc/tokens.css` (OKLCH seeds, fonts, sizes), `palette.css` (derived colors),
`styles/{plain,compact,full}.css`. After a change, render `doc/documenting` and the books of the
projects that use Nexus, and look at the specimens (`specimen-*.html`) in both themes.
