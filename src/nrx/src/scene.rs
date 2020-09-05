
use crate::types::HitRecord;
use crate::types::Material;
use crate::ray::Ray;

pub trait Primitive {
    fn intersect(&self, ray: &Ray) -> Option<(HitRecord, &Material)>;
}

#[derive(Default)]
pub struct Scene<'a> {
    primitives: std::vec::Vec::<&'a dyn Primitive>
}

impl<'a> Scene<'a> {
    pub fn new() -> Self {
        Default::default()
    }

    pub fn add(&mut self, x: &'a dyn Primitive) {
        self.primitives.push(x);
    }
}

impl<'a> Primitive for Scene<'a> {
    fn intersect(&self, ray: &Ray) -> Option<(HitRecord, &Material)> {
        let mut nearest: Option<(HitRecord, &Material)> = None;
        for primitive in &self.primitives {
            let intersection = primitive.intersect(&ray);
            match &intersection {
                Some(hit) => {
                    match &nearest {
                        None => nearest = intersection,
                        Some(nearest_intersection) => {
                            if hit.0.distance < nearest_intersection.0.distance {
                                nearest = intersection;
                            }
                        },
                    }
                },
                None => continue,
            }
        }
        return nearest;
    }
}