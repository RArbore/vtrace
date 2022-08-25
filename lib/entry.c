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

renderer glbl = {0};

static void glfw_framebuffer_resize_callback(__attribute__((unused)) GLFWwindow* window, __attribute__((unused)) int width, __attribute__((unused)) int height) {
    glbl.resized = 1;
}

static void glfw_key_callback(__attribute__((unused)) GLFWwindow* window, int key, __attribute__((unused)) int scancode, int action, __attribute__((unused)) int mods) {
    switch (key) {
    case GLFW_KEY_W:
	glbl.user_input.keys[0] = action == GLFW_RELEASE ? 0 : 1;
	break;
    case GLFW_KEY_A:
	glbl.user_input.keys[1] = action == GLFW_RELEASE ? 0 : 1;
	break;
    case GLFW_KEY_S:
	glbl.user_input.keys[2] = action == GLFW_RELEASE ? 0 : 1;
	break;
    case GLFW_KEY_D:
	glbl.user_input.keys[3] = action == GLFW_RELEASE ? 0 : 1;
	break;
    case GLFW_KEY_SPACE:
	glbl.user_input.keys[4] = action == GLFW_RELEASE ? 0 : 1;
	break;
    case GLFW_KEY_LEFT_SHIFT:
	glbl.user_input.keys[5] = action == GLFW_RELEASE ? 0 : 1;
	break;
    }
}

uint64_t entry(void) {
    result res = init();
    return ((uint64_t) res.vk << 32) | (uint64_t) res.custom;
}

result init(void) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    glbl.window_width = 1000;
    glbl.window_height = 1000;
    glbl.window = glfwCreateWindow(glbl.window_width, glbl.window_height, "vtrace", NULL, NULL);
    glfwSetFramebufferSizeCallback(glbl.window, glfw_framebuffer_resize_callback);
    glfwSetKeyCallback(glbl.window, glfw_key_callback);
    glfwSetInputMode(glbl.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    PROPAGATE(create_instance());
    PROPAGATE(create_surface());
    PROPAGATE(create_physical());
    PROPAGATE(create_device());
    PROPAGATE(create_swapchain());
    PROPAGATE(create_descriptor_pool());
    PROPAGATE(create_descriptor_layouts());
    PROPAGATE(create_descriptor_sets());
    PROPAGATE(create_raster_pipeline());
    PROPAGATE(create_depth_resources());
    PROPAGATE(create_framebuffers());
    PROPAGATE(create_ray_tracing_objects());
    PROPAGATE(create_command_pool());
    PROPAGATE(create_command_buffers());
    PROPAGATE(create_cube_buffer());
    PROPAGATE(create_instance_buffer());
    PROPAGATE(create_staging_texture_buffer());
    PROPAGATE(create_texture_singletons());
    PROPAGATE(create_synchronization());

    return SUCCESS;
}

void cleanup(void) {
    vkDeviceWaitIdle(glbl.device);

    cleanup_swapchain();
    cleanup_instance_buffer();
    cleanup_staging_texture_buffer();
    cleanup_texture_images();

    for (uint32_t i = 0; i < dynarray_len(&glbl.texture_memories); ++i) {
	vkFreeMemory(glbl.device, INDEX(i, glbl.texture_memories, VkDeviceMemory), NULL);
    }

    vkDestroySampler(glbl.device, glbl.texture_sampler, NULL);

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
	vkDestroySemaphore(glbl.device, glbl.image_available_semaphore[i], NULL);
	vkDestroySemaphore(glbl.device, glbl.render_finished_semaphore[i], NULL);
	vkDestroyFence(glbl.device, glbl.frame_in_flight_fence[i], NULL);
	vkDestroySemaphore(glbl.device, glbl.secondary_finished_semaphore[i], NULL);
    }
    vkDestroyFence(glbl.device, glbl.texture_upload_finished_fence, NULL);

    vkDestroyBuffer(glbl.device, glbl.cube_vertex_buffer, NULL);
    vkDestroyBuffer(glbl.device, glbl.cube_index_buffer, NULL);
    vkFreeMemory(glbl.device, glbl.cube_memory, NULL);
    vkDestroyBuffer(glbl.device, glbl.staging_cube_vertex_buffer, NULL);
    vkDestroyBuffer(glbl.device, glbl.staging_cube_index_buffer, NULL);
    vkFreeMemory(glbl.device, glbl.staging_cube_memory, NULL);
 
    vkDestroyCommandPool(glbl.device, glbl.command_pool, NULL);
     
    vkDestroyPipeline(glbl.device, glbl.raster_pipeline, NULL);
    vkDestroyPipelineLayout(glbl.device, glbl.raster_pipeline_layout, NULL);
    vkDestroyDescriptorPool(glbl.device, glbl.descriptor_pool, NULL);
    vkDestroyDescriptorSetLayout(glbl.device, glbl.raster_descriptor_set_layout, NULL);

    vkDestroyRenderPass(glbl.device, glbl.render_pass, NULL);

    vkDestroyDevice(glbl.device, NULL);
    vkDestroySurfaceKHR(glbl.instance, glbl.surface, NULL);
    vkDestroyInstance(glbl.instance, NULL);
    
    glfwDestroyWindow(glbl.window);
    glfwTerminate();
}

user_input* get_input_data_pointer(void) {
    return &glbl.user_input;
}

int32_t render_tick(int32_t* window_width, int32_t* window_height, const render_tick_info* render_tick_info) {
    if (glfwWindowShouldClose(glbl.window)) {
	return -1;
    }

    glfwPollEvents();

    glbl.user_input.last_mouse_x = glbl.user_input.mouse_x;
    glbl.user_input.last_mouse_y = glbl.user_input.mouse_y;
    glfwGetCursorPos(glbl.window, &glbl.user_input.mouse_x, &glbl.user_input.mouse_y);
    if (glbl.num_frames_elapsed == 0) {
	glbl.user_input.last_mouse_x = glbl.user_input.mouse_x;
	glbl.user_input.last_mouse_y = glbl.user_input.mouse_y;
    }
    
    vkWaitForFences(glbl.device, 1, &glbl.frame_in_flight_fence[glbl.current_frame], VK_TRUE, UINT64_MAX);

    uint32_t image_index;
    VkResult acquire_image_result = vkAcquireNextImageKHR(glbl.device, glbl.swapchain, UINT64_MAX, glbl.image_available_semaphore[glbl.current_frame], VK_NULL_HANDLE, &image_index);
    if (acquire_image_result == VK_ERROR_OUT_OF_DATE_KHR) {
	PROPAGATE_C(recreate_swapchain());
	return 0;
    } else if (acquire_image_result != VK_SUCCESS && acquire_image_result != VK_SUBOPTIMAL_KHR) {
	return -1;
    }

    if (dynarray_len(&glbl.raster_pending_descriptor_writes[glbl.current_frame]) > 0) {
	for (uint32_t i = 0; i < dynarray_len(&glbl.raster_pending_descriptor_writes[glbl.current_frame]); ++i) {
	    INDEX(i, glbl.raster_pending_descriptor_writes[glbl.current_frame], VkWriteDescriptorSet).pImageInfo = &INDEX(i, glbl.raster_pending_descriptor_write_infos[glbl.current_frame], descriptor_info).image_info;
	}
	vkUpdateDescriptorSets(glbl.device, dynarray_len(&glbl.raster_pending_descriptor_writes[glbl.current_frame]), glbl.raster_pending_descriptor_writes[glbl.current_frame].data, 0, NULL);
	dynarray_clear(&glbl.raster_pending_descriptor_writes[glbl.current_frame]);
	dynarray_clear(&glbl.raster_pending_descriptor_write_infos[glbl.current_frame]);
    }

    vkResetFences(glbl.device, 1, &glbl.frame_in_flight_fence[glbl.current_frame]);
    vkResetCommandBuffer(glbl.raster_command_buffers[glbl.current_frame], 0);
    vkResetCommandBuffer(glbl.secondary_command_buffers[glbl.current_frame], 0);
    PROPAGATE_C(record_raster_command_buffer(glbl.raster_command_buffers[glbl.current_frame], image_index, render_tick_info));

    VkPipelineStageFlags wait_stages[2] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
    VkSemaphore wait_semaphores[2] = {glbl.image_available_semaphore[glbl.current_frame], glbl.secondary_finished_semaphore[glbl.current_frame]};

    uint32_t did_secondary = 0;
    if (glbl.secondary_queue_size[glbl.secondary_queue_index] > 0) {
        vkResetCommandBuffer(glbl.secondary_command_buffers[glbl.current_frame], 0);
	PROPAGATE_C(record_secondary_command_buffer(glbl.secondary_command_buffers[glbl.current_frame], glbl.secondary_queue_size[glbl.secondary_queue_index], glbl.secondary_queue[glbl.secondary_queue_index]));

	VkSubmitInfo submit_info = {0};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &glbl.secondary_command_buffers[glbl.current_frame];
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &glbl.secondary_finished_semaphore[glbl.current_frame];

	PROPAGATE_VK_C(vkQueueSubmit(glbl.queue, 1, &submit_info, glbl.secondary_finished_fence));

	glbl.secondary_queue_size[glbl.secondary_queue_index] = 0;
	glbl.secondary_finished_fence = VK_NULL_HANDLE;
	glbl.secondary_queue_index = (glbl.secondary_queue_index + 1) % COMMAND_QUEUE_DEPTH;
	did_secondary = 1;
    }

    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = did_secondary ? 2 : 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &glbl.render_finished_semaphore[glbl.current_frame];
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &glbl.raster_command_buffers[glbl.current_frame];

    PROPAGATE_VK_C(vkQueueSubmit(glbl.queue, 1, &submit_info, glbl.frame_in_flight_fence[glbl.current_frame]));

    VkPresentInfoKHR present_info = {0};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &glbl.render_finished_semaphore[glbl.current_frame];
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &glbl.swapchain;
    present_info.pImageIndices = &image_index;

    VkResult queue_present_result = vkQueuePresentKHR(glbl.queue, &present_info);

    if (queue_present_result == VK_ERROR_OUT_OF_DATE_KHR || queue_present_result == VK_SUBOPTIMAL_KHR || glbl.resized) {
	glbl.resized = 0;
	recreate_swapchain();
    } else if (queue_present_result != VK_SUCCESS) {
	return -1;
    }

    glbl.current_frame = (glbl.current_frame + 1) % FRAMES_IN_FLIGHT;
    ++glbl.num_frames_elapsed;
    *window_width = glbl.window_width;
    *window_height = glbl.window_height;
    
    return 0;
}
