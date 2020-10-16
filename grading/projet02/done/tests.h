#pragma once

/**
 * @file tests.h
 * @brief Utilities for tests
 *
 * @date 2017
 */

#include <stdlib.h> // EXIT_FAILURE
#include <check.h>

#include "error.h"

#define ck_assert_bad_param(value) \
    ck_assert_int_eq(value, ERR_BAD_PARAMETER)

#define ck_assert_err_mem(value) \
    ck_assert_int_eq(value, ERR_MEM)

#define ck_assert_err_none(value) \
    ck_assert_int_eq(value, ERR_NONE)

#ifndef ck_assert_ptr_nonnull
#define ck_assert_ptr_nonnull(ptr) \
    ck_assert_ptr_ne(ptr, NULL)
#endif

#ifndef ck_assert_ptr_null
#define ck_assert_ptr_null(ptr) \
    ck_assert_ptr_eq(ptr, NULL)
#endif

#define Add_Case(S, C, Title) \
    TCase* C = tcase_create(Title); \
    suite_add_tcase(S, C)

#define TEST_SUITE(get_suite) \
int main() \
{ \
    SRunner* sr = srunner_create(get_suite()); \
    srunner_run_all(sr, CK_VERBOSE); \
 \
    int number_failed = srunner_ntests_failed(sr); \
    srunner_free(sr); \
 \
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE; \
}
