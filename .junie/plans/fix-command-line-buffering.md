---
sessionId: session-260712-104512-1prm
---

# Requirements

### Overview & Goals
Some subprocess logs break the rolling-window progress display. Investigation shows the root cause is in `baldr::command` (`baldr-cpp/baldr/command.hpp`): its `poll()` method reads raw fixed-size chunks (up to 4096 bytes) directly from the child's stdout/stderr pipe and returns them as-is, without splitting on newline boundaries. Every caller (`Builder::configure`, `Builder::build`, `stream_command` in `main.cpp`) treats each `poll()` return value as a single logical line and forwards it verbatim to `nova::log::info(...)`.

Importantly, `command::poll()` itself must stay binary-safe: it makes no assumption about the byte stream being text, so it must **not** be modified to split on `\n` internally. Instead, a new reusable, opt-in line-reassembly layer is added entirely within `baldr-cpp` (`baldr-cpp/baldr/line_reader.hpp`), mirroring `nova::line_parser`'s `Callable`-storage convention without touching `libnova`/Nova at all, and only the text-oriented call sites (`Builder::configure`, `Builder::build`, `main.cpp::stream_command`) adopt it.

The rolling-window sink (`baldr::log::progress_sink` in `log.cpp`, backed by `baldr::progress` in `progress.cpp`) assumes **one log record == exactly one terminal line**: `progress::msg()` pushes the message into a fixed-size ring buffer and moves the cursor up by `m_buffer.size()` lines to redraw. If a `poll()` chunk contains embedded `\n` characters, or a single output line is split across two separate `poll()`/`read()` calls (chunk boundary cutting a line in half), the one-record-one-line invariant is violated: multi-line chunks print more terminal lines than the cursor arithmetic accounts for, and split lines appear as bogus partial/duplicate entries in the rolling window. Over time this desyncs the cursor position and corrupts the in-place progress rendering.

### Scope
**In Scope**
- Add a new `baldr::line_reader<Callable>` utility to a new file `baldr-cpp/baldr/line_reader.hpp` that reassembles complete lines from raw chunks fed to it incrementally, without assuming the source is text-only until fed.
- Update the text-oriented `command::poll()` call sites (`Builder::configure`, `Builder::build`, `main.cpp::stream_command`) to wrap their polling loop with a `baldr::line_reader` instance.
- Preserve `command::poll()`'s existing raw, binary-safe, unmodified behavior.

**Out of Scope**
- Any modification to `baldr::command` / `command.hpp` (including `poll()`, `m_buffer`, or its public contract) — it stays exactly as-is.
- Any modification to `libnova`/`deps/nova-cpp` whatsoever — the new utility must be self-contained inside `baldr-cpp`, even though it mirrors `nova::line_parser`'s style.
- Changes to `progress::msg/success/failure` cursor arithmetic itself (it is correct as long as the one-record-one-line invariant holds).
- Changes to spdlog sink/pattern formatting.
- Any change to `interactive` mode (no pipe redirection there).

### Functional Requirements
- `baldr::line_reader::feed(chunk)` must invoke its stored callback once per complete line found (delimiter stripped), and never split a chunk in a way that emits a partial line early.
- A line split across two separate `feed()` calls (i.e. across two `command::poll()` reads) must be reassembled and dispatched to the callback only once, in full.
- `baldr::line_reader::feed_eof()` must flush any trailing partial line still buffered (no trailing `\n`) exactly once, and be a no-op on subsequent calls.
- `command::poll()` remains completely unchanged: still binary-safe, still returns raw chunks with no newline-splitting assumption.
- Updated callers (`Builder::configure`, `Builder::build`, `main.cpp::stream_command`) continue to log one line per record, now via `baldr::line_reader`, with no change to `command`'s public contract.
- No file under `deps/nova-cpp` may be modified as part of this change.

# Technical Design

### Current Implementation
`baldr-cpp/baldr/command.hpp`'s `poll()`:
```cpp
auto poll() -> std::string {
    if (m_interactive) return {};
    ssize_t n = ::read(m_pipe.read(), m_buffer.data(), m_buffer.size());
    if (n <= 0) return {};
    return { m_buffer.data(), static_cast<std::size_t>(n) };
}
```
This returns whatever raw bytes arrive in one `read()`, deliberately with no relation to line boundaries — it must stay this way since `command` cannot assume its subprocess output is text.

Consumers (`builder.cpp` lines 43-46, 84-87; `main.cpp` `stream_command` lines 255-258) currently do:
```cpp
std::string line;
while (line = cmd.poll(), !line.empty()) {
    nova::log::info("{}", line);
}
```
each `poll()` chunk becomes one `nova::log::info` call, i.e. one entry in `progress::msg`'s ring buffer (`baldr-cpp/baldr/progress.cpp`), which assumes 1 entry == 1 terminal line — broken whenever a chunk contains embedded `\n` or a line straddles two chunks.

`deps/nova-cpp/libnova/libnova/io.hpp` already has `nova::line_parser<Callable>` (a `Callable` member invoked once per line), but it operates on a whole `std::istream` via blocking `std::getline` until EOF — not suited for incremental, non-blocking chunk-by-chunk consumption of a live pipe. Since Nova must not be touched by this change, its `Callable`-storage convention is mirrored (not reused/inherited) in a brand-new, self-contained type inside `baldr-cpp`.

### Key Decisions
- **`command`/`command.hpp` is never modified.** `poll()` keeps its exact raw, binary-safe, chunk-based contract. The text/line concern is layered entirely outside `command`.
- **Nova (`deps/nova-cpp`) is never modified.** Even though the new utility resembles `nova::line_parser` in spirit, it is added as a new, independent header inside `baldr-cpp`, so no file under `deps/nova-cpp` changes.
- **New reusable primitive lives in `baldr-cpp/baldr/line_reader.hpp`**, not in `libnova`, so line reassembly stays local to the `baldr` module and Nova remains untouched. It follows `nova::line_parser`'s `Callable` constructor-injection convention only as a style reference.
- **Callback-driven `feed()`/`feed_eof()` API** (not a caller-driven double loop, not an internal pull loop over `command`): the reader stores a `Callable` once and invokes it per completed line, collapsing the call site to a single flat loop while keeping `line_reader` decoupled from `command` entirely (it only sees `std::string_view` chunks).

### Proposed Changes
- Add a new header `baldr-cpp/baldr/line_reader.hpp` defining `baldr::line_reader<Callable>`, styled after `nova::line_parser` but fully self-contained within `baldr-cpp`:
  - Constructor stores the `Callable callback` (invoked as `callback(std::string&)` per line), plus an internal `std::string m_buf` accumulation buffer.
  - `feed(std::string_view chunk)`: appends `chunk` to `m_buf`, then repeatedly finds `\n`, extracts/dispatches the line (stripped of `\n`) to the callback, and erases it from `m_buf`, until no more `\n` remain.
  - `feed_eof()`: if `m_buf` is non-empty, dispatches it to the callback once as the final partial line, then clears it; subsequent calls are no-ops.
- Update `Builder::configure`, `Builder::build` (`baldr-cpp/baldr/builder.cpp`) and `stream_command` (`baldr-cpp/baldr/main.cpp`) to `#include <baldr/line_reader.hpp>`, construct a `baldr::line_reader` wrapping a lambda that calls `nova::log::info("{}", line)`, feed it each `cmd.poll()` chunk, and call `feed_eof()` after the polling loop ends.
- `command.hpp` remains byte-for-byte unchanged; `\r` stripping continues to be handled by the existing sink code in `log.cpp`'s `sink_it_`, unaffected by this change.
- No files under `deps/nova-cpp` are added, removed, or edited.

### Data Models / Contracts
```cpp
// baldr-cpp/baldr/line_reader.hpp
namespace baldr {

template <typename Callable>
    requires std::invocable<Callable, std::string&>
class line_reader {
public:
    explicit line_reader(Callable callback) : m_callback(std::move(callback)) {}
    void feed(std::string_view chunk);   // dispatches callback once per completed line
    void feed_eof();                     // flushes trailing partial line once, if any
private:
    Callable m_callback;
    std::string m_buf;
};

} // namespace baldr
```
Updated call-site pattern:
```cpp
baldr::line_reader lines([](std::string& line) { nova::log::info("{}", line); });
std::string chunk;
while (chunk = cmd.poll(), !chunk.empty()) {
    lines.feed(chunk);
}
lines.feed_eof();
```

### Risks
- **Multiple lines per chunk**: `feed()`'s internal loop must correctly dispatch all complete lines found within a single chunk, not just the first, to avoid dropping log lines.
- **Trailing output without newline**: `feed_eof()` must dispatch the leftover exactly once (guarded by clearing `m_buf` after dispatch), and only when non-empty.
- **Binary safety preserved**: since `command::poll()` is untouched, any existing or future binary/raw consumer of `command` is entirely unaffected by this change — only call sites that explicitly construct a `line_reader` opt into line semantics.
- **Nova stays untouched**: since the new type is duplicated locally rather than added to `libnova`, `baldr::line_reader` and `nova::line_parser` will diverge over time if similar logic is later needed elsewhere; acceptable trade-off given the explicit constraint to avoid modifying Nova.

### Follow-up: Extract testable tty.hpp utilities
The ANSI/terminal helpers added while fixing the rolling window (`tty::erase_to_end`, `tty::terminal_width`, and the anonymous-namespace helpers `visible_width`/`cap_visible_width` in `progress.cpp`) are currently untested and partly embedded as free functions/private helpers split across `progress.hpp`/`progress.cpp`. They are extracted into a new `baldr-cpp/baldr/tty.hpp`/`tty.cpp` pair (renamed from the originally proposed `ansi.hpp`/`ansi.cpp`, since the module covers terminal/TTY control in general, not solely ANSI escape sequences), broken into small single-purpose functions, documented with Doxygen comments (no inline `//` comments), and unit-tested with GTest following the same `line_reader.test.cpp` convention:
- `tty::visible_width(line)` and `tty::cap_visible_width(line, max_width)` move out of `progress.cpp`'s anonymous namespace into `baldr::tty` (public, so they're independently testable) — `progress.cpp` calls the `tty::` versions instead of local helpers.
- The ANSI escape-sequence scanning logic shared by both (finding/skipping a CSI sequence starting at a given index) is factored into a small private helper (e.g. `tty::detail::skip_csi_sequence(line, i)`) used by both, removing duplication and each closure gets its own doc comment instead of inline comments.
- `tty::terminal_width()` and `tty::erase_to_end()`/`tty::cursor_up()`/`tty::cursor_down()`/`tty::clear_line()` move (unchanged in behavior) from `progress.hpp`/`progress.cpp` into the new `tty.hpp`/`tty.cpp`, with `progress.hpp` including `<baldr/tty.hpp>` instead of declaring them itself.
- All existing `//`-style inline comments in the moved/refactored code (e.g. the CSI-skip walk, the truncation budget logic) are replaced by Doxygen `/** ... */` documentation on each function, per the repo's `AGENTS.md` style guideline.

### Follow-up: Cap rolling-window line width instead of counting wrapped rows
A second corruption source was found and fixed post-implementation: `progress::msg()`/`success()`/`failure()` in `progress.cpp` originally moved the cursor by counting buffered *messages*, not actual terminal *rows*; any log line longer than the terminal width wraps onto multiple rows and desyncs the cursor. This was first fixed with `tty::erase_to_end()` (`\x1b[0J`) plus a `rows_for()`/`visible_width()` row-counting scheme in `progress.cpp` and `progress.hpp` (`tty::terminal_width()` via `ioctl(TIOCGWINSZ)`).

The row-counting fix is correct but lets the rolling window occupy many more than `m_max_lines` terminal rows when lines are long (e.g. real `conan`/CMake output up to 285 chars in an 80-column terminal), hurting the compact live-status UX. The agreed follow-up:

> Note: `visible_width`/`cap_visible_width` described below now live in `baldr::tty` (`tty.hpp`/`tty.cpp`) per the "Extract testable tty.hpp utilities" follow-up above, rather than staying as private helpers in `progress.cpp`'s anonymous namespace.
- **Truncate only for the rolling window** (`progress::msg()`), not for `failure()`: `failure()` prints permanently (not part of the redrawn window) and must keep the full, untruncated line since it's the actual diagnostic the user needs to read.
- Add a `cap_visible_width(line, max_width)` helper (in the anonymous namespace next to `visible_width()` in `progress.cpp`) that measures visible (ANSI-stripped) width and, if it exceeds `max_width`, truncates the *visible* characters and appends a `"..."` marker — while preserving any trailing ANSI reset/color sequences already present in `line` so color state isn't corrupted mid-window.
- Apply `cap_visible_width(line, tty::terminal_width() - 1)` to each message only in `progress::msg()`'s render loop (right before `fmt::println`/`tui_debug`), so every buffered line is guaranteed to occupy exactly one terminal row again.
- Since capped lines are always exactly one row, revert the `rows_for()` summation in `progress::msg()` back to `tty::cursor_up(m_buffer.size())` (dropping the need for `rows_for()`/row summing entirely); keep `visible_width()` since `cap_visible_width()` still needs it.
- Keep `tty::erase_to_end()` in `msg()`/`success()`/`failure()` regardless of the length cap — it remains a cheap, defensive redraw-correctness measure (e.g. if `m_max_lines` shrinks between frames) independent of line-length capping.
- `success()`/`failure()` are otherwise unaffected: `success()` doesn't print buffered lines, and `failure()` continues to print the full, uncapped `m_buffer` content.

# Testing

### Validation Approach
The repo's existing unit-testing pattern (used by `libbtx`, `libnxs`, `liblexy`) is GoogleTest with a `<name>.test.cpp` file glob-collected by a per-module `test.cmake` (`add_test_target(MODULE_NAME)` function, see `libbtx/test.cmake` / `libbtx/CMakeLists.txt`), wired up via `if(BUILD_TESTING)` + `find_package(GTest REQUIRED)` + `include(GoogleTest)` + `gtest_discover_tests(... DISCOVERY_MODE PRE_TEST)`. `baldr-cpp/baldr` currently has no test target at all (only `baldr/CMakeLists.txt` building the `baldr` executable), so this change introduces the first one, following the exact same convention as `libbtx`.

### Key Scenarios (for `line_reader::feed`/`feed_eof`)
- `TEST(line_reader, SingleCompleteLine)`: one `feed("hello\n")` call dispatches exactly one callback invocation with `"hello"`.
- `TEST(line_reader, MultipleLinesInOneChunk)`: `feed("a\nb\nc\n")` dispatches three callback invocations, in order, `"a"`, `"b"`, `"c"`.
- `TEST(line_reader, LineSplitAcrossTwoFeeds)`: `feed("hel")` then `feed("lo\n")` dispatches exactly one callback invocation with `"hello"` (not two partial ones).
- `TEST(line_reader, NoNewlineYieldsNoDispatch)`: `feed("partial")` alone (no `feed_eof()`) dispatches zero callback invocations.
- `TEST(line_reader, FeedEofFlushesTrailingPartialLine)`: `feed("partial")` followed by `feed_eof()` dispatches exactly one callback invocation with `"partial"`.
- `TEST(line_reader, FeedEofIsNoOpWhenBufferEmpty)`: `feed("line\n")` (already dispatched) followed by `feed_eof()` triggers no additional callback invocation.
- `TEST(line_reader, FeedEofIsNoOpOnSecondCall)`: `feed("partial")`, `feed_eof()`, then a second `feed_eof()` call only dispatches once in total.
- `TEST(line_reader, EmptyLinesArePreserved)`: `feed("a\n\nb\n")` dispatches three lines, the middle one empty (`""`), confirming empty lines aren't silently dropped.

### Edge Cases
- Chunk containing only `"\n"` with nothing buffered before/after must dispatch a single empty-string line, not be skipped.
- Binary-ish bytes with no `\n` at all, fed then flushed via `feed_eof()`, must be returned byte-for-byte unmodified (no `\r` stripping, no truncation) — `line_reader` only recognizes `\n` as a delimiter, consistent with the design decision that `\r` handling stays in `log.cpp`'s sink.
- Repeated `feed()` calls with empty chunks (`feed("")`) must be safe no-ops that don't corrupt buffered state.

### Key Scenarios (for `tty::visible_width`/`tty::cap_visible_width`/`tty::terminal_width`)
- `TEST(tty, VisibleWidthPlainText)`: `visible_width("hello")` returns `5`.
- `TEST(tty, VisibleWidthStripsSingleAnsiSequence)`: `visible_width("\x1b[1;34mhello")` returns `5` (escape sequence excluded).
- `TEST(tty, VisibleWidthStripsMultipleAnsiSequences)`: `visible_width("\x1b[31mred\x1b[0m")` returns `3`.
- `TEST(tty, VisibleWidthEmptyString)`: `visible_width("")` returns `0`.
- `TEST(tty, CapVisibleWidthUnderBudgetReturnsUnchanged)`: `cap_visible_width("short", 10)` returns `"short"` unmodified.
- `TEST(tty, CapVisibleWidthTruncatesWithEllipsis)`: `cap_visible_width("abcdefghij", 5)` returns a string with visible width `5` ending in `"..."`.
- `TEST(tty, CapVisibleWidthPreservesTrailingAnsiReset)`: a colored line longer than budget keeps its trailing `\x1b[0m`/reset sequence after the `"..."` marker.
- `TEST(tty, CapVisibleWidthExactBudgetNotTruncated)`: a line whose visible width exactly equals `max_width` is returned unchanged (no `"..."` appended).
- `TEST(tty, TerminalWidthFallsBackWhenNotATty)`: `terminal_width()` returns a positive value (falls back to `80`) when stdout is not a TTY (e.g. piped in the test process).

### Edge Cases (tty)
- A CSI sequence that is truncated/malformed (starts with `\x1b[` but never terminates) must not cause an out-of-bounds read in `visible_width`/`cap_visible_width`.
- `cap_visible_width(line, 0)` must not crash and should still append `"..."` alone or return an empty result gracefully.
- A line containing only ANSI sequences and no visible characters has `visible_width` of `0` and is never truncated.

### Test Changes
- Add `baldr-cpp/baldr/line_reader.test.cpp` containing the `TEST(line_reader, ...)` cases above, using a small test-local capture (e.g. `std::vector<std::string> received` filled by a lambda passed into `baldr::line_reader`) to assert dispatched lines and counts via `EXPECT_EQ`/`ASSERT_EQ`.
- Add `baldr-cpp/baldr/tty.test.cpp` containing the `TEST(tty, ...)` cases above, exercising the extracted `baldr::tty::visible_width`/`cap_visible_width`/`terminal_width` functions directly (no `progress`/rendering involved).
- Add `baldr-cpp/baldr/test.cmake` mirroring `libbtx/test.cmake`'s `add_test_target(MODULE_NAME)` function (glob `*.test.cpp`, link against `nova::nova`, `GTest::gtest_main`, `GTest::gtest`, `GTest::gmock`, call `gtest_discover_tests(... DISCOVERY_MODE PRE_TEST)`), adapted to link whatever target/sources are needed (header-only for `line_reader.hpp`; `tty.cpp` compiled into the test binary/`baldr` sources for `tty`).
- Update `baldr-cpp/baldr/CMakeLists.txt` to `include(test.cmake)` and, under `if(BUILD_TESTING)` with `find_package(GTest REQUIRED)` + `include(GoogleTest)`, call `add_test_target(baldr)`, matching the `libnxs`/`libbtx` convention.
- No separate test-specification document is added for `tty`; its `tty.test.cpp` GTest cases are considered sufficient documentation of expected behavior, avoiding duplication.

### Regression Test (end-to-end, both fixed bugs)
A single functional regression test, `baldr-cpp/baldr/regression.test.cpp`, exercises the real pipeline together (`baldr::command` real subprocess → `baldr::line_reader` → `baldr::progress::msg`) rather than the individual units in isolation, so it fails again if either fixed bug regresses:

- `TEST(regression, SplitAndOversizedSubprocessLinesRenderOneRowPerRecord)`:
  1. Spawns a real child process via `baldr::command` (e.g. `sh -c '...'`) that writes, deliberately: two log lines inside a single write (previously mis-split), one line emitted across two separate writes with a short sleep in between (previously reassembled incorrectly), and one line far longer than a typical 80-column terminal width (previously desynced the cursor via multi-row wrapping).
  2. Drains `command::poll()` through a `baldr::line_reader` into a `std::vector<std::string>`, asserting the exact reassembled line count/content — regression coverage for the line-splitting bug (Steps 1–3), independent of `command::poll()`'s own binary-safe chunking.
  3. Feeds each reassembled line into a `baldr::progress` instance (`lines(N)`) with stdout captured to a pipe/temp file, and asserts each `msg()` call emits exactly `N` (buffered-line-count) printed rows with a single `cursor_up(N)` — regression coverage for the row-counting/cursor-desync bug (Step 4), confirming the oversized line is truncated with a trailing `"..."` and never wraps.
- Since `baldr::progress::msg()` needs compiling for this test, `baldr-cpp/baldr/test.cmake` additionally compiles `progress.cpp` into the `test-baldr` binary (alongside the existing `tty.cpp`).

### External Shell Regression Script (human-runnable demo)
On top of the GTest regression case, a new shell script `tests/rolling-window.sh` is added following the exact conventions of the existing `tests/*.sh` scripts (`stdout.sh`, `btx.sh`, etc.) and wired into `tests/run.sh`. Its purpose is twofold: (1) act as an automated regression check exercising the real, compiled `baldr` binary end-to-end (not just the unit-level GTest), and (2) double as a manual, narrated demo of the fix for a human running it directly in a terminal.

- Builds `baldr` itself first (mirroring `stdout.sh`'s `baldr -p "$PROJECT_ROOT" ... -t <target>` bootstrap pattern), then locates the built `baldr` binary under `build/<config>/baldr-cpp/baldr/baldr`.
- Drives `baldr run -- <shell scenario>` (the existing `run` subcommand already wired to `stream_command` in `main.cpp`) with small inline `bash -c '...'` scenarios that reproduce the two originally-reported bugs: (a) two log lines written in a single `printf` call, (b) one line written across two separate `printf` calls separated by a short `sleep`, and (c) one line deliberately longer than a typical terminal width (e.g. 200 chars) to exercise the truncation/cap path.
- Wraps each `baldr run -- ...` invocation with `script -qc "..." <logfile>` (pseudo-TTY) so `baldr::progress` takes its interactive/rolling-window code path (matching how a human actually runs `baldr build` in a real terminal), then strips ANSI escape sequences from the captured log with `sed`/a small regex to get the plain rendered text for assertions, while keeping the raw escaped log around as an artifact for manual inspection.
- Reuses the same `run_test`/`run_text_test`-style helper-function pattern from `stdout.sh` (`echo -n "Running test: $name... "`, `PASSED`/`FAILED` counters, a `FAILED_TESTS` array, and a final summary block with a non-zero exit code on failure), so its console output and pass/fail semantics are indistinguishable from the rest of the suite when run via `tests/run.sh`.
- For manual/demo runs, each scenario prints a short human-readable description of *what bug it demonstrates* before running it (e.g. "Scenario: two lines emitted in a single subprocess write...") and echoes the raw captured terminal bytes back out afterward (via `cat -v` or similar) so a person watching the script run can visually see the rolling window render each scenario correctly, not just a PASSED/FAILED line.
- Assertions per scenario: the plain-text log contains exactly the expected reassembled lines (no embedded/duplicated partial lines) for scenarios (a)/(b); the oversized line in scenario (c) appears truncated with a trailing `"..."` and never causes a second wrapped row in the raw escaped log.

# Delivery Steps

### ✓ Step 1: Add baldr::line_reader to a new baldr-cpp header
`baldr::line_reader<Callable>` reassembles complete lines from raw chunks fed incrementally and dispatches each to a stored callback, without any assumption that `command` or its output is text-only, and without touching Nova.

- Create a new file `baldr-cpp/baldr/line_reader.hpp` defining `template <typename Callable> class line_reader` in the `baldr` namespace, styled (not shared) after `nova::line_parser`'s `Callable`-storage convention.
- Constructor stores the callback and an internal `std::string m_buf`.
- Implement `feed(std::string_view chunk)`: appends to `m_buf`, then loops extracting and dispatching every complete `\n`-delimited line currently buffered.
- Implement `feed_eof()`: dispatches and clears any remaining partial line in `m_buf` exactly once; subsequent calls are no-ops.
- Do not modify `baldr-cpp/baldr/command.hpp` in any way — `command::poll()` stays raw and binary-safe.
- Do not modify any file under `deps/nova-cpp`.

### ✓ Step 2: Add GTest unit tests for line_reader::feed and feed_eof
`baldr-cpp/baldr` gains its first unit-test target, exercising `line_reader::feed`/`feed_eof` following the repo's existing `<name>.test.cpp` + `test.cmake` GoogleTest convention (as seen in `libbtx`/`libnxs`).

- Create `baldr-cpp/baldr/test.cmake` with an `add_test_target(baldr)` function mirroring `libbtx/test.cmake` (glob `*.test.cpp`, link `nova::nova`, `GTest::gtest_main`, `GTest::gtest`, `GTest::gmock`, `gtest_discover_tests(... DISCOVERY_MODE PRE_TEST)`).
- Update `baldr-cpp/baldr/CMakeLists.txt` to `include(test.cmake)` and, guarded by `if(BUILD_TESTING)` with `find_package(GTest REQUIRED)` + `include(GoogleTest)`, call `add_test_target(baldr)`.
- Add `baldr-cpp/baldr/line_reader.test.cpp` with `TEST(line_reader, ...)` cases covering: a single complete line, multiple lines in one chunk, a line split across two `feed()` calls, no dispatch on a partial line without `feed_eof()`, `feed_eof()` flushing a trailing partial line exactly once, `feed_eof()` being a no-op when the buffer is already empty or called twice, and empty lines being preserved (`\n\n`).
- Each test constructs a `baldr::line_reader` around a lambda capturing a local `std::vector<std::string>`, feeds it string chunks, and asserts the captured vector's contents/order via `EXPECT_EQ`/`ASSERT_EQ`, matching the assertion style used in `libbtx/btx.test.cpp`.

### ✓ Step 3: Adopt baldr::line_reader at text-oriented command::poll() call sites
`Builder::configure`, `Builder::build`, and `main.cpp::stream_command` now log one complete line per record via `baldr::line_reader`, fixing the rolling-window corruption without touching `command` or Nova.

- Update `baldr-cpp/baldr/builder.cpp` (`configure`, `build`) to `#include <baldr/line_reader.hpp>` and construct a `baldr::line_reader` around a lambda calling `nova::log::info("{}", line)`, feed it each `cmd.poll()` chunk in the existing polling loop, and call `feed_eof()` once the loop ends.
- Apply the same change to `stream_command` in `baldr-cpp/baldr/main.cpp`.
- Manually trace through `baldr::log::progress_sink::sink_it_` (log.cpp) and `baldr::progress::msg` (progress.cpp) to confirm the one-record-one-terminal-line invariant now holds for subprocess text output.
- Build a small subprocess test (e.g. `printf`/`yes`-style commands producing partial/multi-line/no-trailing-newline output through `command`) to validate cross-chunk reassembly and EOF flush behavior end-to-end, while confirming `command::poll()` and all `deps/nova-cpp` files remain unchanged.

### ✓ Step 4: Cap rolling-window line width instead of counting wrapped rows
`progress::msg()` truncates each buffered line to fit exactly one terminal row (visible-width aware, `"..."` suffix on overflow), while `failure()` keeps printing full, untruncated lines.

- Add a `cap_visible_width(line, max_width)` helper next to `visible_width()` in the anonymous namespace of `baldr-cpp/baldr/progress.cpp`: strips ANSI SGR sequences to measure visible width, and if it exceeds `max_width`, truncates the visible characters and appends `"..."`, preserving any trailing ANSI reset/color codes already present in the line.
- In `progress::msg()`'s render loop, apply `cap_visible_width(line, tty::terminal_width() - 1)` to each buffered message right before printing/`tui_debug`, so every rendered line occupies exactly one terminal row.
- Revert `progress::msg()`'s cursor-up call from the `rows_for()` row-summation back to `tty::cursor_up(m_buffer.size())`, since capped lines can no longer wrap; remove `rows_for()` if it becomes unused (keep `visible_width()`, still used by `cap_visible_width()`).
- Leave `success()` unaffected, and leave `failure()` printing the full, uncapped `m_buffer` lines unchanged — only the rolling `msg()` redraw path is capped.
- Keep `tty::erase_to_end()` in all three methods as a defensive redraw-correctness measure, independent of the new length cap.
- Update/add unit or manual verification: re-run `baldr -p ~/repos/dsp build` in a sized terminal and confirm rolling-window lines longer than the terminal width are now truncated with a trailing `"..."` and never wrap, while a triggered `failure()` still shows the complete, untruncated line.

### Follow-up: Document the tty.hpp pattern in the developer guide
The `baldr::tty` module (`tty.hpp`/`tty.cpp`) is a good example of the repo's preferred pattern for a small, self-contained, Doxygen-documented, independently unit-tested utility module (public free functions grouped in a namespace, a `detail::` sub-namespace for shared private helpers, one `.hpp`/`.cpp` pair, one `<name>.test.cpp`). This pattern is written up in `doc/developer-guide.adoc` (AsciiDoc, matching its existing `== Architecture`/`== Building` sections) so future contributors reuse it instead of re-embedding helpers as anonymous-namespace/private functions inside a consuming `.cpp` file.

A dedicated test-specification document for `tty` is intentionally **not** added: the existing `tty.test.cpp` GTest cases (`tty.test.cpp`'s `TEST(tty, ...)` names/assertions) already serve as the human-readable behavioral reference, and a separate spec (like `doc/lexy/test-specification.md`) would just duplicate that content with extra maintenance cost for this module's scope.

### ✓ Step 5: Extract terminal helpers into a new, tested tty.hpp/tty.cpp (renamed from ansi.hpp/ansi.cpp)
`baldr-cpp/baldr` gains a reusable `baldr::tty` module holding all terminal/escape-sequence helpers, each documented with a Doxygen comment (no inline `//` comments), with dedicated GTest coverage. The module (and its namespace, files, and tests) is named `tty` rather than `ansi`, since it groups general terminal/TTY control helpers — not exclusively ANSI escape-sequence parsing.

- Create `baldr-cpp/baldr/tty.hpp` and `baldr-cpp/baldr/tty.cpp` in the `baldr::tty` namespace, moving `clear_line()`, `erase_to_end()`, `cursor_up()`, `cursor_down()`, and `terminal_width()` out of `progress.hpp`/`progress.cpp` unchanged in behavior.
- Move `visible_width(line)` and `cap_visible_width(line, max_width)` out of `progress.cpp`'s anonymous namespace into `baldr::tty` as public, independently-testable functions.
- Factor the shared CSI-sequence-skipping walk (used by both `visible_width` and `cap_visible_width`) into a small private helper (e.g. `tty::detail::skip_csi_sequence`), removing the duplicated scan logic.
- Replace every `//`-style inline comment in the moved/refactored code with a Doxygen `/** ... */` doc comment on the corresponding function, per `AGENTS.md`.
- Update `progress.hpp`/`progress.cpp` to `#include <baldr/tty.hpp>` and call the `tty::` versions instead of local/anonymous-namespace helpers; `progress::msg()` keeps calling `tty::cap_visible_width(line, tty::terminal_width() - 1)`.
- Add `baldr-cpp/baldr/tty.test.cpp` with `TEST(tty, ...)` cases covering: visible width of plain text, visible width with one and multiple stripped ANSI sequences, empty-string width, capping under/at/over budget with the `"..."` marker, preservation of a trailing ANSI reset after truncation, and `terminal_width()` returning a sane fallback value.
- Update `baldr-cpp/baldr/test.cmake`/`CMakeLists.txt` glob/link setup so `tty.test.cpp` is picked up alongside `line_reader.test.cpp` by the existing `add_test_target(baldr)`.
- Rebuild `test-baldr` and confirm all `line_reader` and new `tty` unit tests pass, then rebuild `baldr` and re-run `baldr -p ~/repos/dsp build` to confirm the rolling window still renders identically after the extraction.

### ✓ Step 6: Rename ansi module to tty
All `ansi`-named symbols/files from Step 5 are renamed to `tty` (module already reflects this above); this step exists to explicitly rename any already-applied `ansi.hpp`/`ansi.cpp`/`ansi.test.cpp` artifacts and references left over from the prior implementation to `tty.hpp`/`tty.cpp`/`tty.test.cpp`.

- Rename `baldr-cpp/baldr/ansi.hpp` → `tty.hpp`, `ansi.cpp` → `tty.cpp`, `ansi.test.cpp` → `tty.test.cpp`.
- Replace the `baldr::ansi` namespace (and `ansi::detail::`) with `baldr::tty` (`tty::detail::`) in all moved code and its test file.
- Update `progress.hpp`/`progress.cpp` includes/call sites from `<baldr/ansi.hpp>`/`ansi::...` to `<baldr/tty.hpp>`/`tty::...`.
- Update `baldr-cpp/baldr/test.cmake`/`CMakeLists.txt` glob references from `ansi.test.cpp`/`ansi.cpp` to `tty.test.cpp`/`tty.cpp`.
- Rebuild `test-baldr` and confirm all tests (now `TEST(tty, ...)`) still pass, then rebuild `baldr` to confirm no leftover `ansi` references remain.

### ✓ Step 7: Add an end-to-end regression test covering both fixed bugs
A new `baldr-cpp/baldr/regression.test.cpp` exercises the real `command` → `line_reader` → `progress::msg` pipeline against a real subprocess, guarding against regressions of both the line-splitting bug and the row-counting/cursor-desync bug in a single test case.

- Update `baldr-cpp/baldr/test.cmake` to also compile `progress.cpp` into the `test-baldr` binary (alongside the existing `tty.cpp`), since `baldr::progress::msg()` is exercised directly by this test.
- Add `baldr-cpp/baldr/regression.test.cpp` with `TEST(regression, SplitAndOversizedSubprocessLinesRenderOneRowPerRecord)`: spawn a real subprocess via `baldr::command` that writes two lines in a single write, one line split across two writes with a short sleep in between, and one line far longer than an assumed terminal width.
- In the same test, drain `command::poll()` through a `baldr::line_reader` into a captured `std::vector<std::string>` and assert the exact reassembled line count/content (regression coverage for Steps 1–3).
- Continue the test by feeding each reassembled line into a `baldr::progress` instance with stdout captured (e.g. redirected to a pipe/temp file via `dup2`), asserting each `msg()` redraw prints exactly the buffered-line-count of rows and truncates the oversized line with a trailing `"..."` (regression coverage for Step 4).
- Rebuild `test-baldr` and confirm the new regression test passes alongside all existing `line_reader`/`tty` unit tests.

### ✓ Step 8: Add an external tests/rolling-window.sh shell script (regression check + human demo)
A new `tests/rolling-window.sh`, wired into `tests/run.sh`, drives the real compiled `baldr` binary through `baldr run -- ...` scenarios reproducing both fixed bugs, doubling as a narrated, colorful manual demo when run directly by a human.

- Create `tests/rolling-window.sh` following `tests/stdout.sh`'s structure: same directory/temp-dir setup (`SCRIPT_DIR`/`PROJECT_ROOT`/`TMP_DIR` with a timestamped run id), a `baldr -p "$PROJECT_ROOT" ...` bootstrap build, and the same `run_test`/`PASSED`/`FAILED`/`FAILED_TESTS` helper-function/summary pattern.
- Implement three scenario helpers driving `baldr run -- bash -c '...'` under `script -qc "..." <logfile>` (pseudo-TTY): (a) two lines from one `printf`, (b) one line split across two `printf`s with a `sleep` in between, (c) one ~200-char line to exercise truncation.
- Before each scenario, print a short human-readable description of the bug it demonstrates, and after running it, echo back the captured raw terminal bytes (e.g. via `cat -v`) so a person watching gets a visible before/after style demo, not just a pass/fail line.
- Strip ANSI escapes from each captured log to plain text and assert scenarios (a)/(b) contain the exact expected reassembled lines (no duplicated/partial lines), and scenario (c)'s oversized line appears truncated with a trailing `"..."` and never wraps onto a second row in the raw escaped log.
- Add `tests/rolling-window.sh` to `tests/run.sh`'s invocation list (alongside `stdout.sh`), keeping it executable and self-contained like the other scripts.