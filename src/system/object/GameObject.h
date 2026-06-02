#pragma once

#include "ObjectTypes.h"

// ============================================================
// GameObject constants
// ============================================================

#define SPREAD_DOWN 0
#define SPREAD_UP   1
#define _GO_CONSUMED_KEY "__go_consumed__"

// SPREAD_CONSUME stops propagation into the current node's subtree.
// Only effective during SPREAD_DOWN. In SPREAD_UP, children are
// visited before self, so there is nothing left to skip.
#define SPREAD_CONSUME(payload) Payload_SetValue(payload, _GO_CONSUMED_KEY, int, 1)

#define _GO_CHILD_KEY_MAX 24

static inline uint32_t _go_child_key(char *buf, int index) {
    int len = snprintf(buf, _GO_CHILD_KEY_MAX, "child_%d", index);
    if (len < 0) len = 0;
    return (uint32_t)len;
}

// ============================================================
// Class definition
// ============================================================

#define TYPE GameObject

BEGIN_CLASS(0x0003);
INHERITS(Object);

DECLARE_SELF_MID(SpreadMessage);
DECLARE_SELF_MID(AddChild);
DECLARE_SELF_MID(RemoveChild);
DECLARE_SELF_MID(SetActive);
DECLARE_SELF_MID(SetPriority);
DECLARE_SELF_MID(Update);
DECLARE_SELF_MID(Render);

// ============================================================
// Internal helpers (after MID declarations so they can reference them)
// ============================================================

static inline ClassID _go_find_spread_handler(ClassID cid) {
    ClassID walk = cid;
    while (!CLASSID_ISUNTYPED(walk)) {
        if (ClassDefinitions[walk].CanReceiveMID(MID_GameObject_SELF_SpreadMessage))
            return walk;
        walk = ClassDefinitions[walk].parent_cid;
    }
    return CID_Untyped;
}

#define _GO_SPREAD_STACK_MAX 64

static void _go_spread_to_children(
    TempObjectReference self_node,
    MessagePayload *outer_payload,
    int child_count, int reverse)
{
    if (child_count <= 0) return;

    // Snapshot child pointers so mutations during spread are safe
    TempObjectReference stack_buf[_GO_SPREAD_STACK_MAX];
    TempObjectReference *snapshot = stack_buf;
    if (child_count > _GO_SPREAD_STACK_MAX) {
        snapshot = (TempObjectReference *)malloc(sizeof(TempObjectReference) * child_count);
        if (!snapshot) return;
    }

    // Build snapshot in iteration order
    int snap_count = 0;
    int start = reverse ? child_count - 1 : 0;
    int end   = reverse ? -1 : child_count;
    int step  = reverse ? -1 : 1;
    for (int i = start; i != end; i += step) {
        char kbuf[_GO_CHILD_KEY_MAX];
        uint32_t klen = _go_child_key(kbuf, i);
        TempObjectReference ch = Object_GetRef(self_node, kbuf, klen);
        if (ch != NULL) snapshot[snap_count++] = ch;
    }

    // Iterate snapshot
    for (int i = 0; i < snap_count; i++) {
        TempObjectReference ch = snapshot[i];
        // Child may have been freed by a previous handler in this spread
        if (ch->data == NULL) continue;

        void *ap = _Object_GetValueData(ch->data->values, "active", 6);
        if (ap && *(int *)ap == 0) continue;

        ClassID handler_cid = _go_find_spread_handler(ch->cid);
        if (CLASSID_ISUNTYPED(handler_cid)) continue;

        Payload_SetValue(outer_payload, "Self", TempObjectReference, ch);
        outer_payload->cid_target = ch->cid;
        ClassDefinitions[handler_cid].ReceiveMessage(outer_payload);
    }

    if (snapshot != stack_buf) free(snapshot);
}

// ============================================================
// SELF_Create (extern Object)
// ============================================================

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Create)
    CALL_BASE();
    Self_SetValue("active", int, 1);
    Self_SetValue("child_count", int, 0);
    Self_SetValue("priority", int, 0);
MESSAGE_HANDLER_END()

// ============================================================
// SELF_Destroy (extern Object)
// ============================================================

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Destroy)
    CALL_BASE();
MESSAGE_HANDLER_END()

// ============================================================
// SELF_Update / SELF_Render -- no-op defaults, override in subclasses
// ============================================================

SELF_MESSAGE_HANDLER_BEGIN(Update)
    (void)Self;
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN(Render)
    (void)Self;
MESSAGE_HANDLER_END()

// ============================================================
// SELF_SetActive
// ============================================================

SELF_MESSAGE_HANDLER_BEGIN(SetActive)
    MH_ExtractDeref(active, int);
    Self_SetValue("active", int, active);
MESSAGE_HANDLER_END()

// ============================================================
// SELF_AddChild
// ============================================================

SELF_MESSAGE_HANDLER_BEGIN(AddChild)
    MH_ExtractDeref(child, TempObjectReference);
    int count = Self_GetDeref("child_count", int);

    int child_priority = 0;
    {
        void *pp = _Object_GetValueData(child->data->values, "priority", 8);
        if (pp) child_priority = *(int *)pp;
    }

    int insert_idx = count;
    for (int i = 0; i < count; i++) {
        char kbuf[_GO_CHILD_KEY_MAX];
        uint32_t klen = _go_child_key(kbuf, i);
        TempObjectReference existing = Object_GetRef(Self, kbuf, klen);
        if (existing == NULL) continue;
        void *ep = _Object_GetValueData(existing->data->values, "priority", 8);
        int epri = ep ? *(int *)ep : 0;
        if (epri > child_priority) {
            insert_idx = i;
            break;
        }
    }

    for (int i = count; i > insert_idx; i--) {
        char src_buf[_GO_CHILD_KEY_MAX];
        uint32_t src_len = _go_child_key(src_buf, i - 1);
        TempObjectReference moved = Object_GetRef(Self, src_buf, src_len);

        char dst_buf[_GO_CHILD_KEY_MAX];
        uint32_t dst_len = _go_child_key(dst_buf, i);
        Object_StoreRef(Self, dst_buf, dst_len, moved);
    }

    {
        char kbuf[_GO_CHILD_KEY_MAX];
        uint32_t klen = _go_child_key(kbuf, insert_idx);
        Object_StoreRef(Self, kbuf, klen, child);
    }

    Object_SStoreRef(child, "parent", Self);
    Self_SetValue("child_count", int, count + 1);
MESSAGE_HANDLER_END()

// ============================================================
// SELF_RemoveChild
// ============================================================

SELF_MESSAGE_HANDLER_BEGIN(RemoveChild)
    MH_ExtractDeref(child, TempObjectReference);
    int count = Self_GetDeref("child_count", int);

    int found_idx = -1;
    for (int i = 0; i < count; i++) {
        char kbuf[_GO_CHILD_KEY_MAX];
        uint32_t klen = _go_child_key(kbuf, i);
        TempObjectReference existing = Object_GetRef(Self, kbuf, klen);
        if (existing == child) {
            found_idx = i;
            break;
        }
    }

    if (found_idx < 0) {
        payload->result = MESSAGE_RESULT_NOT_FOUND;
        return;
    }

    // Clear parent ref on child
    if (child->data != NULL && child->data->references != NULL) {
        if (UnsafeHashMap_Has(child->data->references, "parent", 6)) {
            ObjectReference *old = (ObjectReference *)UnsafeHashMap_Get(child->data->references, "parent", 6);
            ObjectContainer_UnRef_Internal(old);
            UnsafeHashMap_Remove(child->data->references, "parent", 6);
        }
    }

    // Remove the child ref at found_idx
    {
        char kbuf[_GO_CHILD_KEY_MAX];
        uint32_t klen = _go_child_key(kbuf, found_idx);
        if (UnsafeHashMap_Has(Self_Refs, kbuf, klen)) {
            ObjectReference *old = (ObjectReference *)UnsafeHashMap_Get(Self_Refs, kbuf, klen);
            ObjectContainer_UnRef_Internal(old);
            UnsafeHashMap_Remove(Self_Refs, kbuf, klen);
        }
    }

    // Shift remaining children down
    for (int i = found_idx; i < count - 1; i++) {
        char src_buf[_GO_CHILD_KEY_MAX];
        uint32_t src_len = _go_child_key(src_buf, i + 1);
        TempObjectReference moved = Object_GetRef(Self, src_buf, src_len);

        char dst_buf[_GO_CHILD_KEY_MAX];
        uint32_t dst_len = _go_child_key(dst_buf, i);
        Object_StoreRef(Self, dst_buf, dst_len, moved);
    }

    // Remove the now-duplicate last slot
    {
        char kbuf[_GO_CHILD_KEY_MAX];
        uint32_t klen = _go_child_key(kbuf, count - 1);
        if (UnsafeHashMap_Has(Self_Refs, kbuf, klen)) {
            ObjectReference *old = (ObjectReference *)UnsafeHashMap_Get(Self_Refs, kbuf, klen);
            ObjectContainer_UnRef_Internal(old);
            UnsafeHashMap_Remove(Self_Refs, kbuf, klen);
        }
    }

    Self_SetValue("child_count", int, count - 1);
MESSAGE_HANDLER_END()

// ============================================================
// SELF_SetPriority
// ============================================================

SELF_MESSAGE_HANDLER_BEGIN(SetPriority)
    MH_ExtractDeref(priority, int);
    Self_SetValue("priority", int, priority);

    TempObjectReference parent = Self_GetRef("parent");
    if (parent != NULL) {
        // Hold a temporary external ref so Self survives the remove+re-add cycle.
        // RemoveChild drops the parent's internal ref, which would free Self
        // if no other refs exist.
        ExternalReference _guard = ObjectContainer_ExternalRef_From_Temp(Self);
        SELF_DISPATCH(parent, MID_GameObject_SELF_RemoveChild, {
            Payload_SetValue(msg, "child", TempObjectReference, Self);
        }, {});
        SELF_DISPATCH(parent, MID_GameObject_SELF_AddChild, {
            Payload_SetValue(msg, "child", TempObjectReference, Self);
        }, {});
        ObjectContainer_UnRef_External(&_guard);
    }
MESSAGE_HANDLER_END()

// ============================================================
// SELF_SpreadMessage
//
// NOTE: Child iteration uses a snapshot of the child list taken before
// iteration begins. Adding or removing children during a spread is safe
// (no crash or stale index), but newly added children will NOT receive
// the current message, and removed children may still be visited if they
// were in the snapshot (they are skipped if their data is NULL).
// ============================================================

SELF_MESSAGE_HANDLER_BEGIN(SpreadMessage)
    MH_ExtractDeref(inner, MessagePayload*);

    int spread_direction = 0;
    {
        void *sd = Payload_Get(inner, "spread_direction");
        if (sd) spread_direction = *(int *)sd;
    }

    int spread_reverse = 0;
    {
        void *sr = Payload_Get(inner, "spread_reverse");
        if (sr) spread_reverse = *(int *)sr;
    }

    int child_count = Self_GetDeref("child_count", int);
    TempObjectReference orig_inner_self = Payload_GetDeref(inner, "Self", TempObjectReference);

    if (spread_direction == SPREAD_DOWN) {
        // Top-down: self first, then children
        Payload_SetValue(inner, "Self", TempObjectReference, Self);
        inner->cid_target = Self->cid;
        DispatchMessage(inner);

        int consumed = 0;
        {
            void *cp = Payload_Get(inner, _GO_CONSUMED_KEY);
            if (cp) consumed = *(int *)cp;
        }
        if (consumed) {
            Payload_SetValue(inner, _GO_CONSUMED_KEY, int, 0);
        } else {
            _go_spread_to_children(Self, payload, child_count, spread_reverse);
        }
    } else {
        // Bottom-up: children first, then self
        // Note: SPREAD_CONSUME has no effect in SPREAD_UP mode because
        // children are visited before self -- there is nothing left to skip.
        _go_spread_to_children(Self, payload, child_count, spread_reverse);

        Payload_SetValue(inner, "Self", TempObjectReference, Self);
        inner->cid_target = Self->cid;
        DispatchMessage(inner);

        // Reset consumed if set (prevent leaking to parent's sibling loop)
        {
            void *cp = Payload_Get(inner, _GO_CONSUMED_KEY);
            if (cp && *(int *)cp != 0) {
                Payload_SetValue(inner, _GO_CONSUMED_KEY, int, 0);
            }
        }
    }

    // Restore inner Self for the caller
    Payload_SetValue(inner, "Self", TempObjectReference, orig_inner_self);
MESSAGE_HANDLER_END()

// ============================================================
// CAN_RECEIVE / RECEIVE_MESSAGE
// ============================================================

CAN_RECEIVE_BEGIN()
    SELF_CAN_RECEIVE_MID_EXTERN(Object, Create)
    SELF_CAN_RECEIVE_MID_EXTERN(Object, Destroy)
    SELF_CAN_RECEIVE_MID(SpreadMessage)
    SELF_CAN_RECEIVE_MID(AddChild)
    SELF_CAN_RECEIVE_MID(RemoveChild)
    SELF_CAN_RECEIVE_MID(SetActive)
    SELF_CAN_RECEIVE_MID(SetPriority)
    SELF_CAN_RECEIVE_MID(Update)
    SELF_CAN_RECEIVE_MID(Render)
CAN_RECEIVE_END()

RECEIVE_MESSAGE_BEGIN()
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Object, Create)
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Object, Destroy)
    SELF_RECEIVE_MESSAGE_ROUTE(SpreadMessage)
    SELF_RECEIVE_MESSAGE_ROUTE(AddChild)
    SELF_RECEIVE_MESSAGE_ROUTE(RemoveChild)
    SELF_RECEIVE_MESSAGE_ROUTE(SetActive)
    SELF_RECEIVE_MESSAGE_ROUTE(SetPriority)
    SELF_RECEIVE_MESSAGE_ROUTE(Update)
    SELF_RECEIVE_MESSAGE_ROUTE(Render)
RECEIVE_MESSAGE_END()

CLASSDEF_INHERITS(Object)

#undef TYPE

// ============================================================
// Helper functions
// ============================================================

static inline TempObjectReference GameObject_CreateRoot(ClassID cid) {
    return Object_Create(cid);
}

static inline ExternalReference GameObject_CreateRootRef(ClassID cid) {
    return Object_CreateRef(cid);
}

static inline TempObjectReference GameObject_CreateChild(TempObjectReference parent, ClassID child_cid) {
    if (parent == NULL) return NULL;
    TempObjectReference child = Object_Create(child_cid);
    if (child == NULL) return NULL;
    SELF_DISPATCH(parent, MID_GameObject_SELF_AddChild, {
        Payload_SetValue(msg, "child", TempObjectReference, child);
    }, {});
    return child;
}

static inline ExternalReference GameObject_CreateChildRef(TempObjectReference parent, ClassID child_cid) {
    if (parent == NULL) return NULL;
    TempObjectReference child = Object_Create(child_cid);
    if (child == NULL) return NULL;
    ExternalReference ref = ObjectContainer_ExternalRef_From_Temp(child);
    SELF_DISPATCH(parent, MID_GameObject_SELF_AddChild, {
        Payload_SetValue(msg, "child", TempObjectReference, child);
    }, {});
    return ref;
}

// ============================================================
// GAMEOBJECT_DISPATCH macro
// ============================================================

#define GAMEOBJECT_DISPATCH(root, mid, direction, params_block, out_block) ({ \
    TempObjectReference _gd_root = (root); \
    uint8_t _gd_result = MESSAGE_RESULT_INVALID_SELF; \
    if (_gd_root == NULL) { LOG_ERROR("GAMEOBJECT_DISPATCH: NULL root"); } \
    else { \
        MessagePayload _gd_inner = PrepareSelfPayload(_gd_root, mid); \
        MessagePayload *msg = &_gd_inner; \
        if (msg->data == NULL) { _gd_result = MESSAGE_RESULT_OOM; } \
        else { \
            Payload_SetValue(msg, "spread_direction", int, direction); \
            params_block \
            MessagePayload _gd_outer = PrepareSelfPayload(_gd_root, MID_GameObject_SELF_SpreadMessage); \
            if (_gd_outer.data == NULL) { _gd_result = MESSAGE_RESULT_OOM; } \
            else { \
                Payload_SetValue(&_gd_outer, "inner", MessagePayload*, &_gd_inner); \
                DispatchMessage(&_gd_outer); \
                out_block \
                _gd_result = _gd_outer.result; \
            } \
            FreePayload(&_gd_outer); \
        } \
        FreePayload(msg); \
    } \
    _gd_result; \
})
