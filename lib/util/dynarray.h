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

#ifndef DYNARRAY_H
#define DYNARRAY_H

#include <stdint.h>

#define INDEX(i, dynarray, T) (*(T*) dynarray_index(i, &dynarray))

typedef struct dynarray {
    void* data;
    uint32_t size;
    uint32_t alloc;
    uint32_t elem_size;
} dynarray;

#include "../common.h"

result dynarray_create(uint32_t elem_size, uint32_t init_alloc_num, dynarray* dynarray);

dynarray dynarray_create_singleton(void* contents, uint32_t elem_size);

result dynarray_push(void* push_data, dynarray* dynarray);

result dynarray_pop(void* pop_data, dynarray* dynarray);

result dynarray_clear(dynarray* dynarray);

uint32_t dynarray_len(dynarray* dynarray);

void* dynarray_index(uint32_t index, dynarray* dynarray);

void* dynarray_first(dynarray* dynarray);

void* dynarray_last(dynarray* dynarray);

result dynarray_destroy(dynarray* dynarray);

void dynarray_debug(dynarray* dynarray);

#endif
