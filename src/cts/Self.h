#pragma once

#include <stdint.h>
#include <stdio.h>

#include "Class.h"

typedef struct ObjectData {
    //Must ONLY contain primitive vals/pointers, NOTHING that needs nested garbage collection, cause it wont be done.
    UnsafeVariedHashMap* values;

    //A hashmap of ObjectReference's.
    UnsafeHashMap* references;
} ObjectData;

typedef ObjectData* ObjectDataPTR;

typedef struct ObjectContainer {
    ObjectDataPTR data;
    ClassID cid;
    
    int reference_counter; //how many references to us currently exist
} ObjectContainer;

typedef struct ObjectContainer* ObjectReference;

typedef struct ObjectContainer* TempObjectReference;

// ============================================================
// SELF message macros
// ============================================================

// Declares a self MID under the current TYPE.
//   DECLARE_SELF_MID(Create)  ->  MID_Counter_SELF_Create = "Counter.SELF_Create"
#define DECLARE_SELF_MID(msgname) DECLARE_MID(BAT2(SELF_, msgname))

// Opens a self message handler under the current TYPE. Auto-extracts Self.
//   SELF_MESSAGE_HANDLER_BEGIN(Increment)
//   -> MESSAGE_HANDLER_BEGIN(SELF_Increment) with Self extracted
#define SELF_MESSAGE_HANDLER_BEGIN(msgname) \
    MESSAGE_HANDLER_BEGIN(BAT2(SELF_, msgname)) \
    MH_Require(Self); \
    TempObjectReference Self = MH_GetDeref(Self, TempObjectReference);

// Opens a self message handler under an EXTERN class. Auto-extracts Self.
// Use for Default lifecycle messages (Create, Destroy).
//   SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Default, Create)
#define SELF_MESSAGE_HANDLER_BEGIN_EXTERN(classname, msgname) \
    MESSAGE_HANDLER_BEGIN_EXTERN(classname, BAT2(SELF_, msgname)) \
    MH_Require(Self); \
    TempObjectReference Self = MH_GetDeref(Self, TempObjectReference);

// Adds a self MID check under the current TYPE.
//   SELF_CAN_RECEIVE_MID(Increment)
#define SELF_CAN_RECEIVE_MID(msgname) \
    CAN_RECEIVE_MID(BAT2(SELF_, msgname))

// Adds a self MID check under an EXTERN class.
//   SELF_CAN_RECEIVE_MID_EXTERN(Default, Create)
#define SELF_CAN_RECEIVE_MID_EXTERN(classname, msgname) \
    CAN_RECEIVE_MID_EXTERN(classname, BAT2(SELF_, msgname))

// Routes a self MID to its handler under the current TYPE.
//   SELF_RECEIVE_MESSAGE_ROUTE(Increment)
#define SELF_RECEIVE_MESSAGE_ROUTE(msgname) \
    RECEIVE_MESSAGE_ROUTE(BAT2(SELF_, msgname))

// Routes a self MID to a handler under an EXTERN class.
//   SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Default, Create)
#define SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(classname, msgname) \
    RECEIVE_MESSAGE_ROUTE_EXTERN(classname, BAT2(SELF_, msgname))

// ============================================================
// Default SELF MIDs (all classes that support Self must handle these)
// ============================================================

#define TYPE Default

DECLARE_SELF_MID(Create);
DECLARE_SELF_MID(Destroy);

#undef TYPE


static inline MessagePayload PrepareSelfPayload(ObjectReference reference, MessageID mid) {
    MessagePayload payload = {0};

    if(reference == NULL){
        LOG_ERROR("ObjectReference is NULL, cannot prepare payload.");
        return payload;
    }

    payload.cid_target = reference->cid;
    memcpy(payload.mid, mid, sizeof(MessageID));
    payload.result = MESSAGE_RESULT_NOTSENT;

    payload.data = UnsafeVariedHashMap_Create(8);
    if (payload.data == NULL) {
        LOG_ERROR("Failed to allocate payload data map.");
        return payload;
    }

    Payload_SetValue(&payload, "Self", TempObjectReference, reference);

    return payload;
}
static TempObjectReference ObjectContainer_CreateGhost(){
    TempObjectReference container = (TempObjectReference)malloc(sizeof(ObjectContainer));
    if(container == NULL){
        LOG_ERROR("Failed to malloc ObjectContainer in ObjectContainerContainer_CreateEmpty");
        return NULL;
    }

    container->data = NULL;
    container->cid = CID_Untyped;
    container->reference_counter = 0;
    return container;
}
//Object container must be: 1. empty (not full), 2. untyped (not typed), 3. unreferenced (not referenced)
static void ObjectContainer_DestroyGhost(TempObjectReference container){
    if(container == NULL){
        LOG_ERROR("ObjectContainer is already destroyed.");
        return;
    }
    if(container->data != NULL){
        LOG_ERROR("ObjectContainer is full, cannot destroy");
        return;
    }
    if(container->cid != CID_Untyped){
        LOG_ERROR("ObjectContainer is typed (%s), cannot destroy", CLASSID_TOSTRING(container->cid));
        return;
    }
    if(container->reference_counter != 0){
        LOG_ERROR("ObjectContainer is referenced, cannot destroy");
        return;
    }

    free(container);   
}
//Object container must be: 1. empty (not full), 2. untyped (not typed)
static void ObjectContainer_TypeEmptyUntyped(TempObjectReference container, ClassID cid){
    if(container == NULL){
        LOG_ERROR("ObjectContainer is destroyed, cannot type.");
        return;
    }
    if(container->cid != CID_Untyped){
        LOG_ERROR("ObjectContainer is typed (%s), cannot type", CLASSID_TOSTRING(container->cid));
        return;
    }
    if(container->data != NULL){
        LOG_ERROR("ObjectContainer is full, cannot type");
        return;
    }

    container->cid = cid;
}
//Object container must be: 1. empty (not full), 2. typed (not untyped)
static void ObjectContainer_FillEmptyTyped(TempObjectReference container){
    if(container == NULL){
        LOG_ERROR("ObjectContainer is destroyed, cannot fill.");
        return;
    }
    if(container->cid == CID_Untyped){
        LOG_ERROR("ObjectContainer is untyped, cannot fill");
        return;
    }
    if(container->data != NULL){
        LOG_ERROR("ObjectContainer is full, cannot fill");
        return;
    }

    if(!CanDispatchMessage(MID_Default_SELF_Create, container->cid)){
        LOG_ERROR("ObjectContainer is typed (%s) but does not support this SELF function, cannot fill", CLASSID_TOSTRING(container->cid));
        return;
    }

    container->data = (ObjectData*)malloc(sizeof(ObjectData));
    if(container->data == NULL){
        LOG_ERROR("Failed to malloc ObjectData in ObjectContainer_FillEmptyTyped");
        return;
    }
    container->data->values = UnsafeVariedHashMap_Create(8);
    if(container->data->values == NULL){
        LOG_ERROR("Failed to malloc UnsafeVariedHashMap in ObjectContainer_FillEmptyTyped");
        free(container->data);
        container->data = NULL;
        return;
    }
    container->data->references = UnsafeHashMap_Create(sizeof(ObjectReference),8);
    if(container->data->references == NULL){
        LOG_ERROR("Failed to malloc UnsafeHashMap in ObjectContainer_FillEmptyTyped");
        UnsafeVariedHashMap_Destroy(container->data->values);
        free(container->data);
        container->data = NULL;
        return;
    }

    MessagePayload payload = PrepareSelfPayload(container, MID_Default_SELF_Create);
    if(payload.data == NULL){
        LOG_ERROR("Failed to prepare payload in ObjectContainer_FillEmptyTyped");
        UnsafeVariedHashMap_Destroy(container->data->values);
        UnsafeHashMap_Destroy(container->data->references);
        free(container->data);
        container->data = NULL;
        return;
    }
    DispatchMessage(&payload);
    if(!MESSAGE_RESULT_ISOK(payload.result)){
        LOG_ERROR("Failed to create self in ObjectContainer_FillEmptyTyped");
        UnsafeVariedHashMap_Destroy(container->data->values);
        UnsafeHashMap_Destroy(container->data->references);
        free(container->data);
        container->data = NULL;
        return;
    }
    FreePayload(&payload);
    
}
//Object container must be: 1. full (not empty), 2. typed (not untyped)
static void ObjectContainer_EmptyFilledTyped(TempObjectReference container){
    if(container == NULL){
        LOG_ERROR("ObjectContainer is destroyed, cannot empty.");
        return;
    }
    if(container->cid == CID_Untyped){
        LOG_ERROR("ObjectContainer is untyped, cannot empty.");
        return;
    }
    if(container->data == NULL){
        LOG_ERROR("ObjectContainer is empty, cannot empty.");
        return;
    }

    if(!CanDispatchMessage(MID_Default_SELF_Destroy, container->cid)){
        LOG_ERROR("ObjectContainer is typed (%s) but does not support this SELF function, cannot empty.", CLASSID_TOSTRING(container->cid));
        return;
    }

    //Prepare payload
    MessagePayload payload = PrepareSelfPayload(container, MID_Default_SELF_Destroy);
    if(payload.data == NULL){
        LOG_ERROR("Failed to prepare payload in ObjectContainer_EmptyFilledTyped");
        return;
    }
    DispatchMessage(&payload);
    if(!MESSAGE_RESULT_ISOK(payload.result)){
        LOG_ERROR("Failed to destroy self in ObjectContainer_EmptyFilledTyped");
        return;
    }
    FreePayload(&payload);

    UnsafeVariedHashMap_Destroy(container->data->values);

    //freeing references is a bit more complex, need to decrement ref counter not them, empty for now since i didnt emplement ref functions yet rip.

    UnsafeHashMap_Destroy(container->data->references);

    free(container->data);
    container->data = NULL;
}
    
static void ObjectContainer_UntypeEmptyTyped(TempObjectReference container){
    if(container == NULL){
        LOG_ERROR("ObjectContainer is destroyed, cannot unype.");
        return;
    }
    if(container->cid == CID_Untyped){
        LOG_ERROR("ObjectContainer is untyped (%s), cannot unype", CLASSID_TOSTRING(container->cid));
        return;
    }
    if(container->data != NULL){
        LOG_ERROR("ObjectContainer is full, cannot unype");
        return;
    }

    container->cid = CID_Untyped;
}

#define ObjectContainer_TempRef_From_Ref(ref) ((TempObjectReference)(ref))

static inline ObjectReference ObjectContainer_Ref_From_TempRef(TempObjectReference tref){
    if(tref!=NULL){
        tref->reference_counter++;
    }
    return (ObjectReference)tref;
}
static inline ObjectReference ObjectContainer_Ref_From_Ref(ObjectReference ref){
    if(ref!=NULL){
        ref->reference_counter++;
    }
    return ref;
}

static inline void ObjectContainer_UnRef(ObjectReference* ref){
    if(ref == NULL){
        LOG_ERROR("Cannot UNREF a null address");
        return;
    }
    ObjectReference obj = *ref;
    if(obj == NULL){
        LOG_ERROR("Cannot UNREF an UNREF'd REF");
        return;
    }

    if(obj->reference_counter < 0){
        LOG_ERROR("OBJECT WITH NEGATIVE REF COUNT STILL HAS REFERENCES AND STILL EXISTS, SOMETHING HAS GONE HORRIBLY WRONG.");
    }

    obj->reference_counter--;

    *ref = NULL;

    if(obj->reference_counter <= 0){
        if(obj->data != NULL){
            ObjectContainer_EmptyFilledTyped(obj);
        }
        if(obj->cid != CID_Untyped){
            ObjectContainer_UntypeEmptyTyped(obj);
        }
        free(obj);
    }
}

// ============================================================
// Object helper macros
// ============================================================

// Full lifecycle: create ghost -> type -> fill. Returns TempObjectReference (no refcount).
//   TempObjectReference obj = Object_Create(CID_Exploder);
static inline TempObjectReference Object_Create(ClassID cid) {
    TempObjectReference obj = ObjectContainer_CreateGhost();
    if (obj == NULL) return NULL;
    ObjectContainer_TypeEmptyUntyped(obj, cid);
    ObjectContainer_FillEmptyTyped(obj);
    if (obj->data == NULL) {
        // Fill failed, roll back
        ObjectContainer_UntypeEmptyTyped(obj);
        ObjectContainer_DestroyGhost(obj);
        return NULL;
    }
    return obj;
}

// Full lifecycle: empty -> untype -> destroy ghost. Takes TempObjectReference (must have 0 refs).
static inline void Object_Destroy(TempObjectReference obj) {
    if (obj == NULL) {
        LOG_ERROR("Cannot destroy NULL object.");
        return;
    }
    if (obj->reference_counter != 0) {
        LOG_ERROR("Cannot destroy object with %d references.", obj->reference_counter);
        return;
    }
    if (obj->data != NULL) {
        ObjectContainer_EmptyFilledTyped(obj);
    }
    if (obj->cid != CID_Untyped) {
        ObjectContainer_UntypeEmptyTyped(obj);
    }
    ObjectContainer_DestroyGhost(obj);
}

// Create and immediately take a reference. Returns ObjectReference (refcount = 1).
//   ObjectReference ref = Object_CreateRef(CID_Exploder);
static inline ObjectReference Object_CreateRef(ClassID cid) {
    TempObjectReference obj = Object_Create(cid);
    if (obj == NULL) return NULL;
    return ObjectContainer_Ref_From_TempRef(obj);
}

// Shorthand: access Self's values hashmap inside a SELF handler.
//   Self_Values  ->  Self->data->values
#define Self_Values (Self->data->values)

// Shorthand: access Self's references hashmap inside a SELF handler.
//   Self_Refs  ->  Self->data->references
#define Self_Refs (Self->data->references)

// Get a typed pointer to a Self value.
//   float* hp = Self_Get("health", float);
#define Self_Get(str_key, type) \
    ((type*)UnsafeVariedHashMap_SGet(Self_Values, str_key))

// Get a Self value by copy (dereference).
//   float hp = Self_GetDeref("health", float);
#define Self_GetDeref(str_key, type) \
    UnsafeVariedHashMap_SGetDeref(Self_Values, str_key, type)

// Set a typed value on Self.
//   Self_SetValue("health", float, 100.0f);
#define Self_SetValue(str_key, type, value) \
    UnsafeVariedHashMap_SSetValue(Self_Values, str_key, type, value)

// Set raw bytes on Self.
//   Self_Set("blob", ptr, size);
#define Self_Set(str_key, value_ptr, value_size) \
    UnsafeVariedHashMap_SSet(Self_Values, str_key, value_ptr, value_size)

// Check if Self has a value.
//   if (Self_Has("health")) { ... }
#define Self_Has(str_key) \
    UnsafeVariedHashMap_SHas(Self_Values, str_key)