#pragma once

#include "camera.h"

#include <nova/vec.h>

#include <vector>

namespace nxs {

void pathtracer(image& img, const camera& cam, const std::vector<primitive>& primitives);

} // namespace nxs
