#pragma once

#include "../tests.h"
#include "class.h"

// -- Test class: Calculator --

#define CID_CALCULATOR ((ClassID)(1))

static MessageID MID_CALCULATOR_ADD = "Calculator.Add";
static MessageID MID_CALCULATOR_SUB = "Calculator.Sub";
static MessageID MID_CALCULATOR_MUL = "Calculator.Mul";

static bool Calculator_CanReceiveMID(MessageID mid) {
    if (strcmp(mid, MID_CALCULATOR_ADD) == 0) return true;
    if (strcmp(mid, MID_CALCULATOR_SUB) == 0) return true;
    if (strcmp(mid, MID_CALCULATOR_MUL) == 0) return true;
    return false;
}

static void Calculator_ReceiveMessage(MessagePayload* payload) {
    if (!UnsafeDictionary_SHas(payload->data_dict, "a") ||
        !UnsafeDictionary_SHas(payload->data_dict, "b") ||
        !UnsafeDictionary_SHas(payload->data_dict, "out")) {
        payload->result = MESSAGE_RESULT_MISSING_PARAMS;
        return;
    }

    int a = *(int*)UnsafeDictionary_SGetValue(payload->data_dict, "a", void*);
    int b = *(int*)UnsafeDictionary_SGetValue(payload->data_dict, "b", void*);
    int *out = (int*)UnsafeDictionary_SGetValue(payload->data_dict, "out", void*);

    if (strcmp(payload->mid, MID_CALCULATOR_ADD) == 0) {
        *out = a + b;
    } else if (strcmp(payload->mid, MID_CALCULATOR_SUB) == 0) {
        *out = a - b;
    } else if (strcmp(payload->mid, MID_CALCULATOR_MUL) == 0) {
        *out = a * b;
    } else {
        payload->result = MESSAGE_RESULT_INVALID_MID;
        return;
    }

    payload->result = MESSAGE_RESULT_SUCCESS;
}

static ClassDefinition Calculator_ClassDef(void) {
    ClassDefinition def = {0};
    def.cid = CID_CALCULATOR;
    strncpy(def.classname, "Calculator", CLASS_MAXNAMELENGTH - 1);
    def.CanReceiveMID = Calculator_CanReceiveMID;
    def.ReceiveMessage = Calculator_ReceiveMessage;
    return def;
}

// -- Registration tests --

static void test_class_register_calculator(void) {
    TEST("class: register Calculator");
    BeginClassRegistrations();
    RegisterClass(Calculator_ClassDef());
    EndClassRegistrations();
    ASSERT(CLASSID_ISREGISTERED(CID_CALCULATOR));
    ASSERT(strcmp(ClassDefinitions[CID_CALCULATOR].classname, "Calculator") == 0);
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
    MessagePayload msg = PreparePayload(CID_CALCULATOR, MID_CALCULATOR_ADD);
    Payload_SetPointerToValue(&msg, "a", int, 10);
    Payload_SetPointerToValue(&msg, "b", int, 25);
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
    MessagePayload msg = PreparePayload(CID_CALCULATOR, MID_CALCULATOR_SUB);
    Payload_SetPointerToValue(&msg, "a", int, 100);
    Payload_SetPointerToValue(&msg, "b", int, 37);
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
    MessagePayload msg = PreparePayload(CID_CALCULATOR, MID_CALCULATOR_MUL);
    Payload_SetPointerToValue(&msg, "a", int, 6);
    Payload_SetPointerToValue(&msg, "b", int, 7);
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
    MessagePayload msg = PreparePayload(CID_CALCULATOR, MID_CALCULATOR_SUB);
    Payload_SetPointerToValue(&msg, "a", int, 3);
    Payload_SetPointerToValue(&msg, "b", int, 10);
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
    MessagePayload msg = PreparePayload(CID_CALCULATOR, MID_CALCULATOR_ADD);
    Payload_SetPointerToValue(&msg, "a", int, 0);
    Payload_SetPointerToValue(&msg, "b", int, 0);
    Payload_SetPointer(&msg, "out", &result);
    DispatchMessage(&msg);
    ASSERT(MESSAGE_RESULT_ISOK(msg.result));
    ASSERT(result == 0);
    FreePayload(&msg);
    PASS();
}

// -- Error handling tests --

static void test_class_dispatch_missing_params(void) {
    TEST("class: dispatch with missing params");
    MessagePayload msg = PreparePayload(CID_CALCULATOR, MID_CALCULATOR_ADD);
    Payload_SetPointerToValue(&msg, "a", int, 5);
    // missing "b" and "out"
    DispatchMessage(&msg);
    ASSERT(msg.result == MESSAGE_RESULT_MISSING_PARAMS);
    FreePayload(&msg);
    PASS();
}

static void test_class_dispatch_invalid_cid(void) {
    TEST("class: dispatch to unregistered CID");
    MessagePayload msg = PreparePayload(9999, MID_CALCULATOR_ADD);
    DispatchMessage(&msg);
    ASSERT(msg.result == MESSAGE_RESULT_INVALID_CID);
    FreePayload(&msg);
    PASS();
}

static void test_class_dispatch_untyped_cid(void) {
    TEST("class: dispatch to untyped CID 0");
    MessagePayload msg = PreparePayload(0, MID_CALCULATOR_ADD);
    DispatchMessage(&msg);
    ASSERT(msg.result == MESSAGE_RESULT_INVALID_CID);
    FreePayload(&msg);
    PASS();
}

static void test_class_dispatch_unsupported_mid(void) {
    TEST("class: dispatch unsupported MID");
    MessageID bad_mid = "Calculator.Divide";
    MessagePayload msg = PreparePayload(CID_CALCULATOR, bad_mid);
    DispatchMessage(&msg);
    ASSERT(msg.result == MESSAGE_RESULT_NOT_SUPPORTED);
    FreePayload(&msg);
    PASS();
}

static void test_class_dispatch_empty_mid(void) {
    TEST("class: dispatch empty MID");
    MessageID empty_mid = "";
    MessagePayload msg = PreparePayload(CID_CALCULATOR, empty_mid);
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
    MessagePayload msg = PreparePayload(CID_CALCULATOR, MID_CALCULATOR_ADD);
    ASSERT(msg.result == MESSAGE_RESULT_NOTSENT);
    ASSERT(msg.data_dict != NULL);
    ASSERT(msg.cid_target == CID_CALCULATOR);
    ASSERT(strcmp(msg.mid, MID_CALCULATOR_ADD) == 0);
    FreePayload(&msg);
    PASS();
}

static void test_class_free_payload_safe(void) {
    TEST("class: FreePayload on already-freed dict");
    MessagePayload msg = PreparePayload(CID_CALCULATOR, MID_CALCULATOR_ADD);
    FreePayload(&msg);
    // data_dict is now destroyed, set to NULL manually and free again
    msg.data_dict = NULL;
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

    LOG_INFO("=== Error Handling Tests ===");
    test_class_dispatch_missing_params();
    test_class_dispatch_invalid_cid();
    test_class_dispatch_untyped_cid();
    test_class_dispatch_unsupported_mid();
    test_class_dispatch_empty_mid();
    test_class_dispatch_null_payload();

    LOG_INFO("=== Payload Tests ===");
    test_class_payload_initial_state();
    test_class_free_payload_safe();
}
