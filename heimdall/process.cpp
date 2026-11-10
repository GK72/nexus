/**
 * Part of Nexus Library
 *
 * @brief   Runs a program to completion and captures its output.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-10
 * @updated     2026-11-10
 * @status      experimental
 * @reviewed    none
 * SPDX-License-Identifier: Unlicense
 */

#include "heimdall/process.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <string_view>

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;  // NOLINT: POSIX

namespace heimdall {

namespace {

/// A file descriptor, closed when it goes out of scope.
class unique_fd {
public:
    unique_fd() = default;
    explicit unique_fd(int fd) : m_fd(fd) {}
    unique_fd(const unique_fd&) = delete;
    auto operator=(const unique_fd&) -> unique_fd& = delete;
    unique_fd(unique_fd&& other) noexcept : m_fd(std::exchange(other.m_fd, -1)) {}
    auto operator=(unique_fd&& other) noexcept -> unique_fd& {
        reset(std::exchange(other.m_fd, -1));
        return *this;
    }
    ~unique_fd() { reset(); }

    [[nodiscard]] auto get() const -> int { return m_fd; }
    [[nodiscard]] auto valid() const -> bool { return m_fd >= 0; }

    void reset(int fd = -1) {
        if (m_fd >= 0) {
            ::close(m_fd);
        }
        m_fd = fd;
    }

private:
    int m_fd = -1;
};

/// The read and the write end of a pipe, both closed on exec.
struct pipe_ends {
    unique_fd read;
    unique_fd write;
};

[[nodiscard]] auto make_pipe() -> pipe_ends {
    std::array<int, 2> fds { -1, -1 };
    if (::pipe2(fds.data(), O_CLOEXEC) != 0) {
        return {};
    }
    return { .read = unique_fd(fds[0]), .write = unique_fd(fds[1]) };
}

/// The environment of this process, with `overrides` on top.
[[nodiscard]] auto environment(const std::vector<std::pair<std::string, std::string>>& overrides)
    -> std::vector<std::string>
{
    std::vector<std::string> entries;
    for (char** entry = environ; entry != nullptr and *entry != nullptr; ++entry) {
        const auto text = std::string_view(*entry);
        const auto key = text.substr(0, text.find('='));
        const bool replaced = std::ranges::any_of(overrides, [&](const auto& item) { return item.first == key; });
        if (not replaced) {
            entries.emplace_back(text);
        }
    }
    for (const auto& [key, value] : overrides) {
        entries.push_back(key + '=' + value);
    }
    return entries;
}

[[nodiscard]] auto pointers(std::vector<std::string>& items) -> std::vector<char*> {
    std::vector<char*> ret;
    ret.reserve(items.size() + 1);
    for (auto& item : items) {
        ret.push_back(item.data());
    }
    ret.push_back(nullptr);
    return ret;
}

/// Reads both pipes until they close or the deadline passes; false on timeout.
[[nodiscard]] auto drain(unique_fd& out, unique_fd& err, process_result& result,
                         std::chrono::steady_clock::time_point deadline) -> bool
{
    std::array<char, 8192> buffer {};
    while (out.valid() or err.valid()) {
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now());
        if (remaining.count() <= 0) {
            return false;
        }

        std::array<pollfd, 2> fds {
            pollfd { .fd = out.get(), .events = POLLIN, .revents = 0 },
            pollfd { .fd = err.get(), .events = POLLIN, .revents = 0 },
        };
        const int ready = ::poll(fds.data(), fds.size(), static_cast<int>(remaining.count()));
        if (ready < 0 and errno != EINTR) {
            return true;
        }

        for (std::size_t index = 0; index < fds.size(); ++index) {
            if (fds[index].fd < 0 or fds[index].revents == 0) {
                continue;
            }
            auto& source = index == 0 ? out : err;
            auto& sink = index == 0 ? result.out : result.err;
            const auto count = ::read(source.get(), buffer.data(), buffer.size());
            if (count > 0) {
                sink.append(buffer.data(), static_cast<std::size_t>(count));
            } else if (count == 0 or errno != EINTR) {
                source.reset();
            }
        }
    }
    return true;
}

} // namespace

auto run_process(const std::vector<std::string>& args, const process_options& options) -> process_result {
    process_result result;
    if (args.empty()) {
        result.status = 127;
        result.err = "no program given";
        return result;
    }

    auto out = make_pipe();
    auto err = make_pipe();
    if (not out.read.valid() or not err.read.valid()) {
        result.status = 127;
        result.err = std::strerror(errno);
        return result;
    }

    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
    posix_spawn_file_actions_adddup2(&actions, out.write.get(), STDOUT_FILENO);
    posix_spawn_file_actions_adddup2(&actions, err.write.get(), STDERR_FILENO);
    if (not options.directory.empty()) {
        posix_spawn_file_actions_addchdir_np(&actions, options.directory.c_str());
    }

    posix_spawnattr_t attributes;
    posix_spawnattr_init(&attributes);
    if (options.new_session) {
        posix_spawnattr_setflags(&attributes, POSIX_SPAWN_SETSID);
    }

    auto argv_items = args;
    auto env_items = environment(options.env);
    auto argv = pointers(argv_items);
    auto envp = pointers(env_items);

    pid_t pid = -1;
    const int spawned = ::posix_spawnp(&pid, argv[0], &actions, &attributes, argv.data(), envp.data());
    posix_spawn_file_actions_destroy(&actions);
    posix_spawnattr_destroy(&attributes);
    out.write.reset();
    err.write.reset();

    if (spawned != 0) {
        result.status = 127;
        result.err = std::strerror(spawned);
        return result;
    }

    const auto deadline = std::chrono::steady_clock::now() + options.timeout;
    if (not drain(out.read, err.read, result, deadline)) {
        result.timed_out = true;
        ::kill(options.new_session ? -pid : pid, SIGKILL);
    }

    int status = 0;
    while (::waitpid(pid, &status, 0) < 0 and errno == EINTR) {
    }
    result.status = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
    return result;
}

} // namespace heimdall
