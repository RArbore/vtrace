/*
 * This file is part of vtrace-rs.
 * vtrace-rs is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 * vtrace-rs is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with vtrace-rs. If not, see <https://www.gnu.org/licenses/>.
 */

use bytemuck::*;

use glm::ext::*;
use glm::*;

use winit::dpi::*;
use winit::event_loop::*;
use winit::window::*;

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

pub const NUM_KEYS: usize = winit::event::VirtualKeyCode::Cut as usize + 1;
pub const NUM_BUTTONS: usize = 3;

pub fn b2u(button: winit::event::MouseButton) -> usize {
    match button {
        winit::event::MouseButton::Left => 0,
        winit::event::MouseButton::Right => 1,
        winit::event::MouseButton::Middle => 2,
        winit::event::MouseButton::Other(x) => x as usize,
    }
}

pub struct Renderer {}

impl Renderer {
    pub fn new(world: &WorldState) -> Renderer {
        Renderer {}
    }

    pub fn add_texture<T: VoxelFormat<Color>>(&mut self, texture: T) {}

    pub fn update_descriptor(&mut self) {}

    pub fn update_instances(&mut self, instances: Vec<GPUInstance>) {}

    pub fn render_loop(mut self, mut world: WorldState) {}
}
