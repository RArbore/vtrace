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
    open_simplex: OpenSimplex,
}

impl TerrainGenerator {
    pub fn new(seed: u32) -> Self {
        let open_simplex = OpenSimplex::new().set_seed(seed);
        TerrainGenerator { seed, open_simplex }
    }

    pub fn gen_chunk(
        chunk_x: u32,
        chunk_y: u32,
        chunk_z: u32,
    ) -> rawchunk::RawStaticChunk<Material, CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE> {
        rawchunk::RawStaticChunk::new(Default::default())
    }
}

#[derive(PartialEq, Eq, Clone, Copy, Default)]
pub enum Material {
    #[default]
    Empty,
}

impl Voxel for Material {}
