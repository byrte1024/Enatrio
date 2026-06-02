#pragma once

// Core object type definitions and SELF macros.
// Included by Self.h and the Object sub-headers.

#include <stdint.h>
#include <stdio.h>
#include "../class/Class.h"

// ============================================================
// Object types
// ============================================================

typedef struct ObjectData {
    // Primitive vals/pointers only -- no nested garbage collection.
    UnsafeVariedHashMap* values;

    // Holds ObjectReference pointers that participate in refcounting/GC.
    UnsafeHashMap* references;
} ObjectData;

typedef ObjectData* ObjectDataPTR;

typedef struct ObjectContainer {
    ObjectDataPTR data;
    ClassID cid;

    int internal_refs;
    int external_refs;
} ObjectContainer;

// Three pointer typedefs all alias ObjectContainer*, but encode ownership
// semantics for the GC:
// - ObjectReference: stored in reference hashmaps. Increments internal_refs.
//   The GC uses internal_refs to detect reference cycles.
// - ExternalReference: held on stack/globals. Increments external_refs.
//   Objects with external_refs > 0 are reachable and never collected.
// - TempObjectReference: raw borrowed pointer, no refcount change.
//   Callers must ensure the target outlives the borrow.
typedef struct ObjectContainer* ObjectReference;
typedef struct ObjectContainer* ExternalReference;
typedef struct ObjectContainer* TempObjectReference;

#define ObjectContainer_TotalRefs(c) ((c)->internal_refs + (c)->external_refs)

// ============================================================
// Value metadata -- every value stored in ObjectData.values
// carries a 6-byte header so the serializer knows how to
// persist it without class-specific knowledge.
// ============================================================

#define SER_RAW    0
#define SER_SKIP   1
#define SER_DEREF  2
#define SER_STRING 3
#define SER_USER_START 16
#define SER_MAX_ID 256

typedef struct {
    ClassID owner;
    uint16_t ser_id;
    uint16_t ser_arg;
} ObjectValueHeader;

// ============================================================
// SELF message macros
// ============================================================

#define DECLARE_SELF_MID(msgname) DECLARE_MID(BAT2(SELF_, msgname))

#define LINTNORE
#define SELF_MESSAGE_HANDLER_BEGIN(msgname) \
    MESSAGE_HANDLER_BEGIN(BAT2(SELF_, msgname)) \
    MH_Require(Self); \
    TempObjectReference Self = MH_GetDeref(Self, TempObjectReference);

#define SELF_MESSAGE_HANDLER_BEGIN_EXTERN(classname, msgname) \
    MESSAGE_HANDLER_BEGIN_EXTERN(classname, BAT2(SELF_, msgname)) \
    MH_Require(Self); \
    TempObjectReference Self = MH_GetDeref(Self, TempObjectReference);

#undef LINTNORE

#define SELF_CAN_RECEIVE_MID(msgname) \
    CAN_RECEIVE_MID(BAT2(SELF_, msgname))

#define SELF_CAN_RECEIVE_MID_EXTERN(classname, msgname) \
    CAN_RECEIVE_MID_EXTERN(classname, BAT2(SELF_, msgname))

#define SELF_RECEIVE_MESSAGE_ROUTE(msgname) \
    RECEIVE_MESSAGE_ROUTE(BAT2(SELF_, msgname))

#define SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(classname, msgname) \
    RECEIVE_MESSAGE_ROUTE_EXTERN(classname, BAT2(SELF_, msgname))

// ============================================================
// Object base class -- CID, classname, and SELF MIDs
// ============================================================

#define TYPE Object
BEGIN_CLASS(0x0001);

DECLARE_SELF_MID(Create);
DECLARE_SELF_MID(Destroy);
DECLARE_SELF_MID(Serialize);
DECLARE_SELF_MID(Deserialize);

#undef TYPE

