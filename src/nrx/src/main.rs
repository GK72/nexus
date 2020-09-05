pub mod args;
pub mod pathtracer;
pub mod ray;
pub mod scene;
pub mod screen;
pub mod sphere;
pub mod types;
pub mod vector;

use std::env;

fn main() {
    let args = args::ArgParser::new(env::args().collect());
    pathtracer::run(args);
}