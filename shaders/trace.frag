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

#extension GL_EXT_nonuniform_qualifier : enable

layout (location = 0) in vec4 screen_position;
layout (location = 1) in vec4 world_position;
layout (location = 2) in vec4 model_position;
layout (location = 3) in flat uint object_id;
layout (location = 4) in flat uint model_id;
layout (location = 5) in flat mat4 model_matrix;

layout (push_constant) uniform PushConstants {
    mat4 projection;
    mat4 camera;
} push;

//layout(set = 0, binding = 0) uniform sampler2D history_read;
//layout(set = 0, binding = 1) uniform sampler3D tex[];
layout(set = 0, binding = 0) uniform sampler3D tex[];

layout (location = 0) out vec4 color;
layout (location = 1) out vec4 history_write;

layout (depth_greater) out float gl_FragDepth;

#define LOD_SCALE 0.0
#define LOD_MAX 4

void main() {
    // Since we write to the depth buffer with custom logic, we "statically"
    // write to it. According to the GLSL specification, this means that we
    // must write to it in all branches. Thus, we just re-write the "default"
    // value at the beginning.
    gl_FragDepth = screen_position.z / screen_position.w;

    mat4 inverse_projection = inverse(push.projection);
    mat4 inverse_camera = inverse(push.camera);
    
    vec3 cam_pos = (inverse_camera * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    vec3 ray_pos = (inverse_camera * inverse_projection * screen_position).xyz;

    mat4 centered_camera = push.camera;
    centered_camera[3][0] = 0.0;
    centered_camera[3][1] = 0.0;
    centered_camera[3][2] = 0.0;
    
    vec3 ray_dir = normalize((inverse(centered_camera) * inverse_projection * screen_position).xyz);

    int lod = min(int(LOD_SCALE * length(cam_pos - ray_pos)), LOD_MAX);

    ivec3 i_model_size = textureSize(tex[model_id], lod);
    vec3 model_size = vec3(i_model_size);
    vec3 model_ray_dir = (inverse(model_matrix) * vec4(ray_dir, 0.0)).xyz;
    vec3 model_ray_pos = (model_position.xyz + 0.5) * model_size;

    ivec3 model_ray_voxel = ivec3(floor(min(model_ray_pos, model_size - 1.0)));
    ivec3 model_ray_step = ivec3(sign(model_ray_dir));
    vec3 model_ray_delta = abs(vec3(length(model_ray_dir)) / model_ray_dir);
    vec3 model_side_dist = (sign(model_ray_dir) * (vec3(model_ray_voxel) - model_ray_pos) + (sign(model_ray_dir) * 0.5) + 0.5) * model_ray_delta;

    uint steps = 0;
    uint max_steps = i_model_size.x + i_model_size.y + i_model_size.z;
    while (steps < max_steps && all(greaterThanEqual(model_ray_voxel, ivec3(0))) && all(lessThan(model_ray_voxel, i_model_size))) {
	vec4 texSample = texture(tex[model_id], model_ray_voxel / model_size);

	if (texSample.w > 0.0) {
	    color = texSample;
	    return;
	}
	
	bvec3 mask = lessThanEqual(model_side_dist.xyz, min(model_side_dist.yzx, model_side_dist.zxy));
	model_side_dist += vec3(mask) * model_ray_delta;
	model_ray_voxel += ivec3(mask) * model_ray_step;
	++steps;
    }
    
    discard;
}
