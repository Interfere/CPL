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
 * C Primitives Library. Array (aka vector) implementation.
 */

#ifndef _CPL_ARRAY_H_
#define _CPL_ARRAY_H_

#include <stdlib.h>
#include <string.h>
#include <cpl/cpl_region.h>

#define _CPL_DEFAULT_ARRAY_SIZE     64

struct cpl_array
{
    size_t          szelem;     /* size of an element */
    size_t          count;      /* count of elements */
    cpl_region_t    region;
};
typedef struct cpl_array cpl_array_t;
typedef struct cpl_array* cpl_array_ref;

/*
 * Creates an empty array with space reserved for _nreserv_ items of size _sz_.
 */
cpl_array_ref cpl_array_create(size_t sz, size_t nreserv);

/*
 * Creates an empty array for items sizeof _sz_
 */
#define cpl_array_create_sz(sz)         cpl_array_create(sz, _CPL_DEFAULT_ARRAY_SIZE)

/*
 * Initialize stack-allocated array with _nreserv_ items of size _sz_.
 */
int cpl_array_init(cpl_array_ref a, size_t sz, size_t nreserv);

/*
 * Deinitialize stack-allocated array.
 */
#define cpl_array_deinit(a)             cpl_region_deinit(&(a)->region)

/*
 * Copies an array.
 */
cpl_array_ref cpl_array_copy(cpl_array_ref __restrict o);

/*
 * Destroys an array.
 */
void cpl_array_destroy(cpl_array_ref __restrict a);

/*
 * Count of elements in array
 */
#define cpl_array_count(a)              ((a)->count)

/*
 * Clears content of an array
 */
void cpl_array_clear(cpl_array_ref __restrict a);

/*
 * Get raw pointer of elements.
 */
#define cpl_array_data(a, type)         (type *)((a)->region.data)

/*
 * Get access of an element.
 */
void* cpl_array_get_p(cpl_array_ref __restrict a, size_t i);
#define cpl_array_get(a, i, type)       (*(type*)cpl_array_get_p(a, i))

/*
 * Add an element to the end of an array
 */
int cpl_array_push_back_p(cpl_array_ref a, void* p, size_t sz);
#define cpl_array_push_back(a, v)       cpl_array_push_back_p(a, &(v), sizeof(v))

/*
 * Delete last element.
 */
#define cpl_array_pop_back(a)           ((a)->count-=1)

/*
 * Access last element.
 */
void* cpl_array_back_p(cpl_array_ref __restrict a);
#define cpl_array_back(a, type)         (*(type*)cpl_array_back_p(a))

/*
 * Change size of an array.
 */
int cpl_array_resize(cpl_array_ref __restrict a, size_t sz);

#endif // _CPL_ARRAY_H_
