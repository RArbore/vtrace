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

static const uint32_t NUM_CUBE_INDICES = 36;

result create_command_pool(void) {
    uint32_t queue_family;
    PROPAGATE(physical_check_queue_family(glbl.physical, &queue_family, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT));

    VkCommandPoolCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    create_info.queueFamilyIndex = queue_family;

    PROPAGATE_VK(vkCreateCommandPool(glbl.device, &create_info, NULL, &glbl.command_pool));
    
    return SUCCESS;
}

result create_command_buffers(void) {
    VkCommandBufferAllocateInfo allocate_info = {0};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = glbl.command_pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = FRAMES_IN_FLIGHT;

    PROPAGATE_VK(vkAllocateCommandBuffers(glbl.device, &allocate_info, &glbl.graphics_command_buffers[0]));
    PROPAGATE_VK(vkAllocateCommandBuffers(glbl.device, &allocate_info, &glbl.copy_command_buffers[0]));
    PROPAGATE_VK(vkAllocateCommandBuffers(glbl.device, &allocate_info, &glbl.layout_transition_command_buffers[0]));
    
    return SUCCESS;
}

result record_graphics_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index, const render_tick_info* render_tick_info) {
    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    PROPAGATE_VK(vkBeginCommandBuffer(command_buffer, &begin_info));

    VkClearValue clear_values[0];
    clear_values[0].color.float32[0] = 0.0f;
    clear_values[0].color.float32[1] = 0.0f;
    clear_values[0].color.float32[2] = 0.0f;
    clear_values[0].color.float32[3] = 1.0f;
    clear_values[1].depthStencil.depth = 1.0f;
    clear_values[1].depthStencil.stencil = 0;
    
    VkRenderPassBeginInfo render_pass_begin_info = {0};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass = glbl.render_pass;
    render_pass_begin_info.framebuffer = glbl.framebuffers[image_index];
    render_pass_begin_info.renderArea.offset.x = 0;
    render_pass_begin_info.renderArea.offset.y = 0;
    render_pass_begin_info.renderArea.extent = glbl.swapchain_extent;
    render_pass_begin_info.clearValueCount = 2;
    render_pass_begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, glbl.graphics_pipeline);

    VkViewport viewport = {0};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) glbl.swapchain_extent.width;
    viewport.height = (float) glbl.swapchain_extent.width;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor = {0};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = glbl.swapchain_extent;
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(command_buffer, 0, 1, &glbl.cube_vertex_buffer, offsets);
    vkCmdBindVertexBuffers(command_buffer, 1, 1, &glbl.instance_buffer, offsets);
    vkCmdBindIndexBuffer(command_buffer, glbl.cube_index_buffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdPushConstants(command_buffer, glbl.graphics_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float) * 4 * 4, render_tick_info->perspective);
    vkCmdPushConstants(command_buffer, glbl.graphics_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(float) * 4 * 4, sizeof(float) * 4 * 4, render_tick_info->camera);

    vkCmdDrawIndexed(command_buffer, NUM_CUBE_INDICES, glbl.instance_count, 0, 0, 0);

    vkCmdEndRenderPass(command_buffer);

    PROPAGATE_VK(vkEndCommandBuffer(command_buffer));
    
    return SUCCESS;
}

result record_copy_command_buffer(VkCommandBuffer command_buffer, uint32_t num_copies, copy_command* command) {
    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    PROPAGATE_VK(vkBeginCommandBuffer(command_buffer, &begin_info));

    for (uint32_t i = 0; i < num_copies; ++i) {
	vkCmdCopyBuffer(command_buffer, command[i].src_buffer, command[i].dst_buffer, 1, &command[i].copy_region);
    }
    
    PROPAGATE_VK(vkEndCommandBuffer(command_buffer));
    
    return SUCCESS;
}

result record_layout_transition_command_buffer(VkCommandBuffer command_buffer, uint32_t num_transitions, transition_command* command) {
    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    PROPAGATE_VK(vkBeginCommandBuffer(command_buffer, &begin_info));

    for (uint32_t i = 0; i < num_transitions; ++i) {
	VkImageMemoryBarrier barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = command[i].old;
	barrier.newLayout = command[i].new;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = command[i].image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;
	
	vkCmdPipelineBarrier(command_buffer, 0, 0, 0, 0, NULL, 0, NULL, 1, &barrier);
    }
    
    PROPAGATE_VK(vkEndCommandBuffer(command_buffer));
    
    return SUCCESS;
}

result queue_copy_buffer(VkBuffer dst_buffer, VkBuffer src_buffer, VkBufferCopy copy_region) {
    if (glbl.copy_queue_size >= COMMAND_QUEUE_SIZE) {
	fprintf(stderr, "ERROR: Attempted to queue copy operation, but copy queue is full");
	return CUSTOM_ERROR;
    }
    
    glbl.copy_queue[glbl.copy_queue_size].src_buffer = src_buffer;
    glbl.copy_queue[glbl.copy_queue_size].dst_buffer = dst_buffer;
    glbl.copy_queue[glbl.copy_queue_size].copy_region = copy_region;
    ++glbl.copy_queue_size;

    return SUCCESS;
}

result queue_layout_transition(VkImage image, VkImageLayout old, VkImageLayout new) {
    if (glbl.transition_queue_size >= COMMAND_QUEUE_SIZE) {
	fprintf(stderr, "ERROR: Attempted to queue transition operation, but transition queue is full");
	return CUSTOM_ERROR;
    }
    
    glbl.transition_queue[glbl.transition_queue_size].image = image;
    glbl.transition_queue[glbl.transition_queue_size].old = old;
    glbl.transition_queue[glbl.transition_queue_size].new = new;
    ++glbl.transition_queue_size;

    return SUCCESS;
}
