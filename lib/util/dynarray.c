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

#include "dynarray.h"

result dynarray_create(uint32_t elem_size, uint32_t init_alloc_num, dynarray* dynarray) {
    dynarray->size = 0;
    dynarray->alloc = init_alloc_num * elem_size;
    dynarray->elem_size = elem_size;

    dynarray->data = malloc(dynarray->alloc);
    if (!dynarray->data) return CUSTOM_ERROR;
    return SUCCESS;
}

result dynarray_push(void* push_data, dynarray* dynarray) {
    if (dynarray->size >= dynarray->alloc) {
	dynarray->alloc *= 2;
	void* attempt = realloc(dynarray->data, dynarray->alloc);
	if (!attempt) {
	    dynarray->alloc = 0;
	    free(dynarray->data);
	    return CUSTOM_ERROR;
	}
	dynarray->data = attempt;
    }

    memcpy((char*) dynarray->data + dynarray->size, push_data, dynarray->elem_size);
    dynarray->size += dynarray->elem_size;
    return SUCCESS;
}

result dynarray_pop(void* pop_data, dynarray* dynarray) {
    if (dynarray->size < dynarray->elem_size) return CUSTOM_ERROR;
    dynarray->size -= dynarray->elem_size;
    if (pop_data) memcpy(pop_data, (char*) dynarray->data + dynarray->size, dynarray->elem_size);
    return SUCCESS;
}

void* dynarray_index(uint32_t index, dynarray* dynarray) {
    return index < dynarray->size / dynarray->elem_size ? (void*) ((char*) dynarray->data + index * dynarray->elem_size) : NULL;
}

result dynarray_destroy(dynarray* dynarray) {
    free(dynarray->data);
    return SUCCESS;
}
