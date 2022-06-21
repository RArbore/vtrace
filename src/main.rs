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

use std::sync::*;

use vulkano::buffer::*;
use vulkano::command_buffer::*;
use vulkano::device::physical::*;
use vulkano::device::*;
use vulkano::image::view::*;
use vulkano::image::*;
use vulkano::instance::*;
use vulkano::pipeline::graphics::input_assembly::*;
use vulkano::pipeline::graphics::vertex_input::*;
use vulkano::pipeline::graphics::viewport::*;
use vulkano::pipeline::*;
use vulkano::render_pass::*;
use vulkano::shader::*;
use vulkano::swapchain::*;
use vulkano::sync::*;

use vulkano_win::*;

use winit::event_loop::*;
use winit::window::*;

#[repr(C)]
#[derive(Default, Copy, Clone, Zeroable, Pod)]
struct Vertex {
    position: [f32; 2],
}

vulkano::impl_vertex!(Vertex, position);

fn main() {
    let required_extensions = vulkano_win::required_extensions();

    let instance = Instance::new(InstanceCreateInfo {
        enabled_extensions: required_extensions,
        ..Default::default()
    })
    .expect("ERROR: Couldn't create Vulkan instance");

    let event_loop = EventLoop::new();
    let surface = WindowBuilder::new()
        .build_vk_surface(&event_loop, instance.clone())
        .unwrap();

    let device_extensions = DeviceExtensions {
        khr_swapchain: true,
        ..DeviceExtensions::none()
    };

    let (physical, queue_family) = PhysicalDevice::enumerate(&instance)
        .filter(|&p| p.supported_extensions().is_superset_of(&device_extensions))
        .filter_map(|p| {
            p.queue_families()
                .find(|&q| q.supports_graphics() && q.supports_surface(&surface).unwrap_or(false))
                .map(|q| (p, q))
        })
        .min_by_key(|(p, _)| match p.properties().device_type {
            PhysicalDeviceType::DiscreteGpu => 0,
            PhysicalDeviceType::IntegratedGpu => 1,
            PhysicalDeviceType::VirtualGpu => 2,
            PhysicalDeviceType::Cpu => 3,
            PhysicalDeviceType::Other => 4,
        })
        .expect("ERROR: No physical device is available");

    let (device, mut queues) = Device::new(
        physical,
        DeviceCreateInfo {
            queue_create_infos: vec![QueueCreateInfo::family(queue_family)],
            enabled_extensions: physical.required_extensions().union(&device_extensions),
            ..Default::default()
        },
    )
    .expect("ERROR: Couldn't create logical device");

    let queue = queues
        .next()
        .expect("ERROR: No queues given to logical device");

    let caps = physical
        .surface_capabilities(&surface, Default::default())
        .expect("ERROR: Failed to get surface capabilities");

    let dimensions = surface.window().inner_size();
    let composite_alpha = caps.supported_composite_alpha.iter().next().unwrap();
    let image_format = Some(
        physical
            .surface_formats(&surface, Default::default())
            .unwrap()[0]
            .0,
    );

    let (swapchain, images) = Swapchain::new(
        device.clone(),
        surface.clone(),
        SwapchainCreateInfo {
            min_image_count: caps.min_image_count + 1,
            image_format,
            image_extent: dimensions.into(),
            image_usage: ImageUsage::color_attachment(),
            composite_alpha,
            ..Default::default()
        },
    )
    .unwrap();

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
                format: swapchain.image_format(),
                samples: 1,
            }
        },
        pass: {
            color: [color],
            depth_stencil: {}
        }
    )
    .unwrap();

    let framebuffers = images
        .iter()
        .map(|image| {
            let view = ImageView::new_default(image.clone()).unwrap();
            Framebuffer::new(
                render_pass.clone(),
                FramebufferCreateInfo {
                    attachments: vec![view],
                    ..Default::default()
                },
            )
            .unwrap()
        })
        .collect::<Vec<_>>();

    let viewport = Viewport {
        origin: [0.0, 0.0],
        dimensions: surface.window().inner_size().into(),
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

    let mut command_buffers = framebuffers
        .iter()
        .map(|framebuffer| {
            let mut builder = AutoCommandBufferBuilder::primary(
                device.clone(),
                queue.family(),
                CommandBufferUsage::MultipleSubmit,
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
                .draw(vertex_buffer.len() as u32, 1, 0, 0)
                .unwrap()
                .end_render_pass()
                .unwrap();

            Arc::new(builder.build().unwrap())
        })
        .collect::<Vec<_>>();

    let frames_in_flight = images.len();
    let mut fences: Vec<Option<Arc<FenceSignalFuture<_>>>> = vec![None; frames_in_flight];
    let mut previous_fence_i = 0;

    let mut window_resized = false;
    let mut recreate_swapchain = false;

    event_loop.run(move |event, _, control_flow| match event {
        winit::event::Event::WindowEvent {
            event: winit::event::WindowEvent::CloseRequested,
            ..
        } => {
            *control_flow = ControlFlow::Exit;
        }
        winit::event::Event::WindowEvent {
            event: winit::event::WindowEvent::Resized(_),
            ..
        } => {
            window_resized = true;
        }
        winit::event::Event::MainEventsCleared => {
            let (image_i, suboptimal, acquire_future) =
                match acquire_next_image(swapchain.clone(), None) {
                    Ok(r) => r,
                    Err(AcquireError::OutOfDate) => {
                        recreate_swapchain = true;
                        return;
                    }
                    Err(e) => panic!("Error: Failed to acquire next image: {:?}", e),
                };

            if suboptimal {
                recreate_swapchain = true;
            }

            if let Some(image_fence) = &fences[image_i] {
                image_fence.wait(None).unwrap();
            }

            let previous_future = match fences[previous_fence_i].clone() {
                None => {
                    let mut now = now(device.clone());
                    now.cleanup_finished();
                    now.boxed()
                }
                Some(fence) => fence.boxed(),
            };

            let future = previous_future
                .join(acquire_future)
                .then_execute(queue.clone(), command_buffers[image_i].clone())
                .unwrap()
                .then_swapchain_present(queue.clone(), swapchain.clone(), image_i)
                .then_signal_fence_and_flush();

            fences[image_i] = match future {
                Ok(value) => Some(Arc::new(value)),
                Err(FlushError::OutOfDate) => {
                    recreate_swapchain = true;
                    None
                }
                Err(e) => {
                    println!("Error: Failed to flush future: {:?}", e);
                    None
                }
            };

            previous_fence_i = image_i;
        }
        _ => (),
    });
}
