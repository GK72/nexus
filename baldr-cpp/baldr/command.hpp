#pragma once

#include <array>
#include <string>
#include <vector>

#include <libnova/error.hpp>
#include <fmt/format.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

namespace baldr {

class command {
public:
    enum class file_descriptor {
        stdout,
        stderr,
        both
    };

    class pipe {
    public:

        pipe() {
            if (::pipe(m_inner.data()) == -1) {
                throw nova::exception("Pipe creation failed");
            };
        }

        [[nodiscard]] auto read()  const -> int { return m_inner[0]; }
        [[nodiscard]] auto write() const -> int { return m_inner[1]; }

        /**
         * @brief   Close the read end of the pipe.
         */
        void close_read() const {
            ::close(read());
        }

        /**
         * @brief   Close the write end of the pipe.
         */
        void close_write() const {
            ::close(write());
        }

        /**
         * @brief   Redirect `stdout` and/or `stderr` to the write end of the
         *          pipe.
         *
         * Closes the original write file descriptor as it is redirected.
         */
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

    command(std::vector<std::string> args)
        : m_args(std::move(args))
        , m_pid(fork())
    {
        if (m_pid == -1) {
            throw nova::exception("Failed to fork process");
        };
    }

    // TODO(refact)
    auto run() {
        if (m_pid == 0) {
            m_pipe.redirect(file_descriptor::both);

            std::vector<char*> argv;
            for (auto& s : m_args) {
                argv.push_back(const_cast<char*>(s.c_str()));
            }
            argv.push_back(nullptr);

            execvp(argv[0], argv.data());
        }

        close(m_pipe.write());
    }

    // TODO(refact)
    void poll() {
        std::string output;
        std::array<char, 4096> buffer{};
        ssize_t n;

        while ((n = read(m_pipe.read(), buffer.data(), buffer.size())) > 0) {
            output.append(buffer.data(), static_cast<std::size_t>(n));
        }

        close(m_pipe.read());

        fmt::println("{}", output);
    }

    /**
     * @brief   Wait for the spawned process to finish.
     *
     * @returns with the exit code of the spawned process.
     */
    auto wait() -> int {
        int status = 0;
        // TODO(refact): Finish implementation.
        // Reference: https://pubs.opengroup.org/onlinepubs/9699919799/functions/wait.html
        waitpid(m_pid, &status, 0);

        if (not WIFEXITED(status)) {
            throw nova::exception("Spawned process failed to correctly exit");
        }

        return WEXITSTATUS(status);
    }

private:
    std::vector<std::string> m_args;
    pipe m_pipe;
    pid_t m_pid;

};

} // namespace baldr
