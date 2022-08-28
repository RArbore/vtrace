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

#include <stdio.h>

#include "common.h"

result create_ray_tracing_objects(void) {
    VkAabbPositionsKHR aabb = {0};
    aabb.minX = -0.5f;
    aabb.minY = -0.5f;
    aabb.minZ = -0.5f;
    aabb.maxX = 0.5f;
    aabb.maxY = 0.5f;
    aabb.maxZ = 0.5f;
    
    VkAccelerationStructureGeometryDataKHR cube_geometry_data = {0};
    cube_geometry_data.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
    cube_geometry_data.aabbs.data.hostAddress = &aabb;
    cube_geometry_data.aabbs.stride = sizeof(VkAabbPositionsKHR);
    
    VkAccelerationStructureGeometryKHR cube_geometry = {0};
    cube_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    cube_geometry.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
    cube_geometry.geometry = cube_geometry_data;

    VkAccelerationStructureBuildGeometryInfoKHR geometry_info = {0};
    geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    geometry_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    geometry_info.geometryCount = 1;
    geometry_info.pGeometries = &cube_geometry;

    uint32_t aabb_num_primitives = 1;
    VkAccelerationStructureBuildSizesInfoKHR build_size;
    vkGetAccelerationStructureBuildSizes(glbl.device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_HOST_KHR, &geometry_info, &aabb_num_primitives, &build_size);
    printf("%lu %lu %lu\n", build_size.accelerationStructureSize, build_size.updateScratchSize, build_size.buildScratchSize);

    /*VkAccelerationStructureCreateInfoKHR create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    
    VkAccelerationStructureKHR tlas;
    PROPAGATE_VK(vkCreateAccelerationStructureKHR(glbl.device, &create_info, NULL, &tlas));*/

    return SUCCESS;
}
