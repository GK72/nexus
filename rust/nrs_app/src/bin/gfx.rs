use nrs::gui;

fn main() {
    let gui = gui::Gui::new("Sandbox");

    let mut value = 0;

    gui.event_loop(move |_keep_alive, ui| {
        ui.window("Main")
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
            });
    });
}
