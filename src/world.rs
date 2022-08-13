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

use std::collections::HashMap;
use std::sync::*;

use crate::gen::*;
use crate::render::*;
use crate::scene::*;
use crate::voxel::*;

const MOVE_SPEED: f32 = 5.0;
const SENSITIVITY: f32 = 0.02;
const PI: f32 = 3.14159265358979323846;

pub struct WorldState {
    pub camera_position: Vec3,
    pub camera_theta: f32,
    pub camera_phi: f32,
    accum_time_frac: f32,
    accum_time_whole: i32,
    pub frame_num: i32,
    entity_texture_registry: HashMap<&'static str, TextureHandle>,
    world_pager: WorldPager,
}

impl WorldState {
    pub fn new(texture_upload_queue: Arc<Mutex<TextureUploadQueue>>) -> WorldState {
        let mut world = WorldState {
            camera_position: vec3(0.0, 0.0, 0.0),
            camera_theta: 0.0,
            camera_phi: PI / 2.0,
            accum_time_frac: 0.0,
            accum_time_whole: 0,
            frame_num: 0,
            entity_texture_registry: HashMap::new(),
            world_pager: WorldPager::new(),
        };

        world.load_texture_from_file(texture_upload_queue, "Treasure");

        world
    }

    fn load_texture_from_file(
        &mut self,
        texture_upload_queue: Arc<Mutex<TextureUploadQueue>>,
        texture_name: &'static str,
    ) {
        let mut texture = load_magica_voxel(format!("assets/{}.vox", texture_name).as_str());

        let handle = texture_upload_queue
            .lock()
            .unwrap()
            .add_texture(Box::new(texture.remove(0)));

        self.entity_texture_registry.insert(texture_name, handle);
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

    pub fn update(
        &mut self,
        dt: f32,
        user_input: UserInput,
        texture_upload_queue: Arc<Mutex<TextureUploadQueue>>,
    ) -> SceneGraph {
        if self.frame_num == 0 {
            self.load_texture_from_file(texture_upload_queue, "AncientTemple");
        }

        self.accum_time_frac += dt;
        if self.accum_time_frac > 1.0 {
            self.accum_time_frac -= 1.0;
            self.accum_time_whole += 1;
        }

        if user_input.key_w > 0 {
            self.camera_position = self.camera_position
                + vec3(cos(self.camera_theta), 0.0, sin(self.camera_theta)) * dt * MOVE_SPEED;
        }
        if user_input.key_s > 0 {
            self.camera_position = self.camera_position
                - vec3(cos(self.camera_theta), 0.0, sin(self.camera_theta)) * dt * MOVE_SPEED;
        }
        if user_input.key_a > 0 {
            self.camera_position = self.camera_position
                + vec3(sin(self.camera_theta), 0.0, -cos(self.camera_theta)) * dt * MOVE_SPEED;
        }
        if user_input.key_d > 0 {
            self.camera_position = self.camera_position
                - vec3(sin(self.camera_theta), 0.0, -cos(self.camera_theta)) * dt * MOVE_SPEED;
        }
        if user_input.key_space > 0 {
            self.camera_position = self.camera_position - vec3(0.0, 1.0, 0.0) * dt * MOVE_SPEED;
        }
        if user_input.key_lshift > 0 {
            self.camera_position = self.camera_position + vec3(0.0, 1.0, 0.0) * dt * MOVE_SPEED;
        }

        self.camera_theta += SENSITIVITY * (user_input.mouse_x - user_input.last_mouse_x) as f32;
        self.camera_phi -= SENSITIVITY * (user_input.mouse_y - user_input.last_mouse_y) as f32;
        if self.camera_theta < 0.0 {
            self.camera_theta += 2.0 * PI;
        }
        if self.camera_theta >= 2.0 * PI {
            self.camera_theta -= 2.0 * PI;
        }
        if self.camera_phi < 0.1 {
            self.camera_phi = 0.1;
        }
        if self.camera_phi >= PI - 0.1 {
            self.camera_phi = PI - 0.1;
        }

        let mut scene = SceneGraph::new();
        let mut scene_entities = SceneGraph::new();
        let mut scene_terrain = SceneGraph::new();

        let handle1 = *self.entity_texture_registry.get("Treasure").unwrap();
        let handle2 = *self.entity_texture_registry.get("AncientTemple").unwrap();
        for x in -5..=5 {
            for z in -5..=5 {
                let identity = Matrix4::new(
                    Vec4::new(1.0, 0.0, 0.0, 0.0),
                    Vec4::new(0.0, 1.0, 0.0, 0.0),
                    Vec4::new(0.0, 0.0, 1.0, 0.0),
                    Vec4::new(0.0, 0.0, 0.0, 1.0),
                );
                let model = ext::translate(&identity, vec3(x as f32 * 1.5, 5.0, z as f32 * 1.5));
                scene_entities.add_child(SceneGraph::new_child(
                    model,
                    if (x + z + 10) as u32 % 2 == 0 {
                        handle1
                    } else {
                        handle2
                    },
                ));
            }
        }

        scene.add_child(scene_entities);
        scene.add_child(scene_terrain);

        self.frame_num += 1;

        scene
    }
}
