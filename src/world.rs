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

const MOVE_SPEED: f32 = 0.005;
const SENSITIVITY: f32 = 0.003;
const PI: f32 = 3.14159265358979323846;

#[derive(Debug)]
pub struct WorldState {
    pub camera_position: Vec3,
    pub camera_theta: f32,
    pub camera_phi: f32,
}

impl WorldState {
    pub fn new() -> WorldState {
        WorldState {
            camera_position: vec3(0.0, 0.0, 0.0),
            camera_theta: 0.0,
            camera_phi: PI / 2.0,
        }
    }

    pub fn get_camera_direction(&self) -> Vec3 {
        vec3(
            cos(self.camera_theta) * sin(self.camera_phi),
            cos(self.camera_phi),
            sin(self.camera_theta) * sin(self.camera_phi),
        )
    }

    pub fn get_horizontal_camera_direction(&self) -> Vec3 {
        vec3(cos(self.camera_theta), 0.0, sin(self.camera_theta))
    }

    pub fn update(&mut self, dt: f32) {
        /*
        if keystate[VirtualKeyCode::W as usize] {
            self.camera_position =
                self.camera_position + self.get_horizontal_camera_direction() * MOVE_SPEED;
        }
        if keystate[VirtualKeyCode::S as usize] {
            self.camera_position =
                self.camera_position - self.get_horizontal_camera_direction() * MOVE_SPEED;
        }
        if keystate[VirtualKeyCode::A as usize] {
            self.camera_position = self.camera_position
                - cross(self.get_horizontal_camera_direction(), vec3(0.0, 1.0, 0.0)) * MOVE_SPEED;
        }
        if keystate[VirtualKeyCode::D as usize] {
            self.camera_position = self.camera_position
                + cross(self.get_horizontal_camera_direction(), vec3(0.0, 1.0, 0.0)) * MOVE_SPEED;
        }
        if keystate[VirtualKeyCode::Space as usize] {
            self.camera_position = self.camera_position - vec3(0.0, 1.0, 0.0) * MOVE_SPEED;
        }
        if keystate[VirtualKeyCode::LShift as usize] {
            self.camera_position = self.camera_position + vec3(0.0, 1.0, 0.0) * MOVE_SPEED;
        }

        let mut dmx = 0.0;
        let mut dmy = 0.0;

        if keystate[VirtualKeyCode::Left as usize] {
            dmx -= SENSITIVITY;
        }
        if keystate[VirtualKeyCode::Right as usize] {
            dmx += SENSITIVITY;
        }
        if keystate[VirtualKeyCode::Up as usize] {
            dmy -= SENSITIVITY;
        }
        if keystate[VirtualKeyCode::Down as usize] {
            dmy += SENSITIVITY;
        }

        self.camera_theta += dmx;
        self.camera_phi -= dmy;
        self.camera_phi = clamp(self.camera_phi, 0.01, PI * 0.99);
        */
    }
}
