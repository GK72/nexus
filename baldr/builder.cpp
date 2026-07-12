#include <baldr/builder.hpp>

#include <libnova/error.hpp>
#include <libnova/log.hpp>

namespace baldr {

builder::builder(std::string project_dir, std::string build_dir)
    : m_project_dir(std::move(project_dir)), m_build_dir(std::move(build_dir)) { }

void builder::configure() const {
    throw nova::exception("builder::configure() is not yet implemented.");
}

void builder::build() const {
    throw nova::exception("builder::build() is not yet implemented.");
}

} // namespace baldr
