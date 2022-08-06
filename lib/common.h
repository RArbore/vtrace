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
#define FRAMES_IN_FLIGHT 2
#define COMMAND_QUEUE_SIZE 16
#define MAX_TEXTURES 4096

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

#define PROPAGATE_VK_C(res)						\
    {									\
	VkResult eval = res;						\
	if (eval != SUCCESS.vk) {					\
	    return 1;							\
	}								\
    }

#define PROPAGATE_C(res)						\
    {									\
	result eval = res;						\
	if (!IS_SUCCESS(eval)) return 1;				\
    }

#define PROPAGATE_CLEAN(res)						\
    {									\
    result PROPAGATE_CLEANUP_RETURN_VALUE_RESERVED = res;		\
    if (!IS_SUCCESS(eval)) {

#define PROPAGATE_END()					\
    return PROPAGATE_CLEANUP_RETURN_VALUE_RESEREVED;	\
    }							\
    }

#define PROPAGATE_VK_CLEAN(res)						\
    {									\
    VkResult PROPAGATE_CLEANUP_EVAL_VALUE_RESERVED = res;		\
    if (PROPAGATE_CLEANUP_EVAL_VALUE_RESERVED != SUCCESS.vk) {

#define PROPAGATE_VK_END()						\
    result PROPAGATE_CLEANUP_RETURN_VALUE_RESERVED = {.vk = PROPAGATE_CLEANUP_EVAL_VALUE_RESERVED, .custom = 0}; \
    return PROPAGATE_CLEANUP_RETURN_VALUE_RESERVED;			\
    }									\
    }

typedef struct render_tick_info {
    void* perspective;
    void* camera;
} render_tick_info;

typedef struct gpu_vertex {
    float pos[3];
} gpu_vertex;

typedef enum secondary_type {
    SECONDARY_TYPE_COPY_BUFFER_BUFFER,
    SECONDARY_TYPE_COPY_BUFFER_IMAGE,
    SECONDARY_TYPE_LAYOUT_TRANSITION
} secondary_type;

typedef struct secondary_command {
    secondary_type type;
    uint32_t delay;
    union {
	struct {
	    VkBuffer src_buffer;
	    VkBuffer dst_buffer;
	    VkBufferCopy copy_region;
	} copy_buffer_buffer;
	struct {
	    VkBuffer src_buffer;
	    VkImage dst_image;
	    VkBufferImageCopy copy_region;
	} copy_buffer_image;
	struct {
	    VkImage image;
	    VkImageLayout old;
	    VkImageLayout new;
	} layout_transition;
    };
} secondary_command;

typedef struct user_input {
    uint8_t keys[6];
    double mouse_x;
    double mouse_y;
    double last_mouse_x;
    double last_mouse_y;
} user_input;

typedef union descriptor_info {
    VkDescriptorImageInfo image_info;
    VkDescriptorBufferInfo buffer_info;
} descriptor_info;

typedef struct renderer {
    uint32_t current_frame;
    uint32_t num_frames_elapsed;
    uint32_t window_width;
    uint32_t window_height;
    uint32_t resized;
    GLFWwindow* window;

    user_input user_input;

    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physical;
    VkDevice device;
    VkQueue queue;

    VkSwapchainKHR swapchain;
    uint32_t swapchain_size;
    VkImage* swapchain_images;
    VkFormat swapchain_format;
    VkExtent2D swapchain_extent;
    VkImageView* swapchain_image_views;

    VkDescriptorPool descriptor_pool;
    VkDescriptorSetLayout graphics_descriptor_set_layout;
    VkDescriptorSet graphics_descriptor_sets[FRAMES_IN_FLIGHT];
    VkWriteDescriptorSet graphics_pending_descriptor_writes[FRAMES_IN_FLIGHT];
    descriptor_info graphics_pending_descriptor_write_infos[FRAMES_IN_FLIGHT];

    VkPipelineLayout graphics_pipeline_layout;
    VkRenderPass render_pass;
    VkPipeline graphics_pipeline;
    VkFramebuffer* framebuffers;
    VkImage depth_image;
    VkDeviceMemory depth_image_memory;
    VkImageView depth_image_view;

    VkCommandPool command_pool;
    VkCommandBuffer graphics_command_buffers[FRAMES_IN_FLIGHT];
    VkCommandBuffer secondary_command_buffers[FRAMES_IN_FLIGHT];

    VkBuffer staging_cube_vertex_buffer;
    VkBuffer staging_cube_index_buffer;
    VkDeviceMemory staging_cube_memory;
    VkBuffer cube_vertex_buffer;
    VkBuffer cube_index_buffer;
    VkDeviceMemory cube_memory;

    uint32_t instance_count;
    uint32_t instance_capacity;
    VkBuffer staging_instance_buffer;
    VkDeviceMemory staging_instance_memory;
    VkBuffer instance_buffer;
    VkDeviceMemory instance_memory;

    uint32_t staging_texture_size;
    VkBuffer staging_texture_buffer;
    VkDeviceMemory staging_texture_memory;
    uint32_t texture_image_count;
    uint32_t texture_image_allocated;
    uint32_t texture_memory_used;
    uint32_t texture_memory_allocated;
    VkImage* texture_images;
    VkImageView* texture_image_views;
    VkDeviceMemory texture_memory;
    VkSampler texture_sampler;
    VkFence texture_upload_finished_fence;

    VkSemaphore image_available_semaphore[FRAMES_IN_FLIGHT];
    VkSemaphore render_finished_semaphore[FRAMES_IN_FLIGHT];
    VkFence frame_in_flight_fence[FRAMES_IN_FLIGHT];

    VkSemaphore secondary_finished_semaphore[FRAMES_IN_FLIGHT];
    VkSemaphore secondary_intercommand_semaphore[FRAMES_IN_FLIGHT];
    uint32_t secondary_queue_size;
    secondary_command secondary_queue[COMMAND_QUEUE_SIZE];
    VkFence secondary_finished_fence;
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

__attribute__((unused)) static const result SUCCESS = {.vk = VK_SUCCESS, .custom = 0};
__attribute__((unused)) static const result CUSTOM_ERROR = {.vk = VK_SUCCESS, .custom = 1};
extern renderer glbl;

uint64_t entry(void);

result init(void);

result create_instance(void);

result create_surface(void);

result create_physical(void);

int32_t physical_score(VkPhysicalDevice physical);

result physical_check_queue_family(VkPhysicalDevice physical, uint32_t* queue_family, VkQueueFlagBits bits);

result physical_check_extensions(VkPhysicalDevice physical);

result physical_check_swapchain_support(VkPhysicalDevice physical, swapchain_support* support);

result physical_check_bindless_support(VkPhysicalDevice physical);

result create_device(void);

result create_swapchain(void);

result choose_swapchain_options(swapchain_support* support, VkSurfaceFormatKHR* surface_format, VkPresentModeKHR* present_mode, VkExtent2D* swap_extent);

result create_shader_module(VkShaderModule* module, const char* shader);

result create_descriptor_pool(void);

result create_descriptor_layouts(void);

result create_descriptor_sets(void);

result create_graphics_pipeline(void);

result create_framebuffers(void);

result create_depth_resources(void);

result create_command_pool(void);

result create_command_buffers(void);

result record_graphics_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index, const render_tick_info* render_tick_info);

result record_secondary_command_buffer(VkCommandBuffer command_buffer, uint32_t num_commands, secondary_command* commands);

result create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer* buffer);

result create_buffer_memory(VkMemoryPropertyFlags properties, VkDeviceMemory* memory, VkBuffer* buffers, uint32_t num_buffers, uint32_t* offsets, uint32_t minimum_size);

result create_image(VkImageCreateFlags flags, VkFormat format, VkExtent3D extent, uint32_t mipLevels, uint32_t arrayLevels, VkImageUsageFlagBits usage, VkImage* image);

result create_image_view(VkImage image, VkImageViewType type, VkFormat format, VkImageSubresourceRange subresource_range, VkImageView* view);

result create_image_memory(VkMemoryPropertyFlags properties, VkDeviceMemory* memory, VkImage* images, uint32_t num_images, uint32_t* offsets, uint32_t minimum_size);

result create_cube_buffer(void);

result create_instance_buffer(void);

int32_t update_instances(const float* instances, uint32_t instance_count);

result create_staging_texture_buffer(void);

result create_texture_resources(void);

result create_texture_sampler(void);

int32_t add_texture(const uint8_t* data, uint32_t width, uint32_t height, uint32_t depth);

result update_descriptors(void);

void get_vertex_input_descriptions(VkVertexInputBindingDescription* vertex_input_binding_description, VkVertexInputAttributeDescription* vertex_input_attribute_description);

result queue_secondary_command(secondary_command command);

result set_secondary_fence(VkFence fence);

result find_memory_type(uint32_t filter, VkMemoryPropertyFlags properties, uint32_t* type);

result create_synchronization(void);

void cleanup(void);

void recreate_swapchain(void);

void cleanup_swapchain(void);

void cleanup_instance_buffer(void);

void cleanup_staging_texture_buffer(void);

void cleanup_texture_images(void);

void cleanup_texture_resources(void);

user_input* get_input_data_pointer(void);

int32_t render_tick(int32_t* window_width, int32_t* window_height, const render_tick_info* render_tick_info);
