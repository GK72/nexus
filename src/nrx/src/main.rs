pub mod vector;
pub mod sphere;
pub mod hit;
pub mod ray;
pub mod pathtracer;

fn main() -> Result<(), std::io::Error> {
    pathtracer::run()?;
    Ok(())
}
