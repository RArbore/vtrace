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

    fn gen_voxel(&self, voxel_x: i32, voxel_y: i32, voxel_z: i32) -> Color {
        let (wx, wy, wz) = (voxel_x as f64, voxel_y as f64, voxel_z as f64);
        let (above_wx, above_wy, above_wz) = (voxel_x as f64, (voxel_y - 4) as f64, voxel_z as f64);

        if wx * wx + wy * wy + wz * wz > 100.0 * 100.0 {
            return Color::new(0, 0, 0, 0);
        }

        let surface =
            above_wx * above_wx + above_wy * above_wy + above_wz * above_wz > 100.0 * 100.0;

        let open_simplex_sample = self.open_simplex.get([wx * 0.1, wy * 0.1, wz * 0.1]);
        let billow_sample = self.billow.get([wx * 0.1, wy * 0.1, wz * 0.1]);

        let stone_color = (150.0, 150.0, 150.0);
        let dirt_color = (255.0, 200.0, 100.0);
        let color = if surface { dirt_color } else { stone_color };

        if open_simplex_sample > 0.0 {
            let tone = 0.5 * billow_sample + 0.5;
            Color::new(
                (tone * color.0) as u8,
                (tone * color.1) as u8,
                (tone * color.2) as u8,
                255,
            )
        } else {
            Color::new(0, 0, 0, 0)
        }
    }

    pub fn gen_chunk(&self, chunk_x: i32, chunk_y: i32, chunk_z: i32) -> Option<Box<Chunk>> {
        let mut chunk = rawchunk::RawStaticChunk::new(Default::default());

        let mut any_filled = false;
        for x in 0..CHUNK_VOXEL_SIZE {
            for y in 0..CHUNK_VOXEL_SIZE {
                for z in 0..CHUNK_VOXEL_SIZE {
                    let (vx, vy, vz) = (
                        x as i32 + chunk_x * CHUNK_VOXEL_SIZE as i32,
                        y as i32 + chunk_y * CHUNK_VOXEL_SIZE as i32,
                        z as i32 + chunk_z * CHUNK_VOXEL_SIZE as i32,
                    );

                    let color = self.gen_voxel(vx, vy, vz);
                    if color != Color::new(0, 0, 0, 0) {
                        any_filled = true;
                    }
                    *chunk.at_mut(z as i32, y as i32, x as i32).unwrap() = color;
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
