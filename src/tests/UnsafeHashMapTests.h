#pragma once

#include "../system/cts/UnsafeDictionary.h" // for _UNSAFE_STRLITERAL_LEN
#include "../system/cts/UnsafeHashMap.h"
#include "../system/tests.h"

static void test_hashmap_create_destroy(void) {
    TEST("hashmap: create and destroy");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    ASSERT(map != NULL);
    ASSERT(map->values != NULL);
    ASSERT(map->entry_count == 0);
    ASSERT(map->bucket_count >= 8);
    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hashmap_set_and_get(void) {
    TEST("hashmap: set and get string keys");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    int v1 = 100, v2 = 200, v3 = 300;
    ASSERT(UnsafeHashMap_Set(map, "health", 6, &v1) == 0);
    ASSERT(UnsafeHashMap_Set(map, "mana", 4, &v2) == 0);
    ASSERT(UnsafeHashMap_Set(map, "armor", 5, &v3) == 0);
    int *r1 = (int *)UnsafeHashMap_Get(map, "health", 6);
    int *r2 = (int *)UnsafeHashMap_Get(map, "mana", 4);
    int *r3 = (int *)UnsafeHashMap_Get(map, "armor", 5);
    ASSERT(r1 != NULL && *r1 == 100);
    ASSERT(r2 != NULL && *r2 == 200);
    ASSERT(r3 != NULL && *r3 == 300);
    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hashmap_getvalue_setvalue(void) {
    TEST("hashmap: GetValue and SetValue macros");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    UnsafeHashMap_SetValue(map, "foo", 3, int, 42);
    UnsafeHashMap_SetValue(map, "bar", 3, int, 99);
    ASSERT(UnsafeHashMap_GetDeref(map, "foo", 3, int) == 42);
    ASSERT(UnsafeHashMap_GetDeref(map, "bar", 3, int) == 99);
    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hashmap_duplicate_key_rejected(void) {
    TEST("hashmap: duplicate key returns -1");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    int v = 10;
    ASSERT(UnsafeHashMap_Set(map, "key", 3, &v) == 0);
    v = 20;
    ASSERT(UnsafeHashMap_Set(map, "key", 3, &v) == -1);
    ASSERT(UnsafeHashMap_GetDeref(map, "key", 3, int) == 10);
    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hashmap_get_missing(void) {
    TEST("hashmap: get nonexistent key returns NULL");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    ASSERT(UnsafeHashMap_Get(map, "nope", 4) == NULL);
    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hashmap_has(void) {
    TEST("hashmap: has returns 1/0");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    int v = 1;
    UnsafeHashMap_Set(map, "yes", 3, &v);
    ASSERT(UnsafeHashMap_Has(map, "yes", 3) == 1);
    ASSERT(UnsafeHashMap_Has(map, "no", 2) == 0);
    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hashmap_remove(void) {
    TEST("hashmap: remove clears key");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    int v = 55;
    UnsafeHashMap_Set(map, "tmp", 3, &v);
    ASSERT(UnsafeHashMap_Has(map, "tmp", 3) == 1);
    ASSERT(UnsafeHashMap_Remove(map, "tmp", 3) == 0);
    ASSERT(UnsafeHashMap_Has(map, "tmp", 3) == 0);
    ASSERT(UnsafeHashMap_Get(map, "tmp", 3) == NULL);
    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hashmap_remove_missing(void) {
    TEST("hashmap: remove nonexistent key returns -1");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    ASSERT(UnsafeHashMap_Remove(map, "ghost", 5) == -1);
    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hashmap_integer_keys(void) {
    TEST("hashmap: integer keys");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 16);
    for (int i = 0; i < 10; i++) {
        uint32_t key = (uint32_t)i;
        int val = i * 100;
        ASSERT(UnsafeHashMap_Set(map, &key, sizeof(key), &val) == 0);
    }
    for (int i = 0; i < 10; i++) {
        uint32_t key = (uint32_t)i;
        int *val = (int *)UnsafeHashMap_Get(map, &key, sizeof(key));
        ASSERT(val != NULL && *val == i * 100);
    }
    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hashmap_zero_length_key(void) {
    TEST("hashmap: zero-length key");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    int v = 999;
    ASSERT(UnsafeHashMap_Set(map, "", 0, &v) == 0);
    ASSERT(UnsafeHashMap_GetDeref(map, "", 0, int) == 999);
    ASSERT(UnsafeHashMap_Get(map, "a", 1) == NULL);
    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hashmap_modify_in_place(void) {
    TEST("hashmap: modify value in place via Get pointer");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    int v = 10;
    UnsafeHashMap_Set(map, "hp", 2, &v);
    int *ptr = (int *)UnsafeHashMap_Get(map, "hp", 2);
    *ptr = 80;
    ASSERT(UnsafeHashMap_GetDeref(map, "hp", 2, int) == 80);
    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hashmap_many_entries(void) {
    TEST("hashmap: many entries stress test");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 4);
    char key[16];
    for (int i = 0; i < 500; i++) {
        int len = snprintf(key, sizeof(key), "k%d", i);
        ASSERT(UnsafeHashMap_Set(map, key, (uint32_t)len, &i) == 0);
    }
    for (int i = 0; i < 500; i++) {
        int len = snprintf(key, sizeof(key), "k%d", i);
        int *val = (int *)UnsafeHashMap_Get(map, key, (uint32_t)len);
        ASSERT(val != NULL && *val == i);
    }
    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hashmap_struct_values(void) {
    TEST("hashmap: struct values");
    typedef struct { float x; float y; } Vec2;
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(Vec2), 8);
    Vec2 pos = {1.5f, 2.5f};
    UnsafeHashMap_Set(map, "player", 6, &pos);
    Vec2 *r = (Vec2 *)UnsafeHashMap_Get(map, "player", 6);
    ASSERT(r != NULL && r->x == 1.5f && r->y == 2.5f);
    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hashmap_logf_string_keys(void) {
    TEST("hashmap: LogF with string keys");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    UnsafeHashMap_SetValue(map, "health", 6, int, 100);
    UnsafeHashMap_SetValue(map, "mana", 4, int, 50);
    UnsafeHashMap_SetValue(map, "armor", 5, int, 25);
    UnsafeHashMap_LogF(map, int, "%d", 1);
    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hashmap_logf_hex_keys(void) {
    TEST("hashmap: LogF with hex keys");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    for (int i = 0; i < 3; i++) {
        uint32_t key = (uint32_t)(i + 1);
        int val = (i + 1) * 100;
        UnsafeHashMap_Set(map, &key, sizeof(key), &val);
    }
    UnsafeHashMap_LogF(map, int, "%d", 0);
    UnsafeHashMap_Destroy(map);
    PASS();
}

static void _fmt_hashmap_int(const void *value, char *buf, uint32_t buf_size) {
    snprintf(buf, buf_size, "%d", *(const int *)value);
}

static void test_hashmap_log_callback(void) {
    TEST("hashmap: Log with callback");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    UnsafeHashMap_SetValue(map, "atk", 3, int, 55);
    UnsafeHashMap_SetValue(map, "def", 3, int, 30);
    UnsafeHashMap_Log(map, _fmt_hashmap_int, 1);
    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hashmap_string_macros(void) {
    TEST("hashmap: S-macros for string keys");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    UnsafeHashMap_SSetValue(map, "health", int, 100);
    UnsafeHashMap_SSetValue(map, "mana", int, 50);

    ASSERT(UnsafeHashMap_SHas(map, "health") == 1);
    ASSERT(UnsafeHashMap_SHas(map, "nope") == 0);
    ASSERT(UnsafeHashMap_SGetDeref(map, "health", int) == 100);
    ASSERT(UnsafeHashMap_SGetDeref(map, "mana", int) == 50);

    int *ptr = (int *)UnsafeHashMap_SGet(map, "health");
    ASSERT(ptr != NULL && *ptr == 100);

    int v = 25;
    ASSERT(UnsafeHashMap_SSet(map, "armor", &v) == 0);
    ASSERT(UnsafeHashMap_SGetDeref(map, "armor", int) == 25);

    ASSERT(UnsafeHashMap_SRemove(map, "mana") == 0);
    ASSERT(UnsafeHashMap_SHas(map, "mana") == 0);

    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hashmap_remove_then_reinsert(void) {
    TEST("hashmap: remove then reinsert same key");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    int v1 = 10, v2 = 20;
    ASSERT(UnsafeHashMap_Set(map, "key", 3, &v1) == 0);
    ASSERT(UnsafeHashMap_Remove(map, "key", 3) == 0);
    ASSERT(UnsafeHashMap_Set(map, "key", 3, &v2) == 0);
    ASSERT(UnsafeHashMap_GetDeref(map, "key", 3, int) == 20);
    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hashmap_collision_chain(void) {
    TEST("hashmap: lookups work after rehash");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 4);
    // Insert enough entries to force at least one rehash
    char key[32];
    for (int i = 0; i < 100; i++) {
        int len = snprintf(key, sizeof(key), "item_%d", i);
        ASSERT(UnsafeHashMap_Set(map, key, (uint32_t)len, &i) == 0);
    }
    // Verify all still accessible
    for (int i = 0; i < 100; i++) {
        int len = snprintf(key, sizeof(key), "item_%d", i);
        int *val = (int *)UnsafeHashMap_Get(map, key, (uint32_t)len);
        ASSERT(val != NULL && *val == i);
    }
    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hashmap_remove_reinsert_reuses_slot(void) {
    TEST("hashmap: remove then reinsert reuses value slot");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    int v1 = 10, v2 = 20, v3 = 30;
    UnsafeHashMap_Set(map, "a", 1, &v1);
    UnsafeHashMap_Set(map, "b", 1, &v2);
    uint32_t count_before = map->values->count;
    ASSERT(count_before == 2);

    ASSERT(UnsafeHashMap_Remove(map, "a", 1) == 0);
    ASSERT(map->free_list->count == 1);

    ASSERT(UnsafeHashMap_Set(map, "a", 1, &v3) == 0);
    ASSERT(map->values->count == count_before);
    ASSERT(map->free_list->count == 0);
    ASSERT(UnsafeHashMap_GetDeref(map, "a", 1, int) == 30);
    ASSERT(UnsafeHashMap_GetDeref(map, "b", 1, int) == 20);

    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hashmap_remove_reinsert_many(void) {
    TEST("hashmap: repeated remove/reinsert cycle keeps values array bounded");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 16);

    char key[8];
    for (int i = 0; i < 10; i++) {
        int len = snprintf(key, sizeof(key), "k%d", i);
        UnsafeHashMap_Set(map, key, (uint32_t)len, &i);
    }
    uint32_t baseline = map->values->count;
    ASSERT(baseline == 10);

    for (int i = 0; i < 10; i++) {
        int len = snprintf(key, sizeof(key), "k%d", i);
        UnsafeHashMap_Remove(map, key, (uint32_t)len);
    }
    ASSERT(map->free_list->count == 10);

    for (int i = 0; i < 10; i++) {
        int len = snprintf(key, sizeof(key), "k%d", i);
        int val = i + 100;
        UnsafeHashMap_Set(map, key, (uint32_t)len, &val);
    }
    ASSERT(map->values->count == baseline);
    ASSERT(map->free_list->count == 0);

    for (int i = 0; i < 10; i++) {
        int len = snprintf(key, sizeof(key), "k%d", i);
        ASSERT(UnsafeHashMap_GetDeref(map, key, (uint32_t)len, int) == i + 100);
    }

    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hashmap_getderef_missing_key(void) {
    TEST("hashmap: GetDeref on missing key returns 0");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    UnsafeHashMap_SetValue(map, "a", 1, int, 42);
    ASSERT(UnsafeHashMap_GetDeref(map, "b", 1, int) == 0);
    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hashmap_set_duplicate_key(void) {
    TEST("hashmap: Set same key twice returns -1 on second call");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    int v1 = 10, v2 = 20;
    ASSERT(UnsafeHashMap_Set(map, "dup", 3, &v1) == 0);
    ASSERT(UnsafeHashMap_Set(map, "dup", 3, &v2) == -1);
    ASSERT(UnsafeHashMap_GetDeref(map, "dup", 3, int) == 10);
    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hashmap_create_null_on_zero(void) {
    TEST("hashmap: Create with zero capacity does not crash");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 0);
    if (map != NULL) {
        UnsafeHashMap_Destroy(map);
    }
    PASS();
}

static void test_varied_hashmap_getderef_missing_key(void) {
    TEST("varied hashmap: GetDeref on missing key returns 0");
    UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);
    int val = 42;
    UnsafeVariedHashMap_Set(map, "a", 1, &val, sizeof(int));
    ASSERT(UnsafeVariedHashMap_GetDeref(map, "b", 1, int) == 0);
    UnsafeVariedHashMap_Destroy(map);
    PASS();
}

// ============================================================
// Helpers for new tests
// ============================================================

static int _verify_bytes_hm(uint8_t *buf, uint32_t offset, const uint8_t *expected, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        if (buf[offset + i] != expected[i]) return 0;
    }
    return 1;
}

static int _hm_foreach_count = 0;
static void _hm_foreach_counter(const void *key, uint32_t key_len, void *value) {
    (void)key; (void)key_len; (void)value;
    _hm_foreach_count++;
}

// ============================================================
// Contract tests (should PASS)
// ============================================================

static void test_hm_contract_set_duplicate_neg1(void) {
    TEST("hashmap contract: Set duplicate returns -1, value unchanged");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    int val = 42, val2 = 99;
    ASSERT(UnsafeHashMap_SSet(map, "key", &val) == 0);
    ASSERT(UnsafeHashMap_SSet(map, "key", &val2) == -1);
    ASSERT(UnsafeHashMap_SGetDeref(map, "key", int) == 42);
    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hm_contract_remove_then_get_null(void) {
    TEST("hashmap contract: Remove then Get returns NULL");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    int val = 42;
    ASSERT(UnsafeHashMap_SSet(map, "key", &val) == 0);
    ASSERT(UnsafeHashMap_SRemove(map, "key") == 0);
    ASSERT(UnsafeHashMap_SGet(map, "key") == NULL);
    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hm_contract_foreach_visits_all(void) {
    TEST("hashmap contract: ForEach visits all 5 entries");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    int v1 = 1, v2 = 2, v3 = 3, v4 = 4, v5 = 5;
    UnsafeHashMap_SSet(map, "a", &v1);
    UnsafeHashMap_SSet(map, "b", &v2);
    UnsafeHashMap_SSet(map, "c", &v3);
    UnsafeHashMap_SSet(map, "d", &v4);
    UnsafeHashMap_SSet(map, "e", &v5);
    _hm_foreach_count = 0;
    UnsafeHashMap_ForEach(map, _hm_foreach_counter);
    ASSERT(_hm_foreach_count == 5);
    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hm_contract_rehash_transparent(void) {
    TEST("hashmap contract: 100 inserts survive rehash");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 4);
    char key[32];
    for (int i = 0; i < 100; i++) {
        int len = snprintf(key, sizeof(key), "key_%d", i);
        ASSERT(UnsafeHashMap_Set(map, key, (uint32_t)len, &i) == 0);
    }
    for (int i = 0; i < 100; i++) {
        int len = snprintf(key, sizeof(key), "key_%d", i);
        int *val = (int *)UnsafeHashMap_Get(map, key, (uint32_t)len);
        ASSERT(val != NULL && *val == i);
    }
    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hm_contract_remove_nonexistent(void) {
    TEST("hashmap contract: Remove nonexistent returns -1");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    ASSERT(UnsafeHashMap_SRemove(map, "never_set") == -1);
    UnsafeHashMap_Destroy(map);
    PASS();
}

// ============================================================
// Contract tests (will FAIL -- Upsert is a stub)
// ============================================================

static void test_hm_contract_upsert_creates(void) {
    TEST("hashmap contract: Upsert creates new entry");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    int val = 42;
    ASSERT(UnsafeHashMap_SUpsert(map, "new_key", &val) == 0);
    ASSERT(UnsafeHashMap_SGet(map, "new_key") != NULL);
    ASSERT(UnsafeHashMap_SGetDeref(map, "new_key", int) == 42);
    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hm_contract_upsert_overwrites(void) {
    TEST("hashmap contract: Upsert overwrites existing value");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    int v1 = 42, v2 = 99;
    ASSERT(UnsafeHashMap_SSet(map, "key", &v1) == 0);
    uint32_t count_before = map->entry_count;
    ASSERT(UnsafeHashMap_SUpsert(map, "key", &v2) == 0);
    ASSERT(UnsafeHashMap_SGetDeref(map, "key", int) == 99);
    ASSERT(map->entry_count == count_before);
    UnsafeHashMap_Destroy(map);
    PASS();
}

// ============================================================
// Metric tests (target behavior -- may FAIL)
// ============================================================

static void test_hm_metric_remove_set_reuses_slot(void) {
    TEST("hashmap metric: Remove+Set reuses values slot");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    int val = 42, val2 = 99;
    UnsafeHashMap_SSet(map, "key", &val);
    uint32_t count_after_set = map->values->count;
    UnsafeHashMap_SRemove(map, "key");
    UnsafeHashMap_SSet(map, "key2", &val2);
    ASSERT(map->values->count == count_after_set);
    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hm_metric_100_remove_add_cycles(void) {
    TEST("hashmap metric: 100 remove+add cycles, values->count unchanged");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    int v = 1;
    UnsafeHashMap_Set(map, "k", 1, &v);
    uint32_t baseline = map->values->count;
    for (int i = 0; i < 100; i++) {
        UnsafeHashMap_Remove(map, "k", 1);
        v = i + 2;
        UnsafeHashMap_Set(map, "k", 1, &v);
    }
    ASSERT(map->values->count == baseline);
    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hm_metric_upsert_no_values_growth(void) {
    TEST("hashmap metric: Upsert does not grow values array");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    int v1 = 42, v2 = 99;
    UnsafeHashMap_SSet(map, "key", &v1);
    uint32_t baseline = map->values->count;
    UnsafeHashMap_SUpsert(map, "key", &v2);
    ASSERT(map->values->count == baseline);
    UnsafeHashMap_Destroy(map);
    PASS();
}

// ============================================================
// Layout tests
// ============================================================

static void test_hm_layout_value_at_slot(void) {
    TEST("hashmap layout: value bytes at slot match Set input");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    int val = 42;
    UnsafeHashMap_SSet(map, "key", &val);

    uint32_t slot = _UnsafeHashMap_FindSlot(map, "key", _UNSAFE_STRLITERAL_LEN("key"));
    UnsafeHashEntry *e = &map->buckets[slot];
    ASSERT(e->value >= 0);

    void *stored = UnsafeArray_Get(map->values, (uint32_t)e->value);
    ASSERT(stored != NULL);
    ASSERT(_verify_bytes_hm((uint8_t *)stored, 0, (const uint8_t *)&val, sizeof(int)));

    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hm_layout_reuse_after_remove(void) {
    TEST("hashmap layout: Remove+Set reuses same values index");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    int v1 = 42, v2 = 99;
    UnsafeHashMap_SSet(map, "key", &v1);

    uint32_t slot1 = _UnsafeHashMap_FindSlot(map, "key", _UNSAFE_STRLITERAL_LEN("key"));
    int32_t idx1 = map->buckets[slot1].value;
    ASSERT(idx1 >= 0);

    UnsafeHashMap_SRemove(map, "key");
    UnsafeHashMap_SSet(map, "key2", &v2);

    uint32_t slot2 = _UnsafeHashMap_FindSlot(map, "key2", _UNSAFE_STRLITERAL_LEN("key2"));
    int32_t idx2 = map->buckets[slot2].value;
    ASSERT(idx2 == idx1);

    void *stored = UnsafeArray_Get(map->values, (uint32_t)idx2);
    ASSERT(_verify_bytes_hm((uint8_t *)stored, 0, (const uint8_t *)&v2, sizeof(int)));

    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hm_layout_upsert_in_place(void) {
    TEST("hashmap layout: Upsert overwrites at same values index");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    int v1 = 42, v2 = 99;
    UnsafeHashMap_SSet(map, "key", &v1);

    uint32_t slot1 = _UnsafeHashMap_FindSlot(map, "key", _UNSAFE_STRLITERAL_LEN("key"));
    int32_t idx1 = map->buckets[slot1].value;
    ASSERT(idx1 >= 0);

    UnsafeHashMap_SUpsert(map, "key", &v2);

    uint32_t slot2 = _UnsafeHashMap_FindSlot(map, "key", _UNSAFE_STRLITERAL_LEN("key"));
    int32_t idx2 = map->buckets[slot2].value;
    ASSERT(idx2 == idx1);

    void *stored = UnsafeArray_Get(map->values, (uint32_t)idx2);
    ASSERT(_verify_bytes_hm((uint8_t *)stored, 0, (const uint8_t *)&v2, sizeof(int)));

    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hm_metric_inline_upsert_no_values_growth(void) {
    TEST("hashmap metric: inline Remove+Set does not grow values array");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    int v1 = 42, v2 = 99;
    UnsafeHashMap_SSet(map, "key", &v1);
    uint32_t baseline = map->values->count;
    UnsafeHashMap_SRemove(map, "key");
    UnsafeHashMap_SSet(map, "key", &v2);
    ASSERT(map->values->count == baseline);
    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hm_layout_inline_upsert_reuses_slot(void) {
    TEST("hashmap layout: inline Remove+Set reuses same values index");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    int v1 = 42, v2 = 99;
    UnsafeHashMap_SSet(map, "key", &v1);

    uint32_t slot1 = _UnsafeHashMap_FindSlot(map, "key", _UNSAFE_STRLITERAL_LEN("key"));
    int32_t idx1 = map->buckets[slot1].value;
    ASSERT(idx1 >= 0);

    UnsafeHashMap_SRemove(map, "key");
    UnsafeHashMap_SSet(map, "key", &v2);

    uint32_t slot2 = _UnsafeHashMap_FindSlot(map, "key", _UNSAFE_STRLITERAL_LEN("key"));
    int32_t idx2 = map->buckets[slot2].value;
    ASSERT(idx2 == idx1);

    void *stored = UnsafeArray_Get(map->values, (uint32_t)idx2);
    ASSERT(_verify_bytes_hm((uint8_t *)stored, 0, (const uint8_t *)&v2, sizeof(int)));

    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hm_metric_1000_remove_add_cycles(void) {
    TEST("hashmap metric: 1000 remove+add cycles, values->count unchanged");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    int v = 1;
    UnsafeHashMap_Set(map, "k", 1, &v);
    uint32_t baseline = map->values->count;
    for (int i = 0; i < 1000; i++) {
        UnsafeHashMap_Remove(map, "k", 1);
        v = i + 2;
        UnsafeHashMap_Set(map, "k", 1, &v);
    }
    ASSERT(map->values->count == baseline);
    ASSERT(UnsafeHashMap_GetDeref(map, "k", 1, int) == 1001);
    UnsafeHashMap_Destroy(map);
    PASS();
}

static void test_hm_metric_1000_upsert_cycles(void) {
    TEST("hashmap metric: 1000 upsert cycles, values->count unchanged");
    UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
    int v = 0;
    UnsafeHashMap_Set(map, "k", 1, &v);
    uint32_t baseline = map->values->count;
    for (int i = 0; i < 1000; i++) {
        v = i;
        UnsafeHashMap_Upsert(map, "k", 1, &v);
    }
    ASSERT(map->values->count == baseline);
    ASSERT(UnsafeHashMap_GetDeref(map, "k", 1, int) == 999);
    UnsafeHashMap_Destroy(map);
    PASS();
}

static void run_unsafe_hashmap_tests(void) {
    LOG_INFO("=== UnsafeHashMap Tests ===");
    test_hashmap_create_destroy();
    test_hashmap_set_and_get();
    test_hashmap_getvalue_setvalue();
    test_hashmap_duplicate_key_rejected();
    test_hashmap_get_missing();
    test_hashmap_has();
    test_hashmap_remove();
    test_hashmap_remove_missing();
    test_hashmap_integer_keys();
    test_hashmap_zero_length_key();
    test_hashmap_modify_in_place();
    test_hashmap_many_entries();
    test_hashmap_struct_values();
    test_hashmap_logf_string_keys();
    test_hashmap_logf_hex_keys();
    test_hashmap_log_callback();
    test_hashmap_string_macros();
    test_hashmap_remove_then_reinsert();
    test_hashmap_collision_chain();
    test_hashmap_remove_reinsert_reuses_slot();
    test_hashmap_remove_reinsert_many();
    test_hashmap_getderef_missing_key();
    test_hashmap_set_duplicate_key();
    test_hashmap_create_null_on_zero();
    test_varied_hashmap_getderef_missing_key();

    // Contract tests
    test_hm_contract_set_duplicate_neg1();
    test_hm_contract_remove_then_get_null();
    test_hm_contract_foreach_visits_all();
    test_hm_contract_rehash_transparent();
    test_hm_contract_remove_nonexistent();
    test_hm_contract_upsert_creates();
    test_hm_contract_upsert_overwrites();

    // Metric tests
    test_hm_metric_remove_set_reuses_slot();
    test_hm_metric_100_remove_add_cycles();
    test_hm_metric_upsert_no_values_growth();
    test_hm_metric_inline_upsert_no_values_growth();
    test_hm_metric_1000_remove_add_cycles();
    test_hm_metric_1000_upsert_cycles();

    // Layout tests
    test_hm_layout_value_at_slot();
    test_hm_layout_reuse_after_remove();
    test_hm_layout_upsert_in_place();
    test_hm_layout_inline_upsert_reuses_slot();
}
