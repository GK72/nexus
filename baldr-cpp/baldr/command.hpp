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

inline auto run(const std::vector<std::string>& cmd) {
    // auto pipes = std::array<int, 2>{ };
    int pipes[2];

    if (pipe(pipes) == -1) { throw nova::exception("Pipe failed"); };
    pid_t pid = fork();
    if (pid == -1) { throw nova::exception("Fork failed"); };

    if (pid == 0) {
        if (dup2(pipes[1], STDOUT_FILENO) == -1) { throw nova::exception("Pipe dup failed"); };
        if (dup2(pipes[1], STDERR_FILENO) == -1) { throw nova::exception("Pipe dup (err) failed"); };
        close(pipes[0]);
        close(pipes[1]);

        std::vector<char*> argv;
        for (auto& s : cmd) {
            argv.push_back(const_cast<char*>(s.c_str()));
        }
        argv.push_back(nullptr);

        execvp(argv[0], argv.data());
    }

    close(pipes[1]); // close write end

    std::string output;
    std::array<char, 4096> buffer{};
    ssize_t n;

    while ((n = read(pipes[0], buffer.data(), buffer.size())) > 0) {
        output.append(buffer.data(), static_cast<std::size_t>(n));
    }

    close(pipes[0]);

    int status = 0;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) {
        fmt::println("Exit code: {}", WEXITSTATUS(status));
    } else {
        fmt::println("Not exited");
    }

    fmt::println("{}", output);
}

} // namespace baldr
