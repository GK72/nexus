#pragma once

#include <array>
#include <string>
#include <vector>
#include <map>

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

    command(const std::vector<std::string>& args, 
            const std::map<std::string, std::string>& env = {},
            bool interactive = false)
        : m_args_vec(args), m_env_map(env), m_interactive(interactive)
    {
        m_args.reserve(m_args_vec.size() + 1);
        for (const auto& arg : m_args_vec) {
            m_args.push_back(const_cast<char*>(arg.c_str()));
        }
        m_args.push_back(nullptr);
    }

    auto run() {
        m_pid = fork();
        if (m_pid == -1) {
            throw nova::exception("Failed to fork process");
        }

        if (m_pid == 0) {
            if (!m_interactive) {
                m_pipe.redirect(file_descriptor::both);
            }

            for (const auto& [key, value] : m_env_map) {
                setenv(key.c_str(), value.c_str(), 1);
            }

            execvp(m_args[0], m_args.data());
            perror("execvp");
            _exit(EXIT_FAILURE);
        }

        if (!m_interactive) {
            ::close(m_pipe.write());
        }
    }

    auto poll() -> std::string {
        if (m_interactive) return {};
        ssize_t n = ::read(m_pipe.read(), m_buffer.data(), m_buffer.size());
        if (n <= 0) return {};
        return { m_buffer.data(), static_cast<std::size_t>(n) };
    }

    auto wait() -> int {
        if (!m_interactive) {
            ::close(m_pipe.read());
        }

        int status = 0;
        waitpid(m_pid, &status, 0);

        if (not WIFEXITED(status)) {
            return EXIT_FAILURE;
        }

        return WEXITSTATUS(status);
    }

private:
    std::vector<std::string> m_args_vec;
    std::vector<char*> m_args;
    std::map<std::string, std::string> m_env_map;
    pipe m_pipe;
    pid_t m_pid = -1;
    bool m_interactive = false;

    static constexpr auto BufferSize = 4096;
    std::array<char, BufferSize> m_buffer{ };
};

} // namespace baldr
