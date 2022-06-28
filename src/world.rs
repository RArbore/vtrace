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

use glm::builtin::*;
use glm::*;

use winit::event::*;

use crate::render::*;

const MOVE_SPEED: f32 = 5.0;

pub struct WorldState {
    pub camera_position: Vec3,
    pub camera_direction: Vec3,
}

impl WorldState {
    pub fn new() -> WorldState {
        WorldState {
            camera_position: vec3(0.0, 0.0, 0.0),
            camera_direction: vec3(1.0, 0.0, 0.0),
        }
    }

    pub fn update(
        &mut self,
        dt: f32,
        keystate: &[bool; NUM_KEYS],
        last_keystate: &[bool; NUM_KEYS],
        mouse_pos: &(f32, f32),
        last_mouse_pos: &(f32, f32),
    ) {
        if keystate[VirtualKeyCode::W as usize] {
            self.camera_position = self.camera_position + self.camera_direction * dt * MOVE_SPEED;
        }
        if keystate[VirtualKeyCode::S as usize] {
            self.camera_position = self.camera_position - self.camera_direction * dt * MOVE_SPEED;
        }
        if keystate[VirtualKeyCode::A as usize] {
            self.camera_position = self.camera_position
                - cross(self.camera_direction, vec3(0.0, 1.0, 0.0)) * dt * MOVE_SPEED;
        }
        if keystate[VirtualKeyCode::D as usize] {
            self.camera_position = self.camera_position
                + cross(self.camera_direction, vec3(0.0, 1.0, 0.0)) * dt * MOVE_SPEED;
        }
    }
}
