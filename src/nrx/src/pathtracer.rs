//
// gkpro @ 2020-08-30
//   Nexus Library
//   Path Tracer
//
//       Based on Matt Godbolt's path tracer
//    https://github.com/mattgodbolt/pt-three-ways

use std::fs::File;
use std::io::prelude::*;

use crate::ray::Ray;
use crate::screen::Screen;
use crate::sphere::Primitive;
use crate::sphere::Sphere;
use crate::types::Material;
use crate::vector::Vec3D;

pub fn clamp<T: std::cmp::PartialOrd>(x: T, min: T, max: T) -> T {
    if x < min { min }
    else if x > max { max }
    else { x }
}

pub fn to_int(x: f64) -> i32 {
    (clamp(x, 0.0, 1.0).powf(1.0 / 2.2) * 255.0 + 0.5) as i32
}

pub fn render(buffer: &mut Screen, scene: &dyn Primitive) {
    let aspect_ratio = (buffer.width() as f64) / (buffer.height() as f64);

    let fov = 50.0;
    let yfov = (fov / 360.0 * std::f64::consts::PI).tan();
    let xfov = yfov * aspect_ratio;

    let camera_pos = Vec3D{ x: 0.0, y: 0.0, z: 0.0 };
    let camera_dir = Vec3D{ x: 0.0, y: 0.0, z: 1.0 };
    let cam_x = Vec3D {x: 1.0, y: 0.0, z: 0.0 };
    let cam_y = cam_x.cross(&camera_dir).normalize();

    for y in 0..buffer.height() {
        let yy = (y as f64) / (buffer.height() as f64) * 2.0 - 1.0;
        for x in 0..buffer.width() {
            let xx = (x as f64) / (buffer.width() as f64) * 2.0 - 1.0;
            let direction = (cam_x * xx * xfov + cam_y * yy * yfov + camera_dir).normalize();
            let ray = Ray::new(camera_pos, direction);
            let hit = scene.intersect(&ray);

            match hit {
                Some(a) => {
                    let colour = a.1.diffuse * direction.dot(&a.0.normal).powi(2);
                    buffer.draw(x, y, &colour)
                },
                None => (),
            };
        }
    }
}

pub fn write_file(buffer: &Screen) -> std::io::Result<()> {
    let s = [
        "P3\n",
        &buffer.width().to_string(), " ",
        &buffer.height().to_string(), "\n",
        &255.to_string(), "\n",
    ].concat();

    let mut img = File::create("image.ppm")?;
    img.write(s.as_bytes())?;

    for y in 0..buffer.height() {
        for x in 0..buffer.width() {
            let colour = buffer.at(x, y);
            let s: String = [
                &to_int(colour.x).to_string(), " ",
                &to_int(colour.y).to_string(), " ",
                &to_int(colour.z).to_string(), " "
            ].concat();
            img.write(s.as_bytes())?;
        }
    }

    Ok(())
}

pub fn run() {
    let mut screen_buffer = Screen::new(640, 480);

    let sphere = Sphere::new(
        &Vec3D{ x: 0.0, y: 0.0, z: 50.0 },
        15.0,
        Material{
            diffuse: Vec3D { x: 1.0, y: 0.0, z: 1.0 },
            emission: Vec3D { x: 0.0, y: 0.0, z: 0.0 }
        }
    );

    render(&mut screen_buffer, &sphere);
    write_file(&screen_buffer).expect("IO Error");
}