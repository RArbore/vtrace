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

use vulkano::device::physical::*;
use vulkano::device::*;
use vulkano::instance::*;

fn main() {
    let instance = Instance::new(InstanceCreateInfo::default())
        .expect("ERROR: Couldn't create Vulkan instance.");
    let physical = PhysicalDevice::enumerate(&instance)
        .next()
        .expect("ERROR: No physical device is available.");
    let queue_family = physical
        .queue_families()
        .find(|&q| q.supports_graphics())
        .expect("ERROR: Couldn't find a graphical queue family.");
    let (device, mut queues) = Device::new(
        physical,
        DeviceCreateInfo {
            queue_create_infos: vec![QueueCreateInfo::family(queue_family)],
            ..Default::default()
        },
    )
    .expect("ERROR: Couldn't create logical device.");
    let queue = queues
        .next()
        .expect("ERROR: No queues given to logical device.");
}
