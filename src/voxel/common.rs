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

pub trait Voxel: PartialEq + Eq + Copy + Default {}

pub trait VoxelData<T: Voxel> {
    fn dim_x(&self) -> (i32, i32);
    fn dim_y(&self) -> (i32, i32);
    fn dim_z(&self) -> (i32, i32);
    fn at<'a>(&'a self, x: i32, y: i32, z: i32) -> Option<&'a T>;
    fn at_mut<'a>(&'a mut self, x: i32, y: i32, z: i32) -> Option<&'a mut T>;
}

pub trait VoxelFormat<T: Voxel>:
    VoxelData<T> + IntoVoxelIterator<Item = T> + FromVoxelIterator<T>
{
}

pub trait RawVoxelData<T: Voxel>: VoxelData<T> {
    fn get_raw(&self) -> *const T;
    fn get_raw_mut(&mut self) -> *mut T;
}

pub trait RawVoxelFormat<T: Voxel>: VoxelFormat<T> + RawVoxelData<T> {}

pub fn contains<V: Voxel, T: VoxelData<V>>(voxels: &T, x: i32, y: i32, z: i32) -> bool {
    x >= voxels.dim_x().0
        && y >= voxels.dim_y().0
        && z >= voxels.dim_z().0
        && x < voxels.dim_x().1
        && y < voxels.dim_y().1
        && z < voxels.dim_z().1
}

pub trait VoxelIterator<T: Voxel>: ExactSizeIterator<Item = T> {
    fn dim_x(&self) -> (i32, i32);
    fn dim_y(&self) -> (i32, i32);
    fn dim_z(&self) -> (i32, i32);
}

pub trait IntoVoxelIterator {
    type Item: Voxel;
    type IntoIter: VoxelIterator<Self::Item>;

    fn into_iter(self) -> Self::IntoIter;
}

pub trait FromVoxelIterator<A: Voxel> {
    fn from_iter<T>(iter: T) -> Self
    where
        T: IntoVoxelIterator<Item = A>;
}

#[repr(C)]
#[derive(PartialEq, Eq, Clone, Copy, Default)]
pub struct Color {
    r: u8,
    g: u8,
    b: u8,
    a: u8,
}

impl Color {
    pub fn new(r: u8, g: u8, b: u8, a: u8) -> Self {
        Color { r, g, b, a }
    }

    pub fn from_uint(x: u32) -> Self {
        Color {
            r: (x & 0x000000FF) as u8,
            g: ((x & 0x0000FF00) >> 8) as u8,
            b: ((x & 0x00FF0000) >> 16) as u8,
            a: ((x & 0xFF000000) >> 24) as u8,
        }
    }
}

impl Voxel for u8 {}
impl Voxel for i32 {}
impl Voxel for Color {}
