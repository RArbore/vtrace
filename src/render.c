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

#include "render.h"

static renderer glbl;

static const char* validation_layers[] = {
    "VK_LAYER_KHRONOS_validation"
};

static const result SUCCESS = {.vk = VK_SUCCESS, .custom = 0};
static const result CUSTOM_ERROR = {.vk = VK_SUCCESS, .custom = 1};

#define PROPAGATE(res)							\
    {									\
	result eval = res;						\
	if (eval.vk != SUCCESS.vk || eval.custom != SUCCESS.custom) return eval; \
    }

#define PROPAGATE_VK(res)						\
    {									\
	VkResult eval = res;						\
	if (eval != SUCCESS.vk) {					\
	    result ret = {.vk = eval, .custom = 0};			\
	    return ret;							\
	}								\
    }

result init(void) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    glbl.window_width = 1000;
    glbl.window_height = 1000;
    glbl.window = glfwCreateWindow(glbl.window_width, glbl.window_height, "vtrace", NULL, NULL);

    PROPAGATE(create_instance());
    PROPAGATE(create_physical());

    return SUCCESS;
}

result create_instance(void) {
    VkApplicationInfo app_info = {0};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "vtrace";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "Custom";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions;
    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    create_info.enabledExtensionCount = glfw_extension_count;
    create_info.ppEnabledExtensionNames = glfw_extensions;
    
#ifndef RELEASE
    create_info.enabledLayerCount = sizeof(validation_layers) / sizeof(const char*);
    create_info.ppEnabledLayerNames = validation_layers;
#else
    create_info.enabledLayerCount = 0;
#endif
    
    PROPAGATE_VK(vkCreateInstance(&create_info, NULL, &glbl.instance));

    return SUCCESS;
}

result create_physical(void) {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(glbl.instance, &device_count, NULL);
    if (device_count == 0) {
	fprintf(stderr, "ERROR: No physical devices available");
	return CUSTOM_ERROR;
    }

    const uint32_t MAX_PHYSICAL_QUERIED = 8;
    VkPhysicalDevice possible[MAX_PHYSICAL_QUERIED];
    device_count = device_count < MAX_PHYSICAL_QUERIED ? device_count : MAX_PHYSICAL_QUERIED;
    VkResult queried_all = vkEnumeratePhysicalDevices(glbl.instance, &device_count, possible);
    if (queried_all != VK_SUCCESS) {
	fprintf(stderr, "WARNING: There are more than %u possible physical devices, just considering the first %u", MAX_PHYSICAL_QUERIED, MAX_PHYSICAL_QUERIED);
    }
    device_count = device_count < MAX_PHYSICAL_QUERIED ? device_count : MAX_PHYSICAL_QUERIED;

    int32_t best_physical = -1;
    int32_t best_score = 0;
    for (uint32_t new_physical = 0; new_physical < device_count; ++new_physical) {
	int32_t new_score = physical_score(possible[new_physical]);
	if (new_score > best_score) {
	    best_score = new_score;
	    best_physical = new_physical;
	}
    }

    if (best_physical == -1) {
	fprintf(stderr, "ERROR: No physical device is suitable");
	return CUSTOM_ERROR;
    }
    glbl.physical = possible[best_physical];

    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(glbl.physical, &device_properties);
    printf("INFO: Using device \"%s\"\n", device_properties.deviceName);

    return SUCCESS;
}

int32_t physical_score(VkPhysicalDevice physical) {
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(physical, &device_properties);

    int32_t device_type_score = 0;
    switch (device_properties.deviceType) {
    case VK_PHYSICAL_DEVICE_TYPE_OTHER:
	device_type_score = 0;
	break;
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
	device_type_score = 1;
	break;
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
	device_type_score = 2;
	break;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
	device_type_score = 3;
	break;
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
	device_type_score = 4;
	break;
    default:
	device_type_score = 0;
    }

    return device_type_score;
}

void cleanup(void) {
    vkDestroyInstance(glbl.instance, NULL);
    
    glfwDestroyWindow(glbl.window);
    glfwTerminate();
}

int32_t render_tick(void) {
    if (glfwWindowShouldClose(glbl.window)) {
	return -1;
    }

    glfwPollEvents();

    return 0;
}
