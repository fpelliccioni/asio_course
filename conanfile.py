from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps


class asio_courseRecipe(ConanFile):
    name = "asio_course"
    version = "1.0.0"
    package_type = "application"
    settings = "os", "compiler", "build_type", "arch"
    exports_sources = "CMakeLists.txt", "src/*"

    def requirements(self):
        self.requires("asio/1.28.0", transitive_headers=True, transitive_libs=True)
        self.requires("fmt/10.0.0", transitive_headers=True, transitive_libs=True)
        self.requires("nlohmann_json/3.11.2", transitive_headers=True, transitive_libs=True)
        # self.requires("boost/1.82.0", transitive_headers=True, transitive_libs=True)

    def configure(self):
        self.options["fmt/*"].header_only = True

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()




