# C++ Coding Guidelines

This document outlines the coding standards and best practices for C++
development in the Nexus project. We focus on Modern C++ (C++20/C++23) to
ensure performance, safety, and maintainability.

## 1. Language Standard

- **Target Standard**: We target **C++20** for Clang and **C++23** for other
  compilers.
- **Extensions**: Do not use compiler-specific extensions
  (`CMAKE_CXX_EXTENSIONS` is `OFF`).
- **New Features**: Leverage modern features like Concepts, Coroutines, Ranges
  (C++20), and `std::expected` (C++23).

## 2. Safety and Resource Management

- **RAII**: Always use Resource Acquisition Is Initialization for resource
  management (memory, file handles, locks).
- **Smart Pointers**:
    - Use `std::unique_ptr` for exclusive ownership.
    - Use `std::shared_ptr` only when shared ownership is truly necessary.
    - Avoid `new` and `delete`; use `std::make_unique` and `std::make_shared`.
- **Initialization**: Always initialize variables; avoid uninitialized state.
- **Raw Pointers and References**:
    - Use raw pointers (`T*`) only for optional, non-owning observers.
    - Use references (`T&`) for mandatory, non-owning observers.
    - Prefer `std::span` for passing contiguous sequences of data.
- **Bounds Checking**: Prefer `std::vector::at()` or `std::span` over raw array
  access where safety is a concern.

## 3. Performance and Optimization

- **Pass by Value vs. Reference**:
    - Pass small, cheap-to-copy types (e.g., `int`, `double`,
      `std::string_view`) by value.
    - Pass larger types by `const T&`.
- **String View**: Use `std::string_view` for read-only string parameters to
  avoid unnecessary allocations.
- **Constexpr**: Use `constexpr` and `consteval` for values and functions that
  can be evaluated at compile time.
- **Move Semantics**: Properly implement and use move constructors and move
  assignment operators to avoid expensive copies. Use `std::move` when
  transferring ownership.

## 4. Error Handling

- **Exceptions**: Use exceptions for truly exceptional circumstances that
  cannot be handled locally.
- **Error Codes**: For expected errors in performance-critical or low-level
  code, prefer `std::expected` (C++23) or `std::optional`.

## 5. Coding Style

- **Naming Conventions**:
    - `snake_case` for functions, variables, classes, enums, and namespaces.
    - `snake_case` for enum values.
    - `m_` prefix for private member variables.
    - `CamelCase` for constants.
- **Trailing Return Types**: Prefer trailing return types for functions (`auto f() -> T`).
- **Logical Operators**: Use `not`, `and`, `or` instead of `!`, `&&`, `||`.
- **Namespaces and Aliases**:
    - Avoid `using` statements (e.g., `using nova::expected;`) in headers.
    - Prefer fully qualified names for external library types (e.g., `nova::expected`, `std::string_view`) to maintain clarity.
    - Use type aliases (`using`) for complex types and provide Doxygen-style documentation for them.
- **Include Directives**:
    - Use angle brackets (`<>`) for all project headers to allow for canonical inclusion (e.g., `#include <libbtx/descriptor.hpp>`).
    - Organize include directives in the following order:
        1. Project headers.
        2. Third-party library headers (e.g., `<yaml-cpp/yaml.h>`, `<libnova/log.hpp>`).
        3. Standard library headers.
    - Group different categories of headers with a blank line.
- **Constants**: Use `const` or `constexpr` by default for all variables that
  do not need to change.
- **Auto**: Use `auto` for complex types (like iterators or lambdas) or when
  the type is obvious from the initialization (e.g., `auto p =
  std::make_unique<T>();`).
- **Standard Types**: Prefer `std::size_t` over `size_t`. Use the `std::` prefix for types from `<cstdint>` (e.g., `std::uint64_t`, `std::uint32_t`).
- **Control Structures**:
    - Always use braces `{}` for `if`, `while`, `for`, and `do-while` statements, even when the body contains only a single statement.
    - Avoid variable shadowing (e.g., do not use the same name for a loop variable and a local variable or member).
- **Initialization**: Use uniform initialization `{ }` with spaces to avoid the "most vexing
  parse" and to prevent narrowing conversions.
- **Functions**: Factor out input/output handling and logic into smaller, testable functions.
- **Constants and Magic Numbers**: Replace magic numbers with named constants.

## 6. Tooling and Quality Assurance

- **Sanitizers**: Regularly run with AddressSanitizer (ASan) and
  UndefinedBehaviorSanitizer (UBSan). Use `SANITIZERS=asan` in CMake.
- **Warnings**: The project enforces strict warnings (`project_warnings`).
  Ensure your code compiles without warnings.
- **Formatting**: Use `clang-format` to maintain consistent code style.
- **Static Analysis**: Use `clang-tidy` for deeper code analysis.
- **Randomness**: Use `nova::random()` for generating random numbers and strings.
- **Automated Verification**: When performing complex validations or testing, create reproduction scripts in the `./tests` directory. 
- **Artifact Management**: All test artifacts and temporary files must be stored in `./.tmp/test-outputs`. Avoid littering the repository with generated files. Group artifacts in unique subdirectories (e.g., by timestamp) to avoid overwriting previous runs. Do NOT clean up these artifacts automatically; the user will handle cleanup manually.

## 7. Commits and Documentation

- **Conventional Commits**: Follow the [Conventional
  Commits](https://www.conventionalcommits.org/) specification for all commit
  messages.
- **Comments**: Write clear, concise comments. Focus on *why* something is
  done, rather than *how* (the code should be self-documenting as much as
  possible). Avoid inline code comments; prefer descriptive Doxygen-style
  comments.
- **In-code Documentation**: Use Doxygen-style comments for ALL functions, classes, and public APIs.
- **CLI Tools**:
    - **Main Structure**: Use `NOVA_MAIN` and `entrypoint` from `libnova/main.hpp` for all CLI tools.
    - **Argument Parsing**: Use `boost::program_options` for command-line argument parsing.
    - **Logging**: Use `nova::log` for program output and error reporting. Use `trace` level for detailed execution logs (e.g., parsing steps, field values) and `info` for high-level progress messages.
