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

#extension GL_EXT_nonuniform_qualifier : enable

layout (location = 0) in vec4 screen_position;
layout (location = 1) in vec4 world_position;
layout (location = 2) in vec4 model_position;
layout (location = 3) in flat uint model_id;

layout (push_constant) uniform PushConstants {
    mat4 projection;
    mat4 camera;
} push;

layout(set = 0, binding = 0) uniform sampler3D tex[];

layout (location = 0) out vec4 color;

void main() 
{
    mat4 inverse_projection = inverse(push.projection);
    mat4 inverse_camera = inverse(push.camera);
    
    vec3 cam_pos = (inverse_camera * vec4(0.0, 0.0, 0.0, 1.0)).xyz;

    mat4 centered_camera = push.camera;
    centered_camera[3][0] = 0.0;
    centered_camera[3][1] = 0.0;
    centered_camera[3][2] = 0.0;
    
    vec3 ray_pos = world_position.xyz;
    vec3 ray_dir = normalize((inverse(centered_camera) * inverse_projection * screen_position).xyz);

    //color = model_id % 2 == 0 ? vec4(vec3(length(ray_pos - cam_pos) / 100.0), 1.0) : vec4(abs(ray_dir), 1.0);
    color = texture(tex[0], model_position.xyz * 0.5 + 0.5);
}
