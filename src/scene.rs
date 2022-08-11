/*
 * This file is part of vtrace.
 * vtrace is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 * vtrace is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with vtrace. If not, see <https://www.gnu.org/licenses/>.
 */

use glm::*;

use crate::render::*;

pub enum SceneGraph {
    Parent {
        model: Matrix4<f32>,
        total_children: u32,
        children: Vec<SceneGraph>,
    },
    Child {
        model: Matrix4<f32>,
        texture_handle: TextureHandle,
    },
}

impl SceneGraph {
    pub fn new() -> Self {
        SceneGraph::Parent {
            model: Matrix4::new(
                Vec4::new(1.0, 0.0, 0.0, 0.0),
                Vec4::new(0.0, 1.0, 0.0, 0.0),
                Vec4::new(0.0, 0.0, 1.0, 0.0),
                Vec4::new(0.0, 0.0, 0.0, 1.0),
            ),
            total_children: 0,
            children: vec![],
        }
    }

    pub fn new_child(model: Matrix4<f32>, texture_handle: TextureHandle) -> Self {
        SceneGraph::Child {
            model,
            texture_handle,
        }
    }

    pub fn get_model(&self) -> &Matrix4<f32> {
        match self {
            SceneGraph::Parent {
                model,
                total_children,
                children,
            } => model,
            SceneGraph::Child {
                model,
                texture_handle,
            } => model,
        }
    }

    pub fn get_model_mut(&mut self) -> &mut Matrix4<f32> {
        match self {
            SceneGraph::Parent {
                model,
                total_children,
                children,
            } => model,
            SceneGraph::Child {
                model,
                texture_handle,
            } => model,
        }
    }

    pub fn add_child(&mut self, child: SceneGraph) {
        match self {
            SceneGraph::Parent {
                model,
                total_children,
                children,
            } => {
                *total_children += child.num_total_children();
                children.push(child);
            }
            SceneGraph::Child {
                model,
                texture_handle,
            } => {
                *self = SceneGraph::Parent {
                    model: *model,
                    total_children: 1 + child.num_total_children(),
                    children: vec![
                        SceneGraph::Child {
                            model: Matrix4::new(
                                Vec4::new(1.0, 0.0, 0.0, 0.0),
                                Vec4::new(0.0, 1.0, 0.0, 0.0),
                                Vec4::new(0.0, 0.0, 1.0, 0.0),
                                Vec4::new(0.0, 0.0, 0.0, 1.0),
                            ),
                            texture_handle: *texture_handle,
                        },
                        child,
                    ],
                };
            }
        };
    }

    pub fn num_total_children(&self) -> u32 {
        match self {
            SceneGraph::Parent {
                model,
                total_children,
                children,
            } => *total_children,
            SceneGraph::Child {
                model,
                texture_handle,
            } => 1,
        }
    }
}

impl IntoIterator for SceneGraph {
    type Item = (Matrix4<f32>, TextureHandle);
    type IntoIter = SceneGraphIter;

    fn into_iter(self) -> Self::IntoIter {
        SceneGraphIter {
            scene: vec![self],
            model_stack: vec![],
        }
    }
}

pub struct SceneGraphIter {
    scene: Vec<SceneGraph>,
    model_stack: Vec<Matrix4<f32>>,
}

impl Iterator for SceneGraphIter {
    type Item = (Matrix4<f32>, TextureHandle);

    fn next(&mut self) -> Option<Self::Item> {
        let scene = self.scene.pop()?;
        match scene {
            SceneGraph::Parent {
                model,
                total_children,
                children,
            } => {
                for child in children.into_iter().rev() {
                    self.scene.push(child);
                }
                self.model_stack
                    .push(if let Some(prev) = self.model_stack.last() {
                        *prev * model
                    } else {
                        model
                    });
                self.next()
            }
            SceneGraph::Child {
                model,
                texture_handle,
            } => Some((
                if let Some(prev) = self.model_stack.last() {
                    *prev * model
                } else {
                    model
                },
                texture_handle,
            )),
        }
    }
}
