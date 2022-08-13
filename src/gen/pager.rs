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

use std::collections::HashMap;
use std::sync::*;

use crate::gen::terrain::*;
use crate::render::*;
use crate::voxel::*;

pub const CHUNK_VOXEL_SIZE: usize = 16;
pub const CHUNK_WORLD_SIZE: f32 = 4.0;

pub type Chunk =
    rawchunk::RawStaticChunk<Color, CHUNK_VOXEL_SIZE, CHUNK_VOXEL_SIZE, CHUNK_VOXEL_SIZE>;

pub struct WorldPager {
    chunks: HashMap<(i32, i32, i32), Option<(Box<Chunk>, TextureHandle)>>,
    terrain_generator: TerrainGenerator,
}

impl WorldPager {
    pub fn new() -> Self {
        WorldPager {
            chunks: HashMap::new(),
            terrain_generator: TerrainGenerator::new(0),
        }
    }

    pub fn page(
        &mut self,
        chunk_x: i32,
        chunk_y: i32,
        chunk_z: i32,
        texture_upload_queue: Arc<Mutex<TextureUploadQueue>>,
    ) {
        match self.chunks.get(&(chunk_x, chunk_y, chunk_z)) {
            Some(_) => {}
            None => {
                let chunk = self.terrain_generator.gen_chunk(chunk_x, chunk_y, chunk_z);
                if let Some(concrete_chunk) = chunk {
                    let handle = texture_upload_queue
                        .lock()
                        .unwrap()
                        .add_texture(concrete_chunk.clone());
                    self.chunks
                        .insert((chunk_x, chunk_y, chunk_z), Some((concrete_chunk, handle)));
                } else {
                    self.chunks.insert((chunk_x, chunk_y, chunk_z), None);
                }
            }
        }
    }
}
