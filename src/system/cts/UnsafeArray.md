# UnsafeArray

A generic, type-erased dynamic array. Stores elements as raw bytes with minimal overhead, hence "Unsafe".

This header also defines the **shared varied-data allocator** used by UnsafeVariedHashMap and UnsafeVariedDictionary for data buffer region management (free list with adjacent coalescing).

Located in `src/cts/` (cts = collections).

## Headers

| Header | Purpose |
|---|---|
| `UnsafeArray.h` | Core array (create, add, remove, get, set, print, log) |
| `UnsafeArrayLINQ.h` | LINQ-style queries (filter, sort, aggregate, etc.) |

## Creating and Destroying

```c
#include "cts/UnsafeArray.h"

// Create with element size and initial capacity (auto-grows when full)
UnsafeArray *arr = UnsafeArray_Create(sizeof(int), 16);

// Always destroy when done
UnsafeArray_Destroy(arr);
```

## Adding Elements

```c
int val = 42;
UnsafeArray_Add(arr, &val);          // append via pointer

// Capacity grows automatically (doubles) when full
for (int i = 0; i < 1000; i++)
    UnsafeArray_Add(arr, &i);        // no need to worry about capacity
```

### Bulk Add

```c
int batch[4] = {10, 20, 30, 40};
UnsafeArray_AddBulk(arr, batch, 4);  // append 4 elements in one memcpy
// Returns 0 on success, -1 on allocation failure
// Grows capacity as needed before copying
```

## Reading Elements

```c
// Via pointer (returns void*, NULL if index >= count)
int *ptr = (int *)UnsafeArray_Get(arr, 0);
if (ptr) printf("val = %d\n", *ptr);

// Via dereference macro (returns the value directly)
// WARNING: dereferences NULL if index is out of bounds -- check first
int val = UnsafeArray_GetDeref(arr, 0, int);
```

**Get is bounds-checked.** Returns NULL if `index >= arr->count`. Always check the return value when the index might be out of range.

## Writing Elements

```c
// Via pointer
int val = 99;
UnsafeArray_Set(arr, 0, &val);

// Via value macro
UnsafeArray_SetValue(arr, 0, int, 99);
```

## Removing Elements

```c
// Order-preserving remove (shifts elements down, O(n))
UnsafeArray_Remove(arr, 2);

// Fast swap-remove (swaps with last element, O(1), changes order)
UnsafeArray_RemoveSwap(arr, 2);

// Both return -1 if index is out of bounds
```

## Other Operations

```c
UnsafeArray_Clear(arr);          // reset count to 0 (does not free memory)
UnsafeArray_ShrinkToFit(arr);    // realloc buffer down to count (frees excess capacity)
arr->count;                      // current element count
arr->capacity;                   // current capacity
arr->element_size;               // size of each element in bytes
```

### ShrinkToFit

```c
// After removing many elements, reclaim unused memory:
UnsafeArray_ShrinkToFit(arr);
// Returns 0 on success, -1 on realloc failure
// No-op if count == capacity or count == 0
// After success, capacity == count
```

## Storing Pointers

When storing pointers, the element size is `sizeof(MyType*)`:

```c
UnsafeArray *ptrs = UnsafeArray_Create(sizeof(Entity*), 16);

Entity *e = &some_entity;
UnsafeArray_Add(ptrs, &e);                              // add pointer

Entity *got = UnsafeArray_GetDeref(ptrs, 0, Entity*);   // get pointer back
UnsafeArray_SetValue(ptrs, 0, Entity*, other_entity);   // overwrite
```

## Storing Structs

```c
typedef struct { float x, y; } Vec2;
UnsafeArray *positions = UnsafeArray_Create(sizeof(Vec2), 64);

Vec2 pos = {1.0f, 2.0f};
UnsafeArray_Add(positions, &pos);

Vec2 *p = (Vec2 *)UnsafeArray_Get(positions, 0);   // pointer into array
p->x = 5.0f;                                        // modify in-place
```

## Printing / Logging

```c
// printf-based (uses \n, good for stdout)
UnsafeArray_PrintF(arr, int, "%d");
UnsafeArray_PrintF(arr, float, "%.2f");

// LOG_INFO-based (one line per call, goes to log file)
UnsafeArray_LogF(arr, int, "%d");
UnsafeArray_LogF(arr, float, "%.2f");

// Callback-based for custom formatting
void my_fmt(uint32_t index, const void *elem, char *buf, uint32_t buf_size) {
    Vec2 *v = (Vec2 *)elem;
    snprintf(buf, buf_size, "(%.1f, %.1f)", v->x, v->y);
}
UnsafeArray_Print(arr, my_fmt);    // printf version
UnsafeArray_Log(arr, my_fmt);      // LOG_INFO version
```

Output:
```
UnsafeArray[4/8] (elem 4 bytes) {
  [0] 10
  [1] 20
  [2] 30
  [3] 40
}
```

---

# UnsafeArrayLINQ

LINQ-style operations for UnsafeArray. Two flavors: **function pointer** versions and **macro** versions with inline expressions.

```c
#include "cts/UnsafeArrayLINQ.h"
```

## Function Pointer API

Requires defining callbacks:

```c
int is_even(const void *e) { return (*(const int *)e) % 2 == 0; }
int int_cmp(const void *a, const void *b) { return *(const int *)a - *(const int *)b; }
```

### Querying (returns new array -- caller must Destroy)

```c
UnsafeArray *evens = UnsafeArray_Where(arr, is_even);    // filter
UnsafeArray *mapped = UnsafeArray_Select(arr, xform, sizeof(float)); // transform

UnsafeArray *tail  = UnsafeArray_Skip(arr, 3);           // skip first 3
UnsafeArray *head  = UnsafeArray_Take(arr, 5);           // first 5 only
UnsafeArray *sw    = UnsafeArray_SkipWhile(arr, pred);   // skip while true
UnsafeArray *tw    = UnsafeArray_TakeWhile(arr, pred);   // take while true
UnsafeArray *both  = UnsafeArray_Concat(a, b);           // join two arrays
```

### Searching (returns pointer into original array)

```c
void *first = UnsafeArray_First(arr, is_even);     // first match or NULL
void *last  = UnsafeArray_Last(arr, is_even);      // last match or NULL
int32_t idx = UnsafeArray_IndexOf(arr, is_even);   // index or -1
int32_t li  = UnsafeArray_LastIndexOf(arr, is_even);
```

### Predicates

```c
UnsafeArray_Any(arr, is_even);            // 1 if any match
UnsafeArray_All(arr, is_even);            // 1 if all match
UnsafeArray_Contains(arr, &value);        // 1 if found (memcmp)
uint32_t n = UnsafeArray_Count(arr, is_even);
```

### Aggregation

```c
void sum_fn(void *acc, const void *e) { *(int *)acc += *(const int *)e; }
int total = 0;
UnsafeArray_Aggregate(arr, &total, sum_fn);

void *lo = UnsafeArray_Min(arr, int_cmp);    // pointer to min
void *hi = UnsafeArray_Max(arr, int_cmp);    // pointer to max
```

### In-Place Mutation

```c
UnsafeArray_OrderBy(arr, int_cmp);                // sort
UnsafeArray_Reverse(arr);                         // reverse
UnsafeArray_Shuffle(arr);                         // Fisher-Yates (call srand() first)
UnsafeArray_Distinct(arr);                        // remove duplicates
uint32_t n = UnsafeArray_RemoveAll(arr, is_even); // remove matching

void negate(void *e) { *(int *)e = -(*(int *)e); }
UnsafeArray_ForEach(arr, negate);                 // apply to each
```

## LINQ Macros (inline expressions, no callbacks needed)

### Filtering & Searching

```c
UnsafeArray *result = LINQ_WHERE(arr, int, x, x > 5 && x % 2 == 0);
int *found          = LINQ_FIRST(arr, int, x, x == 42);
int *last           = LINQ_LAST(arr, int, x, x < 0);
int32_t idx         = LINQ_INDEXOF(arr, int, x, x == 42);
```

### Predicates

```c
LINQ_ANY(arr, int, x, x < 0)           // 1/0
LINQ_ALL(arr, int, x, x > 0)           // 1/0
uint32_t n = LINQ_COUNT(arr, int, x, x % 3 == 0);
```

### Transform

```c
UnsafeArray *doubled = LINQ_SELECT(arr, int, int, x, x * 2);
UnsafeArray *floats  = LINQ_SELECT(arr, int, float, x, x * 0.5f);  // type change
```

### Iteration

```c
LINQ_FOREACH(arr, int, x, { printf("%d\n", x); });        // read-only copy
LINQ_FOREACH_REF(arr, int, p, { *p *= 2; });              // mutable pointer
```

### Removal

```c
uint32_t removed = LINQ_REMOVEALL(arr, int, x, x < 0);
```

### Aggregation

```c
int sum     = LINQ_SUM(arr, int);
int product = LINQ_PRODUCT(arr, int);
int lo      = LINQ_MIN(arr, int);
int hi      = LINQ_MAX(arr, int);
int custom  = LINQ_AGGREGATE(arr, int, int, acc, x, 0, acc + x);
```

### Chaining

```c
UnsafeArray *evens = LINQ_WHERE(arr, int, x, x % 2 == 0);
UnsafeArray_OrderBy(evens, int_cmp);
UnsafeArray *top3 = UnsafeArray_Take(evens, 3);
int sum = LINQ_SUM(top3, int);
UnsafeArray_Destroy(top3);
UnsafeArray_Destroy(evens);
```

## Memory Rules

| Operation | Allocates? | You must Destroy? |
|---|---|---|
| `Where`, `Select`, `Skip`, `Take`, `SkipWhile`, `TakeWhile`, `Concat` | Yes | Yes |
| `LINQ_WHERE`, `LINQ_SELECT` | Yes | Yes |
| `OrderBy`, `Reverse`, `Shuffle`, `Distinct`, `RemoveAll`, `ForEach` | No | No (in-place) |
| `First`, `Last`, `Min`, `Max` | No | No (returns pointer into source) |

---

## Shared Varied-Data Allocator

UnsafeArray.h defines the internal allocator used by UnsafeVariedHashMap and UnsafeVariedDictionary for their data buffers. This is not part of the public API but is documented here because it lives in this header.

### How It Works

Varied-size collections store values in a flat byte buffer (`UnsafeArray` with `element_size = 1`). When a value is removed, its byte region is added to a free list (`UnsafeArray` of `_UnsafeVariedFreeRegion`). When a new value is written, the allocator uses first-fit from the free list before appending to the end of the buffer.

### Adjacent Coalescing

When a region is freed via `_UnsafeVaried_FreeData`, it checks for adjacent free regions and merges them. This prevents fragmentation from repeated insert/remove cycles:

- If the freed region is immediately after an existing free region, they merge
- If the freed region is immediately before an existing free region, they merge
- After one merge, a second pass checks if the merged region can coalesce with yet another neighbor

### Data Structures

| Type | Purpose |
|---|---|
| `_UnsafeVariedFreeRegion` | `{ uint32_t offset, size }` -- one free byte region |
| `_UnsafeVaried_FreeData(list, offset, size)` | Return a byte region to the free list (with coalescing) |
| `_UnsafeVaried_WriteData(data, list, value, size)` | Write bytes using first-fit from free list, or append |
