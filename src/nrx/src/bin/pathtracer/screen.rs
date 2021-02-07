use crate::vector::Vec3D;

pub struct Screen {
    width: u32,
    height: u32,
    pixels: std::vec::Vec::<Vec3D>
}

impl Screen {
    pub fn new(width: u32, height: u32) -> Self {
        Screen{
            width,
            height,
            pixels: vec![Vec3D::new(); (width * height) as usize]
        }
    }

    pub fn at(&self, x: u32, y: u32) -> &Vec3D {
        &self.pixels[(x + y * self.width) as usize]
    }

    pub fn draw(&mut self, x: u32, y: u32, colour: &Vec3D) {
        self.pixels[(x + y * self.width) as usize] = *colour;
    }

    pub fn width(&self) -> u32 { self.width }
    pub fn height(&self) -> u32 { self.height }
}