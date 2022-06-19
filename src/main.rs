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

use vulkano::buffer::*;
use vulkano::command_buffer::*;
use vulkano::device::physical::*;
use vulkano::device::*;
use vulkano::instance::*;
use vulkano::sync::{self, *};

fn main() {
    let instance = Instance::new(InstanceCreateInfo::default())
        .expect("ERROR: Couldn't create Vulkan instance.");

    let physical = PhysicalDevice::enumerate(&instance)
        .next()
        .expect("ERROR: No physical device is available.");

    let queue_family = physical
        .queue_families()
        .find(|&q| q.supports_graphics() && q.supports_compute())
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

    let src_contents = 0..256;
    let src_buffer =
        CpuAccessibleBuffer::from_iter(device.clone(), BufferUsage::all(), false, src_contents)
            .expect("Error: Failed to create buffer.");

    let dst_contents = (0..256).map(|_| 0);
    let dst_buffer =
        CpuAccessibleBuffer::from_iter(device.clone(), BufferUsage::all(), false, dst_contents)
            .expect("Error: Failed to create buffer.");

    let mut builder = AutoCommandBufferBuilder::primary(
        device.clone(),
        queue.family(),
        CommandBufferUsage::OneTimeSubmit,
    )
    .unwrap();

    builder
        .copy_buffer(src_buffer.clone(), dst_buffer.clone())
        .unwrap();

    let command_buffer = builder.build().unwrap();

    let future = sync::now(device.clone())
        .then_execute(queue.clone(), command_buffer)
        .unwrap()
        .then_signal_fence_and_flush()
        .unwrap();
    future.wait(None).unwrap();

    let src_content = src_buffer.read().unwrap();
    let dst_content = dst_buffer.read().unwrap();
    assert_eq!(&*src_content, &*dst_content);
    println!("SUCCESS");
}
