// Minimal 3D SPH fluid simulation -- raylib front-end.
//
// This is the ONLY file that touches raylib; the physics lives in
// sph_sim.hpp with no rendering dependency, so swapping the front-end
// later (e.g. for GLFW+OpenGL) means replacing just this file.
//
// Build & run: via the repo's CMake/Conan build, target `fluid-sim-3d`
// (see baldr usage in the repo README), or manually:
//   conan install . --build=missing && cmake --build build/<preset>
//   --target fluid-sim-3d
//
// Simulation speed (physics substeps per rendered frame) can be set at
// startup with `--substeps N` (default 4), or adjusted live with Up/Down.
//
// The window opens on whatever monitor GLFW/the window manager picks by
// default. Connected monitors are listed on stdout at startup (index, name,
// resolution) -- pass `--monitor N` to open on a specific one instead.

#include "sph_sim.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <raylib.h>

namespace {

Vector3 to_render_pos(const sph3d::vec3& p) {
  // Recenter the simulation domain [0, domain_size]^3 onto raylib's
  // default grid, which is centered at the world origin.
  return { p.x - sph3d::domain_size * 0.5f, p.y,
           p.z - sph3d::domain_size * 0.5f };
}

// Small filled-circle texture used to draw each particle as a camera-facing
// billboard.
Texture2D make_particle_texture() {
  Image image = GenImageColor(32, 32, BLANK);
  ImageDrawCircle(&image, 16, 16, 14, Color{ 80, 160, 255, 255 });
  Texture2D texture = LoadTextureFromImage(image);
  UnloadImage(image);
  return texture;
}

// Flip to draw instanced-looking 3D spheres instead of billboards -- a
// one-line change, ahead of a proper instanced-mesh renderer.
constexpr bool render_as_spheres = true;
constexpr float particle_radius  = 0.35f;

// Simulation speed = physics substeps run per rendered frame. Adjustable at
// startup via `--substeps N` and live via Up/Down (see main loop).
constexpr int default_substeps = 4;
constexpr int min_substeps     = 1;
constexpr int max_substeps     = 60;

struct cli_options {
  int substeps = default_substeps;
  int monitor  = -1; // -1 = unspecified, keep the window manager's choice
};

cli_options parse_args(int argc, char** argv) {
  cli_options opts;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--substeps") == 0 && i + 1 < argc) {
      opts.substeps =
          std::clamp(std::atoi(argv[++i]), min_substeps, max_substeps);
    } else if (std::strcmp(argv[i], "--monitor") == 0 && i + 1 < argc) {
      opts.monitor = std::atoi(argv[++i]);
    }
  }
  return opts;
}

} // namespace

int main(int argc, char** argv) {
  const cli_options opts = parse_args(argc, argv);
  int substeps_per_frame = opts.substeps;

  InitWindow(1024, 768, "Minimal 3D SPH fluid simulation");
  SetTargetFPS(60);

  const int monitor_count = GetMonitorCount();
  for (int m = 0; m < monitor_count; ++m) {
    TraceLog(LOG_INFO, "Monitor %d: %s (%dx%d)%s", m, GetMonitorName(m),
             GetMonitorWidth(m), GetMonitorHeight(m),
             m == GetCurrentMonitor() ? " [current]" : "");
  }
  if (opts.monitor >= 0) {
    if (opts.monitor < monitor_count) {
      SetWindowMonitor(opts.monitor);
    } else {
      TraceLog(LOG_WARNING,
               "--monitor %d does not exist (%d monitor(s) found); ignoring.",
               opts.monitor, monitor_count);
    }
  }

  Camera3D camera{
    .position   = { 16.f, 14.f, 16.f },
    .target     = { 0.f, 3.f, 0.f },
    .up         = { 0.f, 1.f, 0.f },
    .fovy       = 45.f,
    .projection = CAMERA_PERSPECTIVE,
  };

  const Texture2D particle_texture = make_particle_texture();
  auto particles                   = sph3d::make_dam_break();

  while (!WindowShouldClose()) {
    UpdateCamera(&camera, CAMERA_ORBITAL);

    if (IsKeyPressed(KEY_UP)) {
      substeps_per_frame = std::min(substeps_per_frame + 1, max_substeps);
    }
    if (IsKeyPressed(KEY_DOWN)) {
      substeps_per_frame = std::max(substeps_per_frame - 1, min_substeps);
    }

    for (int i = 0; i < substeps_per_frame; ++i) {
      sph3d::step(particles);
    }

    BeginDrawing();
    ClearBackground(RAYWHITE);

    BeginMode3D(camera);
    DrawGrid(20, 1.f);
    DrawCubeWires({ 0.f, sph3d::domain_size * 0.5f, 0.f }, sph3d::domain_size,
                  sph3d::domain_size, sph3d::domain_size, LIGHTGRAY);

    for (const auto& p : particles) {
      const Vector3 pos = to_render_pos(p.pos);
      if constexpr (render_as_spheres) {
        DrawSphere(pos, particle_radius, Color{ 80, 160, 255, 255 });
      } else {
        DrawBillboard(camera, particle_texture, pos, particle_radius * 2.f,
                      WHITE);
      }
    }
    EndMode3D();

    DrawFPS(10, 10);
    DrawText(TextFormat("%zu particles", particles.size()), 10, 32, 20,
             DARKGRAY);
    DrawText(TextFormat("substeps/frame: %d  (Up/Down to change)",
                        substeps_per_frame),
             10, 54, 20, DARKGRAY);
    EndDrawing();
  }

  UnloadTexture(particle_texture);
  CloseWindow();
  return 0;
}
