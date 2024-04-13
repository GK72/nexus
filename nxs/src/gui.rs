//! Immediate GUI module using ImGui Rust bindings and OpenGL as a backend.
//!
//! It is an MVP (Minimal Viable Product) for using it in a sandbox.

use std::time::Instant;
use glium::{Display, Frame, Surface};
use glium::glutin::surface::WindowSurface;
use imgui::{Context, Ui};
use imgui_glium_renderer::Renderer;
use imgui_winit_support::winit::dpi::LogicalSize;
use imgui_winit_support::winit::event::{Event, WindowEvent};
use imgui_winit_support::winit::event_loop::{EventLoop, EventLoopWindowTarget};
use imgui_winit_support::winit::window::{Window, WindowBuilder};
use imgui_winit_support::{HiDpiMode, WinitPlatform};

/// Implementation details for backend and renderer.
struct GuiInternal {
    window: Window,
    imgui: Context,
    platform: WinitPlatform,
    renderer: Renderer
}

/// Main wrapper around backend and renderer.
///
/// Based on: <https://github.com/imgui-rs/imgui-rs/blob/main/imgui-examples/examples/support/mod.rs>
pub struct Gui {
    pub display: Display<WindowSurface>,
    event_loop: EventLoop<()>,
    gui_internal: GuiInternal
}

impl Gui {

    /// Creates a new GUI instance.
    ///
    /// MVP.
    ///
    /// ## Panics
    ///
    /// In case it is called multiple times.
    pub fn new(title: &str) -> Self {
        let event_loop = EventLoop::new()
            .expect("Event loop creation failed: it must be called only once and from the main thread");

        let builder = WindowBuilder::new()
            .with_title(title)
            .with_inner_size(LogicalSize::new(1024, 768));

        let (window, display) = glium::backend::glutin::SimpleWindowBuilder::new()
            .set_window_builder(builder)
            .build(&event_loop);

        let mut imgui = Context::create();
        let mut platform = WinitPlatform::init(&mut imgui);
        platform.attach_window(imgui.io_mut(), &window, HiDpiMode::Default);

        let renderer = Renderer::init(&mut imgui, &display)
            .expect("Failed to initialize the renderer!");

        Gui {
            display,
            event_loop,
            gui_internal: GuiInternal { window, imgui, platform, renderer },
        }
    }

    /// Runs the given function in an [`EventLoop`].
    ///
    /// The function which provides the content to draw receives a keep alive flag and the UI.
    ///
    /// MVP.
    ///
    /// ## Panics
    ///
    /// The function may panic for various reasons from the underlying libraries.
    /// Consult the documentation of error types for more information.
    pub fn event_loop<F>(self, mut content: F)
    where F: FnMut(&mut bool, &mut Ui, &mut Frame)
    {
        let Gui {
            display,
            event_loop,
            mut gui_internal
        } = self;

        let mut last_update = Instant::now();

        event_loop.run(move |event, window| match event {
            Event::NewEvents(_) => {
                let now = Instant::now();
                gui_internal.imgui.io_mut().update_delta_time(now - last_update);
                last_update = now;
            }
            Event::AboutToWait => {
                gui_internal.platform.prepare_frame(gui_internal.imgui.io_mut(), &gui_internal.window)
                    .expect("Failed to prepare frame!");

                gui_internal.window.request_redraw();
            }
            Event::WindowEvent {
                event: WindowEvent::RedrawRequested,
                ..
            } => {
                Gui::redraw(&mut gui_internal, &mut content, window, &display);
            }
            Event::WindowEvent {
                event: WindowEvent::Resized(new_size),
                ..
            } => {
                if new_size.width > 0 && new_size.height > 0 {
                    display.resize((new_size.width, new_size.height));
                }
                gui_internal.platform.handle_event(gui_internal.imgui.io_mut(), &gui_internal.window, &event);
            }
            Event::WindowEvent {
                event: WindowEvent::CloseRequested,
                ..
            } => window.exit(),
            event => {
                gui_internal.platform.handle_event(gui_internal.imgui.io_mut(), &gui_internal.window, &event);
            }
        }).expect("Event loop error");
    }

    /// Prepare and redraw a frame based on `content`.
    ///
    /// ## Panics
    ///
    /// In case the rendering or swapping the buffers fails.
    /// See [`glium::SwapBuffersError`] for more details.
    fn redraw<F>(
        gui_impl: &mut GuiInternal,
        content: &mut F,
        window: &EventLoopWindowTarget<()>,
        display: &Display<WindowSurface>
    )
    where F: FnMut(&mut bool, &mut Ui, &mut Frame)
    {
        let ui = gui_impl.imgui.frame();

        let mut frame = display.draw();
        frame.clear_color_srgb(0.0, 0.0, 0.0, 0.0);

        let mut keep_alive = true;
        content(&mut keep_alive, ui, &mut frame);
        if !keep_alive {
            frame.finish().expect("Failed to swap buffers!");
            window.exit();
            return;
        }

        gui_impl.platform.prepare_render(ui, &gui_impl.window);
        let draw_data = gui_impl.imgui.render();
        gui_impl.renderer.render(&mut frame, draw_data).expect("Failed to render!");
        frame.finish().expect("Failed to swap buffers!");
    }
}

/// Traits used by vectors.
///
/// Use <https://docs.rs/num/latest/num/trait.Float.html> where a complete trait implementation is
/// needed.
pub trait Float: Copy {}

impl Float for f32 {}

/// A three-dimensional vector.
pub struct Vec3<T: Float> {
    data: (T, T, T)
}

impl<T: Float> Vec3<T> {
    pub fn new(x: T, y: T, z: T) -> Self {
        Self { data: (x, y, z) }
    }
    pub fn x(& self) -> T { self.data.0 }
    pub fn y(& self) -> T { self.data.1 }
    pub fn z(& self) -> T { self.data.2 }
    pub fn mut_x(&mut self) -> &mut T { &mut self.data.0 }
    pub fn mut_y(&mut self) -> &mut T { &mut self.data.1 }
    pub fn mut_z(&mut self) -> &mut T { &mut self.data.2 }
}


/// A 3D camera.
///
/// MVP.
///
/// ## Reference
///
/// <https://github.com/glium/glium/blob/master/examples/support/camera.rs>
pub struct Camera {
    aspect_ratio: f32,
    position: Vec3<f32>,
    direction: Vec3<f32>,
    fov: f32,
    far: f32,
    near: f32,
}

impl Camera {
    pub fn new(width: i32, height: i32) -> Self {
        Self {
            aspect_ratio: (width / height) as f32,
            position: Vec3::new(0.1, 0.1, 1.0),
            direction: Vec3::new(0.0, 0.0, -1.0),
            fov: 1.8,
            far: 1024.0,
            near: 0.1,
        }
    }

    pub fn position(&mut self) -> &mut Vec3<f32> {
        &mut self.position
    }

    pub fn perspective(&self) -> [[f32; 4]; 4] {
        let f = 1.0 / (self.fov / 2.0).tan();

        [
            [f / self.aspect_ratio,    0.0,              0.0                  ,   0.0],
            [         0.0         ,     f ,              0.0                  ,   0.0],
            [         0.0         ,    0.0,  (self.far + self.near) / (self.far - self.near)      ,   1.0],
            [         0.0         ,    0.0, -(2.0 * self.far * self.near) / (self.far - self.near),   0.0],
        ]
    }

    pub fn view(&self) -> [[f32; 4]; 4] {
        let f = {
            let f = &self.direction;
            let len = f.x() * f.x() + f.y() * f.y() + f.z() * f.z();
            let len = len.sqrt();
            (f.x() / len, f.y() / len, f.z() / len)
        };

        let up = (0.0, 1.0, 0.0);

        let s = (f.1 * up.2 - f.2 * up.1,
                 f.2 * up.0 - f.0 * up.2,
                 f.0 * up.1 - f.1 * up.0);

        let s_norm = {
            let len = s.0 * s.0 + s.1 * s.1 + s.2 * s.2;
            let len = len.sqrt();
            (s.0 / len, s.1 / len, s.2 / len)
        };

        let u = (s_norm.1 * f.2 - s_norm.2 * f.1,
                 s_norm.2 * f.0 - s_norm.0 * f.2,
                 s_norm.0 * f.1 - s_norm.1 * f.0);

        let p = (-self.position.x() * s.0 - self.position.y() * s.1 - self.position.z() * s.2,
                 -self.position.x() * u.0 - self.position.y() * u.1 - self.position.z() * u.2,
                 -self.position.x() * f.0 - self.position.y() * f.1 - self.position.z() * f.2);

        // note: remember that this is column-major, so the lines of code are actually columns
        [
            [s_norm.0, u.0, f.0, 0.0],
            [s_norm.1, u.1, f.1, 0.0],
            [s_norm.2, u.2, f.2, 0.0],
            [p.0, p.1,  p.2, 1.0],
        ]
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn vec3() {
        let vec = Vec3::new(0.0, 0.0, 0.0);
        assert_eq!(vec.x(), 0.0);
        assert_eq!(vec.y(), 0.0);
        assert_eq!(vec.z(), 0.0);
    }
}