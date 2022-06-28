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

use glm::*;

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

    pub fn update(world: &mut WorldState, dt: f32) {}
}
