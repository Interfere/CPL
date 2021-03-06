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

/********************* Doug Lea's Allocator routines  *************************/

/* flags */
#define DL_PINUSE_BIT           ((size_t)0x1)
#define DL_CINUSE_BIT           ((size_t)0x2)
#define DL_RESERV_BIT           ((size_t)0x4)
#define DL_INUSE_BITS           (DL_CINUSE_BIT | DL_PINUSE_BIT)
#define DL_FLAGS_MASK           (DL_INUSE_BITS | DL_RESERV_BIT)

/* calculation helpers */
#define dl_chunk_plus_offset(c, o)  ((dl_chunk *)((char *)(c) + (o)))
#define dl_chunk_minus_offset(c, o) ((dl_chunk *)((char *)(c) - (o)))

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
#define DL_CHUNK_OVERHEAD       (sizeof(size_t))

#define dl_pad_request(s)       (((s) + DL_CHUNK_OVERHEAD + DL_FLAGS_MASK) & ~DL_FLAGS_MASK)
#define request2size(s)         ((s) < DL_CHUNK_SIZE ? DL_CHUNK_SIZE : dl_pad_request(s))
#define ok_address(c, a)        (((size_t)(c) >= (size_t)(a)->start_addr) && \
((size_t)(c) < (size_t)(a)->end_addr))

/* header <-> body */
#define dl_size(c)              ((c)->head & ~(DL_FLAGS_MASK))
#define chunk2ptr(c)            ((void*)&((c)->list))
#define ptr2chunk(ptr)          ((dl_chunk*)((char*)(ptr) - offsetof(dl_chunk, list)))

struct cpl_dl_allocator
{
    void*       (*xAllocate)(struct cpl_allocator*, size_t);
    void*       (*xRealloc)(struct cpl_allocator*, void* ptr, size_t);
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

static int cpl_dl_expand(struct cpl_dl_allocator* dl_allocator, size_t new_size)
{
    assert(new_size > (size_t)dl_allocator->end_addr - (size_t)dl_allocator->start_addr);
    size_t off = (size_t)dl_allocator->start_addr - (size_t)dl_allocator;
    new_size = ((new_size + off + 0xfff) & ~(0xfff)) - off;
    void* new_end_addr = (char *)dl_allocator->start_addr + new_size;
    if(new_end_addr > dl_allocator->max_addr)
    {
        return 0;
    }
    
    dl_allocator->end_addr = new_end_addr;
    
    return 1;
}

static void* cpl_dl_malloc(struct cpl_allocator* allocator, size_t sz)
{
    struct cpl_dl_allocator* dl_allocator = (struct cpl_dl_allocator *)allocator;
    
    size_t chunksize = request2size(sz);
    dl_chunk *hole = dl_find_smallest_chunk(dl_allocator, chunksize);
    if(!hole)
    {
        /* if we didn't find suitable hole, expand the heap */
        size_t old_size = (size_t)dl_allocator->end_addr - (size_t)dl_allocator->start_addr;
        void* old_end_addr = dl_allocator->end_addr;
        int success = cpl_dl_expand(dl_allocator, old_size + chunksize);
        if(!success)
        {
            return 0;
        }
        
        size_t new_size = (size_t)dl_allocator->end_addr - (size_t)dl_allocator->start_addr;
        
        /* Find endmost header */
        struct cpl_dlist* tmp = 0;
        if(!cpl_dlist_empty(&(dl_allocator->head)))
        {
            struct cpl_dlist* iter;
            cpl_dlist_foreach(iter, &(dl_allocator->head))
            {
                if(iter > tmp)
                {
                    tmp = iter;
                }
            }
        }
        
        dl_chunk* topchunk = cpl_dlist_entry(tmp, dl_chunk, list);
        if(tmp && dl_chunk_plus_offset(topchunk, dl_size(topchunk)) == old_end_addr)
        {
            dl_remove_chunk(topchunk);
            topchunk->head += new_size - old_size;
        }
        else
        {
            topchunk = (dl_chunk *)old_end_addr;
            topchunk->head = DL_PINUSE_BIT | (new_size - old_size);
        }
        
        dl_insert_chunk(dl_allocator, topchunk);
        return cpl_dl_malloc(allocator, sz);
    }
    assert( dl_size(hole) >= chunksize );
    
    dl_remove_chunk(hole);
    
    size_t new_size = dl_size(hole) - chunksize;
    if(new_size < DL_CHUNK_SIZE)
    {
        chunksize = dl_size(hole);
        dl_chunk* next_chunk = dl_chunk_plus_offset(hole, chunksize);
        if(next_chunk != dl_allocator->end_addr)
        {
            next_chunk->head |= DL_PINUSE_BIT;
        }
    }
    else
    {
        dl_chunk* next_chunk = dl_chunk_plus_offset(hole, chunksize);
        if(next_chunk != dl_allocator->end_addr)
        {
            next_chunk->head = new_size | DL_PINUSE_BIT;
            dl_insert_chunk(dl_allocator, next_chunk);
        }
    }
    
    hole->head = (hole->head & DL_PINUSE_BIT) | DL_CINUSE_BIT | chunksize;
    
    return chunk2ptr(hole);
}

static void cpl_dl_free(struct cpl_allocator* allocator, void* ptr)
{
    struct cpl_dl_allocator* dl_allocator = (struct cpl_dl_allocator *)allocator;
    if(ptr)
    {
        if(!ok_address(ptr, dl_allocator))
        {
            goto Lassert;
        }
        
        // Find corresponding chunk
        dl_chunk *chunk = ptr2chunk(ptr);
        
        if((chunk->head & DL_INUSE_BITS) == DL_PINUSE_BIT)
        {
            goto Lassert;
        }
        
        size_t chunk_size = dl_size(chunk);
        
        // Check previous chunk. If inuse bit is not set,
        // then we will unify both free chunks
        if(!dl_pinuse(chunk))
        {
            size_t left_size = chunk->prev_foot;
            dl_chunk *left_chunk = dl_chunk_minus_offset(chunk, left_size);
            
            if(!ok_address(left_chunk, dl_allocator))
            {
                goto Lassert;
            }
            
            chunk_size += left_size;
            chunk = left_chunk;
            
            // Remove chunk from ordered list
            dl_remove_chunk(chunk);
        }
        
        // Find addres of the next chunk
        dl_chunk *right_chunk = dl_chunk_plus_offset(chunk, chunk_size);
        if(ok_address(right_chunk, dl_allocator))
        {
            if(!dl_pinuse(right_chunk))
            {
                goto Lassert;
            }
            
            // check whether right chunk is inuse
            if(dl_cinuse(right_chunk))
            {
                // Clear PINUSE
                clear_pinuse(right_chunk);
            }
            else
            {
                size_t right_size = dl_size(right_chunk);
                
                assert((size_t)dl_chunk_plus_offset(right_chunk, right_size) <= (size_t)dl_allocator->end_addr);
                
                dl_remove_chunk(right_chunk);
                chunk_size += right_size;
            }
        }
        
        if((right_chunk = dl_chunk_plus_offset(chunk, chunk_size)) != dl_allocator->end_addr)
        {
            right_chunk->prev_foot = chunk_size;
        }
        chunk->head = chunk_size | DL_PINUSE_BIT;
        
        dl_insert_chunk(dl_allocator, chunk);
    }
    
    return ;
    
Lassert:
    assert(0);
}

static inline void* cpl_dl_dummy_realloc(struct cpl_allocator* allocator, void* ptr, size_t old_sz, size_t new_sz)
{
    void* mem = cpl_dl_malloc(allocator, new_sz);
    if(mem)
    {
        memcpy(mem, ptr, old_sz);
    }
    cpl_dl_free(allocator, ptr);
    return mem;
}

static void* cpl_dl_realloc(struct cpl_allocator* allocator, void* ptr, size_t sz)
{
    struct cpl_dl_allocator* dl_allocator = (struct cpl_dl_allocator *)allocator;
    if(!ptr)
    {
        return cpl_dl_malloc(allocator, sz);
    }
    
    if(!ok_address(ptr, dl_allocator))
    {
        goto Lassert;
    }
    
    // Find corresponding chunk
    dl_chunk *chunk = ptr2chunk(ptr);
    
    if((chunk->head & DL_INUSE_BITS) == DL_PINUSE_BIT)
    {
        goto Lassert;
    }
    
    size_t curr_size = dl_size(chunk);
    size_t new_size = request2size(sz);
    
    // Find address of next chunk
    dl_chunk *right_chunk = dl_chunk_plus_offset(chunk, curr_size);
    if(new_size <= curr_size) /* already big enough */
    {
        size_t rsize = curr_size - new_size;
        if(rsize < DL_CHUNK_SIZE)
        {
            return ptr;
        }
        else
        {
            if(right_chunk != dl_allocator->end_addr)
            {
                if(!dl_pinuse(right_chunk))
                {
                    goto Lassert;
                }
                
                // check whether right chunk is inuse
                if(dl_cinuse(right_chunk))
                {
                    // Clear PINUSE
                    clear_pinuse(right_chunk);
                }
                else
                {
                    size_t right_size = dl_size(right_chunk);
                    dl_chunk* next_chunk = dl_chunk_plus_offset(right_chunk, right_size);
                    
                    assert((size_t)next_chunk <= (size_t)dl_allocator->end_addr);
                    
                    dl_remove_chunk(right_chunk);
                    rsize += right_size;
                    
                    if(next_chunk != dl_allocator->end_addr)
                    {
                        next_chunk->prev_foot = rsize;
                    }
                }
            }
            dl_chunk *top_chunk = dl_chunk_plus_offset(chunk, new_size);
            top_chunk->head = rsize | DL_PINUSE_BIT;
            dl_insert_chunk(dl_allocator, top_chunk);
            
            chunk->head = (chunk->head & DL_PINUSE_BIT) | DL_CINUSE_BIT | new_size;
            return chunk2ptr(chunk);
        }
    }
    else if(right_chunk == dl_allocator->end_addr) /* No blocks after chunk */
    {
        size_t rsize = new_size - curr_size;
        size_t old_size = (size_t)dl_allocator->end_addr - (size_t)dl_allocator->start_addr;
        int success = cpl_dl_expand(dl_allocator, old_size + rsize);
        if(!success)
        {
            return 0;
        }
        
        size_t top_size = (size_t)dl_allocator->end_addr - (size_t)dl_allocator->start_addr - old_size - rsize;
        
        dl_chunk* top_chunk = dl_chunk_plus_offset(chunk, new_size);
        top_chunk->head = top_size | DL_PINUSE_BIT;
        dl_insert_chunk(dl_allocator, top_chunk);
        
        chunk->head = (chunk->head & DL_PINUSE_BIT) | DL_CINUSE_BIT | new_size;
        return chunk2ptr(chunk);
    }
    else if(dl_cinuse(right_chunk))
    {
        return cpl_dl_dummy_realloc(allocator, ptr, curr_size - DL_CHUNK_OVERHEAD, sz);
    }
    else
    {
        assert(!dl_cinuse(right_chunk));
        
        if(!dl_pinuse(right_chunk))
        {
            goto Lassert;
        }
        
        size_t right_size = dl_size(right_chunk);
        size_t rsize = new_size - curr_size;
        dl_chunk* top_chunk = dl_chunk_plus_offset(right_chunk, right_size);
        if(right_size < rsize)
        {
            if(top_chunk != dl_allocator->end_addr)
            {
                assert(dl_cinuse(top_chunk));
                return cpl_dl_dummy_realloc(allocator, ptr, curr_size - DL_CHUNK_OVERHEAD, sz);
            }
            else
            {
                dl_remove_chunk(right_chunk);
                chunk->head += right_size;
                return cpl_dl_realloc(allocator, ptr, sz);
            }
        }
        else if(right_size > rsize + DL_CHUNK_SIZE)
        {
            dl_chunk* next_chunk = dl_chunk_plus_offset(right_chunk, rsize);
            dl_remove_chunk(right_chunk);
            next_chunk->head = (right_size - rsize) | DL_PINUSE_BIT;
            dl_insert_chunk(dl_allocator, next_chunk);
            
            if(top_chunk != dl_allocator->end_addr)
            {
                top_chunk->prev_foot = (right_size - rsize);
            }
            
            chunk->head = (chunk->head & DL_PINUSE_BIT) | DL_CINUSE_BIT | new_size;
            return chunk2ptr(chunk);
        }
        else
        {
            dl_remove_chunk(right_chunk);
            
            if(top_chunk != dl_allocator->end_addr)
            {
                top_chunk->head |= DL_PINUSE_BIT;
            }
            
            chunk->head = (chunk->head & DL_PINUSE_BIT) | DL_CINUSE_BIT | new_size;
            return chunk2ptr(chunk);
        }
    }
    
Lassert:
    assert(0);
    return 0;
}

static void cpl_dl_allocator_init(struct cpl_dl_allocator* dl_allocator, char* addr, size_t max_size)
{
    /* assign corresponding routines */
    dl_allocator->xAllocate = cpl_dl_malloc;
    dl_allocator->xFree = cpl_dl_free;
    dl_allocator->xRealloc = cpl_dl_realloc;
    
    /* calculate initial size of the heap */
    size_t init_size = (max_size >= 0x10000)?0x10000:max_size;
    
    /* setup allocator */
    size_t off = (sizeof(struct cpl_dl_allocator) - sizeof(size_t) + DL_FLAGS_MASK) & ~(DL_FLAGS_MASK);
    dl_allocator->start_addr = addr + off;
    dl_allocator->end_addr = addr + init_size;
    dl_allocator->max_addr = addr + max_size;
    
    /* setup initial chunk */
    init_size -= off;
    dl_chunk* chunk = (dl_chunk*)dl_allocator->start_addr;
    chunk->head = init_size | DL_PINUSE_BIT;
    
    /* setup linked list of chunks */
    dl_allocator->head.prev = dl_allocator->head.next = (struct cpl_dlist *)&(chunk->list);
    chunk->list.prev = chunk->list.next = &(dl_allocator->head);
}

/********************* Public DL Allocator routines  **************************/
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
    size_t max_size = (size_t)dl_allocator->max_addr - (size_t)dl_allocator;
    int rc = munmap(dl_allocator, max_size);
    assert(rc == 0);
}
