use crate::scene::Primitive;
use crate::types::HitRecord;
use crate::types::Material;
use crate::ray::Ray;
use crate::vector::Vec3D;

pub struct Sphere<'a> {
    pub centre: &'a Vec3D,
    pub radius: f64,
    pub material: Material
}

impl<'a> Sphere<'a> {
    pub fn new(centre: &'a Vec3D, radius: f64, material: Material) -> Self {
        Sphere{
            centre: centre,
            radius: radius,
            material: material
        }
    }
}

impl<'a> Primitive for Sphere<'a> {
    fn intersect(&self, ray: &Ray) -> Option<(HitRecord, &Material)> {
        let distance = *self.centre - ray.origin();
        let b = distance.dot(&ray.direction());
        let mut determinant = b * b - distance.length_squared() + self.radius * self.radius;

        if determinant < 0.0 {
            return None;
        }
        else {
            determinant = determinant.sqrt();
            let epsilon = 1e-4;
            let minus_t = b - determinant;
            let plus_t = b + determinant;
            if minus_t < epsilon && plus_t < epsilon {
                None
            }
            else {
                let t;
                if minus_t > epsilon {
                    t = minus_t;
                }
                else {
                    t = plus_t;
                }

                let hit_pos = ray.position_along(t);
                let mut hit_normal = (hit_pos - *self.centre).normalize();
                if hit_normal.dot(&ray.direction()) > 0.0 {
                    hit_normal = hit_normal * -1.0;
                }

                return Some((
                    HitRecord {
                        distance: t,
                        position: hit_pos,
                        normal: hit_normal
                    }, &self.material
                ));
            }
        }
    }
}