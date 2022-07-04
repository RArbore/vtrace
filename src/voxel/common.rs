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

pub trait Voxel: PartialEq + Eq + Copy + Default {}

pub trait VoxelFormat<'a, T>: IntoIterator + FromIterator<T> {
    fn at_mut(x: i32, y: i32, z: i32) -> Option<&'a mut T>;
    fn at(x: i32, y: i32, z: i32) -> Option<&'a T>;
}

impl Voxel for i32 {}
