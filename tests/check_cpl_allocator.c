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
#include <check.h>
#include "../include/cpl/cpl_allocator.h"

START_TEST(test_cpl_allocator_get_default)
{
    cpl_allocator_ref a;
    a = cpl_allocator_get_default();
    ck_assert_ptr_ne(a, 0);
    ck_assert_ptr_eq(a, cpl_allocator_get_default());
}
END_TEST

static Suite* cpl_allocator_suit(void)
{
    Suite* s = suite_create("Allocator");
    
    /* Default Allocator test case */
    TCase* tc_def = tcase_create("Default Allocator");
    tcase_add_test(tc_def, test_cpl_allocator_get_default);
    
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
