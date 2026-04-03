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

// Opens a self message handler for an extern class's MID, under TYPE's namespace.
// Use for Default lifecycle messages (Create, Destroy).
//   SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Default, Create)
//   With TYPE=Counter -> MESSAGE_HANDLER_Counter_Default_SELF_Create
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


static inline MessagePayload PrepareSelfPayload(TempObjectReference reference, MessageID mid) {
    MessagePayload payload = {0};

    if(reference == NULL){
        LOG_ERROR("Reference is NULL, cannot prepare payload.");
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
    container->internal_refs = 0;
    container->external_refs = 0;
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
    if(ObjectContainer_TotalRefs(container) != 0){
        LOG_ERROR("ObjectContainer is referenced (internal=%d, external=%d), cannot destroy",
            container->internal_refs, container->external_refs);
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
// Forward declaration (needed because EmptyFilledTyped unrefs held references)
static inline void ObjectContainer_UnRef_Internal(ObjectReference* ref);

static void _ObjectContainer_UnRefEach(const void *key, uint32_t key_len, void *value) {
    (void)key; (void)key_len;
    ObjectReference *ref = (ObjectReference *)value;
    if (ref != NULL && *ref != NULL) {
        ObjectContainer_UnRef_Internal(ref);
    }
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

    // Detach data BEFORE unreffing held references.
    // This prevents re-entrant EmptyFilledTyped if a self-reference causes
    // UnRef to try emptying us again (data == NULL guard catches it).
    ObjectData *data = container->data;
    container->data = NULL;

    UnsafeVariedHashMap_Destroy(data->values);

    // UnRef all held references before destroying the hashmap
    UnsafeHashMap_ForEach(data->references, _ObjectContainer_UnRefEach);
    UnsafeHashMap_Destroy(data->references);

    free(data);
}
    
static void ObjectContainer_UntypeEmptyTyped(TempObjectReference container){
    if(container == NULL){
        LOG_ERROR("ObjectContainer is destroyed, cannot untype.");
        return;
    }
    if(container->cid == CID_Untyped){
        LOG_ERROR("ObjectContainer is untyped (%s), cannot untype", CLASSID_TOSTRING(container->cid));
        return;
    }
    if(container->data != NULL){
        LOG_ERROR("ObjectContainer is full, cannot untype");
        return;
    }

    container->cid = CID_Untyped;
}

// ============================================================
// Reference conversions
// ============================================================

// Any ref type -> TempObjectReference (borrowed, no refcount change)
#define ObjectContainer_TempFrom(ref) ((TempObjectReference)(ref))

// ---- Internal refs (ObjectReference) -- only for reference hashmaps ----

// Create internal ref from TempObjectReference. Increments internal_refs.
static inline ObjectReference ObjectContainer_InternalRef_From_Temp(TempObjectReference tref) {
    if (tref != NULL) tref->internal_refs++;
    return (ObjectReference)tref;
}

// Create internal ref from another internal ref. Increments internal_refs.
static inline ObjectReference ObjectContainer_InternalRef_From_Internal(ObjectReference ref) {
    if (ref != NULL) ref->internal_refs++;
    return ref;
}

// Create internal ref from external ref. Increments internal_refs.
static inline ObjectReference ObjectContainer_InternalRef_From_External(ExternalReference ref) {
    if (ref != NULL) ref->internal_refs++;
    return (ObjectReference)ref;
}

// ---- External refs (ExternalReference) -- for stack/globals ----

// Create external ref from TempObjectReference. Increments external_refs.
static inline ExternalReference ObjectContainer_ExternalRef_From_Temp(TempObjectReference tref) {
    if (tref != NULL) tref->external_refs++;
    return (ExternalReference)tref;
}

// Create external ref from another external ref. Increments external_refs.
static inline ExternalReference ObjectContainer_ExternalRef_From_External(ExternalReference ref) {
    if (ref != NULL) ref->external_refs++;
    return ref;
}

// Create external ref from internal ref. Increments external_refs.
static inline ExternalReference ObjectContainer_ExternalRef_From_Internal(ObjectReference ref) {
    if (ref != NULL) ref->external_refs++;
    return (ExternalReference)ref;
}

// ---- UnRef ----

// Drop an internal ref. Decrements internal_refs. If total refs hit 0, object is destroyed.
static inline void ObjectContainer_UnRef_Internal(ObjectReference* ref) {
    if (ref == NULL) { LOG_ERROR("Cannot UNREF a null address"); return; }
    ObjectReference obj = *ref;
    if (obj == NULL) { LOG_ERROR("Cannot UNREF an UNREF'd REF"); return; }

    obj->internal_refs--;
    *ref = NULL;

    if (ObjectContainer_TotalRefs(obj) <= 0) {
        if (obj->data != NULL) ObjectContainer_EmptyFilledTyped(obj);
        if (obj->cid != CID_Untyped) ObjectContainer_UntypeEmptyTyped(obj);
        free(obj);
    }
}

// -- Cycle collection --

inline bool _gc_running = false;

static int _gc_is_visited(UnsafeArray *visited, TempObjectReference target) {
    for (uint32_t i = 0; i < visited->count; i++) {
        if (*(TempObjectReference*)UnsafeArray_Get(visited, i) == target) return 1;
    }
    return 0;
}

static void _gc_add_neighbors(UnsafeArray *worklist, UnsafeArray *visited, TempObjectReference node) {
    if (node->data == NULL || node->data->references == NULL) return;
    UnsafeHashMap *refs = node->data->references;
    for (uint32_t i = 0; i < refs->bucket_count; i++) {
        UnsafeHashEntry *e = &refs->buckets[i];
        if (e->value < 0) continue;
        ObjectReference *ref_ptr = (ObjectReference*)UnsafeArray_Get(refs->values, (uint32_t)e->value);
        if (ref_ptr == NULL || *ref_ptr == NULL) continue;
        TempObjectReference target = ObjectContainer_TempFrom(*ref_ptr);
        if (!_gc_is_visited(visited, target)) {
            UnsafeArray_Add(worklist, &target);
            UnsafeArray_Add(visited, &target);
        }
    }
}

// Traverse component from root. If no node has external_refs > 0, collect all.
static void _ObjectContainer_TryCollectCycle(TempObjectReference root) {
    if (root == NULL || root->data == NULL) return;
    if (_gc_running) return;
    _gc_running = true;

    UnsafeArray *worklist = UnsafeArray_Create(sizeof(TempObjectReference), 16);
    UnsafeArray *visited = UnsafeArray_Create(sizeof(TempObjectReference), 16);

    UnsafeArray_Add(worklist, &root);
    UnsafeArray_Add(visited, &root);

    int has_external = 0;
    uint32_t idx = 0;

    // BFS: find all reachable nodes, check for externals
    while (idx < worklist->count) {
        TempObjectReference node = *(TempObjectReference*)UnsafeArray_Get(worklist, idx);
        idx++;

        if (node->external_refs > 0) {
            has_external = 1;
            break;
        }

        _gc_add_neighbors(worklist, visited, node);
    }

    if (!has_external) {
        // Entire component is garbage.
        // Pass 1: dispatch SELF_Destroy on all filled nodes (proper cleanup).
        for (uint32_t i = 0; i < visited->count; i++) {
            TempObjectReference node = *(TempObjectReference*)UnsafeArray_Get(visited, i);
            if (node->data != NULL) {
                if (CanDispatchMessage(MID_Default_SELF_Destroy, node->cid)) {
                    MessagePayload p = PrepareSelfPayload(node, MID_Default_SELF_Destroy);
                    if (p.data != NULL) {
                        DispatchMessage(&p);
                        FreePayload(&p);
                    }
                }
            }
        }
        // Pass 2: tear down data (without unreffing children -- entire component is being freed).
        for (uint32_t i = 0; i < visited->count; i++) {
            TempObjectReference node = *(TempObjectReference*)UnsafeArray_Get(visited, i);
            if (node->data != NULL) {
                UnsafeVariedHashMap_Destroy(node->data->values);
                UnsafeHashMap_Destroy(node->data->references);
                free(node->data);
                node->data = NULL;
            }
        }
        // Pass 3: free all containers.
        for (uint32_t i = 0; i < visited->count; i++) {
            TempObjectReference node = *(TempObjectReference*)UnsafeArray_Get(visited, i);
            node->internal_refs = 0;
            node->external_refs = 0;
            node->cid = CID_Untyped;
            free(node);
        }
    }

    UnsafeArray_Destroy(worklist);
    UnsafeArray_Destroy(visited);
    _gc_running = false;
}

// Drop an external ref. Decrements external_refs.
// If total refs hit 0, destroyed immediately.
// If external_refs hit 0 but internal_refs remain, try to collect the cycle.
static inline void ObjectContainer_UnRef_External(ExternalReference* ref) {
    if (ref == NULL) { LOG_ERROR("Cannot UNREF a null address"); return; }
    ExternalReference obj = *ref;
    if (obj == NULL) { LOG_ERROR("Cannot UNREF an UNREF'd REF"); return; }

    obj->external_refs--;
    *ref = NULL;

    if (ObjectContainer_TotalRefs(obj) <= 0) {
        if (obj->data != NULL) ObjectContainer_EmptyFilledTyped(obj);
        if (obj->cid != CID_Untyped) ObjectContainer_UntypeEmptyTyped(obj);
        free(obj);
    } else if (obj->external_refs <= 0 && obj->internal_refs > 0 && !_gc_running) {
        // Candidate for cycle collection (skip if GC already running)
        _ObjectContainer_TryCollectCycle(obj);
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
    if (ObjectContainer_TotalRefs(obj) != 0) {
        LOG_ERROR("Cannot destroy object with refs (internal=%d, external=%d).", obj->internal_refs, obj->external_refs);
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

// Empties a filled+typed object. Data is freed, type remains. Refs stay valid.
//   Object_Empty(obj);
static inline void Object_EmptyFilledType(ExternalReference obj) {
    if (obj == NULL) {
        LOG_ERROR("Cannot empty NULL object.");
        return;
    }
    if (obj->data == NULL) {
        LOG_ERROR("Object is already empty.");
        return;
    }
    ObjectContainer_EmptyFilledTyped(ObjectContainer_TempFrom(obj));
}

// Untypes an empty+typed object. Refs stay valid.
//   Object_Untype(obj);
static inline void Object_UntypeEmptyTyped(ExternalReference obj) {
    if (obj == NULL) {
        LOG_ERROR("Cannot untype NULL object.");
        return;
    }
    if (obj->data != NULL) {
        LOG_ERROR("Cannot untype a filled object. Empty it first.");
        return;
    }
    if (obj->cid == CID_Untyped) {
        LOG_ERROR("Object is already untyped.");
        return;
    }
    ObjectContainer_UntypeEmptyTyped(ObjectContainer_TempFrom(obj));
}

// Empties and untypes in one call. Refs stay valid, object becomes a ghost.
//   Object_EmptyAndUntype(obj);
static inline void Object_EmptyAndUntypeFilledType(ExternalReference obj) {
    if (obj == NULL) {
        LOG_ERROR("Cannot empty+untype NULL object.");
        return;
    }
    if (obj->data != NULL) {
        ObjectContainer_EmptyFilledTyped(ObjectContainer_TempFrom(obj));
    }
    if (obj->cid != CID_Untyped) {
        ObjectContainer_UntypeEmptyTyped(ObjectContainer_TempFrom(obj));
    }
}

// Create and immediately take an external reference. Returns ExternalReference (external_refs = 1).
//   ExternalReference ref = Object_CreateRef(CID_Exploder);
static inline ExternalReference Object_CreateRef(ClassID cid) {
    TempObjectReference obj = Object_Create(cid);
    if (obj == NULL) return NULL;
    return ObjectContainer_ExternalRef_From_Temp(obj);
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

// ---- Self reference macros ----

// Store an internal ref from an ExternalReference. Increments target's internal_refs.
//   Self_RefFrom("target", some_external_ref);
#define Self_RefFrom(str_key, ext_ref) do { \
    ObjectReference _sr_ref = ObjectContainer_InternalRef_From_External(ext_ref); \
    UnsafeHashMap_SSet(Self_Refs, str_key, &_sr_ref); \
} while (0)

// Store an internal ref from a TempObjectReference. Increments target's internal_refs.
//   Self_RefFromTemp("target", some_temp);
#define Self_RefFromTemp(str_key, tref) do { \
    ObjectReference _sr_ref = ObjectContainer_InternalRef_From_Temp(tref); \
    UnsafeHashMap_SSet(Self_Refs, str_key, &_sr_ref); \
} while (0)

// Get a stored reference as TempObjectReference (borrowed, no refcount change).
//   TempObjectReference t = Self_GetRef("target");
#define Self_GetRef(str_key) \
    ObjectContainer_TempFrom(*(ObjectReference*)UnsafeHashMap_SGet(Self_Refs, str_key))

// Check if Self holds a reference by key.
//   if (Self_HasRef("target")) { ... }
#define Self_HasRef(str_key) \
    UnsafeHashMap_SHas(Self_Refs, str_key)

// Check if a stored reference points back to Self.
//   if (Self_IsRefSelf("left")) { ... }
#define Self_IsRefSelf(str_key) \
    (Self_HasRef(str_key) && Self_GetRef(str_key) == Self)

// ---- Direct reference helpers (for code outside SELF handlers) ----

// Store a ref to target in obj's references hashmap. Creates an internal ref.
//   Object_StoreRef(obj, "child", some_temp_or_external);
static inline void Object_StoreRef(TempObjectReference obj, const char *key, uint32_t key_len, TempObjectReference target) {
    if (obj == NULL || obj->data == NULL || target == NULL) return;
    ObjectReference iref = ObjectContainer_InternalRef_From_Temp(target);
    UnsafeHashMap_Set(obj->data->references, key, key_len, &iref);
}

#define Object_SStoreRef(obj, str_key, target) \
    Object_StoreRef(obj, str_key, _UNSAFE_STRLITERAL_LEN(str_key), target)

// Get a ref from obj's references hashmap as TempObjectReference.
static inline TempObjectReference Object_GetRef(TempObjectReference obj, const char *key, uint32_t key_len) {
    if (obj == NULL || obj->data == NULL) return NULL;
    void *ptr = UnsafeHashMap_Get(obj->data->references, key, key_len);
    if (ptr == NULL) return NULL;
    return ObjectContainer_TempFrom(*(ObjectReference*)ptr);
}

#define Object_SGetRef(obj, str_key) \
    Object_GetRef(obj, str_key, _UNSAFE_STRLITERAL_LEN(str_key))
