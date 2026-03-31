# UnsafeHashMap

A generic key-value store backed by an open-addressing hash table with linear probing. Keys are arbitrary byte sequences (max 256 bytes). Values are stored in an UnsafeArray.

```c
#include "cts/UnsafeHashMap.h"
```

## How It Works

Keys are hashed with FNV-1a. Buckets are a power-of-2 array. Collisions are resolved by linear probing. Deleted entries use tombstone markers so probe chains stay intact. The table rehashes (doubles) when the load factor exceeds 70%.

This gives O(1) average-case lookup regardless of key length.

## Creating and Destroying

```c
// Create with value element size and initial capacity
UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 16);

// Always destroy when done (frees all bucket keys and values)
UnsafeHashMap_Destroy(map);
```

## Inserting

```c
// Via pointer
int val = 100;
int err = UnsafeHashMap_Set(map, "health", 6, &val);
// Returns 0 on success, -1 if key already exists or key > 256 bytes

// Via value macro
UnsafeHashMap_SetValue(map, "mana", 4, int, 50);
```

**Set does NOT overwrite.** If the key already exists, it returns -1. This prevents silent loss of pointers or heap-allocated values that the caller is responsible for disposing. If you need to update a value, use `Get` and modify in-place.

## Reading

```c
// Via pointer (returns void*, NULL if not found)
int *hp = (int *)UnsafeHashMap_Get(map, "health", 6);
if (hp) printf("health = %d\n", *hp);

// Via value macro (do NOT use if key might not exist -- no NULL check)
int val = UnsafeHashMap_GetValue(map, "health", 6, int);

// Check existence first
if (UnsafeHashMap_Has(map, "health", 6)) {
    int val = UnsafeHashMap_GetValue(map, "health", 6, int);
}
```

## Modifying Values In-Place

Get returns a pointer into the backing array -- you can modify directly:

```c
int *hp = (int *)UnsafeHashMap_Get(map, "health", 6);
*hp = 80;  // changed from 100 to 80, no re-insert needed
```

## Removing

```c
int err = UnsafeHashMap_Remove(map, "mana", 4);
// Returns 0 on success, -1 if key not found
// Note: the value slot in the backing array is not reclaimed
```

Removed entries are marked with a tombstone so that linear probing continues to work correctly for keys that were inserted after the removed entry.

## Key Types

Keys are raw bytes. Any type works as long as you pass the correct length.

### String Keys

Use the `S`-prefixed macros to avoid specifying length. These **only accept string literals** -- passing a `char*` variable is a compile error (prevents overflow attacks from runtime strings).

```c
UnsafeHashMap_SSetValue(map, "health", int, 100);
int hp = UnsafeHashMap_SGetValue(map, "health", int);
int *ptr = (int *)UnsafeHashMap_SGet(map, "health");
UnsafeHashMap_SHas(map, "health");               // 1
UnsafeHashMap_SRemove(map, "health");

// Or with pointer to value
int val = 50;
UnsafeHashMap_SSet(map, "mana", &val);

// This will NOT compile:
const char *key = get_user_input();
UnsafeHashMap_SSet(map, key, &val);  // error: S-macros require string literals
```

Length is computed at compile time via `sizeof` -- zero runtime cost.

For runtime/dynamic string keys, use the standard API with explicit length and **always validate before passing**:

```c
// SAFE: validate the key before using it
const char *key = get_user_input();

// 1. Null check
if (!key) { /* handle error */ }

// 2. Clamp length to max key size (256 bytes)
uint32_t key_len = (uint32_t)strnlen(key, UNSAFEHASHMAP_MAX_KEY_LEN + 1);
if (key_len > UNSAFEHASHMAP_MAX_KEY_LEN) {
    // key too long -- reject or truncate
    LOG_WARNING("Key too long (%u bytes), max is %d", key_len, UNSAFEHASHMAP_MAX_KEY_LEN);
    return;
}

// 3. Now safe to use
UnsafeHashMap_Set(map, key, key_len, &val);
UnsafeHashMap_Get(map, key, key_len);
```

**Never pass `strlen()` on untrusted input without a length cap** -- use `strnlen()` with `UNSAFEHASHMAP_MAX_KEY_LEN` as the bound. The hash map will also return -1 if key_len exceeds 256, but validating early avoids unnecessary work.

### Integer Keys

```c
uint32_t id = 42;
UnsafeHashMap_Set(map, &id, sizeof(id), &val);
UnsafeHashMap_Get(map, &id, sizeof(id));
```

### Struct Keys

```c
typedef struct { int x, y; } TilePos;
TilePos pos = {3, 7};
UnsafeHashMap_Set(map, &pos, sizeof(pos), &val);
```

**Be consistent with key length.** `"health"` with length 6 and length 7 (including null) are different keys.

## Storing Structs as Values

```c
typedef struct { float x, y; } Vec2;
UnsafeHashMap *positions = UnsafeHashMap_Create(sizeof(Vec2), 32);

Vec2 pos = {1.5f, 2.5f};
UnsafeHashMap_Set(positions, "player", 6, &pos);

Vec2 *p = (Vec2 *)UnsafeHashMap_Get(positions, "player", 6);
p->x = 10.0f;  // modify in-place
```

## Storing Pointers as Values

```c
UnsafeHashMap *entities = UnsafeHashMap_Create(sizeof(Entity*), 32);

Entity *e = create_entity();
UnsafeHashMap_Set(entities, "boss", 4, &e);

Entity *got = UnsafeHashMap_GetValue(entities, "boss", 4, Entity*);
```

## Printing / Logging

### String Keys

```c
// printf-based
UnsafeHashMap_PrintF(map, int, "%d", 1);

// LOG_INFO-based (one LOG_INFO call per line)
UnsafeHashMap_LogF(map, int, "%d", 1);
```

Output:
```
UnsafeHashMap[3 entries, 64 buckets] {
  "mana" => 50
  "health" => 100
  "armor" => 25
}
```

### Hex Keys (for non-string keys)

```c
UnsafeHashMap_PrintF(map, int, "%d", 0);
UnsafeHashMap_LogF(map, int, "%d", 0);
```

Output:
```
UnsafeHashMap[3 entries, 64 buckets] {
  [01 00 00 00] => 100
  [03 00 00 00] => 300
  [02 00 00 00] => 200
}
```

### Custom Formatter (callback)

```c
void fmt_vec2(const void *value, char *buf, uint32_t buf_size) {
    Vec2 *v = (Vec2 *)value;
    snprintf(buf, buf_size, "(%.1f, %.1f)", v->x, v->y);
}
UnsafeHashMap_Print(map, fmt_vec2, 1);    // printf, string keys
UnsafeHashMap_Log(map, fmt_vec2, 1);      // LOG_INFO, string keys
```

## UnsafeDictionary vs UnsafeHashMap

| | UnsafeDictionary | UnsafeHashMap |
|---|---|---|
| Backing structure | Quaternary trie (2-bit radix) | Open-addressing hash table |
| Lookup complexity | O(key_length) | O(1) average |
| Memory overhead | High (many trie nodes) | Low (flat bucket array) |
| Iteration order | Sorted by raw key bytes | Unordered (hash-dependent) |
| Best for | Prefix queries, sorted iteration | Fast random access, large entry counts |

The API is identical -- switching between them requires only changing the type name.

## API Reference

| Function | Returns | Description |
|---|---|---|
| `Create(elem_size, capacity)` | `UnsafeHashMap*` | Create new hash map |
| `Destroy(map)` | `void` | Free all memory |
| `Set(map, key, key_len, &val)` | `int` | Insert (0=ok, -1=exists/too long) |
| `Get(map, key, key_len)` | `void*` | Lookup (NULL if missing) |
| `Has(map, key, key_len)` | `int` | 1 if exists, 0 if not |
| `Remove(map, key, key_len)` | `int` | Remove (0=ok, -1=missing) |
| `SetValue(map, key, len, type, val)` | macro | Insert by value |
| `GetValue(map, key, len, type)` | macro | Read by value |
| `Print(map, formatter, string_keys)` | `void` | printf output |
| `PrintF(map, type, fmt, string_keys)` | macro | printf with format |
| `Log(map, formatter, string_keys)` | `void` | LOG_INFO output |
| `LogF(map, type, fmt, string_keys)` | macro | LOG_INFO with format |
| `SSet(map, str_key, &val)` | macro | Insert with string literal key |
| `SGet(map, str_key)` | macro | Lookup with string literal key |
| `SHas(map, str_key)` | macro | Exists check with string literal key |
| `SRemove(map, str_key)` | macro | Remove with string literal key |
| `SSetValue(map, str_key, type, val)` | macro | Insert by value, string literal key |
| `SGetValue(map, str_key, type)` | macro | Read by value, string literal key |

All `S`-macros reject `char*` at compile time -- string literals only. Requires `_UNSAFE_STRLITERAL_LEN` from `UnsafeDictionary.h`.

## Limitations

- Max key length: 256 bytes (returns -1 if exceeded)
- Set does not overwrite -- returns -1 if key exists (prevents lost pointers / undisposed values)
- Remove does not reclaim the value slot (bucket is tombstoned, value stays in backing array)
- Iteration order is hash-dependent, not insertion order or sorted order
- Bucket count is always a power of 2
