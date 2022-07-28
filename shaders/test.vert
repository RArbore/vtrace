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

#version 460

layout(location = 0) in vec3 position;
layout (location = 1) in mat4 model;

layout(location = 0) out vec3 fragColor;

layout (push_constant) uniform PushConstants {
    mat4 projection;
    mat4 camera;
} push;

vec3 colors[4] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0),
    vec3(1.0, 1.0, 1.0)
);

void main() {
    gl_Position = push.projection * push.camera * model * vec4(position, 1.0);
    fragColor = colors[gl_VertexIndex % 4];
}
