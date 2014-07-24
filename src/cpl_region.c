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

#include "cpl_region.h"

#include <assert.h>
#include <math.h>
#include <string.h>

#include "cpl_error.h"

static size_t _cpl_p2(size_t in)
{
    in |= (in >> 1);
    in |= (in >> 2);
    in |= (in >> 4);
    in |= (in >> 8);
    in |= (in >> 16);
    in -= (in >> 1);
    return in;
}

static inline int _cpl_resize_up(cpl_region_ref __restrict r, size_t sz)
{
    size_t alloc = _cpl_p2(sz);
    void *ptr = realloc(r->data, alloc);
    if(!ptr)
    {
        return _CPL_NOMEM;
    }
    r->data = ptr;
    r->alloc = alloc;
    return _CPL_OK;
}

cpl_region_ref cpl_region_create(cpl_allocator_ref allocator, size_t sz)
{
    cpl_region_ref r = (cpl_region_ref)cpl_allocator_allocate(allocator, sizeof(cpl_region_t));
    if(r)
    {
        if(cpl_region_init(allocator, r, sz))
        {
            cpl_allocator_free(allocator, r);
            r = 0;
        }
    }
    return r;
}

int cpl_region_init(cpl_allocator_ref allocator, cpl_region_ref __restrict r, size_t sz)
{
    assert(r);
    if(sz > 64)
    {
        _cpl_p2(sz);
    }
    else
    {
        sz = 64;
    }
    
    r->allocator = allocator;
    r->alloc = sz;
    r->offset = 0;
    r->data = cpl_allocator_allocate(allocator, sz);
    if(!r->data)
    {
        return _CPL_NOMEM;
    }
    
    return _CPL_OK;
}

int cpl_region_append_data(cpl_region_ref __restrict r, const void* __restrict data, size_t sz)
{
    assert(r);
    assert(data && sz);
    
    size_t new_offset = r->offset + sz;
    size_t alloc = r->alloc;
    while(new_offset > alloc)
    {
        alloc *= 2;
    }
    
    if(alloc > r->alloc)
    {
        int res = _cpl_resize_up(r, alloc);
        if(res)
        {
            return res;
        }
    }
    
    memcpy((char*)r->data + r->offset, data, sz);
    r->offset += sz;
    
    return _CPL_OK;
}

int cpl_region_resize(cpl_region_ref __restrict r, size_t sz)
{
    size_t alloc = _cpl_p2(sz);
    void *ptr = realloc(r->data, alloc);
    if(!ptr)
    {
        return _CPL_NOMEM;
    }
    r->data = ptr;
    r->alloc = alloc;
    r->offset = (r->offset > alloc)?alloc:r->offset;
    return _CPL_OK;
}
