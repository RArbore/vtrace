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

#![allow(dead_code)]
#![allow(unused_variables)]

use glm::*;

mod render;
mod voxel;
mod world;

fn main() {
    let mut texture = voxel::load("assets/AncientTemple.vox");

    let mut world = world::WorldState::new();
    let mut renderer = render::Renderer::new(&world);

    renderer.add_texture(texture.remove(0));
    renderer.update_descriptor();

    let instances = vec![
        render::GPUInstance::new(
            0.0,
            &Vec3::new(0.0, 1.0, 0.0),
            &Vec3::new(0.0, 0.0, 0.0),
            &Vec3::new(0.0, -1.3, 0.0),
        ),
        render::GPUInstance::new(
            0.0,
            &Vec3::new(0.0, 1.0, 0.0),
            &Vec3::new(0.0, 0.0, 0.0),
            &Vec3::new(0.0, 0.0, 0.0),
        ),
        render::GPUInstance::new(
            0.0,
            &Vec3::new(0.0, 1.0, 0.0),
            &Vec3::new(0.0, 0.0, 0.0),
            &Vec3::new(0.0, 1.3, 0.0),
        ),
    ];

    renderer.update_instances(&instances);

    let (mut code, mut dt) = renderer.render_tick(&mut world);
    while code {
        world.update(dt / 4.0);
        (code, dt) = renderer.render_tick(&mut world);
    }
}
