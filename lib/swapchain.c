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

result create_swapchain(void) {
    swapchain_support support;
    PROPAGATE(physical_check_swapchain_support(glbl.physical, &support));

    VkSurfaceFormatKHR surface_format;
    VkPresentModeKHR present_mode;
    VkExtent2D swap_extent;
    PROPAGATE(choose_swapchain_options(&support, &surface_format, &present_mode, &swap_extent));

    uint32_t image_count = support.capabilities.minImageCount + 1;
    if (support.capabilities.maxImageCount > 0 && image_count > support.capabilities.maxImageCount) {
	image_count = support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = glbl.surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = swap_extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.preTransform = support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    PROPAGATE_VK(vkCreateSwapchainKHR(glbl.device, &create_info, NULL, &glbl.swapchain));

    vkGetSwapchainImagesKHR(glbl.device, glbl.swapchain, &image_count, NULL);
    glbl.swapchain_images = malloc(image_count * sizeof(VkImage));
    vkGetSwapchainImagesKHR(glbl.device, glbl.swapchain, &image_count, glbl.swapchain_images);

    glbl.swapchain_format = surface_format.format;
    glbl.swapchain_extent = swap_extent;

    glbl.swapchain_image_views = malloc(image_count * sizeof(VkImageView));

    VkImageViewCreateInfo image_view_create_info = {0};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = glbl.swapchain_format;
    image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = 1;
    for (uint32_t image_index = 0; image_index < image_count; ++image_index) {
	image_view_create_info.image = glbl.swapchain_images[image_index];
	PROPAGATE_VK(vkCreateImageView(glbl.device, &image_view_create_info, NULL, &glbl.swapchain_image_views[image_index]));
    }

    glbl.swapchain_size = image_count;
    
    return SUCCESS;
}

result choose_swapchain_options(swapchain_support* support, VkSurfaceFormatKHR* surface_format, VkPresentModeKHR* present_mode, VkExtent2D* swap_extent) {
    uint32_t format_index;
    for (format_index = 0; format_index < support->num_formats; ++format_index) {
	if (support->formats[format_index].format == VK_FORMAT_B8G8R8A8_SRGB && support->formats[format_index].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
	    *surface_format = support->formats[format_index];
	    break;
	}
    }
    if (format_index >= support->num_formats) {
	fprintf(stderr, "ERROR: Queried surface format not found to be available\n");
	return CUSTOM_ERROR;
    }

    uint32_t present_mode_index;
    for (present_mode_index = 0; present_mode_index < support->num_present_modes; ++present_mode_index) {
	if (support->present_modes[present_mode_index] == VK_PRESENT_MODE_MAILBOX_KHR) {
	    *present_mode = support->present_modes[present_mode_index];
	    break;
	}
    }
    if (present_mode_index >= support->num_present_modes) {
	fprintf(stderr, "ERROR: Queried present mode not found to be available\n");
	return CUSTOM_ERROR;
    }

    if (support->capabilities.currentExtent.width != UINT32_MAX) {
	*swap_extent = support->capabilities.currentExtent;
    }
    else {
	int32_t width, height;
	glfwGetFramebufferSize(glbl.window, &width, &height);

	swap_extent->width = (uint32_t) width;
	swap_extent->height = (uint32_t) height;

	swap_extent->width = swap_extent->width < support->capabilities.minImageExtent.width ? support->capabilities.minImageExtent.width : swap_extent->width;
	swap_extent->height = swap_extent->height < support->capabilities.minImageExtent.height ? support->capabilities.minImageExtent.height : swap_extent->height;
	swap_extent->width = swap_extent->width > support->capabilities.maxImageExtent.width ? support->capabilities.maxImageExtent.width : swap_extent->width;
	swap_extent->height = swap_extent->height > support->capabilities.maxImageExtent.height ? support->capabilities.maxImageExtent.height : swap_extent->height;
    }

    return SUCCESS;
}

void recreate_swapchain(void) {
    int width = 0, height = 0;
    glfwGetFramebufferSize(glbl.window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(glbl.window, &width, &height);
        glfwWaitEvents();
    }
    
    vkDeviceWaitIdle(glbl.device);

    cleanup_swapchain();

    create_swapchain();
    create_depth_resources();
    create_framebuffers();
}

void cleanup_swapchain(void) {
    for (uint32_t framebuffer_index = 0; framebuffer_index < glbl.swapchain_size; ++framebuffer_index) {
	vkDestroyFramebuffer(glbl.device, glbl.framebuffers[framebuffer_index], NULL);
    }

    for (uint32_t image_index = 0; image_index < glbl.swapchain_size; ++image_index) {
	vkDestroyImageView(glbl.device, glbl.swapchain_image_views[image_index], NULL);
    }

    vkDestroyImageView(glbl.device, glbl.depth_image_view, NULL);
    vkDestroyImage(glbl.device, glbl.depth_image, NULL);
    vkFreeMemory(glbl.device, glbl.depth_image_memory, NULL);

    free(glbl.swapchain_image_views);
    free(glbl.swapchain_images);
    vkDestroySwapchainKHR(glbl.device, glbl.swapchain, NULL);
}
