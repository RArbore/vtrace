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

use std::time::*;

use crate::voxel::*;
use crate::world::*;

#[repr(C)]
#[derive(Default, Copy, Clone)]
struct GPUVertex {
    position: [f32; 3],
}

#[repr(C)]
#[derive(Default, Debug, Copy, Clone)]
pub struct GPUInstance {
    model: [f32; 16],
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
        model_id: u32,
    ) -> GPUInstance {
        let identity = Matrix4::new(
            Vec4::new(1.0, 0.0, 0.0, 0.0),
            Vec4::new(0.0, 1.0, 0.0, 0.0),
            Vec4::new(0.0, 0.0, 1.0, 0.0),
            Vec4::new(0.0, 0.0, 0.0, 1.0),
        );
        let rotate = glm::ext::rotate(&identity, rotate_angle, *rotate_axis);
        let scale = glm::ext::scale(&rotate, *scale);
        let translate = glm::ext::translate(&rotate, *translate);
        GPUInstance {
            model: [
                translate[0][0],
                translate[0][1],
                translate[0][2],
                translate[0][3],
                translate[1][0],
                translate[1][1],
                translate[1][2],
                translate[1][3],
                translate[2][0],
                translate[2][1],
                translate[2][2],
                translate[2][3],
                translate[3][0],
                translate[3][1],
                translate[3][2],
                unsafe { std::mem::transmute(model_id) },
            ],
        }
    }

    pub fn translate(&mut self, translate: &Vec3) {
        self.model[12] += translate.x;
        self.model[13] += translate.y;
        self.model[14] += translate.z;
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

    fn update_instances(instances: *const GPUInstance, instance_count: u32);

    fn cleanup();
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
}

#[repr(C)]
struct RenderTickInfo {
    perspective: *mut Matrix4<f32>,
    camera: *mut Matrix4<f32>,
}

impl Renderer {
    pub fn new(world: &WorldState) -> Self {
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
            camera: Self::create_camera(&world.camera_position, &world.get_camera_direction(), 0),
            prev_time: Instant::now(),
            prev_frame_time: Instant::now(),
            prev_frame_num: 0,
        }
    }

    fn create_perspective(fov: f32, aspect: f32) -> Matrix4<f32> {
        glm::ext::perspective(fov, aspect, 0.01, 10000.0)
    }

    fn create_camera(position: &Vec3, direction: &Vec3, frame_num: usize) -> Matrix4<f32> {
        let offset = Vec3::new(
            (frame_num >> 0 & 1) as f32 * 0.0002 - 0.0001,
            (frame_num >> 1 & 1) as f32 * 0.0002 - 0.0001,
            (frame_num >> 2 & 1) as f32 * 0.0002 - 0.0001,
        );
        glm::ext::look_at(
            *position,
            *position + *direction + offset,
            Vec3::new(0.0, 1.0, 0.0),
        )
    }

    pub fn get_input_data_pointer(&self) -> *const UserInput {
        unsafe { get_input_data_pointer() }
    }

    pub fn add_texture<T: RawVoxelFormat<Color>>(&mut self, texture: T) {
        let dim_x = texture.dim_x();
        let dim_y = texture.dim_y();
        let dim_z = texture.dim_z();
        assert!(dim_x.1 > dim_x.0);
        assert!(dim_y.1 > dim_y.0);
        assert!(dim_z.1 > dim_z.0);
        unsafe {
            add_texture(
                texture.get_raw(),
                (dim_x.1 - dim_x.0) as u32,
                (dim_y.1 - dim_y.0) as u32,
                (dim_z.1 - dim_z.0) as u32,
            )
        };
    }

    pub fn update_descriptor(&mut self) {}

    pub fn update_instances(&mut self, instances: &Vec<GPUInstance>) {
        unsafe { update_instances(instances.as_ptr(), instances.len() as u32) };
    }

    pub fn render_tick(&mut self, pos: &Vec3, dir: &Vec3) -> (bool, f32) {
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
