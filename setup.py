from pybind11.setup_helpers import Pybind11Extension, build_ext
from pybind11 import get_cmake_dir
import pybind11
from setuptools import setup, Extension
import glob

# C++ソースファイルを収集
cpp_sources = (
    glob.glob("cpp/core/*.cpp") +
    glob.glob("cpp/ai/*.cpp") + 
    glob.glob("cpp/bindings/*.cpp")
)

ext_modules = [
    Pybind11Extension(
        "puyo_ai_platform",
        cpp_sources,
        include_dirs=[
            "cpp",
        ],
        cxx_std=17,
    ),
]

setup(
    name="puyo_ai_platform",
    version="0.1.0",
    description="Puyo Puyo AI Development Platform",
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
    packages=["python"],
    python_requires=">=3.7",
    install_requires=[
        "pybind11>=2.6.0",
        "numpy",
    ],
)