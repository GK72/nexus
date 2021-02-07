use crate::vector::Vec3D;

pub struct HitRecord {
    pub distance: f64,
    pub position: Vec3D,
    pub normal: Vec3D
}

pub struct Material {
    pub emission: Vec3D,
    pub diffuse: Vec3D
}

impl Material {
    pub fn new() -> Self {
        Material {
            emission: Vec3D::new(),
            diffuse: Vec3D::new()
        }
    }
}