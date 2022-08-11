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

use noise::*;

use crate::voxel::*;

const CHUNK_SIZE: usize = 16;

pub struct TerrainGenerator {
    seed: u32,
    open_simplex_frequency: f64,
    open_simplex: OpenSimplex,
}

impl TerrainGenerator {
    pub fn new(seed: u32) -> Self {
        let open_simplex = OpenSimplex::new().set_seed(seed);
        TerrainGenerator {
            seed,
            open_simplex_frequency: 0.1,
            open_simplex,
        }
    }

    pub fn gen_chunk(
        &self,
        chunk_x: i32,
        chunk_y: i32,
        chunk_z: i32,
    ) -> rawchunk::RawStaticChunk<Color, CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE> {
        let mut chunk = rawchunk::RawStaticChunk::new(Default::default());
        for x in 0..CHUNK_SIZE {
            for y in 0..CHUNK_SIZE {
                for z in 0..CHUNK_SIZE {
                    let noise = self.open_simplex.get([
                        (x as i32 + chunk_x * CHUNK_SIZE as i32) as f64
                            * self.open_simplex_frequency,
                        (y as i32 + chunk_y * CHUNK_SIZE as i32) as f64
                            * self.open_simplex_frequency,
                        (z as i32 + chunk_z * CHUNK_SIZE as i32) as f64
                            * self.open_simplex_frequency,
                    ]);
                    *chunk.at_mut(x as i32, y as i32, z as i32).unwrap() = if noise > 0.0 {
                        Color::new(100, 100, 100, 255)
                    } else {
                        Color::new(0, 0, 0, 0)
                    };
                }
            }
        }
        chunk
    }
}
