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
    accum_time_frac: f32,
    accum_time_whole: i32,
}

impl WorldState {
    pub fn new() -> WorldState {
        WorldState {
            camera_position: vec3(0.0, 0.0, 0.0),
            camera_theta: 0.0,
            camera_phi: PI / 2.0,
            accum_time_frac: 0.0,
            accum_time_whole: 0,
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
        self.accum_time_frac += dt;
        if self.accum_time_frac > 1.0 {
            self.accum_time_frac -= 1.0;
            self.accum_time_whole += 1;
        }

        self.camera_theta = self.accum_time_frac * 3.1415926 * 2.0;
        self.camera_position = self.get_camera_direction() * -4.0;
        self.camera_position.y = cos(self.camera_theta * 2.0);
    }
}
