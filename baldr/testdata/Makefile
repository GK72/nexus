# Minimal Makefile-based C++ project, used to manually exercise `baldr build`
# against a non-CMake build system.

CXX ?= c++
CXXFLAGS ?= -std=c++20 -Wall -Wextra

hello: main.cpp
	$(CXX) $(CXXFLAGS) -o hello main.cpp

.PHONY: clean
clean:
	rm -f hello
