from setuptools import setup
from pybind11.setup_helpers import Pybind11Extension, build_ext

ext_modules = [
    Pybind11Extension(
        "test_module",
        ["test_pybind_basic.cpp"],
        cxx_std=17,
    ),
]

setup(
    name="test_module",
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
)