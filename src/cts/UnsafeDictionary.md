# UnsafeDictionary

A generic key-value store backed by a quaternary trie (2-bit radix). Keys are arbitrary byte sequences (max 256 bytes). Values are stored in an UnsafeArray.

```c
#include "cts/UnsafeDictionary.h"
```

## How It Works

Each byte of the key is split into four 2-bit pairs (MSB first). Each pair selects one of 4 children in the trie. A key of N bytes descends 4N levels. Leaf nodes store an index into the values array.

This gives O(key_length) lookup regardless of how many entries exist.

## Creating and Destroying

```c
// Create with value element size and initial capacity
UnsafeDictionary *dict = UnsafeDictionary_Create(sizeof(int), 16);

// Always destroy when done (frees trie nodes and values)
UnsafeDictionary_Destroy(dict);
```

## Inserting

```c
// Via pointer
int val = 100;
int err = UnsafeDictionary_Set(dict, "health", 6, &val);
// Returns 0 on success, -1 if key already exists or key > 256 bytes

// Via value macro
UnsafeDictionary_SetValue(dict, "mana", 4, int, 50);
```

**Set does NOT overwrite.** If the key already exists, it returns -1. This prevents silent loss of pointers or heap-allocated values that the caller is responsible for disposing. If you need to update a value, use `Get` and modify in-place.

## Reading

```c
// Via pointer (returns void*, NULL if not found)
int *hp = (int *)UnsafeDictionary_Get(dict, "health", 6);
if (hp) printf("health = %d\n", *hp);

// Via value macro (do NOT use if key might not exist — no NULL check)
int val = UnsafeDictionary_GetValue(dict, "health", 6, int);

// Check existence first
if (UnsafeDictionary_Has(dict, "health", 6)) {
    int val = UnsafeDictionary_GetValue(dict, "health", 6, int);
}
```

## Modifying Values In-Place

Get returns a pointer into the backing array — you can modify directly:

```c
int *hp = (int *)UnsafeDictionary_Get(dict, "health", 6);
*hp = 80;  // changed from 100 to 80, no re-insert needed
```

## Removing

```c
int err = UnsafeDictionary_Remove(dict, "mana", 4);
// Returns 0 on success, -1 if key not found
// Note: the value slot in the backing array is not reclaimed
```

## Key Types

Keys are raw bytes. Any type works as long as you pass the correct length.

### String Keys

Use the `S`-prefixed macros to avoid specifying length. These **only accept string literals** — passing a `char*` variable is a compile error (prevents overflow attacks from runtime strings).

```c
UnsafeDictionary_SSetValue(dict, "health", int, 100);
int hp = UnsafeDictionary_SGetValue(dict, "health", int);
int *ptr = (int *)UnsafeDictionary_SGet(dict, "health");
UnsafeDictionary_SHas(dict, "health");               // 1
UnsafeDictionary_SRemove(dict, "health");

// Or with pointer to value
int val = 50;
UnsafeDictionary_SSet(dict, "mana", &val);

// This will NOT compile:
const char *key = get_user_input();
UnsafeDictionary_SSet(dict, key, &val);  // error: S-macros require string literals
```

Length is computed at compile time via `sizeof` — zero runtime cost.

For runtime/dynamic string keys, use the standard API with explicit length and **always validate before passing**:

```c
// SAFE: validate the key before using it
const char *key = get_user_input();

// 1. Null check
if (!key) { /* handle error */ }

// 2. Clamp length to max key size (256 bytes)
uint32_t key_len = (uint32_t)strnlen(key, UNSAFEDICT_MAX_KEY_LEN + 1);
if (key_len > UNSAFEDICT_MAX_KEY_LEN) {
    // key too long — reject or truncate
    LOG_WARNING("Key too long (%u bytes), max is %d", key_len, UNSAFEDICT_MAX_KEY_LEN);
    return;
}

// 3. Now safe to use
UnsafeDictionary_Set(dict, key, key_len, &val);
UnsafeDictionary_Get(dict, key, key_len);
```

**Never pass `strlen()` on untrusted input without a length cap** — use `strnlen()` with `UNSAFEDICT_MAX_KEY_LEN` as the bound. The dictionary will also return -1 if key_len exceeds 256, but validating early avoids unnecessary trie traversal.

### Integer Keys

```c
uint32_t id = 42;
UnsafeDictionary_Set(dict, &id, sizeof(id), &val);
UnsafeDictionary_Get(dict, &id, sizeof(id));
```

### Struct Keys

```c
typedef struct { int x, y; } TilePos;
TilePos pos = {3, 7};
UnsafeDictionary_Set(dict, &pos, sizeof(pos), &val);
```

**Be consistent with key length.** `"health"` with length 6 and length 7 (including null) are different keys.

## Storing Structs as Values

```c
typedef struct { float x, y; } Vec2;
UnsafeDictionary *positions = UnsafeDictionary_Create(sizeof(Vec2), 32);

Vec2 pos = {1.5f, 2.5f};
UnsafeDictionary_Set(positions, "player", 6, &pos);

Vec2 *p = (Vec2 *)UnsafeDictionary_Get(positions, "player", 6);
p->x = 10.0f;  // modify in-place
```

## Storing Pointers as Values

```c
UnsafeDictionary *entities = UnsafeDictionary_Create(sizeof(Entity*), 32);

Entity *e = create_entity();
UnsafeDictionary_Set(entities, "boss", 4, &e);

Entity *got = UnsafeDictionary_GetValue(entities, "boss", 4, Entity*);
```

## Printing / Logging

### String Keys

```c
// printf-based
UnsafeDictionary_PrintF(dict, int, "%d", 1);

// LOG_INFO-based (one LOG_INFO call per line)
UnsafeDictionary_LogF(dict, int, "%d", 1);
```

Output:
```
UnsafeDictionary[3 entries, 57 nodes] {
  "armor" => 25
  "health" => 100
  "mana" => 50
}
```

### Hex Keys (for non-string keys)

```c
UnsafeDictionary_PrintF(dict, int, "%d", 0);
UnsafeDictionary_LogF(dict, int, "%d", 0);
```

Output:
```
UnsafeDictionary[3 entries, 43 nodes] {
  [01 00 00 00] => 100
  [02 00 00 00] => 200
  [03 00 00 00] => 300
}
```

### Custom Formatter (callback)

```c
void fmt_vec2(const void *value, char *buf, uint32_t buf_size) {
    Vec2 *v = (Vec2 *)value;
    snprintf(buf, buf_size, "(%.1f, %.1f)", v->x, v->y);
}
UnsafeDictionary_Print(dict, fmt_vec2, 1);    // printf, string keys
UnsafeDictionary_Log(dict, fmt_vec2, 1);      // LOG_INFO, string keys
```

## API Reference

| Function | Returns | Description |
|---|---|---|
| `Create(elem_size, capacity)` | `UnsafeDictionary*` | Create new dictionary |
| `Destroy(dict)` | `void` | Free all memory |
| `Set(dict, key, key_len, &val)` | `int` | Insert (0=ok, -1=exists/too long) |
| `Get(dict, key, key_len)` | `void*` | Lookup (NULL if missing) |
| `Has(dict, key, key_len)` | `int` | 1 if exists, 0 if not |
| `Remove(dict, key, key_len)` | `int` | Remove (0=ok, -1=missing) |
| `SetValue(dict, key, len, type, val)` | macro | Insert by value |
| `GetValue(dict, key, len, type)` | macro | Read by value |
| `Print(dict, formatter, string_keys)` | `void` | printf output |
| `PrintF(dict, type, fmt, string_keys)` | macro | printf with format |
| `Log(dict, formatter, string_keys)` | `void` | LOG_INFO output |
| `LogF(dict, type, fmt, string_keys)` | macro | LOG_INFO with format |
| `SSet(dict, str_key, &val)` | macro | Insert with string literal key |
| `SGet(dict, str_key)` | macro | Lookup with string literal key |
| `SHas(dict, str_key)` | macro | Exists check with string literal key |
| `SRemove(dict, str_key)` | macro | Remove with string literal key |
| `SSetValue(dict, str_key, type, val)` | macro | Insert by value, string literal key |
| `SGetValue(dict, str_key, type)` | macro | Read by value, string literal key |

All `S`-macros reject `char*` at compile time — string literals only.

## Limitations

- Max key length: 256 bytes (returns -1 if exceeded)
- Set does not overwrite — returns -1 if key exists (prevents lost pointers / undisposed values)
- Remove does not reclaim the value slot (trie link is cleared, value stays in backing array)
- Iteration order is trie-order (sorted by raw key bytes), not insertion order
