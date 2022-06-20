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

use bytemuck::*;

use image::{ImageBuffer, Rgba};

use vulkano::buffer::*;
use vulkano::command_buffer::*;
use vulkano::device::physical::*;
use vulkano::device::*;
use vulkano::format::*;
use vulkano::image::view::*;
use vulkano::image::*;
use vulkano::instance::*;
use vulkano::pipeline::graphics::input_assembly::*;
use vulkano::pipeline::graphics::vertex_input::*;
use vulkano::pipeline::graphics::viewport::*;
use vulkano::pipeline::*;
use vulkano::render_pass::*;
use vulkano::shader::*;
use vulkano::sync::{self, *};

#[repr(C)]
#[derive(Default, Copy, Clone, Zeroable, Pod)]
struct Vertex {
    position: [f32; 2],
}

vulkano::impl_vertex!(Vertex, position);

fn main() {
    let instance = Instance::new(InstanceCreateInfo::default())
        .expect("ERROR: Couldn't create Vulkan instance");

    let physical = PhysicalDevice::enumerate(&instance)
        .next()
        .expect("ERROR: No physical device is available");

    let queue_family = physical
        .queue_families()
        .find(|&q| q.supports_graphics())
        .expect("ERROR: Couldn't find a queue family supporting graphics");

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

    let vert_shader =
        unsafe { ShaderModule::from_bytes(device.clone(), include_bytes!("../shaders/vert.spv")) }
            .expect("ERROR: Failed to create shader");

    let frag_shader =
        unsafe { ShaderModule::from_bytes(device.clone(), include_bytes!("../shaders/frag.spv")) }
            .expect("ERROR: Failed to create shader");

    let vertices: Vec<Vertex> = vec![[-0.5, -0.5], [0.0, 0.5], [0.5, -0.25]]
        .iter()
        .map(|x| Vertex { position: *x })
        .collect();

    let vertex_buffer = CpuAccessibleBuffer::from_iter(
        device.clone(),
        BufferUsage::vertex_buffer(),
        false,
        vertices,
    )
    .unwrap();

    let render_pass = vulkano::single_pass_renderpass!(device.clone(),
        attachments: {
            color: {
                load: Clear,
                store: Store,
                format: Format::R8G8B8A8_UNORM,
                samples: 1,
            }
        },
        pass: {
            color: [color],
            depth_stencil: {}
        }
    )
    .unwrap();

    let image = StorageImage::new(
        device.clone(),
        ImageDimensions::Dim2d {
            width: 1024,
            height: 1024,
            array_layers: 1,
        },
        Format::R8G8B8A8_UNORM,
        Some(queue.family()),
    )
    .unwrap();

    let view = ImageView::new_default(image.clone()).unwrap();
    let framebuffer = Framebuffer::new(
        render_pass.clone(),
        FramebufferCreateInfo {
            attachments: vec![view],
            ..Default::default()
        },
    )
    .unwrap();

    let buffer = CpuAccessibleBuffer::from_iter(
        device.clone(),
        BufferUsage::all(),
        false,
        (0..1024 * 1024 * 4).map(|_| 0u8),
    )
    .expect("ERROR: Failed to create buffer");

    let viewport = Viewport {
        origin: [0.0, 0.0],
        dimensions: [1024.0, 1024.0],
        depth_range: 0.0..1.0,
    };

    let pipeline = GraphicsPipeline::start()
        .vertex_input_state(BuffersDefinition::new().vertex::<Vertex>())
        .vertex_shader(vert_shader.entry_point("main").unwrap(), ())
        .input_assembly_state(InputAssemblyState::new())
        .viewport_state(ViewportState::viewport_fixed_scissor_irrelevant([viewport]))
        .fragment_shader(frag_shader.entry_point("main").unwrap(), ())
        .render_pass(Subpass::from(render_pass.clone(), 0).unwrap())
        .build(device.clone())
        .unwrap();

    let mut builder = AutoCommandBufferBuilder::primary(
        device.clone(),
        queue.family(),
        CommandBufferUsage::OneTimeSubmit,
    )
    .unwrap();

    builder
        .begin_render_pass(
            framebuffer.clone(),
            SubpassContents::Inline,
            vec![[0.0, 0.0, 1.0, 1.0].into()],
        )
        .unwrap()
        .bind_pipeline_graphics(pipeline.clone())
        .bind_vertex_buffers(0, vertex_buffer.clone())
        .draw(3, 1, 0, 0)
        .unwrap()
        .end_render_pass()
        .unwrap()
        .copy_image_to_buffer(image, buffer.clone())
        .unwrap();

    let command_buffer = builder.build().unwrap();

    let future = sync::now(device.clone())
        .then_execute(queue.clone(), command_buffer)
        .unwrap()
        .then_signal_fence_and_flush()
        .unwrap();
    future.wait(None).unwrap();

    let buffer_content = buffer.read().unwrap();
    let image = ImageBuffer::<Rgba<u8>, _>::from_raw(1024, 1024, &buffer_content[..]).unwrap();
    image.save("image.png").unwrap();

    println!("SUCCESS");
}
