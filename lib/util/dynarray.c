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

#include "dynarray.h"

result dynarray_create(uint32_t elem_size, uint32_t init_alloc_num, dynarray* dynarray) {
    if (init_alloc_num == 0 || elem_size == 0) return CUSTOM_ERROR;
    dynarray->size = 0;
    dynarray->alloc = init_alloc_num * elem_size;
    dynarray->elem_size = elem_size;

    dynarray->data = malloc(dynarray->alloc);
    if (!dynarray->data) return CUSTOM_ERROR;
    return SUCCESS;
}

dynarray dynarray_create_singleton(void* contents, uint32_t elem_size) {
    dynarray dynarray = {0};
    dynarray.data = contents;
    dynarray.size = elem_size;
    dynarray.alloc = 0;
    dynarray.elem_size = elem_size;
    return dynarray;
}

result dynarray_push(void* push_data, dynarray* dynarray) {
    if (dynarray->alloc == 0) return CUSTOM_ERROR;
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

    if (push_data) {
	memcpy((char*) dynarray->data + dynarray->size, push_data, dynarray->elem_size);
    }
    else {
	memset((char*) dynarray->data + dynarray->size, 0, dynarray->elem_size);
    }
    dynarray->size += dynarray->elem_size;
    return SUCCESS;
}

result dynarray_pop(void* pop_data, dynarray* dynarray) {
    if (dynarray->size < dynarray->elem_size) return CUSTOM_ERROR;
    dynarray->size -= dynarray->elem_size;
    if (pop_data) memcpy(pop_data, (char*) dynarray->data + dynarray->size, dynarray->elem_size);
    return SUCCESS;
}

result dynarray_clear(dynarray* dynarray) {
    dynarray->size = 0;
}

uint32_t dynarray_len(dynarray* dynarray) {
    return dynarray->size / dynarray->elem_size;
}

void* dynarray_index(uint32_t index, dynarray* dynarray) {
    return index < dynarray_len(dynarray) ? (void*) ((char*) dynarray->data + index * dynarray->elem_size) : NULL;
}

void* dynarray_first(dynarray* dynarray) {
    return dynarray->size >= dynarray->elem_size ? dynarray_index(0, dynarray) : NULL;
}

void* dynarray_last(dynarray* dynarray) {
    return dynarray->size >= dynarray->elem_size ? dynarray_index(dynarray_len(dynarray) - 1, dynarray) : NULL;
}

result dynarray_destroy(dynarray* dynarray) {
    free(dynarray->data);
    return SUCCESS;
}

void dynarray_debug(dynarray* dynarray) {
    fprintf(stderr, "DEBUG (dynarray)   size (bytes): %u   allocated (bytes): %u   element size (bytes): %u", dynarray->size, dynarray->alloc, dynarray->elem_size);
}
