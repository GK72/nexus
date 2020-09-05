#[macro_use] pub mod utils;

#[cfg(test)]
mod vector {
    use crate::vector::Vec3D;

    #[test]
    fn addition() {
        let p = Vec3D{ x: 1.0, y: 0.5, z: 4.4 };
        let q = Vec3D{ x: 2.0, y: 3.0, z: 5.0 };
        assert_eq!(p + q, Vec3D{ x: 3.0, y: 3.5, z: 9.4 });
    }

    #[test]
    fn multiplication_with_scalar() {
        let p = Vec3D{ x: 1.0, y: 2.0, z: 4.2 };
        assert_eq!(p * 2.5, Vec3D{ x: 2.5, y: 5.0, z: 10.5 });
    }

    #[test]
    fn cross_product_parallel() {
        let p = Vec3D{ x: 1.0, y: 1.0, z: 1.0 };
        let q = Vec3D{ x: 1.0, y: 1.0, z: 1.0 };
        assert_eq!(p.cross(&q), Vec3D{ x: 0.0, y: 0.0, z: 0.0 });
    }

    #[test]
    fn cross_product() {
        let p = Vec3D{ x: 2.0, y: 1.0, z: 3.0 };
        let q = Vec3D{ x: 1.0, y: 1.0, z: 1.0 };

        assert_eq!(p.cross(&q), Vec3D{ x: -2.0, y: 1.0, z: 1.0 });
    }

    #[test]
    fn dot_product() {
        let p = Vec3D{ x: 2.0, y: 1.0, z: 3.0 };
        let q = Vec3D{ x: 3.0, y: 2.0, z: 5.0 };
        assert_eq!(p.dot(&q), 23.0);
    }

    #[test]
    fn vector_length() {
        let p = Vec3D{ x: 2.0, y: 1.0, z: 3.0 };
        assert_eq!(p.length_squared(), 14.0);
        assert_eq!(p.length(), 14.0f64.sqrt());
    }
}

#[cfg(test)]
pub mod pathtracer {
    use crate::ray::Ray;
    use crate::sphere::Primitive;
    use crate::sphere::Sphere;
    use crate::types::Material;
    use crate::vector::Vec3D;
    use super::utils::*;

    #[test]
    fn ray_distance_along() {
        let ray = Ray::from_points(
            &Vec3D{ x: 10.0, y: 10.0, z: 10.0 },
            &Vec3D{ x: 10.0, y: 10.0, z: 60.0 }
        );

        assert_eq!(ray.position_along(0.0) , Vec3D{ x: 10.0, y: 10.0, z: 10.0 });
        assert_eq!(ray.position_along(50.0), Vec3D{ x: 10.0, y: 10.0, z: 60.0 });
    }

    #[test]
    fn sphere_intersect() {
        let s = Sphere{
            centre: &Vec3D{ x: 10.0, y: 20.0, z: 30.0 },
            radius: 15.0,
            material: Material {
                diffuse: Vec3D { x: 1.0, y: 0.0, z: 1.0 },
                emission: Vec3D { x: 0.1, y: 0.1, z: 0.1 }
            }
        };

        match s.intersect(&Ray::from_points(
            &Vec3D{ x: 0.0, y: 0.0, z: 0.0 },
            &Vec3D{ x: 0.0, y: 1.0, z: 0.0 })) {
                Some(_x) => assert!(false),
                None => assert!(true),
        }

        match s.intersect(&Ray::from_points(
            &Vec3D{ x: 0.0, y: 0.0, z: 0.0 },
            &Vec3D{ x: -10.0, y: -20.0, z: -30.0 })) {
                Some(_x) => assert!(false),
                None => assert!(true),
        }

        match s.intersect(&Ray::from_points(
            &Vec3D{ x: 0.0, y: 0.0, z: 0.0 },
            &Vec3D{ x: 10.0, y: 20.0, z: 30.0 })){
                Some(x) => {
                    assert!(approx(x.0.distance, 22.416573, 0.00001));

                    let pos  = Vec3D { x: 5.99108, y: 11.98216, z: 17.97324 };
                    let norm = Vec3D { x: -0.267261, y: -0.534522, z: -0.801784 };
                    assert_comp!(&x.0.position, &pos, approx_vec_3d);
                    assert_comp!(&x.0.normal, &norm, approx_vec_3d);
                },
                None => assert!(false),
        }
    }
}