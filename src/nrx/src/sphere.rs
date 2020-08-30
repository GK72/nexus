use crate::vector::Vec3D;
// use crate::vector::Vec3Dnorm;
use crate::hit::Hit;
use crate::ray::Ray;

pub struct Sphere<'a> {
    pub centre: &'a Vec3D,
    pub radius: f64
}

impl<'a> Sphere<'a> {
    pub fn new(centre: &'a Vec3D, radius: f64) -> Self {
        Sphere{
            centre: centre,
            radius: radius
        }
    }

    pub fn intersect(&self, ray: &Ray) -> Option<Hit> {
        let distance = *self.centre - ray.origin();
        let b = distance.dot(&ray.direction());
        let mut determinant = b * b - distance.length_squared() + self.radius * self.radius;
        // let determinant = b - distance.length() + self.radius;

        if determinant < 0.0 {
            None
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
                let hit_normal = (hit_pos - *self.centre).normalize();

                Some(Hit {
                    distance: t,
                    position: hit_pos,
                    normal: hit_normal
                })
            }
        }
    }
}
