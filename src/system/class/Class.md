# Class System

## Overview

The class system provides runtime class registration and message-based dispatch.
Classes are registered at startup, then messages can be dispatched to them.

Each class has:
- A unique **ClassID** (`uint16_t`) -- identifies the class at runtime
- A **class name** (`char[64]`) -- human-readable identifier, checked for duplicates
- A **CanReceiveMID** function -- returns true if the class handles a given message
- A **ReceiveMessage** function -- routes incoming messages to the correct handler

Messages carry:
- A **MessageID** (`char[64]`) -- string in the format `"ClassName.MessageName"`
- A **target ClassID** -- which class should receive the message
- A **payload** -- a hash map of named parameters (arbitrary types, stored as raw bytes)
- A **result** code (`uint8_t`) -- set by the handler to indicate success or failure

## Include

```c
#include "class/Class.h"
```

## Defining a Class

A class is defined entirely in a single header file using a series of macros.
The `TYPE` preprocessor symbol must be defined to the class name before using
any class macros, and undefined at the end.

### Step by step

1. **Set the TYPE macro** -- all subsequent macros use this as a namespace prefix.

   ```c
   #define TYPE Exploder
   ```

2. **BEGIN_CLASS(id)** -- declares the ClassID constant and class name string.

   ```c
   BEGIN_CLASS(0x22AB);
   ```

   Expands to:
   ```c
   inline const ClassID CID_Exploder = (ClassID)(0x22AB);
   inline const char CLASSNAME_Exploder[CLASS_MAXNAMELENGTH] = "Exploder";
   ```

3. **DECLARE_MID(MessageName)** -- declares a MessageID string constant.

   ```c
   DECLARE_MID(ShimmiShimmiYea);
   ```

   Expands to:
   ```c
   static MessageID MID_Exploder_ShimmiShimmiYea = "Exploder.ShimmiShimmiYea";
   ```

4. **MESSAGE_HANDLER_BEGIN(Name) / MESSAGE_HANDLER_END()** -- defines a handler
   function. The handler receives a `MessagePayload* payload` parameter.
   `MESSAGE_HANDLER_BEGIN` automatically sets `payload->result` to
   `MESSAGE_RESULT_SUCCESS`.

   ```c
   MESSAGE_HANDLER_BEGIN(ShimmiShimmiYea)
       MH_ExtractDeref(Strength, float);
       LOG_INFO("Strength: %f", Strength);
   MESSAGE_HANDLER_END()
   ```

5. **CAN_RECEIVE block** -- declares which messages the class accepts.

   ```c
   CAN_RECEIVE_BEGIN()
       CAN_RECEIVE_MID(ShimmiShimmiYea)
   CAN_RECEIVE_END()
   ```

6. **RECEIVE_MESSAGE block** -- routes incoming messages to handler functions.

   ```c
   RECEIVE_MESSAGE_BEGIN()
       RECEIVE_MESSAGE_ROUTE(ShimmiShimmiYea)
   RECEIVE_MESSAGE_END()
   ```

   If no route matches, the result is set to `MESSAGE_RESULT_NOT_SUPPORTED`.

7. **CLASSDEF()** -- creates the `ClassDefinition` builder function.

   ```c
   CLASSDEF()
   ```

   Expands to a function `Exploder_ClassDef(void)` that returns a populated
   `ClassDefinition` struct.

8. **Undefine TYPE** -- prevents macro pollution for the next header.

   ```c
   #undef TYPE
   ```

### Complete Example

This is the Exploder class from `src/classes/exploder.h`:

```c
#pragma once

#include "../class/Class.h"

#define TYPE Exploder

BEGIN_CLASS(0x22AB);

DECLARE_MID(ShimmiShimmiYea);

MESSAGE_HANDLER_BEGIN(ShimmiShimmiYea)

    MH_ExtractDeref(Strength, float);

    if(Strength > 7){
        LOG_INFO("Woah, calm down big boy");
    }

    if(Strength < 1){
        LOG_INFO("Thats your best try?");
    }

    LOG_INFO("Strength: %f", Strength);

MESSAGE_HANDLER_END()

CAN_RECEIVE_BEGIN()
    CAN_RECEIVE_MID(ShimmiShimmiYea)
CAN_RECEIVE_END()

RECEIVE_MESSAGE_BEGIN()
    RECEIVE_MESSAGE_ROUTE(ShimmiShimmiYea)
RECEIVE_MESSAGE_END()

CLASSDEF()

#undef TYPE
```

## Registration

Classes must be registered between `BeginClassRegistrations()` and
`EndClassRegistrations()`. This typically happens once at startup.

```c
BeginClassRegistrations();
RegisterClass(Exploder_ClassDef());
EndClassRegistrations();
```

`BeginClassRegistrations()` clears the registration table and opens it for
writes. `EndClassRegistrations()` locks it -- no further registrations are
accepted after this point.

## Sending Messages

```c
// 1. Prepare the payload (allocates the data map)
MessagePayload msg = PreparePayload(CID_Exploder, MID_Exploder_ShimmiShimmiYea);

// 2. Set parameters
Payload_SetValue(&msg, "Strength", float, 5.0f);

// 3. Dispatch
DispatchMessage(&msg);

// 4. Check result
if (MESSAGE_RESULT_ISOK(msg.result)) {
    // success
}

// 5. Free the payload data map
FreePayload(&msg);
```

`PreparePayload` allocates a `UnsafeVariedHashMap` for the payload data and
sets the initial result to `MESSAGE_RESULT_NOTSENT`. `FreePayload` destroys
only the heap-allocated map -- the `MessagePayload` struct itself lives on the
stack.

## Payload API

These macros operate on a `MessagePayload*` pointer.

| Macro | Description |
|-------|-------------|
| `Payload_Set(payload, key, value_ptr, value_size)` | Stores raw bytes into the payload. |
| `Payload_SetValue(payload, key, type, value)` | Stores a typed value (takes the address via compound literal). |
| `Payload_SetLocalValue(payload, key, type, value)` | Creates a stack-local variable and stores a pointer to it. The variable lives until the enclosing scope ends. |
| `Payload_Get(payload, key)` | Returns a `void*` to the stored data, or `NULL` if not found. |
| `Payload_GetDeref(payload, key, type)` | Dereferences the stored data as the given type. |
| `Payload_Has(payload, key)` | Returns 1 if the key exists, 0 otherwise. |
| `Payload_GetSize(payload, key)` | Returns the byte size of the stored value, or 0 if not found. |
| `Payload_Remove(payload, key)` | Removes a key from the payload. Returns 0 on success, -1 if not found. |

### Usage examples

```c
// Store a struct by raw bytes
Payload_Set(&msg, "data", &my_struct, sizeof(MyStruct));

// Store a typed value
Payload_SetValue(&msg, "health", int, 100);

// Store a stack-local value (pointer remains valid in this scope)
Payload_SetLocalValue(&msg, "out", int, 0);

// Read a value
int* hp = (int*)Payload_Get(&msg, "health");
int hp_val = Payload_GetDeref(&msg, "health", int);

// Check existence
if (Payload_Has(&msg, "health")) { /* ... */ }

// Get byte size
uint32_t sz = Payload_GetSize(&msg, "health");

// Remove
Payload_Remove(&msg, "health");
```

## Message Handler Macros

These macros are used inside `MESSAGE_HANDLER_BEGIN` / `MESSAGE_HANDLER_END`
blocks. They implicitly operate on the `payload` parameter.

### Getters

- **MH_Get(paramname, type)** -- Returns a typed pointer (`type*`) to the
  stored value, or `NULL` if not found.

  ```c
  float* ptr = MH_Get(Strength, float);
  if (ptr != NULL) { /* use *ptr */ }
  ```

- **MH_GetDeref(paramname, type)** -- Returns the stored value by copy.
  Unsafe -- crashes if the key is missing. Use `MH_Has` or `MH_Extract` for
  safe access.

  ```c
  float val = MH_GetDeref(Strength, float);
  ```

### Setters

- **MH_Set(paramname, value_ptr, value_size)** -- Stores raw bytes into the
  payload data.

  ```c
  MH_Set(result, &my_data, sizeof(my_data));
  ```

- **MH_SetValue(paramname, type, var)** -- Stores a typed value (takes the
  address for you).

  ```c
  int result = a + b;
  MH_SetValue(result, int, result);
  ```

### Checks

- **MH_Has(paramname)** -- Bool expression: true if the key exists.

  ```c
  if (MH_Has(Strength)) { /* ... */ }
  ```

- **MH_Require(paramname)** -- If the key does not exist, sets
  `payload->result` to `MESSAGE_RESULT_MISSING_PARAMS` and returns from the
  handler immediately.

  ```c
  MH_Require(Strength);
  // Strength is guaranteed to exist here
  ```

### Extractions (safe require + declare)

- **MH_Extract(paramname, type)** -- Requires the key, then declares
  `type* paramname` pointing to the stored value.

  ```c
  MH_Extract(Strength, float);
  // float* Strength is now available
  LOG_INFO("Strength: %f", *Strength);
  ```

- **MH_ExtractDeref(paramname, type)** -- Requires the key, then declares
  `type paramname` as a copy of the stored value.

  ```c
  MH_ExtractDeref(Strength, float);
  // float Strength is now available
  LOG_INFO("Strength: %f", Strength);
  ```

## Result Codes

| Code | Value | Name | Description |
|------|-------|------|-------------|
| `MESSAGE_RESULT_SUCCESS` | 0 | SUCCESS | Operation completed successfully |
| `MESSAGE_RESULT_OOM` | 1 | OOM | Out of memory |
| `MESSAGE_RESULT_MISSING_PARAMS` | 2 | MISSING_PARAMS | Required parameters are missing |
| `MESSAGE_RESULT_INVALID_PARAMS` | 3 | INVALID_PARAMS | Parameters are invalid or out of range |
| `MESSAGE_RESULT_UNKNOWN_ERROR` | 4 | UNKNOWN_ERROR | An unknown error occurred |
| `MESSAGE_RESULT_INVALID_CID` | 5 | INVALID_CID | Target class ID does not exist |
| `MESSAGE_RESULT_INVALID_MID` | 6 | INVALID_MID | Message ID is not recognized |
| `MESSAGE_RESULT_NO_PAYLOAD` | 7 | NO_PAYLOAD | Message payload is NULL |
| `MESSAGE_RESULT_INTERNAL_ERROR` | 8 | INTERNAL_ERROR | Internal processing error |
| `MESSAGE_RESULT_NOT_SUPPORTED` | 9 | NOT_SUPPORTED | Target does not support this message |
| `MESSAGE_RESULT_BUSY` | 10 | BUSY | Target is busy and cannot process now |
| `MESSAGE_RESULT_TIMEOUT` | 11 | TIMEOUT | Operation timed out |
| `MESSAGE_RESULT_DENIED` | 12 | DENIED | Operation was denied by the target |
| `MESSAGE_RESULT_DUPLICATE` | 13 | DUPLICATE | A duplicate entry already exists |
| `MESSAGE_RESULT_NOT_FOUND` | 14 | NOT_FOUND | Requested resource was not found |
| `MESSAGE_RESULT_OVERFLOW` | 15 | OVERFLOW | Data exceeded maximum capacity |
| `MESSAGE_RESULT_NOT_READY` | 16 | NOT_READY | Target is not yet initialized |
| `MESSAGE_RESULT_CANCELLED` | 17 | CANCELLED | Operation was cancelled |
| `MESSAGE_RESULT_PENDING` | 18 | PENDING | Message has not yet been acknowledged |
| `MESSAGE_RESULT_IGNORED` | 19 | IGNORED | Message was never acknowledged |
| `MESSAGE_RESULT_NOTSENT` | 20 | NOTSENT | Message has not been sent |
| `MESSAGE_RESULT_INVALID_SELF` | 21 | INVALID_SELF | Invalid 'Self' object |

Use `MESSAGE_RESULT_ISOK(r)` to check for success (tests `r == 0`).
Use `MESSAGE_RESULT_NAME(r)` to get the short name string.
Use `MESSAGE_RESULT_DESC(r)` to get the human-readable description.

## Extern Messages

A class can handle messages that were declared in another class's namespace.
This is useful when one class needs to respond to another class's message IDs
(for example, handling a shared lifecycle message like `SELF_Create`).

Three EXTERN macros are provided:

- **MESSAGE_HANDLER_BEGIN_EXTERN(classname, handlername)** -- Defines a handler
  for a message that belongs to `classname`'s namespace. With `#define TYPE
  Counter`, `MESSAGE_HANDLER_BEGIN_EXTERN(Default, SELF_Create)` creates a
  function named `MESSAGE_HANDLER_Counter_Default_SELF_Create`.

  ```c
  MESSAGE_HANDLER_BEGIN_EXTERN(Default, SELF_Create)
      // Handle Default.SELF_Create in this class
  MESSAGE_HANDLER_END()
  ```

- **CAN_RECEIVE_MID_EXTERN(classname, msgname)** -- Declares that this class
  can receive a message from another class's namespace. Checks against
  `MID_classname_msgname`.

  ```c
  CAN_RECEIVE_BEGIN()
      CAN_RECEIVE_MID(MyOwnMessage)
      CAN_RECEIVE_MID_EXTERN(Default, SELF_Create)
  CAN_RECEIVE_END()
  ```

- **RECEIVE_MESSAGE_ROUTE_EXTERN(classname, msgname)** -- Routes an incoming
  extern message to its handler. Matches `MID_classname_msgname` and calls
  `MESSAGE_HANDLER_TYPE_classname_msgname`.

  ```c
  RECEIVE_MESSAGE_BEGIN()
      RECEIVE_MESSAGE_ROUTE(MyOwnMessage)
      RECEIVE_MESSAGE_ROUTE_EXTERN(Default, SELF_Create)
  RECEIVE_MESSAGE_END()
  ```

All three EXTERN macros must be used together for each extern message: declare
the handler, advertise it in `CAN_RECEIVE`, and route it in `RECEIVE_MESSAGE`.

## Contract

- **ClassID 0 is reserved.** `CID_Untyped` (`0x0000`) cannot be registered or
  receive messages. Attempting to dispatch to it returns
  `MESSAGE_RESULT_INVALID_CID`.

- **No overwriting.** `RegisterClass` does not overwrite an existing
  registration. If a ClassID is already registered, the call is rejected with
  an error log.

- **Unique names.** Class names are checked for duplicates across all registered
  classes. A duplicate name is rejected with an error log.

- **Handlers must set result.** `MESSAGE_HANDLER_BEGIN` auto-sets
  `payload->result` to `MESSAGE_RESULT_SUCCESS`. If a handler overrides this
  (e.g., to set an error) it must do so explicitly. If the result is still
  `MESSAGE_RESULT_PENDING` after the handler returns, `DispatchMessage` logs an
  error and changes it to `MESSAGE_RESULT_IGNORED`.

- **Registration window.** Classes can only be registered between
  `BeginClassRegistrations()` and `EndClassRegistrations()`. Attempting to
  register outside this window is rejected. Once closed, the dispatch table is
  stable and safe to call without synchronization.

- **CanReceiveMID and ReceiveMessage are required.** A class definition must
  provide both function pointers. `RegisterClass` rejects definitions with
  `NULL` for either.
