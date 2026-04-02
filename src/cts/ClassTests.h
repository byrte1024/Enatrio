#pragma once

#include "../tests.h"
#include "Class.h"

// -- Test class: Calculator --

#define TYPE Calculator

BEGIN_CLASS(0x0001);

DECLARE_MID(Add);
DECLARE_MID(Sub);
DECLARE_MID(Mul);
DECLARE_MID(AddInPlace);
DECLARE_MID(AddResult);

// Add: reads a,b values, writes to out pointer
MESSAGE_HANDLER_BEGIN(Add)
    MH_REQUIRE_VALUE(a, int);
    MH_REQUIRE_VALUE(b, int);
    MH_REQUIRE_STORED_PTR(out, int);
    *MH_out_Ptr = MH_a_Val + MH_b_Val;
MESSAGE_HANDLER_END()

MESSAGE_HANDLER_BEGIN(Sub)
    MH_REQUIRE_VALUE(a, int);
    MH_REQUIRE_VALUE(b, int);
    MH_REQUIRE_STORED_PTR(out, int);
    *MH_out_Ptr = MH_a_Val - MH_b_Val;
MESSAGE_HANDLER_END()

MESSAGE_HANDLER_BEGIN(Mul)
    MH_REQUIRE_VALUE(a, int);
    MH_REQUIRE_VALUE(b, int);
    MH_REQUIRE_STORED_PTR(out, int);
    *MH_out_Ptr = MH_a_Val * MH_b_Val;
MESSAGE_HANDLER_END()

// AddInPlace: takes a pointer to target, adds b value into it
MESSAGE_HANDLER_BEGIN(AddInPlace)
    MH_REQUIRE_STORED_PTR(target, int);
    MH_REQUIRE_VALUE(b, int);
    *MH_target_Ptr += MH_b_Val;
MESSAGE_HANDLER_END()

// AddResult: reads a,b values, writes result back into the payload dict
MESSAGE_HANDLER_BEGIN(AddResult)
    MH_REQUIRE_VALUE(a, int);
    MH_REQUIRE_VALUE(b, int);
    int _sum = MH_a_Val + MH_b_Val;
    UnsafeVariedHashMap_SSet(payload->data, "result", &_sum, sizeof(int));
MESSAGE_HANDLER_END()

CAN_RECEIVE_BEGIN()
    CAN_RECEIVE_MID(Add)
    CAN_RECEIVE_MID(Sub)
    CAN_RECEIVE_MID(Mul)
    CAN_RECEIVE_MID(AddInPlace)
    CAN_RECEIVE_MID(AddResult)
CAN_RECEIVE_END()

RECEIVE_MESSAGE_BEGIN()
    RECEIVE_MESSAGE_ROUTE(Add)
    RECEIVE_MESSAGE_ROUTE(Sub)
    RECEIVE_MESSAGE_ROUTE(Mul)
    RECEIVE_MESSAGE_ROUTE(AddInPlace)
    RECEIVE_MESSAGE_ROUTE(AddResult)
RECEIVE_MESSAGE_END()

CLASSDEF()

#undef TYPE

// -- Registration tests --

static void test_class_register_calculator(void) {
    TEST("class: register Calculator");
    BeginClassRegistrations();
    RegisterClass(Calculator_ClassDef());
    EndClassRegistrations();
    ASSERT(CLASSID_ISREGISTERED(CID_Calculator));
    ASSERT(strcmp(ClassDefinitions[CID_Calculator].classname, "Calculator") == 0);
    PASS();
}

static void test_class_untyped_not_registered(void) {
    TEST("class: CID 0 is untyped");
    ASSERT(CLASSID_ISUNTYPED(0));
    // CLASSID_ISREGISTERED(0) is trivially true (zeroed cid matches 0),
    // but the class system rejects it via CLASSID_ISUNTYPED checks.
    ASSERT(ClassDefinitions[0].CanReceiveMID == NULL);
    ASSERT(ClassDefinitions[0].ReceiveMessage == NULL);
    PASS();
}

static void test_class_unregistered_cid(void) {
    TEST("class: unregistered CID returns false");
    ASSERT(!CLASSID_ISREGISTERED(9999));
    PASS();
}

// -- Dispatch tests --

static void test_class_dispatch_add(void) {
    TEST("class: dispatch Calculator.Add");
    int result = 0;
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Add);
    Payload_SetValue(&msg, "a", int, 10);
    Payload_SetValue(&msg, "b", int, 25);
    Payload_SetPointer(&msg, "out", &result);
    DispatchMessage(&msg);
    ASSERT(MESSAGE_RESULT_ISOK(msg.result));
    ASSERT(result == 35);
    FreePayload(&msg);
    PASS();
}

static void test_class_dispatch_sub(void) {
    TEST("class: dispatch Calculator.Sub");
    int result = 0;
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Sub);
    Payload_SetValue(&msg, "a", int, 100);
    Payload_SetValue(&msg, "b", int, 37);
    Payload_SetPointer(&msg, "out", &result);
    DispatchMessage(&msg);
    ASSERT(MESSAGE_RESULT_ISOK(msg.result));
    ASSERT(result == 63);
    FreePayload(&msg);
    PASS();
}

static void test_class_dispatch_mul(void) {
    TEST("class: dispatch Calculator.Mul");
    int result = 0;
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Mul);
    Payload_SetValue(&msg, "a", int, 6);
    Payload_SetValue(&msg, "b", int, 7);
    Payload_SetPointer(&msg, "out", &result);
    DispatchMessage(&msg);
    ASSERT(MESSAGE_RESULT_ISOK(msg.result));
    ASSERT(result == 42);
    FreePayload(&msg);
    PASS();
}

static void test_class_dispatch_negative_result(void) {
    TEST("class: Sub with negative result");
    int result = 0;
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Sub);
    Payload_SetValue(&msg, "a", int, 3);
    Payload_SetValue(&msg, "b", int, 10);
    Payload_SetPointer(&msg, "out", &result);
    DispatchMessage(&msg);
    ASSERT(MESSAGE_RESULT_ISOK(msg.result));
    ASSERT(result == -7);
    FreePayload(&msg);
    PASS();
}

static void test_class_dispatch_zeroes(void) {
    TEST("class: Add with zeroes");
    int result = 99;
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Add);
    Payload_SetValue(&msg, "a", int, 0);
    Payload_SetValue(&msg, "b", int, 0);
    Payload_SetPointer(&msg, "out", &result);
    DispatchMessage(&msg);
    ASSERT(MESSAGE_RESULT_ISOK(msg.result));
    ASSERT(result == 0);
    FreePayload(&msg);
    PASS();
}

// -- Pointer and dict-output tests --

static void test_class_dispatch_add_in_place(void) {
    TEST("class: AddInPlace modifies target pointer");
    int accumulator = 50;
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_AddInPlace);
    Payload_SetPointer(&msg, "target", &accumulator);
    Payload_SetValue(&msg, "b", int, 30);
    DispatchMessage(&msg);
    ASSERT(MESSAGE_RESULT_ISOK(msg.result));
    ASSERT(accumulator == 80);
    FreePayload(&msg);
    PASS();
}

static void test_class_dispatch_add_in_place_multiple(void) {
    TEST("class: AddInPlace called multiple times on same target");
    int total = 0;
    for (int i = 0; i < 5; i++) {
        MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_AddInPlace);
        Payload_SetPointer(&msg, "target", &total);
        Payload_SetValue(&msg, "b", int, 10);
        DispatchMessage(&msg);
        ASSERT(MESSAGE_RESULT_ISOK(msg.result));
        FreePayload(&msg);
    }
    ASSERT(total == 50);
    PASS();
}

static void test_class_dispatch_add_result_in_dict(void) {
    TEST("class: AddResult writes result into payload dict");
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_AddResult);
    Payload_SetValue(&msg, "a", int, 17);
    Payload_SetValue(&msg, "b", int, 25);
    DispatchMessage(&msg);
    ASSERT(MESSAGE_RESULT_ISOK(msg.result));
    // handler wrote "result" into the dict, read it back
    ASSERT(UnsafeVariedHashMap_SHas(msg.data, "result"));
    int sum = UnsafeVariedHashMap_SGetValue(msg.data, "result", int);
    ASSERT(sum == 42);
    FreePayload(&msg);
    PASS();
}

static void test_class_dispatch_add_result_missing_key(void) {
    TEST("class: AddResult has no result key before dispatch");
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_AddResult);
    Payload_SetValue(&msg, "a", int, 1);
    Payload_SetValue(&msg, "b", int, 2);
    ASSERT(!UnsafeVariedHashMap_SHas(msg.data, "result"));
    DispatchMessage(&msg);
    ASSERT(UnsafeVariedHashMap_SHas(msg.data, "result"));
    ASSERT(UnsafeVariedHashMap_SGetValue(msg.data, "result", int) == 3);
    FreePayload(&msg);
    PASS();
}

static void test_class_dispatch_pointer_modify_external(void) {
    TEST("class: pointer lets handler modify caller's struct");
    typedef struct { int x; int y; } Point;
    Point p = { 10, 20 };
    // Use AddInPlace to modify p.x via pointer
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_AddInPlace);
    Payload_SetPointer(&msg, "target", &p.x);
    Payload_SetValue(&msg, "b", int, 5);
    DispatchMessage(&msg);
    ASSERT(MESSAGE_RESULT_ISOK(msg.result));
    ASSERT(p.x == 15);
    ASSERT(p.y == 20); // y untouched
    FreePayload(&msg);
    PASS();
}

// -- Error handling tests --

static void test_class_dispatch_missing_params(void) {
    TEST("class: dispatch with missing params");
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Add);
    Payload_SetValue(&msg, "a", int, 5);
    // missing "b" and "out"
    DispatchMessage(&msg);
    ASSERT(msg.result == MESSAGE_RESULT_MISSING_PARAMS);
    FreePayload(&msg);
    PASS();
}

static void test_class_dispatch_invalid_cid(void) {
    TEST("class: dispatch to unregistered CID");
    MessagePayload msg = PreparePayload(9999, MID_Calculator_Add);
    DispatchMessage(&msg);
    ASSERT(msg.result == MESSAGE_RESULT_INVALID_CID);
    FreePayload(&msg);
    PASS();
}

static void test_class_dispatch_untyped_cid(void) {
    TEST("class: dispatch to untyped CID 0");
    MessagePayload msg = PreparePayload(0, MID_Calculator_Add);
    DispatchMessage(&msg);
    ASSERT(msg.result == MESSAGE_RESULT_INVALID_CID);
    FreePayload(&msg);
    PASS();
}

static void test_class_dispatch_unsupported_mid(void) {
    TEST("class: dispatch unsupported MID");
    MessageID bad_mid = "Calculator.Divide";
    MessagePayload msg = PreparePayload(CID_Calculator, bad_mid);
    DispatchMessage(&msg);
    ASSERT(msg.result == MESSAGE_RESULT_NOT_SUPPORTED);
    FreePayload(&msg);
    PASS();
}

static void test_class_dispatch_empty_mid(void) {
    TEST("class: dispatch empty MID");
    MessageID empty_mid = "";
    MessagePayload msg = PreparePayload(CID_Calculator, empty_mid);
    DispatchMessage(&msg);
    ASSERT(msg.result == MESSAGE_RESULT_INVALID_MID);
    FreePayload(&msg);
    PASS();
}

static void test_class_dispatch_null_payload(void) {
    TEST("class: dispatch NULL payload");
    MessagePayload *ret = DispatchMessage(NULL);
    ASSERT(ret == NULL);
    PASS();
}

// -- Payload tests --

static void test_class_payload_initial_state(void) {
    TEST("class: payload initial result is NOTSENT");
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Add);
    ASSERT(msg.result == MESSAGE_RESULT_NOTSENT);
    ASSERT(msg.data != NULL);
    ASSERT(msg.cid_target == CID_Calculator);
    ASSERT(strcmp(msg.mid, MID_Calculator_Add) == 0);
    FreePayload(&msg);
    PASS();
}

static void test_class_free_payload_safe(void) {
    TEST("class: FreePayload on already-freed data");
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Add);
    FreePayload(&msg);
    msg.data = NULL;
    FreePayload(&msg);
    PASS();
}

// -- Payload macro tests --

static void test_class_payload_set_value(void) {
    TEST("class: Payload_SetValue stores value by copy");
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Add);
    Payload_SetValue(&msg, "x", int, 77);
    int *got = (int*)UnsafeVariedHashMap_SGet(msg.data, "x");
    ASSERT(got != NULL);
    ASSERT(*got == 77);
    FreePayload(&msg);
    PASS();
}

static void test_class_payload_set_pointer(void) {
    TEST("class: Payload_SetPointer stores pointer");
    int val = 42;
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Add);
    Payload_SetPointer(&msg, "p", &val);
    int *got = (int*)UnsafeVariedHashMap_SGetValue(msg.data, "p", void*);
    ASSERT(got == &val);
    ASSERT(*got == 42);
    FreePayload(&msg);
    PASS();
}

static void test_class_payload_set_value_dispatch(void) {
    TEST("class: Payload_SetValue survives to dispatch");
    int result = 0;
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Add);
    Payload_SetValue(&msg, "a", int, 30);
    Payload_SetValue(&msg, "b", int, 12);
    Payload_SetPointer(&msg, "out", &result);
    DispatchMessage(&msg);
    ASSERT(MESSAGE_RESULT_ISOK(msg.result));
    ASSERT(result == 42);
    FreePayload(&msg);
    PASS();
}

static void test_class_payload_multiple_types(void) {
    TEST("class: Payload_SetValue with different types");
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Add);
    Payload_SetValue(&msg, "i", int, 10);
    Payload_SetValue(&msg, "f", float, 3.14f);
    Payload_SetValue(&msg, "c", char, 'A');

    ASSERT(UnsafeVariedHashMap_SGetValue(msg.data, "i", int) == 10);
    float fv = UnsafeVariedHashMap_SGetValue(msg.data, "f", float);
    ASSERT(fv > 3.13f && fv < 3.15f);
    ASSERT(UnsafeVariedHashMap_SGetValue(msg.data, "c", char) == 'A');

    ASSERT(UnsafeVariedHashMap_SGetSize(msg.data, "i") == sizeof(int));
    ASSERT(UnsafeVariedHashMap_SGetSize(msg.data, "f") == sizeof(float));
    ASSERT(UnsafeVariedHashMap_SGetSize(msg.data, "c") == sizeof(char));
    FreePayload(&msg);
    PASS();
}

static void test_class_payload_duplicate_key(void) {
    TEST("class: duplicate key in payload returns -1");
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Add);
    Payload_SetValue(&msg, "k", int, 5);
    int b = 10;
    int ret = UnsafeVariedHashMap_SSet(msg.data, "k", &b, sizeof(int));
    ASSERT(ret == -1);
    FreePayload(&msg);
    PASS();
}

static void test_class_payload_mixed_dispatch(void) {
    TEST("class: mix SetValue and SetPointer in dispatch");
    int result = 0;
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Add);
    Payload_SetValue(&msg, "a", int, 100);
    Payload_SetValue(&msg, "b", int, 200);
    Payload_SetPointer(&msg, "out", &result);
    DispatchMessage(&msg);
    ASSERT(MESSAGE_RESULT_ISOK(msg.result));
    ASSERT(result == 300);
    FreePayload(&msg);
    PASS();
}

// -- Runner --

static void run_class_tests(void) {
    BeginClassRegistrations();
    RegisterClass(Calculator_ClassDef());
    EndClassRegistrations();

    LOG_INFO("=== Class System Tests ===");
    test_class_register_calculator();
    test_class_untyped_not_registered();
    test_class_unregistered_cid();

    LOG_INFO("=== Dispatch Tests ===");
    test_class_dispatch_add();
    test_class_dispatch_sub();
    test_class_dispatch_mul();
    test_class_dispatch_negative_result();
    test_class_dispatch_zeroes();

    LOG_INFO("=== Pointer and Dict-Output Tests ===");
    test_class_dispatch_add_in_place();
    test_class_dispatch_add_in_place_multiple();
    test_class_dispatch_add_result_in_dict();
    test_class_dispatch_add_result_missing_key();
    test_class_dispatch_pointer_modify_external();

    LOG_INFO("=== Error Handling Tests ===");
    test_class_dispatch_missing_params();
    test_class_dispatch_invalid_cid();
    test_class_dispatch_untyped_cid();
    test_class_dispatch_unsupported_mid();
    test_class_dispatch_empty_mid();
    test_class_dispatch_null_payload();

    LOG_INFO("=== Payload Macro Tests ===");
    test_class_payload_initial_state();
    test_class_free_payload_safe();
    test_class_payload_set_value();
    test_class_payload_set_pointer();
    test_class_payload_set_value_dispatch();
    test_class_payload_multiple_types();
    test_class_payload_duplicate_key();
    test_class_payload_mixed_dispatch();
}
