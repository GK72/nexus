#[macro_use]
extern crate glium;

use glium::{
    Display,
    Frame,
    Program,
    Surface,
    VertexBuffer,
    glutin::{
        surface::WindowSurface
    },
    vertex::VertexBufferAny
};

use nxs::utils;
use nxs::gui;
use nxs::gui::{Camera};

fn shader_3d(display: &Display<WindowSurface>) -> Program {
    program!(display,
        140 => {
            vertex: "
                #version 140

                uniform mat4 persp_matrix;
                uniform mat4 view_matrix;

                in vec3 position;
                in vec3 normal;
                out vec3 v_position;
                out vec3 v_normal;

                void main() {
                    v_position = position;
                    v_normal = normal;
                    gl_Position = persp_matrix * view_matrix * vec4(v_position * 0.005, 1.0);
                }
            ",

            fragment: "
                #version 140

                in vec3 v_normal;
                out vec4 f_color;

                const vec3 LIGHT = vec3(-0.2, 0.8, 0.1);

                void main() {
                    float lum = max(dot(normalize(v_normal), normalize(LIGHT)), 0.0);
                    vec3 color = (0.3 + 0.7 * lum) * vec3(1.0, 1.0, 1.0);
                    f_color = vec4(color, 1.0);
                }
            ",
        },
    )
    .unwrap()
}

fn shader_2d(display: &Display<WindowSurface>) -> Program {
    program!(display,
        100 => {
            vertex: "
                #version 100

                uniform lowp mat4 matrix;

                attribute lowp vec2 position;
                attribute lowp vec3 color;

                varying lowp vec3 vColor;

                void main() {
                    gl_Position = vec4(position, 0.0, 1.0) * matrix;
                    vColor = color;
                }
            ",

            fragment: "
                #version 100
                varying lowp vec3 vColor;

                void main() {
                    gl_FragColor = vec4(vColor, 1.0);
                }
            ",
        },
    )
    .unwrap()
}

#[derive(Copy, Clone)]
struct Vertex2D {
    position: [f32; 2],
    color: [f32; 3],
}

implement_vertex!(Vertex2D, position, color);

fn create_hud(display: &Display<WindowSurface>) -> VertexBufferAny {
    VertexBuffer::new(
        display,
        &[
            Vertex2D {
                position: [0.5, 0.5],
                color: [0.0, 1.0, 0.0],
            },
            Vertex2D {
                position: [0.7, 0.8],
                color: [0.0, 0.0, 1.0],
            },
            Vertex2D {
                position: [0.9, 0.5],
                color: [1.0, 0.0, 0.0],
            },
        ],
    )
    .unwrap().into()
}

fn main() {
    let gui = gui::Gui::new("Sandbox");

    let s3d = shader_3d(&gui.display);
    let s2d = shader_2d(&gui.display);

    let mut camera = Camera::new(640, 480);

    let scene = utils::load_wavefront(&gui.display, include_bytes!("cube.obj"));
    let hud = create_hud(&gui.display);

    gui.event_loop(move |_keep_alive, ui, frame| {
        ui.window("Debug")
            .build(|| {
                let mouse_pos = ui.io().mouse_pos;
                ui.text(format!("Mouse position: {:.1} {:.1}", mouse_pos[0], mouse_pos[1]));
            });

        ui.window("Controls")
            .build(|| {
                ui.text_wrapped("Hello GUI");

                ui.slider("Camera x", -10.0f32, 10.0f32, camera.position().mut_x());
                ui.slider("Camera y", -10.0f32, 10.0f32, camera.position().mut_y());
                ui.slider("Camera z", -10.0f32, 10.0f32, camera.position().mut_z());
            });

        render_scene(
            frame,
            DrawData {
                camera: &camera,
                shader: &s3d,
                vertex_buffer: &scene
            }
        );

        render_hud(
            frame,
            DrawData {
                camera: &camera,
                shader: &s2d,
                vertex_buffer: &hud
            }
        );
    });
}

struct DrawData<'a> {
    camera: &'a Camera,
    shader: &'a Program,
    vertex_buffer: &'a VertexBufferAny
}

fn render_scene(frame: &mut Frame, draw_data: DrawData) {
    let uniforms = uniform! {
        persp_matrix: draw_data.camera.perspective(),
        view_matrix: draw_data.camera.view()
    };

    let ibuf = &glium::index::NoIndices(glium::index::PrimitiveType::TrianglesList);
    // let ibuf = &glium::IndexBuffer::new(&gui_impl.display, glium::index::PrimitiveType::TrianglesList, &[0u16, 1, 2]).unwrap();

    frame.draw(
        draw_data.vertex_buffer,
        ibuf,
        draw_data.shader,
        &uniforms,
        &Default::default(),
    ).unwrap();
}

fn render_hud(frame: &mut Frame, draw_data: DrawData) {
    let uniforms = uniform! {
         matrix: [
            [1.0, 0.0, 0.0, 0.0],
            [0.0, 1.0, 0.0, 0.0],
            [0.0, 0.0, 1.0, 0.0],
            [0.0, 0.0, 0.0, 1.0f32]
        ]
    };

    let ibuf = &glium::index::NoIndices(glium::index::PrimitiveType::TrianglesList);

    frame.draw(
        draw_data.vertex_buffer,
        ibuf,
        draw_data.shader,
        &uniforms,
        &Default::default(),
    ).unwrap();
}
