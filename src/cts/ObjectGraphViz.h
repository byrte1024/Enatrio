#pragma once

#include "Self.h"

// ============================================================
// Graph visualizer -- saves ASCII reference graph to file
// ============================================================

static int _viz_find_id(UnsafeArray *nodes, TempObjectReference target) {
    for (uint32_t i = 0; i < nodes->count; i++) {
        if (*(TempObjectReference*)UnsafeArray_Get(nodes, i) == target) return (int)i;
    }
    return -1;
}

static void Object_VisualizeGraph(const char *filepath, TempObjectReference *roots, int root_count) {
    if (filepath == NULL || roots == NULL || root_count <= 0) return;

    UnsafeArray *nodes = UnsafeArray_Create(sizeof(TempObjectReference), 32);
    UnsafeArray *worklist = UnsafeArray_Create(sizeof(TempObjectReference), 32);

    for (int i = 0; i < root_count; i++) {
        if (roots[i] == NULL) continue;
        if (_viz_find_id(nodes, roots[i]) >= 0) continue;
        UnsafeArray_Add(nodes, &roots[i]);
        UnsafeArray_Add(worklist, &roots[i]);
    }

    uint32_t idx = 0;
    while (idx < worklist->count) {
        TempObjectReference node = *(TempObjectReference*)UnsafeArray_Get(worklist, idx);
        idx++;
        if (node->data == NULL || node->data->references == NULL) continue;
        UnsafeHashMap *refs = node->data->references;
        for (uint32_t b = 0; b < refs->bucket_count; b++) {
            UnsafeHashEntry *e = &refs->buckets[b];
            if (e->value < 0) continue;
            ObjectReference *ref_ptr = (ObjectReference*)UnsafeArray_Get(refs->values, (uint32_t)e->value);
            if (ref_ptr == NULL || *ref_ptr == NULL) continue;
            TempObjectReference target = ObjectContainer_TempFrom(*ref_ptr);
            if (_viz_find_id(nodes, target) < 0) {
                UnsafeArray_Add(nodes, &target);
                UnsafeArray_Add(worklist, &target);
            }
        }
    }

    FILE *f = fopen(filepath, "w");
    if (f == NULL) {
        LOG_ERROR("Failed to open %s for graph visualization", filepath);
        UnsafeArray_Destroy(nodes);
        UnsafeArray_Destroy(worklist);
        return;
    }

    fprintf(f, "=== Object Reference Graph (%u nodes) ===\n\n", nodes->count);

    for (uint32_t i = 0; i < nodes->count; i++) {
        TempObjectReference node = *(TempObjectReference*)UnsafeArray_Get(nodes, i);

        const char *state = "GHOST";
        if (node->cid != CID_Untyped && node->data != NULL) state = "FILLED";
        else if (node->cid != CID_Untyped) state = "EMPTY";

        int is_root = 0;
        for (int r = 0; r < root_count; r++) {
            if (roots[r] == node) { is_root = 1; break; }
        }

        fprintf(f, "[%u] %s (CID=0x%04X) ext=%d int=%d %s%s\n",
            i,
            (node->cid != CID_Untyped) ? CLASSID_TOSTRING(node->cid) : "Untyped",
            node->cid,
            node->external_refs,
            node->internal_refs,
            state,
            is_root ? " *ROOT*" : "");

        if (node->data != NULL && node->data->references != NULL) {
            UnsafeHashMap *refs = node->data->references;
            int has_refs = 0;
            for (uint32_t b = 0; b < refs->bucket_count; b++) {
                UnsafeHashEntry *e = &refs->buckets[b];
                if (e->value < 0) continue;
                has_refs = 1;
                ObjectReference *ref_ptr = (ObjectReference*)UnsafeArray_Get(refs->values, (uint32_t)e->value);
                TempObjectReference target = (ref_ptr && *ref_ptr) ? ObjectContainer_TempFrom(*ref_ptr) : NULL;
                int target_id = target ? _viz_find_id(nodes, target) : -1;

                int is_string = 1;
                for (uint32_t k = 0; k < e->key_len; k++) {
                    unsigned char ch = ((unsigned char*)e->key)[k];
                    if (ch < 32 || ch > 126) { is_string = 0; break; }
                }

                fprintf(f, "  ");
                if (is_string)
                    fprintf(f, "\"%.*s\"", (int)e->key_len, (char*)e->key);
                else {
                    fprintf(f, "[");
                    for (uint32_t k = 0; k < e->key_len; k++) {
                        if (k > 0) fprintf(f, " ");
                        fprintf(f, "%02X", ((unsigned char*)e->key)[k]);
                    }
                    fprintf(f, "]");
                }

                if (target == NULL)
                    fprintf(f, " -> NULL\n");
                else if (target == node)
                    fprintf(f, " -> [%d] (SELF)\n", target_id);
                else
                    fprintf(f, " -> [%d]%s\n", target_id, (target_id >= 0 && (uint32_t)target_id <= i) ? " (BACK-REF)" : "");
            }
            if (!has_refs) fprintf(f, "  (no refs)\n");
        } else {
            fprintf(f, "  (no data)\n");
        }

        if (node->data != NULL && node->data->values != NULL)
            fprintf(f, "  values: %u entries\n", node->data->values->entry_count);

        fprintf(f, "\n");
    }

    int total_ext = 0, total_int = 0;
    for (uint32_t i = 0; i < nodes->count; i++) {
        TempObjectReference node = *(TempObjectReference*)UnsafeArray_Get(nodes, i);
        total_ext += node->external_refs;
        total_int += node->internal_refs;
    }
    fprintf(f, "--- Summary ---\n");
    fprintf(f, "Nodes: %u\n", nodes->count);
    fprintf(f, "Total external refs: %d\n", total_ext);
    fprintf(f, "Total internal refs: %d\n", total_int);
    fprintf(f, "Roots: %d\n", root_count);

    fclose(f);
    UnsafeArray_Destroy(nodes);
    UnsafeArray_Destroy(worklist);
}

#define Object_VisualizeGraphSingle(filepath, root) do { \
    TempObjectReference _vr = (TempObjectReference)(root); \
    Object_VisualizeGraph(filepath, &_vr, 1); \
} while (0)
