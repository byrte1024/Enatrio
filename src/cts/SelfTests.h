#pragma once

#include "../tests.h"
#include "Self.h"
#include "ObjectGraphViz.h"

// ============================================================
// Test class: Counter -- an instanced class with Self
// Stores "count" (int) on Self, supports Increment, Decrement, GetCount, Reset
// ============================================================

#define TYPE Counter

BEGIN_CLASS(0x0010);

DECLARE_SELF_MID(Increment);
DECLARE_SELF_MID(Decrement);
DECLARE_SELF_MID(GetCount);
DECLARE_SELF_MID(Reset);

// -- SELF lifecycle handlers (Default namespace) --

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Default, Create)
    Self_SetValue("count", int, 0);
    LOG_INFO("Counter created, count = 0");
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Default, Destroy)
    LOG_INFO("Counter destroyed, final count = %d", Self_GetDeref("count", int));
MESSAGE_HANDLER_END()

// -- SELF instance handlers --

SELF_MESSAGE_HANDLER_BEGIN(Increment)
    int count = Self_GetDeref("count", int);
    int step = 1;
    if (MH_Has(step)) {
        step = MH_GetDeref(step, int);
    }
    count += step;
    *Self_Get("count", int) = count;
    MH_SetValue(result, int, count);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN(Decrement)
    int *ptr = Self_Get("count", int);
    *ptr -= 1;
    MH_SetValue(result, int, *ptr);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN(GetCount)
    MH_SetValue(result, int, Self_GetDeref("count", int));
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN(Reset)
    *Self_Get("count", int) = 0;
MESSAGE_HANDLER_END()

CAN_RECEIVE_BEGIN()
    SELF_CAN_RECEIVE_MID_EXTERN(Default, Create)
    SELF_CAN_RECEIVE_MID_EXTERN(Default, Destroy)
    SELF_CAN_RECEIVE_MID(Increment)
    SELF_CAN_RECEIVE_MID(Decrement)
    SELF_CAN_RECEIVE_MID(GetCount)
    SELF_CAN_RECEIVE_MID(Reset)
CAN_RECEIVE_END()

RECEIVE_MESSAGE_BEGIN()
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Default, Create)
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Default, Destroy)
    SELF_RECEIVE_MESSAGE_ROUTE(Increment)
    SELF_RECEIVE_MESSAGE_ROUTE(Decrement)
    SELF_RECEIVE_MESSAGE_ROUTE(GetCount)
    SELF_RECEIVE_MESSAGE_ROUTE(Reset)
RECEIVE_MESSAGE_END()

CLASSDEF()

#undef TYPE

// ============================================================
// Helper: dispatch a message to a Counter instance
// ============================================================

static MessagePayload _counter_dispatch(TempObjectReference obj, MessageID mid) {
    MessagePayload msg = PrepareSelfPayload(obj, mid);
    DispatchMessage(&msg);
    return msg;
}

// ============================================================
// Test class: Node -- a tree node that holds references to children
// Stores "value" (int) on Self, optionally refs "left" and "right"
// ============================================================

#define TYPE Node

BEGIN_CLASS(0x0020);

DECLARE_SELF_MID(SetLeft);
DECLARE_SELF_MID(SetRight);
DECLARE_SELF_MID(GetValue);
DECLARE_SELF_MID(SumTree);

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Default, Create)
    MH_ExtractDeref(value, int);
    Self_SetValue("value", int, value);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Default, Destroy)
    (void)Self; // nothing extra to clean up, refs are auto-unref'd
MESSAGE_HANDLER_END()

// SetLeft: takes an ObjectReference to a child node, stores it
SELF_MESSAGE_HANDLER_BEGIN(SetLeft)
    MH_ExtractDeref(child, ObjectReference);
    Self_RefFrom("left", child);
MESSAGE_HANDLER_END()

// SetRight: same for right child
SELF_MESSAGE_HANDLER_BEGIN(SetRight)
    MH_ExtractDeref(child, ObjectReference);
    Self_RefFrom("right", child);
MESSAGE_HANDLER_END()

// GetValue: returns this node's value
SELF_MESSAGE_HANDLER_BEGIN(GetValue)
    MH_SetValue(result, int, Self_GetDeref("value", int));
MESSAGE_HANDLER_END()

// SumTree: recursively sums this node + left subtree + right subtree
SELF_MESSAGE_HANDLER_BEGIN(SumTree)
    int sum = Self_GetDeref("value", int);

    if (Self_HasRef("left") && !Self_IsRefSelf("left")) {
            TempObjectReference left = Self_GetRef("left");
        MessagePayload lm = PrepareSelfPayload(left, MID_Node_SELF_SumTree);
        DispatchMessage(&lm);
        sum += Payload_GetDeref(&lm, "result", int);
        FreePayload(&lm);
    }
    if (Self_HasRef("right") && !Self_IsRefSelf("right")) {
            TempObjectReference right = Self_GetRef("right");
        MessagePayload rm = PrepareSelfPayload(right, MID_Node_SELF_SumTree);
        DispatchMessage(&rm);
        sum += Payload_GetDeref(&rm, "result", int);
        FreePayload(&rm);
    }

    MH_SetValue(result, int, sum);
MESSAGE_HANDLER_END()

CAN_RECEIVE_BEGIN()
    SELF_CAN_RECEIVE_MID_EXTERN(Default, Create)
    SELF_CAN_RECEIVE_MID_EXTERN(Default, Destroy)
    SELF_CAN_RECEIVE_MID(SetLeft)
    SELF_CAN_RECEIVE_MID(SetRight)
    SELF_CAN_RECEIVE_MID(GetValue)
    SELF_CAN_RECEIVE_MID(SumTree)
CAN_RECEIVE_END()

RECEIVE_MESSAGE_BEGIN()
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Default, Create)
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Default, Destroy)
    SELF_RECEIVE_MESSAGE_ROUTE(SetLeft)
    SELF_RECEIVE_MESSAGE_ROUTE(SetRight)
    SELF_RECEIVE_MESSAGE_ROUTE(GetValue)
    SELF_RECEIVE_MESSAGE_ROUTE(SumTree)
RECEIVE_MESSAGE_END()

CLASSDEF()

#undef TYPE

// ============================================================
// Node helper: create a node with a value (needs "value" in SELF_Create payload)
// ============================================================

static TempObjectReference _node_create(int value) {
    TempObjectReference ghost = ObjectContainer_CreateGhost();
    if (ghost == NULL) return NULL;
    ObjectContainer_TypeEmptyUntyped(ghost, CID_Node);

    // Manually fill: alloc data, then dispatch SELF_Create with value param
    ghost->data = (ObjectData*)malloc(sizeof(ObjectData));
    ghost->data->values = UnsafeVariedHashMap_Create(8);
    ghost->data->references = UnsafeHashMap_Create(sizeof(ObjectReference), 8);

    MessagePayload p = PrepareSelfPayload(ghost, MID_Default_SELF_Create);
    Payload_SetValue(&p, "value", int, value);
    DispatchMessage(&p);
    FreePayload(&p);

    return ghost;
}

static void _node_set_child(TempObjectReference parent, MessageID mid, ObjectReference child) {
    MessagePayload m = PrepareSelfPayload(parent, mid);
    Payload_SetValue(&m, "child", ObjectReference, child);
    DispatchMessage(&m);
    FreePayload(&m);
}

static int _node_sum(TempObjectReference node) {
    MessagePayload m = PrepareSelfPayload(node, MID_Node_SELF_SumTree);
    DispatchMessage(&m);
    int result = Payload_GetDeref(&m, "result", int);
    FreePayload(&m);
    return result;
}

// ============================================================
// Object lifecycle tests
// ============================================================

static void test_self_ghost_lifecycle(void) {
    TEST("self: ghost create and destroy");
    TempObjectReference ghost = ObjectContainer_CreateGhost();
    ASSERT(ghost != NULL);
    ASSERT(ghost->cid == CID_Untyped);
    ASSERT(ghost->data == NULL);
    ASSERT(ghost->external_refs == 0);
    ObjectContainer_DestroyGhost(ghost);
    PASS();
}

static void test_self_type_untype(void) {
    TEST("self: type and untype empty container");
    TempObjectReference ghost = ObjectContainer_CreateGhost();
    ObjectContainer_TypeEmptyUntyped(ghost, CID_Counter);
    ASSERT(ghost->cid == CID_Counter);
    ObjectContainer_UntypeEmptyTyped(ghost);
    ASSERT(ghost->cid == CID_Untyped);
    ObjectContainer_DestroyGhost(ghost);
    PASS();
}

static void test_self_fill_empty(void) {
    TEST("self: fill and empty typed container");
    TempObjectReference obj = ObjectContainer_CreateGhost();
    ObjectContainer_TypeEmptyUntyped(obj, CID_Counter);
    ObjectContainer_FillEmptyTyped(obj);
    ASSERT(obj->data != NULL);
    ASSERT(obj->data->values != NULL);
    ASSERT(obj->data->references != NULL);
    ObjectContainer_EmptyFilledTyped(obj);
    ASSERT(obj->data == NULL);
    ObjectContainer_UntypeEmptyTyped(obj);
    ObjectContainer_DestroyGhost(obj);
    PASS();
}

static void test_self_object_create_destroy(void) {
    TEST("self: Object_Create and Object_Destroy");
    TempObjectReference obj = Object_Create(CID_Counter);
    ASSERT(obj != NULL);
    ASSERT(obj->cid == CID_Counter);
    ASSERT(obj->data != NULL);
    ASSERT(obj->external_refs == 0);
    Object_Destroy(obj);
    PASS();
}

static void test_self_object_create_ref(void) {
    TEST("self: Object_CreateRef gives refcount 1");
    ExternalReference ref = Object_CreateRef(CID_Counter);
    ASSERT(ref != NULL);
    ASSERT(ref->external_refs == 1);
    ObjectContainer_UnRef_External(&ref);
    ASSERT(ref == NULL);
    PASS();
}

// ============================================================
// Reference counting tests
// ============================================================

static void test_self_refcount_basic(void) {
    TEST("self: ref from temp increments, unref decrements");
    TempObjectReference obj = Object_Create(CID_Counter);
    ASSERT(obj->external_refs == 0);
    ExternalReference ref = ObjectContainer_ExternalRef_From_Temp(obj);
    ASSERT(obj->external_refs == 1);
    ObjectContainer_UnRef_External(&ref);
    // obj is freed now (refcount went to 0), don't touch it
    ASSERT(ref == NULL);
    PASS();
}

static void test_self_refcount_multiple(void) {
    TEST("self: multiple refs, last unref destroys");
    TempObjectReference obj = Object_Create(CID_Counter);
    ExternalReference ref1 = ObjectContainer_ExternalRef_From_Temp(obj);
    ExternalReference ref2 = ObjectContainer_ExternalRef_From_External(ref1);
    ExternalReference ref3 = ObjectContainer_ExternalRef_From_External(ref2);
    ASSERT(obj->external_refs == 3);

    ObjectContainer_UnRef_External(&ref1);
    ASSERT(ref1 == NULL);
    ASSERT(obj->external_refs == 2);

    ObjectContainer_UnRef_External(&ref2);
    ASSERT(ref2 == NULL);
    ASSERT(obj->external_refs == 1);

    // Last unref -- obj gets destroyed
    ObjectContainer_UnRef_External(&ref3);
    ASSERT(ref3 == NULL);
    PASS();
}

// ============================================================
// Counter instance message tests
// ============================================================

static void test_self_counter_create_inits_zero(void) {
    TEST("self: Counter SELF_Create initializes count to 0");
    TempObjectReference obj = Object_Create(CID_Counter);
    ASSERT(obj->data != NULL);
    int *count = (int*)UnsafeVariedHashMap_SGet(obj->data->values, "count");
    ASSERT(count != NULL);
    ASSERT(*count == 0);
    Object_Destroy(obj);
    PASS();
}

static void test_self_counter_increment(void) {
    TEST("self: Counter Increment increases count");
    TempObjectReference obj = Object_Create(CID_Counter);
    MessagePayload m1 = _counter_dispatch(obj, MID_Counter_SELF_Increment);
    ASSERT(MESSAGE_RESULT_ISOK(m1.result));
    ASSERT(Payload_GetDeref(&m1, "result", int) == 1);
    FreePayload(&m1);

    MessagePayload m2 = _counter_dispatch(obj, MID_Counter_SELF_Increment);
    ASSERT(Payload_GetDeref(&m2, "result", int) == 2);
    FreePayload(&m2);

    Object_Destroy(obj);
    PASS();
}

static void test_self_counter_increment_with_step(void) {
    TEST("self: Counter Increment with custom step");
    TempObjectReference obj = Object_Create(CID_Counter);
    MessagePayload msg = PrepareSelfPayload(obj, MID_Counter_SELF_Increment);
    Payload_SetValue(&msg, "step", int, 5);
    DispatchMessage(&msg);
    ASSERT(MESSAGE_RESULT_ISOK(msg.result));
    ASSERT(Payload_GetDeref(&msg, "result", int) == 5);
    FreePayload(&msg);
    Object_Destroy(obj);
    PASS();
}

static void test_self_counter_decrement(void) {
    TEST("self: Counter Decrement decreases count");
    TempObjectReference obj = Object_Create(CID_Counter);

    // Increment twice
    MessagePayload m1 = _counter_dispatch(obj, MID_Counter_SELF_Increment);
    FreePayload(&m1);
    MessagePayload m2 = _counter_dispatch(obj, MID_Counter_SELF_Increment);
    FreePayload(&m2);

    // Decrement once
    MessagePayload m3 = _counter_dispatch(obj, MID_Counter_SELF_Decrement);
    ASSERT(MESSAGE_RESULT_ISOK(m3.result));
    ASSERT(Payload_GetDeref(&m3, "result", int) == 1);
    FreePayload(&m3);

    Object_Destroy(obj);
    PASS();
}

static void test_self_counter_getcount(void) {
    TEST("self: Counter GetCount returns current count");
    TempObjectReference obj = Object_Create(CID_Counter);

    for (int i = 0; i < 3; i++) {
        MessagePayload mi = _counter_dispatch(obj, MID_Counter_SELF_Increment);
        FreePayload(&mi);
    }

    MessagePayload mg = _counter_dispatch(obj, MID_Counter_SELF_GetCount);
    ASSERT(MESSAGE_RESULT_ISOK(mg.result));
    ASSERT(Payload_GetDeref(&mg, "result", int) == 3);
    FreePayload(&mg);

    Object_Destroy(obj);
    PASS();
}

static void test_self_counter_reset(void) {
    TEST("self: Counter Reset sets count to 0");
    TempObjectReference obj = Object_Create(CID_Counter);

    for (int i = 0; i < 5; i++) {
        MessagePayload mi = _counter_dispatch(obj, MID_Counter_SELF_Increment);
        FreePayload(&mi);
    }

    MessagePayload mr = _counter_dispatch(obj, MID_Counter_SELF_Reset);
    ASSERT(MESSAGE_RESULT_ISOK(mr.result));
    FreePayload(&mr);

    MessagePayload mg = _counter_dispatch(obj, MID_Counter_SELF_GetCount);
    ASSERT(Payload_GetDeref(&mg, "result", int) == 0);
    FreePayload(&mg);

    Object_Destroy(obj);
    PASS();
}

static void test_self_two_instances_independent(void) {
    TEST("self: two Counter instances are independent");
    TempObjectReference a = Object_Create(CID_Counter);
    TempObjectReference b = Object_Create(CID_Counter);

    // Increment a 3 times
    for (int i = 0; i < 3; i++) {
        MessagePayload msg = _counter_dispatch(a, MID_Counter_SELF_Increment);
        FreePayload(&msg);
    }
    // Increment b 7 times
    for (int i = 0; i < 7; i++) {
        MessagePayload msg = _counter_dispatch(b, MID_Counter_SELF_Increment);
        FreePayload(&msg);
    }

    MessagePayload ma = _counter_dispatch(a, MID_Counter_SELF_GetCount);
    MessagePayload mb = _counter_dispatch(b, MID_Counter_SELF_GetCount);
    ASSERT(Payload_GetDeref(&ma, "result", int) == 3);
    ASSERT(Payload_GetDeref(&mb, "result", int) == 7);
    FreePayload(&ma);
    FreePayload(&mb);

    Object_Destroy(a);
    Object_Destroy(b);
    PASS();
}

static void test_self_ref_dispatch(void) {
    TEST("self: dispatch via ObjectReference works");
    ExternalReference ref = Object_CreateRef(CID_Counter);

    MessagePayload msg = PrepareSelfPayload(ref, MID_Counter_SELF_Increment);
    DispatchMessage(&msg);
    ASSERT(MESSAGE_RESULT_ISOK(msg.result));
    ASSERT(Payload_GetDeref(&msg, "result", int) == 1);
    FreePayload(&msg);

    ObjectContainer_UnRef_External(&ref);
    PASS();
}

// ============================================================
// Held reference cleanup tests
// ============================================================

static void test_self_empty_unrefs_held(void) {
    TEST("self: emptying object unrefs held internal refs");
    TempObjectReference holder = Object_Create(CID_Counter);
    TempObjectReference target = Object_Create(CID_Counter);
    Object_SStoreRef(holder, "t", target);
    ASSERT(target->internal_refs == 1);
    ExternalReference h = ObjectContainer_ExternalRef_From_Temp(holder);
    Object_EmptyFilledType(h);
    ASSERT(holder->data == NULL);
    ObjectContainer_UntypeEmptyTyped(holder);
    ObjectContainer_DestroyGhost(holder);
    PASS();
}

static void test_self_empty_unrefs_multiple(void) {
    TEST("self: emptying unrefs multiple held refs");
    TempObjectReference holder = Object_Create(CID_Counter);
    for (int i = 0; i < 5; i++) {
        TempObjectReference t = Object_Create(CID_Counter);
        char key[8];
        snprintf(key, sizeof(key), "t%d", i);
        Object_StoreRef(holder, key, (uint32_t)strlen(key), t);
    }
    ExternalReference h = ObjectContainer_ExternalRef_From_Temp(holder);
    Object_EmptyFilledType(h);
    ObjectContainer_UntypeEmptyTyped(holder);
    ObjectContainer_DestroyGhost(holder);
    PASS();
}

static void test_self_empty_shared_survives(void) {
    TEST("self: shared target survives if ext ref exists");
    TempObjectReference holder = Object_Create(CID_Counter);
    TempObjectReference target = Object_Create(CID_Counter);
    ExternalReference target_ext = ObjectContainer_ExternalRef_From_Temp(target);
    Object_SStoreRef(holder, "s", target);
    ASSERT(target->external_refs == 1);
    ASSERT(target->internal_refs == 1);
    ExternalReference h = ObjectContainer_ExternalRef_From_Temp(holder);
    Object_EmptyFilledType(h);
    ASSERT(target->internal_refs == 0);
    ASSERT(target->external_refs == 1);
    ObjectContainer_UntypeEmptyTyped(holder);
    ObjectContainer_DestroyGhost(holder);
    ObjectContainer_UnRef_External(&target_ext);
    PASS();
}

static void test_self_unref_cascades_held(void) {
    TEST("self: unref holder cascades to held refs");
    ExternalReference h = Object_CreateRef(CID_Counter);
    TempObjectReference target = Object_Create(CID_Counter);
    Object_SStoreRef(h, "child", target);
    ASSERT(target->internal_refs == 1);
    ObjectContainer_UnRef_External(&h);
    ASSERT(h == NULL);
    PASS();
}

// ============================================================
// Node tree tests
// ============================================================

static void test_node_single(void) {
    TEST("node: single node holds value");
    TempObjectReference n = _node_create(42);
    ASSERT(_node_sum(n) == 42);
    Object_Destroy(n);
    PASS();
}

static void test_node_two_children(void) {
    TEST("node: parent + two children sums correctly");
    TempObjectReference root = _node_create(10);
    TempObjectReference left = _node_create(20);
    TempObjectReference right = _node_create(30);
    ExternalReference l = ObjectContainer_ExternalRef_From_Temp(left);
    ExternalReference r = ObjectContainer_ExternalRef_From_Temp(right);
    _node_set_child(root, MID_Node_SELF_SetLeft, l);
    _node_set_child(root, MID_Node_SELF_SetRight, r);
    ASSERT(_node_sum(root) == 60);
    Object_Destroy(root);
    ObjectContainer_UnRef_External(&l);
    ObjectContainer_UnRef_External(&r);
    PASS();
}

static void test_node_shared_child(void) {
    TEST("node: two parents share same child");
    TempObjectReference shared = _node_create(5);
    TempObjectReference p1 = _node_create(10);
    TempObjectReference p2 = _node_create(20);
    ExternalReference s = ObjectContainer_ExternalRef_From_Temp(shared);
    _node_set_child(p1, MID_Node_SELF_SetRight, s);
    _node_set_child(p2, MID_Node_SELF_SetLeft, s);
    ASSERT(shared->external_refs == 1);
    ASSERT(shared->internal_refs == 2);
    Object_Destroy(p1);
    ASSERT(shared->internal_refs == 1);
    Object_Destroy(p2);
    ASSERT(shared->internal_refs == 0);
    ObjectContainer_UnRef_External(&s);
    PASS();
}

static void test_node_self_ref_skips(void) {
    TEST("node: SumTree skips self-ref children");
    TempObjectReference a = _node_create(42);
    ExternalReference ae = ObjectContainer_ExternalRef_From_Temp(a);
    _node_set_child(a, MID_Node_SELF_SetLeft, ae);
    ASSERT(_node_sum(a) == 42);
    Object_Destroy(a);
    PASS();
}

// ============================================================
// Cycle tests (using Object_SStoreRef -- internal refs)
// ============================================================

static void test_cycle_ab_empty(void) {
    TEST("cycle: A<->B, empty A cascades B destruction");
    TempObjectReference a = _node_create(1);
    TempObjectReference b = _node_create(2);
    ExternalReference a_ext = ObjectContainer_ExternalRef_From_Temp(a);
    Object_SStoreRef(a, "b", b);
    Object_SStoreRef(b, "a", a);
    ASSERT(a->external_refs == 1);
    ASSERT(a->internal_refs == 1);
    ASSERT(b->internal_refs == 1);
    Object_EmptyFilledType(a_ext);
    ASSERT(a->data == NULL);
    ASSERT(a->internal_refs == 0);
    ObjectContainer_UntypeEmptyTyped(a);
    ObjectContainer_DestroyGhost(a);
    PASS();
}

static void test_cycle_self_ref_empty(void) {
    TEST("cycle: A self-ref, empty A safe");
    TempObjectReference a = _node_create(1);
    ExternalReference a_ext = ObjectContainer_ExternalRef_From_Temp(a);
    Object_SStoreRef(a, "self", a);
    ASSERT(a->external_refs == 1);
    ASSERT(a->internal_refs == 1);
    Object_EmptyFilledType(a_ext);
    ASSERT(a->data == NULL);
    ASSERT(a->internal_refs == 0);
    ObjectContainer_UntypeEmptyTyped(a);
    ObjectContainer_DestroyGhost(a);
    PASS();
}

static void test_cycle_triangle_empty(void) {
    TEST("cycle: A->B->C->A, empty A cascades all");
    TempObjectReference a = _node_create(1);
    TempObjectReference b = _node_create(2);
    TempObjectReference c = _node_create(3);
    ExternalReference a_ext = ObjectContainer_ExternalRef_From_Temp(a);
    Object_SStoreRef(a, "next", b);
    Object_SStoreRef(b, "next", c);
    Object_SStoreRef(c, "next", a);
    ASSERT(a->internal_refs == 1);
    ASSERT(b->internal_refs == 1);
    ASSERT(c->internal_refs == 1);
    Object_EmptyFilledType(a_ext);
    ASSERT(a->data == NULL);
    ASSERT(a->internal_refs == 0);
    ObjectContainer_UntypeEmptyTyped(a);
    ObjectContainer_DestroyGhost(a);
    PASS();
}

// ============================================================
// Cycle collection tests (auto GC on last external unref)
// ============================================================

static void test_gc_ab_cycle_collected(void) {
    TEST("gc: A<->B cycle collected when last external dropped");
    TempObjectReference a = _node_create(1);
    TempObjectReference b = _node_create(2);
    ExternalReference a_ext = ObjectContainer_ExternalRef_From_Temp(a);
    Object_SStoreRef(a, "b", b);
    Object_SStoreRef(b, "a", a);
    ASSERT(a->external_refs == 1);
    ASSERT(a->internal_refs == 1);
    ASSERT(b->internal_refs == 1);
    // Drop last external -- should trigger cycle collection, destroying both
    ObjectContainer_UnRef_External(&a_ext);
    ASSERT(a_ext == NULL);
    // Both a and b are freed (can't assert on freed memory, but no crash = success)
    PASS();
}

static void test_gc_self_ref_collected(void) {
    TEST("gc: self-referencing object collected when external dropped");
    TempObjectReference a = _node_create(1);
    ExternalReference a_ext = ObjectContainer_ExternalRef_From_Temp(a);
    Object_SStoreRef(a, "self", a);
    ASSERT(a->external_refs == 1);
    ASSERT(a->internal_refs == 1);
    ObjectContainer_UnRef_External(&a_ext);
    ASSERT(a_ext == NULL);
    PASS();
}

static void test_gc_triangle_collected(void) {
    TEST("gc: A->B->C->A triangle collected on last external drop");
    TempObjectReference a = _node_create(1);
    TempObjectReference b = _node_create(2);
    TempObjectReference c = _node_create(3);
    ExternalReference a_ext = ObjectContainer_ExternalRef_From_Temp(a);
    Object_SStoreRef(a, "next", b);
    Object_SStoreRef(b, "next", c);
    Object_SStoreRef(c, "next", a);
    ObjectContainer_UnRef_External(&a_ext);
    ASSERT(a_ext == NULL);
    // Entire triangle collected
    PASS();
}

static void test_gc_partial_external_survives(void) {
    TEST("gc: cycle with one external ref survives collection");
    TempObjectReference a = _node_create(1);
    TempObjectReference b = _node_create(2);
    ExternalReference a_ext = ObjectContainer_ExternalRef_From_Temp(a);
    ExternalReference b_ext = ObjectContainer_ExternalRef_From_Temp(b);
    Object_SStoreRef(a, "b", b);
    Object_SStoreRef(b, "a", a);
    // Drop a's external -- b still has one, so component has externals, no collection
    ObjectContainer_UnRef_External(&a_ext);
    ASSERT(a_ext == NULL);
    // Both still alive because b has an external ref
    ASSERT(b->external_refs == 1);
    ASSERT(b->data != NULL);
    ASSERT(a->data != NULL); // a reachable from b
    // Now drop b's external -- component has 0 externals, collect both
    ObjectContainer_UnRef_External(&b_ext);
    ASSERT(b_ext == NULL);
    PASS();
}

static void test_gc_chain_cascade(void) {
    TEST("gc: A->B->C one-way chain, drop A cascades through B, C survives with ext");
    TempObjectReference a = _node_create(1);
    TempObjectReference b = _node_create(2);
    TempObjectReference c = _node_create(3);
    ExternalReference a_ext = ObjectContainer_ExternalRef_From_Temp(a);
    ExternalReference c_ext = ObjectContainer_ExternalRef_From_Temp(c);
    Object_SStoreRef(a, "b", b);
    Object_SStoreRef(b, "c", c);
    // A has 0 internal refs. Drop A's external -> total 0 -> A destroyed immediately.
    // A's destroy unrefs B -> B total 0 -> B destroyed -> B unrefs C -> C internal 0 but ext 1 -> survives.
    ObjectContainer_UnRef_External(&a_ext);
    ASSERT(c->external_refs == 1);
    ASSERT(c->data != NULL); // C survives
    ObjectContainer_UnRef_External(&c_ext);
    PASS();
}

static void test_gc_many_cycles_collected(void) {
    TEST("gc: 20 A<->B cycles all collected");
    for (int i = 0; i < 20; i++) {
        TempObjectReference a = _node_create(i);
        TempObjectReference b = _node_create(i + 100);
        ExternalReference a_ext = ObjectContainer_ExternalRef_From_Temp(a);
        Object_SStoreRef(a, "b", b);
        Object_SStoreRef(b, "a", a);
        ObjectContainer_UnRef_External(&a_ext);
        // Both collected
    }
    PASS();
}

// ============================================================
// Memory management stress
// ============================================================

// ============================================================
// GC edge cases
// ============================================================

static void test_gc_diamond(void) {
    TEST("gc: diamond A->B,C->D, drop A cascades all");
    TempObjectReference a = _node_create(1);
    TempObjectReference b = _node_create(2);
    TempObjectReference c = _node_create(3);
    TempObjectReference d = _node_create(4);
    ExternalReference a_ext = ObjectContainer_ExternalRef_From_Temp(a);
    Object_SStoreRef(a, "left", b);
    Object_SStoreRef(a, "right", c);
    Object_SStoreRef(b, "d", d);
    Object_SStoreRef(c, "d", d);
    ASSERT(d->internal_refs == 2);
    // Drop A -> cascade: A(0) destroyed, unrefs B+C. B(0) destroyed, unrefs D. C(0) destroyed, unrefs D. D(0) destroyed.
    ObjectContainer_UnRef_External(&a_ext);
    // All destroyed, no crash = success
    PASS();
}

static void test_gc_cycle_partial_ext_no_collect(void) {
    TEST("gc: A<->B both ext, drop A ext, no collect");
    TempObjectReference a = _node_create(1);
    TempObjectReference b = _node_create(2);
    ExternalReference a_ext = ObjectContainer_ExternalRef_From_Temp(a);
    ExternalReference b_ext = ObjectContainer_ExternalRef_From_Temp(b);
    Object_SStoreRef(a, "b", b);
    Object_SStoreRef(b, "a", a);
    ASSERT(a->external_refs == 1);
    ASSERT(a->internal_refs == 1);
    ASSERT(b->external_refs == 1);
    ASSERT(b->internal_refs == 1);
    // Drop A's ext -> A ext=0, int=1. GC traverses: A->B. B has ext=1. No collect.
    ObjectContainer_UnRef_External(&a_ext);
    ASSERT(a->data != NULL); // still alive
    ASSERT(b->data != NULL);
    // Drop B's ext -> B ext=0, int=1. GC traverses: B->A. A ext=0. No external. Collect both.
    ObjectContainer_UnRef_External(&b_ext);
    PASS();
}

static void test_gc_triangle_one_ext(void) {
    TEST("gc: A->B->C->A, only B has ext, drop B ext collects all");
    TempObjectReference a = _node_create(1);
    TempObjectReference b = _node_create(2);
    TempObjectReference c = _node_create(3);
    ExternalReference b_ext = ObjectContainer_ExternalRef_From_Temp(b);
    Object_SStoreRef(a, "next", b);
    Object_SStoreRef(b, "next", c);
    Object_SStoreRef(c, "next", a);
    // Drop B's ext -> B ext=0, int=1. GC from B: B->C->A->B. No externals. Collect all 3.
    ObjectContainer_UnRef_External(&b_ext);
    PASS();
}

static void test_gc_self_plus_other(void) {
    TEST("gc: A refs self and B, drop A ext collects both");
    TempObjectReference a = _node_create(1);
    TempObjectReference b = _node_create(2);
    ExternalReference a_ext = ObjectContainer_ExternalRef_From_Temp(a);
    Object_SStoreRef(a, "self", a);
    Object_SStoreRef(a, "other", b);
    ASSERT(a->internal_refs == 1);
    ASSERT(b->internal_refs == 1);
    // Drop A ext -> A ext=0, int=1. GC from A: self(visited), B. B ext=0. No external. Collect A+B.
    ObjectContainer_UnRef_External(&a_ext);
    PASS();
}

static void test_gc_long_cycle(void) {
    TEST("gc: 5-node cycle collected on ext drop");
    TempObjectReference nodes[5];
    for (int i = 0; i < 5; i++)
        nodes[i] = _node_create(i);
    ExternalReference ext = ObjectContainer_ExternalRef_From_Temp(nodes[0]);
    for (int i = 0; i < 5; i++)
        Object_SStoreRef(nodes[i], "next", nodes[(i + 1) % 5]);
    // All have int=1, only nodes[0] has ext=1
    ObjectContainer_UnRef_External(&ext);
    // GC from nodes[0]: traverses all 5, none have ext. Collect all.
    PASS();
}

static void test_gc_multi_ext_no_trigger(void) {
    TEST("gc: A<->B, A has 2 ext, drop one, no GC trigger");
    TempObjectReference a = _node_create(1);
    TempObjectReference b = _node_create(2);
    ExternalReference a_ext1 = ObjectContainer_ExternalRef_From_Temp(a);
    ExternalReference a_ext2 = ObjectContainer_ExternalRef_From_Temp(a);
    Object_SStoreRef(a, "b", b);
    Object_SStoreRef(b, "a", a);
    ASSERT(a->external_refs == 2);
    // Drop one ext -> A ext=1. No GC trigger (ext still > 0).
    ObjectContainer_UnRef_External(&a_ext1);
    ASSERT(a->external_refs == 1);
    ASSERT(a->data != NULL);
    ASSERT(b->data != NULL);
    // Drop second ext -> A ext=0, int=1. GC: A->B, B ext=0. Collect both.
    ObjectContainer_UnRef_External(&a_ext2);
    PASS();
}

static void test_gc_two_separate_cycles(void) {
    TEST("gc: two separate cycles, only one collected");
    TempObjectReference a = _node_create(1);
    TempObjectReference b = _node_create(2);
    TempObjectReference c = _node_create(3);
    TempObjectReference d = _node_create(4);
    ExternalReference a_ext = ObjectContainer_ExternalRef_From_Temp(a);
    ExternalReference c_ext = ObjectContainer_ExternalRef_From_Temp(c);
    Object_SStoreRef(a, "b", b);
    Object_SStoreRef(b, "a", a);
    Object_SStoreRef(c, "d", d);
    Object_SStoreRef(d, "c", c);
    // Drop A's ext -> GC collects A<->B. C<->D untouched.
    ObjectContainer_UnRef_External(&a_ext);
    ASSERT(c->data != NULL);
    ASSERT(d->data != NULL);
    ASSERT(c->external_refs == 1);
    // Clean up C<->D
    ObjectContainer_UnRef_External(&c_ext);
    PASS();
}

static void test_gc_empty_ghost_ext_drop(void) {
    TEST("gc: empty ghost with ext, drop ext, just frees");
    TempObjectReference ghost = ObjectContainer_CreateGhost();
    ObjectContainer_TypeEmptyUntyped(ghost, CID_Counter);
    ExternalReference ext = ObjectContainer_ExternalRef_From_Temp(ghost);
    // ghost has no data, just typed. ext=1, int=0.
    ObjectContainer_UnRef_External(&ext);
    // Total 0, data NULL, untype+free. No GC needed.
    PASS();
}

// ============================================================
// Graph visualization test
// ============================================================

static void test_viz_graph(void) {
    TEST("viz: complex graph saved to file");
    TempObjectReference root = _node_create(1);
    TempObjectReference a = _node_create(2);
    TempObjectReference b = _node_create(3);
    TempObjectReference c = _node_create(4);

    ExternalReference root_e = ObjectContainer_ExternalRef_From_Temp(root);
    ExternalReference a_e = ObjectContainer_ExternalRef_From_Temp(a);
    ExternalReference b_e = ObjectContainer_ExternalRef_From_Temp(b);

    _node_set_child(root, MID_Node_SELF_SetLeft, a_e);
    _node_set_child(root, MID_Node_SELF_SetRight, b_e);
    Object_SStoreRef(a, "peer", b);
    Object_SStoreRef(b, "self_ref", b);
    Object_SStoreRef(b, "child", c);

    Object_VisualizeGraphSingle("build/tests/graph.txt", root_e);

    Object_Destroy(root);
    ObjectContainer_UnRef_External(&root_e);
    ObjectContainer_UnRef_External(&a_e);
    ObjectContainer_UnRef_External(&b_e);
    PASS();
}

// ============================================================
// Memory management stress
// ============================================================

static void test_self_create_destroy_many(void) {
    TEST("self: create and destroy 100 objects");
    for (int i = 0; i < 100; i++) {
        TempObjectReference obj = Object_Create(CID_Counter);
        Object_Destroy(obj);
    }
    PASS();
}

static void test_self_ext_ref_unref_many(void) {
    TEST("self: create ext ref, unref 100 objects");
    for (int i = 0; i < 100; i++) {
        ExternalReference ref = Object_CreateRef(CID_Counter);
        ObjectContainer_UnRef_External(&ref);
    }
    PASS();
}

static void test_self_many_ext_refs(void) {
    TEST("self: 50 ext refs to one object, unref all");
    TempObjectReference obj = Object_Create(CID_Counter);
    ExternalReference refs[50];
    for (int i = 0; i < 50; i++)
        refs[i] = ObjectContainer_ExternalRef_From_Temp(obj);
    ASSERT(obj->external_refs == 50);
    for (int i = 0; i < 50; i++)
        ObjectContainer_UnRef_External(&refs[i]);
    PASS();
}

// ============================================================
// Runner
// ============================================================

static void run_self_tests(void) {
    BeginClassRegistrations();
    RegisterClass(Counter_ClassDef());
    RegisterClass(Node_ClassDef());
    EndClassRegistrations();

    LOG_INFO("=== Object Lifecycle Tests ===");
    test_self_ghost_lifecycle();
    test_self_type_untype();
    test_self_fill_empty();
    test_self_object_create_destroy();
    test_self_object_create_ref();

    LOG_INFO("=== Reference Counting Tests ===");
    test_self_refcount_basic();
    test_self_refcount_multiple();

    LOG_INFO("=== Counter Instance Tests ===");
    test_self_counter_create_inits_zero();
    test_self_counter_increment();
    test_self_counter_increment_with_step();
    test_self_counter_decrement();
    test_self_counter_getcount();
    test_self_counter_reset();
    test_self_two_instances_independent();
    test_self_ref_dispatch();

    LOG_INFO("=== Held Reference Cleanup ===");
    test_self_empty_unrefs_held();
    test_self_empty_unrefs_multiple();
    test_self_empty_shared_survives();
    test_self_unref_cascades_held();

    LOG_INFO("=== Node Tree Tests ===");
    test_node_single();
    test_node_two_children();
    test_node_shared_child();
    test_node_self_ref_skips();

    LOG_INFO("=== Cycle Tests ===");
    test_cycle_ab_empty();
    test_cycle_self_ref_empty();
    test_cycle_triangle_empty();

    LOG_INFO("=== Cycle Collection (GC) ===");
    test_gc_ab_cycle_collected();
    test_gc_self_ref_collected();
    test_gc_triangle_collected();
    test_gc_partial_external_survives();
    test_gc_chain_cascade();
    test_gc_many_cycles_collected();

    LOG_INFO("=== GC Edge Cases ===");
    test_gc_diamond();
    test_gc_cycle_partial_ext_no_collect();
    test_gc_triangle_one_ext();
    test_gc_self_plus_other();
    test_gc_long_cycle();
    test_gc_multi_ext_no_trigger();
    test_gc_two_separate_cycles();
    test_gc_empty_ghost_ext_drop();

    LOG_INFO("=== Graph Visualization ===");
    test_viz_graph();

    LOG_INFO("=== Memory Management ===");
    test_self_create_destroy_many();
    test_self_ext_ref_unref_many();
    test_self_many_ext_refs();
}

