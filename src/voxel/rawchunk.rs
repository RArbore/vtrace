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

#[derive(PartialEq, Eq, Debug)]
pub struct RawStaticChunk<T: Voxel, const X: usize, const Y: usize, const Z: usize> {
    data: [[[T; Z]; Y]; X],
}

pub struct RawStaticChunkIter<'a, T: Voxel, const X: usize, const Y: usize, const Z: usize> {
    chunk: &'a RawStaticChunk<T, X, Y, Z>,
    index: usize,
}

impl<'a, T: Voxel, const X: usize, const Y: usize, const Z: usize> IntoIterator
    for &'a RawStaticChunk<T, X, Y, Z>
{
    type Item = T;
    type IntoIter = RawStaticChunkIter<'a, T, X, Y, Z>;

    fn into_iter(self) -> Self::IntoIter {
        RawStaticChunkIter {
            chunk: self,
            index: 0,
        }
    }
}

impl<'a, T: Voxel, const X: usize, const Y: usize, const Z: usize> Iterator
    for RawStaticChunkIter<'a, T, X, Y, Z>
{
    type Item = T;

    fn next(&mut self) -> Option<T> {
        if self.index < X * Y * Z {
            let result = self.chunk.data[self.index / Z / Y][self.index / Z % Y][self.index % Z];
            self.index += 1;
            Some(result)
        } else {
            None
        }
    }
}

impl<'a, T: Voxel, const X: usize, const Y: usize, const Z: usize> FromIterator<T>
    for RawStaticChunk<T, X, Y, Z>
{
    fn from_iter<I: IntoIterator<Item = T>>(iter: I) -> Self {
        let mut data = std::mem::MaybeUninit::<[[[T; Z]; Y]; X]>::uninit();

        let mut x = 0;
        let mut y = 0;
        let mut z = 0;

        for i in iter {
            unsafe { (*data.as_mut_ptr())[x][y][z] = i };
            z += 1;
            if z >= Z {
                z = 0;
                y += 1;
            }
            if y >= Y {
                y = 0;
                x += 1;
            }
        }

        assert_eq!(x, X);
        assert_eq!(y, 0);
        assert_eq!(z, 0);

        RawStaticChunk {
            data: unsafe { data.assume_init() },
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn chunk_into_from_iter() {
        let chunk1 = RawStaticChunk::<i32, 3, 8, 5> {
            data: [[[42; 5]; 8]; 3],
        };

        let chunk1_iter = chunk1.into_iter();

        let chunk2 = RawStaticChunk::from_iter(chunk1_iter);

        assert_eq!(chunk1, chunk2);
    }
}
