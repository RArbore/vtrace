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

    PROPAGATE_VK(vkAllocateCommandBuffers(glbl.device, &allocate_info, &glbl.raster_command_buffers[0]));
    PROPAGATE_VK(vkAllocateCommandBuffers(glbl.device, &allocate_info, &glbl.secondary_command_buffers[0]));
    
    return SUCCESS;
}

result record_raster_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index, const render_tick_info* render_tick_info) {
    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    PROPAGATE_VK(vkBeginCommandBuffer(command_buffer, &begin_info));

    VkClearValue clear_values[0];
    clear_values[0].color.float32[0] = 53.0f / 100.0f;
    clear_values[0].color.float32[1] = 81.0f / 100.0f;
    clear_values[0].color.float32[2] = 92.0f / 100.0f;
    clear_values[0].color.float32[3] = 1.0f;
    clear_values[1].depthStencil.depth = 1.0f;
    clear_values[1].depthStencil.stencil = 0;
    
    VkRenderPassBeginInfo render_pass_begin_info = {0};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass = glbl.render_pass;
    render_pass_begin_info.framebuffer = INDEX(image_index, glbl.framebuffers, VkFramebuffer);
    render_pass_begin_info.renderArea.offset.x = 0;
    render_pass_begin_info.renderArea.offset.y = 0;
    render_pass_begin_info.renderArea.extent = glbl.swapchain_extent;
    render_pass_begin_info.clearValueCount = 2;
    render_pass_begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, glbl.raster_pipeline);

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

    vkCmdPushConstants(command_buffer, glbl.raster_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float) * 4 * 4, render_tick_info->perspective);
    vkCmdPushConstants(command_buffer, glbl.raster_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(float) * 4 * 4, sizeof(float) * 4 * 4, render_tick_info->camera);

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, glbl.raster_pipeline_layout, 0, 1, &glbl.raster_descriptor_sets[glbl.current_frame], 0, NULL);

    vkCmdDrawIndexed(command_buffer, NUM_CUBE_INDICES, glbl.instance_count, 0, 0, 0);

    vkCmdEndRenderPass(command_buffer);

    PROPAGATE_VK(vkEndCommandBuffer(command_buffer));
    
    return SUCCESS;
}

result record_secondary_command_buffer(VkCommandBuffer command_buffer, uint32_t num_commands, secondary_command* commands) {
    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    PROPAGATE_VK(vkBeginCommandBuffer(command_buffer, &begin_info));

    uint32_t skips = 1;
    while (skips > 0) {
	skips = 0;
	for (uint32_t i = 0; i < num_commands; ++i) {
	    if (commands[i].ordering > 0) {
		--commands[i].ordering;
		commands[skips] = commands[i];
		++skips;
		continue;
	    }
	    switch (commands[i].type) {
	    case SECONDARY_TYPE_COPY_BUFFER_BUFFER:
		vkCmdCopyBuffer(command_buffer, commands[i].copy_buffer_buffer.src_buffer, commands[i].copy_buffer_buffer.dst_buffer, 1, &commands[i].copy_buffer_buffer.copy_region);
		break;
	    case SECONDARY_TYPE_COPY_BUFFER_IMAGE:
		vkCmdCopyBufferToImage(command_buffer, commands[i].copy_buffer_image.src_buffer, commands[i].copy_buffer_image.dst_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &commands[i].copy_buffer_image.copy_region);
		break;
	    case SECONDARY_TYPE_COPY_IMAGES_IMAGES: {
		VkImageCopy region = {0};
		
		region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.srcSubresource.mipLevel = 0;
		region.srcSubresource.baseArrayLayer = 0;
		region.srcSubresource.layerCount = 1;
		region.srcOffset.x = 0;
		region.srcOffset.y = 0;
		region.srcOffset.z = 0;
		
		region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.dstSubresource.mipLevel = 0;
		region.dstSubresource.baseArrayLayer = 0;
		region.dstSubresource.layerCount = 1;
		region.dstOffset.x = 0;
		region.dstOffset.y = 0;
		region.dstOffset.z = 0;

		for (uint32_t j = 0; j < dynarray_len(&commands[i].copy_images_images.src_images); ++j) {
		    region.extent = INDEX(j, commands[i].copy_images_images.extents, VkExtent3D);
        	    vkCmdCopyImage(command_buffer, INDEX(j, commands[i].copy_images_images.src_images, VkImage), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, INDEX(j, commands[i].copy_images_images.dst_images, VkImage), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		}

		break;
	    }
	    case SECONDARY_TYPE_LAYOUT_TRANSITION: {
		VkImageMemoryBarrier barrier = {0};
		
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = commands[i].layout_transition.old;
		barrier.newLayout = commands[i].layout_transition.new;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		
		for (uint32_t j = 0; j < dynarray_len(&commands[i].layout_transition.images); ++j) {
		    barrier.image = INDEX(j, commands[i].layout_transition.images, VkImage);
		    
		    if (commands[i].layout_transition.old == VK_IMAGE_LAYOUT_UNDEFINED && commands[i].layout_transition.new == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);
		    }
		    else if (commands[i].layout_transition.old == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && commands[i].layout_transition.new == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);
		    }
		    else if (commands[i].layout_transition.old == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && commands[i].layout_transition.new == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);
		    }
		}

		break;
	    }
	    case SECONDARY_TYPE_ACCELERATION_STRUCTURE_BUILD: {
		vkCmdBuildAccelerationStructures(command_buffer, 1, &commands[i].acceleration_structure_build.geometry_info, (const VkAccelerationStructureBuildRangeInfoKHR *const *) &commands[i].acceleration_structure_build.range_info);
		free(commands[i].acceleration_structure_build.range_info);
		free(commands[i].acceleration_structure_build.geometries);
		break;
	    }
	    case SECONDARY_TYPE_CLEANUP: {
		for (uint32_t j = 0; j < dynarray_len(&commands[i].cleanup.images); ++j) {
		    if (commands[i].cleanup.images.size > 0) vkDestroyImage(glbl.device, INDEX(j, commands[i].cleanup.images, VkImage), NULL);
		    if (commands[i].cleanup.image_views.size > 0) vkDestroyImageView(glbl.device, INDEX(j, commands[i].cleanup.image_views, VkImageView), NULL);
		}

		if (commands[i].cleanup.memory) vkFreeMemory(glbl.device, commands[i].cleanup.memory, NULL);

		dynarray_destroy(&commands[i].cleanup.images);
		dynarray_destroy(&commands[i].cleanup.image_views);
		break;
	    }
	    }
	    
	}
	num_commands = skips;
	if (skips > 0) {
	    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 0, NULL);
	}
    }
    
    PROPAGATE_VK(vkEndCommandBuffer(command_buffer));
    
    return SUCCESS;
}

result queue_secondary_command(secondary_command command) {
    uint32_t effective_index = (command.delay + glbl.secondary_queue_index) % COMMAND_QUEUE_DEPTH;
    if (glbl.secondary_queue_size[effective_index] >= COMMAND_QUEUE_SIZE) {
	fprintf(stderr, "ERROR: Attempted to queue copy operation, but copy queue is full");
	return CUSTOM_ERROR;
    }
    
    glbl.secondary_queue[effective_index][glbl.secondary_queue_size[effective_index]] = command;
    ++glbl.secondary_queue_size[effective_index];

    return SUCCESS;
}

result set_secondary_fence(VkFence fence) {
    if (glbl.secondary_finished_fence != VK_NULL_HANDLE) {
	fprintf(stderr, "ERROR: Attempted to set secondary command fence when it's already set");
	return CUSTOM_ERROR;
    }

    glbl.secondary_finished_fence = fence;
    
    return SUCCESS;
}
