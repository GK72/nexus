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

    command(const std::vector<std::string>& args)
        : m_pid(fork())
    {
        m_args.reserve(args.size() + 1);

        for (const auto& arg : args) {
            m_args.push_back(const_cast<char*>(arg.c_str()));                                       // NOLINT(cppcoreguidelines-pro-type-const-cast) | https://pubs.opengroup.org/onlinepubs/009604499/functions/exec.html (RATIONALE)
        }

        m_args.push_back(nullptr);

        if (m_pid == -1) {
            throw nova::exception("Failed to fork process");
        };
    }

    auto run() {
        if (m_pid == 0) {
            m_pipe.redirect(file_descriptor::both);
            execvp(m_args[0], m_args.data());
        }

        close(m_pipe.write());
    }

    auto poll() -> std::string {
        ssize_t n = read(m_pipe.read(), m_buffer.data(), m_buffer.size());
        return { m_buffer.data(), static_cast<std::size_t>(n) };
    }

    /**
     * @brief   Wait for the spawned process to finish.
     *
     * @returns with the exit code of the spawned process.
     *
     * @throws  if wait fails.
     */
    auto wait() -> int {
        close(m_pipe.read());

        int status = 0;
        waitpid(m_pid, &status, 0);

        // TODO: Finish implementation, check for other statuses.
        // Reference: https://pubs.opengroup.org/onlinepubs/9699919799/functions/wait.html
        if (not WIFEXITED(status)) {
            throw nova::exception("Spawned process failed to correctly exit");
        }

        return WEXITSTATUS(status);
    }

private:
    std::vector<char*> m_args;
    pipe m_pipe;
    pid_t m_pid;

    static constexpr auto BufferSize = 4096;
    std::array<char, BufferSize> m_buffer{ };

};

} // namespace baldr
