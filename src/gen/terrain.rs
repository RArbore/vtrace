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

use crate::gen::pager::*;
use crate::voxel::*;

pub struct TerrainGenerator {
    seed: u32,
    open_simplex: OpenSimplex,
    billow: Billow,
}

impl TerrainGenerator {
    pub fn new(seed: u32) -> Self {
        let open_simplex = OpenSimplex::new().set_seed(seed);
        let billow = Billow::new().set_seed(seed);
        TerrainGenerator {
            seed,
            open_simplex,
            billow,
        }
    }

    pub fn gen_chunk(&self, chunk_x: i32, chunk_y: i32, chunk_z: i32) -> Option<Box<Chunk>> {
        let mut chunk = rawchunk::RawStaticChunk::new(Default::default());

        let mut any_filled = false;
        for x in 0..CHUNK_VOXEL_SIZE {
            for y in 0..CHUNK_VOXEL_SIZE {
                for z in 0..CHUNK_VOXEL_SIZE {
                    let (wx, wy, wz) = (
                        (x as i32 + chunk_x * CHUNK_VOXEL_SIZE as i32) as f64,
                        (y as i32 + chunk_y * CHUNK_VOXEL_SIZE as i32) as f64,
                        (z as i32 + chunk_z * CHUNK_VOXEL_SIZE as i32) as f64,
                    );

                    let open_simplex_sample = self.open_simplex.get([wx * 0.1, wy * 0.1, wz * 0.1]);
                    let billow_sample = self.billow.get([wx * 0.1, wy * 0.1, wz * 0.1]);

                    *chunk.at_mut(x as i32, y as i32, z as i32).unwrap() =
                        if open_simplex_sample > 0.0 {
                            let tone = 0.5 * billow_sample + 0.5;
                            any_filled = true;
                            Color::new(
                                (tone * 255.0) as u8,
                                (tone * 255.0) as u8,
                                (tone * 255.0) as u8,
                                255,
                            )
                        } else {
                            Color::new(0, 0, 0, 0)
                        };
                }
            }
        }

        if any_filled {
            Some(Box::new(chunk))
        } else {
            None
        }
    }
}
