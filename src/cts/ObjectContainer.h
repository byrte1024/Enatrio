#pragma once

// ObjectContainer lifecycle functions.
// Included by Self.h -- do not include directly.

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
        LOG_ERROR("Failed to malloc ObjectContainer");
        return NULL;
    }

    container->data = NULL;
    container->cid = CID_Untyped;
    container->internal_refs = 0;
    container->external_refs = 0;
    return container;
}

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
        LOG_ERROR("Failed to malloc ObjectData");
        return;
    }
    container->data->values = UnsafeVariedHashMap_Create(8);
    if(container->data->values == NULL){
        LOG_ERROR("Failed to malloc values hashmap");
        free(container->data);
        container->data = NULL;
        return;
    }
    container->data->references = UnsafeHashMap_Create(sizeof(ObjectReference),8);
    if(container->data->references == NULL){
        LOG_ERROR("Failed to malloc references hashmap");
        UnsafeVariedHashMap_Destroy(container->data->values);
        free(container->data);
        container->data = NULL;
        return;
    }

    MessagePayload payload = PrepareSelfPayload(container, MID_Default_SELF_Create);
    if(payload.data == NULL){
        LOG_ERROR("Failed to prepare SELF_Create payload");
        UnsafeVariedHashMap_Destroy(container->data->values);
        UnsafeHashMap_Destroy(container->data->references);
        free(container->data);
        container->data = NULL;
        return;
    }
    DispatchMessage(&payload);
    if(!MESSAGE_RESULT_ISOK(payload.result)){
        LOG_ERROR("SELF_Create failed");
        UnsafeVariedHashMap_Destroy(container->data->values);
        UnsafeHashMap_Destroy(container->data->references);
        free(container->data);
        container->data = NULL;
        return;
    }
    FreePayload(&payload);
}

// Forward declaration
static inline void ObjectContainer_UnRef_Internal(ObjectReference* ref);

static void _ObjectContainer_UnRefEach(const void *key, uint32_t key_len, void *value) {
    (void)key; (void)key_len;
    ObjectReference *ref = (ObjectReference *)value;
    if (ref != NULL && *ref != NULL) {
        ObjectContainer_UnRef_Internal(ref);
    }
}

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
        LOG_ERROR("ObjectContainer is typed (%s) but does not support SELF_Destroy, cannot empty.", CLASSID_TOSTRING(container->cid));
        return;
    }

    MessagePayload payload = PrepareSelfPayload(container, MID_Default_SELF_Destroy);
    if(payload.data == NULL){
        LOG_ERROR("Failed to prepare SELF_Destroy payload");
        return;
    }
    DispatchMessage(&payload);
    if(!MESSAGE_RESULT_ISOK(payload.result)){
        LOG_ERROR("SELF_Destroy failed");
        return;
    }
    FreePayload(&payload);

    // Detach data BEFORE unreffing held references (prevents re-entrant empty on self-ref)
    ObjectData *data = container->data;
    container->data = NULL;

    UnsafeVariedHashMap_Destroy(data->values);
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
        LOG_ERROR("ObjectContainer is already untyped, cannot untype");
        return;
    }
    if(container->data != NULL){
        LOG_ERROR("ObjectContainer is full, cannot untype");
        return;
    }

    container->cid = CID_Untyped;
}
