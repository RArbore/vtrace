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

#![allow(dead_code)]
#![allow(unused_variables)]

mod render;
mod voxel;
mod world;

use voxel::common::*;

fn main() {
    let world = world::WorldState::new();
    let mut renderer = render::Renderer::new(&world);

    let mut test_texture1 =
        voxel::RawDynamicChunk::new(32, 32, 32, voxel::Color::new(0x00, 0x00, 0x00, 0x00));
    for x in 0..32 {
        for y in 0..32 {
            for z in 0..32 {
                let s = (x + y + z) % 2;
                let t = (x / 2 + y / 2 + z / 2) % 2;
                *test_texture1.at_mut(x as i32, y as i32, z as i32).unwrap() =
                    Color::new(0xFF * s, 0xFF * (1 - s), 0xFF * s, 0xFF * t);
            }
        }
    }

    let mut test_texture2 =
        voxel::RawDynamicChunk::new(32, 32, 32, voxel::Color::new(0x00, 0x00, 0x00, 0x00));
    for x in 0..32 {
        for y in 0..32 {
            for z in 0..32 {
                let s = (x + y + z) % 2;
                let t = (x / 2 + y / 2 + z / 2) % 2;
                *test_texture2.at_mut(x as i32, y as i32, z as i32).unwrap() =
                    Color::new(0xFF * (1 - s), 0xFF * (1 - s), 0xFF * s, 0xFF * (1 - t));
            }
        }
    }

    renderer.add_texture(test_texture1);
    renderer.add_texture(test_texture2);
    renderer.update_descriptor();

    renderer.render_loop(world);
}
