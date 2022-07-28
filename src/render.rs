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

use bytemuck::*;

use glm::ext::*;
use glm::*;

use std::time::*;

use crate::voxel::*;
use crate::world::*;

#[repr(C)]
#[derive(Default, Copy, Clone, Zeroable, Pod)]
struct GPUVertex {
    position: [f32; 3],
}

#[repr(C)]
#[derive(Default, Copy, Clone, Zeroable, Pod)]
pub struct GPUInstance {
    pub model: [f32; 16],
}

extern "C" {
    fn entry() -> u64;
    fn render_tick(
        window_width: *mut i32,
        window_height: *mut i32,
        render_tick_info: *const RenderTickInfo,
    ) -> i32;
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
        }
    }

    fn create_perspective(fov: f32, aspect: f32) -> Matrix4<f32> {
        perspective(fov, aspect, 0.01, 10000.0)
    }

    fn create_camera(position: &Vec3, direction: &Vec3, frame_num: usize) -> Matrix4<f32> {
        let offset = Vec3::new(
            (frame_num >> 0 & 1) as f32 * 0.0002 - 0.0001,
            (frame_num >> 1 & 1) as f32 * 0.0002 - 0.0001,
            (frame_num >> 2 & 1) as f32 * 0.0002 - 0.0001,
        );
        look_at(
            *position,
            *position + *direction + offset,
            vec3(0.0, 1.0, 0.0),
        )
    }

    pub fn add_texture<T: VoxelFormat<Color>>(&mut self, texture: T) {}

    pub fn update_descriptor(&mut self) {}

    pub fn update_instances(&mut self, instances: Vec<GPUInstance>) {}

    pub fn render_tick(&mut self, world: &mut WorldState) -> (bool, f32) {
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
        self.camera = Self::create_camera(
            &world.camera_position,
            &world.get_camera_direction(),
            self.frame_num,
        );

        let dt = self.prev_time.elapsed().as_micros();
        self.prev_time = Instant::now();
        /*println!(
            "FPS: {}   MS: {}",
            1000000.0 / dt as f32,
            dt as f32 / 1000.0
        );*/

        (code, dt as f32 / 1000000.0)
    }
}

impl Drop for Renderer {
    fn drop(&mut self) {
        unsafe { cleanup() };
    }
}
