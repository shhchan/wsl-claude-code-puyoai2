#include <pybind11/pybind11.h>

int add(int i, int j) {
    return i + j;
}

PYBIND11_MODULE(test_module, m) {
    m.doc() = "pybind11 test plugin";
    m.def("add", &add, "A function which adds two numbers");
}