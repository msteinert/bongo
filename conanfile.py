from conans import ConanFile, CMake
import subprocess


def version():
    default = '0.0.0'
    try:
        git = subprocess.Popen(
            ['git', 'describe', '--long', '--dirty'],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True)
    except Exception:
        return default
    val = git.communicate()[0]
    if git.returncode != 0:
        return default
    version = val.strip().split('-')
    return '{}.{}'.format(version[0], version[1])


class BongoConan(ConanFile):
    name = 'bongo'
    version = version()
    license = 'No License'
    url = 'https://engrepo.exegy.net/exegy/bongo'
    description = 'Generally useful C++ interfaces for Linux systems'
    generators = 'cmake'
    scm = {
        'type': 'git',
        'url': 'https://engrepo.exegy.net/exegy/bongo.git',
        'revision': 'auto',
    }
    build_requires = (
        'Catch2/2.9.2@catchorg/stable',
    )

    def configure_cmake(self):
        cmake = CMake(self, generator='Ninja')
        cmake.configure()
        return cmake

    def build(self):
        cmake = self.configure_cmake()
        cmake.build()

    def package(self):
        cmake = self.configure_cmake()
        cmake.install()

    def package_info(self):
        self.info.header_only()
