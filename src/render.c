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

static const uint32_t MAX_VK_ENUMERATIONS = 8;

#define IS_SUCCESS(res) (res.vk == SUCCESS.vk && res.custom == SUCCESS.custom)

#define PROPAGATE(res)							\
    {									\
	result eval = res;						\
	if (!IS_SUCCESS(eval)) return eval;				\
    }

#define PROPAGATE_VK(res)						\
    {									\
	VkResult eval = res;						\
	if (eval != SUCCESS.vk) {					\
	    result ret = {.vk = eval, .custom = 0};			\
	    return ret;							\
	}								\
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

    PROPAGATE(create_instance());
    PROPAGATE(create_physical());
    PROPAGATE(create_device());

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
	fprintf(stderr, "ERROR: No physical devices available\n");
	return CUSTOM_ERROR;
    }

    VkPhysicalDevice possible[MAX_VK_ENUMERATIONS];
    device_count = device_count < MAX_VK_ENUMERATIONS ? device_count : MAX_VK_ENUMERATIONS;
    VkResult queried_all = vkEnumeratePhysicalDevices(glbl.instance, &device_count, possible);
    if (queried_all != VK_SUCCESS) {
	fprintf(stderr, "WARNING: There are more than %u possible physical devices, just considering the first %u\n", MAX_VK_ENUMERATIONS, MAX_VK_ENUMERATIONS);
    }
    device_count = device_count < MAX_VK_ENUMERATIONS ? device_count : MAX_VK_ENUMERATIONS;

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
	fprintf(stderr, "ERROR: No physical device is suitable\n");
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

    result queue_check = physical_check_queue_family(physical, NULL, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
    if (!IS_SUCCESS(queue_check)) {
	return -1;
    }

    return device_type_score;
}

result physical_check_queue_family(VkPhysicalDevice physical, uint32_t* queue_family, VkQueueFlagBits bits) {
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &queue_family_count, NULL);
    if (queue_family_count == 0) {
	fprintf(stderr, "ERROR: No queue families available\n");
	return CUSTOM_ERROR;
    }

    VkQueueFamilyProperties possible[MAX_VK_ENUMERATIONS];
    queue_family_count = queue_family_count < MAX_VK_ENUMERATIONS ? queue_family_count : MAX_VK_ENUMERATIONS;
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &queue_family_count, possible);
    queue_family_count = queue_family_count < MAX_VK_ENUMERATIONS ? queue_family_count : MAX_VK_ENUMERATIONS;

    for (uint32_t queue_family_index = 0; queue_family_index < queue_family_count; ++queue_family_index) {
	if ((possible[queue_family_index].queueFlags & bits) == bits) {
	    if (queue_family) *queue_family = queue_family_index;
	    return SUCCESS;
	}
    }

    return CUSTOM_ERROR;
}

result create_device(void) {
    uint32_t queue_family;
    PROPAGATE(physical_check_queue_family(glbl.physical, &queue_family, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT));

    float queue_priority = 1.0f;

    VkDeviceQueueCreateInfo queue_create_info = {0};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = queue_family;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;

    VkPhysicalDeviceFeatures device_features = {0};

    VkDeviceCreateInfo device_create_info = {0};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pQueueCreateInfos = &queue_create_info;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pEnabledFeatures = &device_features;

    PROPAGATE_VK(vkCreateDevice(glbl.physical, &device_create_info, NULL, &glbl.device));
    vkGetDeviceQueue(glbl.device, queue_family, 0, &glbl.queue);
    
    return SUCCESS;
}

void cleanup(void) {
    vkDestroyDevice(glbl.device, NULL);
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
