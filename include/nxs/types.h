#pragma once

#include "nxs/type_traits.h"

#include <algorithm>
#include <expected>
#include <cmath>
#include <memory_resource>
#include <stdint.h>
#include <vector>

namespace nxs {

struct error {
    std::string msg;
    operator std::string() { return msg; }
};

template <class E> using expected = std::expected<E, error>;
using unexpected = std::unexpected<error>;
using bad_expected_access = std::bad_expected_access<error>;

template <class T> using vector = std::pmr::vector<T>;

} // namespace nxs
