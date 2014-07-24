/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2013 Alexey Komnin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * C Primitives Library. Allocator implementation.
 */

#ifndef _CPL_ALLOCATOR_H_
#define _CPL_ALLOCATOR_H_

#include <stdlib.h>

/**
 * Allocator type to use in CPL internals.
 */
typedef struct cpl_allocator* cpl_allocator_ref;

/**
 * Default allocator accessor. Represents basic allocation routines, such as
 * malloc() and free().
 */
cpl_allocator_ref cpl_allocator_get_default();

/**
 * Constructor and Destructor for pool allocator.
 */
cpl_allocator_ref cpl_allocator_create_pool(size_t chunkSize, int nChunks);
void cpl_allocator_destroy_pool(cpl_allocator_ref);

/**
 * Constructor and Destructor for Doug Lea's allocator.
 */
cpl_allocator_ref cpl_allocator_create_dl(size_t max_size);
void cpl_allocator_destroy_dl(cpl_allocator_ref);

/**
 * Alloc() and Free() routines that encapsulates allocator-related internals.
 */
void* cpl_allocator_allocate(cpl_allocator_ref, size_t);
void cpl_allocator_free(cpl_allocator_ref, void*);

void* cpl_allocator_realloc(cpl_allocator_ref, void*, size_t);

#endif // _CPL_ALLOCATOR_H_
