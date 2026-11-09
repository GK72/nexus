from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain

# Boost's compiled modules, as the pinned recipe (boost/1.87.0) names them. All are off but those
# in BOOST_MODULES_USED; the header-only libraries (Asio, Beast, ...) need none of them.
BOOST_MODULES = (
    "atomic", "charconv", "chrono", "cobalt", "container", "context", "contract", "coroutine",
    "date_time", "exception", "fiber", "filesystem", "graph", "graph_parallel", "iostreams",
    "json", "locale", "log", "math", "mpi", "nowide", "process", "program_options", "python",
    "random", "regex", "serialization", "stacktrace", "system", "test", "thread", "timer",
    "type_erasure", "url", "wave",
)
BOOST_MODULES_USED = ("program_options",)


class Project(ConanFile):
    name = "@name@"
    package_type = "application"
    settings = "os", "compiler", "build_type", "arch"

    options = {"with_tests": [True, False]}
    default_options = {
        "with_tests": True,
        **{f"boost/*:without_{module}": module not in BOOST_MODULES_USED for module in BOOST_MODULES},
    }

    def requirements(self):
        for req in self.conan_data.get("requirements", []):
            self.requires(req)

        if self.options.with_tests:
            for req in self.conan_data.get("test-requirements", []):
                self.requires(req)

    def generate(self):
        CMakeDeps(self).generate()
        tc = CMakeToolchain(self)
        tc.cache_variables["BUILD_TESTING"] = bool(self.options.with_tests)
        tc.generate()
