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

static const gpu_vertex cube_vertices[] = {
    {-0.5f, -0.5f, 0.0f},
    {0.5f, -0.5f, 0.0f},
    {0.5f, 0.5f, 0.0f},
    {-0.5f, 0.5f, 0.0f},
};

static const uint16_t cube_indices[] = {
    0, 1, 2, 2, 3, 0
};

static void glfw_framebuffer_resize_callback(__attribute__((unused)) GLFWwindow* window, __attribute__((unused)) int width, __attribute__((unused)) int height) {
    glbl.resized = 1;
}

uint64_t entry(void) {
    result res = init();
    return ((uint64_t) res.vk << 32) | (uint64_t) res.custom;
}

result init(void) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    glbl.window_width = 1000;
    glbl.window_height = 1000;
    glbl.window = glfwCreateWindow(glbl.window_width, glbl.window_height, "vtrace", NULL, NULL);
    glfwSetFramebufferSizeCallback(glbl.window, glfw_framebuffer_resize_callback);

    PROPAGATE(create_instance());
    PROPAGATE(create_surface());
    PROPAGATE(create_physical());
    PROPAGATE(create_device());
    PROPAGATE(create_swapchain());
    PROPAGATE(create_graphics_pipeline());
    PROPAGATE(create_framebuffers());
    PROPAGATE(create_command_pool());
    PROPAGATE(create_command_buffers());
    PROPAGATE(create_cube_buffer());
    PROPAGATE(create_synchronization());

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

result create_shader_module(VkShaderModule* module, const char* shader_path) {
    FILE* shader_file = fopen(shader_path, "r");

    if (!shader_file) {
	fprintf(stderr, "ERROR: Couldn't open shader file at %s\n", shader_path);
	return CUSTOM_ERROR;
    }

    if (fseek(shader_file, 0, SEEK_END)) {
	fprintf(stderr, "ERROR: Couldn't query size of file using fseek\n");
	fclose(shader_file);
	return CUSTOM_ERROR;
    }

    int64_t shader_file_size = ftell(shader_file);

    if (fseek(shader_file, 0, SEEK_SET)) {
	fprintf(stderr, "ERROR: Couldn't move back to the beginning of shader file\n");
	fclose(shader_file);
	return CUSTOM_ERROR;
    }

    char* shader_file_contents = malloc((shader_file_size + 1) * sizeof(char));
    size_t bytes_read = fread(shader_file_contents, sizeof(char), shader_file_size, shader_file);
    shader_file_contents[bytes_read] = '\0';
    if (bytes_read != (size_t) shader_file_size) {
	fprintf(stderr, "ERROR: Bytes read from file is not the same amount allocated\n");
	free(shader_file_contents);
	fclose(shader_file);
	return CUSTOM_ERROR;
    }

    fclose(shader_file);

    VkShaderModuleCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = bytes_read;
    create_info.pCode = (uint32_t*) shader_file_contents;

    PROPAGATE_VK_CLEAN(vkCreateShaderModule(glbl.device, &create_info, NULL, module));
    free(shader_file_contents);
    PROPAGATE_VK_END();

    free(shader_file_contents);

    return SUCCESS;
}

result create_graphics_pipeline(void) {
    VkShaderModule vertex_shader, fragment_shader;
    PROPAGATE(create_shader_module(&vertex_shader, "shaders/test.vert.spv"));
    PROPAGATE(create_shader_module(&fragment_shader, "shaders/test.frag.spv"));

    VkPipelineShaderStageCreateInfo vertex_shader_stage_create_info = {0};
    vertex_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertex_shader_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_shader_stage_create_info.module = vertex_shader;
    vertex_shader_stage_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo fragment_shader_stage_create_info = {0};
    fragment_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_shader_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_shader_stage_create_info.module = fragment_shader;
    fragment_shader_stage_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stage_create_infos[] = {vertex_shader_stage_create_info, fragment_shader_stage_create_info};

    VkDynamicState pipeline_dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo pipeline_dynamic_state_create_info = {0};
    pipeline_dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    pipeline_dynamic_state_create_info.dynamicStateCount = 2;
    pipeline_dynamic_state_create_info.pDynamicStates = pipeline_dynamic_states;

    VkVertexInputBindingDescription vertex_input_binding_description = {0};
    VkVertexInputAttributeDescription vertex_input_attribute_description = {0};
    VkPipelineVertexInputStateCreateInfo vertex_input_create_info = {0};
    get_vertex_input_descriptions(&vertex_input_binding_description, &vertex_input_attribute_description);
    vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_create_info.vertexBindingDescriptionCount = 1;
    vertex_input_create_info.vertexAttributeDescriptionCount = 1;
    vertex_input_create_info.pVertexBindingDescriptions = &vertex_input_binding_description;
    vertex_input_create_info.pVertexAttributeDescriptions = &vertex_input_attribute_description;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info = {0};
    input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_create_info.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewport_state_create_info = {0};
    viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_create_info.viewportCount = 1;
    viewport_state_create_info.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {0};
    rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state_create_info.depthClampEnable = VK_FALSE;
    rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_state_create_info.lineWidth = 1.0f;
    rasterization_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization_state_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterization_state_create_info.depthBiasEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment_state = {0};
    color_blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment_state.blendEnable = VK_TRUE;
    color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blending_state_create_info = {0};
    color_blending_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending_state_create_info.logicOpEnable = VK_FALSE;
    color_blending_state_create_info.logicOp = VK_LOGIC_OP_COPY;
    color_blending_state_create_info.attachmentCount = 1;
    color_blending_state_create_info.pAttachments = &color_blend_attachment_state;

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {0};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    PROPAGATE_VK(vkCreatePipelineLayout(glbl.device, &pipeline_layout_create_info, NULL, &glbl.graphics_pipeline_layout));

    VkAttachmentDescription color_attachment = {0};
    color_attachment.format = glbl.swapchain_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_reference = {0};
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_reference;

    VkSubpassDependency subpass_dependency = {0};
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_create_info = {0};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = 1;
    render_pass_create_info.pAttachments = &color_attachment;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = &subpass_dependency;

    PROPAGATE_VK(vkCreateRenderPass(glbl.device, &render_pass_create_info, NULL, &glbl.render_pass));

    VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {0};
    graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphics_pipeline_create_info.stageCount = 2;
    graphics_pipeline_create_info.pStages = shader_stage_create_infos;
    graphics_pipeline_create_info.pVertexInputState = &vertex_input_create_info;
    graphics_pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
    graphics_pipeline_create_info.pViewportState = &viewport_state_create_info;
    graphics_pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
    graphics_pipeline_create_info.pColorBlendState = &color_blending_state_create_info;
    graphics_pipeline_create_info.pDynamicState = &pipeline_dynamic_state_create_info;
    graphics_pipeline_create_info.layout = glbl.graphics_pipeline_layout;
    graphics_pipeline_create_info.renderPass = glbl.render_pass;
    graphics_pipeline_create_info.subpass = 0;

    PROPAGATE_VK(vkCreateGraphicsPipelines(glbl.device, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, NULL, &glbl.graphics_pipeline));
 
    vkDestroyShaderModule(glbl.device, vertex_shader, NULL);
    vkDestroyShaderModule(glbl.device, fragment_shader, NULL);
    
    return SUCCESS;
}

result create_framebuffers(void) {
    glbl.framebuffers = malloc(glbl.swapchain_size * sizeof(VkFramebuffer));

    VkFramebufferCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    create_info.renderPass = glbl.render_pass;
    create_info.attachmentCount = 1;
    create_info.width = glbl.swapchain_extent.width;
    create_info.height = glbl.swapchain_extent.height;
    create_info.layers = 1;

    for (uint32_t i = 0; i < glbl.swapchain_size; ++i) {
	create_info.pAttachments = &glbl.swapchain_image_views[i];
	PROPAGATE_VK_CLEAN(vkCreateFramebuffer(glbl.device, &create_info, NULL, &glbl.framebuffers[i]));
	free(glbl.framebuffers);
	PROPAGATE_VK_END();
    }

    return SUCCESS;
}

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

    PROPAGATE_VK(vkAllocateCommandBuffers(glbl.device, &allocate_info, glbl.graphics_command_buffers));
    PROPAGATE_VK(vkAllocateCommandBuffers(glbl.device, &allocate_info, glbl.copy_command_buffers));
    
    return SUCCESS;
}

result record_graphics_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index) {
    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    PROPAGATE_VK(vkBeginCommandBuffer(command_buffer, &begin_info));

    VkClearValue clear_color;
    clear_color.color.float32[0] = 0.0f;
    clear_color.color.float32[1] = 0.0f;
    clear_color.color.float32[2] = 0.0f;
    clear_color.color.float32[3] = 1.0f;
    
    VkRenderPassBeginInfo render_pass_begin_info = {0};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass = glbl.render_pass;
    render_pass_begin_info.framebuffer = glbl.framebuffers[image_index];
    render_pass_begin_info.renderArea.offset.x = 0;
    render_pass_begin_info.renderArea.offset.y = 0;
    render_pass_begin_info.renderArea.extent = glbl.swapchain_extent;
    render_pass_begin_info.clearValueCount = 1;
    render_pass_begin_info.pClearValues = &clear_color;

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
    vkCmdBindIndexBuffer(command_buffer, glbl.cube_index_buffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(command_buffer, sizeof(cube_indices) / sizeof(cube_indices[0]), 1, 0, 0, 0);

    vkCmdEndRenderPass(command_buffer);

    PROPAGATE_VK(vkEndCommandBuffer(command_buffer));
    
    return SUCCESS;
}

result record_copy_command_buffer(VkCommandBuffer command_buffer, copy_command* command) {
    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    PROPAGATE_VK(vkBeginCommandBuffer(command_buffer, &begin_info));

    vkCmdCopyBuffer(command_buffer, command->src_buffer, command->dst_buffer, 1, &command->copy_region);
    
    PROPAGATE_VK(vkEndCommandBuffer(command_buffer));
    
    return SUCCESS;
}

result create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer* buffer) {
    VkBufferCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size = size;
    create_info.usage = usage;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    PROPAGATE_VK(vkCreateBuffer(glbl.device, &create_info, NULL, buffer));

    return SUCCESS;
}

static uint32_t round_up(uint32_t num_to_round, uint32_t multiple)
{
    if (multiple == 0)
        return num_to_round;

    uint32_t remainder = num_to_round % multiple;
    if (remainder == 0)
        return num_to_round;

    return num_to_round + multiple - remainder;
}

result create_memory(VkMemoryPropertyFlags properties, VkDeviceMemory* memory, VkBuffer* buffers, uint32_t num_buffers, uint32_t* offsets) {
    uint32_t allocate_size = 0;
    uint32_t memory_type_bits = 0;
    uint32_t local_offsets[num_buffers];
    if (!offsets) {
	offsets = &local_offsets[0];
    }
    for (uint32_t i = 0; i < num_buffers; ++i) {
	VkMemoryRequirements requirements;
	vkGetBufferMemoryRequirements(glbl.device, buffers[i], &requirements);

	allocate_size = round_up(allocate_size, requirements.alignment);
	offsets[i] = allocate_size;
	allocate_size += round_up(requirements.size, requirements.alignment);

	memory_type_bits |= requirements.memoryTypeBits;
    }

    VkMemoryAllocateInfo allocate_info = {0};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = allocate_size;
    PROPAGATE(find_memory_type(memory_type_bits, properties, &allocate_info.memoryTypeIndex));
    PROPAGATE_VK(vkAllocateMemory(glbl.device, &allocate_info, NULL, memory));

    for (uint32_t i = 0; i < num_buffers; ++i) {
	PROPAGATE_VK(vkBindBufferMemory(glbl.device, buffers[i], *memory, offsets[i]));
    }

    return SUCCESS;
}

result create_cube_buffer(void) {
    uint32_t vertex_buffer_size = sizeof(cube_vertices);
    uint32_t index_buffer_size = sizeof(cube_indices);

    VkBuffer cube_buffers[2];
    PROPAGATE(create_buffer(vertex_buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &cube_buffers[0]));
    PROPAGATE(create_buffer(index_buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &cube_buffers[1]));
    PROPAGATE(create_memory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &glbl.cube_buffer_memory, cube_buffers, 2, NULL));
    glbl.cube_vertex_buffer = cube_buffers[0];
    glbl.cube_index_buffer = cube_buffers[1];

    uint32_t buffer_offsets[2];
    PROPAGATE(create_buffer(vertex_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &cube_buffers[0]));
    PROPAGATE(create_buffer(index_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &cube_buffers[1]));
    PROPAGATE(create_memory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &glbl.staging_cube_buffer_memory, cube_buffers, 2, buffer_offsets));
    glbl.staging_cube_vertex_buffer = cube_buffers[0];
    glbl.staging_cube_index_buffer = cube_buffers[1];

    void* vertex_data;
    void* index_data;
    vkMapMemory(glbl.device, glbl.staging_cube_buffer_memory, buffer_offsets[0], vertex_buffer_size, 0, &vertex_data);
    memcpy(vertex_data, &cube_vertices[0].pos[0], (size_t) vertex_buffer_size);
    vkUnmapMemory(glbl.device, glbl.staging_cube_buffer_memory);
    vkMapMemory(glbl.device, glbl.staging_cube_buffer_memory, buffer_offsets[1], index_buffer_size, 0, &index_data);
    memcpy(index_data, &cube_indices[0], (size_t) index_buffer_size);
    vkUnmapMemory(glbl.device, glbl.staging_cube_buffer_memory);

    VkBufferCopy copy_region = {0};
    copy_region.size = vertex_buffer_size;
    PROPAGATE(queue_copy_buffer(glbl.cube_vertex_buffer, glbl.staging_cube_vertex_buffer, copy_region));
    copy_region.size = index_buffer_size;
    PROPAGATE(queue_copy_buffer(glbl.cube_index_buffer, glbl.staging_cube_index_buffer, copy_region));
    
    return SUCCESS;
}

void get_vertex_input_descriptions(VkVertexInputBindingDescription* vertex_input_binding_description, VkVertexInputAttributeDescription* vertex_input_attribute_description) {
    vertex_input_binding_description->binding = 0;
    vertex_input_binding_description->stride = sizeof(gpu_vertex);
    vertex_input_binding_description->inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    vertex_input_attribute_description->binding = 0;
    vertex_input_attribute_description->location = 0;
    vertex_input_attribute_description->format = VK_FORMAT_R32G32B32_SFLOAT;
    vertex_input_attribute_description->offset = 0;
}

result queue_copy_buffer(VkBuffer dst_buffer, VkBuffer src_buffer, VkBufferCopy copy_region) {
    if (glbl.copy_queue_size >= COPY_QUEUE_SIZE) {
	fprintf(stderr, "ERROR: Attempted to queue copy operation, but copy queue is full");
	return CUSTOM_ERROR;
    }
    
    glbl.copy_queue[glbl.copy_queue_size].src_buffer = src_buffer;
    glbl.copy_queue[glbl.copy_queue_size].dst_buffer = dst_buffer;
    glbl.copy_queue[glbl.copy_queue_size].copy_region = copy_region;
    ++glbl.copy_queue_size;

    return SUCCESS;
}

result find_memory_type(uint32_t filter, VkMemoryPropertyFlags properties, uint32_t* type) {
    VkPhysicalDeviceMemoryProperties physical_mem_properties;
    vkGetPhysicalDeviceMemoryProperties(glbl.physical, &physical_mem_properties);

    for (uint32_t i = 0; i < physical_mem_properties.memoryTypeCount; i++) {
	if ((filter & (1 << i)) && (physical_mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
	    *type = i;
	    return SUCCESS;
	}
    }

    fprintf(stderr, "ERROR: Couldn't find suitable memory type\n");
    return CUSTOM_ERROR;
}

result create_synchronization(void) {
    VkSemaphoreCreateInfo semaphore_info = {0};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = {0};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
	PROPAGATE_VK(vkCreateSemaphore(glbl.device, &semaphore_info, NULL, &glbl.image_available_semaphore[i]));
	PROPAGATE_VK(vkCreateSemaphore(glbl.device, &semaphore_info, NULL, &glbl.copy_finished_semaphore[i]));
	PROPAGATE_VK(vkCreateSemaphore(glbl.device, &semaphore_info, NULL, &glbl.render_finished_semaphore[i]));
	PROPAGATE_VK(vkCreateFence(glbl.device, &fence_info, NULL, &glbl.frame_in_flight_fence[i]));
    }

    return SUCCESS;
}

void cleanup(void) {
    vkDeviceWaitIdle(glbl.device);

    cleanup_swapchain();

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
	vkDestroySemaphore(glbl.device, glbl.image_available_semaphore[i], NULL);
	vkDestroySemaphore(glbl.device, glbl.copy_finished_semaphore[i], NULL);
	vkDestroySemaphore(glbl.device, glbl.render_finished_semaphore[i], NULL);
	vkDestroyFence(glbl.device, glbl.frame_in_flight_fence[i], NULL);
    }

    vkDestroyBuffer(glbl.device, glbl.cube_vertex_buffer, NULL);
    vkDestroyBuffer(glbl.device, glbl.cube_index_buffer, NULL);
    vkFreeMemory(glbl.device, glbl.cube_buffer_memory, NULL);
    vkDestroyBuffer(glbl.device, glbl.staging_cube_vertex_buffer, NULL);
    vkDestroyBuffer(glbl.device, glbl.staging_cube_index_buffer, NULL);
    vkFreeMemory(glbl.device, glbl.staging_cube_buffer_memory, NULL);
    
    vkDestroyCommandPool(glbl.device, glbl.command_pool, NULL);
     
    vkDestroyPipeline(glbl.device, glbl.graphics_pipeline, NULL);
    vkDestroyPipelineLayout(glbl.device, glbl.graphics_pipeline_layout, NULL);

    vkDestroyRenderPass(glbl.device, glbl.render_pass, NULL);

    vkDestroyDevice(glbl.device, NULL);
    vkDestroySurfaceKHR(glbl.instance, glbl.surface, NULL);
    vkDestroyInstance(glbl.instance, NULL);
    
    glfwDestroyWindow(glbl.window);
    glfwTerminate();
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
    create_framebuffers();
}

void cleanup_swapchain(void) {
    for (uint32_t framebuffer_index = 0; framebuffer_index < glbl.swapchain_size; ++framebuffer_index) {
	vkDestroyFramebuffer(glbl.device, glbl.framebuffers[framebuffer_index], NULL);
    }

    for (uint32_t image_index = 0; image_index < glbl.swapchain_size; ++image_index) {
	vkDestroyImageView(glbl.device, glbl.swapchain_image_views[image_index], NULL);
    }

    free(glbl.swapchain_image_views);
    free(glbl.swapchain_images);
    vkDestroySwapchainKHR(glbl.device, glbl.swapchain, NULL);
}

int32_t render_tick(int32_t* window_width, int32_t* window_height) {
    static uint32_t current_frame = 0;
    
    if (glfwWindowShouldClose(glbl.window)) {
	return -1;
    }

    glfwPollEvents();

    vkWaitForFences(glbl.device, 1, &glbl.frame_in_flight_fence[current_frame], VK_TRUE, UINT64_MAX);

    uint32_t image_index;
    VkResult acquire_image_result = vkAcquireNextImageKHR(glbl.device, glbl.swapchain, UINT64_MAX, glbl.image_available_semaphore[current_frame], VK_NULL_HANDLE, &image_index);
    if (acquire_image_result == VK_ERROR_OUT_OF_DATE_KHR) {
	recreate_swapchain();
	return 0;
    } else if (acquire_image_result != VK_SUCCESS && acquire_image_result != VK_SUBOPTIMAL_KHR) {
	return -1;
    }

    vkResetFences(glbl.device, 1, &glbl.frame_in_flight_fence[current_frame]);

    vkResetCommandBuffer(glbl.graphics_command_buffers[current_frame], 0);
    record_graphics_command_buffer(glbl.graphics_command_buffers[current_frame], image_index);

    uint32_t do_copy = 0;
    if (glbl.copy_queue_size > 0) {
	do_copy = 1;
	--glbl.copy_queue_size;

	vkResetCommandBuffer(glbl.copy_command_buffers[current_frame], 0);
	record_copy_command_buffer(glbl.copy_command_buffers[current_frame], &glbl.copy_queue[glbl.copy_queue_size]);

	VkSubmitInfo submit_info = {0};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &glbl.copy_command_buffers[current_frame];
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &glbl.copy_finished_semaphore[current_frame];

	vkQueueSubmit(glbl.queue, 1, &submit_info, VK_NULL_HANDLE);
    }
    
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
    VkSemaphore copy_wait_semaphores[] = {glbl.image_available_semaphore[current_frame], glbl.copy_finished_semaphore[current_frame]};

    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = do_copy ? 2 : 1;
    submit_info.pWaitSemaphores = do_copy ? copy_wait_semaphores : &glbl.image_available_semaphore[current_frame];
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &glbl.render_finished_semaphore[current_frame];
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &glbl.graphics_command_buffers[current_frame];

    vkQueueSubmit(glbl.queue, 1, &submit_info, glbl.frame_in_flight_fence[current_frame]);

    VkPresentInfoKHR present_info = {0};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &glbl.render_finished_semaphore[current_frame];
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

    current_frame = (current_frame + 1) % FRAMES_IN_FLIGHT;
    *window_width = glbl.window_width;
    *window_height = glbl.window_height;
    
    return 0;
}
