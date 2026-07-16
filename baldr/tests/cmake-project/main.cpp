#include <iostream>

int main(int argc, char** argv) {
    std::cout << "Hello from the mini CMake-based project!\n";
    for (int i = 1; i < argc; ++i) {
        std::cout << "arg[" << i << "]=" << argv[i] << "\n";
    }
    return 0;
}
