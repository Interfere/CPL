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

#include "cpl_allocator.h"

#include <assert.h>
#include <stddef.h>
#include <sys/mman.h>
#include <string.h>

/********************** Default Allocator Implementation **********************/
struct cpl_allocator
{
    void* (*xAllocate)(struct cpl_allocator*, size_t);
    void* (*xRealloc)(struct cpl_allocator*, void* ptr, size_t);
    void  (*xFree)(struct cpl_allocator*, void* ptr);
};

static inline void* cpl_default_malloc(struct cpl_allocator* pAllocator, size_t sz)
{
    return malloc(sz);
}

static inline void* cpl_default_realloc(struct cpl_allocator* pAllocator, void* ptr, size_t sz)
{
    return realloc(ptr, sz);
}

static inline void cpl_default_free(struct cpl_allocator* pAllocator, void* ptr)
{
    free(ptr);
}

/*********************** Public Allocator routines  ***************************/
cpl_allocator_ref cpl_allocator_get_default()
{
    static struct cpl_allocator _default_allocator = { cpl_default_malloc, cpl_default_realloc, cpl_default_free };
    return &_default_allocator;
}

void* cpl_allocator_allocate(cpl_allocator_ref allocator, size_t sz)
{
    return allocator->xAllocate(allocator, sz);
}

void cpl_allocator_free(cpl_allocator_ref allocator, void* ptr)
{
    return allocator->xFree(allocator, ptr);
}

void* cpl_allocator_realloc(cpl_allocator_ref allocator, void* ptr, size_t sz)
{
    return allocator->xRealloc(allocator, ptr, sz);
}
