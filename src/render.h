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

typedef struct renderer {
    uint32_t window_width;
    uint32_t window_height;
    GLFWwindow* window;

    VkInstance instance;
    VkPhysicalDevice physical;
} renderer;

typedef struct result {
    VkResult vk;
    int32_t custom;
} result;

uint64_t entry(void);

result init(void);

result create_instance(void);

result create_physical(void);

int32_t physical_score(VkPhysicalDevice physical);

result physical_check_queue_family(VkPhysicalDevice physical, uint32_t* queue_family, VkQueueFlagBits bits);

void cleanup(void);

int32_t render_tick(void);
