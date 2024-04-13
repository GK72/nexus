use glium::{
    implement_vertex,
    Display,
    VertexBuffer,
    glutin::surface::WindowSurface,
    vertex::VertexBufferAny
};

extern "C" {
    fn add(lhs: i32, rhs: i32) -> i32;
}

/// Load an OBJ file into a vertex buffer.
/// 
/// MVP.
/// 
/// ## Reference
/// 
/// <https://github.com/glium/glium/blob/master/examples/support/mod.rs>
pub fn load_wavefront(display: &Display<WindowSurface>, data: &[u8]) -> VertexBufferAny {
    #[derive(Copy, Clone)]
    struct Vertex {
        position: [f32; 3],
        normal: [f32; 3],
        texture: [f32; 2],
    }

    implement_vertex!(Vertex, position, normal, texture);

    let mut data = ::std::io::BufReader::new(data);
    let data = obj::ObjData::load_buf(&mut data).unwrap();

    let mut vertex_data = Vec::new();

    for object in data.objects.iter() {
        for polygon in object.groups.iter().flat_map(|g| g.polys.iter()) {
            match polygon {
                obj::SimplePolygon(indices) => {
                    for v in indices.iter() {
                        let position = data.position[v.0];
                        let texture = v.1.map(|index| data.texture[index]);
                        let normal = v.2.map(|index| data.normal[index]);

                        let texture = texture.unwrap_or([0.0, 0.0]);
                        let normal = normal.unwrap_or([0.0, 0.0, 0.0]);

                        vertex_data.push(Vertex {
                            position,
                            normal,
                            texture,
                        })
                    }
                },
            }
        }
    }

    VertexBuffer::new(display, &vertex_data).unwrap().into()
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn c_binding() {
        unsafe {
            assert_eq!(add(2, 3), 5);
        }
    }
    
    
}