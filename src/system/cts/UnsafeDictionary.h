#pragma once

#include "UnsafeArray.h"

#define UNSAFEDICT_MAX_KEY_LEN 256
#define UNSAFEDICT_EMPTY (-1)

// 2-bit-per-level trie design: trades memory for guaranteed O(key_len)
// lookup without hash collisions. Each byte produces 4 trie levels
// (2 bits each), so lookup cost depends only on key length, never on
// the number of entries or key distribution.
typedef struct UnsafeDictNode {
    int32_t children[4]; // one child per 2-bit pair
    int32_t value;       // index into values array, or UNSAFEDICT_EMPTY
} UnsafeDictNode;

typedef struct UnsafeDictionary {
    UnsafeArray *nodes;
    UnsafeArray *values;
    UnsafeArray *free_list; // int32_t indices of freed value slots
} UnsafeDictionary;

static UnsafeDictNode UnsafeDictNode_Empty(void) {
    UnsafeDictNode node;
    node.children[0] = UNSAFEDICT_EMPTY;
    node.children[1] = UNSAFEDICT_EMPTY;
    node.children[2] = UNSAFEDICT_EMPTY;
    node.children[3] = UNSAFEDICT_EMPTY;
    node.value = UNSAFEDICT_EMPTY;
    return node;
}

static UnsafeDictionary *UnsafeDictionary_Create(uint32_t element_size, uint32_t capacity) {
    UnsafeDictionary *dict = (UnsafeDictionary *)malloc(sizeof(UnsafeDictionary));
    if (!dict) return NULL;
    dict->values = UnsafeArray_Create(element_size, capacity);
    dict->nodes = UnsafeArray_Create(sizeof(UnsafeDictNode), 64);
    dict->free_list = UnsafeArray_Create(sizeof(int32_t), 8);
    // Allocate root node
    UnsafeDictNode root = UnsafeDictNode_Empty();
    UnsafeArray_Add(dict->nodes, &root);
    return dict;
}

static void UnsafeDictionary_Destroy(UnsafeDictionary *dict) {
    UnsafeArray_Destroy(dict->nodes);
    UnsafeArray_Destroy(dict->values);
    UnsafeArray_Destroy(dict->free_list);
    free(dict);
}

// Walks the trie for a key. create=true allocates missing nodes along the path.
static int32_t UnsafeDictionary_Walk(UnsafeDictionary *dict, const void *key, uint32_t key_len, int create) {
    if (key_len > UNSAFEDICT_MAX_KEY_LEN) return UNSAFEDICT_EMPTY;

    const uint8_t *bytes = (const uint8_t *)key;
    int32_t current = 0; // root

    for (uint32_t i = 0; i < key_len; i++) {
        uint8_t byte = bytes[i];
        // 4 pairs of 2 bits per byte, MSB first: bits 7-6, 5-4, 3-2, 1-0
        for (int shift = 6; shift >= 0; shift -= 2) {
            uint8_t pair = (byte >> shift) & 0x03;

            UnsafeDictNode *node = (UnsafeDictNode *)UnsafeArray_Get(dict->nodes, (uint32_t)current);
            int32_t next = node->children[pair];

            if (next == UNSAFEDICT_EMPTY) {
                if (!create) return UNSAFEDICT_EMPTY;
                // Allocate new node
                UnsafeDictNode empty = UnsafeDictNode_Empty();
                next = (int32_t)dict->nodes->count;
                UnsafeArray_Add(dict->nodes, &empty);
                // Re-fetch parent since Add may have reallocated
                node = (UnsafeDictNode *)UnsafeArray_Get(dict->nodes, (uint32_t)current);
                node->children[pair] = next;
            }

            current = next;
        }
    }

    return current;
}

// Returns -1 if key exceeds max length or already exists (no overwrite).
static int UnsafeDictionary_Set(UnsafeDictionary *dict, const void *key, uint32_t key_len, const void *value) {
    if (key_len > UNSAFEDICT_MAX_KEY_LEN) return -1;

    int32_t node_idx = UnsafeDictionary_Walk(dict, key, key_len, 1);
    UnsafeDictNode *node = (UnsafeDictNode *)UnsafeArray_Get(dict->nodes, (uint32_t)node_idx);

    if (node->value != UNSAFEDICT_EMPTY) return -1;

    if (dict->free_list->count > 0) {
        int32_t slot = UnsafeArray_GetDeref(dict->free_list, dict->free_list->count - 1, int32_t);
        dict->free_list->count--;
        UnsafeArray_Set(dict->values, (uint32_t)slot, value);
        node->value = slot;
    } else {
        node->value = (int32_t)dict->values->count;
        UnsafeArray_Add(dict->values, value);
    }
    return 0;
}

static void *UnsafeDictionary_Get(UnsafeDictionary *dict, const void *key, uint32_t key_len) {
    int32_t node_idx = UnsafeDictionary_Walk(dict, key, key_len, 0);
    if (node_idx == UNSAFEDICT_EMPTY) return NULL;

    UnsafeDictNode *node = (UnsafeDictNode *)UnsafeArray_Get(dict->nodes, (uint32_t)node_idx);
    if (node->value == UNSAFEDICT_EMPTY) return NULL;

    return UnsafeArray_Get(dict->values, (uint32_t)node->value);
}

static int UnsafeDictionary_Has(UnsafeDictionary *dict, const void *key, uint32_t key_len) {
    return UnsafeDictionary_Get(dict, key, key_len) != NULL;
}

// Reclaims the value slot but leaves trie nodes in place.
static int UnsafeDictionary_Remove(UnsafeDictionary *dict, const void *key, uint32_t key_len) {
    int32_t node_idx = UnsafeDictionary_Walk(dict, key, key_len, 0);
    if (node_idx == UNSAFEDICT_EMPTY) return -1;

    UnsafeDictNode *node = (UnsafeDictNode *)UnsafeArray_Get(dict->nodes, (uint32_t)node_idx);
    if (node->value == UNSAFEDICT_EMPTY) return -1;

    UnsafeArray_Add(dict->free_list, &node->value);
    node->value = UNSAFEDICT_EMPTY;
    return 0;
}

#define UnsafeDictionary_GetDeref(dict, key, key_len, type) ({ \
    void *_ud_gd_ptr = UnsafeDictionary_Get(dict, key, key_len); \
    _ud_gd_ptr ? *(type *)_ud_gd_ptr : (type){0}; \
})

#define UnsafeDictionary_SetValue(dict, key, key_len, type, value) \
    UnsafeDictionary_Set(dict, key, key_len, &(type){value})

// Iterates all entries via recursive trie walk, reconstructing keys on the fly.
typedef void (*UnsafeDictForEachFn)(const void *key, uint32_t key_len, void *value);

static void _UnsafeDictionary_ForEachWalk(
    UnsafeDictionary *dict, int32_t node_idx,
    uint8_t *key_buf, uint32_t depth,
    UnsafeDictForEachFn fn
) {
    UnsafeDictNode *node = (UnsafeDictNode *)UnsafeArray_Get(dict->nodes, (uint32_t)node_idx);

    if (node->value != UNSAFEDICT_EMPTY) {
        uint32_t key_len = depth / 4;
        fn(key_buf, key_len, UnsafeArray_Get(dict->values, (uint32_t)node->value));
    }

    for (int child = 0; child < 4; child++) {
        if (node->children[child] == UNSAFEDICT_EMPTY) continue;

        uint32_t byte_idx = depth / 4;
        uint32_t pair_idx = depth % 4;
        int shift = 6 - (int)pair_idx * 2;
        uint8_t mask = (uint8_t)(0x03 << shift);

        if (pair_idx == 0) key_buf[byte_idx] = 0;
        key_buf[byte_idx] = (key_buf[byte_idx] & ~mask) | (uint8_t)(child << shift);

        _UnsafeDictionary_ForEachWalk(dict, node->children[child], key_buf, depth + 1, fn);

        node = (UnsafeDictNode *)UnsafeArray_Get(dict->nodes, (uint32_t)node_idx);
    }
}

static void UnsafeDictionary_ForEach(UnsafeDictionary *dict, UnsafeDictForEachFn fn) {
    uint8_t key_buf[UNSAFEDICT_MAX_KEY_LEN];
    memset(key_buf, 0, sizeof(key_buf));
    _UnsafeDictionary_ForEachWalk(dict, 0, key_buf, 0, fn);
}

// Compile-time string literal length. _Static_assert rejects char* variables
// that would give wrong length at runtime -- sizeof(char*) is typically 4 or 8,
// not the string length. Only string literals have array type with correct sizeof.
#define _UNSAFE_STRLITERAL_LEN(s) ({ \
    _Static_assert( \
        !__builtin_types_compatible_p(__typeof__(s), char *) && \
        !__builtin_types_compatible_p(__typeof__(s), const char *), \
        "S-macros require string literals, not char* pointers"); \
    (uint32_t)(sizeof(s) - 1); \
})

// String literal key convenience macros -- length computed at compile time.
#define UnsafeDictionary_SSet(dict, str_key, value_ptr)              UnsafeDictionary_Set(dict, str_key, _UNSAFE_STRLITERAL_LEN(str_key), value_ptr)
#define UnsafeDictionary_SGet(dict, str_key)                         UnsafeDictionary_Get(dict, str_key, _UNSAFE_STRLITERAL_LEN(str_key))
#define UnsafeDictionary_SHas(dict, str_key)                         UnsafeDictionary_Has(dict, str_key, _UNSAFE_STRLITERAL_LEN(str_key))
#define UnsafeDictionary_SRemove(dict, str_key)                      UnsafeDictionary_Remove(dict, str_key, _UNSAFE_STRLITERAL_LEN(str_key))
#define UnsafeDictionary_SGetDeref(dict, str_key, type)              UnsafeDictionary_GetDeref(dict, str_key, _UNSAFE_STRLITERAL_LEN(str_key), type)
#define UnsafeDictionary_SSetValue(dict, str_key, type, value)       UnsafeDictionary_SetValue(dict, str_key, _UNSAFE_STRLITERAL_LEN(str_key), type, value)

typedef void (*UnsafeDictFormatter)(const void *value, char *buf, uint32_t buf_size);

#define LINTNORE
static void _UnsafeDictionary_PrintWalk(
    UnsafeDictionary *dict,
    int32_t node_idx,
    uint8_t *key_buf,
    uint32_t depth,        // depth in 2-bit steps
    UnsafeDictFormatter fmt_value,
    int string_keys
) {
    UnsafeDictNode *node = (UnsafeDictNode *)UnsafeArray_Get(dict->nodes, (uint32_t)node_idx);

    if (node->value != UNSAFEDICT_EMPTY) {
        char val_buf[256];
        fmt_value(UnsafeArray_Get(dict->values, (uint32_t)node->value), val_buf, sizeof(val_buf));

        uint32_t key_len = depth / 4;
        if (string_keys) {
            printf("  \"%.*s\" => %s\n", (int)key_len, (char *)key_buf, val_buf);
        } else {
            printf("  [");
            for (uint32_t i = 0; i < key_len; i++) {
                if (i > 0) printf(" ");
                printf("%02X", key_buf[i]);
            }
            printf("] => %s\n", val_buf);
        }
    }

    for (int child = 0; child < 4; child++) {
        if (node->children[child] == UNSAFEDICT_EMPTY) continue;

        uint32_t byte_idx = depth / 4;
        uint32_t pair_idx = depth % 4;
        int shift = 6 - (int)pair_idx * 2;
        uint8_t mask = (uint8_t)(0x03 << shift);

        if (pair_idx == 0) key_buf[byte_idx] = 0;
        key_buf[byte_idx] = (key_buf[byte_idx] & ~mask) | (uint8_t)(child << shift);

        _UnsafeDictionary_PrintWalk(dict, node->children[child], key_buf, depth + 1, fmt_value, string_keys);

        node = (UnsafeDictNode *)UnsafeArray_Get(dict->nodes, (uint32_t)node_idx);
    }
}

// string_keys=1 prints keys as strings, 0 as hex bytes.
static void UnsafeDictionary_Print(UnsafeDictionary *dict, UnsafeDictFormatter fmt_value, int string_keys) {
    uint32_t entry_count = 0;
    for (uint32_t i = 0; i < dict->nodes->count; i++) {
        UnsafeDictNode *n = (UnsafeDictNode *)UnsafeArray_Get(dict->nodes, i);
        if (n->value != UNSAFEDICT_EMPTY) entry_count++;
    }

    printf("UnsafeDictionary[%u entries, %u nodes] {\n", entry_count, dict->nodes->count);
    uint8_t key_buf[UNSAFEDICT_MAX_KEY_LEN];
    memset(key_buf, 0, sizeof(key_buf));
    _UnsafeDictionary_PrintWalk(dict, 0, key_buf, 0, fmt_value, string_keys);
    printf("}\n");
}

//   UnsafeDictionary_PrintF(dict, int, "%d", 1);       // string keys
//   UnsafeDictionary_PrintF(dict, float, "%.2f", 0);   // hex keys
static const char   *_udict_pf_fmt = NULL;
static uint32_t      _udict_pf_esz = 0;
static void _udict_pf_fn(const void *v, char *b, uint32_t s) {
    _unsafe_fmt_snprintf(v, b, s, _udict_pf_fmt, _udict_pf_esz);
}
#define UnsafeDictionary_PrintF(dict, type, fmt, string_keys) do { \
    _udict_pf_fmt = (fmt); \
    _udict_pf_esz = (uint32_t)sizeof(type); \
    UnsafeDictionary_Print(dict, _udict_pf_fn, string_keys); \
} while (0)
#undef LINTNORE

// --- Log variants (LOG_INFO per line) ---

static void _UnsafeDictionary_LogWalk(
    UnsafeDictionary *dict,
    int32_t node_idx,
    uint8_t *key_buf,
    uint32_t depth,
    UnsafeDictFormatter fmt_value,
    int string_keys
) {
    UnsafeDictNode *node = (UnsafeDictNode *)UnsafeArray_Get(dict->nodes, (uint32_t)node_idx);

    if (node->value != UNSAFEDICT_EMPTY) {
        char val_buf[256];
        fmt_value(UnsafeArray_Get(dict->values, (uint32_t)node->value), val_buf, sizeof(val_buf));

        uint32_t key_len = depth / 4;
        if (string_keys) {
            LOG_INFO("  \"%.*s\" => %s", (int)key_len, (char *)key_buf, val_buf);
        } else {
            char hex[UNSAFEDICT_MAX_KEY_LEN * 3 + 1];
            uint32_t pos = 0;
            for (uint32_t i = 0; i < key_len; i++) {
                if (i > 0) hex[pos++] = ' ';
                pos += (uint32_t)snprintf(hex + pos, sizeof(hex) - pos, "%02X", key_buf[i]);
            }
            hex[pos] = '\0';
            LOG_INFO("  [%s] => %s", hex, val_buf);
        }
    }

    for (int child = 0; child < 4; child++) {
        if (node->children[child] == UNSAFEDICT_EMPTY) continue;

        uint32_t byte_idx = depth / 4;
        uint32_t pair_idx = depth % 4;
        int shift = 6 - (int)pair_idx * 2;
        uint8_t mask = (uint8_t)(0x03 << shift);

        if (pair_idx == 0) key_buf[byte_idx] = 0;
        key_buf[byte_idx] = (key_buf[byte_idx] & ~mask) | (uint8_t)(child << shift);

        _UnsafeDictionary_LogWalk(dict, node->children[child], key_buf, depth + 1, fmt_value, string_keys);

        node = (UnsafeDictNode *)UnsafeArray_Get(dict->nodes, (uint32_t)node_idx);
    }
}

// string_keys=1 logs keys as strings, 0 as hex bytes.
static void UnsafeDictionary_Log(UnsafeDictionary *dict, UnsafeDictFormatter fmt_value, int string_keys) {
    uint32_t entry_count = 0;
    for (uint32_t i = 0; i < dict->nodes->count; i++) {
        UnsafeDictNode *n = (UnsafeDictNode *)UnsafeArray_Get(dict->nodes, i);
        if (n->value != UNSAFEDICT_EMPTY) entry_count++;
    }

    LOG_INFO("UnsafeDictionary[%u entries, %u nodes] {", entry_count, dict->nodes->count);
    uint8_t key_buf[UNSAFEDICT_MAX_KEY_LEN];
    memset(key_buf, 0, sizeof(key_buf));
    _UnsafeDictionary_LogWalk(dict, 0, key_buf, 0, fmt_value, string_keys);
    LOG_INFO("}");
}

//   UnsafeDictionary_LogF(dict, int, "%d", 1);       // string keys
//   UnsafeDictionary_LogF(dict, float, "%.2f", 0);   // hex keys
static const char   *_udict_lf_fmt = NULL;
static uint32_t      _udict_lf_esz = 0;
static void _udict_lf_fn(const void *v, char *b, uint32_t s) {
    _unsafe_fmt_snprintf(v, b, s, _udict_lf_fmt, _udict_lf_esz);
}
#define UnsafeDictionary_LogF(dict, type, fmt, string_keys) do { \
    _udict_lf_fmt = (fmt); \
    _udict_lf_esz = (uint32_t)sizeof(type); \
    UnsafeDictionary_Log(dict, _udict_lf_fn, string_keys); \
} while (0)

// ============================================================
// UnsafeVariedDictionary -- trie dictionary with varied value sizes
// ============================================================

typedef struct UnsafeVariedEntry {
    uint32_t offset; // byte offset into the data buffer
    uint32_t size;   // size of this value in bytes
} UnsafeVariedEntry;

typedef struct UnsafeVariedDictionary {
    UnsafeArray *nodes;     // trie nodes (UnsafeDictNode)
    UnsafeArray *entries;   // UnsafeVariedEntry index
    UnsafeArray *data;      // raw byte buffer (element_size = 1)
    UnsafeArray *free_list; // int32_t indices of freed entry slots (data bytes not reclaimed)
} UnsafeVariedDictionary;

static UnsafeVariedDictionary *UnsafeVariedDictionary_Create(uint32_t capacity) {
    UnsafeVariedDictionary *dict = (UnsafeVariedDictionary *)malloc(sizeof(UnsafeVariedDictionary));
    if (!dict) return NULL;
    dict->entries = UnsafeArray_Create(sizeof(UnsafeVariedEntry), capacity);
    dict->data = UnsafeArray_Create(1, capacity * 8);
    dict->nodes = UnsafeArray_Create(sizeof(UnsafeDictNode), 64);
    dict->free_list = UnsafeArray_Create(sizeof(int32_t), 8);
    UnsafeDictNode root = UnsafeDictNode_Empty();
    UnsafeArray_Add(dict->nodes, &root);
    return dict;
}

static void UnsafeVariedDictionary_Destroy(UnsafeVariedDictionary *dict) {
    UnsafeArray_Destroy(dict->nodes);
    UnsafeArray_Destroy(dict->entries);
    UnsafeArray_Destroy(dict->data);
    UnsafeArray_Destroy(dict->free_list);
    free(dict);
}

// Same trie walk as UnsafeDictionary_Walk but on the varied-size variant.
static int32_t UnsafeVariedDictionary_Walk(UnsafeVariedDictionary *dict, const void *key, uint32_t key_len, int create) {
    if (key_len > UNSAFEDICT_MAX_KEY_LEN) return UNSAFEDICT_EMPTY;

    const uint8_t *bytes = (const uint8_t *)key;
    int32_t current = 0;

    for (uint32_t i = 0; i < key_len; i++) {
        uint8_t byte = bytes[i];
        for (int shift = 6; shift >= 0; shift -= 2) {
            uint8_t pair = (byte >> shift) & 0x03;

            UnsafeDictNode *node = (UnsafeDictNode *)UnsafeArray_Get(dict->nodes, (uint32_t)current);
            int32_t next = node->children[pair];

            if (next == UNSAFEDICT_EMPTY) {
                if (!create) return UNSAFEDICT_EMPTY;
                UnsafeDictNode empty = UnsafeDictNode_Empty();
                next = (int32_t)dict->nodes->count;
                UnsafeArray_Add(dict->nodes, &empty);
                node = (UnsafeDictNode *)UnsafeArray_Get(dict->nodes, (uint32_t)current);
                node->children[pair] = next;
            }

            current = next;
        }
    }

    return current;
}

// Returns -1 if key already exists (no overwrite).
static int UnsafeVariedDictionary_Set(UnsafeVariedDictionary *dict, const void *key, uint32_t key_len, const void *value, uint32_t value_size) {
    if (key_len > UNSAFEDICT_MAX_KEY_LEN) return -1;

    int32_t node_idx = UnsafeVariedDictionary_Walk(dict, key, key_len, 1);
    UnsafeDictNode *node = (UnsafeDictNode *)UnsafeArray_Get(dict->nodes, (uint32_t)node_idx);

    if (node->value != UNSAFEDICT_EMPTY) return -1;

    // Record entry (reuse freed slot if available, data bytes always appended)
    UnsafeVariedEntry entry;
    entry.offset = dict->data->count;
    entry.size = value_size;

    if (dict->free_list->count > 0) {
        int32_t slot = UnsafeArray_GetDeref(dict->free_list, dict->free_list->count - 1, int32_t);
        dict->free_list->count--;
        UnsafeArray_Set(dict->entries, (uint32_t)slot, &entry);
        node->value = slot;
    } else {
        node->value = (int32_t)dict->entries->count;
        UnsafeArray_Add(dict->entries, &entry);
    }

    // Append raw bytes
    const uint8_t *src = (const uint8_t *)value;
    for (uint32_t i = 0; i < value_size; i++) {
        UnsafeArray_Add(dict->data, &src[i]);
    }

    return 0;
}

static void *UnsafeVariedDictionary_Get(UnsafeVariedDictionary *dict, const void *key, uint32_t key_len) {
    int32_t node_idx = UnsafeVariedDictionary_Walk(dict, key, key_len, 0);
    if (node_idx == UNSAFEDICT_EMPTY) return NULL;

    UnsafeDictNode *node = (UnsafeDictNode *)UnsafeArray_Get(dict->nodes, (uint32_t)node_idx);
    if (node->value == UNSAFEDICT_EMPTY) return NULL;

    UnsafeVariedEntry *entry = (UnsafeVariedEntry *)UnsafeArray_Get(dict->entries, (uint32_t)node->value);
    return UnsafeArray_Get(dict->data, entry->offset);
}

static uint32_t UnsafeVariedDictionary_GetSize(UnsafeVariedDictionary *dict, const void *key, uint32_t key_len) {
    int32_t node_idx = UnsafeVariedDictionary_Walk(dict, key, key_len, 0);
    if (node_idx == UNSAFEDICT_EMPTY) return 0;

    UnsafeDictNode *node = (UnsafeDictNode *)UnsafeArray_Get(dict->nodes, (uint32_t)node_idx);
    if (node->value == UNSAFEDICT_EMPTY) return 0;

    UnsafeVariedEntry *entry = (UnsafeVariedEntry *)UnsafeArray_Get(dict->entries, (uint32_t)node->value);
    return entry->size;
}

static int UnsafeVariedDictionary_Has(UnsafeVariedDictionary *dict, const void *key, uint32_t key_len) {
    return UnsafeVariedDictionary_Get(dict, key, key_len) != NULL;
}

// Entry index slots are reclaimed, but data bytes in the buffer are not --
// acceptable trade-off since most payloads are short-lived.
static int UnsafeVariedDictionary_Remove(UnsafeVariedDictionary *dict, const void *key, uint32_t key_len) {
    int32_t node_idx = UnsafeVariedDictionary_Walk(dict, key, key_len, 0);
    if (node_idx == UNSAFEDICT_EMPTY) return -1;

    UnsafeDictNode *node = (UnsafeDictNode *)UnsafeArray_Get(dict->nodes, (uint32_t)node_idx);
    if (node->value == UNSAFEDICT_EMPTY) return -1;

    UnsafeArray_Add(dict->free_list, &node->value);
    node->value = UNSAFEDICT_EMPTY;
    return 0;
}

// Iterates all entries via recursive trie walk, reconstructing keys on the fly.
typedef void (*UnsafeVariedDictForEachFn)(const void *key, uint32_t key_len, void *value, uint32_t value_size);

static void _UnsafeVariedDictionary_ForEachWalk(
    UnsafeVariedDictionary *dict, int32_t node_idx,
    uint8_t *key_buf, uint32_t depth,
    UnsafeVariedDictForEachFn fn
) {
    UnsafeDictNode *node = (UnsafeDictNode *)UnsafeArray_Get(dict->nodes, (uint32_t)node_idx);

    if (node->value != UNSAFEDICT_EMPTY) {
        UnsafeVariedEntry *entry = (UnsafeVariedEntry *)UnsafeArray_Get(dict->entries, (uint32_t)node->value);
        uint32_t key_len = depth / 4;
        fn(key_buf, key_len, UnsafeArray_Get(dict->data, entry->offset), entry->size);
    }

    for (int child = 0; child < 4; child++) {
        if (node->children[child] == UNSAFEDICT_EMPTY) continue;

        uint32_t byte_idx = depth / 4;
        uint32_t pair_idx = depth % 4;
        int shift = 6 - (int)pair_idx * 2;
        uint8_t mask = (uint8_t)(0x03 << shift);

        if (pair_idx == 0) key_buf[byte_idx] = 0;
        key_buf[byte_idx] = (key_buf[byte_idx] & ~mask) | (uint8_t)(child << shift);

        _UnsafeVariedDictionary_ForEachWalk(dict, node->children[child], key_buf, depth + 1, fn);

        node = (UnsafeDictNode *)UnsafeArray_Get(dict->nodes, (uint32_t)node_idx);
    }
}

static void UnsafeVariedDictionary_ForEach(UnsafeVariedDictionary *dict, UnsafeVariedDictForEachFn fn) {
    uint8_t key_buf[UNSAFEDICT_MAX_KEY_LEN];
    memset(key_buf, 0, sizeof(key_buf));
    _UnsafeVariedDictionary_ForEachWalk(dict, 0, key_buf, 0, fn);
}

#define UnsafeVariedDictionary_GetDeref(dict, key, key_len, type) ({ \
    void *_uvd_gd_ptr = UnsafeVariedDictionary_Get(dict, key, key_len); \
    _uvd_gd_ptr ? *(type *)_uvd_gd_ptr : (type){0}; \
})

#define UnsafeVariedDictionary_SetValue(dict, key, key_len, type, value) \
    UnsafeVariedDictionary_Set(dict, key, key_len, &(type){value}, sizeof(type))

// String literal key convenience macros -- length computed at compile time.
#define UnsafeVariedDictionary_SSet(dict, str_key, value_ptr, value_size) UnsafeVariedDictionary_Set(dict, str_key, _UNSAFE_STRLITERAL_LEN(str_key), value_ptr, value_size)
#define UnsafeVariedDictionary_SGet(dict, str_key)                       UnsafeVariedDictionary_Get(dict, str_key, _UNSAFE_STRLITERAL_LEN(str_key))
#define UnsafeVariedDictionary_SGetSize(dict, str_key)                   UnsafeVariedDictionary_GetSize(dict, str_key, _UNSAFE_STRLITERAL_LEN(str_key))
#define UnsafeVariedDictionary_SHas(dict, str_key)                       UnsafeVariedDictionary_Has(dict, str_key, _UNSAFE_STRLITERAL_LEN(str_key))
#define UnsafeVariedDictionary_SRemove(dict, str_key)                    UnsafeVariedDictionary_Remove(dict, str_key, _UNSAFE_STRLITERAL_LEN(str_key))
#define UnsafeVariedDictionary_SGetDeref(dict, str_key, type)            UnsafeVariedDictionary_GetDeref(dict, str_key, _UNSAFE_STRLITERAL_LEN(str_key), type)
#define UnsafeVariedDictionary_SSetValue(dict, str_key, type, value)     UnsafeVariedDictionary_SetValue(dict, str_key, _UNSAFE_STRLITERAL_LEN(str_key), type, value)
