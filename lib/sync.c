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

result create_synchronization(void) {
    VkSemaphoreCreateInfo semaphore_info = {0};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = {0};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
	PROPAGATE_VK(vkCreateSemaphore(glbl.device, &semaphore_info, NULL, &glbl.image_available_semaphore[i]));
	PROPAGATE_VK(vkCreateSemaphore(glbl.device, &semaphore_info, NULL, &glbl.render_finished_semaphore[i]));
	PROPAGATE_VK(vkCreateFence(glbl.device, &fence_info, NULL, &glbl.frame_in_flight_fence[i]));
	PROPAGATE_VK(vkCreateSemaphore(glbl.device, &semaphore_info, NULL, &glbl.copy_finished_semaphore[i]));
    }

    return SUCCESS;
}
