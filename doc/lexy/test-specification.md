# Test Specification

This document outlines the test cases for the `lexy` module.

## Test Suite: Descriptor

The `Descriptor` suite verifies the core functionality of loading message
descriptors from YAML strings and files.

### Test Case: TC-001 - Positive - Full Descriptor Loading

Verifies that a descriptor with all optional fields and various field types is
correctly parsed.

- **Input**: YAML string with `name`, `version` at top-level, and a `message`
  containing `name`, `id` (hex), and `fields` with `uint`, `bool`, and `string`
  (each with mandatory `type`).
- **Expectation**: All fields are correctly represented in the `lexy::descriptor`
  instance.

### Test Case: TC-002 - Positive - Hex and Decimal IDs

Verifies that message IDs can be specified in both decimal and hexadecimal formats
inside the `message` section.

- **Input 1**: YAML with `message: { id: 4660 }`.
- **Input 2**: YAML with `message: { id: 0x1234 }`.
- **Expectation**: Both result in the same numerical ID (4660) in the descriptor.

### Test Case: TC-003 - Negative - Missing Type

Verifies that a field without a `type` results in an error.

- **Input**: YAML with a field containing `name` and `length`, but no `type`.
- **Expectation**: `lexy::load_descriptor` returns a `nova::error` with a
  descriptive message ("Missing mandatory field: type").

### Test Case: TC-004 - Negative - Missing ID

Verifies that a descriptor missing the `id` in the `message` section results in
an error.

- **Input**: YAML with a `message` section containing `name` and `fields`, but no `id`.
- **Expectation**: `lexy::load_descriptor` returns a `nova::error` with a
  descriptive message ("Missing mandatory field: message:id").

### Test Case: TC-005 - Negative - Unknown Field Type

Verifies that an unknown field type results in an error.

- **Input**: YAML with a field of type `float`.
- **Expectation**: `lexy::load_descriptor` returns a `nova::error` with a
  descriptive message ("Unknown field type: float").

### Test Case: TC-006 - Negative - Unknown Length Type

Verifies that an unknown length type results in an error.

- **Input**: YAML with a field of `length_type: kilobyte`.
- **Expectation**: `lexy::load_descriptor` returns a `nova::error` with a
  descriptive message ("Unknown length type: kilobyte").

### Test Case: TC-007 - Negative - Invalid YAML

Verifies that malformed YAML content results in an error.

- **Input**: A string that is not a valid YAML.
- **Expectation**: `lexy::load_descriptor` returns a `nova::error` containing
  a YAML parsing error message.

### Test Case: TC-008 - Negative - Missing Name

Verifies that a descriptor missing the top-level `name` results in an error.

- **Input**: YAML with only `id`.
- **Expectation**: `lexy::load_descriptor` returns a `nova::error` with a
  descriptive message ("Missing mandatory field: name").

### Test Case: TC-009 - Negative - Invalid Reference

> Note: This is a technical debt. This should be tested via a sanity check feature.

Verifies that a descriptor with a non-existent length reference fails during
decoding.

- **Input**: YAML with a field `length` referring to a non-existent field.
- **Expectation**: `lexy::decode` returns a `nova::error` containing "Length
  reference not found".

### Test Case: TC-010 - Negative - Missing Message

Verifies that a descriptor missing the `message` section results in an error.

- **Input**: YAML with `name` but no `message`.
- **Expectation**: `lexy::load_descriptor` returns a `nova::error` with a
  descriptive message ("Missing mandatory field: message").
