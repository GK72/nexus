use std::time::Instant;
use glium::{Display, Surface};
use glium::glutin::surface::WindowSurface;
use imgui::{Context, Ui};
use imgui_glium_renderer::Renderer;
use imgui_winit_support::winit::dpi::LogicalSize;
use imgui_winit_support::winit::event::{Event, WindowEvent};
use imgui_winit_support::winit::event_loop::{EventLoop, EventLoopWindowTarget};
use imgui_winit_support::winit::window::{Window, WindowBuilder};
use imgui_winit_support::{HiDpiMode, WinitPlatform};

/// Main wrapper around backend and renderer.
///
/// Based on: https://github.com/imgui-rs/imgui-rs/blob/main/imgui-examples/examples/support/mod.rs
pub struct Gui {
    pub event_loop: EventLoop<()>,
    pub window: Window,
    pub imgui: Context,
    pub platform: WinitPlatform,
    pub display: Display<WindowSurface>,
    pub renderer: Renderer
}

impl Gui {

    /// Creates a new GUI instance.
    ///
    /// MVP.
    ///
    /// ## Panics
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
            event_loop,
            window,
            imgui,
            platform,
            display,
            renderer
        }
    }

    /// Runs the given function in an [`EventLoop`].
    ///
    /// The function receives a keep alive flag and the UI.
    ///
    /// MVP.
    pub fn event_loop<F>(mut self, mut func: F)
    where F: FnMut(&mut bool, &mut Ui)
    {
        let mut last_update = Instant::now();

        self.event_loop.run(move |event, window| match event {
            Event::NewEvents(_) => {
                let now = Instant::now();
                self.imgui.io_mut().update_delta_time(now - last_update);
                last_update = now;
            }
            Event::AboutToWait => {
                self.platform.prepare_frame(self.imgui.io_mut(), &self.window)
                    .expect("Failed to prepare frame!");

                self.window.request_redraw();
            }
            Event::WindowEvent {
                event: WindowEvent::RedrawRequested,
                ..
            } => {
                let ui = self.imgui.frame();

                let mut keep_alive = true;
                func(&mut keep_alive, ui);
                if !keep_alive {
                    window.exit();
                }

                let mut frame = self.display.draw();
                frame.clear_color_srgb(0.0, 0.0, 0.0, 0.0);
                self.platform.prepare_render(ui, &self.window);

                let draw_data = self.imgui.render();

                self.renderer.render(&mut frame, draw_data).expect("Render fail");
                frame.finish().expect("Swap failed");

                // TODO(refact): find a way to factor out this block of code into a function.
                //               partial move happens because self contains `event_loop`.

                // self.redraw(&mut func, &window);
            }
            Event::WindowEvent {
                event: WindowEvent::Resized(new_size),
                ..
            } => {
                if new_size.width > 0 && new_size.height > 0 {
                    self.display.resize((new_size.width, new_size.height));
                }
                self.platform.handle_event(self.imgui.io_mut(), &self.window, &event);
            }
            Event::WindowEvent {
                event: WindowEvent::CloseRequested,
                ..
            } => window.exit(),
            event => {
                self.platform.handle_event(self.imgui.io_mut(), &self.window, &event);
            }
        }).expect("Event loop error");
    }

    #[allow(dead_code)]
    fn redraw<F>(&mut self, func: &mut F, window: &EventLoopWindowTarget<()>)
        where F: FnMut(&mut bool, &mut Ui)
    {
        let ui = self.imgui.frame();

        let mut keep_alive = true;
        func(&mut keep_alive, ui);
        if !keep_alive {
            window.exit();
        }

        let mut frame = self.display.draw();
        frame.clear_color_srgb(0.0, 0.0, 0.0, 0.0);
        self.platform.prepare_render(ui, &self.window);

        let draw_data = self.imgui.render();

        self.renderer.render(&mut frame, draw_data).expect("Render fail");
        frame.finish().expect("Swap failed");

    }
}