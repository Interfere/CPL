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
#include <string.h>
#include <sys/mman.h>

#include "cpl_list.h"

#ifndef MAP_ANONYMOUS
#   ifdef MAP_ANON
#       define MAP_ANONYMOUS MAP_ANON
#   endif
#endif

/*********************** Pool Allocator Implementation ************************/
struct cpl_pool_allocator
{
    /* struct cpl_allocator */
    void* (*xAllocate)(struct cpl_allocator*, size_t);
    void* (*xRealloc)(struct cpl_allocator*, void* ptr, size_t);
    void  (*xFree)(struct cpl_allocator*, void* ptr);
    
    /* pool-specific data */
    void*   pool;
    size_t  poolSize;
    size_t  chunkSize;
    int     nChunks;
    cpl_slist_t list;
};

static void* cpl_pool_malloc(struct cpl_allocator* pAllocator, size_t sz)
{
    struct cpl_pool_allocator* pPoolAllocator = (struct cpl_pool_allocator *)pAllocator;
    assert(sz == pPoolAllocator->chunkSize);
    void* ptr = cpl_slist_pop(&pPoolAllocator->list);
    return ptr;
}

static void cpl_pool_free(struct cpl_allocator* pAllocator, void* ptr)
{
    struct cpl_pool_allocator* pPoolAllocator = (struct cpl_pool_allocator *)pAllocator;
    cpl_slist_add(&pPoolAllocator->list, ptr);
}

static void* cpl_pool_realloc(struct cpl_allocator* pAllocator, void* ptr, size_t sz)
{
    void* new_ptr = cpl_pool_malloc(pAllocator, sz);
    if(new_ptr)
    {
        memcpy(new_ptr, ptr, sz);
        cpl_pool_free(pAllocator, ptr);
        ptr = new_ptr;
    }
    
    return ptr;
}

/******************** Public Pool Allocator routines  *************************/
cpl_allocator_ref cpl_allocator_create_pool(size_t chunkSize, int nChunks)
{
    assert(chunkSize < 8192 && chunkSize > 16);
    
    size_t poolSize = chunkSize * nChunks;
    void* poolBuffer = mmap(0, poolSize + sizeof(struct cpl_pool_allocator), PROT_READ|PROT_WRITE,
                            MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    
    if(poolBuffer == MAP_FAILED)
    {
        return 0;
    }
    
    struct cpl_pool_allocator* poolAllocator = (struct cpl_pool_allocator*)((char*)poolBuffer + poolSize);
    
    poolAllocator->xAllocate = cpl_pool_malloc;
    poolAllocator->xRealloc = cpl_pool_realloc;
    poolAllocator->xFree = cpl_pool_free;
    poolAllocator->pool = poolBuffer;
    poolAllocator->poolSize = poolSize;
    poolAllocator->chunkSize = chunkSize;
    poolAllocator->nChunks = nChunks;
    CPL_SLIST_INIT(poolAllocator->list);
    
    for (int i = nChunks-1; i >= 0; --i)
    {
        cpl_slist_add(&poolAllocator->list, (cpl_slist_ref)((char*)poolBuffer + i*chunkSize));
    }
    
    return (cpl_allocator_ref)poolAllocator;
}

void cpl_allocator_destroy_pool(cpl_allocator_ref allocator)
{
    assert(allocator != cpl_allocator_get_default());
    struct cpl_pool_allocator* pPoolAllocator = (struct cpl_pool_allocator *)allocator;
    int rc = munmap(pPoolAllocator->pool, pPoolAllocator->poolSize + sizeof(struct cpl_pool_allocator));
    assert(rc == 0);
}
