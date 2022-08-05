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

layout(set = 0, binding = 0) uniform sampler3D tex;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 objectPos;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(tex, objectPos + 0.5);//vec4(fragColor, 1.0);
}
