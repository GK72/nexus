#include "chai_bindings.h"
#include "nxs/ph.h"

#include <nova/utils.h>
#include <fmt/format.h>
#include <cstdlib>

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    if (argc != 2) {
        fmt::print("Missing argument: Chaiscript filepath\n");
        return EXIT_FAILURE;
    }

    auto chai = nxs::chai();
    chai.eval_file(argv[1]);

    // Usage of directly pulling in CMake target from dependency
    if (nxs::ph() == nova::ph()) {
        return EXIT_SUCCESS;
    }
}
