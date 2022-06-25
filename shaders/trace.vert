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

#version 460

layout (location = 0) in vec3 position;

layout (push_constant) uniform PushConstants {
    mat4 projection;
    mat4 camera;
} push;

layout (location = 0) out vec4 out_position;

void main() {
    out_position = push.projection * push.camera * vec4(position, 1.0);
    gl_Position = out_position;
}
