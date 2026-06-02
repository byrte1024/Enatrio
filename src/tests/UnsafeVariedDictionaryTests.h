#pragma once

#include "../system/tests.h"
#include "../system/cts/UnsafeDictionary.h"

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

// -- Helpers --

static int _verify_bytes_vd(uint8_t *buf, uint32_t offset,
                             const uint8_t *expected, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        if (buf[offset + i] != expected[i]) return 0;
    }
    return 1;
}

// ForEach recorder state
static uint32_t _vd_foreach_sizes[16];
static char _vd_foreach_keys[16][32];
static int _vd_foreach_idx = 0;
static void _vd_foreach_recorder(const void *key, uint32_t key_len, void *value, uint32_t value_size) {
    (void)value;
    memcpy(_vd_foreach_keys[_vd_foreach_idx], key, key_len);
    _vd_foreach_keys[_vd_foreach_idx][key_len] = '\0';
    _vd_foreach_sizes[_vd_foreach_idx] = value_size;
    _vd_foreach_idx++;
}

// -- Contract tests --

static void test_varied_dict_contract_set_get_remove_has(void) {
    TEST("varied dict contract: set, get, has, remove");
    UnsafeVariedDictionary *dict = UnsafeVariedDictionary_Create(8);
    int32_t val = 42;

    // Set and Get
    ASSERT(UnsafeVariedDictionary_SSet(dict, "key", &val, sizeof(int32_t)) == 0);
    int32_t *got = (int32_t *)UnsafeVariedDictionary_SGet(dict, "key");
    ASSERT(got != NULL);
    ASSERT(*got == 42);

    // Has
    ASSERT(UnsafeVariedDictionary_SHas(dict, "key") == 1);
    ASSERT(UnsafeVariedDictionary_SHas(dict, "missing") == 0);

    // Remove
    ASSERT(UnsafeVariedDictionary_SRemove(dict, "key") == 0);
    ASSERT(UnsafeVariedDictionary_SHas(dict, "key") == 0);
    ASSERT(UnsafeVariedDictionary_SGet(dict, "key") == NULL);
    ASSERT(UnsafeVariedDictionary_SRemove(dict, "key") == -1);

    UnsafeVariedDictionary_Destroy(dict);
    PASS();
}

static void test_varied_dict_contract_getsize(void) {
    TEST("varied dict contract: GetSize returns correct sizes");
    UnsafeVariedDictionary *dict = UnsafeVariedDictionary_Create(8);

    int32_t i = 1;
    double d = 2.0;
    char c = 'A';
    uint8_t buf[7] = {1,2,3,4,5,6,7};

    UnsafeVariedDictionary_SSet(dict, "i32", &i, sizeof(int32_t));
    UnsafeVariedDictionary_SSet(dict, "f64", &d, sizeof(double));
    UnsafeVariedDictionary_SSet(dict, "c8", &c, sizeof(char));
    UnsafeVariedDictionary_SSet(dict, "buf7", buf, 7);

    ASSERT(UnsafeVariedDictionary_SGetSize(dict, "i32") == sizeof(int32_t));
    ASSERT(UnsafeVariedDictionary_SGetSize(dict, "f64") == sizeof(double));
    ASSERT(UnsafeVariedDictionary_SGetSize(dict, "c8") == sizeof(char));
    ASSERT(UnsafeVariedDictionary_SGetSize(dict, "buf7") == 7);
    ASSERT(UnsafeVariedDictionary_SGetSize(dict, "nope") == 0);

    UnsafeVariedDictionary_Destroy(dict);
    PASS();
}

static void test_varied_dict_contract_foreach_trie_order(void) {
    TEST("varied dict contract: ForEach visits in trie order (a,b,c)");
    UnsafeVariedDictionary *dict = UnsafeVariedDictionary_Create(8);

    int32_t vb = 2, va = 1, vc = 3;
    UnsafeVariedDictionary_SSet(dict, "b", &vb, sizeof(int32_t));
    UnsafeVariedDictionary_SSet(dict, "a", &va, sizeof(int32_t));
    UnsafeVariedDictionary_SSet(dict, "c", &vc, sizeof(int32_t));

    _vd_foreach_idx = 0;
    UnsafeVariedDictionary_ForEach(dict, _vd_foreach_recorder);

    ASSERT(_vd_foreach_idx == 3);
    ASSERT(strcmp(_vd_foreach_keys[0], "a") == 0);
    ASSERT(strcmp(_vd_foreach_keys[1], "b") == 0);
    ASSERT(strcmp(_vd_foreach_keys[2], "c") == 0);
    ASSERT(_vd_foreach_sizes[0] == sizeof(int32_t));
    ASSERT(_vd_foreach_sizes[1] == sizeof(int32_t));
    ASSERT(_vd_foreach_sizes[2] == sizeof(int32_t));

    UnsafeVariedDictionary_Destroy(dict);
    PASS();
}

// -- Contract tests (Upsert stub -- expected to FAIL) --

static void test_varied_dict_contract_upsert_creates(void) {
    TEST("varied dict contract: upsert creates new entry");
    UnsafeVariedDictionary *dict = UnsafeVariedDictionary_Create(8);

    int32_t val = 55;
    int rc = UnsafeVariedDictionary_SUpsert(dict, "uk", &val, sizeof(int32_t));
    ASSERT(rc == 0);
    int32_t *got = (int32_t *)UnsafeVariedDictionary_SGet(dict, "uk");
    ASSERT(got != NULL);
    ASSERT(*got == 55);

    UnsafeVariedDictionary_Destroy(dict);
    PASS();
}

static void test_varied_dict_contract_upsert_overwrites(void) {
    TEST("varied dict contract: upsert overwrites existing value");
    UnsafeVariedDictionary *dict = UnsafeVariedDictionary_Create(8);

    int32_t v1 = 10, v2 = 20;
    UnsafeVariedDictionary_SSet(dict, "ow", &v1, sizeof(int32_t));
    int rc = UnsafeVariedDictionary_SUpsert(dict, "ow", &v2, sizeof(int32_t));
    ASSERT(rc == 0);
    int32_t *got = (int32_t *)UnsafeVariedDictionary_SGet(dict, "ow");
    ASSERT(got != NULL);
    ASSERT(*got == 20);

    UnsafeVariedDictionary_Destroy(dict);
    PASS();
}

static void test_varied_dict_contract_upsert_different_size(void) {
    TEST("varied dict contract: upsert with different size");
    UnsafeVariedDictionary *dict = UnsafeVariedDictionary_Create(8);

    int32_t small = 42;
    UnsafeVariedDictionary_SSet(dict, "sz", &small, sizeof(int32_t));
    ASSERT(UnsafeVariedDictionary_SGetSize(dict, "sz") == sizeof(int32_t));

    double big = 3.14;
    int rc = UnsafeVariedDictionary_SUpsert(dict, "sz", &big, sizeof(double));
    ASSERT(rc == 0);
    ASSERT(UnsafeVariedDictionary_SGetSize(dict, "sz") == sizeof(double));

    double *got = (double *)UnsafeVariedDictionary_SGet(dict, "sz");
    ASSERT(got != NULL);
    ASSERT(*got > 3.13 && *got < 3.15);

    UnsafeVariedDictionary_Destroy(dict);
    PASS();
}

// -- Metric tests (expected to FAIL -- define target behavior) --

static void test_varied_dict_metric_remove_set_cycle_no_growth(void) {
    TEST("varied dict metric: 1000 remove+set cycles, data->count unchanged");
    UnsafeVariedDictionary *dict = UnsafeVariedDictionary_Create(8);

    int32_t val = 0;
    UnsafeVariedDictionary_SSet(dict, "cyc", &val, sizeof(int32_t));
    uint32_t baseline = dict->data->count;

    for (int i = 0; i < 1000; i++) {
        UnsafeVariedDictionary_SRemove(dict, "cyc");
        val = i;
        UnsafeVariedDictionary_SSet(dict, "cyc", &val, sizeof(int32_t));
    }

    ASSERT(dict->data->count == baseline);
    UnsafeVariedDictionary_Destroy(dict);
    PASS();
}

static void test_varied_dict_metric_upsert_cycle_no_growth(void) {
    TEST("varied dict metric: 1000 same-size upserts, data->count unchanged");
    UnsafeVariedDictionary *dict = UnsafeVariedDictionary_Create(8);

    int32_t val = 0;
    UnsafeVariedDictionary_SSet(dict, "ups", &val, sizeof(int32_t));
    uint32_t baseline = dict->data->count;

    for (int i = 0; i < 1000; i++) {
        val = i;
        UnsafeVariedDictionary_SUpsert(dict, "ups", &val, sizeof(int32_t));
    }

    ASSERT(dict->data->count == baseline);
    UnsafeVariedDictionary_Destroy(dict);
    PASS();
}

static void test_varied_dict_metric_oscillate_no_growth(void) {
    TEST("varied dict metric: 1000 oscillating 4b/8b upserts, data->count unchanged");
    UnsafeVariedDictionary *dict = UnsafeVariedDictionary_Create(8);

    // Seed with 8-byte value (largest oscillation size)
    double big = 1.0;
    UnsafeVariedDictionary_SSet(dict, "osc", &big, sizeof(double));
    uint32_t baseline = dict->data->count;

    for (int i = 0; i < 1000; i++) {
        if (i % 2 == 0) {
            int32_t small = i;
            UnsafeVariedDictionary_SUpsert(dict, "osc", &small, sizeof(int32_t));
        } else {
            double large = (double)i;
            UnsafeVariedDictionary_SUpsert(dict, "osc", &large, sizeof(double));
        }
    }

    ASSERT(dict->data->count == baseline);
    UnsafeVariedDictionary_Destroy(dict);
    PASS();
}

// -- Layout tests --

static void test_varied_dict_layout_bytes(void) {
    TEST("varied dict layout: verify raw bytes at data offset");
    UnsafeVariedDictionary *dict = UnsafeVariedDictionary_Create(8);

    uint8_t payload[3] = {0xAA, 0xBB, 0xCC};
    UnsafeVariedDictionary_SSet(dict, "k", payload, 3);

    // Walk the trie to find the node and entry
    int32_t node_idx = UnsafeVariedDictionary_Walk(dict, "k", 1, 0);
    ASSERT(node_idx != UNSAFEDICT_EMPTY);
    UnsafeDictNode *node = (UnsafeDictNode *)UnsafeArray_Get(dict->nodes, (uint32_t)node_idx);
    ASSERT(node->value != UNSAFEDICT_EMPTY);
    UnsafeVariedEntry *entry = (UnsafeVariedEntry *)UnsafeArray_Get(dict->entries, (uint32_t)node->value);
    ASSERT(entry->size == 3);

    uint8_t expected[3] = {0xAA, 0xBB, 0xCC};
    ASSERT(_verify_bytes_vd((uint8_t *)dict->data->data, entry->offset, expected, 3));

    UnsafeVariedDictionary_Destroy(dict);
    PASS();
}

static void test_varied_dict_layout_upsert_in_place(void) {
    TEST("varied dict layout: upsert same size reuses offset");
    UnsafeVariedDictionary *dict = UnsafeVariedDictionary_Create(8);

    int32_t v1 = 0x11223344;
    UnsafeVariedDictionary_SSet(dict, "k", &v1, sizeof(int32_t));

    // Record original offset
    int32_t node_idx = UnsafeVariedDictionary_Walk(dict, "k", 1, 0);
    UnsafeDictNode *node = (UnsafeDictNode *)UnsafeArray_Get(dict->nodes, (uint32_t)node_idx);
    UnsafeVariedEntry *entry = (UnsafeVariedEntry *)UnsafeArray_Get(dict->entries, (uint32_t)node->value);
    uint32_t original_offset = entry->offset;

    // Upsert same size
    int32_t v2 = 0x55667788;
    int rc = UnsafeVariedDictionary_SUpsert(dict, "k", &v2, sizeof(int32_t));
    ASSERT(rc == 0);

    // Re-fetch entry (pointers may have moved)
    node_idx = UnsafeVariedDictionary_Walk(dict, "k", 1, 0);
    node = (UnsafeDictNode *)UnsafeArray_Get(dict->nodes, (uint32_t)node_idx);
    entry = (UnsafeVariedEntry *)UnsafeArray_Get(dict->entries, (uint32_t)node->value);

    ASSERT(entry->offset == original_offset);
    ASSERT(entry->size == sizeof(int32_t));

    // Verify new bytes
    uint8_t *data_ptr = (uint8_t *)dict->data->data + entry->offset;
    int32_t stored;
    memcpy(&stored, data_ptr, sizeof(int32_t));
    ASSERT(stored == 0x55667788);

    UnsafeVariedDictionary_Destroy(dict);
    PASS();
}

static void test_varied_dict_layout_remove_set_reuse(void) {
    TEST("varied dict layout: remove+set same size reuses data offset");
    UnsafeVariedDictionary *dict = UnsafeVariedDictionary_Create(8);

    int32_t v1 = 0xAABBCCDD;
    UnsafeVariedDictionary_SSet(dict, "k", &v1, sizeof(int32_t));

    // Record original offset
    int32_t node_idx = UnsafeVariedDictionary_Walk(dict, "k", 1, 0);
    UnsafeDictNode *node = (UnsafeDictNode *)UnsafeArray_Get(dict->nodes, (uint32_t)node_idx);
    UnsafeVariedEntry *entry = (UnsafeVariedEntry *)UnsafeArray_Get(dict->entries, (uint32_t)node->value);
    uint32_t original_offset = entry->offset;

    UnsafeVariedDictionary_SRemove(dict, "k");

    // Re-set with same size
    int32_t v2 = 0x11223344;
    UnsafeVariedDictionary_SSet(dict, "k", &v2, sizeof(int32_t));

    // Verify new value at same data offset
    node_idx = UnsafeVariedDictionary_Walk(dict, "k", 1, 0);
    node = (UnsafeDictNode *)UnsafeArray_Get(dict->nodes, (uint32_t)node_idx);
    entry = (UnsafeVariedEntry *)UnsafeArray_Get(dict->entries, (uint32_t)node->value);

    // The entry slot is reused, but data offset reuse requires data_free_list (not yet implemented)
    ASSERT(entry->offset == original_offset);

    uint8_t *data_ptr = (uint8_t *)dict->data->data + entry->offset;
    int32_t stored;
    memcpy(&stored, data_ptr, sizeof(int32_t));
    ASSERT(stored == 0x11223344);

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

    // Contract tests
    test_varied_dict_contract_set_get_remove_has();
    test_varied_dict_contract_getsize();
    test_varied_dict_contract_foreach_trie_order();

    // Contract tests (Upsert -- expected to FAIL until Upsert is implemented)
    test_varied_dict_contract_upsert_creates();
    test_varied_dict_contract_upsert_overwrites();
    test_varied_dict_contract_upsert_different_size();

    // Metric tests (expected to FAIL -- define target behavior)
    test_varied_dict_metric_remove_set_cycle_no_growth();
    test_varied_dict_metric_upsert_cycle_no_growth();
    test_varied_dict_metric_oscillate_no_growth();

    // Layout tests
    test_varied_dict_layout_bytes();
    test_varied_dict_layout_upsert_in_place();
    test_varied_dict_layout_remove_set_reuse();
}
