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

use std::sync::*;
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
    fn render_tick() -> i32;
    fn cleanup();
}

pub struct Renderer {}

impl Renderer {
    pub fn new(world: &WorldState) -> Self {
        let code = unsafe { entry() };
        if code != 0 {
            panic!("ERROR: Vulkan initialization failed",);
        }
        Renderer {}
    }

    pub fn add_texture<T: VoxelFormat<Color>>(&mut self, texture: T) {}

    pub fn update_descriptor(&mut self) {}

    pub fn update_instances(&mut self, instances: Vec<GPUInstance>) {}

    pub fn render_tick(&mut self, world: &mut WorldState) -> bool {
        unsafe { render_tick() == 0 }
    }
}

impl Drop for Renderer {
    fn drop(&mut self) {
        unsafe { cleanup() };
    }
}
