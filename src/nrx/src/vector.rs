use std::ops;

#[macro_export]
macro_rules! op_add {
    () => {
        println!("Operator add");
    };
}

#[derive(Default, PartialEq, Copy, Clone, Debug)]
pub struct Vec3D {
    pub x: f64,
    pub y: f64,
    pub z: f64
}

#[derive(PartialEq, Copy, Clone, Debug)]
pub struct Vec3Dnorm {
    x: f64,
    y: f64,
    z: f64
}

impl Vec3D {
    pub fn new() -> Self {
        Default::default()
    }

    pub fn cross(&self, b: &Self) -> Self {
        Vec3D {
            x: self.y * b.z - self.z * b.y,
            y: self.z * b.x - self.x * b.z,
            z: self.x * b.y - self.y * b.x
        }
    }

    pub fn dot(&self, rhs: &Self) -> f64 {
        self.x * rhs.x +
        self.y * rhs.y +
        self.z * rhs.z
    }

    pub fn normalize(&self) -> Self {
        *self * (1.0 / self.length())
    }

    pub fn length_squared(&self) -> f64 {
        self.dot(&self)
    }

    pub fn length(&self) -> f64 {
        self.length_squared().sqrt()
    }
}

impl ops::Sub<Vec3D> for Vec3D {
    type Output = Vec3D;
    fn sub(self, rhs: Vec3D) -> Vec3D {
        Vec3D{
            x: self.x - rhs.x,
            y: self.y - rhs.y,
            z: self.z - rhs.z
        }
    }
}

impl ops::Add<Vec3D> for Vec3D {
    type Output = Vec3D;
    fn add(self, rhs: Vec3D) -> Vec3D {
        Vec3D{
            x: self.x + rhs.x,
            y: self.y + rhs.y,
            z: self.z + rhs.z
        }
    }
}


impl ops::Mul<f64> for Vec3D {
    type Output = Vec3D;
    fn mul(self, rhs: f64) -> Vec3D {
        Vec3D{
            x: self.x * rhs,
            y: self.y * rhs,
            z: self.z * rhs
        }
    }
}

// impl ops::Add<Vec3Dnorm> for Vec3D{
    // type Output = Vec3D;
    // fn add(self, rhs: Vec3Dnorm) -> Vec3D{
        // Vec3D{
            // x: self.x + rhs.x,
            // y: self.y + rhs.y,
            // z: self.z + rhs.z
        // }
    // }
// }
// impl ops::Mul<f64> for Vec3Dnorm {
    // type Output = Vec3Dnorm;
    // fn mul(self, rhs: f64) -> Vec3Dnorm {
        // Vec3Dnorm{
            // x: self.x * rhs,
            // y: self.y * rhs,
            // z: self.z * rhs
        // }
    // }
// }

// impl Vec3Dnorm {
    // pub fn new(vec: Vec3D) -> Self {
        // let ret = vec * (1.0 / vec.length());
        // Vec3Dnorm { x: ret.x, y: ret.y, z: ret.z }
    // }
// }
