#pragma once

#include "UnsafeDictionary.h"
#include "../tests.h"

static void test_dict_create_destroy(void) {
    TEST("dict: create and destroy");
    UnsafeDictionary *dict = UnsafeDictionary_Create(sizeof(int), 8);
    ASSERT(dict != NULL);
    ASSERT(dict->values != NULL);
    ASSERT(dict->nodes != NULL);
    ASSERT(dict->nodes->count == 1);
    UnsafeDictionary_Destroy(dict);
    PASS();
}

static void test_dict_set_and_get(void) {
    TEST("dict: set and get string keys");
    UnsafeDictionary *dict = UnsafeDictionary_Create(sizeof(int), 8);
    int v1 = 100, v2 = 200, v3 = 300;
    ASSERT(UnsafeDictionary_Set(dict, "health", 6, &v1) == 0);
    ASSERT(UnsafeDictionary_Set(dict, "mana", 4, &v2) == 0);
    ASSERT(UnsafeDictionary_Set(dict, "armor", 5, &v3) == 0);
    int *r1 = (int *)UnsafeDictionary_Get(dict, "health", 6);
    int *r2 = (int *)UnsafeDictionary_Get(dict, "mana", 4);
    int *r3 = (int *)UnsafeDictionary_Get(dict, "armor", 5);
    ASSERT(r1 != NULL && *r1 == 100);
    ASSERT(r2 != NULL && *r2 == 200);
    ASSERT(r3 != NULL && *r3 == 300);
    UnsafeDictionary_Destroy(dict);
    PASS();
}

static void test_dict_getvalue_setvalue(void) {
    TEST("dict: GetValue and SetValue macros");
    UnsafeDictionary *dict = UnsafeDictionary_Create(sizeof(int), 8);
    UnsafeDictionary_SetValue(dict, "foo", 3, int, 42);
    UnsafeDictionary_SetValue(dict, "bar", 3, int, 99);
    ASSERT(UnsafeDictionary_GetDeref(dict, "foo", 3, int) == 42);
    ASSERT(UnsafeDictionary_GetDeref(dict, "bar", 3, int) == 99);
    UnsafeDictionary_Destroy(dict);
    PASS();
}

static void test_dict_duplicate_key_rejected(void) {
    TEST("dict: duplicate key returns -1");
    UnsafeDictionary *dict = UnsafeDictionary_Create(sizeof(int), 8);
    int v = 10;
    ASSERT(UnsafeDictionary_Set(dict, "key", 3, &v) == 0);
    v = 20;
    ASSERT(UnsafeDictionary_Set(dict, "key", 3, &v) == -1);
    ASSERT(UnsafeDictionary_GetDeref(dict, "key", 3, int) == 10);
    UnsafeDictionary_Destroy(dict);
    PASS();
}

static void test_dict_get_missing(void) {
    TEST("dict: get nonexistent key returns NULL");
    UnsafeDictionary *dict = UnsafeDictionary_Create(sizeof(int), 8);
    ASSERT(UnsafeDictionary_Get(dict, "nope", 4) == NULL);
    UnsafeDictionary_Destroy(dict);
    PASS();
}

static void test_dict_has(void) {
    TEST("dict: has returns 1/0");
    UnsafeDictionary *dict = UnsafeDictionary_Create(sizeof(int), 8);
    int v = 1;
    UnsafeDictionary_Set(dict, "yes", 3, &v);
    ASSERT(UnsafeDictionary_Has(dict, "yes", 3) == 1);
    ASSERT(UnsafeDictionary_Has(dict, "no", 2) == 0);
    UnsafeDictionary_Destroy(dict);
    PASS();
}

static void test_dict_remove(void) {
    TEST("dict: remove clears key");
    UnsafeDictionary *dict = UnsafeDictionary_Create(sizeof(int), 8);
    int v = 55;
    UnsafeDictionary_Set(dict, "tmp", 3, &v);
    ASSERT(UnsafeDictionary_Has(dict, "tmp", 3) == 1);
    ASSERT(UnsafeDictionary_Remove(dict, "tmp", 3) == 0);
    ASSERT(UnsafeDictionary_Has(dict, "tmp", 3) == 0);
    ASSERT(UnsafeDictionary_Get(dict, "tmp", 3) == NULL);
    UnsafeDictionary_Destroy(dict);
    PASS();
}

static void test_dict_remove_missing(void) {
    TEST("dict: remove nonexistent key returns -1");
    UnsafeDictionary *dict = UnsafeDictionary_Create(sizeof(int), 8);
    ASSERT(UnsafeDictionary_Remove(dict, "ghost", 5) == -1);
    UnsafeDictionary_Destroy(dict);
    PASS();
}

static void test_dict_integer_keys(void) {
    TEST("dict: integer keys");
    UnsafeDictionary *dict = UnsafeDictionary_Create(sizeof(int), 16);
    for (int i = 0; i < 10; i++) {
        uint32_t key = (uint32_t)i;
        int val = i * 100;
        ASSERT(UnsafeDictionary_Set(dict, &key, sizeof(key), &val) == 0);
    }
    for (int i = 0; i < 10; i++) {
        uint32_t key = (uint32_t)i;
        int *val = (int *)UnsafeDictionary_Get(dict, &key, sizeof(key));
        ASSERT(val != NULL && *val == i * 100);
    }
    UnsafeDictionary_Destroy(dict);
    PASS();
}

static void test_dict_prefix_keys(void) {
    TEST("dict: keys that are prefixes of each other");
    UnsafeDictionary *dict = UnsafeDictionary_Create(sizeof(int), 8);
    int v1 = 1, v2 = 2, v3 = 3;
    ASSERT(UnsafeDictionary_Set(dict, "a", 1, &v1) == 0);
    ASSERT(UnsafeDictionary_Set(dict, "ab", 2, &v2) == 0);
    ASSERT(UnsafeDictionary_Set(dict, "abc", 3, &v3) == 0);
    ASSERT(UnsafeDictionary_GetDeref(dict, "a", 1, int) == 1);
    ASSERT(UnsafeDictionary_GetDeref(dict, "ab", 2, int) == 2);
    ASSERT(UnsafeDictionary_GetDeref(dict, "abc", 3, int) == 3);
    UnsafeDictionary_Destroy(dict);
    PASS();
}

static void test_dict_empty_key(void) {
    TEST("dict: zero-length key");
    UnsafeDictionary *dict = UnsafeDictionary_Create(sizeof(int), 8);
    int v = 999;
    ASSERT(UnsafeDictionary_Set(dict, "", 0, &v) == 0);
    ASSERT(UnsafeDictionary_GetDeref(dict, "", 0, int) == 999);
    ASSERT(UnsafeDictionary_Get(dict, "a", 1) == NULL);
    UnsafeDictionary_Destroy(dict);
    PASS();
}

static void test_dict_modify_in_place(void) {
    TEST("dict: modify value in place via Get pointer");
    UnsafeDictionary *dict = UnsafeDictionary_Create(sizeof(int), 8);
    int v = 10;
    UnsafeDictionary_Set(dict, "hp", 2, &v);
    int *ptr = (int *)UnsafeDictionary_Get(dict, "hp", 2);
    *ptr = 80;
    ASSERT(UnsafeDictionary_GetDeref(dict, "hp", 2, int) == 80);
    UnsafeDictionary_Destroy(dict);
    PASS();
}

static void test_dict_many_entries(void) {
    TEST("dict: many entries stress test");
    UnsafeDictionary *dict = UnsafeDictionary_Create(sizeof(int), 4);
    char key[16];
    for (int i = 0; i < 500; i++) {
        int len = snprintf(key, sizeof(key), "k%d", i);
        ASSERT(UnsafeDictionary_Set(dict, key, (uint32_t)len, &i) == 0);
    }
    for (int i = 0; i < 500; i++) {
        int len = snprintf(key, sizeof(key), "k%d", i);
        int *val = (int *)UnsafeDictionary_Get(dict, key, (uint32_t)len);
        ASSERT(val != NULL && *val == i);
    }
    UnsafeDictionary_Destroy(dict);
    PASS();
}

static void test_dict_struct_values(void) {
    TEST("dict: struct values");
    typedef struct { float x; float y; } Vec2;
    UnsafeDictionary *dict = UnsafeDictionary_Create(sizeof(Vec2), 8);
    Vec2 pos = {1.5f, 2.5f};
    UnsafeDictionary_Set(dict, "player", 6, &pos);
    Vec2 *r = (Vec2 *)UnsafeDictionary_Get(dict, "player", 6);
    ASSERT(r != NULL && r->x == 1.5f && r->y == 2.5f);
    UnsafeDictionary_Destroy(dict);
    PASS();
}

static void test_dict_logf_string_keys(void) {
    TEST("dict: LogF with string keys");
    UnsafeDictionary *dict = UnsafeDictionary_Create(sizeof(int), 8);
    UnsafeDictionary_SetValue(dict, "health", 6, int, 100);
    UnsafeDictionary_SetValue(dict, "mana", 4, int, 50);
    UnsafeDictionary_SetValue(dict, "armor", 5, int, 25);
    UnsafeDictionary_LogF(dict, int, "%d", 1);
    UnsafeDictionary_Destroy(dict);
    PASS();
}

static void test_dict_logf_hex_keys(void) {
    TEST("dict: LogF with hex keys");
    UnsafeDictionary *dict = UnsafeDictionary_Create(sizeof(int), 8);
    for (int i = 0; i < 3; i++) {
        uint32_t key = (uint32_t)(i + 1);
        int val = (i + 1) * 100;
        UnsafeDictionary_Set(dict, &key, sizeof(key), &val);
    }
    UnsafeDictionary_LogF(dict, int, "%d", 0);
    UnsafeDictionary_Destroy(dict);
    PASS();
}

static void _fmt_dict_int(const void *value, char *buf, uint32_t buf_size) {
    snprintf(buf, buf_size, "%d", *(const int *)value);
}

static void test_dict_log_callback(void) {
    TEST("dict: Log with callback");
    UnsafeDictionary *dict = UnsafeDictionary_Create(sizeof(int), 8);
    UnsafeDictionary_SetValue(dict, "atk", 3, int, 55);
    UnsafeDictionary_SetValue(dict, "def", 3, int, 30);
    UnsafeDictionary_Log(dict, _fmt_dict_int, 1);
    UnsafeDictionary_Destroy(dict);
    PASS();
}

static void test_dict_string_macros(void) {
    TEST("dict: S-macros for string keys");
    UnsafeDictionary *dict = UnsafeDictionary_Create(sizeof(int), 8);
    UnsafeDictionary_SSetValue(dict, "health", int, 100);
    UnsafeDictionary_SSetValue(dict, "mana", int, 50);

    ASSERT(UnsafeDictionary_SHas(dict, "health") == 1);
    ASSERT(UnsafeDictionary_SHas(dict, "nope") == 0);
    ASSERT(UnsafeDictionary_SGetDeref(dict, "health", int) == 100);
    ASSERT(UnsafeDictionary_SGetDeref(dict, "mana", int) == 50);

    int *ptr = (int *)UnsafeDictionary_SGet(dict, "health");
    ASSERT(ptr != NULL && *ptr == 100);

    int v = 25;
    ASSERT(UnsafeDictionary_SSet(dict, "armor", &v) == 0);
    ASSERT(UnsafeDictionary_SGetDeref(dict, "armor", int) == 25);

    ASSERT(UnsafeDictionary_SRemove(dict, "mana") == 0);
    ASSERT(UnsafeDictionary_SHas(dict, "mana") == 0);

    UnsafeDictionary_Destroy(dict);
    PASS();
}

static void run_unsafe_dictionary_tests(void) {
    LOG_INFO("=== UnsafeDictionary Tests ===");
    test_dict_create_destroy();
    test_dict_set_and_get();
    test_dict_getvalue_setvalue();
    test_dict_duplicate_key_rejected();
    test_dict_get_missing();
    test_dict_has();
    test_dict_remove();
    test_dict_remove_missing();
    test_dict_integer_keys();
    test_dict_prefix_keys();
    test_dict_empty_key();
    test_dict_modify_in_place();
    test_dict_many_entries();
    test_dict_struct_values();
    test_dict_logf_string_keys();
    test_dict_logf_hex_keys();
    test_dict_log_callback();
    test_dict_string_macros();
}
