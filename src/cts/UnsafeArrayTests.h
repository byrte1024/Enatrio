#pragma once

#include "UnsafeArray.h"
#include "../tests.h"

static void test_array_create_destroy(void) {
    TEST("array: create and destroy");
    UnsafeArray *arr = UnsafeArray_Create(sizeof(int), 8);
    ASSERT(arr != NULL);
    ASSERT(arr->element_size == sizeof(int));
    ASSERT(arr->capacity == 8);
    ASSERT(arr->count == 0);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_array_add_and_get(void) {
    TEST("array: add and get");
    UnsafeArray *arr = UnsafeArray_Create(sizeof(int), 4);
    int vals[] = {10, 20, 30, 40};
    for (int i = 0; i < 4; i++) UnsafeArray_Add(arr, &vals[i]);
    ASSERT(arr->count == 4);
    for (int i = 0; i < 4; i++) {
        int *p = (int *)UnsafeArray_Get(arr, (uint32_t)i);
        ASSERT(*p == vals[i]);
    }
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_array_getvalue_setvalue(void) {
    TEST("array: GetValue and SetValue");
    UnsafeArray *arr = UnsafeArray_Create(sizeof(int), 4);
    int v = 0;
    UnsafeArray_Add(arr, &v);
    UnsafeArray_Add(arr, &v);
    UnsafeArray_SetValue(arr, 0, int, 42);
    UnsafeArray_SetValue(arr, 1, int, 99);
    ASSERT(UnsafeArray_GetDeref(arr, 0, int) == 42);
    ASSERT(UnsafeArray_GetDeref(arr, 1, int) == 99);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_array_set(void) {
    TEST("array: set overwrites");
    UnsafeArray *arr = UnsafeArray_Create(sizeof(int), 4);
    int v = 5;
    UnsafeArray_Add(arr, &v);
    int w = 77;
    UnsafeArray_Set(arr, 0, &w);
    ASSERT(UnsafeArray_GetDeref(arr, 0, int) == 77);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_array_auto_grow(void) {
    TEST("array: auto grow past capacity");
    UnsafeArray *arr = UnsafeArray_Create(sizeof(int), 2);
    ASSERT(arr->capacity == 2);
    for (int i = 0; i < 100; i++) UnsafeArray_Add(arr, &i);
    ASSERT(arr->count == 100);
    ASSERT(arr->capacity >= 100);
    for (int i = 0; i < 100; i++) {
        ASSERT(UnsafeArray_GetDeref(arr, (uint32_t)i, int) == i);
    }
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_array_remove_ordered(void) {
    TEST("array: remove preserves order");
    UnsafeArray *arr = UnsafeArray_Create(sizeof(int), 8);
    for (int i = 0; i < 5; i++) UnsafeArray_Add(arr, &i);
    UnsafeArray_Remove(arr, 2);
    ASSERT(arr->count == 4);
    ASSERT(UnsafeArray_GetDeref(arr, 0, int) == 0);
    ASSERT(UnsafeArray_GetDeref(arr, 1, int) == 1);
    ASSERT(UnsafeArray_GetDeref(arr, 2, int) == 3);
    ASSERT(UnsafeArray_GetDeref(arr, 3, int) == 4);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_array_remove_swap(void) {
    TEST("array: remove swap is unordered O(1)");
    UnsafeArray *arr = UnsafeArray_Create(sizeof(int), 8);
    for (int i = 0; i < 5; i++) UnsafeArray_Add(arr, &i);
    UnsafeArray_RemoveSwap(arr, 1);
    ASSERT(arr->count == 4);
    ASSERT(UnsafeArray_GetDeref(arr, 0, int) == 0);
    ASSERT(UnsafeArray_GetDeref(arr, 1, int) == 4);
    ASSERT(UnsafeArray_GetDeref(arr, 2, int) == 2);
    ASSERT(UnsafeArray_GetDeref(arr, 3, int) == 3);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_array_remove_first_last(void) {
    TEST("array: remove first and last element");
    UnsafeArray *arr = UnsafeArray_Create(sizeof(int), 8);
    for (int i = 0; i < 4; i++) UnsafeArray_Add(arr, &i);
    UnsafeArray_Remove(arr, 0);
    ASSERT(arr->count == 3);
    ASSERT(UnsafeArray_GetDeref(arr, 0, int) == 1);
    UnsafeArray_Remove(arr, 2);
    ASSERT(arr->count == 2);
    ASSERT(UnsafeArray_GetDeref(arr, 1, int) == 2);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_array_remove_invalid(void) {
    TEST("array: remove out of bounds returns -1");
    UnsafeArray *arr = UnsafeArray_Create(sizeof(int), 4);
    int v = 1;
    UnsafeArray_Add(arr, &v);
    ASSERT(UnsafeArray_Remove(arr, 5) == -1);
    ASSERT(UnsafeArray_RemoveSwap(arr, 5) == -1);
    ASSERT(arr->count == 1);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_array_clear(void) {
    TEST("array: clear resets count");
    UnsafeArray *arr = UnsafeArray_Create(sizeof(int), 8);
    for (int i = 0; i < 5; i++) UnsafeArray_Add(arr, &i);
    UnsafeArray_Clear(arr);
    ASSERT(arr->count == 0);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_array_pointers(void) {
    TEST("array: storing pointers with GetValue/SetValue");
    UnsafeArray *arr = UnsafeArray_Create(sizeof(int *), 4);
    int a = 10, b = 20;
    int *pa = &a, *pb = &b;
    UnsafeArray_Add(arr, &pa);
    UnsafeArray_Add(arr, &pb);
    ASSERT(UnsafeArray_GetDeref(arr, 0, int *) == &a);
    ASSERT(UnsafeArray_GetDeref(arr, 1, int *) == &b);
    ASSERT(*UnsafeArray_GetDeref(arr, 0, int *) == 10);
    ASSERT(*UnsafeArray_GetDeref(arr, 1, int *) == 20);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_array_large_elements(void) {
    TEST("array: large struct elements");
    typedef struct { int x; int y; char name[32]; } BigStruct;
    UnsafeArray *arr = UnsafeArray_Create(sizeof(BigStruct), 4);
    BigStruct s1 = {1, 2, "hello"};
    BigStruct s2 = {3, 4, "world"};
    UnsafeArray_Add(arr, &s1);
    UnsafeArray_Add(arr, &s2);
    BigStruct *r1 = (BigStruct *)UnsafeArray_Get(arr, 0);
    BigStruct *r2 = (BigStruct *)UnsafeArray_Get(arr, 1);
    ASSERT(r1->x == 1 && r1->y == 2 && strcmp(r1->name, "hello") == 0);
    ASSERT(r2->x == 3 && r2->y == 4 && strcmp(r2->name, "world") == 0);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_array_logf(void) {
    TEST("array: LogF visualization");
    UnsafeArray *arr = UnsafeArray_Create(sizeof(int), 4);
    for (int i = 10; i <= 40; i += 10) UnsafeArray_Add(arr, &i);
    UnsafeArray_LogF(arr, int, "%d");
    UnsafeArray_Destroy(arr);
    PASS();
}

static void _fmt_float(uint32_t index, const void *elem, char *buf, uint32_t buf_size) {
    (void)index;
    snprintf(buf, buf_size, "%.2f", *(const float *)elem);
}

static void test_array_log_callback(void) {
    TEST("array: Log with callback");
    UnsafeArray *arr = UnsafeArray_Create(sizeof(float), 4);
    float vals[] = {1.5f, 2.7f, 3.14f};
    for (int i = 0; i < 3; i++) UnsafeArray_Add(arr, &vals[i]);
    UnsafeArray_Log(arr, _fmt_float);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void run_unsafe_array_tests(void) {
    LOG_INFO("=== UnsafeArray Tests ===");
    test_array_create_destroy();
    test_array_add_and_get();
    test_array_getvalue_setvalue();
    test_array_set();
    test_array_auto_grow();
    test_array_remove_ordered();
    test_array_remove_swap();
    test_array_remove_first_last();
    test_array_remove_invalid();
    test_array_clear();
    test_array_pointers();
    test_array_large_elements();
    test_array_logf();
    test_array_log_callback();
}
