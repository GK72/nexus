# BTX Format Specification

The BTX format is a human-readable representation of binary data, supporting
both hexadecimal and bit-level expressions. It is designed to be easy to write,
read, and process using standard command-line tools.

## Syntax Rules

### Hexadecimal Characters
  - Expressed using the `\xHH` syntax, where `H` is a hexadecimal digit (0-9,
    a-f, A-F).
  - Each `\xHH` sequence represents exactly one byte.

### Binary Bits
  - Expressed using the `\bBB...` syntax, where `B` is a bit (0 or 1).
  - Each `\b` token must represent exactly 8 bit positions.
  - If fewer than 8 bits are defined, the placeholder `_` must be used to
    fill the remaining positions.
  - Bits are accumulated across any number of `\b` tokens.
  - Every sequence of 8 bits forms one byte.
  - This allows for bit-packing, where multiple fields are defined on different
    lines or within different tokens.

### Whitespace and Separators

  - Whitespace (spaces, tabs, newlines) and the separator `'` (e.g.,
    `\b0000'1111`) are only used for readability and they are ignored during
    processing.

  - The `_` character must be used for padding (e.g., `\b101_'____`).

### Comments
  - Comments start with `//` and extend to the end of the line.
  - Comments are ignored during processing.
