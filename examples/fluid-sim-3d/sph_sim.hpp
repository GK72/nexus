// Minimal 3D smoothed-particle hydrodynamics (SPH) fluid simulation.
//
// Header-only and rendering-agnostic on purpose: this file has no
// dependency on any graphics library, so the front-end (currently raylib,
// see main.cpp) can be swapped later -- e.g. for GLFW+OpenGL -- without
// touching the physics.
//
// Follows Muller, Charypar & Gross, "Particle-Based Fluid Simulation for
// Interactive Applications" (2003): density/pressure from a poly6 kernel,
// pressure force from a spiky kernel gradient, viscosity force from a
// viscosity kernel laplacian, integrated with semi-implicit Euler and
// reflective box boundaries.

#pragma once

#include <cmath>
#include <numbers>
#include <vector>

namespace sph3d {

struct vec3 {
  float x = 0.f;
  float y = 0.f;
  float z = 0.f;

  vec3 operator+(const vec3& o) const { return { x + o.x, y + o.y, z + o.z }; }
  vec3 operator-(const vec3& o) const { return { x - o.x, y - o.y, z - o.z }; }
  vec3 operator*(float s) const { return { x * s, y * s, z * s }; }
  vec3& operator+=(const vec3& o) {
    x += o.x;
    y += o.y;
    z += o.z;
    return *this;
  }

  float length_sq() const { return x * x + y * y + z * z; }
  float length() const { return std::sqrt(length_sq()); }
};

struct particle {
  vec3 pos;
  vec3 vel;
  vec3 force;
  float density  = 0.f;
  float pressure = 0.f;
};

// -- Simulation domain (arbitrary length units) -----------------------------
constexpr float domain_size      = 20.f;
constexpr float boundary_eps     = 0.5f;
constexpr float boundary_damping = -0.4f;

// -- Physical constants (tuned for stability at this scale) -----------------
constexpr float h    = 1.4f; // smoothing radius
constexpr float h_sq = h * h;
constexpr float mass = 0.6f;
constexpr float rest_density =
    1.3f; // matches a lattice packed at spacing h*0.55
constexpr float gas_constant = 60.f;
constexpr float viscosity    = 6.f;
constexpr float dt           = 0.0006f;
constexpr vec3 gravity{ 0.f, -9.f, 0.f };

inline const float pi = std::numbers::pi_v<float>;
// Standard 3D SPH kernel normalizations (Muller et al. 2003), for smoothing
// radius h.
inline const float poly6_coef      = 315.f / (64.f * pi * std::pow(h, 9.f));
inline const float spiky_grad_coef = -45.f / (pi * std::pow(h, 6.f));
inline const float visc_lap_coef   = 45.f / (pi * std::pow(h, 6.f));

inline void compute_density_pressure(std::vector<particle>& particles) {
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

inline void compute_forces(std::vector<particle>& particles) {
  for (auto& pi : particles) {
    vec3 pressure_force{};
    vec3 viscosity_force{};
    for (const auto& pj : particles) {
      if (&pi == &pj) {
        continue;
      }
      const vec3 diff = pj.pos - pi.pos;
      const float r   = diff.length();
      if (r < h && r > 1e-5f) {
        const vec3 dir = diff * (1.f / r);
        const float hr = h - r;

        pressure_force += dir
                          * (-mass * (pi.pressure + pj.pressure)
                             / (2.f * pj.density) * spiky_grad_coef * hr * hr);

        viscosity_force +=
            (pj.vel - pi.vel)
            * (viscosity * mass / pj.density * visc_lap_coef * hr);
      }
    }
    pi.force = pressure_force + viscosity_force + gravity * pi.density;
  }
}

// Mild global velocity damping. Compensates for the energy drift inherent
// to a simple explicit (semi-implicit Euler) integrator over long runs --
// without it the fluid never fully settles and slowly gains energy.
constexpr float global_damping = 0.997f;

inline void integrate(std::vector<particle>& particles) {
  for (auto& p : particles) {
    p.vel += p.force * (dt / p.density);
    p.vel = p.vel * global_damping;
    p.pos += p.vel * dt;

    auto reflect = [](float& pos, float& vel, float lo, float hi) {
      if (pos < lo) {
        vel *= boundary_damping;
        pos = lo;
      }
      if (pos > hi) {
        vel *= boundary_damping;
        pos = hi;
      }
    };
    reflect(p.pos.x, p.vel.x, boundary_eps, domain_size - boundary_eps);
    reflect(p.pos.y, p.vel.y, boundary_eps, domain_size - boundary_eps);
    reflect(p.pos.z, p.vel.z, boundary_eps, domain_size - boundary_eps);
  }
}

inline void step(std::vector<particle>& particles) {
  compute_density_pressure(particles);
  compute_forces(particles);
  integrate(particles);
}

// Dam-break initial condition: a block of particles in one corner of the
// domain, spaced to roughly match rest density at equilibrium.
inline std::vector<particle> make_dam_break() {
  std::vector<particle> particles;
  constexpr float spacing = h * 0.55f;
  constexpr float block   = domain_size * 0.25f;
  for (float x = boundary_eps + spacing; x < boundary_eps + block;
       x += spacing) {
    for (float y = boundary_eps + spacing; y < domain_size * 0.55f;
         y += spacing) {
      for (float z = boundary_eps + spacing; z < boundary_eps + block;
           z += spacing) {
        particles.push_back(
            particle{ .pos = { x, y, z }, .vel = {}, .force = {} });
      }
    }
  }
  return particles;
}

} // namespace sph3d
