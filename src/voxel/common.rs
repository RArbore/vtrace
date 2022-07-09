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

pub trait Voxel: PartialEq + Eq + Copy + Default + bytemuck::Pod {}

pub trait VoxelFormat<T: Voxel>: IntoVoxelIterator<Item = T> + FromVoxelIterator<T> {
    fn dim_x(&self) -> (i32, i32);
    fn dim_y(&self) -> (i32, i32);
    fn dim_z(&self) -> (i32, i32);
    fn at<'a>(&'a self, x: i32, y: i32, z: i32) -> Option<&'a T>;
    fn at_mut<'a>(&'a mut self, x: i32, y: i32, z: i32) -> Option<&'a mut T>;
}

pub fn contains<V: Voxel, T: VoxelFormat<V>>(voxels: &T, x: i32, y: i32, z: i32) -> bool {
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
#[derive(PartialEq, Eq, Clone, Copy, Default, bytemuck::Zeroable, bytemuck::Pod)]
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
}

impl Voxel for u8 {}
impl Voxel for i32 {}
impl Voxel for Color {}
