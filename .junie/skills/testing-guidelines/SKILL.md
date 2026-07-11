---
name: testing-guidelines
description: Quality assurance and general testing guidelines
---

# Testing Guidelines

Use this skill when writing or updating tests for any component to ensure
consistency and robustness across the project.

## Negative Test Scenarios

To ensure robust error handling and clear feedback for users, all negative test
scenarios MUST verify the content of the error message or error state returned
by the library or module.

- **Mandatory Error Verification**: Every test case that expects a failure (e.g.,
  `nova::expected` with `nova::error` or an exception) must check the error
  message string.
- **Message Clarity**: Error messages should be descriptive and help the user
  identify the exact cause and location of the error (e.g., "Missing mandatory
  field: type", "Unknown field type: float").
- **Exact Match or Substring**: Depending on the stability of the error message,
  use exact matches or substring searches. Substring searches are preferred
  for messages containing dynamic content or library-generated details (e.g.,
  YAML-CPP exceptions).
- **Consistency**: Maintain consistency between the implementation and the test
  expectations.

## Test Data and Traceability

- **Unique IDs**: Each test case must have a unique ID (e.g., `TC-001`). This
  helps in referencing tests in test specifications and communication.
- **Global Inputs**: For complex inputs (like YAML or long binary blobs), use
  global `constexpr` or static variables (e.g., `constexpr auto InputTC_<ID>`) to
  keep the test functions clean and readable.
- **Standard Formatting**: Prefer using the standard project formatting for test
  data to ensure it's easy to read and maintain.

## Documentation

- **Test Specification**: All major features should have a corresponding test
  specification (e.g., `doc/<component>/test-specification.md`) that derives
  test cases from the feature specification.
- **ID Traceability**: The IDs used in the test specification should match the
  IDs in the test implementation.
