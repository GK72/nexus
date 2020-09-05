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

use crate::args::ArgParser;
use crate::ray::Ray;
use crate::screen::Screen;
use crate::scene::Scene;
use crate::scene::Primitive;
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

pub fn render(buffer: &mut Screen, scene: &dyn Primitive, sampling: i32, trace_depth: i32) {
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
            let mut rng = rand::thread_rng();
            let mut colour = Vec3D::new();

            for _ in 0..sampling {
                colour += radiance(
                    scene,
                    Ray::new(&camera_pos, &direction),
                    &mut rng, 0, trace_depth);
                }
            buffer.draw(x, y, &(colour * (1.0 / sampling as f64)));
        }
    }
}

pub fn radiance(scene: &dyn Primitive, ray: Ray, rng: &mut ThreadRng, mut depth: i32, max_depth: i32) -> Vec3D {
    let intersection = scene.intersect(&ray);
    match intersection {
        Some(x) => {
            let hit = x.0;
            let material = x.1;

            depth += 1;
            if depth > max_depth {
                return material.emission;
            }
            else {
                let rand_polar = rng.gen_range(0.0, std::f64::consts::PI * 2.0);
                let rand_unit = rng.gen_range(0.0, 1.0);
                let rand_unit_sqrt = (rand_unit as f64).sqrt();

                let w = hit.normal;
                let u = if w.x.abs() > 0.1 {
                    Vec3D{ x: 0.0, y: 1.0, z: 0.0 }
                } else {
                    Vec3D{ x: 1.0, y: 0.0, z: 0.0 }.cross(&w)
                };
                let v = w.cross(&u);

                let new_dir = u * rand_polar.cos() * rand_unit_sqrt
                    + v * rand_polar.sin() * rand_unit_sqrt
                    + w * (1.0 - rand_unit as f64).sqrt();

                let new_ray = Ray::new(&hit.position, &new_dir.normalize());

                return material.emission + material.diffuse * radiance(scene, new_ray, rng, depth, max_depth);
            }
        },
        None => Vec3D::new()
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

pub fn run(mut args: ArgParser) {
    args.add("--sampling".to_string());
    args.add("--width".to_string());
    args.add("--height".to_string());
    args.add("--trace-depth".to_string());
    args.process();

    let width = args.get("--width".to_string(), "320".to_string()).parse::<u32>().unwrap();
    let height = args.get("--height".to_string(), "240".to_string()).parse::<u32>().unwrap();
    let sampling = args.get("--sampling".to_string(), "10".to_string()).parse::<i32>().unwrap();
    let trace_depth = args.get("--trace-depth".to_string(), "5".to_string()).parse::<i32>().unwrap();

    let mut screen_buffer = Screen::new(width, height);

    let mut scene: Scene = Default::default();
    let sphere_a = Sphere::new(
        &Vec3D{ x: -10.0, y: 0.0, z: 50.0 },
        9.0,
        Material{
            diffuse: Vec3D { x: 1.0, y: 0.0, z: 1.0 },
            emission: Vec3D { x: 0.0, y: 0.0, z: 0.0 }
        }
    );

    let sphere_b = Sphere::new(
        &Vec3D{ x: 10.0, y: 0.0, z: 50.0 },
        9.0,
        Material{
            diffuse: Vec3D { x: 0.0, y: 0.0, z: 0.0 },
            emission: Vec3D { x: 0.4, y: 0.4, z: 0.4 }
        }
    );

    scene.add(&sphere_a);
    scene.add(&sphere_b);

    render(&mut screen_buffer, &scene, sampling, trace_depth);
    write_file(&screen_buffer).expect("IO Error");
}