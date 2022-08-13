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

use std::sync::*;

mod gen;
mod render;
mod scene;
mod voxel;
mod world;

fn main() {
    let renderer = Arc::new(Mutex::new(render::Renderer::new()));
    let texture_upload_queue = Arc::new(Mutex::new(render::TextureUploadQueue::new()));
    let mut world = world::WorldState::new(texture_upload_queue.clone());

    let input_ptr = renderer.lock().unwrap().get_input_data_pointer();

    let mut scene = world.update(0.0, unsafe { *input_ptr }, texture_upload_queue.clone());

    let (mut code, mut dt) = (true, 0.0);
    while code {
        let render_camera_pos = world.camera_position;
        let render_camera_dir = world.get_camera_direction();

        renderer.lock().unwrap().update_instances(scene);
        let renderer_clone = renderer.clone();
        let texture_upload_queue_clone = texture_upload_queue.clone();

        let thread_handle = std::thread::spawn(move || {
            renderer_clone.lock().unwrap().render_tick(
                &render_camera_pos,
                &render_camera_dir,
                texture_upload_queue_clone,
            )
        });

        scene = world.update(dt, unsafe { *input_ptr }, texture_upload_queue.clone());

        (code, dt) = thread_handle.join().unwrap();
    }
}
