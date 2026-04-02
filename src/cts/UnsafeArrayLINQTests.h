#pragma once

#include "UnsafeArrayLINQ.h"
#include "../tests.h"

// --- Helper predicates/comparators ---

static int _is_even(const void *elem) { return (*(const int *)elem) % 2 == 0; }
static int _less_than_5(const void *elem) { return *(const int *)elem < 5; }
static int _int_cmp(const void *a, const void *b) { return *(const int *)a - *(const int *)b; }
static void _double_it(const void *src, void *dst) { *(int *)dst = (*(const int *)src) * 2; }
static void _sum_acc(void *acc, const void *elem) { *(int *)acc += *(const int *)elem; }
static void _negate(void *elem) { *(int *)elem = -(*(int *)elem); }

static UnsafeArray *_make_ints(const int *vals, int n) {
    UnsafeArray *arr = UnsafeArray_Create(sizeof(int), (uint32_t)n);
    for (int i = 0; i < n; i++) UnsafeArray_Add(arr, &vals[i]);
    return arr;
}

// --- Tests ---

static void test_linq_where(void) {
    TEST("linq: Where filters elements");
    UnsafeArray *arr = _make_ints((int[]){1,2,3,4,5,6}, 6);
    UnsafeArray *evens = UnsafeArray_Where(arr, _is_even);
    ASSERT(evens->count == 3);
    ASSERT(UnsafeArray_GetDeref(evens, 0, int) == 2);
    ASSERT(UnsafeArray_GetDeref(evens, 1, int) == 4);
    ASSERT(UnsafeArray_GetDeref(evens, 2, int) == 6);
    UnsafeArray_Destroy(evens);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_linq_where_none(void) {
    TEST("linq: Where returns empty when no match");
    UnsafeArray *arr = _make_ints((int[]){1,3,5}, 3);
    UnsafeArray *evens = UnsafeArray_Where(arr, _is_even);
    ASSERT(evens->count == 0);
    UnsafeArray_Destroy(evens);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_linq_select(void) {
    TEST("linq: Select transforms elements");
    UnsafeArray *arr = _make_ints((int[]){1,2,3}, 3);
    UnsafeArray *doubled = UnsafeArray_Select(arr, _double_it, sizeof(int));
    ASSERT(doubled->count == 3);
    ASSERT(UnsafeArray_GetDeref(doubled, 0, int) == 2);
    ASSERT(UnsafeArray_GetDeref(doubled, 1, int) == 4);
    ASSERT(UnsafeArray_GetDeref(doubled, 2, int) == 6);
    UnsafeArray_Destroy(doubled);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_linq_first(void) {
    TEST("linq: First finds matching element");
    UnsafeArray *arr = _make_ints((int[]){1,3,4,6}, 4);
    int *r = (int *)UnsafeArray_First(arr, _is_even);
    ASSERT(r != NULL && *r == 4);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_linq_first_null(void) {
    TEST("linq: First returns NULL when no match");
    UnsafeArray *arr = _make_ints((int[]){1,3,5}, 3);
    ASSERT(UnsafeArray_First(arr, _is_even) == NULL);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_linq_last(void) {
    TEST("linq: Last finds last matching element");
    UnsafeArray *arr = _make_ints((int[]){2,3,4,5,6}, 5);
    int *r = (int *)UnsafeArray_Last(arr, _is_even);
    ASSERT(r != NULL && *r == 6);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_linq_indexof(void) {
    TEST("linq: IndexOf returns first match index");
    UnsafeArray *arr = _make_ints((int[]){1,3,5,4,2}, 5);
    ASSERT(UnsafeArray_IndexOf(arr, _is_even) == 3);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_linq_lastindexof(void) {
    TEST("linq: LastIndexOf returns last match index");
    UnsafeArray *arr = _make_ints((int[]){2,3,4,5,6}, 5);
    ASSERT(UnsafeArray_LastIndexOf(arr, _is_even) == 4);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_linq_any(void) {
    TEST("linq: Any true/false");
    UnsafeArray *arr = _make_ints((int[]){1,3,5}, 3);
    ASSERT(UnsafeArray_Any(arr, _is_even) == 0);
    UnsafeArray *arr2 = _make_ints((int[]){1,2,3}, 3);
    ASSERT(UnsafeArray_Any(arr2, _is_even) == 1);
    UnsafeArray_Destroy(arr);
    UnsafeArray_Destroy(arr2);
    PASS();
}

static void test_linq_all(void) {
    TEST("linq: All true/false");
    UnsafeArray *arr = _make_ints((int[]){2,4,6}, 3);
    ASSERT(UnsafeArray_All(arr, _is_even) == 1);
    UnsafeArray *arr2 = _make_ints((int[]){2,3,6}, 3);
    ASSERT(UnsafeArray_All(arr2, _is_even) == 0);
    UnsafeArray_Destroy(arr);
    UnsafeArray_Destroy(arr2);
    PASS();
}

static void test_linq_contains(void) {
    TEST("linq: Contains by memcmp");
    UnsafeArray *arr = _make_ints((int[]){10,20,30}, 3);
    int yes = 20, no = 25;
    ASSERT(UnsafeArray_Contains(arr, &yes) == 1);
    ASSERT(UnsafeArray_Contains(arr, &no) == 0);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_linq_count(void) {
    TEST("linq: Count matching elements");
    UnsafeArray *arr = _make_ints((int[]){1,2,3,4,5,6}, 6);
    ASSERT(UnsafeArray_Count(arr, _is_even) == 3);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_linq_aggregate(void) {
    TEST("linq: Aggregate sums all elements");
    UnsafeArray *arr = _make_ints((int[]){1,2,3,4,5}, 5);
    int sum = 0;
    UnsafeArray_Aggregate(arr, &sum, _sum_acc);
    ASSERT(sum == 15);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_linq_min_max(void) {
    TEST("linq: Min and Max");
    UnsafeArray *arr = _make_ints((int[]){5,1,3,9,2}, 5);
    int *mn = (int *)UnsafeArray_Min(arr, _int_cmp);
    int *mx = (int *)UnsafeArray_Max(arr, _int_cmp);
    ASSERT(mn != NULL && *mn == 1);
    ASSERT(mx != NULL && *mx == 9);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_linq_min_max_empty(void) {
    TEST("linq: Min/Max on empty array returns NULL");
    UnsafeArray *arr = UnsafeArray_Create(sizeof(int), 4);
    ASSERT(UnsafeArray_Min(arr, _int_cmp) == NULL);
    ASSERT(UnsafeArray_Max(arr, _int_cmp) == NULL);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_linq_foreach(void) {
    TEST("linq: ForEach mutates in place");
    UnsafeArray *arr = _make_ints((int[]){1,2,3}, 3);
    UnsafeArray_ForEach(arr, _negate);
    ASSERT(UnsafeArray_GetDeref(arr, 0, int) == -1);
    ASSERT(UnsafeArray_GetDeref(arr, 1, int) == -2);
    ASSERT(UnsafeArray_GetDeref(arr, 2, int) == -3);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_linq_removeall(void) {
    TEST("linq: RemoveAll removes matching, preserves order");
    UnsafeArray *arr = _make_ints((int[]){1,2,3,4,5,6}, 6);
    uint32_t removed = UnsafeArray_RemoveAll(arr, _is_even);
    ASSERT(removed == 3);
    ASSERT(arr->count == 3);
    ASSERT(UnsafeArray_GetDeref(arr, 0, int) == 1);
    ASSERT(UnsafeArray_GetDeref(arr, 1, int) == 3);
    ASSERT(UnsafeArray_GetDeref(arr, 2, int) == 5);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_linq_orderby(void) {
    TEST("linq: OrderBy sorts in place");
    UnsafeArray *arr = _make_ints((int[]){5,1,4,2,3}, 5);
    UnsafeArray_OrderBy(arr, _int_cmp);
    for (int i = 0; i < 5; i++) {
        ASSERT(UnsafeArray_GetDeref(arr, (uint32_t)i, int) == i + 1);
    }
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_linq_reverse(void) {
    TEST("linq: Reverse in place");
    UnsafeArray *arr = _make_ints((int[]){1,2,3,4,5}, 5);
    UnsafeArray_Reverse(arr);
    ASSERT(UnsafeArray_GetDeref(arr, 0, int) == 5);
    ASSERT(UnsafeArray_GetDeref(arr, 1, int) == 4);
    ASSERT(UnsafeArray_GetDeref(arr, 2, int) == 3);
    ASSERT(UnsafeArray_GetDeref(arr, 3, int) == 2);
    ASSERT(UnsafeArray_GetDeref(arr, 4, int) == 1);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_linq_distinct(void) {
    TEST("linq: Distinct removes duplicates");
    UnsafeArray *arr = _make_ints((int[]){1,2,2,3,1,3,4}, 7);
    uint32_t removed = UnsafeArray_Distinct(arr);
    ASSERT(removed == 3);
    ASSERT(arr->count == 4);
    ASSERT(UnsafeArray_GetDeref(arr, 0, int) == 1);
    ASSERT(UnsafeArray_GetDeref(arr, 1, int) == 2);
    ASSERT(UnsafeArray_GetDeref(arr, 2, int) == 3);
    ASSERT(UnsafeArray_GetDeref(arr, 3, int) == 4);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_linq_skip_take(void) {
    TEST("linq: Skip and Take");
    UnsafeArray *arr = _make_ints((int[]){10,20,30,40,50}, 5);
    UnsafeArray *skipped = UnsafeArray_Skip(arr, 2);
    ASSERT(skipped->count == 3);
    ASSERT(UnsafeArray_GetDeref(skipped, 0, int) == 30);
    UnsafeArray *taken = UnsafeArray_Take(arr, 3);
    ASSERT(taken->count == 3);
    ASSERT(UnsafeArray_GetDeref(taken, 2, int) == 30);
    UnsafeArray_Destroy(skipped);
    UnsafeArray_Destroy(taken);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_linq_skipwhile_takewhile(void) {
    TEST("linq: SkipWhile and TakeWhile");
    UnsafeArray *arr = _make_ints((int[]){1,2,3,4,5,6}, 6);
    UnsafeArray *sw = UnsafeArray_SkipWhile(arr, _less_than_5);
    ASSERT(sw->count == 2);
    ASSERT(UnsafeArray_GetDeref(sw, 0, int) == 5);
    ASSERT(UnsafeArray_GetDeref(sw, 1, int) == 6);
    UnsafeArray *tw = UnsafeArray_TakeWhile(arr, _less_than_5);
    ASSERT(tw->count == 4);
    ASSERT(UnsafeArray_GetDeref(tw, 3, int) == 4);
    UnsafeArray_Destroy(sw);
    UnsafeArray_Destroy(tw);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_linq_concat(void) {
    TEST("linq: Concat joins two arrays");
    UnsafeArray *a = _make_ints((int[]){1,2}, 2);
    UnsafeArray *b = _make_ints((int[]){3,4,5}, 3);
    UnsafeArray *c = UnsafeArray_Concat(a, b);
    ASSERT(c->count == 5);
    for (int i = 0; i < 5; i++) {
        ASSERT(UnsafeArray_GetDeref(c, (uint32_t)i, int) == i + 1);
    }
    UnsafeArray_Destroy(a);
    UnsafeArray_Destroy(b);
    UnsafeArray_Destroy(c);
    PASS();
}

static void test_linq_chained(void) {
    TEST("linq: chained Where -> OrderBy -> Take");
    UnsafeArray *arr = _make_ints((int[]){9,2,7,4,1,8,3,6,5}, 9);
    UnsafeArray *evens = UnsafeArray_Where(arr, _is_even);
    UnsafeArray_OrderBy(evens, _int_cmp);
    UnsafeArray *top2 = UnsafeArray_Take(evens, 2);
    ASSERT(top2->count == 2);
    ASSERT(UnsafeArray_GetDeref(top2, 0, int) == 2);
    ASSERT(UnsafeArray_GetDeref(top2, 1, int) == 4);
    UnsafeArray_Destroy(top2);
    UnsafeArray_Destroy(evens);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_linq_shuffle(void) {
    TEST("linq: Shuffle changes order, preserves elements");
    srand(42);
    UnsafeArray *arr = _make_ints((int[]){1,2,3,4,5,6,7,8,9,10}, 10);
    UnsafeArray_Shuffle(arr);
    // After shuffle, sort and verify all elements still present
    UnsafeArray_OrderBy(arr, _int_cmp);
    for (int i = 0; i < 10; i++) {
        ASSERT(UnsafeArray_GetDeref(arr, (uint32_t)i, int) == i + 1);
    }
    UnsafeArray_Destroy(arr);
    PASS();
}

// ============================================================
//  LINQ Macro Tests
// ============================================================

static void test_macro_where(void) {
    TEST("macro: LINQ_WHERE inline expression");
    UnsafeArray *arr = _make_ints((int[]){1,2,3,4,5,6,7,8}, 8);
    UnsafeArray *big_evens = LINQ_WHERE(arr, int, x, x > 3 && x % 2 == 0);
    ASSERT(big_evens->count == 3);
    ASSERT(UnsafeArray_GetDeref(big_evens, 0, int) == 4);
    ASSERT(UnsafeArray_GetDeref(big_evens, 1, int) == 6);
    ASSERT(UnsafeArray_GetDeref(big_evens, 2, int) == 8);
    UnsafeArray_Destroy(big_evens);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_macro_first_last(void) {
    TEST("macro: LINQ_FIRST and LINQ_LAST");
    UnsafeArray *arr = _make_ints((int[]){1,3,4,6,7}, 5);
    int *first = LINQ_FIRST(arr, int, x, x % 2 == 0);
    int *last = LINQ_LAST(arr, int, x, x % 2 == 0);
    ASSERT(first != NULL && *first == 4);
    ASSERT(last != NULL && *last == 6);
    int *none = LINQ_FIRST(arr, int, x, x > 100);
    ASSERT(none == NULL);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_macro_indexof(void) {
    TEST("macro: LINQ_INDEXOF");
    UnsafeArray *arr = _make_ints((int[]){10,20,30,40}, 4);
    ASSERT(LINQ_INDEXOF(arr, int, x, x == 30) == 2);
    ASSERT(LINQ_INDEXOF(arr, int, x, x == 99) == -1);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_macro_any_all(void) {
    TEST("macro: LINQ_ANY and LINQ_ALL");
    UnsafeArray *arr = _make_ints((int[]){2,4,6}, 3);
    ASSERT(LINQ_ALL(arr, int, x, x % 2 == 0) == 1);
    ASSERT(LINQ_ANY(arr, int, x, x == 4) == 1);
    ASSERT(LINQ_ANY(arr, int, x, x == 5) == 0);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_macro_count(void) {
    TEST("macro: LINQ_COUNT");
    UnsafeArray *arr = _make_ints((int[]){1,2,3,4,5,6}, 6);
    ASSERT(LINQ_COUNT(arr, int, x, x > 3) == 3);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_macro_select(void) {
    TEST("macro: LINQ_SELECT transform");
    UnsafeArray *arr = _make_ints((int[]){1,2,3}, 3);
    UnsafeArray *doubled = LINQ_SELECT(arr, int, int, x, x * 2);
    ASSERT(doubled->count == 3);
    ASSERT(UnsafeArray_GetDeref(doubled, 0, int) == 2);
    ASSERT(UnsafeArray_GetDeref(doubled, 1, int) == 4);
    ASSERT(UnsafeArray_GetDeref(doubled, 2, int) == 6);
    UnsafeArray_Destroy(doubled);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_macro_select_type_change(void) {
    TEST("macro: LINQ_SELECT int -> float");
    UnsafeArray *arr = _make_ints((int[]){1,2,3}, 3);
    UnsafeArray *floats = LINQ_SELECT(arr, int, float, x, x * 0.5f);
    ASSERT(floats->element_size == sizeof(float));
    ASSERT(UnsafeArray_GetDeref(floats, 0, float) == 0.5f);
    ASSERT(UnsafeArray_GetDeref(floats, 1, float) == 1.0f);
    ASSERT(UnsafeArray_GetDeref(floats, 2, float) == 1.5f);
    UnsafeArray_Destroy(floats);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_macro_foreach_ref(void) {
    TEST("macro: LINQ_FOREACH_REF mutates in place");
    UnsafeArray *arr = _make_ints((int[]){1,2,3}, 3);
    LINQ_FOREACH_REF(arr, int, p, { *p *= 10; });
    ASSERT(UnsafeArray_GetDeref(arr, 0, int) == 10);
    ASSERT(UnsafeArray_GetDeref(arr, 1, int) == 20);
    ASSERT(UnsafeArray_GetDeref(arr, 2, int) == 30);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_macro_removeall(void) {
    TEST("macro: LINQ_REMOVEALL inline");
    UnsafeArray *arr = _make_ints((int[]){-2,-1,0,1,2}, 5);
    uint32_t removed = LINQ_REMOVEALL(arr, int, x, x < 0);
    ASSERT(removed == 2);
    ASSERT(arr->count == 3);
    ASSERT(UnsafeArray_GetDeref(arr, 0, int) == 0);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_macro_aggregate(void) {
    TEST("macro: LINQ_AGGREGATE custom fold");
    UnsafeArray *arr = _make_ints((int[]){1,2,3,4}, 4);
    int sum = LINQ_AGGREGATE(arr, int, int, acc, x, 0, acc + x);
    ASSERT(sum == 10);
    int product = LINQ_AGGREGATE(arr, int, int, acc, x, 1, acc * x);
    ASSERT(product == 24);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_macro_sum_product(void) {
    TEST("macro: LINQ_SUM and LINQ_PRODUCT");
    UnsafeArray *arr = _make_ints((int[]){1,2,3,4,5}, 5);
    ASSERT(LINQ_SUM(arr, int) == 15);
    ASSERT(LINQ_PRODUCT(arr, int) == 120);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_macro_min_max(void) {
    TEST("macro: LINQ_MIN and LINQ_MAX");
    UnsafeArray *arr = _make_ints((int[]){5,1,9,2,7}, 5);
    ASSERT(LINQ_MIN(arr, int) == 1);
    ASSERT(LINQ_MAX(arr, int) == 9);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_macro_chained(void) {
    TEST("macro: chained LINQ_WHERE -> LINQ_SUM");
    UnsafeArray *arr = _make_ints((int[]){1,2,3,4,5,6,7,8,9,10}, 10);
    UnsafeArray *evens = LINQ_WHERE(arr, int, x, x % 2 == 0);
    int sum = LINQ_SUM(evens, int);
    ASSERT(sum == 30); // 2+4+6+8+10
    UnsafeArray_Destroy(evens);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void run_unsafe_array_linq_tests(void) {
    LOG_INFO("=== UnsafeArrayLINQ Tests ===");
    test_linq_where();
    test_linq_where_none();
    test_linq_select();
    test_linq_first();
    test_linq_first_null();
    test_linq_last();
    test_linq_indexof();
    test_linq_lastindexof();
    test_linq_any();
    test_linq_all();
    test_linq_contains();
    test_linq_count();
    test_linq_aggregate();
    test_linq_min_max();
    test_linq_min_max_empty();
    test_linq_foreach();
    test_linq_removeall();
    test_linq_orderby();
    test_linq_reverse();
    test_linq_distinct();
    test_linq_skip_take();
    test_linq_skipwhile_takewhile();
    test_linq_concat();
    test_linq_chained();
    test_linq_shuffle();

    LOG_INFO("=== LINQ Macro Tests ===");
    test_macro_where();
    test_macro_first_last();
    test_macro_indexof();
    test_macro_any_all();
    test_macro_count();
    test_macro_select();
    test_macro_select_type_change();
    test_macro_foreach_ref();
    test_macro_removeall();
    test_macro_aggregate();
    test_macro_sum_product();
    test_macro_min_max();
    test_macro_chained();
}
