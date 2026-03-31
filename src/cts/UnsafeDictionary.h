#pragma once

#include "UnsafeArray.h"

#define UNSAFEDICT_MAX_KEY_LEN 256
#define UNSAFEDICT_EMPTY (-1)

typedef struct UnsafeDictNode {
    int32_t children[4]; // one child per 2-bit pair
    int32_t value;       // index into values array, or UNSAFEDICT_EMPTY
} UnsafeDictNode;

typedef struct UnsafeDictionary {
    UnsafeArray *nodes;
    UnsafeArray *values;
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
    dict->values = UnsafeArray_Create(element_size, capacity);
    dict->nodes = UnsafeArray_Create(sizeof(UnsafeDictNode), 64);
    // Allocate root node
    UnsafeDictNode root = UnsafeDictNode_Empty();
    UnsafeArray_Add(dict->nodes, &root);
    return dict;
}

static void UnsafeDictionary_Destroy(UnsafeDictionary *dict) {
    UnsafeArray_Destroy(dict->nodes);
    UnsafeArray_Destroy(dict->values);
    free(dict);
}

// Walks the trie for a key. If create is true, allocates missing nodes along the way.
// Returns the node index at the end of the key path, or UNSAFEDICT_EMPTY if not found.
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

// Inserts a key-value pair. Returns -1 if key exceeds max length or already exists.
static int UnsafeDictionary_Set(UnsafeDictionary *dict, const void *key, uint32_t key_len, const void *value) {
    if (key_len > UNSAFEDICT_MAX_KEY_LEN) return -1;

    int32_t node_idx = UnsafeDictionary_Walk(dict, key, key_len, 1);
    UnsafeDictNode *node = (UnsafeDictNode *)UnsafeArray_Get(dict->nodes, (uint32_t)node_idx);

    if (node->value != UNSAFEDICT_EMPTY) return -1;

    node->value = (int32_t)dict->values->count;
    UnsafeArray_Add(dict->values, value);
    return 0;
}

// Returns a pointer to the value for the given key, or NULL if not found.
static void *UnsafeDictionary_Get(UnsafeDictionary *dict, const void *key, uint32_t key_len) {
    int32_t node_idx = UnsafeDictionary_Walk(dict, key, key_len, 0);
    if (node_idx == UNSAFEDICT_EMPTY) return NULL;

    UnsafeDictNode *node = (UnsafeDictNode *)UnsafeArray_Get(dict->nodes, (uint32_t)node_idx);
    if (node->value == UNSAFEDICT_EMPTY) return NULL;

    return UnsafeArray_Get(dict->values, (uint32_t)node->value);
}

// Returns 1 if the key exists, 0 otherwise.
static int UnsafeDictionary_Has(UnsafeDictionary *dict, const void *key, uint32_t key_len) {
    return UnsafeDictionary_Get(dict, key, key_len) != NULL;
}

// Removes a key from the dictionary. Returns 0 on success, -1 if not found.
// Note: the value slot is not reclaimed, only the trie link is cleared.
static int UnsafeDictionary_Remove(UnsafeDictionary *dict, const void *key, uint32_t key_len) {
    int32_t node_idx = UnsafeDictionary_Walk(dict, key, key_len, 0);
    if (node_idx == UNSAFEDICT_EMPTY) return -1;

    UnsafeDictNode *node = (UnsafeDictNode *)UnsafeArray_Get(dict->nodes, (uint32_t)node_idx);
    if (node->value == UNSAFEDICT_EMPTY) return -1;

    node->value = UNSAFEDICT_EMPTY;
    return 0;
}

#define UnsafeDictionary_GetValue(dict, key, key_len, type) \
    (*(type *)UnsafeDictionary_Get(dict, key, key_len))

#define UnsafeDictionary_SetValue(dict, key, key_len, type, value) do { \
    type _ud_tmp = (value); \
    UnsafeDictionary_Set(dict, key, key_len, &_ud_tmp); \
} while (0)

// Compile-time string literal length. Rejects char* pointers at compile time.
#define _UNSAFE_STRLITERAL_LEN(s) ({ \
    _Static_assert( \
        !__builtin_types_compatible_p(__typeof__(s), char *) && \
        !__builtin_types_compatible_p(__typeof__(s), const char *), \
        "S-macros require string literals, not char* pointers"); \
    (uint32_t)(sizeof(s) - 1); \
})

// String literal key convenience macros — length computed at compile time.
// Only accepts string literals (e.g. "health"), NOT char* variables.
#define UnsafeDictionary_SSet(dict, str_key, value_ptr)              UnsafeDictionary_Set(dict, str_key, _UNSAFE_STRLITERAL_LEN(str_key), value_ptr)
#define UnsafeDictionary_SGet(dict, str_key)                         UnsafeDictionary_Get(dict, str_key, _UNSAFE_STRLITERAL_LEN(str_key))
#define UnsafeDictionary_SHas(dict, str_key)                         UnsafeDictionary_Has(dict, str_key, _UNSAFE_STRLITERAL_LEN(str_key))
#define UnsafeDictionary_SRemove(dict, str_key)                      UnsafeDictionary_Remove(dict, str_key, _UNSAFE_STRLITERAL_LEN(str_key))
#define UnsafeDictionary_SGetValue(dict, str_key, type)              UnsafeDictionary_GetValue(dict, str_key, _UNSAFE_STRLITERAL_LEN(str_key), type)
#define UnsafeDictionary_SSetValue(dict, str_key, type, value)       UnsafeDictionary_SetValue(dict, str_key, _UNSAFE_STRLITERAL_LEN(str_key), type, value)

typedef void (*UnsafeDictFormatter)(const void *value, char *buf, uint32_t buf_size);

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

// Prints the dictionary with a value formatter. string_keys=1 prints keys as strings, 0 as hex.
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

// Prints the dictionary using a printf-style format for values.
//   UnsafeDictionary_PrintF(dict, int, "%d", 1);       // string keys
//   UnsafeDictionary_PrintF(dict, float, "%.2f", 0);   // hex keys
#define UnsafeDictionary_PrintF(dict, type, fmt, string_keys) do { \
    void _udf_fn(const void *_v, char *_b, uint32_t _s) { \
        snprintf(_b, _s, fmt, *(const type *)_v); \
    } \
    UnsafeDictionary_Print(dict, _udf_fn, string_keys); \
} while (0)

// --- Log variants (LOG_INFO per line, no \n needed) ---

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

// Logs the dictionary using LOG_INFO for each line via a value formatter callback.
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

// Logs the dictionary using LOG_INFO with a printf-style format for values.
//   UnsafeDictionary_LogF(dict, int, "%d", 1);       // string keys
//   UnsafeDictionary_LogF(dict, float, "%.2f", 0);   // hex keys
#define UnsafeDictionary_LogF(dict, type, fmt, string_keys) do { \
    void _udlf_fn(const void *_v, char *_b, uint32_t _s) { \
        snprintf(_b, _s, fmt, *(const type *)_v); \
    } \
    UnsafeDictionary_Log(dict, _udlf_fn, string_keys); \
} while (0)
