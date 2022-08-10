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
mod scene;
mod voxel;
mod world;

fn main() {
    let mut texture1 = voxel::load("assets/AncientTemple.vox");
    let mut texture2 = voxel::load("assets/Treasure.vox");

    let mut world = world::WorldState::new();
    let renderer = std::sync::Arc::new(std::sync::Mutex::new(render::Renderer::new(&world)));

    renderer
        .lock()
        .unwrap()
        .add_texture(Box::new(texture1.remove(0)));
    renderer
        .lock()
        .unwrap()
        .add_texture(Box::new(texture2.remove(0)));

    let input_ptr = renderer.lock().unwrap().get_input_data_pointer();

    let mut scene = world.update(0.0, unsafe { *input_ptr });

    let (mut code, mut dt) = (true, 0.0);
    while code {
        let render_camera_pos = world.camera_position;
        let render_camera_dir = world.get_camera_direction();

        renderer.lock().unwrap().update_instances(scene);
        let renderer_clone = renderer.clone();

        let handle = std::thread::spawn(move || {
            renderer_clone
                .lock()
                .unwrap()
                .render_tick(&render_camera_pos, &render_camera_dir)
        });

        scene = world.update(dt, unsafe { *input_ptr });

        (code, dt) = handle.join().unwrap();
    }
}
