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
layout (location = 4) in mat4 model_matrix;

layout (push_constant) uniform PushConstants {
    mat4 projection;
    mat4 camera;
} push;

layout(set = 0, binding = 0) uniform sampler3D tex[];

layout (location = 0) out vec4 color;

layout (depth_greater) out float gl_FragDepth;

void main() {
    // Since we write to the depth buffer with custom logic, we "statically"
    // write to it. According to the GLSL specification, this means that we
    // must write to it in all branches. Thus, we just re-write the "default"
    // value at the beginning.
    gl_FragDepth = screen_position.z / screen_position.w;

    mat4 inverse_projection = inverse(push.projection);
    mat4 inverse_camera = inverse(push.camera);
    
    vec3 cam_pos = (inverse_camera * vec4(0.0, 0.0, 0.0, 1.0)).xyz;

    mat4 centered_camera = push.camera;
    centered_camera[3][0] = 0.0;
    centered_camera[3][1] = 0.0;
    centered_camera[3][2] = 0.0;
    
    vec3 ray_dir = normalize((inverse(centered_camera) * inverse_projection * screen_position).xyz);

    uvec3 u_model_size = textureSize(tex[model_id], 0);
    vec3 model_size = vec3(u_model_size);
    vec3 model_ray_dir = (inverse(model_matrix) * vec4(ray_dir, 0.0)).xyz;
    vec3 model_ray_pos = ((model_position.xyz + 1.0) / 2.0) * model_size;

    vec3 model_ray_dir_sign = sign(model_ray_dir);
    vec3 model_ray_dir_abs = abs(model_ray_dir);

    model_ray_pos += model_ray_dir * 0.99;

    uint steps = 0;
    uint max_steps = u_model_size.x + u_model_size.y + u_model_size.z;
    while (steps < max_steps && all(greaterThanEqual(model_ray_pos, vec3(0.0))) && all(lessThan(model_ray_pos, model_size))) {
	vec4 texSample = texture(tex[model_id], model_ray_pos / model_size);
	if (texSample.w > 0.0) {
	    color = texSample;
	    return;
	}

	vec3 model_axis_dist = fract(-model_ray_pos * model_ray_dir_sign) + 0.000001;
	vec3 model_ray_dist = model_axis_dist / model_ray_dir_abs;
	float model_nearest_ray_dist = min(model_ray_dist.x, min(model_ray_dist.y, model_ray_dist.z));
	model_ray_pos += model_ray_dir * model_nearest_ray_dist;
	++steps;
    }

    if (steps >= max_steps) {
	color = vec4(0.0, 0.0, 0.0, 1.0);
	return;
    }
    
    discard;
}
