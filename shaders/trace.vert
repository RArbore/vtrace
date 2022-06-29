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
layout (location = 1) in mat4 model;

layout (push_constant) uniform PushConstants {
    mat4 projection;
    mat4 camera;
} push;

layout (location = 0) out vec4 out_position;
layout (location = 1) out flat uint model_id;

void main() {
    // To decrease the model instance size from 17 to 16 bytes, store
    // the model id in the bottom right entry in the model matrix.
    // For an arbitrary scale/rotation/translate transformation matrix,
    // this entry will always be 1.
    model_id = floatBitsToInt(model[3][3]);
    mat4 recovered_model = transpose(model);
    recovered_model[3][3] = 1.0;

    out_position = push.projection * push.camera * recovered_model * vec4(position, 1.0);
    gl_Position = out_position;
}
