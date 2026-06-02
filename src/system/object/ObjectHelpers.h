#pragma once

#include "ObjectRef.h"

// ============================================================
// Value storage helpers -- prepend ObjectValueHeader to every value
// ============================================================

#define _OBJECT_VALUE_MAX_SIZE (64u * 1024u)

static int _Object_StoreValue(UnsafeVariedHashMap *map, const void *key, uint32_t key_len,
                               const void *value, uint32_t value_size,
                               ClassID owner, uint16_t ser_id, uint16_t ser_arg) {
    if (value_size > _OBJECT_VALUE_MAX_SIZE) return -1;
    uint32_t total = (uint32_t)sizeof(ObjectValueHeader) + value_size;
    uint8_t *buf = (uint8_t *)malloc(total);
    if (!buf) return -1;
    ObjectValueHeader *hdr = (ObjectValueHeader *)buf;
    hdr->owner = owner;
    hdr->ser_id = ser_id;
    hdr->ser_arg = ser_arg;
    memcpy(buf + sizeof(ObjectValueHeader), value, value_size);
    UnsafeVariedHashMap_Remove(map, key, key_len);
    int result = UnsafeVariedHashMap_Set(map, key, key_len, buf, total);
    free(buf);
    return result;
}

static void *_Object_GetValueData(UnsafeVariedHashMap *map, const void *key, uint32_t key_len) {
    void *ptr = UnsafeVariedHashMap_Get(map, key, key_len);
    if (!ptr) return NULL;
    return (uint8_t *)ptr + sizeof(ObjectValueHeader);
}

static ObjectValueHeader *_Object_GetValueHeader(UnsafeVariedHashMap *map, const void *key, uint32_t key_len) {
    return (ObjectValueHeader *)UnsafeVariedHashMap_Get(map, key, key_len);
}

static int _Object_HasValue(UnsafeVariedHashMap *map, const void *key, uint32_t key_len) {
    return UnsafeVariedHashMap_Has(map, key, key_len);
}

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

#define Self_SetValue(str_key, type, value) \
    _Object_StoreValue(Self_Values, str_key, _UNSAFE_STRLITERAL_LEN(str_key), \
                       &(type){value}, sizeof(type), \
                       BAT2(CID_, TYPE), SER_RAW, 0)

#define Self_Set(str_key, value_ptr, value_size) \
    _Object_StoreValue(Self_Values, str_key, _UNSAFE_STRLITERAL_LEN(str_key), \
                       value_ptr, value_size, \
                       BAT2(CID_, TYPE), SER_RAW, 0)

#define Self_SetTransient(str_key, type, value) \
    _Object_StoreValue(Self_Values, str_key, _UNSAFE_STRLITERAL_LEN(str_key), \
                       &(type){value}, sizeof(type), \
                       BAT2(CID_, TYPE), SER_SKIP, 0)

#define Self_SetDeref(str_key, type, value, pointto_size) \
    _Object_StoreValue(Self_Values, str_key, _UNSAFE_STRLITERAL_LEN(str_key), \
                       &(type){value}, sizeof(type), \
                       BAT2(CID_, TYPE), SER_DEREF, (uint16_t)(pointto_size))

#define Self_SetCustom(str_key, type, value, custom_ser_id, custom_arg) \
    _Object_StoreValue(Self_Values, str_key, _UNSAFE_STRLITERAL_LEN(str_key), \
                       &(type){value}, sizeof(type), \
                       BAT2(CID_, TYPE), (uint16_t)(custom_ser_id), (uint16_t)(custom_arg))

#define Self_Get(str_key, type) \
    ((type*)_Object_GetValueData(Self_Values, str_key, _UNSAFE_STRLITERAL_LEN(str_key)))

#define Self_GetDeref(str_key, type) ({ \
    void *_sgd_ptr = _Object_GetValueData(Self_Values, str_key, _UNSAFE_STRLITERAL_LEN(str_key)); \
    _sgd_ptr ? *(type*)_sgd_ptr : (type){0}; \
})

#define Self_Has(str_key) \
    _Object_HasValue(Self_Values, str_key, _UNSAFE_STRLITERAL_LEN(str_key))

#define Self_GetHeader(str_key) \
    _Object_GetValueHeader(Self_Values, str_key, _UNSAFE_STRLITERAL_LEN(str_key))

// ============================================================
// Self_ reference macros
// ============================================================

#define Self_RefFrom(str_key, ext_ref) do { \
    if (UnsafeHashMap_SHas(Self_Refs, str_key)) { \
        ObjectReference *_old = (ObjectReference*)UnsafeHashMap_SGet(Self_Refs, str_key); \
        ObjectContainer_UnRef_Internal(_old); \
        UnsafeHashMap_SRemove(Self_Refs, str_key); \
    } \
    ObjectReference _sr_ref = ObjectContainer_InternalRef_From_External(ext_ref); \
    UnsafeHashMap_SSet(Self_Refs, str_key, &_sr_ref); \
} while (0)

#define Self_RefFromTemp(str_key, tref) do { \
    if (UnsafeHashMap_SHas(Self_Refs, str_key)) { \
        ObjectReference *_old = (ObjectReference*)UnsafeHashMap_SGet(Self_Refs, str_key); \
        ObjectContainer_UnRef_Internal(_old); \
        UnsafeHashMap_SRemove(Self_Refs, str_key); \
    } \
    ObjectReference _sr_ref = ObjectContainer_InternalRef_From_Temp(tref); \
    UnsafeHashMap_SSet(Self_Refs, str_key, &_sr_ref); \
} while (0)

#define Self_GetRef(str_key) \
    (Self_HasRef(str_key) ? ObjectContainer_TempFrom(*(ObjectReference*)UnsafeHashMap_SGet(Self_Refs, str_key)) : NULL)

#define Self_HasRef(str_key) \
    UnsafeHashMap_SHas(Self_Refs, str_key)

#define Self_IsRefSelf(str_key) \
    (Self_HasRef(str_key) && Self_GetRef(str_key) == Self)

// ============================================================
// Direct reference helpers (outside SELF handlers)
// ============================================================

static inline void Object_StoreRef(TempObjectReference obj, const char *key, uint32_t key_len, TempObjectReference target) {
    if (obj == NULL || obj->data == NULL || target == NULL) return;
    if (UnsafeHashMap_Has(obj->data->references, key, key_len)) {
        ObjectReference *old = (ObjectReference*)UnsafeHashMap_Get(obj->data->references, key, key_len);
        ObjectContainer_UnRef_Internal(old);
        UnsafeHashMap_Remove(obj->data->references, key, key_len);
    }
    ObjectReference iref = ObjectContainer_InternalRef_From_Temp(target);
    UnsafeHashMap_Set(obj->data->references, key, key_len, &iref);
}

#define Object_SStoreRef(obj, str_key, target) \
    Object_StoreRef(obj, str_key, _UNSAFE_STRLITERAL_LEN(str_key), target)

static inline void Object_RemoveRef(TempObjectReference obj, const char *key, uint32_t key_len) {
    if (obj == NULL || obj->data == NULL) return;
    if (UnsafeHashMap_Has(obj->data->references, key, key_len)) {
        ObjectReference *old = (ObjectReference*)UnsafeHashMap_Get(obj->data->references, key, key_len);
        ObjectContainer_UnRef_Internal(old);
        UnsafeHashMap_Remove(obj->data->references, key, key_len);
    }
}

#define Object_SRemoveRef(obj, str_key) \
    Object_RemoveRef(obj, str_key, _UNSAFE_STRLITERAL_LEN(str_key))

static inline TempObjectReference Object_GetRef(TempObjectReference obj, const char *key, uint32_t key_len) {
    if (obj == NULL || obj->data == NULL) return NULL;
    void *ptr = UnsafeHashMap_Get(obj->data->references, key, key_len);
    if (ptr == NULL) return NULL;
    return ObjectContainer_TempFrom(*(ObjectReference*)ptr);
}

#define Object_SGetRef(obj, str_key) \
    Object_GetRef(obj, str_key, _UNSAFE_STRLITERAL_LEN(str_key))

// ============================================================
// Singleton pattern macros
//
// Use AFTER CLASSDEF() and #undef TYPE. Provides:
//   - ClassName_CreateSingleton()  -> ExternalReference (creates the one instance)
//   - ClassName_DestroySingleton() -> void (unrefs and NULLs)
//   - ClassName_HasSingleton()     -> int (1 if exists)
//   - GET_SINGLETON(ClassName)     -> TempObjectReference (borrowed ref)
//
// Example:
//   CLASSDEF()
//   #undef TYPE
//   DECLARE_SINGLETON(Window)
//
//   // usage:
//   Window_CreateSingleton();
//   TempObjectReference win = GET_SINGLETON(Window);
//   Window_DestroySingleton();
// ============================================================

#define LINTNORE

#define DECLARE_SINGLETON(classname) \
    static ExternalReference _##classname##_singleton = NULL; \
    \
    static ExternalReference classname##_CreateSingleton(void) { \
        if (_##classname##_singleton != NULL) { \
            LOG_ERROR(#classname " singleton already exists"); \
            return _##classname##_singleton; \
        } \
        _##classname##_singleton = Object_CreateRef(CID_##classname); \
        if (_##classname##_singleton == NULL) { \
            LOG_ERROR("Failed to create " #classname " singleton"); \
        } \
        return _##classname##_singleton; \
    } \
    \
    static void classname##_DestroySingleton(void) { \
        if (_##classname##_singleton == NULL) { \
            LOG_ERROR("No " #classname " singleton to destroy"); \
            return; \
        } \
        ObjectContainer_UnRef_External(&_##classname##_singleton); \
    } \
    \
    static int classname##_HasSingleton(void) { \
        return _##classname##_singleton != NULL; \
    }

#define GET_SINGLETON(classname) \
    (ObjectContainer_TempFrom(_##classname##_singleton))

#undef LINTNORE
