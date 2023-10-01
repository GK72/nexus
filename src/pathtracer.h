#pragma once

#include "types.h"

#include <nova/vec.h>

#include <vector>

namespace nxs {

class pathtracer {
public:
    struct config {
        int sampling = 1;
    };


    pathtracer(image& img, const camera& cam, const std::vector<primitive>& primitives)
        : m_image(img)
        , m_cam(cam)
        , m_primitives(primitives)
    {}

    void update();
    auto& config() { return m_config; }

private:
    image& m_image;
    const camera& m_cam;
    const std::vector<primitive>& m_primitives;

    struct config m_config;

    nova::Color sample(int n, int x, int y);
};

} // namespace nxs
