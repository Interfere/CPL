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

#include "cpl_list.h"

#include <assert.h>

void cpl_slist_add(cpl_slist_ref __restrict list, cpl_slist_ref __restrict entry)
{
    assert(list);
    assert(entry);
    
    entry->next = list->next;
    list->next = entry;
}

cpl_slist_ref cpl_slist_pop(cpl_slist_ref __restrict list)
{
    assert(list);
    
    cpl_slist_ref first = list->next;
    if(first)
    {
        list->next = first->next;
    }
    return first;
}

void cpl_slist_del_first(cpl_slist_ref __restrict list)
{
    assert(list);
    
    if(list->next)
    {
        list->next = list->next->next;
    }
}