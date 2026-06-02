#pragma once

#include "UnsafeArray.h"

#define UNSAFEHASHMAP_MAX_KEY_LEN 256
#define UNSAFEHASHMAP_EMPTY (-1)
// Tombstone marker for deleted entries -- linear probing requires
// tombstones to avoid breaking probe chains. Without them, a deleted
// slot would terminate probes early and hide entries inserted after it.
#define UNSAFEHASHMAP_DELETED (-2)
#define UNSAFEHASHMAP_DEFAULT_CAPACITY 64
#define UNSAFEHASHMAP_LOAD_FACTOR_NUM 7
#define UNSAFEHASHMAP_LOAD_FACTOR_DEN 10

typedef struct UnsafeHashEntry {
    void *key;
    uint32_t key_len;
    int32_t value;  // index into values array, or UNSAFEHASHMAP_EMPTY / DELETED
} UnsafeHashEntry;

typedef struct UnsafeHashMap {
    UnsafeHashEntry *buckets;
    uint32_t bucket_count;
    uint32_t entry_count;
    UnsafeArray *values;
    UnsafeArray *free_list; // int32_t indices of freed value slots
} UnsafeHashMap;

// FNV-1a: good distribution for short keys, no external dependencies,
// and the XOR-before-multiply variant reduces clustering on similar inputs.
static uint32_t _UnsafeHashMap_Hash(const void *key, uint32_t key_len) {
    const uint8_t *bytes = (const uint8_t *)key;
    uint32_t hash = 2166136261u;
    for (uint32_t i = 0; i < key_len; i++) {
        hash ^= bytes[i];
        hash *= 16777619u;
    }
    return hash;
}

static void _UnsafeHashMap_InitBuckets(UnsafeHashEntry *buckets, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        buckets[i].key = NULL;
        buckets[i].key_len = 0;
        buckets[i].value = UNSAFEHASHMAP_EMPTY;
    }
}

static UnsafeHashMap *UnsafeHashMap_Create(uint32_t element_size, uint32_t capacity) {
    UnsafeHashMap *map = (UnsafeHashMap *)malloc(sizeof(UnsafeHashMap));
    if (!map) return NULL;
    uint32_t bucket_count = UNSAFEHASHMAP_DEFAULT_CAPACITY;
    while (bucket_count < capacity * 2) bucket_count *= 2;
    map->buckets = (UnsafeHashEntry *)malloc(sizeof(UnsafeHashEntry) * bucket_count);
    if (!map->buckets) { free(map); return NULL; }
    map->bucket_count = bucket_count;
    map->entry_count = 0;
    map->values = UnsafeArray_Create(element_size, capacity);
    map->free_list = UnsafeArray_Create(sizeof(int32_t), 8);
    _UnsafeHashMap_InitBuckets(map->buckets, bucket_count);
    return map;
}

static void UnsafeHashMap_Destroy(UnsafeHashMap *map) {
    for (uint32_t i = 0; i < map->bucket_count; i++) {
        if (map->buckets[i].key != NULL) {
            free(map->buckets[i].key);
        }
    }
    free(map->buckets);
    UnsafeArray_Destroy(map->values);
    UnsafeArray_Destroy(map->free_list);
    free(map);
}

// Linear probe search. Returns the first usable slot (matching key, first
// tombstone, or first empty). Load factor control guarantees termination.
static uint32_t _UnsafeHashMap_FindSlot(UnsafeHashMap *map, const void *key, uint32_t key_len) {
    uint32_t hash = _UnsafeHashMap_Hash(key, key_len);
    uint32_t idx = hash & (map->bucket_count - 1);
    uint32_t first_deleted = map->bucket_count; // sentinel

    for (uint32_t i = 0; i < map->bucket_count; i++) {
        uint32_t probe = (idx + i) & (map->bucket_count - 1);
        UnsafeHashEntry *e = &map->buckets[probe];

        if (e->value == UNSAFEHASHMAP_EMPTY) {
            // Empty slot -- key not in table
            return (first_deleted < map->bucket_count) ? first_deleted : probe;
        }

        if (e->value == UNSAFEHASHMAP_DELETED) {
            if (first_deleted == map->bucket_count) first_deleted = probe;
            continue;
        }

        // Occupied -- check key match
        if (e->key_len == key_len && memcmp(e->key, key, key_len) == 0) {
            return probe;
        }
    }

    return (first_deleted < map->bucket_count) ? first_deleted : map->bucket_count;
}

static void _UnsafeHashMap_Rehash(UnsafeHashMap *map) {
    uint32_t old_count = map->bucket_count;
    UnsafeHashEntry *old_buckets = map->buckets;

    if (old_count > UINT32_MAX / 2) return; // can't grow further
    uint32_t new_count = old_count * 2;
    UnsafeHashEntry *new_buckets = (UnsafeHashEntry *)malloc(sizeof(UnsafeHashEntry) * new_count);
    if (!new_buckets) return; // leave map unchanged
    map->buckets = new_buckets;
    map->bucket_count = new_count;
    _UnsafeHashMap_InitBuckets(map->buckets, new_count);

    for (uint32_t i = 0; i < old_count; i++) {
        UnsafeHashEntry *old = &old_buckets[i];
        if (old->value < 0) {
            // EMPTY or DELETED -- free key if any (deleted entries have keys)
            if (old->key != NULL) free(old->key);
            continue;
        }
        // Re-insert into new table
        uint32_t slot = _UnsafeHashMap_FindSlot(map, old->key, old->key_len);
        map->buckets[slot] = *old; // transfer ownership of key allocation
    }

    free(old_buckets);
}

// Returns -1 if key exceeds max length or already exists (no overwrite).
static int UnsafeHashMap_Set(UnsafeHashMap *map, const void *key, uint32_t key_len, const void *value) {
    if (key_len > UNSAFEHASHMAP_MAX_KEY_LEN) return -1;

    // Check load factor before insert
    if ((uint64_t)(map->entry_count + 1) * UNSAFEHASHMAP_LOAD_FACTOR_DEN >
        (uint64_t)map->bucket_count * UNSAFEHASHMAP_LOAD_FACTOR_NUM) {
        _UnsafeHashMap_Rehash(map);
    }

    uint32_t slot = _UnsafeHashMap_FindSlot(map, key, key_len);
    if (slot == map->bucket_count) return -1; // table full (sentinel)
    UnsafeHashEntry *e = &map->buckets[slot];

    // Already exists?
    if (e->value >= 0 && e->key_len == key_len && memcmp(e->key, key, key_len) == 0) {
        return -1;
    }

    // Insert
    e->key = malloc(key_len);
    if (!e->key) return -1;
    memcpy(e->key, key, key_len);
    e->key_len = key_len;

    if (map->free_list->count > 0) {
        int32_t reuse = UnsafeArray_GetDeref(map->free_list, map->free_list->count - 1, int32_t);
        map->free_list->count--;
        UnsafeArray_Set(map->values, (uint32_t)reuse, value);
        e->value = reuse;
    } else {
        e->value = (int32_t)map->values->count;
        UnsafeArray_Add(map->values, value);
    }
    map->entry_count++;
    return 0;
}

static void *UnsafeHashMap_Get(UnsafeHashMap *map, const void *key, uint32_t key_len) {
    uint32_t slot = _UnsafeHashMap_FindSlot(map, key, key_len);
    if (slot == map->bucket_count) return NULL;
    UnsafeHashEntry *e = &map->buckets[slot];

    if (e->value < 0) return NULL;
    if (e->key_len != key_len || memcmp(e->key, key, key_len) != 0) return NULL;

    return UnsafeArray_Get(map->values, (uint32_t)e->value);
}

static int UnsafeHashMap_Has(UnsafeHashMap *map, const void *key, uint32_t key_len) {
    return UnsafeHashMap_Get(map, key, key_len) != NULL;
}

// Marks slot as DELETED (tombstone) to preserve probe chains.
static int UnsafeHashMap_Remove(UnsafeHashMap *map, const void *key, uint32_t key_len) {
    uint32_t slot = _UnsafeHashMap_FindSlot(map, key, key_len);
    if (slot == map->bucket_count) return -1;
    UnsafeHashEntry *e = &map->buckets[slot];

    if (e->value < 0) return -1;
    if (e->key_len != key_len || memcmp(e->key, key, key_len) != 0) return -1;

    int32_t freed_slot = e->value;
    UnsafeArray_Add(map->free_list, &freed_slot);
    free(e->key);
    e->key = NULL;
    e->key_len = 0;
    e->value = UNSAFEHASHMAP_DELETED;
    map->entry_count--;
    return 0;
}

static int UnsafeHashMap_Upsert(UnsafeHashMap *map, const void *key, uint32_t key_len, const void *value) {
    uint32_t slot = _UnsafeHashMap_FindSlot(map, key, key_len);
    if (slot == map->bucket_count) return -1;
    UnsafeHashEntry *e = &map->buckets[slot];
    if (e->value >= 0 && e->key_len == key_len && memcmp(e->key, key, key_len) == 0) {
        memcpy(UnsafeArray_Get(map->values, (uint32_t)e->value), value, map->values->element_size);
        return 0;
    }
    return UnsafeHashMap_Set(map, key, key_len, value);
}

typedef void (*UnsafeHashMapForEachFn)(const void *key, uint32_t key_len, void *value);

static void UnsafeHashMap_ForEach(UnsafeHashMap *map, UnsafeHashMapForEachFn fn) {
    for (uint32_t i = 0; i < map->bucket_count; i++) {
        UnsafeHashEntry *e = &map->buckets[i];
        if (e->value < 0) continue;
        fn(e->key, e->key_len, UnsafeArray_Get(map->values, (uint32_t)e->value));
    }
}

#define UnsafeHashMap_GetDeref(map, key, key_len, type) ({ \
    void *_uhm_gd_ptr = UnsafeHashMap_Get(map, key, key_len); \
    _uhm_gd_ptr ? *(type *)_uhm_gd_ptr : (type){0}; \
})

#define UnsafeHashMap_SetValue(map, key, key_len, type, value) \
    UnsafeHashMap_Set(map, key, key_len, &(type){value})

// String literal key convenience macros -- length computed at compile time.
#define UnsafeHashMap_SSet(map, str_key, value_ptr)              UnsafeHashMap_Set(map, str_key, _UNSAFE_STRLITERAL_LEN(str_key), value_ptr)
#define UnsafeHashMap_SGet(map, str_key)                         UnsafeHashMap_Get(map, str_key, _UNSAFE_STRLITERAL_LEN(str_key))
#define UnsafeHashMap_SHas(map, str_key)                         UnsafeHashMap_Has(map, str_key, _UNSAFE_STRLITERAL_LEN(str_key))
#define UnsafeHashMap_SRemove(map, str_key)                      UnsafeHashMap_Remove(map, str_key, _UNSAFE_STRLITERAL_LEN(str_key))
#define UnsafeHashMap_SGetDeref(map, str_key, type)              UnsafeHashMap_GetDeref(map, str_key, _UNSAFE_STRLITERAL_LEN(str_key), type)
#define UnsafeHashMap_SSetValue(map, str_key, type, value)       UnsafeHashMap_SetValue(map, str_key, _UNSAFE_STRLITERAL_LEN(str_key), type, value)
#define UnsafeHashMap_SUpsert(map, str_key, value_ptr)           UnsafeHashMap_Upsert(map, str_key, _UNSAFE_STRLITERAL_LEN(str_key), value_ptr)

// _UNSAFE_STRLITERAL_LEN is defined in UnsafeDictionary.h (included via UnsafeArray.h's
// sibling). Include UnsafeDictionary.h first if using this file standalone.

typedef void (*UnsafeHashMapFormatter)(const void *value, char *buf, uint32_t buf_size);

#define LINTNORE
// string_keys=1 prints keys as strings, 0 as hex bytes.
static void UnsafeHashMap_Print(UnsafeHashMap *map, UnsafeHashMapFormatter fmt_value, int string_keys) {
    printf("UnsafeHashMap[%u entries, %u buckets] {\n", map->entry_count, map->bucket_count);
    for (uint32_t i = 0; i < map->bucket_count; i++) {
        UnsafeHashEntry *e = &map->buckets[i];
        if (e->value < 0) continue;

        char val_buf[256];
        fmt_value(UnsafeArray_Get(map->values, (uint32_t)e->value), val_buf, sizeof(val_buf));

        if (string_keys) {
            printf("  \"%.*s\" => %s\n", (int)e->key_len, (char *)e->key, val_buf);
        } else {
            printf("  [");
            for (uint32_t j = 0; j < e->key_len; j++) {
                if (j > 0) printf(" ");
                printf("%02X", ((uint8_t *)e->key)[j]);
            }
            printf("] => %s\n", val_buf);
        }
    }
    printf("}\n");
}

static const char   *_uhm_pf_fmt = NULL;
static uint32_t      _uhm_pf_esz = 0;
static void _uhm_pf_fn(const void *v, char *b, uint32_t s) {
    _unsafe_fmt_snprintf(v, b, s, _uhm_pf_fmt, _uhm_pf_esz);
}
#define UnsafeHashMap_PrintF(map, type, fmt, string_keys) do { \
    _uhm_pf_fmt = (fmt); \
    _uhm_pf_esz = (uint32_t)sizeof(type); \
    UnsafeHashMap_Print(map, _uhm_pf_fn, string_keys); \
} while (0)
#undef LINTNORE

// --- Log variants (LOG_INFO per line) ---

static void UnsafeHashMap_Log(UnsafeHashMap *map, UnsafeHashMapFormatter fmt_value, int string_keys) {
    LOG_INFO("UnsafeHashMap[%u entries, %u buckets] {", map->entry_count, map->bucket_count);
    for (uint32_t i = 0; i < map->bucket_count; i++) {
        UnsafeHashEntry *e = &map->buckets[i];
        if (e->value < 0) continue;

        char val_buf[256];
        fmt_value(UnsafeArray_Get(map->values, (uint32_t)e->value), val_buf, sizeof(val_buf));

        if (string_keys) {
            LOG_INFO("  \"%.*s\" => %s", (int)e->key_len, (char *)e->key, val_buf);
        } else {
            char hex[UNSAFEHASHMAP_MAX_KEY_LEN * 3 + 1];
            uint32_t pos = 0;
            for (uint32_t j = 0; j < e->key_len; j++) {
                if (j > 0) hex[pos++] = ' ';
                pos += (uint32_t)snprintf(hex + pos, sizeof(hex) - pos, "%02X", ((uint8_t *)e->key)[j]);
            }
            hex[pos] = '\0';
            LOG_INFO("  [%s] => %s", hex, val_buf);
        }
    }
    LOG_INFO("}");
}

static const char   *_uhm_lf_fmt = NULL;
static uint32_t      _uhm_lf_esz = 0;
static void _uhm_lf_fn(const void *v, char *b, uint32_t s) {
    _unsafe_fmt_snprintf(v, b, s, _uhm_lf_fmt, _uhm_lf_esz);
}
#define UnsafeHashMap_LogF(map, type, fmt, string_keys) do { \
    _uhm_lf_fmt = (fmt); \
    _uhm_lf_esz = (uint32_t)sizeof(type); \
    UnsafeHashMap_Log(map, _uhm_lf_fn, string_keys); \
} while (0)

// ============================================================
// UnsafeVariedHashMap -- hash map with varied value sizes.
// Same probe/tombstone/rehash design as UnsafeHashMap above,
// but stores values as variable-length byte spans.
// ============================================================

typedef struct UnsafeVariedHashEntry {
    void *key;
    uint32_t key_len;
    int32_t value;  // index into entries array, or UNSAFEHASHMAP_EMPTY / DELETED
} UnsafeVariedHashEntry;

typedef struct UnsafeVariedHashEntryInfo {
    uint32_t offset; // byte offset into the data buffer
    uint32_t size;   // size of this value in bytes
} UnsafeVariedHashEntryInfo;

typedef struct UnsafeVariedHashMap {
    UnsafeVariedHashEntry *buckets;
    uint32_t bucket_count;
    uint32_t entry_count;
    UnsafeArray *entries;   // UnsafeVariedHashEntryInfo index
    UnsafeArray *data;      // raw byte buffer (element_size = 1)
    UnsafeArray *free_list; // int32_t indices of freed entry slots (data bytes not reclaimed)
} UnsafeVariedHashMap;

static void _UnsafeVariedHashMap_InitBuckets(UnsafeVariedHashEntry *buckets, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        buckets[i].key = NULL;
        buckets[i].key_len = 0;
        buckets[i].value = UNSAFEHASHMAP_EMPTY;
    }
}

static UnsafeVariedHashMap *UnsafeVariedHashMap_Create(uint32_t capacity) {
    UnsafeVariedHashMap *map = (UnsafeVariedHashMap *)malloc(sizeof(UnsafeVariedHashMap));
    if (!map) return NULL;
    uint32_t bucket_count = UNSAFEHASHMAP_DEFAULT_CAPACITY;
    while (bucket_count < capacity * 2) bucket_count *= 2;
    map->buckets = (UnsafeVariedHashEntry *)malloc(sizeof(UnsafeVariedHashEntry) * bucket_count);
    if (!map->buckets) { free(map); return NULL; }
    map->bucket_count = bucket_count;
    map->entry_count = 0;
    map->entries = UnsafeArray_Create(sizeof(UnsafeVariedHashEntryInfo), capacity);
    map->data = UnsafeArray_Create(1, capacity * 8);
    map->free_list = UnsafeArray_Create(sizeof(int32_t), 8);
    _UnsafeVariedHashMap_InitBuckets(map->buckets, bucket_count);
    return map;
}

static void UnsafeVariedHashMap_Destroy(UnsafeVariedHashMap *map) {
    for (uint32_t i = 0; i < map->bucket_count; i++) {
        if (map->buckets[i].key != NULL) {
            free(map->buckets[i].key);
        }
    }
    free(map->buckets);
    UnsafeArray_Destroy(map->entries);
    UnsafeArray_Destroy(map->data);
    UnsafeArray_Destroy(map->free_list);
    free(map);
}

static uint32_t _UnsafeVariedHashMap_FindSlot(UnsafeVariedHashMap *map, const void *key, uint32_t key_len) {
    uint32_t hash = _UnsafeHashMap_Hash(key, key_len);
    uint32_t idx = hash & (map->bucket_count - 1);
    uint32_t first_deleted = map->bucket_count;

    for (uint32_t i = 0; i < map->bucket_count; i++) {
        uint32_t probe = (idx + i) & (map->bucket_count - 1);
        UnsafeVariedHashEntry *e = &map->buckets[probe];

        if (e->value == UNSAFEHASHMAP_EMPTY) {
            return (first_deleted < map->bucket_count) ? first_deleted : probe;
        }

        if (e->value == UNSAFEHASHMAP_DELETED) {
            if (first_deleted == map->bucket_count) first_deleted = probe;
            continue;
        }

        if (e->key_len == key_len && memcmp(e->key, key, key_len) == 0) {
            return probe;
        }
    }

    return (first_deleted < map->bucket_count) ? first_deleted : map->bucket_count;
}

static void _UnsafeVariedHashMap_Rehash(UnsafeVariedHashMap *map) {
    uint32_t old_count = map->bucket_count;
    UnsafeVariedHashEntry *old_buckets = map->buckets;

    if (old_count > UINT32_MAX / 2) return; // can't grow further
    uint32_t new_count = old_count * 2;
    UnsafeVariedHashEntry *new_buckets = (UnsafeVariedHashEntry *)malloc(sizeof(UnsafeVariedHashEntry) * new_count);
    if (!new_buckets) return; // leave map unchanged
    map->buckets = new_buckets;
    map->bucket_count = new_count;
    _UnsafeVariedHashMap_InitBuckets(map->buckets, new_count);

    for (uint32_t i = 0; i < old_count; i++) {
        UnsafeVariedHashEntry *old = &old_buckets[i];
        if (old->value < 0) {
            if (old->key != NULL) free(old->key);
            continue;
        }
        uint32_t slot = _UnsafeVariedHashMap_FindSlot(map, old->key, old->key_len);
        map->buckets[slot] = *old;
    }

    free(old_buckets);
}

// Returns -1 if key already exists (no overwrite).
static int UnsafeVariedHashMap_Set(UnsafeVariedHashMap *map, const void *key, uint32_t key_len, const void *value, uint32_t value_size) {
    if (key_len > UNSAFEHASHMAP_MAX_KEY_LEN) return -1;

    if ((uint64_t)(map->entry_count + 1) * UNSAFEHASHMAP_LOAD_FACTOR_DEN >
        (uint64_t)map->bucket_count * UNSAFEHASHMAP_LOAD_FACTOR_NUM) {
        _UnsafeVariedHashMap_Rehash(map);
    }

    uint32_t slot = _UnsafeVariedHashMap_FindSlot(map, key, key_len);
    if (slot == map->bucket_count) return -1; // table full (sentinel)
    UnsafeVariedHashEntry *e = &map->buckets[slot];

    if (e->value >= 0 && e->key_len == key_len && memcmp(e->key, key, key_len) == 0) {
        return -1;
    }

    e->key = malloc(key_len);
    if (!e->key) return -1;
    memcpy(e->key, key, key_len);
    e->key_len = key_len;

    // Reuse freed entry slot if available, data bytes always appended
    UnsafeVariedHashEntryInfo info;
    info.offset = map->data->count;
    info.size = value_size;

    if (map->free_list->count > 0) {
        int32_t slot = UnsafeArray_GetDeref(map->free_list, map->free_list->count - 1, int32_t);
        map->free_list->count--;
        UnsafeArray_Set(map->entries, (uint32_t)slot, &info);
        e->value = slot;
    } else {
        e->value = (int32_t)map->entries->count;
        UnsafeArray_Add(map->entries, &info);
    }

    const uint8_t *src = (const uint8_t *)value;
    for (uint32_t i = 0; i < value_size; i++) {
        UnsafeArray_Add(map->data, &src[i]);
    }

    map->entry_count++;
    return 0;
}

static void *UnsafeVariedHashMap_Get(UnsafeVariedHashMap *map, const void *key, uint32_t key_len) {
    uint32_t slot = _UnsafeVariedHashMap_FindSlot(map, key, key_len);
    if (slot == map->bucket_count) return NULL;
    UnsafeVariedHashEntry *e = &map->buckets[slot];

    if (e->value < 0) return NULL;
    if (e->key_len != key_len || memcmp(e->key, key, key_len) != 0) return NULL;

    UnsafeVariedHashEntryInfo *info = (UnsafeVariedHashEntryInfo *)UnsafeArray_Get(map->entries, (uint32_t)e->value);
    return UnsafeArray_Get(map->data, info->offset);
}

static uint32_t UnsafeVariedHashMap_GetSize(UnsafeVariedHashMap *map, const void *key, uint32_t key_len) {
    uint32_t slot = _UnsafeVariedHashMap_FindSlot(map, key, key_len);
    if (slot == map->bucket_count) return 0;
    UnsafeVariedHashEntry *e = &map->buckets[slot];

    if (e->value < 0) return 0;
    if (e->key_len != key_len || memcmp(e->key, key, key_len) != 0) return 0;

    UnsafeVariedHashEntryInfo *info = (UnsafeVariedHashEntryInfo *)UnsafeArray_Get(map->entries, (uint32_t)e->value);
    return info->size;
}

static int UnsafeVariedHashMap_Has(UnsafeVariedHashMap *map, const void *key, uint32_t key_len) {
    return UnsafeVariedHashMap_Get(map, key, key_len) != NULL;
}

// Marks slot as DELETED. Entry index slots are reclaimed via free_list,
// but data bytes in the buffer are not -- acceptable for short-lived maps.
static int UnsafeVariedHashMap_Remove(UnsafeVariedHashMap *map, const void *key, uint32_t key_len) {
    uint32_t slot = _UnsafeVariedHashMap_FindSlot(map, key, key_len);
    if (slot == map->bucket_count) return -1;
    UnsafeVariedHashEntry *e = &map->buckets[slot];

    if (e->value < 0) return -1;
    if (e->key_len != key_len || memcmp(e->key, key, key_len) != 0) return -1;

    int32_t freed_slot = e->value;
    UnsafeArray_Add(map->free_list, &freed_slot);
    free(e->key);
    e->key = NULL;
    e->key_len = 0;
    e->value = UNSAFEHASHMAP_DELETED;
    map->entry_count--;
    return 0;
}

// Upsert: insert or overwrite. Single probe -- no tombstone churn.
static int UnsafeVariedHashMap_Upsert(UnsafeVariedHashMap *map, const void *key, uint32_t key_len, const void *value, uint32_t value_size) {
    if (UnsafeVariedHashMap_Has(map, key, key_len)) {
        UnsafeVariedHashMap_Remove(map, key, key_len);
    }
    return UnsafeVariedHashMap_Set(map, key, key_len, value, value_size);
}

typedef void (*UnsafeVariedHashMapForEachFn)(const void *key, uint32_t key_len, void *value, uint32_t value_size);

static void UnsafeVariedHashMap_ForEach(UnsafeVariedHashMap *map, UnsafeVariedHashMapForEachFn fn) {
    for (uint32_t i = 0; i < map->bucket_count; i++) {
        UnsafeVariedHashEntry *e = &map->buckets[i];
        if (e->value < 0) continue;
        UnsafeVariedHashEntryInfo *info = (UnsafeVariedHashEntryInfo *)UnsafeArray_Get(map->entries, (uint32_t)e->value);
        fn(e->key, e->key_len, UnsafeArray_Get(map->data, info->offset), info->size);
    }
}

#define UnsafeVariedHashMap_GetDeref(map, key, key_len, type) ({ \
    void *_uvhm_gd_ptr = UnsafeVariedHashMap_Get(map, key, key_len); \
    _uvhm_gd_ptr ? *(type *)_uvhm_gd_ptr : (type){0}; \
})

#define UnsafeVariedHashMap_SetValue(map, key, key_len, type, value) \
    UnsafeVariedHashMap_Set(map, key, key_len, &(type){value}, sizeof(type))

// String literal key convenience macros -- length computed at compile time.
#define UnsafeVariedHashMap_SSet(map, str_key, value_ptr, value_size) UnsafeVariedHashMap_Set(map, str_key, _UNSAFE_STRLITERAL_LEN(str_key), value_ptr, value_size)
#define UnsafeVariedHashMap_SGet(map, str_key)                       UnsafeVariedHashMap_Get(map, str_key, _UNSAFE_STRLITERAL_LEN(str_key))
#define UnsafeVariedHashMap_SGetSize(map, str_key)                   UnsafeVariedHashMap_GetSize(map, str_key, _UNSAFE_STRLITERAL_LEN(str_key))
#define UnsafeVariedHashMap_SHas(map, str_key)                       UnsafeVariedHashMap_Has(map, str_key, _UNSAFE_STRLITERAL_LEN(str_key))
#define UnsafeVariedHashMap_SRemove(map, str_key)                    UnsafeVariedHashMap_Remove(map, str_key, _UNSAFE_STRLITERAL_LEN(str_key))
#define UnsafeVariedHashMap_SGetDeref(map, str_key, type)            UnsafeVariedHashMap_GetDeref(map, str_key, _UNSAFE_STRLITERAL_LEN(str_key), type)
#define UnsafeVariedHashMap_SSetValue(map, str_key, type, value)     UnsafeVariedHashMap_SetValue(map, str_key, _UNSAFE_STRLITERAL_LEN(str_key), type, value)
#define UnsafeVariedHashMap_SUpsert(map, str_key, value_ptr, value_size) UnsafeVariedHashMap_Upsert(map, str_key, _UNSAFE_STRLITERAL_LEN(str_key), value_ptr, value_size)
