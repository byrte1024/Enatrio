#pragma once

// Core object type definitions and SELF macros.
// Included by Self.h and the Object sub-headers.

#include <stdint.h>
#include <stdio.h>
#include "Class.h"

// ============================================================
// Object types
// ============================================================

typedef struct ObjectData {
    // Primitive vals/pointers only. NO nested garbage collection.
    UnsafeVariedHashMap* values;

    // A hashmap of ObjectReference's.
    UnsafeHashMap* references;
} ObjectData;

typedef ObjectData* ObjectDataPTR;

typedef struct ObjectContainer {
    ObjectDataPTR data;
    ClassID cid;

    int internal_refs; // refs from other objects' reference hashmaps (ObjectReference)
    int external_refs; // refs from stack/globals (ExternalReference)
} ObjectContainer;

// ObjectReference: stored ONLY inside reference hashmaps. Increments internal_refs.
typedef struct ObjectContainer* ObjectReference;

// ExternalReference: held on stack/globals. Increments external_refs.
typedef struct ObjectContainer* ExternalReference;

// TempObjectReference: raw pointer, no refcount. Borrowed, not owned.
typedef struct ObjectContainer* TempObjectReference;

#define ObjectContainer_TotalRefs(c) ((c)->internal_refs + (c)->external_refs)

// ============================================================
// SELF message macros
// ============================================================

#define DECLARE_SELF_MID(msgname) DECLARE_MID(BAT2(SELF_, msgname))

#define SELF_MESSAGE_HANDLER_BEGIN(msgname) \
    MESSAGE_HANDLER_BEGIN(BAT2(SELF_, msgname)) \
    MH_Require(Self); \
    TempObjectReference Self = MH_GetDeref(Self, TempObjectReference);

#define SELF_MESSAGE_HANDLER_BEGIN_EXTERN(classname, msgname) \
    MESSAGE_HANDLER_BEGIN_EXTERN(classname, BAT2(SELF_, msgname)) \
    MH_Require(Self); \
    TempObjectReference Self = MH_GetDeref(Self, TempObjectReference);

#define SELF_CAN_RECEIVE_MID(msgname) \
    CAN_RECEIVE_MID(BAT2(SELF_, msgname))

#define SELF_CAN_RECEIVE_MID_EXTERN(classname, msgname) \
    CAN_RECEIVE_MID_EXTERN(classname, BAT2(SELF_, msgname))

#define SELF_RECEIVE_MESSAGE_ROUTE(msgname) \
    RECEIVE_MESSAGE_ROUTE(BAT2(SELF_, msgname))

#define SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(classname, msgname) \
    RECEIVE_MESSAGE_ROUTE_EXTERN(classname, BAT2(SELF_, msgname))

// ============================================================
// Default SELF MIDs
// ============================================================

#define TYPE Default

DECLARE_SELF_MID(Create);
DECLARE_SELF_MID(Destroy);

#undef TYPE
