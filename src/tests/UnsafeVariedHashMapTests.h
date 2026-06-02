#pragma once

#include "../system/tests.h"
#include "../system/cts/UnsafeDictionary.h"
#include "../system/cts/UnsafeHashMap.h"

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

static void test_varied_hm_remove_reinsert_reuses_entry(void) {
    TEST("varied hashmap: remove then reinsert reuses entry slot");
    UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);
    int v1 = 10, v2 = 20, v3 = 30;
    UnsafeVariedHashMap_SSet(map, "a", &v1, sizeof(int));
    UnsafeVariedHashMap_SSet(map, "b", &v2, sizeof(int));
    uint32_t entries_before = map->entries->count;
    ASSERT(entries_before == 2);

    ASSERT(UnsafeVariedHashMap_SRemove(map, "a") == 0);
    ASSERT(map->free_list->count == 1);

    ASSERT(UnsafeVariedHashMap_SSet(map, "a", &v3, sizeof(int)) == 0);
    ASSERT(map->entries->count == entries_before);
    ASSERT(map->free_list->count == 0);
    ASSERT(UnsafeVariedHashMap_SGetDeref(map, "a", int) == 30);
    ASSERT(UnsafeVariedHashMap_SGetDeref(map, "b", int) == 20);

    UnsafeVariedHashMap_Destroy(map);
    PASS();
}

// -- Helpers --

static int _verify_bytes_vhm(uint8_t *buf, uint32_t offset,
                              const uint8_t *expected, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        if (buf[offset + i] != expected[i]) return 0;
    }
    return 1;
}

static uint32_t _vhm_foreach_sizes[16];
static int _vhm_foreach_idx = 0;
static void _vhm_foreach_recorder(const void *key, uint32_t key_len,
                                   void *value, uint32_t value_size) {
    (void)key; (void)key_len; (void)value;
    _vhm_foreach_sizes[_vhm_foreach_idx++] = value_size;
}

// -- Contract tests --

static void test_varied_hm_contract_set_duplicate_neg1(void) {
    TEST("varied hashmap contract: set duplicate returns -1");
    UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);
    int a = 1, b = 2;
    ASSERT(UnsafeVariedHashMap_SSet(map, "k", &a, sizeof(int)) == 0);
    ASSERT(UnsafeVariedHashMap_SSet(map, "k", &b, sizeof(int)) == -1);
    ASSERT(UnsafeVariedHashMap_SGetDeref(map, "k", int) == 1);
    UnsafeVariedHashMap_Destroy(map);
    PASS();
}

static void test_varied_hm_contract_upsert_creates(void) {
    TEST("varied hashmap contract: upsert nonexistent creates entry");
    UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);
    int val = 42;
    ASSERT(UnsafeVariedHashMap_SUpsert(map, "k", &val, sizeof(int)) == 0);
    ASSERT(UnsafeVariedHashMap_SHas(map, "k") == 1);
    ASSERT(UnsafeVariedHashMap_SGetDeref(map, "k", int) == 42);
    UnsafeVariedHashMap_Destroy(map);
    PASS();
}

static void test_varied_hm_contract_upsert_overwrites(void) {
    TEST("varied hashmap contract: upsert overwrites existing value");
    UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);
    int a = 10, b = 20;
    ASSERT(UnsafeVariedHashMap_SSet(map, "k", &a, sizeof(int)) == 0);
    ASSERT(UnsafeVariedHashMap_SUpsert(map, "k", &b, sizeof(int)) == 0);
    ASSERT(UnsafeVariedHashMap_SGetDeref(map, "k", int) == 20);
    UnsafeVariedHashMap_Destroy(map);
    PASS();
}

static void test_varied_hm_contract_upsert_different_size(void) {
    TEST("varied hashmap contract: upsert changes value size");
    UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);
    int i_val = 42;
    double d_val = 3.14;
    ASSERT(UnsafeVariedHashMap_SSet(map, "val", &i_val, sizeof(int)) == 0);
    ASSERT(UnsafeVariedHashMap_SGetSize(map, "val") == sizeof(int));
    ASSERT(UnsafeVariedHashMap_SUpsert(map, "val", &d_val, sizeof(double)) == 0);
    ASSERT(UnsafeVariedHashMap_SGetSize(map, "val") == sizeof(double));
    double got = UnsafeVariedHashMap_SGetDeref(map, "val", double);
    ASSERT(got > 3.13 && got < 3.15);
    UnsafeVariedHashMap_Destroy(map);
    PASS();
}

static void test_varied_hm_contract_remove_get_null(void) {
    TEST("varied hashmap contract: remove then get returns NULL, size 0");
    UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);
    int val = 99;
    ASSERT(UnsafeVariedHashMap_SSet(map, "k", &val, sizeof(int)) == 0);
    ASSERT(UnsafeVariedHashMap_SRemove(map, "k") == 0);
    ASSERT(UnsafeVariedHashMap_SGet(map, "k") == NULL);
    ASSERT(UnsafeVariedHashMap_SGetSize(map, "k") == 0);
    UnsafeVariedHashMap_Destroy(map);
    PASS();
}

static void test_varied_hm_contract_foreach_sizes(void) {
    TEST("varied hashmap contract: foreach reports correct sizes");
    UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);
    int32_t i_val = 1;
    double d_val = 2.0;
    char c_val = 'A';
    UnsafeVariedHashMap_SSet(map, "i", &i_val, sizeof(int32_t));
    UnsafeVariedHashMap_SSet(map, "d", &d_val, sizeof(double));
    UnsafeVariedHashMap_SSet(map, "c", &c_val, sizeof(char));

    _vhm_foreach_idx = 0;
    UnsafeVariedHashMap_ForEach(map, _vhm_foreach_recorder);
    ASSERT(_vhm_foreach_idx == 3);

    // Sort collected sizes for deterministic comparison (hash order varies)
    for (int a = 0; a < _vhm_foreach_idx - 1; a++) {
        for (int b = a + 1; b < _vhm_foreach_idx; b++) {
            if (_vhm_foreach_sizes[a] > _vhm_foreach_sizes[b]) {
                uint32_t tmp = _vhm_foreach_sizes[a];
                _vhm_foreach_sizes[a] = _vhm_foreach_sizes[b];
                _vhm_foreach_sizes[b] = tmp;
            }
        }
    }
    ASSERT(_vhm_foreach_sizes[0] == sizeof(char));
    ASSERT(_vhm_foreach_sizes[1] == sizeof(int32_t));
    ASSERT(_vhm_foreach_sizes[2] == sizeof(double));

    UnsafeVariedHashMap_Destroy(map);
    PASS();
}

// -- Metric tests (expected to FAIL -- target behavior, data reuse not implemented) --

static void test_varied_hm_metric_remove_set_cycle_no_growth(void) {
    TEST("varied hashmap metric: remove+set cycle no data growth");
    UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);
    int val = 42;
    UnsafeVariedHashMap_SSet(map, "k", &val, sizeof(int));
    uint32_t baseline = map->data->count;
    for (int i = 0; i < 1000; i++) {
        val = i;
        UnsafeVariedHashMap_SRemove(map, "k");
        UnsafeVariedHashMap_SSet(map, "k", &val, sizeof(int));
    }
    ASSERT(map->data->count == baseline);
    UnsafeVariedHashMap_Destroy(map);
    PASS();
}

static void test_varied_hm_metric_upsert_cycle_no_growth(void) {
    TEST("varied hashmap metric: upsert cycle no data growth");
    UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);
    int val = 42;
    UnsafeVariedHashMap_SSet(map, "k", &val, sizeof(int));
    uint32_t baseline = map->data->count;
    for (int i = 0; i < 1000; i++) {
        val = i;
        UnsafeVariedHashMap_SUpsert(map, "k", &val, sizeof(int));
    }
    ASSERT(map->data->count == baseline);
    UnsafeVariedHashMap_Destroy(map);
    PASS();
}

static void test_varied_hm_metric_oscillate_no_growth(void) {
    TEST("varied hashmap metric: oscillating upsert sizes no data growth");
    UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);
    double dbl_val = 3.14;
    UnsafeVariedHashMap_SSet(map, "val", &dbl_val, sizeof(double));
    uint32_t baseline = map->data->count;
    int int_val = 1;
    for (int i = 0; i < 1000; i++) {
        int_val = i;
        dbl_val = (double)i;
        UnsafeVariedHashMap_SUpsert(map, "val", &int_val, sizeof(int));
        UnsafeVariedHashMap_SUpsert(map, "val", &dbl_val, sizeof(double));
    }
    ASSERT(map->data->count == baseline);
    UnsafeVariedHashMap_Destroy(map);
    PASS();
}

static void test_varied_hm_metric_upsert_smaller_no_growth(void) {
    TEST("varied hashmap metric: upsert smaller no data growth");
    UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);
    double dbl_val = 3.14;
    UnsafeVariedHashMap_SSet(map, "val", &dbl_val, sizeof(double));
    uint32_t baseline = map->data->count;
    int int_val = 42;
    UnsafeVariedHashMap_SUpsert(map, "val", &int_val, sizeof(int));
    ASSERT(map->data->count == baseline);
    UnsafeVariedHashMap_Destroy(map);
    PASS();
}

// -- Layout tests --

static void test_varied_hm_layout_bytes(void) {
    TEST("varied hashmap layout: raw bytes at data offset");
    UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);
    uint8_t bytes[3] = {0xAA, 0xBB, 0xCC};
    UnsafeVariedHashMap_SSet(map, "k", bytes, 3);

    uint32_t slot = _UnsafeVariedHashMap_FindSlot(map, "k", 1);
    UnsafeVariedHashEntry *e = &map->buckets[slot];
    ASSERT(e->value >= 0);
    UnsafeVariedHashEntryInfo *info = (UnsafeVariedHashEntryInfo *)UnsafeArray_Get(map->entries, (uint32_t)e->value);
    ASSERT(info->size == 3);
    uint8_t expected[3] = {0xAA, 0xBB, 0xCC};
    ASSERT(_verify_bytes_vhm(map->data->data, info->offset, expected, 3));

    UnsafeVariedHashMap_Destroy(map);
    PASS();
}

static void test_varied_hm_layout_upsert_in_place(void) {
    TEST("varied hashmap layout: upsert same size overwrites in place");
    UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);
    int val = 42;
    UnsafeVariedHashMap_SSet(map, "k", &val, sizeof(int));

    uint32_t slot = _UnsafeVariedHashMap_FindSlot(map, "k", 1);
    UnsafeVariedHashEntry *e = &map->buckets[slot];
    UnsafeVariedHashEntryInfo *info = (UnsafeVariedHashEntryInfo *)UnsafeArray_Get(map->entries, (uint32_t)e->value);
    uint32_t original_offset = info->offset;

    val = 99;
    UnsafeVariedHashMap_SUpsert(map, "k", &val, sizeof(int));

    slot = _UnsafeVariedHashMap_FindSlot(map, "k", 1);
    e = &map->buckets[slot];
    info = (UnsafeVariedHashEntryInfo *)UnsafeArray_Get(map->entries, (uint32_t)e->value);
    ASSERT(info->offset == original_offset);

    int *got = (int *)UnsafeArray_Get(map->data, info->offset);
    ASSERT(*got == 99);

    UnsafeVariedHashMap_Destroy(map);
    PASS();
}

static void test_varied_hm_layout_remove_set_reuse(void) {
    TEST("varied hashmap layout: remove+set reuses data offset");
    UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);
    int val = 42;
    UnsafeVariedHashMap_SSet(map, "k", &val, sizeof(int));

    uint32_t slot = _UnsafeVariedHashMap_FindSlot(map, "k", 1);
    UnsafeVariedHashEntry *e = &map->buckets[slot];
    UnsafeVariedHashEntryInfo *info = (UnsafeVariedHashEntryInfo *)UnsafeArray_Get(map->entries, (uint32_t)e->value);
    uint32_t original_offset = info->offset;

    UnsafeVariedHashMap_SRemove(map, "k");

    val = 99;
    UnsafeVariedHashMap_SSet(map, "k2", &val, sizeof(int));

    slot = _UnsafeVariedHashMap_FindSlot(map, "k2", 2);
    e = &map->buckets[slot];
    info = (UnsafeVariedHashEntryInfo *)UnsafeArray_Get(map->entries, (uint32_t)e->value);
    ASSERT(info->offset == original_offset);

    int *got = (int *)UnsafeArray_Get(map->data, info->offset);
    ASSERT(*got == 99);

    UnsafeVariedHashMap_Destroy(map);
    PASS();
}

static void test_varied_hm_metric_inline_upsert_cycle_no_growth(void) {
    TEST("varied hashmap metric: inline Remove+Set cycle no data growth");
    UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);
    int val = 42;
    UnsafeVariedHashMap_SSet(map, "k", &val, sizeof(int));
    uint32_t baseline = map->data->count;
    for (int i = 0; i < 1000; i++) {
        val = i;
        UnsafeVariedHashMap_SRemove(map, "k");
        UnsafeVariedHashMap_SSet(map, "k", &val, sizeof(int));
    }
    ASSERT(map->data->count == baseline);
    UnsafeVariedHashMap_Destroy(map);
    PASS();
}

static void test_varied_hm_metric_inline_oscillate_no_growth(void) {
    TEST("varied hashmap metric: inline Remove+Set oscillating sizes no data growth");
    UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);
    double dbl_val = 3.14;
    UnsafeVariedHashMap_SSet(map, "val", &dbl_val, sizeof(double));
    uint32_t baseline = map->data->count;
    int int_val = 1;
    for (int i = 0; i < 1000; i++) {
        int_val = i;
        dbl_val = (double)i;
        UnsafeVariedHashMap_SRemove(map, "val");
        UnsafeVariedHashMap_SSet(map, "val", &int_val, sizeof(int));
        UnsafeVariedHashMap_SRemove(map, "val");
        UnsafeVariedHashMap_SSet(map, "val", &dbl_val, sizeof(double));
    }
    ASSERT(map->data->count == baseline);
    UnsafeVariedHashMap_Destroy(map);
    PASS();
}

static void test_varied_hm_layout_inline_upsert_reuse(void) {
    TEST("varied hashmap layout: inline Remove+Set reuses data offset");
    UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);
    int val = 42;
    UnsafeVariedHashMap_SSet(map, "k", &val, sizeof(int));

    uint32_t slot = _UnsafeVariedHashMap_FindSlot(map, "k", 1);
    UnsafeVariedHashEntry *e = &map->buckets[slot];
    UnsafeVariedHashEntryInfo *info = (UnsafeVariedHashEntryInfo *)UnsafeArray_Get(map->entries, (uint32_t)e->value);
    uint32_t original_offset = info->offset;

    UnsafeVariedHashMap_SRemove(map, "k");

    val = 99;
    UnsafeVariedHashMap_SSet(map, "k", &val, sizeof(int));

    slot = _UnsafeVariedHashMap_FindSlot(map, "k", 1);
    e = &map->buckets[slot];
    info = (UnsafeVariedHashEntryInfo *)UnsafeArray_Get(map->entries, (uint32_t)e->value);
    ASSERT(info->offset == original_offset);

    int *got = (int *)UnsafeArray_Get(map->data, info->offset);
    ASSERT(*got == 99);

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
    test_varied_hm_remove_reinsert_reuses_entry();
    // Contract tests
    test_varied_hm_contract_set_duplicate_neg1();
    test_varied_hm_contract_upsert_creates();
    test_varied_hm_contract_upsert_overwrites();
    test_varied_hm_contract_upsert_different_size();
    test_varied_hm_contract_remove_get_null();
    test_varied_hm_contract_foreach_sizes();
    // Metric tests (expected to FAIL)
    test_varied_hm_metric_remove_set_cycle_no_growth();
    test_varied_hm_metric_upsert_cycle_no_growth();
    test_varied_hm_metric_oscillate_no_growth();
    test_varied_hm_metric_upsert_smaller_no_growth();
    test_varied_hm_metric_inline_upsert_cycle_no_growth();
    test_varied_hm_metric_inline_oscillate_no_growth();
    // Layout tests
    test_varied_hm_layout_bytes();
    test_varied_hm_layout_upsert_in_place();
    test_varied_hm_layout_remove_set_reuse();
    test_varied_hm_layout_inline_upsert_reuse();
}
