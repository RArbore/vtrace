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

use winit::dpi::*;
use winit::event_loop::*;
use winit::window::*;

#[repr(C)]
#[derive(Default, Copy, Clone, Zeroable, Pod)]
struct GPUVertex {
    position: [f32; 2],
}

vulkano::impl_vertex!(GPUVertex, position);

pub struct Renderer {
    event_loop: EventLoop<()>,
    surface: Arc<Surface<Window>>,

    instance: Arc<Instance>,
    device: Arc<Device>,
    queue: Arc<Queue>,

    swapchain: Arc<Swapchain<Window>>,
    images: Vec<Arc<SwapchainImage<Window>>>,

    vert_shader: Arc<ShaderModule>,
    frag_shader: Arc<ShaderModule>,
    cube_vertex_buffer: Arc<ImmutableBuffer<[GPUVertex]>>,

    render_pass: Arc<RenderPass>,
    framebuffers: Vec<Arc<Framebuffer>>,
    viewport: Viewport,

    graphics_pipeline: Arc<GraphicsPipeline>,
    command_buffers: Vec<Arc<PrimaryAutoCommandBuffer>>,
}

impl Renderer {
    fn get_caps<'a>(
        physical: PhysicalDevice<'a>,
        surface: Arc<Surface<Window>>,
    ) -> SurfaceCapabilities {
        physical
            .surface_capabilities(&surface, Default::default())
            .expect("ERROR: Failed to get surface capabilities")
    }

    fn get_size(surface: Arc<Surface<Window>>) -> PhysicalSize<u32> {
        surface.window().inner_size()
    }

    fn get_shader(device: Arc<Device>, bytes: &[u8]) -> Arc<ShaderModule> {
        unsafe { ShaderModule::from_bytes(device.clone(), bytes) }.unwrap()
    }

    fn get_device_extensions() -> DeviceExtensions {
        DeviceExtensions {
            khr_swapchain: true,
            ..DeviceExtensions::none()
        }
    }

    fn create_instance() -> Arc<Instance> {
        Instance::new(InstanceCreateInfo {
            enabled_extensions: required_extensions(),
            ..Default::default()
        })
        .expect("ERROR: Couldn't create Vulkan instance")
    }

    fn create_physical_and_queue_family<'a>(
        instance: &'a Arc<Instance>,
        surface: Arc<Surface<Window>>,
    ) -> (PhysicalDevice<'a>, QueueFamily<'a>) {
        PhysicalDevice::enumerate(instance)
            .filter(|&p| {
                p.supported_extensions()
                    .is_superset_of(&Self::get_device_extensions())
            })
            .filter_map(|p| {
                p.queue_families()
                    .find(|&q| {
                        q.supports_graphics() && q.supports_surface(&surface).unwrap_or(false)
                    })
                    .map(|q| (p, q))
            })
            .min_by_key(|(p, _)| match p.properties().device_type {
                PhysicalDeviceType::DiscreteGpu => 0,
                PhysicalDeviceType::IntegratedGpu => 1,
                PhysicalDeviceType::VirtualGpu => 2,
                PhysicalDeviceType::Cpu => 3,
                PhysicalDeviceType::Other => 4,
            })
            .expect("ERROR: No physical device is available")
    }

    fn create_device_and_queue<'a>(
        physical: PhysicalDevice<'a>,
        queue_family: QueueFamily<'a>,
    ) -> (Arc<Device>, Arc<Queue>) {
        let (device, mut queues) = Device::new(
            physical,
            DeviceCreateInfo {
                queue_create_infos: vec![QueueCreateInfo::family(queue_family)],
                enabled_extensions: physical
                    .required_extensions()
                    .union(&Self::get_device_extensions()),
                ..Default::default()
            },
        )
        .expect("ERROR: Couldn't create logical device");

        let queue = queues
            .next()
            .expect("ERROR: No queues given to logical device");

        (device, queue)
    }

    fn create_swapchain<'a>(
        physical: PhysicalDevice<'a>,
        device: Arc<Device>,
        surface: Arc<Surface<Window>>,
    ) -> (Arc<Swapchain<Window>>, Vec<Arc<SwapchainImage<Window>>>) {
        let dimensions = Self::get_size(surface.clone());
        let composite_alpha = Self::get_caps(physical, surface.clone())
            .supported_composite_alpha
            .iter()
            .next()
            .unwrap();
        let image_format = Some(
            physical
                .surface_formats(&surface, Default::default())
                .unwrap()[0]
                .0,
        );

        Swapchain::new(
            device.clone(),
            surface.clone(),
            SwapchainCreateInfo {
                min_image_count: Self::get_caps(physical, surface).min_image_count + 1,
                image_format,
                image_extent: dimensions.into(),
                image_usage: ImageUsage::color_attachment(),
                composite_alpha,
                ..Default::default()
            },
        )
        .unwrap()
    }

    fn create_shaders_and_buffers(
        device: Arc<Device>,
        queue: Arc<Queue>,
    ) -> (
        Arc<ShaderModule>,
        Arc<ShaderModule>,
        (
            Arc<ImmutableBuffer<[GPUVertex]>>,
            CommandBufferExecFuture<NowFuture, PrimaryAutoCommandBuffer>,
        ),
    ) {
        let vert_shader = Self::get_shader(device.clone(), include_bytes!("../shaders/vert.spv"));

        let frag_shader = Self::get_shader(device.clone(), include_bytes!("../shaders/frag.spv"));

        let vertices: Vec<GPUVertex> = vec![[-0.5, -0.5], [0.0, 0.5], [0.5, -0.25]]
            .iter()
            .map(|x| GPUVertex { position: *x })
            .collect();

        let vertex_buffer =
            ImmutableBuffer::from_iter(vertices, BufferUsage::vertex_buffer(), queue.clone())
                .unwrap();

        (vert_shader, frag_shader, vertex_buffer)
    }

    fn create_render_pass(
        device: Arc<Device>,
        swapchain: Arc<Swapchain<Window>>,
    ) -> Arc<RenderPass> {
        vulkano::single_pass_renderpass!(device.clone(),
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
        .unwrap()
    }

    fn create_framebuffers(
        images: Vec<Arc<SwapchainImage<Window>>>,
        render_pass: Arc<RenderPass>,
    ) -> Vec<Arc<Framebuffer>> {
        images
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
            .collect::<Vec<_>>()
    }

    fn create_viewport(surface: Arc<Surface<Window>>) -> Viewport {
        Viewport {
            origin: [0.0, 0.0],
            dimensions: Self::get_size(surface).into(),
            depth_range: 0.0..1.0,
        }
    }

    fn create_graphics_pipeline(
        device: Arc<Device>,
        vert_shader: Arc<ShaderModule>,
        frag_shader: Arc<ShaderModule>,
        render_pass: Arc<RenderPass>,
        viewport: Viewport,
    ) -> Arc<GraphicsPipeline> {
        GraphicsPipeline::start()
            .vertex_input_state(BuffersDefinition::new().vertex::<GPUVertex>())
            .vertex_shader(vert_shader.entry_point("main").unwrap(), ())
            .input_assembly_state(InputAssemblyState::new())
            .viewport_state(ViewportState::viewport_fixed_scissor_irrelevant([viewport]))
            .fragment_shader(frag_shader.entry_point("main").unwrap(), ())
            .render_pass(Subpass::from(render_pass.clone(), 0).unwrap())
            .build(device.clone())
            .unwrap()
    }

    fn create_command_buffers(
        device: Arc<Device>,
        queue: Arc<Queue>,
        cube_vertex_buffer: Arc<ImmutableBuffer<[GPUVertex]>>,
        framebuffers: Vec<Arc<Framebuffer>>,
        graphics_pipeline: Arc<GraphicsPipeline>,
    ) -> Vec<Arc<PrimaryAutoCommandBuffer>> {
        framebuffers
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
                    .bind_pipeline_graphics(graphics_pipeline.clone())
                    .bind_vertex_buffers(0, cube_vertex_buffer.clone())
                    .draw(cube_vertex_buffer.len() as u32, 1, 0, 0)
                    .unwrap()
                    .end_render_pass()
                    .unwrap();

                Arc::new(builder.build().unwrap())
            })
            .collect::<Vec<_>>()
    }

    pub fn new() -> Renderer {
        let instance = Self::create_instance();

        let event_loop = EventLoop::new();
        let surface = WindowBuilder::new()
            .build_vk_surface(&event_loop, instance.clone())
            .unwrap();

        let (physical, queue_family) =
            Self::create_physical_and_queue_family(&instance, surface.clone());

        let (device, queue) = Self::create_device_and_queue(physical.clone(), queue_family.clone());

        let (swapchain, images) =
            Self::create_swapchain(physical.clone(), device.clone(), surface.clone());

        let (vert_shader, frag_shader, (cube_vertex_buffer, cube_vertex_buffer_future)) =
            Self::create_shaders_and_buffers(device.clone(), queue.clone());

        let render_pass = Self::create_render_pass(device.clone(), swapchain.clone());
        let framebuffers = Self::create_framebuffers(images.clone(), render_pass.clone());
        let viewport = Self::create_viewport(surface.clone());

        let graphics_pipeline = Self::create_graphics_pipeline(
            device.clone(),
            vert_shader.clone(),
            frag_shader.clone(),
            render_pass.clone(),
            viewport.clone(),
        );
        let command_buffers = Self::create_command_buffers(
            device.clone(),
            queue.clone(),
            cube_vertex_buffer.clone(),
            framebuffers.clone(),
            graphics_pipeline.clone(),
        );

        cube_vertex_buffer_future.flush().unwrap();

        Renderer {
            event_loop,
            surface,
            instance,
            device,
            queue,
            swapchain,
            images,
            vert_shader,
            frag_shader,
            cube_vertex_buffer,
            render_pass,
            framebuffers,
            viewport,
            graphics_pipeline,
            command_buffers,
        }
    }

    pub fn render_loop(mut self) {
        let mut fences: Vec<Option<Arc<FenceSignalFuture<_>>>> = vec![None; self.images.len()];

        let mut previous_fence_i = 0;
        let mut window_resized = false;
        let mut recreate_swapchain = false;

        self.event_loop
            .run(move |event, _, control_flow| match event {
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
                    if window_resized || recreate_swapchain {
                        let new_dimensions = Self::get_size(self.surface.clone());

                        let (swapchain, images) =
                            match self.swapchain.recreate(SwapchainCreateInfo {
                                image_extent: new_dimensions.into(),
                                ..self.swapchain.create_info()
                            }) {
                                Ok(s) => s,
                                Err(SwapchainCreationError::ImageExtentNotSupported { .. }) => {
                                    return
                                }
                                Err(e) => panic!("ERROR: Failed to recreate swapchain: {:?}", e),
                            };

                        self.swapchain = swapchain;

                        if window_resized {
                            self.viewport.dimensions = new_dimensions.into();

                            let framebuffers =
                                Self::create_framebuffers(images.clone(), self.render_pass.clone());
                            self.framebuffers = framebuffers;

                            let graphics_pipeline = Self::create_graphics_pipeline(
                                self.device.clone(),
                                self.vert_shader.clone(),
                                self.frag_shader.clone(),
                                self.render_pass.clone(),
                                self.viewport.clone(),
                            );
                            self.graphics_pipeline = graphics_pipeline;

                            let command_buffers = Self::create_command_buffers(
                                self.device.clone(),
                                self.queue.clone(),
                                self.cube_vertex_buffer.clone(),
                                self.framebuffers.clone(),
                                self.graphics_pipeline.clone(),
                            );
                            self.command_buffers = command_buffers;

                            window_resized = false;
                        }

                        recreate_swapchain = false;
                    }

                    let (image_i, suboptimal, acquire_future) =
                        match acquire_next_image(self.swapchain.clone(), None) {
                            Ok(r) => r,
                            Err(AcquireError::OutOfDate) => {
                                recreate_swapchain = true;
                                return;
                            }
                            Err(e) => panic!("ERROR: Failed to acquire next image: {:?}", e),
                        };

                    if suboptimal {
                        recreate_swapchain = true;
                    }

                    if let Some(image_fence) = &fences[image_i] {
                        image_fence.wait(None).unwrap();
                    }

                    let previous_future = match fences[previous_fence_i].clone() {
                        None => {
                            let mut now = now(self.device.clone());
                            now.cleanup_finished();
                            now.boxed()
                        }
                        Some(fence) => fence.boxed(),
                    };

                    let future = previous_future
                        .join(acquire_future)
                        .then_execute(self.queue.clone(), self.command_buffers[image_i].clone())
                        .unwrap()
                        .then_swapchain_present(self.queue.clone(), self.swapchain.clone(), image_i)
                        .then_signal_fence_and_flush();

                    fences[image_i] = match future {
                        Ok(value) => Some(Arc::new(value)),
                        Err(FlushError::OutOfDate) => {
                            recreate_swapchain = true;
                            None
                        }
                        Err(e) => {
                            println!("ERROR: Failed to flush future: {:?}", e);
                            None
                        }
                    };

                    previous_fence_i = image_i;
                }
                _ => (),
            });
    }
}
