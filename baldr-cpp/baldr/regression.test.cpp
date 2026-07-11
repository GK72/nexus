#include <gtest/gtest.h>
#include <baldr/command.hpp>
#include <baldr/line_reader.hpp>
#include <baldr/progress.hpp>

#include <fmt/format.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

namespace {

/**
 * @brief   Redirects stdout to a temporary file for the duration of `fn`,
 *          then returns everything that was written to it.
 */
template <typename Callable>
std::string capture_stdout(Callable&& fn) {
    char path[] = "/tmp/baldr_regression_XXXXXX";
    int tmp_fd = mkstemp(path);

    fflush(stdout);
    int saved_stdout = dup(STDOUT_FILENO);
    dup2(tmp_fd, STDOUT_FILENO);
    ::close(tmp_fd);

    std::forward<Callable>(fn)();

    fflush(stdout);
    dup2(saved_stdout, STDOUT_FILENO);
    ::close(saved_stdout);

    std::ifstream in(path);
    std::ostringstream buf;
    buf << in.rdbuf();
    in.close();
    std::remove(path);

    return buf.str();
}

} // namespace

TEST(regression, SplitAndOversizedSubprocessLinesRenderOneRowPerRecord) {
    std::string oversized(200, 'x');

    std::string script = fmt::format(
        "printf 'line1\\nline2\\n'; "
        "printf 'par'; sleep 0.05; printf 'tial\\n'; "
        "printf '{}\\n'",
        oversized
    );

    baldr::command cmd({ "sh", "-c", script });
    cmd.run();

    std::vector<std::string> lines;
    baldr::line_reader reader([&lines](std::string& line) { lines.push_back(line); });

    std::string chunk;
    while (chunk = cmd.poll(), !chunk.empty()) {
        reader.feed(chunk);
    }
    reader.feed_eof();

    int exit_code = cmd.wait();
    ASSERT_EQ(exit_code, EXIT_SUCCESS);

    ASSERT_EQ(lines.size(), 4);
    EXPECT_EQ(lines[0], "line1");
    EXPECT_EQ(lines[1], "line2");
    EXPECT_EQ(lines[2], "partial");
    EXPECT_EQ(lines[3], oversized);

    baldr::progress prog;
    prog.lines(static_cast<int>(lines.size()));

    std::string output = capture_stdout([&] {
        for (auto& line : lines) {
            prog.msg(line);
        }
    });

    std::size_t last_erase = output.rfind("\x1b[0J");
    ASSERT_NE(last_erase, std::string::npos);
    std::string last_frame = output.substr(last_erase);

    std::size_t printed_rows = 0;
    std::size_t pos = 0;
    while ((pos = last_frame.find('\n', pos)) != std::string::npos) {
        ++printed_rows;
        ++pos;
    }
    EXPECT_EQ(printed_rows, lines.size());

    std::string expected_cursor_up = fmt::format("\x1b[{}A", lines.size());
    EXPECT_NE(last_frame.find(expected_cursor_up), std::string::npos);

    std::size_t oversized_pos = last_frame.find(oversized.substr(0, 20));
    ASSERT_NE(oversized_pos, std::string::npos);
    std::size_t line_end = last_frame.find('\n', oversized_pos);
    std::string rendered_oversized = last_frame.substr(oversized_pos, line_end - oversized_pos);
    EXPECT_NE(rendered_oversized.find("..."), std::string::npos);
    EXPECT_LT(rendered_oversized.size(), oversized.size());
}
