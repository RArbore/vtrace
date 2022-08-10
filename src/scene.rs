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

use crate::render::GPUInstance;

pub enum SceneGraph {
    Parent {
        model: Matrix4<f32>,
        total_children: u32,
        children: Vec<SceneGraph>,
    },
    Child {
        model: Matrix4<f32>,
        texture_id: u32,
    },
}

impl SceneGraph {
    fn new() -> Self {
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

    fn get_model(&self) -> &Matrix4<f32> {
        match self {
            SceneGraph::Parent {
                model,
                total_children,
                children,
            } => model,
            SceneGraph::Child { model, texture_id } => model,
        }
    }

    fn get_model_mut(&mut self) -> &mut Matrix4<f32> {
        match self {
            SceneGraph::Parent {
                model,
                total_children,
                children,
            } => model,
            SceneGraph::Child { model, texture_id } => model,
        }
    }

    fn add_child(&mut self, child: SceneGraph) {
        match self {
            SceneGraph::Parent {
                model,
                total_children,
                children,
            } => {
                *total_children += child.num_total_children();
                children.push(child);
            }
            SceneGraph::Child { model, texture_id } => {
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
                            texture_id: *texture_id,
                        },
                        child,
                    ],
                };
            }
        };
    }

    fn num_total_children(&self) -> u32 {
        match self {
            SceneGraph::Parent {
                model,
                total_children,
                children,
            } => *total_children,
            SceneGraph::Child { model, texture_id } => 1,
        }
    }
}

impl IntoIterator for SceneGraph {
    type Item = GPUInstance;
    type IntoIter = SceneGraphIter;

    fn into_iter(self) -> Self::IntoIter {
        SceneGraphIter { scene: vec![self] }
    }
}

pub struct SceneGraphIter {
    scene: Vec<SceneGraph>,
}

impl Iterator for SceneGraphIter {
    type Item = GPUInstance;

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
                self.next()
            }
            SceneGraph::Child { model, texture_id } => {
                Some(GPUInstance::from_model(model, texture_id))
            }
        }
    }
}
