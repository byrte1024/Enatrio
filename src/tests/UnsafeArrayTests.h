#pragma once

#include "../system/cts/UnsafeArray.h"
#include "../system/tests.h"

static int _verify_bytes_arr(uint8_t *buf, uint32_t offset, const uint8_t *expected, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        if (buf[offset + i] != expected[i]) return 0;
    }
    return 1;
}

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

static void test_array_create_zero_capacity(void) {
    TEST("array: create with zero capacity");
    UnsafeArray *arr = UnsafeArray_Create(sizeof(int), 0);
    ASSERT(arr != NULL);
    ASSERT(arr->capacity >= 1);
    int v = 42;
    int rc = UnsafeArray_Add(arr, &v);
    ASSERT(rc == 0);
    ASSERT(arr->count == 1);
    ASSERT(UnsafeArray_GetDeref(arr, 0, int) == 42);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_array_add_returns_int(void) {
    TEST("array: Add returns 0 on success");
    UnsafeArray *arr = UnsafeArray_Create(sizeof(int), 4);
    int v = 10;
    int rc = UnsafeArray_Add(arr, &v);
    ASSERT(rc == 0);
    v = 20;
    rc = UnsafeArray_Add(arr, &v);
    ASSERT(rc == 0);
    ASSERT(arr->count == 2);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_array_grow_preserves_data(void) {
    TEST("array: grow preserves data across reallocs");
    UnsafeArray *arr = UnsafeArray_Create(sizeof(int), 2);
    for (int i = 0; i < 100; i++) UnsafeArray_Add(arr, &i);
    ASSERT(arr->count == 100);
    for (int i = 0; i < 100; i++) {
        ASSERT(UnsafeArray_GetDeref(arr, (uint32_t)i, int) == i);
    }
    UnsafeArray_Destroy(arr);
    PASS();
}

// --- Contract tests (should PASS) ---

static void test_array_contract_add_beyond_capacity(void) {
    TEST("array contract: add beyond capacity");
    UnsafeArray *arr = UnsafeArray_Create(sizeof(int), 4);
    for (int i = 0; i < 10; i++) UnsafeArray_Add(arr, &i);
    ASSERT(arr->count == 10);
    for (int i = 0; i < 10; i++) {
        ASSERT(UnsafeArray_GetDeref(arr, (uint32_t)i, int) == i);
    }
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_array_contract_remove_shifts(void) {
    TEST("array contract: remove shifts elements");
    UnsafeArray *arr = UnsafeArray_Create(sizeof(int), 8);
    int vals[] = {10, 20, 30};
    for (int i = 0; i < 3; i++) UnsafeArray_Add(arr, &vals[i]);
    UnsafeArray_Remove(arr, 1);
    ASSERT(arr->count == 2);
    ASSERT(UnsafeArray_GetDeref(arr, 0, int) == 10);
    ASSERT(UnsafeArray_GetDeref(arr, 1, int) == 30);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_array_contract_removeswap(void) {
    TEST("array contract: removeswap swaps last into slot");
    UnsafeArray *arr = UnsafeArray_Create(sizeof(int), 8);
    int vals[] = {10, 20, 30};
    for (int i = 0; i < 3; i++) UnsafeArray_Add(arr, &vals[i]);
    UnsafeArray_RemoveSwap(arr, 0);
    ASSERT(arr->count == 2);
    ASSERT(UnsafeArray_GetDeref(arr, 0, int) == 30);
    ASSERT(UnsafeArray_GetDeref(arr, 1, int) == 20);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_array_contract_set_no_count_change(void) {
    TEST("array contract: set does not change count");
    UnsafeArray *arr = UnsafeArray_Create(sizeof(int), 8);
    int vals[] = {1, 2, 3};
    for (int i = 0; i < 3; i++) UnsafeArray_Add(arr, &vals[i]);
    ASSERT(arr->count == 3);
    int newval = 99;
    UnsafeArray_Set(arr, 1, &newval);
    ASSERT(arr->count == 3);
    ASSERT(UnsafeArray_GetDeref(arr, 1, int) == 99);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_array_contract_get_oob(void) {
    TEST("array contract: get out of bounds returns NULL");
    UnsafeArray *arr = UnsafeArray_Create(sizeof(int), 8);
    int vals[] = {1, 2, 3};
    for (int i = 0; i < 3; i++) UnsafeArray_Add(arr, &vals[i]);
    ASSERT(UnsafeArray_Get(arr, 3) == NULL);
    ASSERT(UnsafeArray_Get(arr, 100) == NULL);
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_array_contract_clear(void) {
    TEST("array contract: clear preserves capacity");
    UnsafeArray *arr = UnsafeArray_Create(sizeof(int), 8);
    for (int i = 0; i < 5; i++) UnsafeArray_Add(arr, &i);
    uint32_t cap_before = arr->capacity;
    UnsafeArray_Clear(arr);
    ASSERT(arr->count == 0);
    ASSERT(arr->capacity == cap_before);
    UnsafeArray_Destroy(arr);
    PASS();
}

// --- Contract tests (will FAIL -- stubs return -1) ---

static void test_array_contract_addbulk(void) {
    TEST("array contract: addbulk adds N elements");
    UnsafeArray *arr = UnsafeArray_Create(sizeof(int), 8);
    int data[5] = {1, 2, 3, 4, 5};
    int rc = UnsafeArray_AddBulk(arr, data, 5);
    ASSERT(rc == 0);
    ASSERT(arr->count == 5);
    for (int i = 0; i < 5; i++) {
        ASSERT(UnsafeArray_GetDeref(arr, (uint32_t)i, int) == data[i]);
    }
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_array_contract_addbulk_matches_individual(void) {
    TEST("array contract: addbulk matches individual adds");
    int data[5] = {10, 20, 30, 40, 50};
    UnsafeArray *arr1 = UnsafeArray_Create(sizeof(int), 8);
    UnsafeArray_AddBulk(arr1, data, 5);
    UnsafeArray *arr2 = UnsafeArray_Create(sizeof(int), 8);
    for (int i = 0; i < 5; i++) UnsafeArray_Add(arr2, &data[i]);
    ASSERT(arr1->count == arr2->count);
    ASSERT(memcmp(arr1->data, arr2->data, (size_t)arr1->count * arr1->element_size) == 0);
    UnsafeArray_Destroy(arr1);
    UnsafeArray_Destroy(arr2);
    PASS();
}

// --- Metric tests (will FAIL -- stubs) ---

static void test_array_metric_shrinktofit(void) {
    TEST("array metric: shrinktofit reduces capacity to count");
    UnsafeArray *arr = UnsafeArray_Create(sizeof(int), 128);
    for (int i = 0; i < 100; i++) UnsafeArray_Add(arr, &i);
    for (int i = 0; i < 90; i++) UnsafeArray_RemoveSwap(arr, arr->count - 1);
    ASSERT(arr->count == 10);
    int rc = UnsafeArray_ShrinkToFit(arr);
    ASSERT(rc == 0);
    ASSERT(arr->capacity == arr->count);
    for (int i = 0; i < (int)arr->count; i++) {
        (void)UnsafeArray_Get(arr, (uint32_t)i);
    }
    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_array_metric_addbulk_same_growth(void) {
    TEST("array metric: addbulk same count as individual adds");
    int data[50];
    for (int i = 0; i < 50; i++) data[i] = i * 3;
    UnsafeArray *arr1 = UnsafeArray_Create(sizeof(int), 4);
    UnsafeArray_AddBulk(arr1, data, 50);
    UnsafeArray *arr2 = UnsafeArray_Create(sizeof(int), 4);
    for (int i = 0; i < 50; i++) UnsafeArray_Add(arr2, &data[i]);
    ASSERT(arr1->count == arr2->count);
    UnsafeArray_Destroy(arr1);
    UnsafeArray_Destroy(arr2);
    PASS();
}

// --- Layout tests (should PASS) ---

static void test_array_layout_add(void) {
    TEST("array layout: add places bytes at correct offsets");
    UnsafeArray *arr = UnsafeArray_Create(sizeof(int), 8);
    int v;
    uint8_t expected[4];

    v = 10; UnsafeArray_Add(arr, &v);
    v = 20; UnsafeArray_Add(arr, &v);
    v = 30; UnsafeArray_Add(arr, &v);

    v = 10; memcpy(expected, &v, 4);
    ASSERT(_verify_bytes_arr(arr->data, 0, expected, 4));
    v = 20; memcpy(expected, &v, 4);
    ASSERT(_verify_bytes_arr(arr->data, 4, expected, 4));
    v = 30; memcpy(expected, &v, 4);
    ASSERT(_verify_bytes_arr(arr->data, 8, expected, 4));

    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_array_layout_remove(void) {
    TEST("array layout: remove shifts bytes correctly");
    UnsafeArray *arr = UnsafeArray_Create(sizeof(int), 8);
    int v;
    uint8_t expected[4];

    v = 10; UnsafeArray_Add(arr, &v);
    v = 20; UnsafeArray_Add(arr, &v);
    v = 30; UnsafeArray_Add(arr, &v);
    UnsafeArray_Remove(arr, 1);

    v = 10; memcpy(expected, &v, 4);
    ASSERT(_verify_bytes_arr(arr->data, 0, expected, 4));
    v = 30; memcpy(expected, &v, 4);
    ASSERT(_verify_bytes_arr(arr->data, 4, expected, 4));

    UnsafeArray_Destroy(arr);
    PASS();
}

static void test_array_layout_removeswap(void) {
    TEST("array layout: removeswap places last at removed index");
    UnsafeArray *arr = UnsafeArray_Create(sizeof(int), 8);
    int v;
    uint8_t expected[4];

    v = 10; UnsafeArray_Add(arr, &v);
    v = 20; UnsafeArray_Add(arr, &v);
    v = 30; UnsafeArray_Add(arr, &v);
    UnsafeArray_RemoveSwap(arr, 0);

    v = 30; memcpy(expected, &v, 4);
    ASSERT(_verify_bytes_arr(arr->data, 0, expected, 4));
    v = 20; memcpy(expected, &v, 4);
    ASSERT(_verify_bytes_arr(arr->data, 4, expected, 4));

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
    test_array_create_zero_capacity();
    test_array_add_returns_int();
    test_array_grow_preserves_data();
    // Contract tests (should PASS)
    test_array_contract_add_beyond_capacity();
    test_array_contract_remove_shifts();
    test_array_contract_removeswap();
    test_array_contract_set_no_count_change();
    test_array_contract_get_oob();
    test_array_contract_clear();
    // Contract tests (will FAIL -- stubs)
    test_array_contract_addbulk();
    test_array_contract_addbulk_matches_individual();
    // Metric tests (will FAIL -- stubs)
    test_array_metric_shrinktofit();
    test_array_metric_addbulk_same_growth();
    // Layout tests (should PASS)
    test_array_layout_add();
    test_array_layout_remove();
    test_array_layout_removeswap();
}
