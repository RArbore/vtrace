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

result create_raster_pipeline(void) {
    VkShaderModule vertex_shader, fragment_shader;
    PROPAGATE(create_shader_module(&vertex_shader, "shaders/trace.vert.spv"));
    PROPAGATE(create_shader_module(&fragment_shader, "shaders/trace.frag.spv"));

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

    VkVertexInputBindingDescription vertex_input_binding_description[2] = {0};
    VkVertexInputAttributeDescription vertex_input_attribute_description[5] = {0};
    VkPipelineVertexInputStateCreateInfo vertex_input_create_info = {0};
    get_vertex_input_descriptions(vertex_input_binding_description, vertex_input_attribute_description);
    vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_create_info.vertexBindingDescriptionCount = sizeof(vertex_input_binding_description) / sizeof(vertex_input_binding_description[0]);
    vertex_input_create_info.vertexAttributeDescriptionCount = sizeof(vertex_input_attribute_description) / sizeof(vertex_input_attribute_description[0]);
    vertex_input_create_info.pVertexBindingDescriptions = vertex_input_binding_description;
    vertex_input_create_info.pVertexAttributeDescriptions = vertex_input_attribute_description;

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

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {0};
    depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_create_info.depthTestEnable = VK_TRUE;
    depth_stencil_state_create_info.depthWriteEnable = VK_TRUE;
    depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;

    VkPushConstantRange push_constant_range = {0};
    push_constant_range.offset = 0;
    push_constant_range.size = sizeof(float) * 4 * 4 * 2;
    push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {0};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.pushConstantRangeCount = 1;
    pipeline_layout_create_info.pPushConstantRanges = &push_constant_range;
    pipeline_layout_create_info.setLayoutCount = 1;
    pipeline_layout_create_info.pSetLayouts = &glbl.raster_descriptor_set_layout;

    PROPAGATE_VK(vkCreatePipelineLayout(glbl.device, &pipeline_layout_create_info, NULL, &glbl.raster_pipeline_layout));

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

    VkAttachmentDescription depth_attachment = {0};
    depth_attachment.format = VK_FORMAT_D32_SFLOAT;
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_reference = {0};
    depth_attachment_reference.attachment = 1;
    depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_reference;
    subpass.pDepthStencilAttachment = &depth_attachment_reference;

    VkSubpassDependency subpass_dependency = {0};
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription attachments[] = {color_attachment, depth_attachment};
    
    VkRenderPassCreateInfo render_pass_create_info = {0};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = 2;
    render_pass_create_info.pAttachments = attachments;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = &subpass_dependency;

    PROPAGATE_VK(vkCreateRenderPass(glbl.device, &render_pass_create_info, NULL, &glbl.render_pass));

    VkGraphicsPipelineCreateInfo raster_pipeline_create_info = {0};
    raster_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    raster_pipeline_create_info.stageCount = 2;
    raster_pipeline_create_info.pStages = shader_stage_create_infos;
    raster_pipeline_create_info.pVertexInputState = &vertex_input_create_info;
    raster_pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
    raster_pipeline_create_info.pViewportState = &viewport_state_create_info;
    raster_pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
    raster_pipeline_create_info.pColorBlendState = &color_blending_state_create_info;
    raster_pipeline_create_info.pDepthStencilState = &depth_stencil_state_create_info;
    raster_pipeline_create_info.pDynamicState = &pipeline_dynamic_state_create_info;
    raster_pipeline_create_info.layout = glbl.raster_pipeline_layout;
    raster_pipeline_create_info.renderPass = glbl.render_pass;
    raster_pipeline_create_info.subpass = 0;

    PROPAGATE_VK(vkCreateGraphicsPipelines(glbl.device, VK_NULL_HANDLE, 1, &raster_pipeline_create_info, NULL, &glbl.raster_pipeline));
 
    vkDestroyShaderModule(glbl.device, vertex_shader, NULL);
    vkDestroyShaderModule(glbl.device, fragment_shader, NULL);
    
    return SUCCESS;
}

result create_framebuffers(void) {
    PROPAGATE(dynarray_create(sizeof(VkFramebuffer), dynarray_len(&glbl.swapchain_images), &glbl.framebuffers));

    VkFramebufferCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    create_info.renderPass = glbl.render_pass;
    create_info.attachmentCount = 2;
    create_info.width = glbl.swapchain_extent.width;
    create_info.height = glbl.swapchain_extent.height;
    create_info.layers = 1;

    for (uint32_t i = 0; i < dynarray_len(&glbl.swapchain_images); ++i) {
	VkImageView attachments[] = {INDEX(i, glbl.swapchain_image_views, VkImageView), glbl.depth_image_view};
	create_info.pAttachments = attachments;
	PROPAGATE(dynarray_push(NULL, &glbl.framebuffers));
	PROPAGATE_VK(vkCreateFramebuffer(glbl.device, &create_info, NULL, dynarray_index(i, &glbl.framebuffers)));
    }

    return SUCCESS;
}

result create_depth_resources(void) {
    VkExtent3D extent;
    extent.width = glbl.swapchain_extent.width;
    extent.height = glbl.swapchain_extent.height;
    extent.depth = 1;

    PROPAGATE(create_image(0, VK_FORMAT_D32_SFLOAT, extent, 1, 1, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, &glbl.depth_image));

    PROPAGATE(create_image_memory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &glbl.depth_image_memory, &glbl.depth_image, 1, NULL, 0));

    VkImageSubresourceRange subresource_range = {0};
    subresource_range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    subresource_range.baseMipLevel = 0;
    subresource_range.levelCount = 1;
    subresource_range.baseArrayLayer = 0;
    subresource_range.layerCount = 1;
    
    PROPAGATE(create_image_view(glbl.depth_image, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_D32_SFLOAT, subresource_range, &glbl.depth_image_view));

    return SUCCESS;
}
