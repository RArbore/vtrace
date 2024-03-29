/*
 * This file is part of vtrace.
 * vtrace is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 * vtrace is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with vtrace. If not, see <https://www.gnu.org/licenses/>.
 */

use glm::*;

use std::collections::HashMap;
use std::collections::VecDeque;
use std::sync::*;
use std::time::*;

use crate::scene::*;
use crate::voxel::*;

#[repr(C)]
#[derive(Default, Copy, Clone)]
struct GPUVertex {
    position: [f32; 3],
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct GPUInstance {
    model: Matrix4<f32>,
}

#[repr(C)]
#[derive(Default, Debug, Copy, Clone)]
pub struct UserInput {
    pub key_w: u8,
    pub key_a: u8,
    pub key_s: u8,
    pub key_d: u8,
    pub key_space: u8,
    pub key_lshift: u8,

    pub mouse_x: f64,
    pub mouse_y: f64,
    pub last_mouse_x: f64,
    pub last_mouse_y: f64,
}

impl GPUInstance {
    pub fn new(
        rotate_angle: f32,
        rotate_axis: &Vec3,
        scale: &Vec3,
        translate: &Vec3,
        texture_id: u32,
    ) -> Self {
        let identity = Matrix4::new(
            Vec4::new(1.0, 0.0, 0.0, 0.0),
            Vec4::new(0.0, 1.0, 0.0, 0.0),
            Vec4::new(0.0, 0.0, 1.0, 0.0),
            Vec4::new(0.0, 0.0, 0.0, 1.0),
        );
        let rotate = glm::ext::rotate(&identity, rotate_angle, *rotate_axis);
        let scale = glm::ext::scale(&rotate, *scale);
        let mut translate = glm::ext::translate(&rotate, *translate);
        translate[3][3] = unsafe { std::mem::transmute(texture_id) };
        GPUInstance { model: translate }
    }

    pub fn from_model(model: Matrix4<f32>, texture_id: u32) -> Self {
        let mut merged = model;
        merged[3][3] = unsafe { std::mem::transmute(texture_id) };
        GPUInstance { model: merged }
    }

    pub fn translate(&mut self, translate: &Vec3) {
        self.model[3][0] += translate.x;
        self.model[3][1] += translate.y;
        self.model[3][2] += translate.z;
    }

    pub fn scale(&mut self, scale: &Vec3) {
        self.model[0][0] *= scale.x;
        self.model[1][1] *= scale.y;
        self.model[2][2] *= scale.z;
    }

    pub fn rotate(&mut self, angle: f32, axis: &Vec3) {
        self.model = glm::ext::rotate(&self.model, angle, normalize(*axis));
    }
}

impl Default for GPUInstance {
    fn default() -> Self {
        GPUInstance {
            model: Matrix4::new(
                Vec4::new(1.0, 0.0, 0.0, 0.0),
                Vec4::new(0.0, 1.0, 0.0, 0.0),
                Vec4::new(0.0, 0.0, 1.0, 0.0),
                Vec4::new(0.0, 0.0, 0.0, 1.0),
            ),
        }
    }
}

extern "C" {
    fn entry() -> u64;

    fn render_tick(
        window_width: *mut i32,
        window_height: *mut i32,
        render_tick_info: *const RenderTickInfo,
    ) -> i32;

    fn get_input_data_pointer() -> *const UserInput;

    fn add_texture(data: *const Color, width: u32, height: u32, depth: u32) -> i32;

    fn start_update_instances(instance_count: u32) -> *mut GPUInstance;

    fn end_update_instances(instance_count: u32) -> i32;

    fn cleanup();
}

#[derive(PartialEq, Eq, Hash, Clone, Copy)]
pub struct TextureHandle {
    id: u32,
}

pub struct Renderer {
    window_width: i32,
    window_height: i32,
    prev_window_width: i32,
    prev_window_height: i32,
    fov: f32,
    frame_num: usize,
    perspective: Matrix4<f32>,
    camera: Matrix4<f32>,
    prev_time: Instant,
    prev_frame_time: Instant,
    prev_frame_num: usize,
    texture_handle_lookup: HashMap<TextureHandle, u32>,
}

pub struct TextureUploadQueue {
    num_textures_created: u32,
    texture_upload_queue: VecDeque<(Box<dyn RawVoxelData<Color> + Send>, TextureHandle)>,
}

impl TextureUploadQueue {
    pub fn new() -> Self {
        TextureUploadQueue {
            num_textures_created: 0,
            texture_upload_queue: VecDeque::new(),
        }
    }

    pub fn add_texture(&mut self, texture: Box<dyn RawVoxelData<Color> + Send>) -> TextureHandle {
        let handle = TextureHandle {
            id: self.num_textures_created,
        };
        self.num_textures_created += 1;
        self.texture_upload_queue.push_back((texture, handle));
        handle
    }

    pub fn pop(&mut self) -> Option<(Box<dyn RawVoxelData<Color> + Send>, TextureHandle)> {
        self.texture_upload_queue.pop_front()
    }
}

#[repr(C)]
struct RenderTickInfo {
    perspective: *mut Matrix4<f32>,
    camera: *mut Matrix4<f32>,
}

impl Renderer {
    pub fn new() -> Self {
        let code = unsafe { entry() };
        if code != 0 {
            panic!("ERROR: Vulkan initialization failed",);
        }

        let fov = 80.0 / 180.0 * 3.1415926;

        Renderer {
            window_width: 1,
            window_height: 1,
            prev_window_width: 1,
            prev_window_height: 1,
            fov,
            frame_num: 0,
            perspective: Self::create_perspective(fov, 1.0),
            camera: Self::create_camera(&vec3(0.0, 0.0, 0.0), &vec3(1.0, 0.0, 0.0), 0),
            prev_time: Instant::now(),
            prev_frame_time: Instant::now(),
            prev_frame_num: 0,
            texture_handle_lookup: HashMap::new(),
        }
    }

    fn create_perspective(fov: f32, aspect: f32) -> Matrix4<f32> {
        glm::ext::perspective(fov, aspect, 0.01, 10000.0)
    }

    fn create_camera(position: &Vec3, direction: &Vec3, frame_num: usize) -> Matrix4<f32> {
        glm::ext::look_at(*position, *position + *direction, Vec3::new(0.0, 1.0, 0.0))
    }

    pub fn get_input_data_pointer(&self) -> *const UserInput {
        unsafe { get_input_data_pointer() }
    }

    pub fn update_instances(&mut self, scene: SceneGraph) {
        let instance_count = scene.num_total_children();
        let mut ptr = unsafe { start_update_instances(instance_count) };
        if ptr == 0 as *mut GPUInstance {
            panic!("ERROR: Updating instances failed",);
        }
        let mut true_instance_count = 0;
        for (model, texture_handle) in scene {
            if let Some(texture_id) = self.texture_handle_lookup.get(&texture_handle) {
                unsafe { *ptr = GPUInstance::from_model(model, *texture_id) };
                ptr = unsafe { ptr.offset(1) };
                true_instance_count += 1;
            }
        }
        let code = unsafe { end_update_instances(true_instance_count) };
        if code != 0 {
            panic!("ERROR: Updating instances failed",);
        }
    }

    pub fn render_tick(
        &mut self,
        pos: &Vec3,
        dir: &Vec3,
        texture_upload_queue: Arc<Mutex<TextureUploadQueue>>,
    ) -> (bool, f32) {
        if let Some((texture, handle)) = texture_upload_queue.lock().unwrap().pop() {
            let dim_x = texture.dim_x();
            let dim_y = texture.dim_y();
            let dim_z = texture.dim_z();
            assert!(dim_x.1 > dim_x.0);
            assert!(dim_y.1 > dim_y.0);
            assert!(dim_z.1 > dim_z.0);

            let texture_id = unsafe {
                add_texture(
                    texture.get_raw(),
                    (dim_x.1 - dim_x.0) as u32,
                    (dim_y.1 - dim_y.0) as u32,
                    (dim_z.1 - dim_z.0) as u32,
                )
            };

            if texture_id < 0 {
                panic!("ERROR: Adding texture failed",);
            }

            self.texture_handle_lookup.insert(handle, texture_id as u32);
        }

        let render_tick_info = RenderTickInfo {
            perspective: &mut self.perspective,
            camera: &mut self.camera,
        };
        let code = unsafe {
            render_tick(
                &mut self.window_width,
                &mut self.window_height,
                &render_tick_info,
            ) == 0
        };

        if self.window_width != self.prev_window_width
            || self.window_height != self.prev_window_height
        {
            self.perspective = Self::create_perspective(
                self.fov,
                self.window_width as f32 / self.window_height as f32,
            );

            self.prev_window_width = self.window_width;
            self.prev_window_height = self.window_height;
        }
        self.camera = Self::create_camera(pos, dir, self.frame_num);

        let dt = self.prev_time.elapsed().as_micros();
        if dt > 1000000 {
            self.prev_time = Instant::now();
            let num_frames = self.frame_num - self.prev_frame_num;
            self.prev_frame_num = self.frame_num;
            println!(
                "FPS: {}   MS: {}",
                1000000.0 * num_frames as f32 / dt as f32,
                dt as f32 / 1000.0 / num_frames as f32
            );
        }

        let frame_dt = self.prev_frame_time.elapsed().as_micros();
        self.prev_frame_time = Instant::now();

        self.frame_num += 1;

        (code, frame_dt as f32 / 1000000.0)
    }
}

impl Drop for Renderer {
    fn drop(&mut self) {
        unsafe { cleanup() };
    }
}
