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

use super::common::*;

#[derive(PartialEq, Eq, Debug, Clone, Copy)]
pub struct RawStaticChunk<T: Voxel, const X: usize, const Y: usize, const Z: usize> {
    data: [[[T; Z]; Y]; X],
}

impl<T: Voxel, const X: usize, const Y: usize, const Z: usize> RawStaticChunk<T, X, Y, Z> {
    pub fn new(v: T) -> Self {
        RawStaticChunk {
            data: [[[v; Z]; Y]; X],
        }
    }
}

pub struct RawStaticChunkIter<T: Voxel, const X: usize, const Y: usize, const Z: usize> {
    chunk: RawStaticChunk<T, X, Y, Z>,
    index: usize,
}

impl<T: Voxel, const X: usize, const Y: usize, const Z: usize> IntoVoxelIterator
    for RawStaticChunk<T, X, Y, Z>
{
    type Item = T;
    type IntoIter = RawStaticChunkIter<T, X, Y, Z>;

    fn into_iter(self) -> Self::IntoIter {
        RawStaticChunkIter {
            chunk: self,
            index: 0,
        }
    }
}

impl<T: Voxel, const X: usize, const Y: usize, const Z: usize> Iterator
    for RawStaticChunkIter<T, X, Y, Z>
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

impl<T: Voxel, const X: usize, const Y: usize, const Z: usize> ExactSizeIterator
    for RawStaticChunkIter<T, X, Y, Z>
{
    fn len(&self) -> usize {
        self.dim_x().1 as usize * self.dim_y().1 as usize * self.dim_z().1 as usize
    }
}

impl<T: Voxel, const X: usize, const Y: usize, const Z: usize> VoxelIterator<T>
    for RawStaticChunkIter<T, X, Y, Z>
{
    fn dim_x(&self) -> (i32, i32) {
        self.chunk.dim_x()
    }

    fn dim_y(&self) -> (i32, i32) {
        self.chunk.dim_y()
    }

    fn dim_z(&self) -> (i32, i32) {
        self.chunk.dim_z()
    }
}

impl<T: Voxel, const X: usize, const Y: usize, const Z: usize> FromVoxelIterator<T>
    for RawStaticChunk<T, X, Y, Z>
{
    fn from_iter<I: IntoVoxelIterator<Item = T>>(iter: I) -> Self {
        let mut chunk = RawStaticChunk::new(Default::default());

        let mut x = 0;
        let mut y = 0;
        let mut z = 0;

        for i in iter.into_iter() {
            chunk.data[x][y][z] = i;
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

        chunk
    }
}

impl<T: Voxel, const X: usize, const Y: usize, const Z: usize> VoxelData<T>
    for RawStaticChunk<T, X, Y, Z>
{
    fn dim_x(&self) -> (i32, i32) {
        (0, X as i32)
    }

    fn dim_y(&self) -> (i32, i32) {
        (0, Y as i32)
    }

    fn dim_z(&self) -> (i32, i32) {
        (0, Z as i32)
    }

    fn at<'a>(&'a self, x: i32, y: i32, z: i32) -> Option<&'a T> {
        if contains(self, x, y, z) {
            let x = x as usize;
            let y = y as usize;
            let z = z as usize;
            Some(&self.data[x][y][z])
        } else {
            None
        }
    }

    fn at_mut<'a>(&'a mut self, x: i32, y: i32, z: i32) -> Option<&'a mut T> {
        if contains(self, x, y, z) {
            let x = x as usize;
            let y = y as usize;
            let z = z as usize;
            Some(&mut self.data[x][y][z])
        } else {
            None
        }
    }
}

impl<T: Voxel, const X: usize, const Y: usize, const Z: usize> VoxelFormat<T>
    for RawStaticChunk<T, X, Y, Z>
{
}

impl<T: Voxel, const X: usize, const Y: usize, const Z: usize> RawVoxelData<T>
    for RawStaticChunk<T, X, Y, Z>
{
    fn get_raw(&self) -> *const T {
        self.data[0][0].as_ptr()
    }

    fn get_raw_mut(&mut self) -> *mut T {
        self.data[0][0].as_mut_ptr()
    }
}

impl<T: Voxel, const X: usize, const Y: usize, const Z: usize> RawVoxelFormat<T>
    for RawStaticChunk<T, X, Y, Z>
{
}

#[derive(PartialEq, Eq, Debug)]
pub struct RawDynamicChunk<T: Voxel> {
    data: Box<[T]>,
    dim_x: usize,
    dim_y: usize,
    dim_z: usize,
}

impl<T: Voxel> RawDynamicChunk<T> {
    pub fn new(dim_x: usize, dim_y: usize, dim_z: usize, v: T) -> Self {
        let vec = vec![v; dim_x * dim_y * dim_z];
        RawDynamicChunk {
            data: vec.into_boxed_slice(),
            dim_x,
            dim_y,
            dim_z,
        }
    }
}

pub struct RawDynamicChunkIter<T: Voxel> {
    chunk: RawDynamicChunk<T>,
    index: usize,
}

impl<T: Voxel> IntoVoxelIterator for RawDynamicChunk<T> {
    type Item = T;
    type IntoIter = RawDynamicChunkIter<T>;

    fn into_iter(self) -> Self::IntoIter {
        RawDynamicChunkIter {
            chunk: self,
            index: 0,
        }
    }
}

impl<T: Voxel> Iterator for RawDynamicChunkIter<T> {
    type Item = T;

    fn next(&mut self) -> Option<T> {
        if self.index < self.chunk.dim_x * self.chunk.dim_y * self.chunk.dim_z {
            let result = self.chunk.data[self.index];
            self.index += 1;
            Some(result)
        } else {
            None
        }
    }
}

impl<T: Voxel> ExactSizeIterator for RawDynamicChunkIter<T> {
    fn len(&self) -> usize {
        self.dim_x().1 as usize * self.dim_y().1 as usize * self.dim_z().1 as usize
    }
}

impl<T: Voxel> VoxelIterator<T> for RawDynamicChunkIter<T> {
    fn dim_x(&self) -> (i32, i32) {
        self.chunk.dim_x()
    }

    fn dim_y(&self) -> (i32, i32) {
        self.chunk.dim_y()
    }

    fn dim_z(&self) -> (i32, i32) {
        self.chunk.dim_z()
    }
}

impl<'a, T: Voxel> FromVoxelIterator<T> for RawDynamicChunk<T> {
    fn from_iter<I: IntoVoxelIterator<Item = T>>(into_iter: I) -> Self {
        let iter = into_iter.into_iter();

        let dim_x = iter.dim_x().1 as usize;
        let dim_y = iter.dim_y().1 as usize;
        let dim_z = iter.dim_z().1 as usize;

        let mut chunk = RawDynamicChunk::new(dim_x, dim_y, dim_z, Default::default());

        let mut index = 0;

        for i in iter {
            chunk.data[index] = i;
            index += 1;
        }

        assert_eq!(index, dim_x * dim_y * dim_z);

        chunk
    }
}

impl<T: Voxel> VoxelData<T> for RawDynamicChunk<T> {
    fn dim_x(&self) -> (i32, i32) {
        (0, self.dim_x as i32)
    }

    fn dim_y(&self) -> (i32, i32) {
        (0, self.dim_y as i32)
    }

    fn dim_z(&self) -> (i32, i32) {
        (0, self.dim_z as i32)
    }

    fn at<'a>(&'a self, x: i32, y: i32, z: i32) -> Option<&'a T> {
        if contains(self, x, y, z) {
            let x = x as usize;
            let y = y as usize;
            let z = z as usize;
            Some(&self.data[z + self.dim_z * (y + self.dim_y * x)])
        } else {
            None
        }
    }

    fn at_mut<'a>(&'a mut self, x: i32, y: i32, z: i32) -> Option<&'a mut T> {
        if contains(self, x, y, z) {
            let x = x as usize;
            let y = y as usize;
            let z = z as usize;
            Some(&mut self.data[z + self.dim_z * (y + self.dim_y * x)])
        } else {
            None
        }
    }
}

impl<T: Voxel> VoxelFormat<T> for RawDynamicChunk<T> {}

impl<T: Voxel> RawVoxelData<T> for RawDynamicChunk<T> {
    fn get_raw(&self) -> *const T {
        self.data.as_ptr()
    }

    fn get_raw_mut(&mut self) -> *mut T {
        self.data.as_mut_ptr()
    }
}

impl<T: Voxel> RawVoxelFormat<T> for RawDynamicChunk<T> {}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn rawchunk_test1() {
        let chunk1 = RawStaticChunk::<i32, 3, 8, 5>::new(42);

        let chunk2 = RawDynamicChunk::from_iter(chunk1);

        for v2 in chunk2.into_iter() {
            assert_eq!(42, v2);
        }
    }

    #[test]
    fn rawchunk_test2() {
        let chunk1 = RawDynamicChunk::<i32>::new(3, 8, 5, 42);
        let chunk1_iter = chunk1.into_iter();

        for v in chunk1_iter {
            assert_eq!(v, 42);
        }
    }
}
