# Hard-Coded Limits and Magic Numbers

Reference for every hard-coded limit and magic number in the Enatrio engine.

---

## Class System (src/system/class/Class.h)

| Define | Value | Purpose | When to change |
|---|---|---|---|
| `CLASSID_MAX` | `UINT16_MAX` (65535) | Maximum ClassID value. ClassDefinitions array is sized to CLASSID_MAX+1. | Unlikely. 65535 classes is far more than any game needs. Reducing it saves ~5.6MB of BSS but limits class count. |
| `CLASS_MAXNAMELENGTH` | 64 | Max characters in a class name string. | Only if class names need to be longer. Keep in mind this is per-entry in ClassDefinitions. |

## Collections -- ByteStream (src/system/cts/ByteStream.h)

| Define | Value | Purpose | When to change |
|---|---|---|---|
| `BYTESTREAM_MAX_SIZE` | 1 GB (1024^3) | Maximum buffer size a ByteStream can grow to. Also caps file load size. | Reduce for memory-constrained platforms. Increase if you need to serialize very large data sets. Override with `#define BYTESTREAM_MAX_SIZE value` before including ByteStream.h. |

## Collections -- UnsafeDictionary (src/system/cts/UnsafeDictionary.h)

| Define | Value | Purpose | When to change |
|---|---|---|---|
| `UNSAFEDICT_MAX_KEY_LEN` | 256 | Maximum key length in bytes for trie-based dictionaries. | Increase if you need longer keys. Each extra byte adds 4 trie levels, increasing memory usage. |

## Collections -- UnsafeHashMap (src/system/cts/UnsafeHashMap.h)

| Define | Value | Purpose | When to change |
|---|---|---|---|
| `UNSAFEHASHMAP_MAX_KEY_LEN` | 256 | Maximum key length in bytes. | Match with UNSAFEDICT_MAX_KEY_LEN if both are used for the same keys. |
| `UNSAFEHASHMAP_DEFAULT_CAPACITY` | 64 | Initial bucket count when creating a hash map. Always a power of 2. | Increase if you know the map will hold many entries (avoids early rehashes). Decrease for small maps to save memory. |
| `UNSAFEHASHMAP_LOAD_FACTOR_NUM` | 7 | Load factor numerator (7/10 = 70%). | Together with DEN, controls when the table rehashes. Lower = more memory, fewer collisions. Higher = less memory, more collisions. |
| `UNSAFEHASHMAP_LOAD_FACTOR_DEN` | 10 | Load factor denominator. | See above. |

## Object System -- Value Storage (src/system/object/ObjectHelpers.h)

| Define | Value | Purpose | When to change |
|---|---|---|---|
| `_OBJECT_VALUE_MAX_SIZE` | 64 KB (64*1024) | Maximum size of a single value stored in an object's values hashmap. Prevents stack overflow from attacker-controlled deserialized data. | Increase if game objects legitimately need to store very large value blobs (e.g., embedded bitmaps). |

## Object System -- Garbage Collection (src/system/object/ObjectRef.h)

| Define | Value | Purpose | When to change |
|---|---|---|---|
| `_GC_MAX_RECURSION_DEPTH` | 256 | Maximum recursion depth for cycle collection. Prevents stack overflow from long orphan chains. | Increase if you have very deep reference chains that need single-pass collection. 256 handles chains up to 256 objects deep. |

## Object System -- Serialization Registry (src/system/object/ObjectTypes.h)

| Define | Value | Purpose | When to change |
|---|---|---|---|
| `SER_MAX_ID` | 256 | Maximum number of serialization function pairs (IDs 0-255). Built-ins use 0-3, user IDs start at 16. | Increase if you need more than 240 custom serialization handlers. |

## Object System -- Serialization Limits (src/system/object/Serialization.h)

These are security limits that cap what a .cob file can request during deserialization.
They prevent malicious save files from causing excessive memory allocation, CPU burn,
or buffer overflows.

| Define | Value | Purpose | When to change |
|---|---|---|---|
| `SER_MAX_OBJECTS` | 1 million (1024^2) | Maximum number of objects in a single serialized graph. | Increase for games with very large worlds. Decrease for mobile/embedded to limit memory risk. |
| `SER_MAX_VALUES_PER_OBJECT` | 64K (64*1024) | Maximum number of values a single object can have in a .cob file. | Unlikely to need changing. 64K values per object is far beyond normal use. |
| `SER_MAX_KEY_LEN` | 256 | Maximum key length in bytes during deserialization. Matches dictionary/hashmap limits. | Keep in sync with UNSAFEDICT_MAX_KEY_LEN and UNSAFEHASHMAP_MAX_KEY_LEN. |
| `SER_MAX_DATA_SIZE` | 1 MB (1024^2) | Maximum size of a single serialized value in bytes. | Increase if objects store very large blobs. Must be <= _OBJECT_VALUE_MAX_SIZE for the restored data to be accepted. |

---

## Overriding Limits

All limits wrapped in `#ifndef` guards can be overridden by defining them before
including the header:

```c
#define BYTESTREAM_MAX_SIZE (512u * 1024u * 1024u)  // 512 MB
#include "system/cts/ByteStream.h"
```

Or globally via compiler flags:

```
-DBYTESTREAM_MAX_SIZE=536870912
```

Limits NOT wrapped in `#ifndef` (like `CLASSID_MAX`, `UNSAFEDICT_MAX_KEY_LEN`) require
editing the header directly.

## Notes

- All size limits use `uint32_t` arithmetic. The theoretical maximum for any single
  allocation is ~4 GB (UINT32_MAX), but practical limits are much lower due to
  platform memory constraints.
- Serialization limits are SECURITY boundaries, not just performance tuning. Increasing
  them widens the attack surface for malicious .cob files.
- The load factor for UnsafeHashMap is a performance tradeoff, not a security boundary.
