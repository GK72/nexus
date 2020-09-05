use crate::vector::Vec3D;

#[derive(Debug)]
pub struct Ray {
    origin: Vec3D,
    direction: Vec3D
}

impl Ray {
    pub fn new(origin: &Vec3D, direction: &Vec3D) -> Ray {
        Ray {
            origin: *origin,
            direction: *direction
        }
    }

    pub fn from_points(a: &Vec3D, b: &Vec3D) -> Ray {
        // TODO: a != b
        Ray {
            origin: *a,
            direction: (*b - *a).normalize()
        }
    }

    pub fn position_along(&self, x: f64) -> Vec3D {
        self.origin + self.direction * x
    }

    pub fn origin(&self) -> Vec3D { self.origin }
    pub fn direction(&self) -> Vec3D { self.direction }
}