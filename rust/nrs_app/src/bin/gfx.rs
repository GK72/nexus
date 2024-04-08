#[macro_use]
extern crate glium;

use glium::Surface;

use nrs::gui;

fn main() {
    let gui = gui::Gui::new("Sandbox");

    let mut value = 0;

    let program = program!(&gui.display,
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
        .unwrap();

    gui.event_loop(move |_keep_alive, ui, frame, display| {
        ui.window("Controls")
            .build(|| {
                ui.text_wrapped("Hello Rust");

                if ui.button("Click me") {
                    value += 1;
                }

                ui.text(format!("Value: {}", value));

                ui.separator();

                let mouse_pos = ui.io().mouse_pos;

                ui.text(
                    format!(
                        "Mouse position: {:.1} {:.1}",
                        mouse_pos[0], mouse_pos[1]
                    )
                );
                
                let ibuf = &glium::index::NoIndices(glium::index::PrimitiveType::TrianglesList);
                // let ibuf = &glium::IndexBuffer::new(&gui_impl.display, glium::index::PrimitiveType::TrianglesList, &[0u16, 1, 2]).unwrap();
                
                #[derive(Copy, Clone)]
                struct Vertex {
                    position: [f32; 2],
                    color: [f32; 3],
                }

                implement_vertex!(Vertex, position, color);
                let vertex_buffer = {
                    glium::VertexBuffer::new(
                        display,
                        &[
                            Vertex {
                                position: [-0.5, -0.5],
                                color: [0.0, 1.0, 0.0],
                            },
                            Vertex {
                                position: [0.0, 0.5],
                                color: [0.0, 0.0, 1.0],
                            },
                            Vertex {
                                position: [0.5, -0.5],
                                color: [1.0, 0.0, 0.0],
                            },
                        ],
                    )
                        .unwrap()
                };
                let uniforms = uniform! {
                    matrix: [
                        [1.0, 0.0, 0.0, 0.0],
                        [0.0, 1.0, 0.0, 0.0],
                        [0.0, 0.0, 1.0, 0.0],
                        [0.0, 0.0, 0.0, 1.0f32]
                    ]
                };

                frame.draw(
                    &vertex_buffer,
                    ibuf,
                    &program,
                    &uniforms,
                    &Default::default(),
                ).unwrap();

            });
    });
}
