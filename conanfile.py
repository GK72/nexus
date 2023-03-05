from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout
from conans import tools, util, ConanFile

required_conan_version = ">=1.52.0"

class Nexus(ConanFile):
    name = "nexus"
    description = "Nexus Library"
    version = "0.1.0"

    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {
        "shared": False,
        "fPIC": True
    }

    exports_sources = "CMakeLists.txt", "include/*", "src/*", "cmake/*", "conanfile.txt"

    @property
    def _min_cppstd(self):
        return 23

    def config_ooptions(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        self.requires("catch2/3.2.1")
        self.requires("fmt/9.1.0")
        self.requires("rapidjson/1.1.0")
        self.requires("spdlog/1.11.0")

    def validate(self):
        if self.settings.compiler.get_safe("cppstd"):
            tools.check_min_cppstd(self, self._min_cppstd)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure(variables={ "PACKAGING": 1 })
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["nexus"]
        self.cpp_info.set_property("cmake_file_name", "nexus")
        self.cpp_info.set_property("cmake_target_name", "nexus::nexus")
        self.cpp_info.names["cmake_find_package"] = "nexus"
        self.cpp_info.names["cmake_find_package_multi"] = "nexus"

        if self.settings.os in ["Linux", "Macos"]:
            self.cpp_info.system_libs.append("pthread")
