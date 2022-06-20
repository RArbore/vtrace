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
use vulkano::descriptor_set::*;
use vulkano::device::physical::*;
use vulkano::device::*;
use vulkano::instance::*;
use vulkano::pipeline::*;
use vulkano::shader::*;
use vulkano::sync::{self, *};

fn main() {
    let instance = Instance::new(InstanceCreateInfo::default())
        .expect("ERROR: Couldn't create Vulkan instance");

    let physical = PhysicalDevice::enumerate(&instance)
        .next()
        .expect("ERROR: No physical device is available");

    let queue_family = physical
        .queue_families()
        .find(|&q| q.supports_graphics() && q.supports_compute())
        .expect("ERROR: Couldn't find a graphical queue family");

    let (device, mut queues) = Device::new(
        physical,
        DeviceCreateInfo {
            queue_create_infos: vec![QueueCreateInfo::family(queue_family)],
            ..Default::default()
        },
    )
    .expect("ERROR: Couldn't create logical device");

    let queue = queues
        .next()
        .expect("ERROR: No queues given to logical device");

    let shader = unsafe {
        ShaderModule::from_bytes(device.clone(), include_bytes!("../shaders/compute.spv"))
    }
    .expect("ERROR: Failed to create shader");

    let compute_pipeline = ComputePipeline::new(
        device.clone(),
        shader.entry_point("main").unwrap(),
        &(),
        None,
        |_| {},
    )
    .expect("ERROR: Failed to create compute pipeline");

    let buffer = CpuAccessibleBuffer::from_iter(device.clone(), BufferUsage::all(), false, 0..256)
        .expect("Error: Failed to create buffer");

    let layout = compute_pipeline.layout().set_layouts().get(0).unwrap();
    let set = PersistentDescriptorSet::new(
        layout.clone(),
        [WriteDescriptorSet::buffer(0, buffer.clone())],
    )
    .unwrap();

    let mut builder = AutoCommandBufferBuilder::primary(
        device.clone(),
        queue.family(),
        CommandBufferUsage::OneTimeSubmit,
    )
    .unwrap();

    builder
        .bind_pipeline_compute(compute_pipeline.clone())
        .bind_descriptor_sets(
            PipelineBindPoint::Compute,
            compute_pipeline.layout().clone(),
            0,
            set,
        )
        .dispatch([4, 1, 1])
        .unwrap();

    let command_buffer = builder.build().unwrap();

    let future = sync::now(device.clone())
        .then_execute(queue.clone(), command_buffer)
        .unwrap()
        .then_signal_fence_and_flush()
        .unwrap();

    future.wait(None).unwrap();

    let read_contents = buffer.read().unwrap();
    for (n, val) in read_contents.iter().enumerate() {
        assert_eq!(n as u32 % 64 | n as u32 / 64, *val);
    }

    println!("SUCCESS");
}
