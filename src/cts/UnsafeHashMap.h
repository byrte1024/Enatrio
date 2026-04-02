#pragma once

#include "UnsafeArray.h"

#define UNSAFEHASHMAP_MAX_KEY_LEN 256
#define UNSAFEHASHMAP_EMPTY (-1)
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
} UnsafeHashMap;

// FNV-1a hash
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
    uint32_t bucket_count = UNSAFEHASHMAP_DEFAULT_CAPACITY;
    while (bucket_count < capacity * 2) bucket_count *= 2;
    map->buckets = (UnsafeHashEntry *)malloc(sizeof(UnsafeHashEntry) * bucket_count);
    map->bucket_count = bucket_count;
    map->entry_count = 0;
    map->values = UnsafeArray_Create(element_size, capacity);
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
    free(map);
}

// Find bucket index for a key. Returns the index where the key is, or where it
// should be inserted. If the key is not found and no empty slot exists (should
// not happen with load factor control), returns bucket_count.
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

    uint32_t new_count = old_count * 2;
    map->buckets = (UnsafeHashEntry *)malloc(sizeof(UnsafeHashEntry) * new_count);
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

// Inserts a key-value pair. Returns -1 if key exceeds max length or already exists.
static int UnsafeHashMap_Set(UnsafeHashMap *map, const void *key, uint32_t key_len, const void *value) {
    if (key_len > UNSAFEHASHMAP_MAX_KEY_LEN) return -1;

    // Check load factor before insert
    if ((map->entry_count + 1) * UNSAFEHASHMAP_LOAD_FACTOR_DEN >
        map->bucket_count * UNSAFEHASHMAP_LOAD_FACTOR_NUM) {
        _UnsafeHashMap_Rehash(map);
    }

    uint32_t slot = _UnsafeHashMap_FindSlot(map, key, key_len);
    UnsafeHashEntry *e = &map->buckets[slot];

    // Already exists?
    if (e->value >= 0 && e->key_len == key_len && memcmp(e->key, key, key_len) == 0) {
        return -1;
    }

    // Insert
    e->key = malloc(key_len);
    memcpy(e->key, key, key_len);
    e->key_len = key_len;
    e->value = (int32_t)map->values->count;
    UnsafeArray_Add(map->values, value);
    map->entry_count++;
    return 0;
}

// Returns a pointer to the value for the given key, or NULL if not found.
static void *UnsafeHashMap_Get(UnsafeHashMap *map, const void *key, uint32_t key_len) {
    uint32_t slot = _UnsafeHashMap_FindSlot(map, key, key_len);
    UnsafeHashEntry *e = &map->buckets[slot];

    if (e->value < 0) return NULL;
    if (e->key_len != key_len || memcmp(e->key, key, key_len) != 0) return NULL;

    return UnsafeArray_Get(map->values, (uint32_t)e->value);
}

// Returns 1 if the key exists, 0 otherwise.
static int UnsafeHashMap_Has(UnsafeHashMap *map, const void *key, uint32_t key_len) {
    return UnsafeHashMap_Get(map, key, key_len) != NULL;
}

// Removes a key from the hash map. Returns 0 on success, -1 if not found.
// Note: the value slot is not reclaimed, only the bucket is marked deleted.
static int UnsafeHashMap_Remove(UnsafeHashMap *map, const void *key, uint32_t key_len) {
    uint32_t slot = _UnsafeHashMap_FindSlot(map, key, key_len);
    UnsafeHashEntry *e = &map->buckets[slot];

    if (e->value < 0) return -1;
    if (e->key_len != key_len || memcmp(e->key, key, key_len) != 0) return -1;

    free(e->key);
    e->key = NULL;
    e->key_len = 0;
    e->value = UNSAFEHASHMAP_DELETED;
    map->entry_count--;
    return 0;
}

#define UnsafeHashMap_GetDeref(map, key, key_len, type) \
    (*(type *)UnsafeHashMap_Get(map, key, key_len))

#define UnsafeHashMap_SetValue(map, key, key_len, type, value) \
    UnsafeHashMap_Set(map, key, key_len, &(type){value})

// String literal key convenience macros -- length computed at compile time.
// Only accepts string literals (e.g. "health"), NOT char* variables.
#define UnsafeHashMap_SSet(map, str_key, value_ptr)              UnsafeHashMap_Set(map, str_key, _UNSAFE_STRLITERAL_LEN(str_key), value_ptr)
#define UnsafeHashMap_SGet(map, str_key)                         UnsafeHashMap_Get(map, str_key, _UNSAFE_STRLITERAL_LEN(str_key))
#define UnsafeHashMap_SHas(map, str_key)                         UnsafeHashMap_Has(map, str_key, _UNSAFE_STRLITERAL_LEN(str_key))
#define UnsafeHashMap_SRemove(map, str_key)                      UnsafeHashMap_Remove(map, str_key, _UNSAFE_STRLITERAL_LEN(str_key))
#define UnsafeHashMap_SGetDeref(map, str_key, type)              UnsafeHashMap_GetDeref(map, str_key, _UNSAFE_STRLITERAL_LEN(str_key), type)
#define UnsafeHashMap_SSetValue(map, str_key, type, value)       UnsafeHashMap_SetValue(map, str_key, _UNSAFE_STRLITERAL_LEN(str_key), type, value)

// _UNSAFE_STRLITERAL_LEN is defined in UnsafeDictionary.h; if using standalone,
// include UnsafeDictionary.h first or define the macro separately.

typedef void (*UnsafeHashMapFormatter)(const void *value, char *buf, uint32_t buf_size);

// Prints the hash map with a value formatter. string_keys=1 prints keys as strings, 0 as hex.
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

#define UnsafeHashMap_PrintF(map, type, fmt, string_keys) do { \
    void _uhmf_fn(const void *_v, char *_b, uint32_t _s) { \
        snprintf(_b, _s, fmt, *(const type *)_v); \
    } \
    UnsafeHashMap_Print(map, _uhmf_fn, string_keys); \
} while (0)

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

#define UnsafeHashMap_LogF(map, type, fmt, string_keys) do { \
    void _uhmlf_fn(const void *_v, char *_b, uint32_t _s) { \
        snprintf(_b, _s, fmt, *(const type *)_v); \
    } \
    UnsafeHashMap_Log(map, _uhmlf_fn, string_keys); \
} while (0)

// ============================================================
// UnsafeVariedHashMap -- hash map with varied value sizes
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
    UnsafeArray *entries; // UnsafeVariedHashEntryInfo index
    UnsafeArray *data;    // raw byte buffer (element_size = 1)
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
    uint32_t bucket_count = UNSAFEHASHMAP_DEFAULT_CAPACITY;
    while (bucket_count < capacity * 2) bucket_count *= 2;
    map->buckets = (UnsafeVariedHashEntry *)malloc(sizeof(UnsafeVariedHashEntry) * bucket_count);
    map->bucket_count = bucket_count;
    map->entry_count = 0;
    map->entries = UnsafeArray_Create(sizeof(UnsafeVariedHashEntryInfo), capacity);
    map->data = UnsafeArray_Create(1, capacity * 8);
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

    uint32_t new_count = old_count * 2;
    map->buckets = (UnsafeVariedHashEntry *)malloc(sizeof(UnsafeVariedHashEntry) * new_count);
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

// Inserts a key-value pair with a specified value size. Returns -1 if key already exists.
static int UnsafeVariedHashMap_Set(UnsafeVariedHashMap *map, const void *key, uint32_t key_len, const void *value, uint32_t value_size) {
    if (key_len > UNSAFEHASHMAP_MAX_KEY_LEN) return -1;

    if ((map->entry_count + 1) * UNSAFEHASHMAP_LOAD_FACTOR_DEN >
        map->bucket_count * UNSAFEHASHMAP_LOAD_FACTOR_NUM) {
        _UnsafeVariedHashMap_Rehash(map);
    }

    uint32_t slot = _UnsafeVariedHashMap_FindSlot(map, key, key_len);
    UnsafeVariedHashEntry *e = &map->buckets[slot];

    if (e->value >= 0 && e->key_len == key_len && memcmp(e->key, key, key_len) == 0) {
        return -1;
    }

    e->key = malloc(key_len);
    memcpy(e->key, key, key_len);
    e->key_len = key_len;

    UnsafeVariedHashEntryInfo info;
    info.offset = map->data->count;
    info.size = value_size;

    e->value = (int32_t)map->entries->count;
    UnsafeArray_Add(map->entries, &info);

    const uint8_t *src = (const uint8_t *)value;
    for (uint32_t i = 0; i < value_size; i++) {
        UnsafeArray_Add(map->data, &src[i]);
    }

    map->entry_count++;
    return 0;
}

// Returns a pointer to the value for the given key, or NULL if not found.
static void *UnsafeVariedHashMap_Get(UnsafeVariedHashMap *map, const void *key, uint32_t key_len) {
    uint32_t slot = _UnsafeVariedHashMap_FindSlot(map, key, key_len);
    UnsafeVariedHashEntry *e = &map->buckets[slot];

    if (e->value < 0) return NULL;
    if (e->key_len != key_len || memcmp(e->key, key, key_len) != 0) return NULL;

    UnsafeVariedHashEntryInfo *info = (UnsafeVariedHashEntryInfo *)UnsafeArray_Get(map->entries, (uint32_t)e->value);
    return UnsafeArray_Get(map->data, info->offset);
}

// Returns the size of the value for the given key, or 0 if not found.
static uint32_t UnsafeVariedHashMap_GetSize(UnsafeVariedHashMap *map, const void *key, uint32_t key_len) {
    uint32_t slot = _UnsafeVariedHashMap_FindSlot(map, key, key_len);
    UnsafeVariedHashEntry *e = &map->buckets[slot];

    if (e->value < 0) return 0;
    if (e->key_len != key_len || memcmp(e->key, key, key_len) != 0) return 0;

    UnsafeVariedHashEntryInfo *info = (UnsafeVariedHashEntryInfo *)UnsafeArray_Get(map->entries, (uint32_t)e->value);
    return info->size;
}

// Returns 1 if the key exists, 0 otherwise.
static int UnsafeVariedHashMap_Has(UnsafeVariedHashMap *map, const void *key, uint32_t key_len) {
    return UnsafeVariedHashMap_Get(map, key, key_len) != NULL;
}

// Removes a key. Returns 0 on success, -1 if not found.
static int UnsafeVariedHashMap_Remove(UnsafeVariedHashMap *map, const void *key, uint32_t key_len) {
    uint32_t slot = _UnsafeVariedHashMap_FindSlot(map, key, key_len);
    UnsafeVariedHashEntry *e = &map->buckets[slot];

    if (e->value < 0) return -1;
    if (e->key_len != key_len || memcmp(e->key, key, key_len) != 0) return -1;

    free(e->key);
    e->key = NULL;
    e->key_len = 0;
    e->value = UNSAFEHASHMAP_DELETED;
    map->entry_count--;
    return 0;
}

#define UnsafeVariedHashMap_GetDeref(map, key, key_len, type) \
    (*(type *)UnsafeVariedHashMap_Get(map, key, key_len))

#define UnsafeVariedHashMap_SetValue(map, key, key_len, type, value) \
    UnsafeVariedHashMap_Set(map, key, key_len, &(type){value}, sizeof(type))

// String literal key convenience macros
#define UnsafeVariedHashMap_SSet(map, str_key, value_ptr, value_size) UnsafeVariedHashMap_Set(map, str_key, _UNSAFE_STRLITERAL_LEN(str_key), value_ptr, value_size)
#define UnsafeVariedHashMap_SGet(map, str_key)                       UnsafeVariedHashMap_Get(map, str_key, _UNSAFE_STRLITERAL_LEN(str_key))
#define UnsafeVariedHashMap_SGetSize(map, str_key)                   UnsafeVariedHashMap_GetSize(map, str_key, _UNSAFE_STRLITERAL_LEN(str_key))
#define UnsafeVariedHashMap_SHas(map, str_key)                       UnsafeVariedHashMap_Has(map, str_key, _UNSAFE_STRLITERAL_LEN(str_key))
#define UnsafeVariedHashMap_SRemove(map, str_key)                    UnsafeVariedHashMap_Remove(map, str_key, _UNSAFE_STRLITERAL_LEN(str_key))
#define UnsafeVariedHashMap_SGetDeref(map, str_key, type)            UnsafeVariedHashMap_GetDeref(map, str_key, _UNSAFE_STRLITERAL_LEN(str_key), type)
#define UnsafeVariedHashMap_SSetValue(map, str_key, type, value)     UnsafeVariedHashMap_SetValue(map, str_key, _UNSAFE_STRLITERAL_LEN(str_key), type, value)
