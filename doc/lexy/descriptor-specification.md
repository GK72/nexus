# Lexy Descriptor Specification

## Overview

The Lexy Descriptor is a YAML-based schema definition that describes the
structure of binary messages. It allows the `btx` toolset to decode, format,
and generate binary data based on human-readable definitions.

## Descriptor Structure

A Lexy descriptor consists of general metadata and a specific message schema.

### Top-Level Metadata

- `name`: (Required) A human-readable name for the descriptor. Used for
  documentation and testing.
- `version`: (Optional) Version of the descriptor schema.
  - Note: Parsed into the `descriptor` struct but not currently used by the
    core logic.

### Message Schema (`message`)

The `message` object defines the content of the binary blob.

- `name`: (Required) The name of the message being described.
- `id`: (Required) A unique identifier for the message type. The ID must be
  provided in the protocol to be able to find the relevant descriptor. This
  field is always 2 bytes.
- `fields`: (Required) A list of field definitions, processed in order.

### Field Definition

Each field in the `fields` list can have the following properties:

- `name`: (Required) Name of the field. Used for identification and as a
  reference for other fields' lengths.
- `type`: (Required) The data type of the field.
  - `uint`: Unsigned integer. Supports bit-packed values.
  - `bool`: Boolean value (usually 1 bit).
  - `string`: ASCII/UTF-8 string (must be byte-aligned).
- `length`: (Required) The length of the field. Can be:
  - A fixed numeric value (e.g., `1`, `4`, `12`).
  - A reference to the name of a previously defined field (e.g., `name_length`).
- `length_type`: (Optional, default: `byte`) The unit for the `length` property.
  - `bit`: Bits.
  - `byte`: Bytes (8 bits).
- `encoding`: (Optional) Encoding information about the field type. See field types
  for more information.

**Note on Ignored Fields**: The current parser ignores the `description`,
`encoding`, and any other fields present in the YAML but not listed above.

## Field Type Details

### `uint`

Decodes an unsigned integer from the binary stream. Supports bit-packed values
(e.g., a 12-bit integer). If `length_type` is `byte`, the numeric length is
multiplied by 8 bits.

Encoding: little-endian by default. Big-endian is not yet supported.

### `bool`

Decodes a single bit as a boolean. The bit position is advanced by 1.

Encoding: Not applicable. It shall be ignored by the implementation.

### `string`

Decodes a sequence of bytes as a string.
- **Restriction**: String fields must be byte-aligned. If the current bit
  position is not a multiple of 8, an error shall be returned.
- **Length**: The length must resolve to a non-zero byte count.

Encoding: Only UTF-8 is supported.

## Reference Handling

The `length` of a field can refer to a previously parsed field by its `name`.
When a reference is used:
1. The referenced field must have been defined earlier in the `fields` list.
2. The value of the referenced field (treated as a number) is used as the
   length for the current field.
3. If the reference is not found or cannot be resolved to a numeric value, an
   error is returned during decoding/generation.
