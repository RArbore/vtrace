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

#include <stdint.h>

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#define MAX_VK_ENUMERATIONS 16

typedef struct renderer {
    uint32_t window_width;
    uint32_t window_height;
    GLFWwindow* window;

    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physical;
    VkDevice device;
    VkQueue queue;
    VkSwapchainKHR swapchain;
    VkImage* swapchain_images;
    VkFormat swapchain_format;
    VkExtent2D swapchain_extent;
    VkImageView* swapchain_image_views;
    uint32_t swapchain_image_count;
} renderer;

typedef struct swapchain_support {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR formats[MAX_VK_ENUMERATIONS];
    VkPresentModeKHR present_modes[MAX_VK_ENUMERATIONS];
    uint32_t num_formats;
    uint32_t num_present_modes;
} swapchain_support;

typedef struct result {
    VkResult vk;
    int32_t custom;
} result;

uint64_t entry(void);

result init(void);

result create_instance(void);

result create_surface(void);

result create_physical(void);

int32_t physical_score(VkPhysicalDevice physical);

result physical_check_queue_family(VkPhysicalDevice physical, uint32_t* queue_family, VkQueueFlagBits bits);

result physical_check_extensions(VkPhysicalDevice physical);

result physical_check_swapchain_support(VkPhysicalDevice physical, swapchain_support* support);

result create_device(void);

result create_swapchain(void);

result choose_swapchain_options(swapchain_support* support, VkSurfaceFormatKHR* surface_format, VkPresentModeKHR* present_mode, VkExtent2D* swap_extent);

void cleanup(void);

int32_t render_tick(void);
