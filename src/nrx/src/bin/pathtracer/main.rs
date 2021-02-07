pub mod args;
pub mod progress;

pub mod pathtracer;
pub mod ray;
pub mod scene;
pub mod screen;
pub mod sphere;
pub mod types;
pub mod vector;

pub mod tests;

fn main() -> std::io::Result<()> {
    let args = args::ArgParser::new(std::env::args().collect());

    pathtracer::run(args)?;

    return Ok(());
}
