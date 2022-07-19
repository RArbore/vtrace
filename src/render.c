/*
 * This file is part of vtrace-rs.
 * vtrace-rs is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 * vtrace-rs is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with vtrace-rs. If not, see <https://www.gnu.org/licenses/>.
 */

#include "render.h"

static renderer glbl;

void init(void) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    glbl.window_width = 800;
    glbl.window_height = 600;
    glbl.window = glfwCreateWindow(glbl.window_width, glbl.window_height, "vtrace-rs", NULL, NULL);
}

void cleanup(void) {
    glfwDestroyWindow(glbl.window);
    glfwTerminate();
}

int32_t render_tick(void) {
    if (glfwWindowShouldClose(glbl.window)) {
	cleanup();
	return -1;
    }

    glfwPollEvents();

    return 0;
}
