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

#define PROPAGATE(result)			\
    {						\
	VkResult eval = result;			\
	if (eval != VK_SUCCESS) return eval;	\
    }

static renderer glbl;

static const char* validation_layers[] = {
    "VK_LAYER_KHRONOS_validation"
};

VkResult init(void) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    glbl.window_width = 1000;
    glbl.window_height = 1000;
    glbl.window = glfwCreateWindow(glbl.window_width, glbl.window_height, "vtrace", NULL, NULL);

    PROPAGATE(create_instance());

    return VK_SUCCESS;
}

VkResult create_instance(void) {
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
    
    PROPAGATE(vkCreateInstance(&create_info, NULL, &glbl.instance));

    return VK_SUCCESS;
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
