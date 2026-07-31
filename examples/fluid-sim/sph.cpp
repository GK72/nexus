// Minimal 2D smoothed-particle hydrodynamics (SPH) fluid simulation.
//
// Classic dam-break demo: a block of particles is dropped into a box under
// gravity, interacts through pressure and viscosity forces, and settles
// into a fluid layer. Rendered as ASCII animation directly in the terminal
// -- no external dependencies beyond the standard library.
//
// Physics follows Muller, Charypar & Gross, "Particle-Based Fluid
// Simulation for Interactive Applications" (2003): density/pressure from a
// poly6 kernel, pressure force from a spiky kernel gradient, viscosity
// force from a viscosity kernel laplacian, integrated with semi-implicit
// Euler and reflective box boundaries.
//
// Build & run:
//   g++ -O2 -std=c++20 sph.cpp -o sph && ./sph

#include <chrono>
#include <cmath>
#include <cstdio>
#include <numbers>
#include <thread>
#include <vector>

namespace {

struct vec2 {
    float x = 0.f;
    float y = 0.f;

    vec2 operator+(const vec2& o) const { return {x + o.x, y + o.y}; }
    vec2 operator-(const vec2& o) const { return {x - o.x, y - o.y}; }
    vec2 operator*(float s) const { return {x * s, y * s}; }
    vec2& operator+=(const vec2& o) { x += o.x; y += o.y; return *this; }

    float length_sq() const { return x * x + y * y; }
    float length() const { return std::sqrt(length_sq()); }
};

struct particle {
    vec2 pos;
    vec2 vel;
    vec2 force;
    float density = 0.f;
    float pressure = 0.f;
};

// -- Simulation domain (arbitrary length units) -----------------------------
constexpr float view_width = 120.f;
constexpr float view_height = 80.f;
constexpr float boundary_eps = 1.f;
constexpr float boundary_damping = -0.4f;

// -- Physical constants (tuned for stability at this scale) -----------------
constexpr float h = 4.f;              // smoothing radius
constexpr float h_sq = h * h;
constexpr float mass = 1.f;
constexpr float rest_density = 2.f;
constexpr float gas_constant = 8.f;
constexpr float viscosity = 3.f;
constexpr float dt = 0.01f;
constexpr vec2 gravity{0.f, -6.f};

const float pi = std::numbers::pi_v<float>;
// 2D poly6 kernel (density) and its associated spiky/viscosity kernels
// (pressure force, viscosity force), normalized for a smoothing radius h.
const float poly6_coef = 4.f / (pi * std::pow(h, 8.f));
const float spiky_grad_coef = -30.f / (pi * std::pow(h, 5.f));
const float visc_lap_coef = 40.f / (pi * std::pow(h, 5.f));

void compute_density_pressure(std::vector<particle>& particles) {
    for (auto& pi : particles) {
        pi.density = 0.f;
        for (const auto& pj : particles) {
            const float r_sq = (pj.pos - pi.pos).length_sq();
            if (r_sq < h_sq) {
                const float term = h_sq - r_sq;
                pi.density += mass * poly6_coef * term * term * term;
            }
        }
        pi.pressure = gas_constant * (pi.density - rest_density);
    }
}

void compute_forces(std::vector<particle>& particles) {
    for (auto& pi : particles) {
        vec2 pressure_force{};
        vec2 viscosity_force{};
        for (const auto& pj : particles) {
            if (&pi == &pj) {
                continue;
            }
            const vec2 diff = pj.pos - pi.pos;
            const float r = diff.length();
            if (r < h && r > 1e-5f) {
                const vec2 dir = diff * (1.f / r);
                const float hr = h - r;

                pressure_force += dir * (-mass * (pi.pressure + pj.pressure) /
                                          (2.f * pj.density) * spiky_grad_coef * hr * hr);

                viscosity_force += (pj.vel - pi.vel) *
                                    (viscosity * mass / pj.density * visc_lap_coef * hr);
            }
        }
        pi.force = pressure_force + viscosity_force + gravity * pi.density;
    }
}

void integrate(std::vector<particle>& particles) {
    for (auto& p : particles) {
        p.vel += p.force * (dt / p.density);
        p.pos += p.vel * dt;

        if (p.pos.x < boundary_eps) {
            p.vel.x *= boundary_damping;
            p.pos.x = boundary_eps;
        }
        if (p.pos.x > view_width - boundary_eps) {
            p.vel.x *= boundary_damping;
            p.pos.x = view_width - boundary_eps;
        }
        if (p.pos.y < boundary_eps) {
            p.vel.y *= boundary_damping;
            p.pos.y = boundary_eps;
        }
        if (p.pos.y > view_height - boundary_eps) {
            p.vel.y *= boundary_damping;
            p.pos.y = view_height - boundary_eps;
        }
    }
}

std::vector<particle> make_dam_break() {
    std::vector<particle> particles;
    constexpr float spacing = h * 0.5f;
    for (float y = boundary_eps + spacing; y < view_height * 0.7f; y += spacing) {
        for (float x = boundary_eps + spacing; x < view_width * 0.4f; x += spacing) {
            particles.push_back(particle{.pos = {x, y}, .vel = {}, .force = {}});
        }
    }
    return particles;
}

// Coarse ASCII framebuffer the simulation domain is rasterized into.
constexpr int grid_cols = 80;
constexpr int grid_rows = 30;

void render(const std::vector<particle>& particles, int step) {
    static char grid[grid_rows][grid_cols + 1];
    for (auto& row : grid) {
        for (int c = 0; c < grid_cols; ++c) {
            row[c] = '.';
        }
        row[grid_cols] = '\0';
    }

    for (const auto& p : particles) {
        const int col = static_cast<int>(p.pos.x / view_width * grid_cols);
        const int row = static_cast<int>(p.pos.y / view_height * grid_rows);
        if (col >= 0 && col < grid_cols && row >= 0 && row < grid_rows) {
            // Flip vertically: simulation y grows upward, terminal rows grow downward.
            grid[grid_rows - 1 - row][col] = 'o';
        }
    }

    std::printf("\033[2J\033[H");
    std::printf("SPH fluid simulation -- step %d, %zu particles\n", step, particles.size());
    for (const auto& row : grid) {
        std::printf("%s\n", row);
    }
    std::fflush(stdout);
}

}  // namespace

int main() {
    std::vector<particle> particles = make_dam_break();

    constexpr int total_steps = 2000;
    constexpr int render_every = 4;

    for (int step = 0; step < total_steps; ++step) {
        compute_density_pressure(particles);
        compute_forces(particles);
        integrate(particles);

        if (step % render_every == 0) {
            render(particles, step);
            std::this_thread::sleep_for(std::chrono::milliseconds(12));
        }
    }

    render(particles, total_steps);
    return 0;
}
