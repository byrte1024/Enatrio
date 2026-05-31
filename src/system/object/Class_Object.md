# Object System -- Managed Lifecycle, References, and Garbage Collection

## Overview

The object system provides managed object lifecycle with reference counting and
cycle collection. Objects are containers that hold data -- a values hashmap for
primitive data and a references hashmap for links to other objects. The system is
built on top of the class system: each object has a ClassID and receives messages
dispatched through the class message router.

All three pointer types (`TempObjectReference`, `ObjectReference`,
`ExternalReference`) are aliases for `ObjectContainer*`. The distinction is
purely semantic and encodes ownership for the garbage collector.

## Include

```c
#include "object/Self.h"  // includes everything
```

`Self.h` pulls in `ObjectTypes.h`, `ObjectContainer.h`, `ObjectRef.h`, and
`ObjectHelpers.h`.

---

## Object Lifecycle

Objects follow a strict state machine. Each transition has a dedicated function
with guards that prevent invalid states.

```
Ghost -> Typed -> Filled -> (use) -> Empty -> Untyped -> Ghost -> Destroyed
```

### States

- **Ghost** -- Allocated `ObjectContainer` via `malloc`. No class, no data.
  `cid` is `CID_Untyped`, `data` is `NULL`. Registered in the global object
  registry.

- **Typed** -- A `ClassID` has been assigned via `ObjectContainer_TypeEmptyUntyped`.
  Still no data (`data` is `NULL`).

- **Filled** -- Data is allocated (values hashmap + references hashmap) and the
  `SELF_Create` message has been dispatched. The object is now live and can
  receive messages, hold values, and reference other objects.

- **Empty** -- `SELF_Destroy` has been dispatched and data has been freed.
  All held internal references are released. `data` is `NULL` again.

- **Untyped** -- `ClassID` reset to `CID_Untyped` via
  `ObjectContainer_UntypeEmptyTyped`.

- **Destroyed** -- Container freed and unregistered from the global registry.

### Transition Functions (Low-Level)

| Function                              | Transition         |
|---------------------------------------|--------------------|
| `ObjectContainer_CreateGhost()`       | (none) -> Ghost    |
| `ObjectContainer_TypeEmptyUntyped()`  | Ghost -> Typed     |
| `ObjectContainer_FillEmptyTyped()`    | Typed -> Filled    |
| `ObjectContainer_EmptyFilledTyped()`  | Filled -> Empty    |
| `ObjectContainer_UntypeEmptyTyped()`  | Empty -> Untyped   |
| `ObjectContainer_DestroyGhost()`      | Untyped -> Destroyed |

Each function validates preconditions and logs an error if the object is in the
wrong state.

---

## Creating Objects (High-Level API)

The high-level helpers run the full Ghost -> Typed -> Filled sequence in one
call:

```c
// Create and get a temporary (unowned) reference.
// No refcount is incremented -- the caller borrows the pointer.
TempObjectReference obj = Object_Create(CID_MyClass);

// Create and get an owned external reference.
// Increments external_refs by 1.
ExternalReference obj = Object_CreateRef(CID_MyClass);
```

To manually destroy an unreferenced object:

```c
Object_Destroy(obj);  // runs Empty -> Untype -> DestroyGhost
```

`Object_Destroy` refuses to proceed if the object still has references
(`internal_refs` or `external_refs` != 0).

---

## Reference Types

All three types are `ObjectContainer*`. The typedef name signals ownership:

| Type                   | Refcount Effect         | Usage                                  |
|------------------------|-------------------------|----------------------------------------|
| `TempObjectReference`  | None                    | Borrowed pointer for transient access. Caller must ensure the target outlives the borrow. |
| `ObjectReference`      | Increments `internal_refs` | Stored in another object's reference hashmap. Tracked by the GC for cycle detection. |
| `ExternalReference`    | Increments `external_refs` | Held on the stack or in global variables. Objects with `external_refs > 0` are always reachable and never collected. |

### Total Refs

```c
#define ObjectContainer_TotalRefs(c) ((c)->internal_refs + (c)->external_refs)
```

When total refs reach 0, the object is automatically emptied, untyped, and freed.

---

## Creating References

Convert between reference types. Each conversion that produces an
`ObjectReference` or `ExternalReference` increments the corresponding refcount.
`TempObjectReference` conversions never change refcounts.

```c
// Temp -> External (for keeping on the stack)
ExternalReference ext = ObjectContainer_ExternalRef_From_Temp(temp);

// External -> Internal (for storing in another object's refs)
ObjectReference iref = ObjectContainer_InternalRef_From_External(ext);

// Temp -> Internal
ObjectReference iref = ObjectContainer_InternalRef_From_Temp(temp);

// Internal -> External
ExternalReference ext = ObjectContainer_ExternalRef_From_Internal(iref);

// External -> External (duplicate an external ref)
ExternalReference ext2 = ObjectContainer_ExternalRef_From_External(ext);

// Internal -> Internal (duplicate an internal ref)
ObjectReference iref2 = ObjectContainer_InternalRef_From_Internal(iref);

// Any -> Temp (no refcount change, just cast)
TempObjectReference temp = ObjectContainer_TempFrom(ref);
```

---

## Releasing References

```c
// Decrement external_refs and NULL the pointer variable.
ObjectContainer_UnRef_External(&ext);

// Decrement internal_refs and NULL the pointer variable.
ObjectContainer_UnRef_Internal(&iref);
```

Both functions take a pointer-to-pointer so they can NULL the caller's variable,
preventing use-after-free.

When total refs hit 0, the object is automatically cleaned up:

1. `ObjectContainer_EmptyFilledTyped` -- dispatches `SELF_Destroy`, frees data
2. `ObjectContainer_UntypeEmptyTyped` -- resets ClassID
3. Unregistered from the global registry and freed

---

## Cycle Collection

Reference cycles (A -> B -> A) would keep `internal_refs` above 0 forever.
The cycle collector handles this:

### When It Triggers

- **Automatically** when `ObjectContainer_UnRef_External` drops `external_refs`
  to 0 but `internal_refs` is still > 0. This is the only state where a cycle
  could be keeping objects alive.

- **Manually** via `Object_GarbageCollect()`, which sweeps all registered objects
  and runs cycle collection on any object with `external_refs == 0` and
  `internal_refs > 0`.

### Algorithm

1. BFS from the candidate object, following internal references in each object's
   reference hashmap.
2. If any node in the reachable component has `external_refs > 0`, the entire
   component is reachable -- collection is aborted.
3. If the entire component has 0 external refs, the whole cycle is collected:
   - **Pass 1**: Dispatch `SELF_Destroy` to each object for proper cleanup.
   - **Pass 2**: Tear down data. For references pointing outside the component,
     decrement the target's `internal_refs`. If those external neighbors become
     orphaned (0 external refs, only internal refs), they are added to a
     follow-up candidate list.
   - **Pass 3**: Unregister and free all containers in the component.
   - **Pass 4**: Recursively run cycle collection on orphaned neighbors
     discovered in Pass 2.

### Re-Entrancy Guard

The `_gc_running` flag prevents infinite recursion. `SELF_Destroy` handlers may
drop references that would normally trigger further GC -- the flag causes those
nested GC attempts to bail out immediately.

### Global Sweep

```c
Object_GarbageCollect();
```

Snapshots all candidates (objects with `external_refs == 0`, `internal_refs > 0`,
and live data), then runs `_ObjectContainer_TryCollectCycle` on each. Re-checks
liveness before each attempt because previous iterations may have already freed
the candidate.

---

## SELF Handlers

SELF handlers are special message handlers that receive a `Self` pointer
(`TempObjectReference`) to the object being operated on.

### Defining Handlers

```c
#define TYPE MyClass

DECLARE_SELF_MID(Create);
DECLARE_SELF_MID(Destroy);

SELF_MESSAGE_HANDLER_BEGIN(Create)
    Self_SetValue("health", int, 100);
    Self_SetValue("name", char[32], "unnamed");
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN(Destroy)
    LOG_INFO("MyClass destroyed");
MESSAGE_HANDLER_END()
```

### Registering Handlers

```c
CAN_RECEIVE_BEGIN()
    SELF_CAN_RECEIVE_MID(Create)
    SELF_CAN_RECEIVE_MID(Destroy)
    CAN_RECEIVE_MID(OtherMessage)
CAN_RECEIVE_END()

RECEIVE_MESSAGE_BEGIN()
    SELF_RECEIVE_MESSAGE_ROUTE(Create)
    SELF_RECEIVE_MESSAGE_ROUTE(Destroy)
    RECEIVE_MESSAGE_ROUTE(OtherMessage)
RECEIVE_MESSAGE_END()
```

### Default SELF MIDs

The default SELF message IDs (`SELF_Create` and `SELF_Destroy`) are declared in
`ObjectTypes.h` under `#define TYPE Default`. The actual MIDs are:

- `MID_Default_SELF_Create`
- `MID_Default_SELF_Destroy`

These are the MIDs dispatched by `ObjectContainer_FillEmptyTyped` and
`ObjectContainer_EmptyFilledTyped`.

### Extern Variants

For defining SELF handlers outside the class's `#define TYPE` scope:

```c
SELF_MESSAGE_HANDLER_BEGIN_EXTERN(MyClass, Create)
    // ...
MESSAGE_HANDLER_END()

SELF_CAN_RECEIVE_MID_EXTERN(MyClass, Create)
SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(MyClass, Create)
```

---

## Self_ Value Macros (Inside SELF Handlers)

These macros operate on the object's values hashmap via the `Self` pointer:

```c
Self_SetValue("key", type, value)   // store a typed value
Self_Set("key", ptr, size)          // store raw bytes
Self_Get("key", type)               // returns type* (pointer to stored value)
Self_GetDeref("key", type)          // returns type (dereferences the pointer)
Self_Has("key")                     // returns int (1 if key exists)
```

`Self_Values` expands to `Self->data->values` for direct hashmap access.

---

## Self_ Reference Macros (Inside SELF Handlers)

These macros operate on the object's references hashmap:

```c
Self_RefFrom("key", ext_ref)       // store an internal ref from ExternalReference
Self_RefFromTemp("key", temp_ref)  // store an internal ref from TempObjectReference
Self_GetRef("key")                 // returns TempObjectReference (borrowed)
Self_HasRef("key")                 // returns int (1 if key exists)
Self_IsRefSelf("key")              // returns int (1 if the ref points to Self)
```

`Self_Refs` expands to `Self->data->references` for direct hashmap access.

Both `Self_RefFrom` and `Self_RefFromTemp` create an `ObjectReference` (incrementing
`internal_refs` on the target) and store it in the references hashmap.

---

## Direct Reference Helpers (Outside SELF Handlers)

For manipulating object references when you have a `TempObjectReference` but are
not inside a SELF handler:

```c
// Store an internal reference from obj to target under the given key.
Object_StoreRef(obj, "key", key_len, target);

// String-literal shorthand (computes key_len at compile time).
Object_SStoreRef(obj, "key", target);

// Retrieve a reference as TempObjectReference.
TempObjectReference ref = Object_GetRef(obj, "key", key_len);

// String-literal shorthand.
TempObjectReference ref = Object_SGetRef(obj, "key");
```

---

## Dispatching Messages to Objects

### Self Dispatch Helpers

Two shorthand forms handle prepare/dispatch/free automatically, with `Self`
pre-set in the payload:

**`SelfDispatch(ref, mid)`** -- fire-and-forget, no params, no out values.
Returns `uint8_t` result code.

```c
SelfDispatch(obj, MID_Default_SELF_Create);
```

**`SELF_DISPATCH(ref, mid, { params }, { outs })`** -- with params and/or out
values. `msg` is a `MessagePayload*` available inside both blocks. Payload is
freed automatically after the out block. Returns `uint8_t` result code.

```c
// Params only
SELF_DISPATCH(obj, MID_Counter_Increment, {
    Payload_SetValue(msg, "Amount", int, 5);
}, {});

// Out values only
int count;
SELF_DISPATCH(obj, MID_Counter_GetCount, {}, {
    count = Payload_GetDeref(msg, "Count", int);
});

// Both params and out values
int result;
SELF_DISPATCH(obj, MID_Counter_AddAndGet, {
    Payload_SetValue(msg, "Amount", int, 10);
}, {
    result = Payload_GetDeref(msg, "Count", int);
});

// Check result
uint8_t r = SELF_DISPATCH(obj, MID_Counter_Reset, {}, {});
if (!MESSAGE_RESULT_ISOK(r)) { /* handle error */ }
```

### Manual Self Dispatch

For cases where the helpers don't fit:

```c
MessagePayload msg = PrepareSelfPayload(obj, MID_Counter_Increment);
Payload_SetValue(&msg, "Amount", int, 5);
DispatchMessage(&msg);
FreePayload(&msg);
```

`PrepareSelfPayload` works like `PreparePayload` but automatically sets
`cid_target` from the object's ClassID and stores the `Self` pointer in the
payload.

---

## Singleton Pattern

For classes that should have exactly one instance (e.g., Window, Audio):

```c
// After CLASSDEF() and #undef TYPE:
DECLARE_SINGLETON(Window)
```

This expands to:
- `Window_CreateSingleton()` -- creates the one instance via Object_CreateRef, errors if already exists
- `Window_DestroySingleton()` -- unrefs and NULLs the static reference
- `Window_HasSingleton()` -- returns 1 if the singleton exists, 0 otherwise
- `GET_SINGLETON(Window)` -- returns a TempObjectReference (borrowed) to the singleton

Usage:
```c
Window_CreateSingleton();
TempObjectReference win = GET_SINGLETON(Window);

SELF_DISPATCH(win, MID_Window_SELF_Open, {
    Payload_SetValue(msg, "Width", int, 640);
    Payload_SetValue(msg, "Height", int, 480);
}, {});

// ... game loop ...

Window_DestroySingleton();
```

The singleton is just a static ExternalReference. DECLARE_SINGLETON must appear
after CLASSDEF() and #undef TYPE since it references CID_ClassName.

---

## Visualization

Dump the object reference graph to a text file for debugging:

```c
// Visualize a single object and its reachable graph.
Object_VisualizeGraphSingle("graph.txt", obj);

// Visualize from multiple root objects.
TempObjectReference roots[] = { obj1, obj2, obj3 };
Object_VisualizeGraph("graph.txt", roots, 3);
```

The output is an ASCII graph showing each object's class, state, refcounts,
reference edges (with SELF and BACK-REF annotations), and a summary of total
nodes and references.

---

## Contract

- Objects **must** support `SELF_Create` and `SELF_Destroy` to be filled.
  `ObjectContainer_FillEmptyTyped` checks `CanDispatchMessage` for
  `MID_Default_SELF_Create` and refuses to proceed if the class cannot handle it.

- `SELF_Create` is dispatched during `ObjectContainer_FillEmptyTyped` after
  allocating the values and references hashmaps.

- `SELF_Destroy` is dispatched during `ObjectContainer_EmptyFilledTyped` before
  tearing down data.

- Data is detached (`container->data = NULL`) **before** unreffing held
  references. This prevents re-entrant destruction when an object holds a
  reference to itself or participates in a mutual reference.

- The global object registry (`_object_registry`) tracks all live containers.
  It is used by `Object_GarbageCollect` to find candidates and by the cycle
  collector to verify that targets are still alive.

- Cycle collection is guarded against re-entrancy by the `_gc_running` flag.
  Nested GC attempts (triggered by `SELF_Destroy` dropping references) are
  silently skipped.

---

## Serialization

### Include

```c
#include "system/object/Serialization.h"
```

### Overview

The serialization system persists entire object graphs -- values, references,
and topology -- to a ByteStream or .cob file. It handles circular references,
multiple roots, and arbitrary value types through a registry of
serialize/deserialize function pairs.

### Serialization Function Registry

Every value stored in an object carries a `ser_id` that identifies which
serialize/deserialize function pair to use. Built-in IDs:

| ID | Name | Behavior | ser_arg usage |
|---|---|---|---|
| 0 | `SER_RAW` | Copy bytes as-is (default) | unused |
| 1 | `SER_SKIP` | Transient -- not serialized | unused |
| 2 | `SER_DEREF` | Dereference pointer, copy pointed-to data | pointed-to size in bytes |
| 3 | `SER_STRING` | Follow char*, serialize string + null terminator | unused |

User IDs 16-255 are available for custom handlers.

### Registering Custom Handlers

```c
void *my_encrypt(const void *data, uint32_t size, uint16_t arg, uint32_t *out_size) {
    // transform data for storage
}
void *my_decrypt(const void *data, uint32_t size, uint16_t arg, uint32_t *out_size) {
    // reconstruct from stored data
}

#define SER_ENCRYPTED 16
Ser_Register(SER_ENCRYPTED, my_encrypt, my_decrypt, "Encrypted");
```

Works like class registration -- each ID can only be registered once. IDs below
SER_USER_START (16) are reserved.

### Storing Values with Serialization Metadata

```c
Self_SetValue("health", int, 100);                          // SER_RAW (default)
Self_SetTransient("frame_cache", int, 0);                   // SER_SKIP
Self_SetDeref("name", char*, name_ptr, 64);                 // SER_DEREF, arg=64
Self_SetCustom("secret", MyData, data, SER_ENCRYPTED, 0);   // custom handler
```

### Serializing Object Graphs

```c
// Serialize all objects reachable from roots
ByteStream *stream = Object_Serialize(roots, root_count);

// Deserialize -- returns array of ExternalReferences (caller frees array)
int loaded_count;
ExternalReference *loaded = Object_Deserialize(stream, &loaded_count);

// File convenience wrappers (.cob files)
Object_SaveToFile("save.cob", roots, root_count);
ExternalReference *loaded = Object_LoadFromFile("save.cob", &loaded_count);
```

### How Serialization Works

**Serialize:**
1. BFS from roots to discover all reachable objects
2. Assign each a uint32_t serialization ID
3. For each value: call the registered serialize function (based on ser_id),
   write the transformed bytes
4. For each reference: replace the pointer with the target's serialization ID
5. SER_SKIP values are omitted entirely

**Deserialize (three phases):**
1. Create all objects up-front (Ghost -> Type -> Fill via SELF_Create)
2. Restore values -- deserialize function reconstructs bytes, overwrites
   SELF_Create defaults
3. Restore references -- resolve serialization IDs back to pointers using the
   ID map

Circular references work because all objects exist before any references are
wired.

### .cob File Format

```
ECOB magic (4 bytes)
version (uint32_t)
object_count (uint32_t)
root_count (uint32_t)

[Root table]
  per root: serialization_id, external_refs

[Object table]
  per object:
    serialization_id, class_id
    value_count
    per value: key_len, key, owner_cid, ser_id, ser_arg, data_size, data
    ref_count
    per ref: key_len, key, target_serialization_id
```

### Security

All values read from .cob files are validated before use:
- object_count and root_count capped at SER_MAX_OBJECTS (1M)
- value_count and ref_count capped at SER_MAX_VALUES_PER_OBJECT (64K)
- key_len capped at SER_MAX_KEY_LEN (256)
- data_size capped at SER_MAX_DATA_SIZE (1MB)
- ser_id bounds-checked against object_count
- target_id bounds-checked against object_count
- Stream truncation detected via ByteStream_Remaining checks
- All limits are configurable via #define before including the header

See `system/HARD.md` for the full list of configurable limits.

### SELF_Serialize / SELF_Deserialize Hooks

Two optional SELF messages for classes that need custom serialization logic:

- `MID_Default_SELF_Serialize` -- dispatched before reading values. Convert
  ephemeral state into serializable form.
- `MID_Default_SELF_Deserialize` -- dispatched after values and refs are
  restored. Reconstruct derived state.

Classes that only store plain values (ints, floats, structs via SER_RAW) do not
need these hooks.

---

## ObjectContainer Struct

```c
typedef struct ObjectContainer {
    ObjectDataPTR data;      // NULL when ghost/empty, allocated when filled
    ClassID cid;             // CID_Untyped when ghost/untyped
    int internal_refs;       // refs from other objects' reference hashmaps
    int external_refs;       // refs from stack/globals
} ObjectContainer;

typedef struct ObjectData {
    UnsafeVariedHashMap* values;     // primitive values, no GC participation
    UnsafeHashMap* references;       // ObjectReference pointers, GC-tracked
} ObjectData;
```
