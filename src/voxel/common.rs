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

pub trait VoxelFormat<'a, T>: IntoIterator + FromIterator<T> {
    fn at_mut(x: i32, y: i32, z: i32) -> Option<&'a mut T>;
    fn at(x: i32, y: i32, z: i32) -> Option<&'a T>;
}

pub struct RawChunk<T, const W: usize> {
    data: [[[T; W]; W]; W],
}

pub struct RawChunkIter<'a, T, const W: usize> {
    chunk: &'a RawChunk<T, W>,
    index: usize,
}

impl<'a, T, const W: usize> IntoIterator for &'a RawChunk<T, W> {
    type Item = &'a T;
    type IntoIter = RawChunkIter<'a, T, W>;

    fn into_iter(self) -> Self::IntoIter {
        RawChunkIter {
            chunk: self,
            index: 0,
        }
    }
}

impl<'a, T, const W: usize> Iterator for RawChunkIter<'a, T, W> {
    type Item = &'a T;

    fn next(&mut self) -> Option<&'a T> {
        if self.index < W * W * W {
            let result = &self.chunk.data[self.index / W / W][self.index / W % W][self.index % W];
            self.index += 1;
            Some(result)
        } else {
            None
        }
    }
}
