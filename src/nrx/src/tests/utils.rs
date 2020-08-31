use crate::vector::Vec3D;

pub fn approx(lhs: f64, rhs: f64, epsilon: f64) -> bool {
    (lhs - rhs).abs() < epsilon
}

pub fn approx_vec_3d(lhs: &Vec3D, rhs: &Vec3D, epsilon: f64) -> bool {
    approx(lhs.x, rhs.x, epsilon)
    && approx(lhs.y, rhs.y, epsilon)
    && approx(lhs.z, rhs.z, epsilon)
}

#[macro_use]
mod macros {
    #[macro_export]
    macro_rules! assert_comp {
        ($lhs:expr, $rhs:expr) => {
            assert_eq!($lhs, $rhs,
                "\nExpected : {:?}\
                 \nActual   : {:?}\n",
                $lhs, $rhs
            );
        };
        ($lhs:expr, $rhs:expr, $func:ident) => {
            assert!($func($lhs, $rhs, 0.00001),
                "\nExpected : {:.5?}\
                 \nActual   : {:.5?}\n",
                $lhs, $rhs
            );
        };
    }
}