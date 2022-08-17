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

result create_descriptor_pool(void) {
    VkDescriptorPoolSize pool_size = {0};
    pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_size.descriptorCount = FRAMES_IN_FLIGHT * MAX_TEXTURES;

    VkDescriptorPoolCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    create_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
    create_info.poolSizeCount = 1;
    create_info.pPoolSizes = &pool_size;
    create_info.maxSets = FRAMES_IN_FLIGHT * MAX_TEXTURES;

    PROPAGATE_VK(vkCreateDescriptorPool(glbl.device, &create_info, NULL, &glbl.descriptor_pool));

    return SUCCESS;
}

result create_descriptor_layouts(void) {
    VkDescriptorSetLayoutBinding sampler_layout_binding = {0};
    sampler_layout_binding.binding = 0;
    sampler_layout_binding.descriptorCount = MAX_TEXTURES;
    sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler_layout_binding.pImmutableSamplers = NULL;
    sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding bindings[] = {sampler_layout_binding};

    VkDescriptorBindingFlags bindless_flags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;

    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT layout_binding_flags_create_info = {0};
    layout_binding_flags_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
    layout_binding_flags_create_info.bindingCount = 1;
    layout_binding_flags_create_info.pBindingFlags = &bindless_flags;
    
    VkDescriptorSetLayoutCreateInfo layout_create_info = {0};
    layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_create_info.pNext = &layout_binding_flags_create_info;
    layout_create_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
    layout_create_info.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
    layout_create_info.pBindings = bindings;

    PROPAGATE_VK(vkCreateDescriptorSetLayout(glbl.device, &layout_create_info, NULL, &glbl.graphics_descriptor_set_layout));
    
    return SUCCESS;
}

result create_descriptor_sets(void) {
    VkDescriptorSetLayout layouts[FRAMES_IN_FLIGHT];
    uint32_t max_variable_count[FRAMES_IN_FLIGHT];
    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
	layouts[i] = glbl.graphics_descriptor_set_layout;
	max_variable_count[i] = MAX_TEXTURES;
    }
    
    VkDescriptorSetVariableDescriptorCountAllocateInfoEXT variable_count_allocate_info = {0};
    variable_count_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
    variable_count_allocate_info.descriptorSetCount = FRAMES_IN_FLIGHT;
    variable_count_allocate_info.pDescriptorCounts = max_variable_count;
    
    VkDescriptorSetAllocateInfo allocate_info = {0};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.pNext = &variable_count_allocate_info;
    allocate_info.descriptorPool = glbl.descriptor_pool;
    allocate_info.descriptorSetCount = FRAMES_IN_FLIGHT;
    allocate_info.pSetLayouts = layouts;

    PROPAGATE_VK(vkAllocateDescriptorSets(glbl.device, &allocate_info, glbl.graphics_descriptor_sets));

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
	PROPAGATE(dynarray_create(sizeof(descriptor_info), 8, &glbl.graphics_pending_descriptor_write_infos[i]));
	PROPAGATE(dynarray_create(sizeof(VkWriteDescriptorSet), 8, &glbl.graphics_pending_descriptor_writes[i]));
    }

    return SUCCESS;
}

result create_texture_sampler(void) {
    VkSamplerCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    create_info.magFilter = VK_FILTER_NEAREST;
    create_info.minFilter = VK_FILTER_NEAREST;

    create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    create_info.anisotropyEnable = VK_FALSE;
    create_info.maxAnisotropy = 1.0;
    create_info.compareEnable = VK_FALSE;
    create_info.compareOp = VK_COMPARE_OP_ALWAYS;

    create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    create_info.mipLodBias = 0.0f;
    create_info.minLod = 0.0f;
    create_info.maxLod = 0.0f;

    PROPAGATE_VK(vkCreateSampler(glbl.device, &create_info, NULL, &glbl.texture_sampler));
    
    return SUCCESS;
}

result update_descriptors(uint32_t update_texture) {
    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
	dynarray_push(NULL, &glbl.graphics_pending_descriptor_write_infos[i]);
	dynarray_push(NULL, &glbl.graphics_pending_descriptor_writes[i]);

	descriptor_info* write_info = dynarray_last(&glbl.graphics_pending_descriptor_write_infos[i]);
	VkWriteDescriptorSet* write = dynarray_last(&glbl.graphics_pending_descriptor_writes[i]);
	
	write_info->image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	write_info->image_info.imageView = INDEX(update_texture, glbl.texture_image_views, VkImageView);
	write_info->image_info.sampler = glbl.texture_sampler;

	write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write->pNext = NULL;
	write->dstSet = glbl.graphics_descriptor_sets[i];
	write->dstBinding = 0;
	write->dstArrayElement = update_texture;
	write->descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write->descriptorCount = 1;
	write->pImageInfo = &write_info->image_info;
	write->pBufferInfo = NULL;
	write->pTexelBufferView = NULL;
    }

    return SUCCESS;
}
