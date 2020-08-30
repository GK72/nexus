#[cfg(test)]
mod test_pathtracer {
    use crate::vector::Vec3D;
    use crate::sphere::Sphere;
    use crate::ray::Ray;

    fn approx(lhs: f64, rhs: f64, epsilon: f64) -> bool {
        (lhs - rhs).abs() < epsilon
    }

    fn approxVec3D(lhs: &Vec3D, rhs: &Vec3D, epsilon: f64) -> bool {
        println!("lhs: {:?}", lhs);
        println!("rhs: {:?}", rhs);
        approx(lhs.x, rhs.x, epsilon) &&
        approx(lhs.y, rhs.y, epsilon) &&
        approx(lhs.z, rhs.z, epsilon)
    }

    #[test]
    fn point_new() {
        let p = Vec3D{ x: 1.0, y: 1.0, z: 1.0 };
        assert_eq!(p.x, 1.0);
    }

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
            radius: 15.0
        };

        match s.intersect(&Ray::from_points(
            &Vec3D{ x: 0.0, y: 0.0, z: 0.0 },
            &Vec3D{ x: 0.0, y: 1.0, z: 0.0 })){
                Some(_x) => assert!(false),
                None     => assert!(true),
        }

        match s.intersect(&Ray::from_points(
            &Vec3D{ x: 0.0, y: 0.0, z: 0.0 },
            &Vec3D{ x: -10.0, y: -20.0, z: -30.0 })){
                Some(_x) => assert!(false),
                None     => assert!(true),
        }

        match s.intersect(&Ray::from_points(
            &Vec3D{ x: 0.0, y: 0.0, z: 0.0 },
            &Vec3D{ x: 10.0, y: 20.0, z: 30.0 })){
                Some(x) => {
                    assert!(approx(x.distance, 22.416573, 0.00001));
                    assert!(approxVec3D(&x.position, &Vec3D { x: 5.99108, y: 11.98216, z: 17.97324 }, 0.00001));
                    assert!(approxVec3D(&x.normal, &Vec3D { x: -0.267261, y: -0.534522, z: -0.801784 }, 0.00001));
                },
                None => assert!(false),
        }
    }
}

