from conan import ConanFile

class Recipe(ConanFile):
    build_policy = "missing"

    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    def requirements(self):
        self.requires("ftxui/3.0.0")
        self.requires("cli11/2.2.0")
        self.requires("spdlog/1.11.0")
        self.requires("nlohmann_json/3.11.2")