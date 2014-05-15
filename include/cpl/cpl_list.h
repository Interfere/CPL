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
    struct cpl_slist *next;
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

struct cpl_dlist
{
    struct cpl_dlist *next, *prev;
};
typedef struct cpl_dlist cpl_dlist_t;
typedef struct cpl_dlist* cpl_dlist_ref;

/**
 * check whether the list is empty
 */
static inline int cpl_dlist_empty(struct cpl_dlist* head)
{
    return head->next == head;
}

static inline void __cpl_dlist_add(struct cpl_dlist* item,
                                   struct cpl_dlist* prev,
                                   struct cpl_dlist* next)
{
    next->prev = item;
    item->next = next;
    item->prev = prev;
    prev->next = item;
}

static inline void cpl_dlist_add_tail(struct cpl_dlist *item, struct cpl_dlist* next)
{
    __cpl_dlist_add(item, next->prev, next);
}

static inline void cpl_dlist_del(struct cpl_dlist* item)
{
    item->prev->next = item->next;
    item->next->prev = item->prev;
}

/**
 * get the struct for the entry
 */
#define cpl_dlist_entry(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

/**
 * iterate over the list
 */
#define cpl_dlist_foreach(pos, head) \
    for(pos = (head)->next; pos != (head); pos = pos->next)

#endif // _CPL_LIST_H_
