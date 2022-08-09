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

pub enum SceneGraph {
    Parent {
        model: Matrix4<f32>,
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
            children: vec![],
        }
    }

    fn add_child(&mut self, child: SceneGraph) {
        match self {
            SceneGraph::Parent { model, children } => children.push(child),
            SceneGraph::Child { model, texture_id } => {
                *self = SceneGraph::Parent {
                    model: *model,
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
}
