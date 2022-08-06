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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "common.h"

static const gpu_vertex cube_vertices[] = {
    {-0.5f, -0.5f,  0.5f},
    {0.5f, -0.5f,  0.5f},
    {-0.5f,  0.5f,  0.5f},
    {0.5f,  0.5f,  0.5f},
    {-0.5f, -0.5f, -0.5f},
    {0.5f, -0.5f, -0.5f},
    {-0.5f,  0.5f, -0.5f},
    {0.5f,  0.5f, -0.5f},
};

static const uint16_t cube_indices[] = {
    0, 1, 2, 2, 1, 3,
    4, 6, 5, 6, 7, 5,
    2, 3, 6, 6, 3, 7,
    0, 4, 1, 1, 4, 5,
    0, 2, 4, 4, 2, 6,
    1, 5, 3, 3, 5, 7
};

static uint32_t round_up(uint32_t num_to_round, uint32_t multiple) {
    if (multiple == 0)
        return num_to_round;

    uint32_t remainder = num_to_round % multiple;
    if (remainder == 0)
        return num_to_round;

    return num_to_round + multiple - remainder;
}

result create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer* buffer) {
    VkBufferCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size = size;
    create_info.usage = usage;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    PROPAGATE_VK(vkCreateBuffer(glbl.device, &create_info, NULL, buffer));

    return SUCCESS;
}

result create_buffer_memory(VkMemoryPropertyFlags properties, VkDeviceMemory* memory, VkBuffer* buffers, uint32_t num_buffers, uint32_t* offsets, uint32_t minimum_size) {
    uint32_t allocate_size = 0;
    uint32_t memory_type_bits = 0;
    uint32_t local_offsets[num_buffers];
    if (!offsets) {
	offsets = &local_offsets[0];
    }
    for (uint32_t i = 0; i < num_buffers; ++i) {
	VkMemoryRequirements requirements;
	vkGetBufferMemoryRequirements(glbl.device, buffers[i], &requirements);

	allocate_size = round_up(allocate_size, requirements.alignment);
	offsets[i] = allocate_size;
	allocate_size += round_up(requirements.size, requirements.alignment);

	memory_type_bits |= requirements.memoryTypeBits;
    }

    if (allocate_size < minimum_size) allocate_size = minimum_size;

    VkMemoryAllocateInfo allocate_info = {0};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = allocate_size;
    PROPAGATE(find_memory_type(memory_type_bits, properties, &allocate_info.memoryTypeIndex));
    PROPAGATE_VK(vkAllocateMemory(glbl.device, &allocate_info, NULL, memory));

    for (uint32_t i = 0; i < num_buffers; ++i) {
	PROPAGATE_VK(vkBindBufferMemory(glbl.device, buffers[i], *memory, offsets[i]));
    }

    return SUCCESS;
}

result create_image(VkImageCreateFlags flags, VkFormat format, VkExtent3D extent, uint32_t mipLevels, uint32_t arrayLevels, VkImageUsageFlagBits usage, VkImage* image) {
    VkImageCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    create_info.flags = flags;
    create_info.imageType = extent.depth > 1 ? VK_IMAGE_TYPE_3D : extent.height > 1 ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_1D;
    create_info.format = format;
    create_info.extent = extent;
    create_info.mipLevels = mipLevels;
    create_info.arrayLayers = arrayLevels;
    create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    create_info.usage = usage;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    PROPAGATE_VK(vkCreateImage(glbl.device, &create_info, NULL, image));

    return SUCCESS;
}

result create_image_view(VkImage image, VkImageViewType type, VkFormat format, VkImageSubresourceRange subresource_range, VkImageView* view) {
    VkImageViewCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = image;
    create_info.viewType = type;
    create_info.format = format;
    create_info.subresourceRange = subresource_range;

    PROPAGATE_VK(vkCreateImageView(glbl.device, &create_info, NULL, view));
    
    return SUCCESS;
}

result create_image_memory(VkMemoryPropertyFlags properties, VkDeviceMemory* memory, VkImage* images, uint32_t num_images, uint32_t* offsets, uint32_t minimum_size) {
    uint32_t allocate_size = 0;
    uint32_t memory_type_bits = 0;
    uint32_t local_offsets[num_images];
    if (!offsets) {
	offsets = &local_offsets[0];
    }
    for (uint32_t i = 0; i < num_images; ++i) {
	VkMemoryRequirements requirements;
	vkGetImageMemoryRequirements(glbl.device, images[i], &requirements);

	allocate_size = round_up(allocate_size, requirements.alignment);
	offsets[i] = allocate_size;
	allocate_size += round_up(requirements.size, requirements.alignment);

	memory_type_bits |= requirements.memoryTypeBits;
    }

    if (allocate_size < minimum_size) allocate_size = minimum_size;

    VkMemoryAllocateInfo allocate_info = {0};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = allocate_size;
    PROPAGATE(find_memory_type(memory_type_bits, properties, &allocate_info.memoryTypeIndex));
    PROPAGATE_VK(vkAllocateMemory(glbl.device, &allocate_info, NULL, memory));

    for (uint32_t i = 0; i < num_images; ++i) {
	PROPAGATE_VK(vkBindImageMemory(glbl.device, images[i], *memory, offsets[i]));
    }

    return SUCCESS;
}

result create_cube_buffer(void) {
    uint32_t vertex_buffer_size = sizeof(cube_vertices);
    uint32_t index_buffer_size = sizeof(cube_indices);

    VkBuffer cube_buffers[2];
    PROPAGATE(create_buffer(vertex_buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &cube_buffers[0]));
    PROPAGATE(create_buffer(index_buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &cube_buffers[1]));
    PROPAGATE(create_buffer_memory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &glbl.cube_memory, cube_buffers, sizeof(cube_buffers) / sizeof(cube_buffers[0]), NULL, 0));
    glbl.cube_vertex_buffer = cube_buffers[0];
    glbl.cube_index_buffer = cube_buffers[1];

    uint32_t buffer_offsets[2];
    PROPAGATE(create_buffer(vertex_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &cube_buffers[0]));
    PROPAGATE(create_buffer(index_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &cube_buffers[1]));
    PROPAGATE(create_buffer_memory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &glbl.staging_cube_memory, cube_buffers, sizeof(cube_buffers) / sizeof(cube_buffers[0]), buffer_offsets, 0));
    glbl.staging_cube_vertex_buffer = cube_buffers[0];
    glbl.staging_cube_index_buffer = cube_buffers[1];

    void* vertex_data;
    void* index_data;
    vkMapMemory(glbl.device, glbl.staging_cube_memory, buffer_offsets[0], vertex_buffer_size, 0, &vertex_data);
    memcpy(vertex_data, &cube_vertices[0].pos[0], (size_t) vertex_buffer_size);
    vkUnmapMemory(glbl.device, glbl.staging_cube_memory);
    vkMapMemory(glbl.device, glbl.staging_cube_memory, buffer_offsets[1], index_buffer_size, 0, &index_data);
    memcpy(index_data, &cube_indices[0], (size_t) index_buffer_size);
    vkUnmapMemory(glbl.device, glbl.staging_cube_memory);

    secondary_command copy_command = {0};
    copy_command.type = SECONDARY_TYPE_COPY_BUFFER_BUFFER;
    copy_command.delay = 0;
    copy_command.copy_buffer_buffer.src_buffer = glbl.staging_cube_vertex_buffer;
    copy_command.copy_buffer_buffer.dst_buffer = glbl.cube_vertex_buffer;
    copy_command.copy_buffer_buffer.copy_region.size = vertex_buffer_size;

    PROPAGATE(queue_secondary_command(copy_command));

    copy_command.copy_buffer_buffer.src_buffer = glbl.staging_cube_index_buffer;
    copy_command.copy_buffer_buffer.dst_buffer = glbl.cube_index_buffer;
    copy_command.copy_buffer_buffer.copy_region.size = index_buffer_size;

    PROPAGATE(queue_secondary_command(copy_command));
    
    return SUCCESS;
}

static uint32_t round_up_p2(uint32_t x) {
    uint32_t rounded = 1;
    while (rounded < x) rounded *= 2;
    return rounded;
}

result create_instance_buffer(void) {
    uint32_t instance_capacity = round_up_p2(glbl.instance_count + 1);
    
    PROPAGATE(create_buffer(instance_capacity * sizeof(float) * 4 * 4, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &glbl.instance_buffer));
    PROPAGATE(create_buffer_memory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &glbl.instance_memory, &glbl.instance_buffer, 1, NULL, 0));

    PROPAGATE(create_buffer(instance_capacity * sizeof(float) * 4 * 4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &glbl.staging_instance_buffer));
    PROPAGATE(create_buffer_memory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &glbl.staging_instance_memory, &glbl.staging_instance_buffer, 1, NULL, 0));

    glbl.instance_capacity = instance_capacity;

    return SUCCESS;
}

int32_t update_instances(const float* instances, uint32_t instance_count) {
    glbl.instance_count = instance_count;
    if (instance_count > glbl.instance_capacity) {
	cleanup_instance_buffer();
	PROPAGATE_C(create_instance_buffer());
    }

    void* instance_data;
    vkMapMemory(glbl.device, glbl.staging_instance_memory, 0, instance_count * sizeof(float) * 4 * 4, 0, &instance_data);
    memcpy(instance_data, instances, instance_count * sizeof(float) * 4 * 4);
    vkUnmapMemory(glbl.device, glbl.staging_instance_memory);

    secondary_command copy_command = {0};
    copy_command.type = SECONDARY_TYPE_COPY_BUFFER_BUFFER;
    copy_command.delay = 0;
    copy_command.copy_buffer_buffer.src_buffer = glbl.staging_instance_buffer;
    copy_command.copy_buffer_buffer.dst_buffer = glbl.instance_buffer;
    copy_command.copy_buffer_buffer.copy_region.size = instance_count * sizeof(float) * 4 * 4;

    PROPAGATE_C(queue_secondary_command(copy_command));

    return 0;
}

result create_staging_texture_buffer(void) {
    if (glbl.staging_texture_size == 0) glbl.staging_texture_size = 1;

    PROPAGATE(create_buffer(glbl.staging_texture_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &glbl.staging_texture_buffer));
    PROPAGATE(create_buffer_memory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &glbl.staging_texture_memory, &glbl.staging_texture_buffer, 1, NULL, 0));

    return SUCCESS;
}

result create_texture_resources(void) {
    if (glbl.texture_memory_allocated > 0) PROPAGATE(create_image_memory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &glbl.texture_memory, glbl.texture_images, glbl.texture_image_count, NULL, glbl.texture_memory_allocated));
    
    return SUCCESS;
}

int32_t add_texture(const uint8_t* data, uint32_t width, uint32_t height, uint32_t depth) {
    if (glbl.texture_image_count >= MAX_TEXTURES) {
	fprintf(stderr, "ERROR: Tried allocating too many textures");
	return 1;
    }
    
    vkWaitForFences(glbl.device, 1, &glbl.texture_upload_finished_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(glbl.device, 1, &glbl.texture_upload_finished_fence);
    
    uint32_t upload_size = 4 * width * height * depth;
    if (upload_size > glbl.staging_texture_size) {
	glbl.staging_texture_size = round_up_p2(upload_size);
	cleanup_staging_texture_buffer();
	PROPAGATE_C(create_staging_texture_buffer());
    }

    void* texture_data;
    vkMapMemory(glbl.device, glbl.staging_texture_memory, 0, upload_size, 0, &texture_data);
    memcpy(texture_data, data, upload_size);
    vkUnmapMemory(glbl.device, glbl.staging_texture_memory);

    VkImage image;
    VkImageView image_view;

    VkExtent3D extent;
    extent.width = width;
    extent.height = height;
    extent.depth = depth;
    
    PROPAGATE_C(create_image(0, VK_FORMAT_R8G8B8A8_SRGB, extent, 1, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, &image));

    if (glbl.texture_image_count >= glbl.texture_image_allocated) {
	glbl.texture_image_allocated = round_up_p2(glbl.texture_image_allocated + 1);
	glbl.texture_images = realloc(glbl.texture_images, glbl.texture_image_allocated * sizeof(VkImage));
	glbl.texture_image_views = realloc(glbl.texture_image_views, glbl.texture_image_allocated * sizeof(VkImageView));
    }

    glbl.texture_images[glbl.texture_image_count] = image;
    ++glbl.texture_image_count;

    VkMemoryRequirements requirements;
    vkGetImageMemoryRequirements(glbl.device, image, &requirements);
    uint32_t desired_offset = round_up(glbl.texture_memory_used, requirements.alignment);
    uint32_t needed_size = desired_offset + requirements.size;
    //printf("%lu %lu %u %u\n", requirements.size, requirements.alignment, desired_offset, needed_size);
    if (needed_size > glbl.texture_memory_allocated) {
	// TODO: Allocate new memory, queue copy, queue freeing of old memory
	cleanup_texture_resources();
	glbl.texture_memory_allocated = needed_size * 2;
	PROPAGATE_C(create_texture_resources());
    }
    else {
	// TODO: Fix trying to bind with too little memory
	PROPAGATE_VK_C(vkBindImageMemory(glbl.device, image, glbl.texture_memory, desired_offset));
    }
    glbl.texture_memory_used = needed_size;

    VkImageSubresourceRange subresource_range; 
    subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource_range.baseMipLevel = 0;
    subresource_range.levelCount = 1;
    subresource_range.baseArrayLayer = 0;
    subresource_range.layerCount = 1;

    PROPAGATE_C(create_image_view(image, VK_IMAGE_VIEW_TYPE_3D, VK_FORMAT_R8G8B8A8_SRGB, subresource_range, &image_view));
    
    glbl.texture_image_views[glbl.texture_image_count - 1] = image_view;

    secondary_command transition_command;
    transition_command.type = SECONDARY_TYPE_LAYOUT_TRANSITION;
    transition_command.delay = 0;
    transition_command.layout_transition.image = glbl.texture_images[glbl.texture_image_count - 1];
    transition_command.layout_transition.old = VK_IMAGE_LAYOUT_UNDEFINED;
    transition_command.layout_transition.new = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    queue_secondary_command(transition_command);

    VkBufferImageCopy region = {0};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    
    region.imageOffset.x = 0;
    region.imageOffset.y = 0;
    region.imageOffset.z = 0;
    region.imageExtent = extent;

    secondary_command copy_command;
    copy_command.type = SECONDARY_TYPE_COPY_BUFFER_IMAGE;
    copy_command.delay = 1;
    copy_command.copy_buffer_image.src_buffer = glbl.staging_texture_buffer;
    copy_command.copy_buffer_image.dst_image = image;
    copy_command.copy_buffer_image.copy_region = region;
    queue_secondary_command(copy_command);

    transition_command.delay = 2;
    transition_command.layout_transition.old = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transition_command.layout_transition.new = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    queue_secondary_command(transition_command);

    PROPAGATE_C(update_descriptors());
    PROPAGATE_C(set_secondary_fence(glbl.texture_upload_finished_fence));

    return 0;
}

void get_vertex_input_descriptions(VkVertexInputBindingDescription* vertex_input_binding_descriptions, VkVertexInputAttributeDescription* vertex_input_attribute_descriptions) {
    vertex_input_binding_descriptions[0].binding = 0;
    vertex_input_binding_descriptions[0].stride = sizeof(gpu_vertex);
    vertex_input_binding_descriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    vertex_input_binding_descriptions[1].binding = 1;
    vertex_input_binding_descriptions[1].stride = sizeof(float) * 4 * 4;
    vertex_input_binding_descriptions[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

    vertex_input_attribute_descriptions[0].binding = 0;
    vertex_input_attribute_descriptions[0].location = 0;
    vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertex_input_attribute_descriptions[0].offset = 0;

    for (uint32_t i = 0; i < 4; ++i) {
	vertex_input_attribute_descriptions[1 + i].binding = 1;
	vertex_input_attribute_descriptions[1 + i].location = 1 + i;
	vertex_input_attribute_descriptions[1 + i].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	vertex_input_attribute_descriptions[1 + i].offset = sizeof(float) * 4 * i;
    }
}

result find_memory_type(uint32_t filter, VkMemoryPropertyFlags properties, uint32_t* type) {
    VkPhysicalDeviceMemoryProperties physical_mem_properties;
    vkGetPhysicalDeviceMemoryProperties(glbl.physical, &physical_mem_properties);

    for (uint32_t i = 0; i < physical_mem_properties.memoryTypeCount; i++) {
	if ((filter & (1 << i)) && (physical_mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
	    *type = i;
	    return SUCCESS;
	}
    }

    fprintf(stderr, "ERROR: Couldn't find suitable memory type\n");
    return CUSTOM_ERROR;
}

void cleanup_instance_buffer(void) {
    vkQueueWaitIdle(glbl.queue);
    vkDestroyBuffer(glbl.device, glbl.staging_instance_buffer, NULL);
    vkFreeMemory(glbl.device, glbl.staging_instance_memory, NULL);
    vkDestroyBuffer(glbl.device, glbl.instance_buffer, NULL);
    vkFreeMemory(glbl.device, glbl.instance_memory, NULL);
}

void cleanup_staging_texture_buffer(void) {
    vkQueueWaitIdle(glbl.queue);
    vkDestroyBuffer(glbl.device, glbl.staging_texture_buffer, NULL);
    vkFreeMemory(glbl.device, glbl.staging_texture_memory, NULL);
}

void cleanup_texture_images(void) {
    vkQueueWaitIdle(glbl.queue);
    for (uint32_t i = 0; i < glbl.texture_image_count; ++i) {
	vkDestroyImage(glbl.device, glbl.texture_images[i], NULL);
	vkDestroyImageView(glbl.device, glbl.texture_image_views[i], NULL);
    }
    free(glbl.texture_images);
    free(glbl.texture_image_views);
}

void cleanup_texture_resources(void) {
    vkFreeMemory(glbl.device, glbl.texture_memory, NULL);
}
