#pragma once

#include "../tests.h"
#include "UnsafeDictionary.h"
#include "UnsafeHashMap.h"

// -- UnsafeVariedHashMap Tests --

static void test_varied_hm_create_destroy(void) {
    TEST("varied hashmap: create and destroy");
    UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);
    ASSERT(map != NULL);
    UnsafeVariedHashMap_Destroy(map);
    PASS();
}

static void test_varied_hm_set_get_int(void) {
    TEST("varied hashmap: set and get int");
    UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);
    int val = 42;
    UnsafeVariedHashMap_Set(map, "health", 6, &val, sizeof(int));
    int *got = (int *)UnsafeVariedHashMap_Get(map, "health", 6);
    ASSERT(got != NULL);
    ASSERT(*got == 42);
    UnsafeVariedHashMap_Destroy(map);
    PASS();
}

static void test_varied_hm_different_sizes(void) {
    TEST("varied hashmap: store different sized values");
    UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);

    int32_t i = 100;
    float f = 3.14f;
    double d = 2.71828;
    char c = 'X';
    int64_t big = 9999999999LL;

    UnsafeVariedHashMap_SSet(map, "i32", &i, sizeof(int32_t));
    UnsafeVariedHashMap_SSet(map, "f32", &f, sizeof(float));
    UnsafeVariedHashMap_SSet(map, "f64", &d, sizeof(double));
    UnsafeVariedHashMap_SSet(map, "c8", &c, sizeof(char));
    UnsafeVariedHashMap_SSet(map, "i64", &big, sizeof(int64_t));

    ASSERT(UnsafeVariedHashMap_SGetDeref(map, "i32", int32_t) == 100);
    ASSERT(UnsafeVariedHashMap_SGetDeref(map, "f32", float) > 3.13f);
    ASSERT(UnsafeVariedHashMap_SGetDeref(map, "f32", float) < 3.15f);
    ASSERT(UnsafeVariedHashMap_SGetDeref(map, "f64", double) > 2.71);
    ASSERT(UnsafeVariedHashMap_SGetDeref(map, "f64", double) < 2.72);
    ASSERT(UnsafeVariedHashMap_SGetDeref(map, "c8", char) == 'X');
    ASSERT(UnsafeVariedHashMap_SGetDeref(map, "i64", int64_t) == 9999999999LL);

    UnsafeVariedHashMap_Destroy(map);
    PASS();
}

static void test_varied_hm_get_size(void) {
    TEST("varied hashmap: GetSize returns correct sizes");
    UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);

    int32_t i = 1;
    double d = 2.0;
    char c = 'A';
    UnsafeVariedHashMap_SSet(map, "i", &i, sizeof(int32_t));
    UnsafeVariedHashMap_SSet(map, "d", &d, sizeof(double));
    UnsafeVariedHashMap_SSet(map, "c", &c, sizeof(char));

    ASSERT(UnsafeVariedHashMap_SGetSize(map, "i") == sizeof(int32_t));
    ASSERT(UnsafeVariedHashMap_SGetSize(map, "d") == sizeof(double));
    ASSERT(UnsafeVariedHashMap_SGetSize(map, "c") == sizeof(char));
    ASSERT(UnsafeVariedHashMap_SGetSize(map, "missing") == 0);

    UnsafeVariedHashMap_Destroy(map);
    PASS();
}

static void test_varied_hm_setvalue_macro(void) {
    TEST("varied hashmap: SSetValue macro");
    UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);

    UnsafeVariedHashMap_SSetValue(map, "hp", int, 250);
    UnsafeVariedHashMap_SSetValue(map, "speed", float, 1.5f);

    ASSERT(UnsafeVariedHashMap_SGetDeref(map, "hp", int) == 250);
    float sp = UnsafeVariedHashMap_SGetDeref(map, "speed", float);
    ASSERT(sp > 1.4f && sp < 1.6f);

    UnsafeVariedHashMap_Destroy(map);
    PASS();
}

static void test_varied_hm_duplicate_key(void) {
    TEST("varied hashmap: duplicate key returns -1");
    UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);
    int a = 1, b = 2;
    ASSERT(UnsafeVariedHashMap_SSet(map, "x", &a, sizeof(int)) == 0);
    ASSERT(UnsafeVariedHashMap_SSet(map, "x", &b, sizeof(int)) == -1);
    ASSERT(UnsafeVariedHashMap_SGetDeref(map, "x", int) == 1);
    UnsafeVariedHashMap_Destroy(map);
    PASS();
}

static void test_varied_hm_has(void) {
    TEST("varied hashmap: has returns 1/0");
    UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);
    UnsafeVariedHashMap_SSetValue(map, "yes", int, 1);
    ASSERT(UnsafeVariedHashMap_SHas(map, "yes") == 1);
    ASSERT(UnsafeVariedHashMap_SHas(map, "no") == 0);
    UnsafeVariedHashMap_Destroy(map);
    PASS();
}

static void test_varied_hm_get_nonexistent(void) {
    TEST("varied hashmap: get nonexistent returns NULL");
    UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);
    ASSERT(UnsafeVariedHashMap_SGet(map, "nope") == NULL);
    UnsafeVariedHashMap_Destroy(map);
    PASS();
}

static void test_varied_hm_remove(void) {
    TEST("varied hashmap: remove clears key");
    UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);
    UnsafeVariedHashMap_SSetValue(map, "rm", int, 99);
    ASSERT(UnsafeVariedHashMap_SHas(map, "rm") == 1);
    ASSERT(UnsafeVariedHashMap_SRemove(map, "rm") == 0);
    ASSERT(UnsafeVariedHashMap_SHas(map, "rm") == 0);
    ASSERT(UnsafeVariedHashMap_SRemove(map, "rm") == -1);
    UnsafeVariedHashMap_Destroy(map);
    PASS();
}

static void test_varied_hm_struct_value(void) {
    TEST("varied hashmap: store struct value");
    typedef struct { int x; int y; float z; } Vec3i;
    UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);
    Vec3i v = { 10, 20, 3.0f };
    UnsafeVariedHashMap_SSet(map, "pos", &v, sizeof(Vec3i));
    Vec3i *got = (Vec3i *)UnsafeVariedHashMap_SGet(map, "pos");
    ASSERT(got != NULL);
    ASSERT(got->x == 10);
    ASSERT(got->y == 20);
    ASSERT(got->z > 2.9f && got->z < 3.1f);
    ASSERT(UnsafeVariedHashMap_SGetSize(map, "pos") == sizeof(Vec3i));
    UnsafeVariedHashMap_Destroy(map);
    PASS();
}

static void test_varied_hm_many_entries(void) {
    TEST("varied hashmap: many entries stress test");
    UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);
    char key[16];
    for (int i = 0; i < 200; i++) {
        int len = snprintf(key, sizeof(key), "k%d", i);
        UnsafeVariedHashMap_Set(map, key, (uint32_t)len, &i, sizeof(int));
    }
    for (int i = 0; i < 200; i++) {
        int len = snprintf(key, sizeof(key), "k%d", i);
        int *got = (int *)UnsafeVariedHashMap_Get(map, key, (uint32_t)len);
        ASSERT(got != NULL);
        ASSERT(*got == i);
    }
    UnsafeVariedHashMap_Destroy(map);
    PASS();
}

static void test_varied_hm_rehash(void) {
    TEST("varied hashmap: lookups work after rehash");
    UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(4);
    char key[16];
    for (int i = 0; i < 100; i++) {
        int len = snprintf(key, sizeof(key), "key%d", i);
        UnsafeVariedHashMap_Set(map, key, (uint32_t)len, &i, sizeof(int));
    }
    for (int i = 0; i < 100; i++) {
        int len = snprintf(key, sizeof(key), "key%d", i);
        int *got = (int *)UnsafeVariedHashMap_Get(map, key, (uint32_t)len);
        ASSERT(got != NULL);
        ASSERT(*got == i);
    }
    UnsafeVariedHashMap_Destroy(map);
    PASS();
}

static void run_unsafe_varied_hashmap_tests(void) {
    LOG_INFO("=== UnsafeVariedHashMap Tests ===");
    test_varied_hm_create_destroy();
    test_varied_hm_set_get_int();
    test_varied_hm_different_sizes();
    test_varied_hm_get_size();
    test_varied_hm_setvalue_macro();
    test_varied_hm_duplicate_key();
    test_varied_hm_has();
    test_varied_hm_get_nonexistent();
    test_varied_hm_remove();
    test_varied_hm_struct_value();
    test_varied_hm_many_entries();
    test_varied_hm_rehash();
}
