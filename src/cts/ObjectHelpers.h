#pragma once

// Object helper functions and Self_ macros.
// Included by Self.h -- do not include directly.

// ============================================================
// Object lifecycle helpers
// ============================================================

static inline TempObjectReference Object_Create(ClassID cid) {
    TempObjectReference obj = ObjectContainer_CreateGhost();
    if (obj == NULL) return NULL;
    ObjectContainer_TypeEmptyUntyped(obj, cid);
    ObjectContainer_FillEmptyTyped(obj);
    if (obj->data == NULL) {
        ObjectContainer_UntypeEmptyTyped(obj);
        ObjectContainer_DestroyGhost(obj);
        return NULL;
    }
    return obj;
}

static inline void Object_Destroy(TempObjectReference obj) {
    if (obj == NULL) { LOG_ERROR("Cannot destroy NULL object."); return; }
    if (ObjectContainer_TotalRefs(obj) != 0) {
        LOG_ERROR("Cannot destroy object with refs (internal=%d, external=%d).", obj->internal_refs, obj->external_refs);
        return;
    }
    if (obj->data != NULL) ObjectContainer_EmptyFilledTyped(obj);
    if (obj->cid != CID_Untyped) ObjectContainer_UntypeEmptyTyped(obj);
    ObjectContainer_DestroyGhost(obj);
}

static inline void Object_EmptyFilledType(ExternalReference obj) {
    if (obj == NULL) { LOG_ERROR("Cannot empty NULL object."); return; }
    if (obj->data == NULL) { LOG_ERROR("Object is already empty."); return; }
    ObjectContainer_EmptyFilledTyped(ObjectContainer_TempFrom(obj));
}

static inline void Object_UntypeEmptyTyped(ExternalReference obj) {
    if (obj == NULL) { LOG_ERROR("Cannot untype NULL object."); return; }
    if (obj->data != NULL) { LOG_ERROR("Cannot untype a filled object. Empty it first."); return; }
    if (obj->cid == CID_Untyped) { LOG_ERROR("Object is already untyped."); return; }
    ObjectContainer_UntypeEmptyTyped(ObjectContainer_TempFrom(obj));
}

static inline void Object_EmptyAndUntypeFilledType(ExternalReference obj) {
    if (obj == NULL) { LOG_ERROR("Cannot empty+untype NULL object."); return; }
    if (obj->data != NULL) ObjectContainer_EmptyFilledTyped(ObjectContainer_TempFrom(obj));
    if (obj->cid != CID_Untyped) ObjectContainer_UntypeEmptyTyped(ObjectContainer_TempFrom(obj));
}

static inline ExternalReference Object_CreateRef(ClassID cid) {
    TempObjectReference obj = Object_Create(cid);
    if (obj == NULL) return NULL;
    return ObjectContainer_ExternalRef_From_Temp(obj);
}

// ============================================================
// Self_ value macros (for use inside SELF handlers)
// ============================================================

#define Self_Values (Self->data->values)
#define Self_Refs (Self->data->references)

#define Self_Get(str_key, type) \
    ((type*)UnsafeVariedHashMap_SGet(Self_Values, str_key))

#define Self_GetDeref(str_key, type) \
    UnsafeVariedHashMap_SGetDeref(Self_Values, str_key, type)

#define Self_SetValue(str_key, type, value) \
    UnsafeVariedHashMap_SSetValue(Self_Values, str_key, type, value)

#define Self_Set(str_key, value_ptr, value_size) \
    UnsafeVariedHashMap_SSet(Self_Values, str_key, value_ptr, value_size)

#define Self_Has(str_key) \
    UnsafeVariedHashMap_SHas(Self_Values, str_key)

// ============================================================
// Self_ reference macros
// ============================================================

#define Self_RefFrom(str_key, ext_ref) do { \
    ObjectReference _sr_ref = ObjectContainer_InternalRef_From_External(ext_ref); \
    UnsafeHashMap_SSet(Self_Refs, str_key, &_sr_ref); \
} while (0)

#define Self_RefFromTemp(str_key, tref) do { \
    ObjectReference _sr_ref = ObjectContainer_InternalRef_From_Temp(tref); \
    UnsafeHashMap_SSet(Self_Refs, str_key, &_sr_ref); \
} while (0)

#define Self_GetRef(str_key) \
    ObjectContainer_TempFrom(*(ObjectReference*)UnsafeHashMap_SGet(Self_Refs, str_key))

#define Self_HasRef(str_key) \
    UnsafeHashMap_SHas(Self_Refs, str_key)

#define Self_IsRefSelf(str_key) \
    (Self_HasRef(str_key) && Self_GetRef(str_key) == Self)

// ============================================================
// Direct reference helpers (outside SELF handlers)
// ============================================================

static inline void Object_StoreRef(TempObjectReference obj, const char *key, uint32_t key_len, TempObjectReference target) {
    if (obj == NULL || obj->data == NULL || target == NULL) return;
    ObjectReference iref = ObjectContainer_InternalRef_From_Temp(target);
    UnsafeHashMap_Set(obj->data->references, key, key_len, &iref);
}

#define Object_SStoreRef(obj, str_key, target) \
    Object_StoreRef(obj, str_key, _UNSAFE_STRLITERAL_LEN(str_key), target)

static inline TempObjectReference Object_GetRef(TempObjectReference obj, const char *key, uint32_t key_len) {
    if (obj == NULL || obj->data == NULL) return NULL;
    void *ptr = UnsafeHashMap_Get(obj->data->references, key, key_len);
    if (ptr == NULL) return NULL;
    return ObjectContainer_TempFrom(*(ObjectReference*)ptr);
}

#define Object_SGetRef(obj, str_key) \
    Object_GetRef(obj, str_key, _UNSAFE_STRLITERAL_LEN(str_key))
