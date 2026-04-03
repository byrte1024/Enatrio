#pragma once

#include "../tests.h"
#include "Self.h"

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
        ObjectReference left = Self_GetRef("left");
        MessagePayload lm = PrepareSelfPayload(left, MID_Node_SELF_SumTree);
        DispatchMessage(&lm);
        sum += Payload_GetDeref(&lm, "result", int);
        FreePayload(&lm);
    }
    if (Self_HasRef("right") && !Self_IsRefSelf("right")) {
        ObjectReference right = Self_GetRef("right");
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
    ASSERT(ghost->reference_counter == 0);
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
    ASSERT(obj->reference_counter == 0);
    Object_Destroy(obj);
    PASS();
}

static void test_self_object_create_ref(void) {
    TEST("self: Object_CreateRef gives refcount 1");
    ObjectReference ref = Object_CreateRef(CID_Counter);
    ASSERT(ref != NULL);
    ASSERT(ref->reference_counter == 1);
    ObjectContainer_UnRef(&ref);
    ASSERT(ref == NULL);
    PASS();
}

// ============================================================
// Reference counting tests
// ============================================================

static void test_self_refcount_basic(void) {
    TEST("self: ref from temp increments, unref decrements");
    TempObjectReference obj = Object_Create(CID_Counter);
    ASSERT(obj->reference_counter == 0);
    ObjectReference ref = ObjectContainer_Ref_From_TempRef(obj);
    ASSERT(obj->reference_counter == 1);
    ObjectContainer_UnRef(&ref);
    // obj is freed now (refcount went to 0), don't touch it
    ASSERT(ref == NULL);
    PASS();
}

static void test_self_refcount_multiple(void) {
    TEST("self: multiple refs, last unref destroys");
    TempObjectReference obj = Object_Create(CID_Counter);
    ObjectReference ref1 = ObjectContainer_Ref_From_TempRef(obj);
    ObjectReference ref2 = ObjectContainer_Ref_From_Ref(ref1);
    ObjectReference ref3 = ObjectContainer_Ref_From_Ref(ref2);
    ASSERT(obj->reference_counter == 3);

    ObjectContainer_UnRef(&ref1);
    ASSERT(ref1 == NULL);
    ASSERT(obj->reference_counter == 2);

    ObjectContainer_UnRef(&ref2);
    ASSERT(ref2 == NULL);
    ASSERT(obj->reference_counter == 1);

    // Last unref -- obj gets destroyed
    ObjectContainer_UnRef(&ref3);
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
    ObjectReference ref = Object_CreateRef(CID_Counter);

    MessagePayload msg = PrepareSelfPayload(ref, MID_Counter_SELF_Increment);
    DispatchMessage(&msg);
    ASSERT(MESSAGE_RESULT_ISOK(msg.result));
    ASSERT(Payload_GetDeref(&msg, "result", int) == 1);
    FreePayload(&msg);

    ObjectContainer_UnRef(&ref);
    PASS();
}

// ============================================================
// Memory management / lifecycle stress tests
// ============================================================

static void test_self_create_destroy_many(void) {
    TEST("self: create and destroy 100 objects");
    for (int i = 0; i < 100; i++) {
        TempObjectReference obj = Object_Create(CID_Counter);
        ASSERT(obj != NULL);
        ASSERT(obj->data != NULL);
        Object_Destroy(obj);
    }
    PASS();
}

static void test_self_create_ref_unref_many(void) {
    TEST("self: create ref, unref 100 objects");
    for (int i = 0; i < 100; i++) {
        ObjectReference ref = Object_CreateRef(CID_Counter);
        ASSERT(ref != NULL);
        ASSERT(ref->reference_counter == 1);
        ObjectContainer_UnRef(&ref);
        ASSERT(ref == NULL);
    }
    PASS();
}

static void test_self_many_refs_one_object(void) {
    TEST("self: 50 refs to one object, unref all");
    TempObjectReference obj = Object_Create(CID_Counter);
    ObjectReference refs[50];
    for (int i = 0; i < 50; i++) {
        refs[i] = ObjectContainer_Ref_From_TempRef(obj);
    }
    ASSERT(obj->reference_counter == 50);

    for (int i = 0; i < 49; i++) {
        ObjectContainer_UnRef(&refs[i]);
        ASSERT(refs[i] == NULL);
        ASSERT(obj->reference_counter == 49 - i);
    }
    // Last unref destroys
    ObjectContainer_UnRef(&refs[49]);
    ASSERT(refs[49] == NULL);
    PASS();
}

static void test_self_dispatch_after_multiple_create(void) {
    TEST("self: dispatch to many objects, verify independence");
    TempObjectReference objs[10];
    for (int i = 0; i < 10; i++) {
        objs[i] = Object_Create(CID_Counter);
        ASSERT(objs[i] != NULL);
    }
    // Increment each object i times
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j <= i; j++) {
            MessagePayload m = _counter_dispatch(objs[i], MID_Counter_SELF_Increment);
            FreePayload(&m);
        }
    }
    // Verify counts
    for (int i = 0; i < 10; i++) {
        MessagePayload m = _counter_dispatch(objs[i], MID_Counter_SELF_GetCount);
        ASSERT(Payload_GetDeref(&m, "result", int) == i + 1);
        FreePayload(&m);
    }
    // Destroy all
    for (int i = 0; i < 10; i++) {
        Object_Destroy(objs[i]);
    }
    PASS();
}

static void test_self_ref_dispatch_then_unref(void) {
    TEST("self: dispatch via ref, then unref cleans up");
    ObjectReference ref = Object_CreateRef(CID_Counter);

    // Use the object through its ref
    for (int i = 0; i < 10; i++) {
        MessagePayload m = PrepareSelfPayload(ref, MID_Counter_SELF_Increment);
        DispatchMessage(&m);
        ASSERT(MESSAGE_RESULT_ISOK(m.result));
        FreePayload(&m);
    }

    MessagePayload mg = PrepareSelfPayload(ref, MID_Counter_SELF_GetCount);
    DispatchMessage(&mg);
    ASSERT(Payload_GetDeref(&mg, "result", int) == 10);
    FreePayload(&mg);

    // Unref destroys (SELF_Destroy runs, frees data)
    ObjectContainer_UnRef(&ref);
    ASSERT(ref == NULL);
    PASS();
}

static void test_self_payload_freed_on_all_paths(void) {
    TEST("self: FreePayload works on dispatched and undispatched payloads");
    TempObjectReference obj = Object_Create(CID_Counter);

    // Dispatched payload
    MessagePayload m1 = _counter_dispatch(obj, MID_Counter_SELF_Increment);
    ASSERT(m1.data != NULL);
    FreePayload(&m1);

    // Undispatched payload (just prepared, never sent)
    MessagePayload m2 = PrepareSelfPayload(obj, MID_Counter_SELF_GetCount);
    ASSERT(m2.data != NULL);
    FreePayload(&m2);

    Object_Destroy(obj);
    PASS();
}

static void test_self_ghost_step_by_step_free(void) {
    TEST("self: step-by-step lifecycle frees correctly");
    // Ghost
    TempObjectReference obj = ObjectContainer_CreateGhost();
    ASSERT(obj != NULL);

    // Type
    ObjectContainer_TypeEmptyUntyped(obj, CID_Counter);
    ASSERT(obj->cid == CID_Counter);
    ASSERT(obj->data == NULL);

    // Fill (allocs data + values + references + dispatches SELF_Create)
    ObjectContainer_FillEmptyTyped(obj);
    ASSERT(obj->data != NULL);
    ASSERT(obj->data->values != NULL);
    ASSERT(obj->data->references != NULL);

    // Use it
    MessagePayload m = _counter_dispatch(obj, MID_Counter_SELF_Increment);
    ASSERT(Payload_GetDeref(&m, "result", int) == 1);
    FreePayload(&m);

    // Empty (dispatches SELF_Destroy, frees data)
    ObjectContainer_EmptyFilledTyped(obj);
    ASSERT(obj->data == NULL);

    // Untype
    ObjectContainer_UntypeEmptyTyped(obj);
    ASSERT(obj->cid == CID_Untyped);

    // Destroy ghost (frees container)
    ObjectContainer_DestroyGhost(obj);
    PASS();
}

// ============================================================
// Held reference cleanup tests
// ============================================================

static void test_self_empty_unrefs_held_references(void) {
    TEST("self: emptying object unrefs its held references");
    // Create two objects: holder and target
    TempObjectReference holder = Object_Create(CID_Counter);
    TempObjectReference target = Object_Create(CID_Counter);

    // Take a ref to target and store it in holder's references hashmap
    ObjectReference target_ref = ObjectContainer_Ref_From_TempRef(target);
    ASSERT(target->reference_counter == 1);

    UnsafeHashMap_SSet(holder->data->references, "target", &target_ref);

    // Empty the holder -- should UnRef the held reference to target
    Object_EmptyFilledType(ObjectContainer_Ref_From_TempRef(holder));
    // target's refcount should have dropped to 0, meaning it was auto-destroyed
    // holder is now empty but still typed
    ASSERT(holder->data == NULL);
    ASSERT(holder->cid == CID_Counter);

    // Clean up holder (empty+untype+destroy)
    ObjectContainer_UntypeEmptyTyped(holder);
    ObjectContainer_DestroyGhost(holder);
    PASS();
}

static void test_self_empty_unrefs_multiple_held(void) {
    TEST("self: emptying object unrefs multiple held references");
    TempObjectReference holder = Object_Create(CID_Counter);

    // Create 5 targets, ref them, store in holder
    TempObjectReference targets[5];
    for (int i = 0; i < 5; i++) {
        targets[i] = Object_Create(CID_Counter);
        ObjectReference ref = ObjectContainer_Ref_From_TempRef(targets[i]);
        char key[16];
        snprintf(key, sizeof(key), "t%d", i);
        UnsafeHashMap_Set(holder->data->references, key, (uint32_t)strlen(key), &ref);
    }

    // All targets have refcount 1
    for (int i = 0; i < 5; i++) {
        ASSERT(targets[i]->reference_counter == 1);
    }

    // Empty holder -- all 5 refs should be unref'd, destroying all targets
    Object_EmptyFilledType(ObjectContainer_Ref_From_TempRef(holder));

    // Clean up holder
    ObjectContainer_UntypeEmptyTyped(holder);
    ObjectContainer_DestroyGhost(holder);
    PASS();
}

static void test_self_empty_unrefs_shared_target(void) {
    TEST("self: emptying with shared ref only decrements, not destroys");
    TempObjectReference holder = Object_Create(CID_Counter);
    TempObjectReference target = Object_Create(CID_Counter);

    // Two refs to target: one held by holder, one external
    ObjectReference ext_ref = ObjectContainer_Ref_From_TempRef(target);
    ObjectReference held_ref = ObjectContainer_Ref_From_TempRef(target);
    ASSERT(target->reference_counter == 2);

    UnsafeHashMap_SSet(holder->data->references, "shared", &held_ref);

    // Empty holder -- decrements target refcount but doesn't destroy it
    Object_EmptyFilledType(ObjectContainer_Ref_From_TempRef(holder));
    ASSERT(target->reference_counter == 1);
    ASSERT(target->data != NULL); // still alive

    // Clean up
    ObjectContainer_UntypeEmptyTyped(holder);
    ObjectContainer_DestroyGhost(holder);
    ObjectContainer_UnRef(&ext_ref); // last ref, destroys target
    ASSERT(ext_ref == NULL);
    PASS();
}

static void test_self_unref_with_held_refs_cascades(void) {
    TEST("self: UnRef on holder cascades to held refs");
    ObjectReference holder_ref = Object_CreateRef(CID_Counter);
    TempObjectReference target = Object_Create(CID_Counter);

    ObjectReference target_ref = ObjectContainer_Ref_From_TempRef(target);
    ASSERT(target->reference_counter == 1);

    UnsafeHashMap_SSet(holder_ref->data->references, "child", &target_ref);

    // UnRef holder -- destroys holder, which empties it, which unrefs target
    ObjectContainer_UnRef(&holder_ref);
    ASSERT(holder_ref == NULL);
    // target was also destroyed (refcount went to 0)
    PASS();
}

// ============================================================
// Node tree tests
// ============================================================

static void test_node_single(void) {
    TEST("node: single node holds value");
    TempObjectReference n = _node_create(42);
    ASSERT(n != NULL);
    ASSERT(n->data != NULL);
    MessagePayload m = PrepareSelfPayload(n, MID_Node_SELF_GetValue);
    DispatchMessage(&m);
    ASSERT(Payload_GetDeref(&m, "result", int) == 42);
    FreePayload(&m);
    Object_Destroy(n);
    PASS();
}

static void test_node_sum_single(void) {
    TEST("node: SumTree on leaf returns own value");
    TempObjectReference n = _node_create(10);
    ASSERT(_node_sum(n) == 10);
    Object_Destroy(n);
    PASS();
}

static void test_node_two_children(void) {
    TEST("node: parent with two children sums correctly");
    //       root(10)
    //      /        \
    //  left(20)  right(30)
    TempObjectReference root = _node_create(10);
    TempObjectReference left = _node_create(20);
    TempObjectReference right = _node_create(30);

    ObjectReference left_ref = ObjectContainer_Ref_From_TempRef(left);
    ObjectReference right_ref = ObjectContainer_Ref_From_TempRef(right);

    _node_set_child(root, MID_Node_SELF_SetLeft, left_ref);
    _node_set_child(root, MID_Node_SELF_SetRight, right_ref);

    ASSERT(_node_sum(root) == 60); // 10 + 20 + 30

    // Destroy root -- should unref children
    Object_Destroy(root);
    // Children still alive via our local refs
    ASSERT(left->reference_counter == 1);
    ASSERT(right->reference_counter == 1);
    ObjectContainer_UnRef(&left_ref);
    ObjectContainer_UnRef(&right_ref);
    PASS();
}

static void test_node_deep_tree(void) {
    TEST("node: deep tree sums all nodes");
    //         root(1)
    //        /
    //      a(2)
    //     /
    //   b(3)
    //  /
    // c(4)
    TempObjectReference c = _node_create(4);
    TempObjectReference b = _node_create(3);
    TempObjectReference a = _node_create(2);
    TempObjectReference root = _node_create(1);

    ObjectReference c_ref = ObjectContainer_Ref_From_TempRef(c);
    ObjectReference b_ref = ObjectContainer_Ref_From_TempRef(b);
    ObjectReference a_ref = ObjectContainer_Ref_From_TempRef(a);

    _node_set_child(b, MID_Node_SELF_SetLeft, c_ref);
    _node_set_child(a, MID_Node_SELF_SetLeft, b_ref);
    _node_set_child(root, MID_Node_SELF_SetLeft, a_ref);

    ASSERT(_node_sum(root) == 10); // 1+2+3+4

    Object_Destroy(root);
    // a still alive (our ref + nothing else since root unref'd)
    // but root's unref of a cascaded: a unref'd b, b unref'd c
    // Our local refs still hold them
    ObjectContainer_UnRef(&a_ref);
    ObjectContainer_UnRef(&b_ref);
    ObjectContainer_UnRef(&c_ref);
    PASS();
}

static void test_node_full_binary_tree(void) {
    TEST("node: full binary tree of 7 nodes");
    //           root(1)
    //          /       \
    //       l(2)       r(3)
    //      /   \      /   \
    //   ll(4) lr(5) rl(6) rr(7)
    TempObjectReference ll = _node_create(4);
    TempObjectReference lr = _node_create(5);
    TempObjectReference rl = _node_create(6);
    TempObjectReference rr = _node_create(7);
    TempObjectReference l  = _node_create(2);
    TempObjectReference r  = _node_create(3);
    TempObjectReference root = _node_create(1);

    ObjectReference ll_ref = ObjectContainer_Ref_From_TempRef(ll);
    ObjectReference lr_ref = ObjectContainer_Ref_From_TempRef(lr);
    ObjectReference rl_ref = ObjectContainer_Ref_From_TempRef(rl);
    ObjectReference rr_ref = ObjectContainer_Ref_From_TempRef(rr);
    ObjectReference l_ref  = ObjectContainer_Ref_From_TempRef(l);
    ObjectReference r_ref  = ObjectContainer_Ref_From_TempRef(r);

    _node_set_child(l, MID_Node_SELF_SetLeft, ll_ref);
    _node_set_child(l, MID_Node_SELF_SetRight, lr_ref);
    _node_set_child(r, MID_Node_SELF_SetLeft, rl_ref);
    _node_set_child(r, MID_Node_SELF_SetRight, rr_ref);
    _node_set_child(root, MID_Node_SELF_SetLeft, l_ref);
    _node_set_child(root, MID_Node_SELF_SetRight, r_ref);

    ASSERT(_node_sum(root) == 28); // 1+2+3+4+5+6+7

    // Destroy root, then unref all local refs
    Object_Destroy(root);
    ObjectContainer_UnRef(&l_ref);
    ObjectContainer_UnRef(&r_ref);
    ObjectContainer_UnRef(&ll_ref);
    ObjectContainer_UnRef(&lr_ref);
    ObjectContainer_UnRef(&rl_ref);
    ObjectContainer_UnRef(&rr_ref);
    PASS();
}

static void test_node_shared_child(void) {
    TEST("node: two parents share same child");
    //  parent1(10)   parent2(20)
    //       \         /
    //      shared(5)
    TempObjectReference shared = _node_create(5);
    TempObjectReference p1 = _node_create(10);
    TempObjectReference p2 = _node_create(20);

    ObjectReference shared_ref = ObjectContainer_Ref_From_TempRef(shared);

    _node_set_child(p1, MID_Node_SELF_SetRight, shared_ref);
    _node_set_child(p2, MID_Node_SELF_SetLeft, shared_ref);

    // shared has 3 refs: our local + p1's + p2's
    ASSERT(shared->reference_counter == 3);

    ASSERT(_node_sum(p1) == 15); // 10 + 5
    ASSERT(_node_sum(p2) == 25); // 20 + 5

    // Destroy p1 -- unrefs shared, but shared still alive (2 refs left)
    Object_Destroy(p1);
    ASSERT(shared->reference_counter == 2);

    // Destroy p2 -- unrefs shared, still alive (1 ref: our local)
    Object_Destroy(p2);
    ASSERT(shared->reference_counter == 1);
    ASSERT(shared->data != NULL); // still alive

    ObjectContainer_UnRef(&shared_ref); // last ref, destroys shared
    ASSERT(shared_ref == NULL);
    PASS();
}

static void test_node_cascade_destroy_via_unref(void) {
    TEST("node: sole-owner tree destroyed by single unref");
    //      root(1)
    //     /       \
    //   l(2)     r(3)
    TempObjectReference root = _node_create(1);
    TempObjectReference l = _node_create(2);
    TempObjectReference r = _node_create(3);

    // Only one ref each, held by root
    ObjectReference l_ref = ObjectContainer_Ref_From_TempRef(l);
    ObjectReference r_ref = ObjectContainer_Ref_From_TempRef(r);
    _node_set_child(root, MID_Node_SELF_SetLeft, l_ref);
    _node_set_child(root, MID_Node_SELF_SetRight, r_ref);

    // Verify tree works
    ASSERT(_node_sum(root) == 6);

    // Take a single ref to root
    ObjectReference root_ref = ObjectContainer_Ref_From_TempRef(root);

    // Unref root -- should cascade: root destroys -> unrefs l and r -> they destroy
    ObjectContainer_UnRef(&root_ref);
    ASSERT(root_ref == NULL);
    // l and r also destroyed (their only refs were from root + our locals)
    // Our local ObjectReference vars were stored in root's refs, which got unref'd
    // But l_ref and r_ref are still holding... let's unref those too
    ObjectContainer_UnRef(&l_ref);
    ObjectContainer_UnRef(&r_ref);
    PASS();
}

// ============================================================
// Self_IsRefSelf tests
// ============================================================

static void test_node_self_ref_sum_skips(void) {
    TEST("node: SumTree skips self-referencing children");
    TempObjectReference a = _node_create(42);
    ObjectReference a_ref = ObjectContainer_Ref_From_TempRef(a);
    _node_set_child(a, MID_Node_SELF_SetLeft, a_ref);

    // SumTree should return just 42, not infinite recurse
    ASSERT(_node_sum(a) == 42);
    Object_Destroy(a);
    PASS();
}

static void test_node_self_ref_both_children(void) {
    TEST("node: SumTree with both children pointing to self");
    TempObjectReference a = _node_create(10);
    ObjectReference ref1 = ObjectContainer_Ref_From_TempRef(a);
    ObjectReference ref2 = ObjectContainer_Ref_From_TempRef(a);
    _node_set_child(a, MID_Node_SELF_SetLeft, ref1);
    _node_set_child(a, MID_Node_SELF_SetRight, ref2);

    ASSERT(_node_sum(a) == 10);
    Object_Destroy(a);
    PASS();
}

static void test_node_self_ref_mixed(void) {
    TEST("node: SumTree with one self-ref and one real child");
    TempObjectReference a = _node_create(10);
    TempObjectReference b = _node_create(20);

    ObjectReference a_self = ObjectContainer_Ref_From_TempRef(a);
    ObjectReference b_ref = ObjectContainer_Ref_From_TempRef(b);
    _node_set_child(a, MID_Node_SELF_SetLeft, a_self);
    _node_set_child(a, MID_Node_SELF_SetRight, b_ref);

    ASSERT(_node_sum(a) == 30); // 10 + 20, left self-ref skipped
    Object_Destroy(a);
    ObjectContainer_UnRef(&b_ref);
    PASS();
}

// ============================================================
// Cycle and self-reference tests
// ============================================================

static void test_cycle_ab_empty_a(void) {
    TEST("cycle: A<->B, empty A cascades to destroy B");
    // Create A and B
    TempObjectReference a = _node_create(1);
    TempObjectReference b = _node_create(2);

    // Stack ref to A
    ObjectReference a_stack = ObjectContainer_Ref_From_TempRef(a);
    // A holds ref to B
    ObjectReference b_from_a = ObjectContainer_Ref_From_TempRef(b);
    UnsafeHashMap_SSet(a->data->references, "b", &b_from_a);
    // B holds ref to A
    ObjectReference a_from_b = ObjectContainer_Ref_From_TempRef(a);
    UnsafeHashMap_SSet(b->data->references, "a", &a_from_b);

    // A: refcount=2 (stack + B), B: refcount=1 (A)
    ASSERT(a->reference_counter == 2);
    ASSERT(b->reference_counter == 1);

    // Empty A -- should cascade: A unrefs B, B refcount->0, B empties, B unrefs A, A refcount->1
    Object_EmptyFilledType(a_stack);
    ASSERT(a->reference_counter == 1);
    ASSERT(a->data == NULL); // A is emptied

    // B was fully destroyed (refcount hit 0 during A's unref loop)
    // A is still alive via stack ref, empty+typed
    ObjectContainer_UntypeEmptyTyped(a);
    ObjectContainer_DestroyGhost(a);
    // a_stack was not UnRef'd, just used for Object_EmptyFilledType which takes ObjectReference
    // but the container was freed by DestroyGhost, so don't touch a_stack
    PASS();
}

static void test_cycle_self_ref_empty(void) {
    TEST("cycle: A self-ref, empty A drops self-ref correctly");
    TempObjectReference a = _node_create(1);

    // Stack ref
    ObjectReference a_stack = ObjectContainer_Ref_From_TempRef(a);
    // Self-ref
    ObjectReference a_self = ObjectContainer_Ref_From_TempRef(a);
    UnsafeHashMap_SSet(a->data->references, "self", &a_self);

    // A: refcount=2 (stack + self)
    ASSERT(a->reference_counter == 2);

    // Empty A -- unrefs self, refcount->1 (stack). No crash, no recursion.
    Object_EmptyFilledType(a_stack);
    ASSERT(a->reference_counter == 1);
    ASSERT(a->data == NULL);

    // Clean up
    ObjectContainer_UntypeEmptyTyped(a);
    ObjectContainer_DestroyGhost(a);
    PASS();
}

static void test_cycle_ab_drop_stack_ref(void) {
    TEST("cycle: A<->B, drop stack ref leaves disconnected cycle");
    TempObjectReference a = _node_create(1);
    TempObjectReference b = _node_create(2);

    // Stack ref to A
    ObjectReference a_stack = ObjectContainer_Ref_From_TempRef(a);
    // A holds ref to B
    ObjectReference b_from_a = ObjectContainer_Ref_From_TempRef(b);
    UnsafeHashMap_SSet(a->data->references, "b", &b_from_a);
    // B holds ref to A
    ObjectReference a_from_b = ObjectContainer_Ref_From_TempRef(a);
    UnsafeHashMap_SSet(b->data->references, "a", &a_from_b);

    // A: refcount=2 (stack + B), B: refcount=1 (A)
    ASSERT(a->reference_counter == 2);
    ASSERT(b->reference_counter == 1);

    // Drop stack ref -- A refcount->1, no cleanup triggered
    ObjectContainer_UnRef(&a_stack);
    ASSERT(a_stack == NULL);
    ASSERT(a->reference_counter == 1); // only B's ref
    ASSERT(b->reference_counter == 1); // only A's ref
    // Both still alive -- disconnected cycle (known leak, handled later)

    // Manual cleanup to avoid actual leak in test
    // Break the cycle by emptying A directly via temp ref
    ObjectContainer_EmptyFilledTyped(a);
    // That cascaded: A unrefs B, B->0, B empties, B unrefs A, A->0
    // Both are now empty. A was freed by UnRef cascade? Let's check.
    // Actually: A emptied -> unrefs B -> B refcount 0 -> B empties -> B unrefs A -> A refcount 0
    // A refcount 0 triggers: A->data is already NULL (we just emptied it), so skip empty.
    // A->cid != Untyped, so untype. Then free(A).
    // B similarly freed.
    // Both are gone. Test done.
    PASS();
}

static void test_cycle_self_ref_drop_stack(void) {
    TEST("cycle: A self-ref, drop stack ref leaves disconnected self-loop");
    TempObjectReference a = _node_create(1);

    ObjectReference a_stack = ObjectContainer_Ref_From_TempRef(a);
    ObjectReference a_self = ObjectContainer_Ref_From_TempRef(a);
    UnsafeHashMap_SSet(a->data->references, "self", &a_self);

    // A: refcount=2 (stack + self)
    ASSERT(a->reference_counter == 2);

    // Drop stack ref -- refcount->1, no cleanup
    ObjectContainer_UnRef(&a_stack);
    ASSERT(a_stack == NULL);
    ASSERT(a->reference_counter == 1); // only self-ref
    // Disconnected self-loop (known leak)

    // Manual cleanup: empty directly
    ObjectContainer_EmptyFilledTyped(a);
    // A emptied -> unrefs self -> refcount 0 -> A->data already NULL -> skip empty -> untype -> free
    PASS();
}

static void test_cycle_triangle_empty_a(void) {
    TEST("cycle: A->B->C->A triangle, empty A cascades all");
    TempObjectReference a = _node_create(1);
    TempObjectReference b = _node_create(2);
    TempObjectReference c = _node_create(3);

    ObjectReference a_stack = ObjectContainer_Ref_From_TempRef(a);

    ObjectReference b_ref = ObjectContainer_Ref_From_TempRef(b);
    UnsafeHashMap_SSet(a->data->references, "next", &b_ref);

    ObjectReference c_ref = ObjectContainer_Ref_From_TempRef(c);
    UnsafeHashMap_SSet(b->data->references, "next", &c_ref);

    ObjectReference a_ref = ObjectContainer_Ref_From_TempRef(a);
    UnsafeHashMap_SSet(c->data->references, "next", &a_ref);

    // A: refcount=2 (stack + C), B: refcount=1 (A), C: refcount=1 (B)
    ASSERT(a->reference_counter == 2);
    ASSERT(b->reference_counter == 1);
    ASSERT(c->reference_counter == 1);

    // Refs are under "next", not "left"/"right", so SumTree won't recurse into them.

    // Empty A -- cascades: A unrefs B->0, B empties, B unrefs C->0, C empties, C unrefs A->1
    Object_EmptyFilledType(a_stack);
    ASSERT(a->reference_counter == 1);
    ASSERT(a->data == NULL);

    // B and C fully destroyed via cascade
    // Clean up A
    ObjectContainer_UntypeEmptyTyped(a);
    ObjectContainer_DestroyGhost(a);
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

    LOG_INFO("=== Held Reference Cleanup Tests ===");
    test_self_empty_unrefs_held_references();
    test_self_empty_unrefs_multiple_held();
    test_self_empty_unrefs_shared_target();
    test_self_unref_with_held_refs_cascades();

    LOG_INFO("=== Node Tree Tests ===");
    test_node_single();
    test_node_sum_single();
    test_node_two_children();
    test_node_deep_tree();
    test_node_full_binary_tree();
    test_node_shared_child();
    test_node_cascade_destroy_via_unref();

    LOG_INFO("=== Self_IsRefSelf Tests ===");
    test_node_self_ref_sum_skips();
    test_node_self_ref_both_children();
    test_node_self_ref_mixed();

    LOG_INFO("=== Cycle and Self-Reference Tests ===");
    test_cycle_ab_empty_a();
    test_cycle_self_ref_empty();
    test_cycle_ab_drop_stack_ref();
    test_cycle_self_ref_drop_stack();
    test_cycle_triangle_empty_a();

    LOG_INFO("=== Memory Management Tests ===");
    test_self_create_destroy_many();
    test_self_create_ref_unref_many();
    test_self_many_refs_one_object();
    test_self_dispatch_after_multiple_create();
    test_self_ref_dispatch_then_unref();
    test_self_payload_freed_on_all_paths();
    test_self_ghost_step_by_step_free();
}
