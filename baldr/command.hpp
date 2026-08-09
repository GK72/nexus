/**
 * Part of Baldr
 *
 * Process spawning.
 *
 * @author      Gábor Krisztián Girhiny
 * @coauthor    Claude Sonnet 5 (Junie)
 * @date        2026-07-12
 */

#pragma once

#include <array>
#include <string>
#include <vector>
#include <map>

#include <libnova/error.hpp>
#include <fmt/format.h>

#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <tuple>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>

namespace baldr {

struct resource_usage {
    std::chrono::microseconds user_time { 0 };
    std::chrono::microseconds system_time { 0 };
    long peak_rss_kb = 0;   // `ru_maxrss`; kilobytes on Linux.
};

[[nodiscard]] inline
auto create_resource_usage(const rusage raw_usage) -> resource_usage {
    return {
        .user_time   = std::chrono::seconds{raw_usage.ru_utime.tv_sec} + std::chrono::microseconds{raw_usage.ru_utime.tv_usec},
        .system_time = std::chrono::seconds{raw_usage.ru_stime.tv_sec} + std::chrono::microseconds{raw_usage.ru_stime.tv_usec},
        .peak_rss_kb = raw_usage.ru_maxrss,
    };
}

/**
 * @brief   Spawns a child process, exposing its combined stdout/stderr
 *          output through a pull-based `poll()`, or leaving the child
 *          attached to the parent's own TTY in `interactive` mode.
 *
 * Environment variable overrides (`m_env_map`) are added on top of the
 * current process's environment, e.g. to force `CC`/`CXX` for a `cmake`
 * invocation.
 */
class command {
public:
    enum class file_descriptor {
        stdout,
        stderr,
        both
    };

    /**
     * @brief   Detailed outcome of a finished process, distinguishing a
     *          normal exit from being killed by a signal or from `execvp`
     *          itself failing (e.g. the executable doesn't exist).
     */
    class exit_status {
    public:
        enum class kind {
            exited,       // Process ran and exited normally; `value` is the exit code.
            signaled,     // Process was killed by a signal; `value` is the signal number.
            exec_failed,  // `execvp` itself failed; `value` is the `errno` from `execvp`.
        };

        exit_status(const char* name, kind type, int value, resource_usage usage = {})
            : m_name(name)
            , m_type(type)
            , m_value(value)
            , m_usage(usage)
        {}

        [[nodiscard]] auto success() const -> bool {
            return m_type == kind::exited
                && m_value == 0;
        }

        /**
         * @brief   Shell-style numeric exit code, for callers that only
         *          care about a single integer (128+signal for signals,
         *          127 for a failed `execvp`, following common convention).
         */
        [[nodiscard]] auto code() const -> int {
            switch (m_type) {
                case kind::exited:      return m_value;
                case kind::signaled:    return 128 + m_value;
                case kind::exec_failed: return 127;
            }
            return EXIT_FAILURE;
        }

        /**
         * @brief   Whether the process was terminated by `SIGINT`, i.e. the
         *          user hit Ctrl-C rather than the process actually failing.
         */
        [[nodiscard]] auto interrupted() const -> bool {
            return m_type == kind::signaled
                && m_value == SIGINT;
        }

        /**
         * @brief   Human-readable description of the outcome, naming the signal
         *          or the `errno` reason instead of just a bare, ambiguous exit code.
         */
        [[nodiscard]] auto describe() const -> std::string {
            switch (m_type) {
                case kind::exited:
                    return fmt::format("`{}` exited with code {}.", m_name, m_value);
                case kind::signaled:
                    return fmt::format("`{}` was terminated by signal {} ({}).", m_name, m_value, strsignal(m_value));
                case kind::exec_failed:
                    return fmt::format("Failed to execute `{}`: {}.", m_name, strerror(m_value));
            }
            return fmt::format("`{}` exited abnormally.", m_name);
        }

        /**
         * @brief   Resource usage accumulated by the process.
         */
        [[nodiscard]] auto usage() const -> const resource_usage& {
            return m_usage;
        }

    private:
        const char* m_name;
        kind m_type = kind::exited;
        int m_value = 0;
        resource_usage m_usage;

    };

    /**
     * @brief   RAII wrapper around a `pipe(2)` pair, with helpers to
     *          redirect the write end onto a standard file descriptor in
     *          the child process.
     */
    class pipe {
    public:
        pipe() {
            if (::pipe(m_inner.data()) == -1) {
                throw nova::exception("Pipe creation failed");
            };
        }

        [[nodiscard]] auto read()  const -> int { return m_inner[0]; }
        [[nodiscard]] auto write() const -> int { return m_inner[1]; }

        void close_read() const {
            ::close(read());
        }

        void close_write() const {
            ::close(write());
        }

        void redirect(file_descriptor fd) const {
            switch (fd) {
                case file_descriptor::stdout:
                    redirect_impl(STDOUT_FILENO);
                    break;
                case file_descriptor::stderr:
                    redirect_impl(STDERR_FILENO);
                    break;
                case file_descriptor::both:
                    redirect_impl(STDOUT_FILENO);
                    redirect_impl(STDERR_FILENO);
                    break;
            }
            close_write();
        }

    private:
        std::array<int, 2> m_inner {};

        void redirect_impl(int fd) const {
            if (dup2(write(), fd) == -1) {
                throw nova::exception("Pipe duplication failed");
            };
        }
    };

    /**
     * @brief   A wrapper around a pseudo-terminal, used in place of a
     *          plain `pipe(2)` for the child's stdout/stderr.
     *
     * Unlike a pipe, a pty makes the child (and anything it in turn spawns) see
     * a real terminal on `isatty()`, so they keep emitting their own ANSI
     * colors instead of silently falling back to plain text because they think
     * they're talking to a file/pipe.
     */
    class pty {
    public:
        pty() {
            m_master = ::posix_openpt(O_RDWR | O_NOCTTY);
            if (m_master == -1) {
                throw nova::exception("Failed to open pty master");
            }

            if (::grantpt(m_master) == -1 || ::unlockpt(m_master) == -1) {
                ::close(m_master);
                throw nova::exception("Failed to configure pty");
            }

            std::array<char, 64> path{};
            if (::ptsname_r(m_master, path.data(), path.size()) != 0) {
                ::close(m_master);
                throw nova::exception("Failed to resolve pty slave path");
            }

            m_slave_path = path.data();

            winsize ws{};
            if (::ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0 || ws.ws_row == 0) {
                ws.ws_row = 50;
                ws.ws_col = 200;
            }
            ::ioctl(m_master, TIOCSWINSZ, &ws);
        }

        [[nodiscard]] auto master() const -> int { return m_master; }

        /**
         * @brief   Open the slave side and dup it onto `fd`, then close both
         *          the parent's master and the temporary slave descriptor.
         *
         * Child-side only: called after `fork()`, before `execvp()`.
         */
        void redirect_child(file_descriptor fd) const {
            int slave = ::open(m_slave_path.c_str(), O_RDWR);
            if (slave == -1) {
                throw nova::exception("Failed to open pty slave");
            }

            switch (fd) {
                case file_descriptor::stdout:
                    dup2(slave, STDOUT_FILENO);
                    break;
                case file_descriptor::stderr:
                    dup2(slave, STDERR_FILENO);
                    break;
                case file_descriptor::both:
                    dup2(slave, STDOUT_FILENO);
                    dup2(slave, STDERR_FILENO);
                    break;
            }

            if (slave > STDERR_FILENO) {
                ::close(slave);
            }
            ::close(m_master);
        }

    private:
        int m_master = -1;
        std::string m_slave_path;
    };

    command(
            const std::vector<std::string>& args,
            const std::map<std::string, std::string>& env = {},
            std::string working_directory = {},
            bool interactive = false
    )
        : m_args_vec(args)
        , m_env_map(env)
        , m_interactive(interactive)
        , m_working_directory(std::move(working_directory))
    {
        m_args.reserve(m_args_vec.size() + 1);
        for (const auto& arg : m_args_vec) {
            m_args.push_back(const_cast<char*>(arg.c_str()));
        }
        m_args.push_back(nullptr);
    }

    /**
     * @brief   Fork and exec the process. In non-interactive mode, the
     *          child's stdout/stderr are redirected onto a pty, so the
     *          child (and anything it spawns) still thinks it's talking to
     *          a real terminal and keeps emitting its own colors.
     */
    auto run() {
        if (fcntl(m_error_pipe.write(), F_SETFD, FD_CLOEXEC) == -1) {
            throw nova::exception("Failed to configure error pipe");
        }

        m_pid = fork();
        if (m_pid == -1) {
            throw nova::exception("Failed to fork process");
        }

        if (m_pid == 0) {
            m_error_pipe.close_read();

            if (not m_interactive) {
                if (setsid() == -1) {
                    int setsid_errno = errno;
                    std::ignore = ::write(m_error_pipe.write(), &setsid_errno, sizeof(setsid_errno));
                    _exit(EXIT_FAILURE);
                }
                m_pty.redirect_child(file_descriptor::both);
            }

            for (const auto& [key, value] : m_env_map) {
                setenv(key.c_str(), value.c_str(), 1);
            }

            if (not m_working_directory.empty() && chdir(m_working_directory.c_str()) == -1) {
                int chdir_errno = errno;
                std::ignore = ::write(m_error_pipe.write(), &chdir_errno, sizeof(chdir_errno));
                _exit(EXIT_FAILURE);
            }

            execvp(m_args[0], m_args.data());
            int exec_errno = errno;
            std::ignore = ::write(m_error_pipe.write(), &exec_errno, sizeof(exec_errno));
            _exit(EXIT_FAILURE);
        }

        m_error_pipe.close_write();
        int exec_errno = 0;
        ssize_t n = ::read(m_error_pipe.read(), &exec_errno, sizeof(exec_errno));
        m_error_pipe.close_read();
        if (n == sizeof(exec_errno)) {
            m_exec_failed = true;
            m_exec_errno = exec_errno;
        }
    }

    /**
     * @brief   Read and return the next available chunk of combined
     *          stdout/stderr output.
     *
     * @return  The chunk read, or an empty string on EOF/`interactive` mode.
     *          An empty return does not necessarily mean EOF for a single
     *          `read()`; callers should keep polling until `wait()`.
     *
     * A pty master reports the child side going away as `read()` failing
     * with `EIO` rather than returning `0`, unlike a plain pipe; both are
     * folded into the same empty-return, end-of-output case here.
     */
    auto poll() -> std::string {
        if (m_interactive) {
            return {};
        }

        ssize_t n = 0;
        do {
            n = ::read(m_pty.master(), m_buffer.data(), m_buffer.size());
        } while (n == -1 && errno == EINTR);

        if (n <= 0) {
            return {};
        }

        return {
            m_buffer.data(),
            static_cast<std::size_t>(n)
        };
    }

    /**
     * @brief   PID of the spawned process, valid once `run()` has returned.
     *
     * Useful for a caller that wants to act on the child independently of
     * this class, e.g. registering it with `signal_handler::watch()` for a
     * force-kill escalation while `poll()`/`wait()` are in progress.
     */
    [[nodiscard]] auto pid() const -> pid_t {
        return m_pid;
    }

    /**
     * @brief   Wait for the process to exit.
     *
     * @return  The detailed outcome, distinguishing a normal exit from a
     *          signal or a failed `execvp`, and carrying its resource usage
     *          (CPU time, peak memory) as reported by `wait4(2)`.
     */
    auto wait() -> exit_status {
        if (not m_interactive) {
            ::close(m_pty.master());
        }

        int status = 0;
        struct rusage raw_usage {};
        while (wait4(m_pid, &status, 0, &raw_usage) == -1 && errno == EINTR) {
            // Retry rather than trust a `status` a failed wait4() never
            // wrote to; see doc/baldr/developer-manual.adoc ("SA_RESTART").
        }

        if (m_exec_failed) {
            return { m_args[0], exit_status::kind::exec_failed, m_exec_errno };
        }

        auto usage = create_resource_usage(raw_usage);

        if (WIFSIGNALED(status)) {
            return { m_args[0], exit_status::kind::signaled, WTERMSIG(status), usage };
        }

        if (WIFEXITED(status)) {
            return { m_args[0], exit_status::kind::exited, WEXITSTATUS(status), usage };
        }

        return { m_args[0], exit_status::kind::exited, EXIT_FAILURE, usage };
    }

private:
    std::vector<std::string> m_args_vec;
    std::vector<char*> m_args;
    std::map<std::string, std::string> m_env_map;
    pty m_pty;
    pipe m_error_pipe;
    pid_t m_pid = -1;
    bool m_interactive = false;
    std::string m_working_directory;
    bool m_exec_failed = false;
    int m_exec_errno = 0;

    static constexpr auto BufferSize = 4096;
    std::array<char, BufferSize> m_buffer{ };
};

} // namespace baldr
