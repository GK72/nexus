#pragma once

#include <string>

namespace baldr {

/**
 * @brief   Wraps `cmake` configure/build invocations for the target project.
 *
 * Currently a stub: a later commit adds an actual process wrapper (`command`,
 * `line_reader`) and wires `configure()`/`build()` to a real `cmake`
 * invocation.
 */
class builder {
public:
    explicit builder(std::string project_dir = ".", std::string build_dir = "build");

    /**
     * @brief   Configure the project. Not yet implemented.
     */
    void configure() const;

    /**
     * @brief   Build the project. Not yet implemented.
     */
    void build() const;

private:
    std::string m_project_dir;
    std::string m_build_dir;
};

} // namespace baldr
