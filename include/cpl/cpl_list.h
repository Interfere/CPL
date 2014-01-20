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
 * C Primitives Library. Linked list implementation.
 */

#ifndef _CPL_LIST_H_
#define _CPL_LIST_H_

struct cpl_slist
{
    struct cpl_slist *next, *prev;
};
typedef struct cpl_slist cpl_slist_t;
typedef struct cpl_slist* cpl_slist_ref;

/**
 * Initialize empty list.
 */
#define CPL_SLIST_INIT(list)        ((list).next = 0)

/**
 * Add new entry in the list.
 */
void cpl_slist_add(cpl_slist_ref __restrict list, cpl_slist_ref __restrict entry);

/**
 * Remove and return first entry in the list.
 */
cpl_slist_ref cpl_slist_pop(cpl_slist_ref __restrict list);

/**
 * Remove first entry in the list.
 */
void cpl_slist_del_first(cpl_slist_ref __restrict list);

#endif // _CPL_LIST_H_
