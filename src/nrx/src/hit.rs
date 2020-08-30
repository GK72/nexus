use crate::vector::Vec3D;

pub struct Hit {
    pub distance: f64,
    pub position: Vec3D,
    pub normal: Vec3D
}
