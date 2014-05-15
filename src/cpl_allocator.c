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
#include "cpl_list.h"

#ifndef MAP_ANONYMOUS
#   ifdef MAP_ANON
#       define MAP_ANONYMOUS MAP_ANON
#   endif
#endif

/********************** Default Allocator Implementation **********************/
struct cpl_allocator
{
    void* (*xAllocate)(struct cpl_allocator*, size_t);
    void  (*xFree)(struct cpl_allocator*, void* ptr);
};

static void* cpl_default_malloc(struct cpl_allocator* pAllocator, size_t sz)
{
    return malloc(sz);
}

static void cpl_default_free(struct cpl_allocator* pAllocator, void* ptr)
{
    free(ptr);
}

/*********************** Pool Allocator Implementation ************************/
struct cpl_pool_allocator
{
    void* (*xAllocate)(struct cpl_allocator*, size_t);
    void  (*xFree)(struct cpl_allocator*, void* ptr);
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

/********************* Doug Lea's Allocator routines  *************************/

/* flags */
#define DL_PINUSE_BIT           ((size_t)0x1)
#define DL_CINUSE_BIT           ((size_t)0x2)
#define DL_RESERV_BIT           ((size_t)0x4)
#define DL_INUSE_BITS           (DL_CINUSE_BIT | DL_PINUSE_BIT)
#define DL_FLAGS_MASK           (DL_INUSE_BITS | DL_RESERV_BIT)

/* calculation helpers */
#define dl_chunk_plus_offset(c, o) ((dl_chunk *)((char *)(c) + (o)))

/* flag getters */
#define dl_pinuse(c)            ((c)->head & DL_PINUSE_BIT)
#define dl_cinuse(c)            ((c)->head & DL_CINUSE_BIT)

/* flag setters */
#define set_pinuse(c)           ((c)->head |= DL_PINUSE_BIT)
#define clear_pinuse(c)         ((c)->head &= ~DL_PINUSE_BIT)
#define set_inuse(c, s)         ((c)->head = dl_pinuse(c) | DL_CINUSE_BIT | (s)), \
                                set_pinuse(dl_chunk_plus_offset(c, s))

/* chunk sizes */
#define DL_CHUNK_SIZE           (sizeof(struct dl_chunk))
#define DL_CHUNK_OVERHEAD       (2 * sizeof(size_t))

#define dl_pad_request(s)       (((s) + DL_CHUNK_OVERHEAD + DL_FLAGS_MASK) & ~DL_FLAGS_MASK)
#define request2size(s)         ((s) < DL_CHUNK_SIZE ? DL_CHUNK_SIZE : dl_pad_request(s))

/* header <-> body */
#define dl_size(c)              ((c)->head & ~(DL_FLAGS_MASK))
#define chunk2ptr(c)            ((void*)&((c)->list))
#define ptr2chunk(ptr)          ((dl_chunk*)((char*)(ptr) - offsetof(dl_chunk, list)))

struct cpl_dl_allocator
{
    void*       (*xAllocate)(struct cpl_allocator*, size_t);
    void        (*xFree)(struct cpl_allocator*, void* ptr);
    /* The Heap */
    void*       start_addr;
    void*       end_addr;
    void*       max_addr;
    cpl_dlist_t head;
};

struct dl_chunk
{
    size_t      prev_foot;
    size_t      head;
    cpl_dlist_t list;
};
typedef struct dl_chunk dl_chunk;

static dl_chunk* dl_find_smallest_chunk(struct cpl_dl_allocator* dl_allocator, size_t sz)
{
    dl_chunk* tmp = 0;
    struct cpl_dlist* iter;
    cpl_dlist_foreach(iter, &(dl_allocator->head))
    {
        tmp = cpl_dlist_entry(iter, dl_chunk, list);
        if(dl_size(tmp) >= sz)
        {
            break;
        }
    }
    
    if(iter == &(dl_allocator->head))
    {
        tmp = 0;
    }
    
    return tmp;
}

static void dl_insert_chunk(struct cpl_dl_allocator* dl_allocator, dl_chunk* chunk)
{
    struct cpl_dlist* iter = &(dl_allocator->head);
    if(!cpl_dlist_empty(iter))
    {
        size_t size = dl_size(chunk);
        cpl_dlist_foreach(iter, &(dl_allocator->head))
        {
            dl_chunk* tmp = cpl_dlist_entry(iter, dl_chunk, list);
            if(dl_size(tmp) >= size)
            {
                break;
            }
        }
    }
    cpl_dlist_add_tail(&(chunk->list), iter);
}

static void dl_remove_chunk(dl_chunk* chunk)
{
    cpl_dlist_del(&(chunk->list));
}

static void* cpl_dl_malloc(struct cpl_allocator* allocator, size_t sz)
{
    struct cpl_dl_allocator* dl_allocator = (struct cpl_dl_allocator *)allocator;
    
    size_t chunksize = request2size(sz);
    dl_chunk *hole = dl_find_smallest_chunk(dl_allocator, chunksize);
    if(!hole)
    {
        // TODO:
    }
    assert( dl_size(hole) >= chunksize);
    
    dl_remove_chunk(hole);
    if(dl_size(hole) >= chunksize + DL_CHUNK_SIZE)
    {
        size_t new_size = dl_size(hole) - chunksize;
        dl_chunk* new_chunk = dl_chunk_plus_offset(hole, chunksize);
        new_chunk->head = new_size;
        dl_insert_chunk(dl_allocator, new_chunk);
    }
    else
    {
        chunksize = dl_size(hole);
    }
    
    set_inuse(hole, chunksize);
    
    return chunk2ptr(hole);
    
_Lfailure:
    return 0;
}

static void cpl_dl_allocator_init(struct cpl_dl_allocator* dl_allocator, char* addr, size_t max_size)
{
    /* calculate initial size of the heap */
    size_t init_size = (max_size >= 0x10000)?0x10000:max_size;
    
    /* setup allocator */
    dl_allocator->start_addr = addr;
    dl_allocator->end_addr = addr + init_size;
    dl_allocator->max_addr = addr + max_size;
    
    /* setup initial chunk */
    init_size -= sizeof(struct cpl_dl_allocator);
    dl_chunk* chunk = (dl_chunk*)&(dl_allocator->head);
    chunk->head = init_size | DL_PINUSE_BIT;
    size_t* footer = (size_t*)((char*)dl_allocator->end_addr - sizeof(size_t));
    *footer = init_size;
    
    /* setup linked list of chunks */
    dl_allocator->head.next = (struct cpl_dlist *)&(chunk->list);
    chunk->list.prev = chunk->list.next = &(dl_allocator->head);
}

/*********************** Public Allocator routines  ***************************/
cpl_allocator_ref cpl_allocator_get_default()
{
    static struct cpl_allocator _default_allocator = { cpl_default_malloc, cpl_default_free };
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

cpl_allocator_ref cpl_allocator_create_pool(size_t chunkSize, int nChunks)
{
    assert(chunkSize < 8192 && chunkSize > 32);
    
    size_t poolSize = chunkSize * nChunks;
    void* poolBuffer = mmap(0, poolSize + sizeof(struct cpl_pool_allocator), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    
    if(poolBuffer == MAP_FAILED)
    {
        return 0;
    }
    
    struct cpl_pool_allocator* poolAllocator = (struct cpl_pool_allocator*)((char*)poolBuffer + poolSize);
    
    poolAllocator->xAllocate = cpl_pool_malloc;
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

cpl_allocator_ref cpl_allocator_create_dl(size_t max_size)
{
    /* align to page size */
    if(max_size & 0xFFF)
    {
        max_size &= 0x1000;
        max_size += 0x1000;
    }
    
    /* reserve pages */
    char* addr = mmap(0, max_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(addr == MAP_FAILED)
    {
        /* failed to map */
        return 0;
    }
    
    /* place allocator struct right after the heap and setup fields */
    struct cpl_dl_allocator* dl_allocator = (struct cpl_dl_allocator *)addr;
    cpl_dl_allocator_init(dl_allocator, addr, max_size);
    
    return (cpl_allocator_ref)dl_allocator;
}

void cpl_allocator_destroy_dl(cpl_allocator_ref allocator)
{
    assert(allocator != cpl_allocator_get_default());
    struct cpl_dl_allocator* dl_allocator = (struct cpl_dl_allocator *)allocator;
    size_t max_size = (char*)dl_allocator->max_addr - (char*)dl_allocator->start_addr;
    int rc = munmap(dl_allocator->start_addr, max_size);
    assert(rc == 0);
}
