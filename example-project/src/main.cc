#include <nxs/data.h>

#include <array>

int main() {
    const auto data = nxs::data_view(std::to_array({ 1 }));
}
