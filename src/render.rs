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

use glm::ext::*;
use glm::*;

use std::sync::*;
use std::time::*;

use vulkano::buffer::cpu_pool::*;
use vulkano::buffer::*;
use vulkano::command_buffer::*;
use vulkano::descriptor_set::layout::*;
use vulkano::descriptor_set::persistent::*;
use vulkano::descriptor_set::*;
use vulkano::device::physical::*;
use vulkano::device::*;
use vulkano::format::*;
use vulkano::image::view::*;
use vulkano::image::*;
use vulkano::instance::*;
use vulkano::memory::pool::*;
use vulkano::pipeline::graphics::color_blend::*;
use vulkano::pipeline::graphics::depth_stencil::*;
use vulkano::pipeline::graphics::input_assembly::*;
use vulkano::pipeline::graphics::rasterization::*;
use vulkano::pipeline::graphics::vertex_input::*;
use vulkano::pipeline::graphics::viewport::*;
use vulkano::pipeline::layout::*;
use vulkano::pipeline::*;
use vulkano::render_pass::*;
use vulkano::sampler::*;
use vulkano::shader::*;
use vulkano::swapchain::*;
use vulkano::sync::*;

use vulkano_win::*;

use winit::dpi::*;
use winit::event_loop::*;
use winit::window::*;

use crate::voxel::*;
use crate::world::*;

#[repr(C)]
#[derive(Default, Copy, Clone, Zeroable, Pod)]
struct GPUVertex {
    position: [f32; 3],
}

vulkano::impl_vertex!(GPUVertex, position);

#[repr(C)]
#[derive(Default, Copy, Clone, Zeroable, Pod)]
pub struct GPUInstance {
    pub model: [f32; 16],
}

vulkano::impl_vertex!(GPUInstance, model);

mod vs {
    vulkano_shaders::shader! {
        ty: "vertex",
        path: "shaders/trace.vert"
    }
}

mod fs {
    vulkano_shaders::shader! {
        ty: "fragment",
        path: "shaders/trace.frag"
    }
}

pub const NUM_KEYS: usize = winit::event::VirtualKeyCode::Cut as usize + 1;
pub const NUM_BUTTONS: usize = 3;

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
    cube_instance_buffer: CpuBufferPool<GPUInstance>,

    render_pass: Arc<RenderPass>,
    framebuffers: Vec<Arc<Framebuffer>>,
    viewport: Viewport,

    graphics_pipeline_layout: Arc<PipelineLayout>,
    graphics_pipeline: Arc<GraphicsPipeline>,
    command_buffers: Vec<Arc<PrimaryAutoCommandBuffer>>,

    perspective: Matrix4<f32>,
    camera: Matrix4<f32>,

    textures: Vec<(Arc<ImmutableImage>, Arc<ImageView<ImmutableImage>>)>,
    pending_texture_futures: Vec<CommandBufferExecFuture<NowFuture, PrimaryAutoCommandBuffer>>,
    sampler: Arc<Sampler>,

    descriptor_set: Arc<PersistentDescriptorSet>,

    instances_chunk: Arc<CpuBufferPoolChunk<GPUInstance, Arc<StdMemoryPool>>>,

    keystate: [bool; NUM_KEYS],
    last_keystate: [bool; NUM_KEYS],
    mouse_buttons: [bool; NUM_BUTTONS],
    last_mouse_buttons: [bool; NUM_BUTTONS],
    mouse_pos: (f64, f64),
    last_mouse_pos: (f64, f64),
}

pub fn b2u(button: winit::event::MouseButton) -> usize {
    match button {
        winit::event::MouseButton::Left => 0,
        winit::event::MouseButton::Right => 1,
        winit::event::MouseButton::Middle => 2,
        winit::event::MouseButton::Other(x) => x as usize,
    }
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

    fn get_device_extensions() -> DeviceExtensions {
        DeviceExtensions {
            khr_swapchain: true,
            ..DeviceExtensions::none()
        }
    }

    fn get_device_features() -> Features {
        Features {
            descriptor_binding_partially_bound: true,
            descriptor_binding_variable_descriptor_count: true,
            runtime_descriptor_array: true,
            ..Features::none()
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
        let (physical_device, queue_family) = PhysicalDevice::enumerate(instance)
            .filter(|&p| {
                p.supported_extensions()
                    .is_superset_of(&Self::get_device_extensions())
                    && p.supported_features()
                        .is_superset_of(&Self::get_device_features())
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
            .expect("ERROR: No suitable physical device is available");

        println!("Using device: {}", physical_device.properties().device_name);

        (physical_device, queue_family)
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
                enabled_features: Self::get_device_features(),
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
                .unwrap()[1]
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
                present_mode: PresentMode::Mailbox,
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
        CpuBufferPool<GPUInstance>,
    ) {
        let vert_shader = vs::load(device.clone()).unwrap();

        let frag_shader = fs::load(device.clone()).unwrap();

        let vertices: Vec<GPUVertex> = vec![
            [-1.0, -1.0, -1.0],
            [-1.0, -1.0, 1.0],
            [-1.0, 1.0, 1.0],
            [1.0, 1.0, -1.0],
            [-1.0, -1.0, -1.0],
            [-1.0, 1.0, -1.0],
            [1.0, -1.0, 1.0],
            [-1.0, -1.0, -1.0],
            [1.0, -1.0, -1.0],
            [1.0, 1.0, -1.0],
            [1.0, -1.0, -1.0],
            [-1.0, -1.0, -1.0],
            [-1.0, -1.0, -1.0],
            [-1.0, 1.0, 1.0],
            [-1.0, 1.0, -1.0],
            [1.0, -1.0, 1.0],
            [-1.0, -1.0, 1.0],
            [-1.0, -1.0, -1.0],
            [-1.0, 1.0, 1.0],
            [-1.0, -1.0, 1.0],
            [1.0, -1.0, 1.0],
            [1.0, 1.0, 1.0],
            [1.0, -1.0, -1.0],
            [1.0, 1.0, -1.0],
            [1.0, -1.0, -1.0],
            [1.0, 1.0, 1.0],
            [1.0, -1.0, 1.0],
            [1.0, 1.0, 1.0],
            [1.0, 1.0, -1.0],
            [-1.0, 1.0, -1.0],
            [1.0, 1.0, 1.0],
            [-1.0, 1.0, -1.0],
            [-1.0, 1.0, 1.0],
            [1.0, 1.0, 1.0],
            [-1.0, 1.0, 1.0],
            [1.0, -1.0, 1.0],
        ]
        .iter()
        .map(|x| GPUVertex { position: *x })
        .collect();

        let cube_vertex_buffer =
            ImmutableBuffer::from_iter(vertices, BufferUsage::vertex_buffer(), queue.clone())
                .unwrap();

        (
            vert_shader,
            frag_shader,
            cube_vertex_buffer,
            CpuBufferPool::vertex_buffer(device.clone()),
        )
    }

    fn create_render_pass(
        device: Arc<Device>,
        swapchain: Arc<Swapchain<Window>>,
    ) -> Arc<RenderPass> {
        vulkano::single_pass_renderpass!(
            device.clone(),
            attachments: {
                color: {
                    load: Clear,
                    store: Store,
                    format: swapchain.image_format(),
                    samples: 1,
                },
                depth: {
                    load: Clear,
                    store: DontCare,
                    format: Format::D32_SFLOAT,
                    samples: 1,
                }
            },
            pass: {
                color: [color],
                depth_stencil: {depth}
            }
        )
        .unwrap()
    }

    fn create_framebuffers(
        device: Arc<Device>,
        images: Vec<Arc<SwapchainImage<Window>>>,
        render_pass: Arc<RenderPass>,
    ) -> Vec<Arc<Framebuffer>> {
        let dimensions = images[0].dimensions().width_height();
        let depth_buffer = ImageView::new_default(
            AttachmentImage::transient(device.clone(), dimensions, Format::D32_SFLOAT).unwrap(),
        )
        .unwrap();

        images
            .iter()
            .map(|image| {
                let view = ImageView::new_default(image.clone()).unwrap();
                Framebuffer::new(
                    render_pass.clone(),
                    FramebufferCreateInfo {
                        attachments: vec![view, depth_buffer.clone()],
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

    fn create_graphics_pipeline_layout(
        device: Arc<Device>,
        frag_shader: Arc<ShaderModule>,
    ) -> Arc<PipelineLayout> {
        let mut layout_create_infos: Vec<_> = DescriptorSetLayoutCreateInfo::from_requirements(
            frag_shader
                .entry_point("main")
                .unwrap()
                .descriptor_requirements(),
        );

        let binding = layout_create_infos[0].bindings.get_mut(&0).unwrap();
        binding.variable_descriptor_count = true;
        binding.descriptor_count = 4096;

        let set_layouts = layout_create_infos
            .into_iter()
            .map(|desc| Ok(DescriptorSetLayout::new(device.clone(), desc.clone())?))
            .collect::<Result<Vec<_>, DescriptorSetLayoutCreationError>>()
            .unwrap();

        PipelineLayout::new(
            device.clone(),
            PipelineLayoutCreateInfo {
                set_layouts,
                push_constant_ranges: frag_shader
                    .entry_point("main")
                    .unwrap()
                    .push_constant_requirements()
                    .cloned()
                    .into_iter()
                    .collect(),
                ..Default::default()
            },
        )
        .unwrap()
    }

    fn create_graphics_pipeline(
        device: Arc<Device>,
        vert_shader: Arc<ShaderModule>,
        frag_shader: Arc<ShaderModule>,
        render_pass: Arc<RenderPass>,
        viewport: Viewport,
        layout: Arc<PipelineLayout>,
    ) -> Arc<GraphicsPipeline> {
        GraphicsPipeline::start()
            .rasterization_state(RasterizationState::new().cull_mode(CullMode::Front))
            .vertex_input_state(
                BuffersDefinition::new()
                    .vertex::<GPUVertex>()
                    .instance::<GPUInstance>(),
            )
            .vertex_shader(vert_shader.entry_point("main").unwrap(), ())
            .input_assembly_state(InputAssemblyState::new())
            .viewport_state(ViewportState::viewport_fixed_scissor_irrelevant([viewport]))
            .color_blend_state(ColorBlendState::new(1).blend_alpha())
            .fragment_shader(frag_shader.entry_point("main").unwrap(), ())
            .depth_stencil_state(DepthStencilState::simple_depth_test())
            .render_pass(Subpass::from(render_pass.clone(), 0).unwrap())
            .with_pipeline_layout(device.clone(), layout)
            .unwrap()
    }

    fn create_command_buffers(
        device: Arc<Device>,
        queue: Arc<Queue>,
        cube_vertex_buffer: Arc<ImmutableBuffer<[GPUVertex]>>,
        cube_instance_buffer: Arc<CpuBufferPoolChunk<GPUInstance, Arc<StdMemoryPool>>>,
        framebuffers: Vec<Arc<Framebuffer>>,
        graphics_pipeline: Arc<GraphicsPipeline>,
        descriptor_set: Arc<PersistentDescriptorSet>,
        perspective: &Matrix4<f32>,
        camera: &Matrix4<f32>,
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
                        vec![
                            [53.0 / 100.0, 81.0 / 100.0, 92.0 / 100.0, 1.0].into(),
                            1.0.into(),
                        ],
                    )
                    .unwrap()
                    .bind_pipeline_graphics(graphics_pipeline.clone())
                    .bind_descriptor_sets(
                        PipelineBindPoint::Graphics,
                        graphics_pipeline.layout().clone(),
                        0,
                        descriptor_set.clone(),
                    )
                    .bind_vertex_buffers(
                        0,
                        (cube_vertex_buffer.clone(), cube_instance_buffer.clone()),
                    )
                    .push_constants(
                        graphics_pipeline.layout().clone(),
                        0,
                        (*perspective, *camera),
                    )
                    .draw(
                        cube_vertex_buffer.len() as u32,
                        cube_instance_buffer.len() as u32,
                        0,
                        0,
                    )
                    .unwrap()
                    .end_render_pass()
                    .unwrap();

                Arc::new(builder.build().unwrap())
            })
            .collect::<Vec<_>>()
    }

    fn create_debug_texture(
        queue: Arc<Queue>,
        color: u32,
    ) -> (Arc<ImmutableImage>, Arc<ImageView<ImmutableImage>>) {
        let (image, image_future) = ImmutableImage::from_iter(
            [color] as [u32; 1],
            ImageDimensions::Dim3d {
                width: 1,
                height: 1,
                depth: 1,
            },
            MipmapsCount::One,
            Format::R8G8B8A8_SRGB,
            queue.clone(),
        )
        .unwrap();
        let image_view = ImageView::new_default(image.clone()).unwrap();

        image_future.flush().unwrap();
        (image, image_view)
    }

    fn create_perspective(surface: Arc<Surface<Window>>) -> Matrix4<f32> {
        let size = Self::get_size(surface);
        perspective(
            80.0 / 180.0 * 3.1415926,
            size.width as f32 / size.height as f32,
            0.01,
            10000.0,
        )
    }

    fn create_camera(position: &Vec3, direction: &Vec3) -> Matrix4<f32> {
        look_at(*position, *position + *direction, vec3(0.0, 1.0, 0.0))
    }

    pub fn new(world: &WorldState) -> Renderer {
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

        let (
            vert_shader,
            frag_shader,
            (cube_vertex_buffer, cube_vertex_buffer_future),
            cube_instance_buffer,
        ) = Self::create_shaders_and_buffers(device.clone(), queue.clone());

        let render_pass = Self::create_render_pass(device.clone(), swapchain.clone());
        let framebuffers =
            Self::create_framebuffers(device.clone(), images.clone(), render_pass.clone());
        let viewport = Self::create_viewport(surface.clone());

        let perspective = Self::create_perspective(surface.clone());
        let camera = Self::create_camera(&world.camera_position, &world.get_camera_direction());

        let graphics_pipeline_layout =
            Self::create_graphics_pipeline_layout(device.clone(), frag_shader.clone());
        let graphics_pipeline = Self::create_graphics_pipeline(
            device.clone(),
            vert_shader.clone(),
            frag_shader.clone(),
            render_pass.clone(),
            viewport.clone(),
            graphics_pipeline_layout.clone(),
        );

        let sampler = Sampler::new(
            device.clone(),
            SamplerCreateInfo {
                ..Default::default()
            },
        )
        .unwrap();

        let textures = vec![Self::create_debug_texture(queue.clone(), 0xFF808080)];

        let layout = graphics_pipeline.layout().set_layouts().get(0).unwrap();
        let descriptor_set = PersistentDescriptorSet::new_variable(
            layout.clone(),
            1,
            [WriteDescriptorSet::image_view_sampler_array(
                0,
                0,
                [(textures.first().unwrap().1.clone() as _, sampler.clone())],
            )],
        )
        .unwrap();

        let instances_chunk = cube_instance_buffer.chunk(vec![]).unwrap();

        let command_buffers = Self::create_command_buffers(
            device.clone(),
            queue.clone(),
            cube_vertex_buffer.clone(),
            instances_chunk.clone(),
            framebuffers.clone(),
            graphics_pipeline.clone(),
            descriptor_set.clone(),
            &perspective,
            &camera,
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
            cube_instance_buffer,
            render_pass,
            framebuffers,
            viewport,
            graphics_pipeline_layout,
            graphics_pipeline,
            command_buffers,
            perspective,
            camera,
            textures,
            pending_texture_futures: vec![],
            sampler,
            descriptor_set,
            instances_chunk,
            keystate: [false; NUM_KEYS],
            last_keystate: [false; NUM_KEYS],
            mouse_buttons: [false; NUM_BUTTONS],
            last_mouse_buttons: [false; NUM_BUTTONS],
            mouse_pos: (0.0, 0.0),
            last_mouse_pos: (0.0, 0.0),
        }
    }

    pub fn add_texture<T: VoxelFormat<Color>>(&mut self, texture: T) {
        let dim_x = texture.dim_x();
        let dim_y = texture.dim_y();
        let dim_z = texture.dim_z();
        assert!(dim_x.1 > dim_x.0);
        assert!(dim_y.1 > dim_y.0);
        assert!(dim_z.1 > dim_z.0);
        let iter = texture.into_iter();
        let (image, image_future) = ImmutableImage::from_iter(
            iter,
            ImageDimensions::Dim3d {
                width: (dim_x.1 - dim_x.0) as u32,
                height: (dim_y.1 - dim_y.0) as u32,
                depth: (dim_z.1 - dim_z.0) as u32,
            },
            MipmapsCount::Log2,
            Format::R8G8B8A8_SRGB,
            self.queue.clone(),
        )
        .unwrap();
        let image_view = ImageView::new_default(image.clone()).unwrap();

        self.textures.push((image.clone(), image_view.clone()));
    }

    pub fn update_descriptor(&mut self) {
        let layout = self
            .graphics_pipeline
            .layout()
            .set_layouts()
            .get(0)
            .unwrap();
        self.descriptor_set = PersistentDescriptorSet::new_variable(
            layout.clone(),
            self.textures.len() as u32,
            [WriteDescriptorSet::image_view_sampler_array(
                0,
                0,
                self.textures
                    .iter()
                    .map(|(_, view)| (view.clone() as _, self.sampler.clone())),
            )],
        )
        .unwrap();
    }

    pub fn update_instances(&mut self, instances: Vec<GPUInstance>) {
        self.instances_chunk = self.cube_instance_buffer.chunk(instances).unwrap();
    }

    pub fn render_loop(mut self, mut world: WorldState) {
        let mut fences: Vec<Option<Arc<FenceSignalFuture<_>>>> = vec![None; self.images.len()];

        let mut previous_fence_i = 0;
        let mut window_resized = false;
        let mut recreate_swapchain = false;

        let mut before_time = Instant::now();
        let mut num_frames_in_sec = 0;

        self.event_loop.run(move |event, _, control_flow| {
            let dt = Instant::now();

            let mut cursor_moved = false;

            match event {
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
                winit::event::Event::WindowEvent {
                    event:
                        winit::event::WindowEvent::KeyboardInput {
                            input:
                                winit::event::KeyboardInput {
                                    state,
                                    virtual_keycode: Some(keycode),
                                    ..
                                },
                            ..
                        },
                    ..
                } => {
                    self.last_keystate[keycode as usize] = self.keystate[keycode as usize];
                    self.keystate[keycode as usize] =
                        if state == winit::event::ElementState::Pressed {
                            true
                        } else {
                            false
                        };
                }
                winit::event::Event::WindowEvent {
                    event: winit::event::WindowEvent::MouseInput { state, button, .. },
                    ..
                } => {
                    self.last_mouse_buttons[b2u(button)] = self.last_mouse_buttons[b2u(button)];
                    self.mouse_buttons[b2u(button)] =
                        if state == winit::event::ElementState::Pressed {
                            true
                        } else {
                            false
                        };
                }
                winit::event::Event::WindowEvent {
                    event:
                        winit::event::WindowEvent::CursorMoved {
                            position: PhysicalPosition { x, y },
                            ..
                        },
                    ..
                } => {
                    self.last_mouse_pos = self.mouse_pos;
                    self.mouse_pos = (x, y);
                    cursor_moved = true;
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

                            let framebuffers = Self::create_framebuffers(
                                self.device.clone(),
                                images.clone(),
                                self.render_pass.clone(),
                            );
                            self.framebuffers = framebuffers;

                            let graphics_pipeline = Self::create_graphics_pipeline(
                                self.device.clone(),
                                self.vert_shader.clone(),
                                self.frag_shader.clone(),
                                self.render_pass.clone(),
                                self.viewport.clone(),
                                self.graphics_pipeline_layout.clone(),
                            );
                            self.graphics_pipeline = graphics_pipeline;

                            let perspective = Self::create_perspective(self.surface.clone());
                            self.perspective = perspective;

                            window_resized = false;
                        }

                        recreate_swapchain = false;
                    }

                    let camera =
                        Self::create_camera(&world.camera_position, &world.get_camera_direction());
                    self.camera = camera;

                    let command_buffers = Self::create_command_buffers(
                        self.device.clone(),
                        self.queue.clone(),
                        self.cube_vertex_buffer.clone(),
                        self.instances_chunk.clone(),
                        self.framebuffers.clone(),
                        self.graphics_pipeline.clone(),
                        self.descriptor_set.clone(),
                        &self.perspective,
                        &self.camera,
                    );
                    self.command_buffers = command_buffers;

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
                    num_frames_in_sec += 1;
                }
                _ => (),
            }
            let elapsed = before_time.elapsed().as_micros();
            if elapsed > 1000000 {
                println!("FPS: {}", num_frames_in_sec);
                before_time = Instant::now();
                num_frames_in_sec = 0;
            }

            world.update(
                dt.elapsed().as_millis() as f32 / 1000.0,
                &self.keystate,
                &self.last_keystate,
                &self.mouse_buttons,
                &self.last_mouse_buttons,
                &self.mouse_pos,
                &self.last_mouse_pos,
                cursor_moved,
            );
        });
    }
}
