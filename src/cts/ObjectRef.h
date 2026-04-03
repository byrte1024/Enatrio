#pragma once

// Reference conversions, UnRef, and cycle GC.
// Included by Self.h -- do not include directly.

// ============================================================
// Reference conversions
// ============================================================

#define ObjectContainer_TempFrom(ref) ((TempObjectReference)(ref))

// ---- Internal refs (ObjectReference) -- only for reference hashmaps ----

static inline ObjectReference ObjectContainer_InternalRef_From_Temp(TempObjectReference tref) {
    if (tref != NULL) tref->internal_refs++;
    return (ObjectReference)tref;
}

static inline ObjectReference ObjectContainer_InternalRef_From_Internal(ObjectReference ref) {
    if (ref != NULL) ref->internal_refs++;
    return ref;
}

static inline ObjectReference ObjectContainer_InternalRef_From_External(ExternalReference ref) {
    if (ref != NULL) ref->internal_refs++;
    return (ObjectReference)ref;
}

// ---- External refs (ExternalReference) -- for stack/globals ----

static inline ExternalReference ObjectContainer_ExternalRef_From_Temp(TempObjectReference tref) {
    if (tref != NULL) tref->external_refs++;
    return (ExternalReference)tref;
}

static inline ExternalReference ObjectContainer_ExternalRef_From_External(ExternalReference ref) {
    if (ref != NULL) ref->external_refs++;
    return ref;
}

static inline ExternalReference ObjectContainer_ExternalRef_From_Internal(ObjectReference ref) {
    if (ref != NULL) ref->external_refs++;
    return (ExternalReference)ref;
}

// ============================================================
// UnRef
// ============================================================

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

// ============================================================
// Cycle collection
// ============================================================

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

    while (idx < worklist->count) {
        TempObjectReference node = *(TempObjectReference*)UnsafeArray_Get(worklist, idx);
        idx++;
        if (node->external_refs > 0) { has_external = 1; break; }
        _gc_add_neighbors(worklist, visited, node);
    }

    if (!has_external) {
        // Pass 1: dispatch SELF_Destroy (proper cleanup)
        for (uint32_t i = 0; i < visited->count; i++) {
            TempObjectReference node = *(TempObjectReference*)UnsafeArray_Get(visited, i);
            if (node->data != NULL && CanDispatchMessage(MID_Default_SELF_Destroy, node->cid)) {
                MessagePayload p = PrepareSelfPayload(node, MID_Default_SELF_Destroy);
                if (p.data != NULL) { DispatchMessage(&p); FreePayload(&p); }
            }
        }
        // Pass 2: tear down data
        for (uint32_t i = 0; i < visited->count; i++) {
            TempObjectReference node = *(TempObjectReference*)UnsafeArray_Get(visited, i);
            if (node->data != NULL) {
                UnsafeVariedHashMap_Destroy(node->data->values);
                UnsafeHashMap_Destroy(node->data->references);
                free(node->data);
                node->data = NULL;
            }
        }
        // Pass 3: free containers
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
        _ObjectContainer_TryCollectCycle(obj);
    }
}
