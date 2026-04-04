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
        _ObjectRegistry_Unregister(obj);
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

static int _gc_is_registered(TempObjectReference target) {
    if (_object_registry == NULL) return 0;
    for (uint32_t i = 0; i < _object_registry->count; i++) {
        if (*(TempObjectReference*)UnsafeArray_Get(_object_registry, i) == target) return 1;
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
        // Skip targets that were already freed (stale refs from previous collections)
        if (!_gc_is_registered(target)) continue;
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
        // Collect external neighbors that are NOT in the collected set.
        // After we free our component, these neighbors lose internal refs
        // and may become orphaned candidates for follow-up GC.
        UnsafeArray *orphan_candidates = UnsafeArray_Create(sizeof(TempObjectReference), 8);

        // Pass 1: dispatch SELF_Destroy (proper cleanup)
        for (uint32_t i = 0; i < visited->count; i++) {
            TempObjectReference node = *(TempObjectReference*)UnsafeArray_Get(visited, i);
            if (node->data != NULL && CanDispatchMessage(MID_Default_SELF_Destroy, node->cid)) {
                MessagePayload p = PrepareSelfPayload(node, MID_Default_SELF_Destroy);
                if (p.data != NULL) { DispatchMessage(&p); FreePayload(&p); }
            }
        }
        // Pass 2: tear down data. Before destroying refs, scan for external targets.
        for (uint32_t i = 0; i < visited->count; i++) {
            TempObjectReference node = *(TempObjectReference*)UnsafeArray_Get(visited, i);
            if (node->data != NULL) {
                // Scan references for targets outside our collected set
                UnsafeHashMap *refs = node->data->references;
                for (uint32_t b = 0; b < refs->bucket_count; b++) {
                    UnsafeHashEntry *e = &refs->buckets[b];
                    if (e->value < 0) continue;
                    ObjectReference *rp = (ObjectReference*)UnsafeArray_Get(refs->values, (uint32_t)e->value);
                    if (rp == NULL || *rp == NULL) continue;
                    TempObjectReference target = ObjectContainer_TempFrom(*rp);
                    if (!_gc_is_registered(target)) continue; // stale ref
                    // If target is NOT in our collected set, it's an external neighbor
                    if (!_gc_is_visited(visited, target)) {
                        // Decrement its internal ref (we're about to destroy the ref holder)
                        target->internal_refs--;
                        // If it now has 0 externals and only internals, it's a candidate
                        if (target->external_refs <= 0 && ObjectContainer_TotalRefs(target) > 0) {
                            if (!_gc_is_visited(orphan_candidates, target)) {
                                UnsafeArray_Add(orphan_candidates, &target);
                            }
                        } else if (ObjectContainer_TotalRefs(target) <= 0) {
                            // Target has no refs at all now -- destroy it directly
                            if (target->data != NULL) ObjectContainer_EmptyFilledTyped(target);
                            if (target->cid != CID_Untyped) ObjectContainer_UntypeEmptyTyped(target);
                            _ObjectRegistry_Unregister(target);
                            free(target);
                        }
                    }
                }
                UnsafeVariedHashMap_Destroy(node->data->values);
                UnsafeHashMap_Destroy(node->data->references);
                free(node->data);
                node->data = NULL;
            }
        }
        // Pass 3: unregister and free containers
        for (uint32_t i = 0; i < visited->count; i++) {
            TempObjectReference node = *(TempObjectReference*)UnsafeArray_Get(visited, i);
            node->internal_refs = 0;
            node->external_refs = 0;
            node->cid = CID_Untyped;
            _ObjectRegistry_Unregister(node);
            free(node);
        }

        // Pass 4: follow-up GC on orphaned neighbors
        UnsafeArray_Destroy(worklist);
        UnsafeArray_Destroy(visited);
        _gc_running = false;

        for (uint32_t i = 0; i < orphan_candidates->count; i++) {
            TempObjectReference candidate = *(TempObjectReference*)UnsafeArray_Get(orphan_candidates, i);
            // Must verify candidate is still alive in registry (could have been freed
            // at line 162-168 or by a previous iteration's collection)
            if (!_gc_is_registered(candidate)) continue;
            if (candidate->data != NULL && candidate->external_refs <= 0 && candidate->internal_refs > 0) {
                _ObjectContainer_TryCollectCycle(candidate);
            }
        }
        UnsafeArray_Destroy(orphan_candidates);
        return; // already cleaned up worklist/visited
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
        _ObjectRegistry_Unregister(obj);
        free(obj);
    } else if (obj->external_refs <= 0 && obj->internal_refs > 0 && !_gc_running) {
        _ObjectContainer_TryCollectCycle(obj);
    }
}

// ============================================================
// Global garbage collection sweep
// ============================================================

// Sweep all registered objects. Collect any component with 0 external refs.
static void Object_GarbageCollect(void) {
    if (_object_registry == NULL) return;
    if (_gc_running) return;

    // Find objects with 0 externals that might be part of orphaned cycles.
    // We iterate backwards since _TryCollectCycle may unregister entries.
    // Use a snapshot of candidates first to avoid modifying the array during iteration.
    UnsafeArray *candidates = UnsafeArray_Create(sizeof(TempObjectReference), 16);
    for (uint32_t i = 0; i < _object_registry->count; i++) {
        TempObjectReference obj = *(TempObjectReference*)UnsafeArray_Get(_object_registry, i);
        if (obj->external_refs <= 0 && obj->internal_refs > 0 && obj->data != NULL) {
            UnsafeArray_Add(candidates, &obj);
        }
    }

    for (uint32_t i = 0; i < candidates->count; i++) {
        TempObjectReference obj = *(TempObjectReference*)UnsafeArray_Get(candidates, i);
        // Re-check: still alive, still a candidate? (previous collections may have freed it)
        // Check if still in registry
        int still_alive = 0;
        if (_object_registry != NULL) {
            for (uint32_t j = 0; j < _object_registry->count; j++) {
                if (*(TempObjectReference*)UnsafeArray_Get(_object_registry, j) == obj) {
                    still_alive = 1;
                    break;
                }
            }
        }
        if (still_alive && obj->external_refs <= 0 && obj->internal_refs > 0 && obj->data != NULL) {
            _ObjectContainer_TryCollectCycle(obj);
        }
    }

    UnsafeArray_Destroy(candidates);
}
