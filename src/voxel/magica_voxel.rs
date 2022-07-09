/*
 * This file is part of vtrace-rs.
 * vtrace-rs is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 * vtrace-rs is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with vtrace-rs. If not, see <https://www.gnu.org/licenses/>.
 */

use super::common::*;
use super::rawchunk::*;

pub fn load(filepath: &str) -> Vec<RawDynamicChunk<u8>> {
    let dot_vox_data = dot_vox::load(filepath).unwrap();

    let mut chunks = vec![];
    for model in dot_vox_data.models {
        let mut chunk = RawDynamicChunk::new(
            model.size.x as usize,
            model.size.y as usize,
            model.size.z as usize,
            0,
        );

        for voxel in model.voxels {
            *chunk
                .at_mut(voxel.x as i32, voxel.y as i32, voxel.z as i32)
                .unwrap() = voxel.i;
        }

        chunks.push(chunk);
    }

    chunks
}