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
    where F: FnMut(&mut bool, &mut Ui, &mut Frame, &Display<WindowSurface>)
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
    where F: FnMut(&mut bool, &mut Ui, &mut Frame, &Display<WindowSurface>)
    {
        let ui = gui_impl.imgui.frame();

        let mut frame = display.draw();
        frame.clear_color_srgb(0.0, 0.0, 0.0, 0.0);
        
        let mut keep_alive = true;
        content(&mut keep_alive, ui, &mut frame, display);
        if !keep_alive {
            frame.finish();
            window.exit();
            return;
        }

        gui_impl.platform.prepare_render(ui, &gui_impl.window);
        let draw_data = gui_impl.imgui.render();
        gui_impl.renderer.render(&mut frame, draw_data).expect("Failed to render!");
        frame.finish().expect("Failed to swap buffers!");
    }
}