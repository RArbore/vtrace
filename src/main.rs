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

mod render;
mod voxel;
mod world;

fn main() {
    let mut texture = voxel::load("assets/AncientTemple.vox");

    let mut world = world::WorldState::new();
    let renderer = std::sync::Arc::new(std::sync::Mutex::new(render::Renderer::new(&world)));

    renderer.lock().unwrap().add_texture(texture.remove(0));
    renderer.lock().unwrap().update_descriptor();

    let mut instances1 = vec![];
    for x in -100..=100 {
        for z in -100..=100 {
            instances1.push(render::GPUInstance::new(
                0.0,
                &glm::Vec3::new(0.0, 1.0, 0.0),
                &glm::Vec3::new(0.0, 0.0, 0.0),
                &glm::Vec3::new(x as f32 * 1.5, 0.0, z as f32 * 1.5),
            ));
        }
    }

    let mut instances2 = vec![Default::default(); instances1.len()];

    let mut instances = (&mut instances1, &mut instances2);

    let (mut code, mut dt) = (true, 0.0);
    while code {
        let render_camera_pos = world.camera_position;
        let render_camera_dir = world.get_camera_direction();

        renderer.lock().unwrap().update_instances(instances.0);
        let renderer_clone = renderer.clone();

        let handle = std::thread::spawn(move || {
            renderer_clone
                .lock()
                .unwrap()
                .render_tick(&render_camera_pos, &render_camera_dir)
        });

        world.update(dt / 10.0, instances.0, instances.1);

        (code, dt) = handle.join().unwrap();

        instances = (instances.1, instances.0);
    }
}
