pub mod hit;
pub mod pathtracer;
pub mod ray;
pub mod sphere;
pub mod vector;

fn main() -> Result<(), std::io::Error> {
    pathtracer::run()?;
    Ok(())
}
