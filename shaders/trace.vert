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

layout (location = 0) in vec3 position;
layout (location = 1) in mat4 model;

layout (push_constant) uniform PushConstants {
    mat4 projection;
    mat4 camera;
} push;

layout (location = 0) out vec4 screen_position;
layout (location = 1) out vec4 world_position;
layout (location = 2) out vec4 model_position;
layout (location = 3) out flat uint object_id;
layout (location = 4) out flat uint model_id;
layout (location = 5) out flat mat4 model_matrix;

void main() {
    // To decrease the model instance size from 17 to 16 bytes, store
    // the model id in the bottom right entry in the model matrix.
    // For an arbitrary scale/rotation/translate transformation matrix,
    // this entry will always be 1.
    object_id = gl_InstanceIndex;
    model_id = floatBitsToInt(model[3][3]);
    mat4 recovered_model = model;
    recovered_model[3][3] = 1.0;

    model_matrix = recovered_model;
    model_position = vec4(position, 1.0);
    world_position = recovered_model * model_position;
    screen_position = push.projection * push.camera * world_position;
    gl_Position = screen_position;
}
