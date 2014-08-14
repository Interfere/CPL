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
 * Tests for C Primitives Library. Allocator.
 */

#include <stdlib.h>
#include <stdio.h>
#include <check.h>
#include "../include/cpl/cpl_allocator.h"

#define SMALLSIZE   72
#define MEDIUMSIZE  896
#define BIGSIZE     16384
#define HUGESIZE    ((size_t)-1)

/****************************** Usefule Routines ******************************/
static void markblock(volatile void *ptr, size_t size, unsigned bias, int doprint)
{
    size_t n, i;
    unsigned long *pl;
    unsigned long val;
    
    pl = (unsigned long *)ptr;
    n = size / sizeof(unsigned long);
    
    for (i = 0; i < n; i++)
    {
        val = (unsigned long)i ^ (unsigned long)bias;
        pl[i] = val;
        if(doprint && (i%64 == 63))
        {
            printf(".");
        }
    }
    if(doprint)
    {
        printf("\n");
    }
}

static int checkblock(volatile void *ptr, size_t size, unsigned bias, int doprint)
{
    size_t n, i;
    unsigned long *pl;
    unsigned long val;
    
    pl = (unsigned long *)ptr;
    n = size / sizeof(unsigned long);
    
    for (i = 0; i < n; i++)
    {
        val = (unsigned long)i ^ (unsigned long)bias;
        if(pl[i] != val)
        {
            ck_assert_msg(0, "data mismatch at offset %lu of block at %p: %lu vs. %lu",
                          (unsigned long)(i * sizeof(unsigned long)), pl, pl[i], val);
            return 0;
        }
        if(doprint && (i%64 == 63))
        {
            printf(".");
        }
    }
    if(doprint)
    {
        printf("\n");
    }
    return 1;
}

/************************************ Tests ***********************************/
START_TEST(test_cpl_allocator_get_default)
{
    cpl_allocator_ref a;
    a = cpl_allocator_get_default();
    ck_assert_ptr_ne(a, 0);
    ck_assert_ptr_eq(a, cpl_allocator_get_default());
}
END_TEST

START_TEST(test_default_allocator_test1)
{
    volatile void *x;
    
    x = malloc(BIGSIZE);
    ck_assert_ptr_ne((void *)x, 0);
    
    markblock(x, BIGSIZE, 0, 0);
    ck_assert(checkblock(x, BIGSIZE, 0, 0));
    
    free((void *)x);
}
END_TEST

START_TEST(test_default_allocator_test2)
{
    volatile void *x;
    size_t size;
    
    for(size = HUGESIZE; (x = malloc(size)) == 0; size = size/2)
    {
        printf("%9lu bytes failed\n", (unsigned long)size);
    }
    printf("%9lu bytes succeeded\n", (unsigned long)size);
    
    markblock(x, size, 0, 1);
    ck_assert(checkblock(x, size, 0, 1));
    
    free((void*)x);
    
    x = malloc(size);
    ck_assert_ptr_ne((void *)x, 0);
    free((void*)x);
}
END_TEST


/************************************ Suits ***********************************/
static Suite* cpl_allocator_suit(void)
{
    Suite* s = suite_create("Allocator");
    
    /* Default Allocator test case */
    TCase* tc_def = tcase_create("Default Allocator");
    tcase_set_timeout(tc_def, 4000);
    
    tcase_add_test(tc_def, test_cpl_allocator_get_default);
    tcase_add_test(tc_def, test_default_allocator_test1);
    tcase_add_test(tc_def, test_default_allocator_test2);
    
    suite_add_tcase(s, tc_def);
    
    return s;
}

int main()
{
    int nfailed = 0;
    
    Suite* s = cpl_allocator_suit();
    SRunner* sr = srunner_create(s);
    
    srunner_run_all(sr, CK_NORMAL);
    nfailed = srunner_ntests_failed(sr);
    
    srunner_free(sr);
    
    return (nfailed == 0)?EXIT_SUCCESS:EXIT_FAILURE;
}
