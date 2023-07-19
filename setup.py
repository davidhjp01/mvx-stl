import os
import pathlib
import platform
import re
import shutil
import subprocess
import sys
from distutils.version import LooseVersion

from setuptools import Extension, setup, find_packages
from setuptools.command.build_ext import build_ext
from setuptools.command.sdist import sdist


class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=""):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuild(build_ext):
    def run(self):
        try:
            out = subprocess.check_output(["cmake", "--version"])
        except OSError:
            raise RuntimeError(
                "CMake must be installed to build the following extensions: "
                + ", ".join(e.name for e in self.extensions)
            )

        if platform.system() == "Windows":
            cmake_version = LooseVersion(
                re.search(r"version\s*([\d.]+)", out.decode()).group(1)
            )
            if cmake_version < "3.1.0":
                raise RuntimeError("CMake >= 3.1.0 is required on Windows")

        self._use_ninja = True
        try:
            out = subprocess.check_output(["ninja", "--version"])
        except OSError:
            self._use_ninja = False

        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
        # required for auto-detection of auxiliary "native" libs
        if not extdir.endswith(os.path.sep):
            extdir += os.path.sep

        cmake_args = [
            "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=" + extdir,
            "-DPYTHON_EXECUTABLE=" + sys.executable,
            "-DSIGNALTL_TEST=OFF",
            "-DSIGNALTL_COVERAGE=OFF",
            "-DSIGNALTL_EXAMPLES=OFF",
            "-DSIGNALTL_INSTALL=OFF",
        ]

        if self._use_ninja:
            print("using Ninja")
            cmake_args += ["-GNinja"]

        cfg = "Debug" if self.debug else "Release"
        build_args = ["--config", cfg]

        if platform.system() == "Windows":
            cmake_args += [
                "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{}={}".format(cfg.upper(), extdir)
            ]
        else:
            cmake_args += ["-DCMAKE_BUILD_TYPE=" + cfg]

        if not self._use_ninja:
            if platform.system() == "Windows":
                if sys.maxsize > 2 ** 32:
                    cmake_args += ["-A", "x64"]
                build_args += ["--", "/m"]
            else:
                build_args += ["--", "-j4"]

        env = os.environ.copy()
        env["CXXFLAGS"] = '{} -DVERSION_INFO=\\"{}\\"'.format(
            env.get("CXXFLAGS", ""), self.distribution.get_version()
        )
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        subprocess.check_call(
            ["cmake", ext.sourcedir] + cmake_args, cwd=self.build_temp, env=env
        )
        subprocess.check_call(
            ["cmake", "--build", "."] + build_args, cwd=self.build_temp, env=env
        )


class SdistBuild(sdist):
    def run(self) -> None:
        print("Git submodule update..")
        subprocess.check_call(["git", "submodule", "update", "--init"])
        super().run()


setup(
    ext_modules=[CMakeExtension("signal_tl._cext")],
    packages=find_packages(),
    cmdclass=dict(build_ext=CMakeBuild, sdist=SdistBuild)
)
