from conans import ConanFile, CMake
import subprocess


def version():
    default = "0.0.0"
    try:
        git = subprocess.Popen(
            ["git", "describe", "--long", "--dirty"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
        )
    except Exception:
        return default
    val = git.communicate()[0]
    if git.returncode != 0:
        return default
    version = val.strip().split("-")
    return "{}.{}".format(version[0], version[1])


class BongoConan(ConanFile):
    name = "bongo"
    version = version()
    license = "BSD 3-clause"
    url = "https://github.com/msteinert/bongo"
    description = "Go APIs ported to C++"
    generators = "cmake_find_package", "cmake_paths"
    settings = "os", "compiler", "build_type", "arch"
    scm = {
        "type": "git",
        "url": "https://github.com/msteinert/bongo.git",
        "revision": "auto",
    }
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
    }
    default_options = (
        "shared=False",
        "fPIC=True",
    )
    build_requires = ("catch2/2.13.7",)

    def configure_cmake(self):
        cmake = CMake(self, generator="Ninja")
        cmake.configure()
        return cmake

    def build(self):
        cmake = self.configure_cmake()
        cmake.build()

    def package(self):
        cmake = self.configure_cmake()
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["bongo"]
