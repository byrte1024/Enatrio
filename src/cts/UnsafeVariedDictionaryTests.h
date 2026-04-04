#pragma once

#include "../tests.h"
#include "UnsafeDictionary.h"

// -- UnsafeVariedDictionary Tests --

static void test_varied_dict_create_destroy(void) {
    TEST("varied dict: create and destroy");
    UnsafeVariedDictionary *dict = UnsafeVariedDictionary_Create(8);
    ASSERT(dict != NULL);
    UnsafeVariedDictionary_Destroy(dict);
    PASS();
}

static void test_varied_dict_set_get_int(void) {
    TEST("varied dict: set and get int");
    UnsafeVariedDictionary *dict = UnsafeVariedDictionary_Create(8);
    int val = 42;
    UnsafeVariedDictionary_Set(dict, "health", 6, &val, sizeof(int));
    int *got = (int *)UnsafeVariedDictionary_Get(dict, "health", 6);
    ASSERT(got != NULL);
    ASSERT(*got == 42);
    UnsafeVariedDictionary_Destroy(dict);
    PASS();
}

static void test_varied_dict_different_sizes(void) {
    TEST("varied dict: store different sized values");
    UnsafeVariedDictionary *dict = UnsafeVariedDictionary_Create(8);

    int32_t i = 100;
    float f = 3.14f;
    double d = 2.71828;
    char c = 'X';
    int64_t big = 9999999999LL;

    UnsafeVariedDictionary_SSet(dict, "i32", &i, sizeof(int32_t));
    UnsafeVariedDictionary_SSet(dict, "f32", &f, sizeof(float));
    UnsafeVariedDictionary_SSet(dict, "f64", &d, sizeof(double));
    UnsafeVariedDictionary_SSet(dict, "c8", &c, sizeof(char));
    UnsafeVariedDictionary_SSet(dict, "i64", &big, sizeof(int64_t));

    ASSERT(UnsafeVariedDictionary_SGetDeref(dict, "i32", int32_t) == 100);
    ASSERT(UnsafeVariedDictionary_SGetDeref(dict, "f32", float) > 3.13f);
    ASSERT(UnsafeVariedDictionary_SGetDeref(dict, "f32", float) < 3.15f);
    ASSERT(UnsafeVariedDictionary_SGetDeref(dict, "f64", double) > 2.71);
    ASSERT(UnsafeVariedDictionary_SGetDeref(dict, "f64", double) < 2.72);
    ASSERT(UnsafeVariedDictionary_SGetDeref(dict, "c8", char) == 'X');
    ASSERT(UnsafeVariedDictionary_SGetDeref(dict, "i64", int64_t) == 9999999999LL);

    UnsafeVariedDictionary_Destroy(dict);
    PASS();
}

static void test_varied_dict_get_size(void) {
    TEST("varied dict: GetSize returns correct sizes");
    UnsafeVariedDictionary *dict = UnsafeVariedDictionary_Create(8);

    int32_t i = 1;
    double d = 2.0;
    char c = 'A';
    UnsafeVariedDictionary_SSet(dict, "i", &i, sizeof(int32_t));
    UnsafeVariedDictionary_SSet(dict, "d", &d, sizeof(double));
    UnsafeVariedDictionary_SSet(dict, "c", &c, sizeof(char));

    ASSERT(UnsafeVariedDictionary_SGetSize(dict, "i") == sizeof(int32_t));
    ASSERT(UnsafeVariedDictionary_SGetSize(dict, "d") == sizeof(double));
    ASSERT(UnsafeVariedDictionary_SGetSize(dict, "c") == sizeof(char));
    ASSERT(UnsafeVariedDictionary_SGetSize(dict, "missing") == 0);

    UnsafeVariedDictionary_Destroy(dict);
    PASS();
}

static void test_varied_dict_setvalue_macro(void) {
    TEST("varied dict: SSetValue macro");
    UnsafeVariedDictionary *dict = UnsafeVariedDictionary_Create(8);

    UnsafeVariedDictionary_SSetValue(dict, "hp", int, 250);
    UnsafeVariedDictionary_SSetValue(dict, "speed", float, 1.5f);

    ASSERT(UnsafeVariedDictionary_SGetDeref(dict, "hp", int) == 250);
    float sp = UnsafeVariedDictionary_SGetDeref(dict, "speed", float);
    ASSERT(sp > 1.4f && sp < 1.6f);

    UnsafeVariedDictionary_Destroy(dict);
    PASS();
}

static void test_varied_dict_duplicate_key(void) {
    TEST("varied dict: duplicate key returns -1");
    UnsafeVariedDictionary *dict = UnsafeVariedDictionary_Create(8);
    int a = 1, b = 2;
    ASSERT(UnsafeVariedDictionary_SSet(dict, "x", &a, sizeof(int)) == 0);
    ASSERT(UnsafeVariedDictionary_SSet(dict, "x", &b, sizeof(int)) == -1);
    ASSERT(UnsafeVariedDictionary_SGetDeref(dict, "x", int) == 1);
    UnsafeVariedDictionary_Destroy(dict);
    PASS();
}

static void test_varied_dict_has(void) {
    TEST("varied dict: has returns 1/0");
    UnsafeVariedDictionary *dict = UnsafeVariedDictionary_Create(8);
    UnsafeVariedDictionary_SSetValue(dict, "yes", int, 1);
    ASSERT(UnsafeVariedDictionary_SHas(dict, "yes") == 1);
    ASSERT(UnsafeVariedDictionary_SHas(dict, "no") == 0);
    UnsafeVariedDictionary_Destroy(dict);
    PASS();
}

static void test_varied_dict_get_nonexistent(void) {
    TEST("varied dict: get nonexistent returns NULL");
    UnsafeVariedDictionary *dict = UnsafeVariedDictionary_Create(8);
    ASSERT(UnsafeVariedDictionary_SGet(dict, "nope") == NULL);
    UnsafeVariedDictionary_Destroy(dict);
    PASS();
}

static void test_varied_dict_remove(void) {
    TEST("varied dict: remove clears key");
    UnsafeVariedDictionary *dict = UnsafeVariedDictionary_Create(8);
    UnsafeVariedDictionary_SSetValue(dict, "rm", int, 99);
    ASSERT(UnsafeVariedDictionary_SHas(dict, "rm") == 1);
    ASSERT(UnsafeVariedDictionary_SRemove(dict, "rm") == 0);
    ASSERT(UnsafeVariedDictionary_SHas(dict, "rm") == 0);
    ASSERT(UnsafeVariedDictionary_SRemove(dict, "rm") == -1);
    UnsafeVariedDictionary_Destroy(dict);
    PASS();
}

static void test_varied_dict_struct_value(void) {
    TEST("varied dict: store struct value");
    typedef struct { int x; int y; float z; } Vec3i;
    UnsafeVariedDictionary *dict = UnsafeVariedDictionary_Create(8);
    Vec3i v = { 10, 20, 3.0f };
    UnsafeVariedDictionary_SSet(dict, "pos", &v, sizeof(Vec3i));
    Vec3i *got = (Vec3i *)UnsafeVariedDictionary_SGet(dict, "pos");
    ASSERT(got != NULL);
    ASSERT(got->x == 10);
    ASSERT(got->y == 20);
    ASSERT(got->z > 2.9f && got->z < 3.1f);
    ASSERT(UnsafeVariedDictionary_SGetSize(dict, "pos") == sizeof(Vec3i));
    UnsafeVariedDictionary_Destroy(dict);
    PASS();
}

static void test_varied_dict_many_entries(void) {
    TEST("varied dict: many entries stress test");
    UnsafeVariedDictionary *dict = UnsafeVariedDictionary_Create(8);
    char key[16];
    for (int i = 0; i < 200; i++) {
        int len = snprintf(key, sizeof(key), "k%d", i);
        UnsafeVariedDictionary_Set(dict, key, (uint32_t)len, &i, sizeof(int));
    }
    for (int i = 0; i < 200; i++) {
        int len = snprintf(key, sizeof(key), "k%d", i);
        int *got = (int *)UnsafeVariedDictionary_Get(dict, key, (uint32_t)len);
        ASSERT(got != NULL);
        ASSERT(*got == i);
    }
    UnsafeVariedDictionary_Destroy(dict);
    PASS();
}

static void test_varied_dict_remove_reinsert_reuses_entry(void) {
    TEST("varied dict: remove then reinsert reuses entry slot");
    UnsafeVariedDictionary *dict = UnsafeVariedDictionary_Create(8);
    int v1 = 10, v2 = 20, v3 = 30;
    UnsafeVariedDictionary_SSet(dict, "a", &v1, sizeof(int));
    UnsafeVariedDictionary_SSet(dict, "b", &v2, sizeof(int));
    uint32_t entries_before = dict->entries->count;
    ASSERT(entries_before == 2);

    ASSERT(UnsafeVariedDictionary_SRemove(dict, "a") == 0);
    ASSERT(dict->free_list->count == 1);

    ASSERT(UnsafeVariedDictionary_SSet(dict, "a", &v3, sizeof(int)) == 0);
    ASSERT(dict->entries->count == entries_before);
    ASSERT(dict->free_list->count == 0);
    ASSERT(UnsafeVariedDictionary_SGetDeref(dict, "a", int) == 30);
    ASSERT(UnsafeVariedDictionary_SGetDeref(dict, "b", int) == 20);

    UnsafeVariedDictionary_Destroy(dict);
    PASS();
}

static void run_unsafe_varied_dictionary_tests(void) {
    LOG_INFO("=== UnsafeVariedDictionary Tests ===");
    test_varied_dict_create_destroy();
    test_varied_dict_set_get_int();
    test_varied_dict_different_sizes();
    test_varied_dict_get_size();
    test_varied_dict_setvalue_macro();
    test_varied_dict_duplicate_key();
    test_varied_dict_has();
    test_varied_dict_get_nonexistent();
    test_varied_dict_remove();
    test_varied_dict_struct_value();
    test_varied_dict_many_entries();
    test_varied_dict_remove_reinsert_reuses_entry();
}
