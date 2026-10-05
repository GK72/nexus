# Test Specification

This document outlines the test cases for the `dgm` module, derived from
[spec.md](spec.md). Implementation: `dgm/dgm.test.cpp`.

## Test Suite: Dgm

### Positioning

| ID     | Type     | Title                               | Input                                                                 | Expectation                                                               |
|--------|----------|-------------------------------------|-----------------------------------------------------------------------|---------------------------------------------------------------------------|
| TC-001 | Positive | Absolute placement and defaults     | `box a at 10,20`                                                      | Rect `10,20 120x50`; label defaults to id.                                |
| TC-002 | Positive | `right-of` with default gap, centred | `a` 100x60 at origin; `b` 80x20 `right-of a`                          | `b` at `140,20`.                                                           |
| TC-003 | Positive | All directions with explicit gap    | `left-of gap 10`, `above gap 20`, `below gap 0` around a 100x50 box   | `-50,0`, `0,-50`, `20,50`.                                                 |
| TC-004 | Positive | Flow placement                      | `a`, `b`, `c below a`, `d`                                            | First at origin; `b` at `160,0`; `d` follows `c` at `160,90`.              |
| TC-005 | Positive | Centring rounds towards top/left    | Odd positive and odd negative height differences                       | `y = 4` and `y = -6`.                                                      |
| TC-006 | Positive | Negative coordinates, option order  | `box a size 30x40 at -15,-25`                                         | Rect `-15,-25 30x40`.                                                      |

### Syntax and arrows

| ID     | Type     | Title                               | Input                                                                 | Expectation                                                               |
|--------|----------|-------------------------------------|-----------------------------------------------------------------------|---------------------------------------------------------------------------|
| TC-007 | Positive | Comments, blank lines, escapes      | Comment lines, trailing comment, tab indent, `\"` and `//` in a label | Two boxes; label keeps the quote and the `//`.                             |
| TC-008 | Positive | Arrows clipped to edges             | Horizontal and 45° arrows between 100x50 boxes                        | Endpoints exactly on the box edges.                                        |
| TC-009 | Positive | Arrow forward reference             | Arrow before the boxes it connects                                    | Builds; label preserved.                                                   |
| TC-010 | Positive | SVG output                          | Two boxes, one labelled arrow, label with `&<>`                       | `viewBox` includes 20px margin; label XML-escaped; line and marker present. |

### Errors

Every negative case checks the exact error message, including the line number.

| ID     | Title                          | Input (abridged)                     | Message                                                 |
|--------|--------------------------------|--------------------------------------|---------------------------------------------------------|
| TC-011 | Unknown statement              | `circle b`                           | `line 2: unknown statement 'circle'`                    |
| TC-012 | Invalid box id                 | `box`, `box "a"`, `box a.b`          | `line 1: invalid box id '…'`                            |
| TC-013 | Duplicate id                   | `box a` twice                        | `line 2: duplicate box id 'a'`                          |
| TC-014 | Invalid position               | `at 10`, `at 10,y`, `at`             | `line 1: invalid position '…', expected X,Y`            |
| TC-015 | Invalid size                   | `size 0x10`, `size 10by10`           | `line 1: invalid size '…', expected WxH`                |
| TC-016 | Invalid gap                    | `gap -5`, `gap` with no value        | `line 2: invalid gap '…'`                               |
| TC-017 | Relative to undeclared box     | `box a right-of b` before `b`        | `line 1: unknown box 'b' (must be declared before use)` |
| TC-018 | Repeated option                | `at` + `below`; `size` twice         | `placement given more than once` / `size given more than once` |
| TC-019 | Arrow missing `->`             | `arrow a b`                          | `line 3: expected '->' in arrow`                        |
| TC-020 | Arrow to unknown box           | `arrow a -> z`                       | `line 2: arrow refers to unknown box 'z'`               |
| TC-021 | Self arrow                     | `arrow a -> a`                       | `line 2: arrow from 'a' to itself is not supported`     |
| TC-022 | Unterminated string            | `box a "oops`                        | `line 1: unterminated string`                           |
| TC-023 | Unexpected token               | Label after placement; trailing word | `line N: unexpected token '…'`                          |
