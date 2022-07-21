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

#include "render.h"

static renderer glbl;

static const char* validation_layers[] = {
    "VK_LAYER_KHRONOS_validation"
};

static const char* device_extensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static const result SUCCESS = {.vk = VK_SUCCESS, .custom = 0};
static const result CUSTOM_ERROR = {.vk = VK_SUCCESS, .custom = 1};

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
    PROPAGATE(create_surface());
    PROPAGATE(create_physical());
    PROPAGATE(create_device());
    PROPAGATE(create_swapchain());

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
    create_info.enabledLayerCount = sizeof(validation_layers) / sizeof(validation_layers[0]);
    create_info.ppEnabledLayerNames = validation_layers;
#else
    create_info.enabledLayerCount = 0;
#endif
    
    PROPAGATE_VK(vkCreateInstance(&create_info, NULL, &glbl.instance));

    return SUCCESS;
}

result create_surface(void) {
    PROPAGATE_VK(glfwCreateWindowSurface(glbl.instance, glbl.window, NULL, &glbl.surface));
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

    result extension_check = physical_check_extensions(physical);
    if (!IS_SUCCESS(extension_check)) {
	return -1;
    }

    swapchain_support support;
    result support_check = physical_check_swapchain_support(physical, &support);
    if (!IS_SUCCESS(support_check) || support.num_formats == 0 || support.num_present_modes == 0) {
	return -1;
    }

    return device_type_score;
}

result physical_check_queue_family(VkPhysicalDevice physical, uint32_t* queue_family, VkQueueFlagBits bits) {
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &queue_family_count, NULL);
    if (queue_family_count == 0) {
	return CUSTOM_ERROR;
    }

    VkQueueFamilyProperties possible[MAX_VK_ENUMERATIONS];
    queue_family_count = queue_family_count < MAX_VK_ENUMERATIONS ? queue_family_count : MAX_VK_ENUMERATIONS;
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &queue_family_count, possible);
    queue_family_count = queue_family_count < MAX_VK_ENUMERATIONS ? queue_family_count : MAX_VK_ENUMERATIONS;

    for (uint32_t queue_family_index = 0; queue_family_index < queue_family_count; ++queue_family_index) {
	if ((possible[queue_family_index].queueFlags & bits) == bits) {
	    VkBool32 present_support = VK_FALSE;
	    vkGetPhysicalDeviceSurfaceSupportKHR(physical, queue_family_index, glbl.surface, &present_support);
	    if (present_support == VK_TRUE) {
		if (queue_family) *queue_family = queue_family_index;
		return SUCCESS;
	    }
	}
    }

    return CUSTOM_ERROR;
}

result physical_check_extensions(VkPhysicalDevice physical) {
    uint32_t extension_count = 0;
    vkEnumerateDeviceExtensionProperties(physical, NULL, &extension_count, NULL);

    VkExtensionProperties* available_extensions = malloc(extension_count * sizeof(VkExtensionProperties));
    vkEnumerateDeviceExtensionProperties(physical, NULL, &extension_count, available_extensions);

    uint32_t required_extension_index;
    for (required_extension_index = 0; required_extension_index < sizeof(device_extensions) / sizeof(device_extensions[0]); ++required_extension_index) {
	uint32_t available_extension_index;
	for (available_extension_index = 0; available_extension_index < extension_count; ++available_extension_index) {
	    if (!strcmp(available_extensions[available_extension_index].extensionName, device_extensions[required_extension_index])) {
		break;
	    }
	}
	if (available_extension_index >= extension_count) {
	    break;
	}
    }
    free(available_extensions);
    if (required_extension_index < sizeof(device_extensions) / sizeof(device_extensions[0])) {
	return CUSTOM_ERROR;
    }
    else {
	return SUCCESS;
    }
}

result physical_check_swapchain_support(VkPhysicalDevice physical, swapchain_support* support) {
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, glbl.surface, &support->capabilities);

    vkGetPhysicalDeviceSurfaceFormatsKHR(physical, glbl.surface, &support->num_formats, NULL);
    support->num_formats = support->num_formats < MAX_VK_ENUMERATIONS ? support->num_formats : MAX_VK_ENUMERATIONS;
    PROPAGATE_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical, glbl.surface, &support->num_formats, support->formats));
    support->num_formats = support->num_formats < MAX_VK_ENUMERATIONS ? support->num_formats : MAX_VK_ENUMERATIONS;

    vkGetPhysicalDeviceSurfacePresentModesKHR(physical, glbl.surface, &support->num_present_modes, NULL);
    support->num_present_modes = support->num_present_modes < MAX_VK_ENUMERATIONS ? support->num_present_modes : MAX_VK_ENUMERATIONS;
    PROPAGATE_VK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical, glbl.surface, &support->num_present_modes, support->present_modes));
    support->num_present_modes = support->num_present_modes < MAX_VK_ENUMERATIONS ? support->num_present_modes : MAX_VK_ENUMERATIONS;

    return SUCCESS;
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
    device_create_info.enabledExtensionCount = sizeof(device_extensions) / sizeof(device_extensions[0]);
    device_create_info.ppEnabledExtensionNames = device_extensions;

    PROPAGATE_VK(vkCreateDevice(glbl.physical, &device_create_info, NULL, &glbl.device));
    vkGetDeviceQueue(glbl.device, queue_family, 0, &glbl.queue);
    
    return SUCCESS;
}

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

    free(glbl.swapchain_images);
    vkGetSwapchainImagesKHR(glbl.device, glbl.swapchain, &image_count, NULL);
    glbl.swapchain_images = malloc(image_count * sizeof(VkImage));
    vkGetSwapchainImagesKHR(glbl.device, glbl.swapchain, &image_count, glbl.swapchain_images);

    glbl.swapchain_format = surface_format.format;
    glbl.swapchain_extent = swap_extent;

    free(glbl.swapchain_image_views);
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

    glbl.swapchain_image_count = image_count;
    
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
	if (support->present_modes[present_mode_index] == VK_PRESENT_MODE_FIFO_KHR) {
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

void cleanup(void) {
    for (uint32_t image_index = 0; image_index < glbl.swapchain_image_count; ++image_index) {
	vkDestroyImageView(glbl.device, glbl.swapchain_image_views[image_index], NULL);
    }
    free(glbl.swapchain_image_views);
    free(glbl.swapchain_images);
    vkDestroySwapchainKHR(glbl.device, glbl.swapchain, NULL);
    vkDestroyDevice(glbl.device, NULL);
    vkDestroySurfaceKHR(glbl.instance, glbl.surface, NULL);
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
