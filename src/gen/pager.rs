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

use crate::voxel::*;

pub const CHUNK_SIZE: usize = 16;

pub type Chunk = rawchunk::RawStaticChunk<Color, CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE>;

pub struct WorldPager {
    chunks: HashMap<(i32, i32, i32), Option<Chunk>>,
}
