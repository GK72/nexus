//
// gkpro @ 2020-08-30
//   Nexus Library
//   Path Tracer
//
//       Based on Matt Godbolt's path tracer
//    https://github.com/mattgodbolt/pt-three-ways

use std::fs::File;
use std::io::prelude::*;

use crate::vector::Vec3D;
use crate::sphere::Sphere;
use crate::ray::Ray;

pub fn clamp(x: f64, min: f64, max: f64) -> f64 {
    if x < min { min }
    else if x > max { max }
    else { x }
}

pub fn to_int(x: f64) -> i32 {
    (clamp(x, 0.0, 1.0).powf(1.0 / 2.2) * 255.0 + 0.5) as i32
}

pub fn run() -> std::io::Result<()> {
    let width = 640;
    let height = 480;
    let aspect_ratio = (width as f64) / (height as f64);

    let yfov = 0.5135;
    let xfov = yfov * aspect_ratio;

    let camera_pos = Vec3D{ x: 0.0, y: 0.0, z: 0.0 };
    let camera_dir = Vec3D{ x: 0.0, y: 0.0, z: 1.0 };
    let cam_x = Vec3D {x: 1.0, y: 0.0, z: 0.0 };
    let cam_y = cam_x.cross(&camera_dir).normalize();

    let sphere = Sphere::new(
        &Vec3D{ x: 0.0, y: 0.0, z: 50.0 },
        15.0
    );

    let mut s: String = String::from("");
    s.push_str("P3\n");
    s.push_str(&width.to_string());
    s.push_str(" ");
    s.push_str(&height.to_string());
    s.push_str("\n");
    s.push_str(&255.to_string());
    s.push_str("\n");

    let mut img = File::create("image.ppm")?;
    img.write(s.as_bytes())?;

    for y in 0..height {
        let yy = (y as f64) / (height as f64) * 2.0 - 1.0;
        for x in 0..width {
            let xx = (x as f64) / (width as f64) * 2.0 - 1.0;
            let direction = (cam_x * xx * xfov + cam_y * yy * yfov + camera_dir).normalize();
            let ray = Ray::new(camera_pos, direction);
            let hit = sphere.intersect(&ray);

            match hit {
                Some(x) => {
                    let colour = Vec3D{ x: 1.0, y: 0.0, z: 1.0 } * direction.dot(&x.normal).powi(2);
                    let mut s: String = String::from("");
                    s.push_str(&to_int(colour.x).to_string());
                    s.push_str(" ");
                    s.push_str(&to_int(colour.y).to_string());
                    s.push_str(" ");
                    s.push_str(&to_int(colour.z).to_string());
                    s.push_str(" ");
                    img.write(s.as_bytes())
                },
                None     => img.write(b"0 0 0 "),
            }?;
        }
    }
    Ok(())
}
