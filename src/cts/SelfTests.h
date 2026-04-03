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
// Runner
// ============================================================

static void run_self_tests(void) {
    BeginClassRegistrations();
    RegisterClass(Counter_ClassDef());
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

    LOG_INFO("=== Memory Management Tests ===");
    test_self_create_destroy_many();
    test_self_create_ref_unref_many();
    test_self_many_refs_one_object();
    test_self_dispatch_after_multiple_create();
    test_self_ref_dispatch_then_unref();
    test_self_payload_freed_on_all_paths();
    test_self_ghost_step_by_step_free();
}
