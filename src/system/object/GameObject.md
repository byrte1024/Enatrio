# GameObject

## Overview

GameObject is a scene graph node. GameObjects form a tree where each node can
have a parent and any number of children. Messages dispatched to the root
propagate through the entire tree via the SpreadMessage system.

**CID:** `0x0003`
**Inherits:** Object
**File:** `src/system/object/GameObject.h`

## Include

```c
#include "system/object/Self.h"
```

Self.h is the aggregate header that includes GameObject.h along with the full
object system.

## Registration

GameObject must be registered after Object and before any class that inherits
from it:

```c
BeginClassRegistrations();
RegisterClass(Object_ClassDef());
RegisterClass(GameObject_ClassDef());
RegisterClass(MyGameClass_ClassDef());  // inherits GameObject
EndClassRegistrations();
```

## Scene Graph

### Children

Children are stored as internal object references with keys `child_0`,
`child_1`, etc. They participate in the refcounting/GC system automatically.
When a parent is destroyed, its internal refs to children are released. If no
other refs exist, the children are collected.

Children are always stored in **priority-sorted order** (lower priority runs
first). Sorting happens at add/remove/priority-change time, never during
iteration.

### Parent Back-Reference

Each child holds an internal ref to its parent under the key `"parent"`. Root
nodes have no parent ref. The parent ref is set by AddChild and cleared by
RemoveChild.

### Active Flag

Each GameObject has an `active` flag (default 1). Inactive nodes and their
entire subtrees are skipped during SpreadMessage propagation. Toggle with
SELF_SetActive:

```c
SELF_DISPATCH(node, MID_GameObject_SELF_SetActive, {
    Payload_SetValue(msg, "active", int, 0);  // deactivate
}, {});
```

## Self Values

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `active` | int | 1 | Whether this node participates in spread |
| `child_count` | int | 0 | Current number of children |
| `priority` | int | 0 | Sort priority among siblings (lower first) |

## MIDs

| MID | Direction | Description |
|-----|-----------|-------------|
| `SELF_SpreadMessage` | -- | Internal propagation handler (do not call directly) |
| `SELF_AddChild` | -- | Add a child to this node |
| `SELF_RemoveChild` | -- | Remove a child from this node |
| `SELF_SetActive` | -- | Set the active flag |
| `SELF_SetPriority` | -- | Change priority, re-sorts in parent |
| `SELF_Update` | Spread | Called each frame for logic. Override in subclasses. |
| `SELF_Render` | Spread | Called each frame for drawing. Override in subclasses. |

SELF_Update and SELF_Render are declared on GameObject with no-op default
handlers. Subclasses override them to add behavior. They are intended to be
dispatched via GAMEOBJECT_DISPATCH, not SELF_DISPATCH.

## SpreadMessage

SpreadMessage is the core propagation system. It takes an "inner" payload
(the actual message, e.g. Update with delta_time) and walks the tree, swapping
Self on the inner payload for each node before dispatching it.

### Direction

Direction is a flag on the inner payload, set by GAMEOBJECT_DISPATCH:

| Constant | Value | Description |
|----------|-------|-------------|
| `SPREAD_DOWN` | 0 | Top-down: self first, then children (depth-first) |
| `SPREAD_UP` | 1 | Bottom-up: children first (to leaves), then self |

### Reverse

The `spread_reverse` flag (int, default 0) reverses child iteration order.
When set to 1, children iterate from highest index to lowest. Combined with
priority sorting, this runs higher-priority children last.

### Shared Payload

The inner payload is created ONCE and shared across the entire tree traversal.
Every node's handler sees the same data map. An early node can modify a value
(e.g. delta_time), and later nodes see the modified value. Child ordering
(priority) determines who runs first and thus who writes first.

### Consumption (SPREAD_DOWN only)

A handler can call `SPREAD_CONSUME(payload)` to stop propagation into its own
subtree. The node's children are skipped, but siblings continue normally.

```c
SELF_MESSAGE_HANDLER_BEGIN_EXTERN(GameObject, Update)
    IGNORE_BASE();
    // Handle input, then consume so children don't also process it
    SPREAD_CONSUME(payload);
MESSAGE_HANDLER_END()
```

**SPREAD_CONSUME only works during SPREAD_DOWN.** In SPREAD_UP mode, children
are visited before self, so there is nothing left to skip when the handler
runs. The consumed flag is cleaned up but has no observable effect.

The consumed flag uses a reserved internal key (`__go_consumed__`). Do not set
this key directly -- use the SPREAD_CONSUME macro. The linter (R110) flags
direct use of the key.

### Snapshot Iteration

Child iteration uses a snapshot of the child list taken before iteration
begins. This means:

- **Adding children during spread is safe.** The new child will NOT receive the
  current message (it was not in the snapshot). It will exist in the child list
  after the spread completes and will receive subsequent dispatches.

- **Removing children during spread is safe.** A removed child may still be in
  the snapshot, but it is skipped if its data is NULL (already freed).

- **No crash or stale-index bugs.** The cached child_count and loop index
  operate on the snapshot, not the live list.

The snapshot uses a stack-allocated array for up to 64 children
(`_GO_SPREAD_STACK_MAX`). For larger child counts, a heap allocation is used
and freed after iteration.

## GAMEOBJECT_DISPATCH

The primary way to send a message through the scene tree.

```c
GAMEOBJECT_DISPATCH(root, mid, direction, { params }, { output });
```

| Parameter | Description |
|-----------|-------------|
| `root` | TempObjectReference to the root of the subtree |
| `mid` | The MessageID to dispatch (e.g. `MID_GameObject_SELF_Update`) |
| `direction` | `SPREAD_DOWN` or `SPREAD_UP` |
| `params` | Block that sets values on the inner payload (`msg` is a `MessagePayload*`) |
| `output` | Block that reads values from the inner payload after the spread |

Returns `uint8_t` result code.

### Examples

```c
// Update all nodes in the tree
float dt = GetFrameTime();
GAMEOBJECT_DISPATCH(root, MID_GameObject_SELF_Update, SPREAD_DOWN, {
    Payload_SetValue(msg, "dt", float, dt);
}, {});

// Render all nodes
GAMEOBJECT_DISPATCH(root, MID_GameObject_SELF_Render, SPREAD_DOWN, {}, {});

// Reverse iteration
GAMEOBJECT_DISPATCH(root, MID_GameObject_SELF_Render, SPREAD_DOWN, {
    Payload_SetValue(msg, "spread_reverse", int, 1);
}, {});

// Bottom-up propagation
GAMEOBJECT_DISPATCH(root, MID_GameObject_SELF_Update, SPREAD_UP, {
    Payload_SetValue(msg, "dt", float, dt);
}, {});
```

### How It Works

1. Creates the "inner" payload -- the actual message with Self set to root
2. Sets `spread_direction` on the inner payload
3. Runs the user's params_block to set additional values
4. Creates an "outer" SpreadMessage payload wrapping the inner payload
5. Dispatches SpreadMessage to root
6. SpreadMessage recursively walks the tree, swapping Self for each node
7. Runs the user's output_block to read results
8. Frees both payloads

The `root` argument is evaluated exactly once (stored in a local variable).

## Child Management

### AddChild

```c
SELF_DISPATCH(parent, MID_GameObject_SELF_AddChild, {
    Payload_SetValue(msg, "child", TempObjectReference, child);
}, {});
```

1. Reads the child's priority value
2. Finds the correct insertion index by scanning existing children
3. Shifts higher-index children up to make room
4. Stores the child at the insertion index
5. Sets the child's parent back-reference
6. Increments child_count

### RemoveChild

```c
SELF_DISPATCH(parent, MID_GameObject_SELF_RemoveChild, {
    Payload_SetValue(msg, "child", TempObjectReference, child);
}, {});
```

1. Finds the child's index by scanning child_0..child_{count-1}
2. Returns MESSAGE_RESULT_NOT_FOUND if not found
3. Clears the child's parent back-reference
4. Removes the child's internal ref
5. Shifts remaining children down to fill the gap
6. Removes the duplicate last slot
7. Decrements child_count

### SetPriority

```c
SELF_DISPATCH(node, MID_GameObject_SELF_SetPriority, {
    Payload_SetValue(msg, "priority", int, 10);
}, {});
```

Updates the node's priority value. If the node has a parent, removes itself and
re-adds to trigger a re-sort.

**Safety:** SetPriority acquires a temporary external ref on Self before the
remove+re-add cycle to prevent use-after-free. The ref is released after
re-insertion.

## Helper Functions

| Function | Returns | Description |
|----------|---------|-------------|
| `GameObject_CreateRoot(cid)` | TempObjectReference | Creates a root node (no parent) |
| `GameObject_CreateRootRef(cid)` | ExternalReference | Same, with an external ref |
| `GameObject_CreateChild(parent, cid)` | TempObjectReference | Creates a child and adds it to parent |
| `GameObject_CreateChildRef(parent, cid)` | ExternalReference | Same, with an external ref |

`GameObject_CreateChild` creates the object via `Object_Create`, then
dispatches SELF_AddChild to the parent. The child is held alive by the parent's
internal ref. No external ref is created unless you use `CreateChildRef`.

## Defining a Subclass

A class that inherits from GameObject needs to:

1. Override SELF_Create (extern Object) with CALL_BASE to initialize base values
2. Override SELF_Destroy (extern Object) with CALL_BASE for cleanup
3. Override SELF_Update and/or SELF_Render (extern GameObject) for behavior
4. List only the MIDs it actually handles in CAN_RECEIVE/RECEIVE_MESSAGE
   (inherited MIDs like SpreadMessage, AddChild, etc. are resolved by the
   inheritance chain walk -- do NOT re-route them)

### Complete Example

```c
#pragma once

#include "../system/object/Self.h"

#define TYPE BouncingBox

BEGIN_CLASS(0x2200);
INHERITS(GameObject);

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Create)
    CALL_BASE();
    Self_SetTransient("x", float, 20.0f);
    Self_SetTransient("y", float, 40.0f);
    Self_SetTransient("dx", float, 30.0f);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Destroy)
    CALL_BASE();
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(GameObject, Update)
    IGNORE_BASE();
    float dt = 0.0f;
    if (MH_Has(dt)) dt = MH_GetDeref(dt, float);

    float x = Self_GetDeref("x", float);
    float dx = Self_GetDeref("dx", float);
    x += dx * dt;
    Self_SetTransient("x", float, x);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(GameObject, Render)
    IGNORE_BASE();
    float x = Self_GetDeref("x", float);
    float y = Self_GetDeref("y", float);
    DrawRectangle((int)x, (int)y, 12, 12, RED);
MESSAGE_HANDLER_END()

CAN_RECEIVE_BEGIN()
    SELF_CAN_RECEIVE_MID_EXTERN(Object, Create)
    SELF_CAN_RECEIVE_MID_EXTERN(Object, Destroy)
    SELF_CAN_RECEIVE_MID_EXTERN(GameObject, Update)
    SELF_CAN_RECEIVE_MID_EXTERN(GameObject, Render)
CAN_RECEIVE_END()

RECEIVE_MESSAGE_BEGIN()
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Object, Create)
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Object, Destroy)
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(GameObject, Update)
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(GameObject, Render)
RECEIVE_MESSAGE_END()

CLASSDEF_INHERITS(GameObject)

#undef TYPE
```

Note: The subclass does NOT list SpreadMessage, AddChild, RemoveChild, etc.
in its CAN_RECEIVE/RECEIVE_MESSAGE blocks. Those are handled by GameObject
via the inheritance chain walk. Only MIDs that the subclass actually overrides
or introduces need to be listed.

## Performance

Each payload parameter costs ~12-30ns per node in the tree. For a tree with
100 nodes, a 10-parameter Update message costs ~30us total. Pack related
parameters into structs to reduce this:

```c
// SLOW: 3 separate keys = 3 hash lookups per node
GAMEOBJECT_DISPATCH(root, MID_GameObject_SELF_Update, SPREAD_DOWN, {
    Payload_SetValue(msg, "dt", float, dt);
    Payload_SetValue(msg, "frame", int, frame);
    Payload_SetValue(msg, "paused", int, 0);
}, {});

// FAST: 1 struct key = 1 hash lookup per node
typedef struct { float dt; int frame; int paused; } FrameInfo;
GAMEOBJECT_DISPATCH(root, MID_GameObject_SELF_Update, SPREAD_DOWN, {
    Payload_SetValue(msg, "frame", FrameInfo, ((FrameInfo){dt, frame, 0}));
}, {});
```

Per-node cost on CI (ubuntu-latest):
- SpreadMessage overhead: ~40 ns/node
- Per payload parameter: ~12-30 ns/node
- Struct-packed single param: ~12 ns/node regardless of struct size

## Contract

- **Priority 0 is the default.** Children with equal priority are ordered by
  insertion order within the same priority level.

- **Children are internal refs.** They participate in the GC. A tree with no
  external refs is eligible for cycle collection.

- **Parent back-ref creates a cycle.** Parent holds internal ref to child,
  child holds internal ref to parent. The GC cycle collector handles this.
  Dropping the last external ref on the root collects the entire tree.

- **Do not call SpreadMessage directly.** Use GAMEOBJECT_DISPATCH. SpreadMessage
  expects a specific outer payload structure with an inner payload pointer.

- **SPREAD_CONSUME is SPREAD_DOWN only.** It has no effect in SPREAD_UP mode.
  This is by design -- in bottom-up traversal, children run before the handler,
  so there is nothing to skip.

- **Snapshot iteration.** Child mutations during spread are safe but newly added
  children do not receive the current message. Removed children are skipped if
  already freed.

- **SetPriority is safe without external refs.** The handler acquires a guard
  ref internally before the remove+re-add cycle.

- **Reserved key `__go_consumed__`.** Do not use this key name in payloads.
  It is reserved for the spread consumption system. The linter (R110) enforces
  use of the SPREAD_CONSUME macro.
