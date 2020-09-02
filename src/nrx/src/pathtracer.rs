//
// gkpro @ 2020-08-30
//   Nexus Library
//   Path Tracer
//
//       Based on Matt Godbolt's path tracer
//    https://github.com/mattgodbolt/pt-three-ways

extern crate rand;

use std::fs::File;
use std::io::prelude::*;
use rand::prelude::*;

use crate::ray::Ray;
use crate::screen::Screen;
use crate::sphere::Primitive;
use crate::sphere::Sphere;
use crate::types::Material;
use crate::vector::Vec3D;

pub fn clamp<T: std::cmp::PartialOrd>(x: T, min: T, max: T) -> T {
    if x < min { return min; }
    else if x > max { return max; }
    else { return x; }
}

pub fn to_int(x: f64) -> i32 {
    return (clamp(x, 0.0, 1.0).powf(1.0 / 2.2) * 255.0 + 0.5) as i32;
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
            let rng = rand::thread_rng();
            let mut colour = Vec3D::new();

            let num_samples = 100;
            for _ in 0..num_samples {
                colour += radiance(
                    scene,
                    Ray::new(camera_pos, direction),
                    rng, 5);
                }
            buffer.draw(x, y, &(colour * (1.0 / num_samples as f64)));
        }
    }
}

pub fn radiance(scene: &dyn Primitive, ray: Ray, mut rng: ThreadRng, mut depth: i32) -> Vec3D {
    let hit = scene.intersect(&ray);
    match hit {
        Some(a) => {
            depth += 1;
            if depth > 5 {
                return a.1.emission;
            }
            else {
                let rand_polar = rng.gen_range(0.0, std::f64::consts::PI * 2.0);
                let rand_unit = rng.gen_range(0.0, 1.0);
                let rand_unit_sqrt = (rand_unit as f64).sqrt();

                let w = a.0.normal;
                let u = if w.x.abs() > 0.1 {
                    Vec3D{ x: 0.0, y: 1.0, z: 0.0 }
                } else {
                    Vec3D{ x: 1.0, y: 0.0, z: 0.0 }
                };

                let v = w.cross(&u);

                let new_dir = u * rand_polar.cos() * rand_unit_sqrt
                    + v * rand_polar.sin() * rand_unit_sqrt
                    + w * (1.0 - rand_unit as f64).sqrt();

                let new_ray = Ray::from_points(&a.0.position, &new_dir.normalize());

                return a.1.emission + a.1.diffuse * radiance(scene, new_ray, rng, depth);
            }
        },
        None => return Vec3D::new()
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
            emission: Vec3D { x: 0.1, y: 0.1, z: 0.1 }
        }
    );

    render(&mut screen_buffer, &sphere);
    write_file(&screen_buffer).expect("IO Error");
}