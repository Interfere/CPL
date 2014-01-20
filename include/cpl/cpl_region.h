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
 * C Primitives Library. Region of RAM (aka memory buffer) implementation.
 */

#ifndef _CPL_REGION_H_
#define _CPL_REGION_H_

#include <stdlib.h>

typedef struct cpl_region cpl_region_t;
typedef struct cpl_region* cpl_region_ref;
struct cpl_region
{
    size_t      alloc;
    size_t      offset;
    void        *data;
};

cpl_region_ref cpl_region_create(size_t sz);
int cpl_region_init(cpl_region_ref __restrict r, size_t sz);

#define cpl_region_create_default() cpl_region_create(0)

#define cpl_region_deinit(r)        free((r)->data)
#define cpl_region_destroy(r)       free((r)->data);free(r)

int cpl_region_append_data(cpl_region_ref __restrict r, const void* __restrict data, size_t sz);
#define cpl_region_append_region(r, o) cpl_region_append_data(r, (o)->data, (o)->offset)

int cpl_region_resize(cpl_region_ref __restrict r, size_t sz);

#endif // _CPL_REGION_H_
