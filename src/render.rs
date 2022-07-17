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

#[repr(C)]
#[derive(Default, Copy, Clone, Zeroable, Pod)]
pub struct GPUInstance {
    pub model: [f32; 16],
}

pub const NUM_KEYS: usize = winit::event::VirtualKeyCode::Cut as usize + 1;
pub const NUM_BUTTONS: usize = 3;

pub fn b2u(button: winit::event::MouseButton) -> usize {
    match button {
        winit::event::MouseButton::Left => 0,
        winit::event::MouseButton::Right => 1,
        winit::event::MouseButton::Middle => 2,
        winit::event::MouseButton::Other(x) => x as usize,
    }
}

pub struct Renderer {
    event_loop: EventLoop<()>,
    window: Window,

    keystate: [bool; NUM_KEYS],
    last_keystate: [bool; NUM_KEYS],
    mouse_buttons: [bool; NUM_BUTTONS],
    last_mouse_buttons: [bool; NUM_BUTTONS],
    mouse_pos: (f64, f64),
    last_mouse_pos: (f64, f64),
}

impl Renderer {
    pub fn new(world: &WorldState) -> Renderer {
        let event_loop = EventLoop::new();

        let window = WindowBuilder::new()
            .with_title("vtrace-rs")
            .with_inner_size(winit::dpi::LogicalSize::new(800, 800))
            .build(&event_loop)
            .expect("ERROR: Failed to create window");

        Renderer {
            event_loop,
            window,
            keystate: [false; NUM_KEYS],
            last_keystate: [false; NUM_KEYS],
            mouse_buttons: [false; NUM_BUTTONS],
            last_mouse_buttons: [false; NUM_BUTTONS],
            mouse_pos: (0.0, 0.0),
            last_mouse_pos: (0.0, 0.0),
        }
    }

    pub fn add_texture<T: VoxelFormat<Color>>(&mut self, texture: T) {}

    pub fn update_descriptor(&mut self) {}

    pub fn update_instances(&mut self, instances: Vec<GPUInstance>) {}

    pub fn render_loop(mut self, mut world: WorldState) {
        let mut window_resized = false;

        let mut before_time = Instant::now();
        let mut num_frames_in_sec = 0;
        let mut frame_num = 0;

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
                    num_frames_in_sec += 1;
                    frame_num += 1;
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
