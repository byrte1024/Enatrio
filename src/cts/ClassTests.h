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
DECLARE_MID(Swap);
DECLARE_MID(Clamp);
DECLARE_MID(FMA);
DECLARE_MID(Stats);
DECLARE_MID(OptionalInc);
DECLARE_MID(RawCopy);

// Add: ExtractDeref + SetValue
MESSAGE_HANDLER_BEGIN(Add)
    MH_ExtractDeref(a, int);
    MH_ExtractDeref(b, int);
    MH_SetValue(result, int, a + b);
MESSAGE_HANDLER_END()

// Sub: ExtractDeref + SetValue
MESSAGE_HANDLER_BEGIN(Sub)
    MH_ExtractDeref(a, int);
    MH_ExtractDeref(b, int);
    MH_SetValue(result, int, a - b);
MESSAGE_HANDLER_END()

// Mul: ExtractDeref + SetValue
MESSAGE_HANDLER_BEGIN(Mul)
    MH_ExtractDeref(a, int);
    MH_ExtractDeref(b, int);
    MH_SetValue(result, int, a * b);
MESSAGE_HANDLER_END()

// AddInPlace: ExtractDeref pointer, modify caller's memory
MESSAGE_HANDLER_BEGIN(AddInPlace)
    MH_ExtractDeref(target, int*);
    MH_ExtractDeref(b, int);
    *target += b;
MESSAGE_HANDLER_END()

// Swap: Extract (pointer to stored), swap two values in-place in the payload
MESSAGE_HANDLER_BEGIN(Swap)
    MH_Extract(a, int);
    MH_Extract(b, int);
    int tmp = *a;
    *a = *b;
    *b = tmp;
MESSAGE_HANDLER_END()

// Clamp: uses MH_Get (nullable) for optional min/max with defaults
MESSAGE_HANDLER_BEGIN(Clamp)
    MH_ExtractDeref(value, int);
    int lo = 0;
    int hi = 100;
    int *lo_ptr = MH_Get(min, int);
    int *hi_ptr = MH_Get(max, int);
    if (lo_ptr) lo = *lo_ptr;
    if (hi_ptr) hi = *hi_ptr;
    int clamped = value < lo ? lo : (value > hi ? hi : value);
    MH_SetValue(result, int, clamped);
MESSAGE_HANDLER_END()

// FMA: float fused multiply-add: result = a * b + c (mixed types test)
MESSAGE_HANDLER_BEGIN(FMA)
    MH_ExtractDeref(a, float);
    MH_ExtractDeref(b, float);
    MH_ExtractDeref(c, float);
    MH_SetValue(result, float, a * b + c);
MESSAGE_HANDLER_END()

// Stats: multiple outputs -- computes sum, count, avg from an array pointer + length
MESSAGE_HANDLER_BEGIN(Stats)
    MH_ExtractDeref(data, int*);
    MH_ExtractDeref(len, int);
    int sum = 0;
    for (int i = 0; i < len; i++) sum += data[i];
    MH_SetValue(sum, int, sum);
    MH_SetValue(count, int, len);
    float avg = (len > 0) ? (float)sum / (float)len : 0.0f;
    MH_SetValue(avg, float, avg);
MESSAGE_HANDLER_END()

// OptionalInc: uses MH_Has to conditionally increment, tests Has + Get
MESSAGE_HANDLER_BEGIN(OptionalInc)
    MH_ExtractDeref(value, int);
    int step = 1;
    if (MH_Has(step)) {
        step = MH_GetDeref(step, int);
    }
    MH_SetValue(result, int, value + step);
MESSAGE_HANDLER_END()

// RawCopy: uses MH_Set with raw pointer+size, copies a struct through
MESSAGE_HANDLER_BEGIN(RawCopy)
    MH_Require(input);
    void *src = UnsafeVariedHashMap_SGet(payload->data, "input");
    uint32_t sz = UnsafeVariedHashMap_SGetSize(payload->data, "input");
    MH_Set(output, src, sz);
MESSAGE_HANDLER_END()

CAN_RECEIVE_BEGIN()
    CAN_RECEIVE_MID(Add)
    CAN_RECEIVE_MID(Sub)
    CAN_RECEIVE_MID(Mul)
    CAN_RECEIVE_MID(AddInPlace)
    CAN_RECEIVE_MID(Swap)
    CAN_RECEIVE_MID(Clamp)
    CAN_RECEIVE_MID(FMA)
    CAN_RECEIVE_MID(Stats)
    CAN_RECEIVE_MID(OptionalInc)
    CAN_RECEIVE_MID(RawCopy)
CAN_RECEIVE_END()

RECEIVE_MESSAGE_BEGIN()
    RECEIVE_MESSAGE_ROUTE(Add)
    RECEIVE_MESSAGE_ROUTE(Sub)
    RECEIVE_MESSAGE_ROUTE(Mul)
    RECEIVE_MESSAGE_ROUTE(AddInPlace)
    RECEIVE_MESSAGE_ROUTE(Swap)
    RECEIVE_MESSAGE_ROUTE(Clamp)
    RECEIVE_MESSAGE_ROUTE(FMA)
    RECEIVE_MESSAGE_ROUTE(Stats)
    RECEIVE_MESSAGE_ROUTE(OptionalInc)
    RECEIVE_MESSAGE_ROUTE(RawCopy)
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
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Add);
    Payload_SetValue(&msg, "a", int, 10);
    Payload_SetValue(&msg, "b", int, 25);
    DispatchMessage(&msg);
    ASSERT(MESSAGE_RESULT_ISOK(msg.result));
    ASSERT(Payload_GetDeref(&msg, "result", int) == 35);
    FreePayload(&msg);
    PASS();
}

static void test_class_dispatch_sub(void) {
    TEST("class: dispatch Calculator.Sub");
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Sub);
    Payload_SetValue(&msg, "a", int, 100);
    Payload_SetValue(&msg, "b", int, 37);
    DispatchMessage(&msg);
    ASSERT(MESSAGE_RESULT_ISOK(msg.result));
    ASSERT(Payload_GetDeref(&msg, "result", int) == 63);
    FreePayload(&msg);
    PASS();
}

static void test_class_dispatch_mul(void) {
    TEST("class: dispatch Calculator.Mul");
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Mul);
    Payload_SetValue(&msg, "a", int, 6);
    Payload_SetValue(&msg, "b", int, 7);
    DispatchMessage(&msg);
    ASSERT(MESSAGE_RESULT_ISOK(msg.result));
    ASSERT(Payload_GetDeref(&msg, "result", int) == 42);
    FreePayload(&msg);
    PASS();
}

static void test_class_dispatch_negative_result(void) {
    TEST("class: Sub with negative result");
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Sub);
    Payload_SetValue(&msg, "a", int, 3);
    Payload_SetValue(&msg, "b", int, 10);
    DispatchMessage(&msg);
    ASSERT(MESSAGE_RESULT_ISOK(msg.result));
    ASSERT(Payload_GetDeref(&msg, "result", int) == -7);
    FreePayload(&msg);
    PASS();
}

static void test_class_dispatch_zeroes(void) {
    TEST("class: Add with zeroes");
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Add);
    Payload_SetValue(&msg, "a", int, 0);
    Payload_SetValue(&msg, "b", int, 0);
    DispatchMessage(&msg);
    ASSERT(MESSAGE_RESULT_ISOK(msg.result));
    ASSERT(Payload_GetDeref(&msg, "result", int) == 0);
    FreePayload(&msg);
    PASS();
}

// -- Pointer and dict-output tests --

static void test_class_dispatch_add_in_place(void) {
    TEST("class: AddInPlace modifies target pointer");
    int accumulator = 50;
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_AddInPlace);
    Payload_SetValue(&msg, "target", void*, &accumulator);
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
        Payload_SetValue(&msg, "target", void*, &total);
        Payload_SetValue(&msg, "b", int, 10);
        DispatchMessage(&msg);
        ASSERT(MESSAGE_RESULT_ISOK(msg.result));
        FreePayload(&msg);
    }
    ASSERT(total == 50);
    PASS();
}

static void test_class_dispatch_result_in_dict(void) {
    TEST("class: Add writes result into payload dict");
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Add);
    Payload_SetValue(&msg, "a", int, 17);
    Payload_SetValue(&msg, "b", int, 25);
    ASSERT(!Payload_Has(&msg, "result"));
    DispatchMessage(&msg);
    ASSERT(MESSAGE_RESULT_ISOK(msg.result));
    ASSERT(Payload_Has(&msg, "result"));
    ASSERT(Payload_GetDeref(&msg, "result", int) == 42);
    FreePayload(&msg);
    PASS();
}

static void test_class_dispatch_pointer_modify_external(void) {
    TEST("class: pointer lets handler modify caller's struct");
    typedef struct { int x; int y; } Point;
    Point p = { 10, 20 };
    // Use AddInPlace to modify p.x via pointer
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_AddInPlace);
    Payload_SetValue(&msg, "target", void*, &p.x);
    Payload_SetValue(&msg, "b", int, 5);
    DispatchMessage(&msg);
    ASSERT(MESSAGE_RESULT_ISOK(msg.result));
    ASSERT(p.x == 15);
    ASSERT(p.y == 20); // y untouched
    FreePayload(&msg);
    PASS();
}

// -- Swap tests (MH_Extract: pointer to stored value, mutate in-place) --

static void test_class_dispatch_swap(void) {
    TEST("class: Swap exchanges two values in payload");
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Swap);
    Payload_SetValue(&msg, "a", int, 10);
    Payload_SetValue(&msg, "b", int, 20);
    DispatchMessage(&msg);
    ASSERT(MESSAGE_RESULT_ISOK(msg.result));
    ASSERT(Payload_GetDeref(&msg, "a", int) == 20);
    ASSERT(Payload_GetDeref(&msg, "b", int) == 10);
    FreePayload(&msg);
    PASS();
}

static void test_class_dispatch_swap_same(void) {
    TEST("class: Swap with equal values is no-op");
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Swap);
    Payload_SetValue(&msg, "a", int, 5);
    Payload_SetValue(&msg, "b", int, 5);
    DispatchMessage(&msg);
    ASSERT(MESSAGE_RESULT_ISOK(msg.result));
    ASSERT(Payload_GetDeref(&msg, "a", int) == 5);
    ASSERT(Payload_GetDeref(&msg, "b", int) == 5);
    FreePayload(&msg);
    PASS();
}

// -- Clamp tests (MH_Get nullable, optional params with defaults) --

static void test_class_dispatch_clamp_within(void) {
    TEST("class: Clamp value within range unchanged");
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Clamp);
    Payload_SetValue(&msg, "value", int, 50);
    Payload_SetValue(&msg, "min", int, 0);
    Payload_SetValue(&msg, "max", int, 100);
    DispatchMessage(&msg);
    ASSERT(MESSAGE_RESULT_ISOK(msg.result));
    ASSERT(Payload_GetDeref(&msg, "result", int) == 50);
    FreePayload(&msg);
    PASS();
}

static void test_class_dispatch_clamp_below(void) {
    TEST("class: Clamp value below min");
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Clamp);
    Payload_SetValue(&msg, "value", int, -10);
    Payload_SetValue(&msg, "min", int, 0);
    Payload_SetValue(&msg, "max", int, 100);
    DispatchMessage(&msg);
    ASSERT(Payload_GetDeref(&msg, "result", int) == 0);
    FreePayload(&msg);
    PASS();
}

static void test_class_dispatch_clamp_above(void) {
    TEST("class: Clamp value above max");
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Clamp);
    Payload_SetValue(&msg, "value", int, 200);
    Payload_SetValue(&msg, "min", int, 0);
    Payload_SetValue(&msg, "max", int, 100);
    DispatchMessage(&msg);
    ASSERT(Payload_GetDeref(&msg, "result", int) == 100);
    FreePayload(&msg);
    PASS();
}

static void test_class_dispatch_clamp_defaults(void) {
    TEST("class: Clamp uses defaults when min/max omitted");
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Clamp);
    Payload_SetValue(&msg, "value", int, 999);
    // no min/max -- defaults to 0..100
    DispatchMessage(&msg);
    ASSERT(Payload_GetDeref(&msg, "result", int) == 100);
    FreePayload(&msg);
    PASS();
}

static void test_class_dispatch_clamp_partial(void) {
    TEST("class: Clamp with only min set");
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Clamp);
    Payload_SetValue(&msg, "value", int, -5);
    Payload_SetValue(&msg, "min", int, 10);
    // no max -- defaults to 100
    DispatchMessage(&msg);
    ASSERT(Payload_GetDeref(&msg, "result", int) == 10);
    FreePayload(&msg);
    PASS();
}

// -- FMA tests (float types, MH_ExtractDeref + MH_SetValue with float) --

static void test_class_dispatch_fma(void) {
    TEST("class: FMA computes a*b+c with floats");
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_FMA);
    Payload_SetValue(&msg, "a", float, 2.0f);
    Payload_SetValue(&msg, "b", float, 3.0f);
    Payload_SetValue(&msg, "c", float, 1.5f);
    DispatchMessage(&msg);
    ASSERT(MESSAGE_RESULT_ISOK(msg.result));
    float r = Payload_GetDeref(&msg, "result", float);
    ASSERT(r > 7.4f && r < 7.6f); // 2*3+1.5 = 7.5
    FreePayload(&msg);
    PASS();
}

static void test_class_dispatch_fma_zeroes(void) {
    TEST("class: FMA with zeroes");
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_FMA);
    Payload_SetValue(&msg, "a", float, 0.0f);
    Payload_SetValue(&msg, "b", float, 99.0f);
    Payload_SetValue(&msg, "c", float, 5.0f);
    DispatchMessage(&msg);
    float r = Payload_GetDeref(&msg, "result", float);
    ASSERT(r > 4.9f && r < 5.1f); // 0*99+5 = 5
    FreePayload(&msg);
    PASS();
}

// -- Stats tests (pointer to external array, multiple outputs, mixed types) --

static void test_class_dispatch_stats(void) {
    TEST("class: Stats computes sum, count, avg");
    int data[] = {10, 20, 30, 40};
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Stats);
    Payload_SetValue(&msg, "data", int*, data);
    Payload_SetValue(&msg, "len", int, 4);
    DispatchMessage(&msg);
    ASSERT(MESSAGE_RESULT_ISOK(msg.result));
    ASSERT(Payload_GetDeref(&msg, "sum", int) == 100);
    ASSERT(Payload_GetDeref(&msg, "count", int) == 4);
    float avg = Payload_GetDeref(&msg, "avg", float);
    ASSERT(avg > 24.9f && avg < 25.1f);
    FreePayload(&msg);
    PASS();
}

static void test_class_dispatch_stats_single(void) {
    TEST("class: Stats with single element");
    int data[] = {42};
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Stats);
    Payload_SetValue(&msg, "data", int*, data);
    Payload_SetValue(&msg, "len", int, 1);
    DispatchMessage(&msg);
    ASSERT(Payload_GetDeref(&msg, "sum", int) == 42);
    ASSERT(Payload_GetDeref(&msg, "count", int) == 1);
    float avg = Payload_GetDeref(&msg, "avg", float);
    ASSERT(avg > 41.9f && avg < 42.1f);
    FreePayload(&msg);
    PASS();
}

// -- OptionalInc tests (MH_Has + MH_GetDeref for optional params) --

static void test_class_dispatch_optional_inc_default(void) {
    TEST("class: OptionalInc uses default step of 1");
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_OptionalInc);
    Payload_SetValue(&msg, "value", int, 10);
    DispatchMessage(&msg);
    ASSERT(Payload_GetDeref(&msg, "result", int) == 11);
    FreePayload(&msg);
    PASS();
}

static void test_class_dispatch_optional_inc_custom(void) {
    TEST("class: OptionalInc uses custom step");
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_OptionalInc);
    Payload_SetValue(&msg, "value", int, 10);
    Payload_SetValue(&msg, "step", int, 5);
    DispatchMessage(&msg);
    ASSERT(Payload_GetDeref(&msg, "result", int) == 15);
    FreePayload(&msg);
    PASS();
}

// -- RawCopy tests (MH_Set with raw ptr+size, MH_Require, struct passthrough) --

static void test_class_dispatch_raw_copy_int(void) {
    TEST("class: RawCopy passes int through");
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_RawCopy);
    Payload_SetValue(&msg, "input", int, 42);
    DispatchMessage(&msg);
    ASSERT(MESSAGE_RESULT_ISOK(msg.result));
    ASSERT(Payload_GetDeref(&msg, "output", int) == 42);
    ASSERT(Payload_GetSize(&msg, "output") == sizeof(int));
    FreePayload(&msg);
    PASS();
}

static void test_class_dispatch_raw_copy_struct(void) {
    TEST("class: RawCopy passes struct through");
    typedef struct { int x; int y; float z; } Vec3;
    Vec3 v = { 1, 2, 3.0f };
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_RawCopy);
    Payload_Set(&msg, "input", &v, sizeof(Vec3));
    DispatchMessage(&msg);
    ASSERT(MESSAGE_RESULT_ISOK(msg.result));
    Vec3 *out = (Vec3*)Payload_Get(&msg, "output");
    ASSERT(out != NULL);
    ASSERT(out->x == 1);
    ASSERT(out->y == 2);
    ASSERT(out->z > 2.9f && out->z < 3.1f);
    ASSERT(Payload_GetSize(&msg, "output") == sizeof(Vec3));
    FreePayload(&msg);
    PASS();
}

static void test_class_dispatch_raw_copy_missing(void) {
    TEST("class: RawCopy fails without input");
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_RawCopy);
    DispatchMessage(&msg);
    ASSERT(msg.result == MESSAGE_RESULT_MISSING_PARAMS);
    FreePayload(&msg);
    PASS();
}

// -- Error handling tests --

static void test_class_dispatch_missing_params(void) {
    TEST("class: dispatch with missing params");
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Add);
    Payload_SetValue(&msg, "a", int, 5);
    // missing "b"
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

static void test_class_payload_set_get(void) {
    TEST("class: Payload Set/Get/GetDeref/Has");
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Add);
    Payload_SetValue(&msg, "x", int, 77);
    ASSERT(Payload_Has(&msg, "x"));
    ASSERT(!Payload_Has(&msg, "y"));
    ASSERT(Payload_GetDeref(&msg, "x", int) == 77);
    ASSERT(*(int*)Payload_Get(&msg, "x") == 77);
    ASSERT(Payload_GetSize(&msg, "x") == sizeof(int));
    FreePayload(&msg);
    PASS();
}

static void test_class_payload_multiple_types(void) {
    TEST("class: Payload stores different types");
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Add);
    Payload_SetValue(&msg, "i", int, 10);
    Payload_SetValue(&msg, "f", float, 3.14f);
    Payload_SetValue(&msg, "c", char, 'A');
    ASSERT(Payload_GetDeref(&msg, "i", int) == 10);
    float fv = Payload_GetDeref(&msg, "f", float);
    ASSERT(fv > 3.13f && fv < 3.15f);
    ASSERT(Payload_GetDeref(&msg, "c", char) == 'A');
    FreePayload(&msg);
    PASS();
}

static void test_class_payload_duplicate_key(void) {
    TEST("class: duplicate key returns -1");
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Add);
    Payload_SetValue(&msg, "k", int, 5);
    ASSERT(Payload_Set(&msg, "k", &(int){10}, sizeof(int)) == -1);
    ASSERT(Payload_GetDeref(&msg, "k", int) == 5);
    FreePayload(&msg);
    PASS();
}

static void test_class_payload_dispatch_roundtrip(void) {
    TEST("class: set values, dispatch, read result");
    MessagePayload msg = PreparePayload(CID_Calculator, MID_Calculator_Add);
    Payload_SetValue(&msg, "a", int, 100);
    Payload_SetValue(&msg, "b", int, 200);
    DispatchMessage(&msg);
    ASSERT(MESSAGE_RESULT_ISOK(msg.result));
    ASSERT(Payload_GetDeref(&msg, "result", int) == 300);
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
    test_class_dispatch_result_in_dict();
    test_class_dispatch_pointer_modify_external();

    LOG_INFO("=== Swap Tests (MH_Extract) ===");
    test_class_dispatch_swap();
    test_class_dispatch_swap_same();

    LOG_INFO("=== Clamp Tests (MH_Get nullable, optional params) ===");
    test_class_dispatch_clamp_within();
    test_class_dispatch_clamp_below();
    test_class_dispatch_clamp_above();
    test_class_dispatch_clamp_defaults();
    test_class_dispatch_clamp_partial();

    LOG_INFO("=== FMA Tests (float ExtractDeref + SetValue) ===");
    test_class_dispatch_fma();
    test_class_dispatch_fma_zeroes();

    LOG_INFO("=== Stats Tests (pointer to array, multiple outputs) ===");
    test_class_dispatch_stats();
    test_class_dispatch_stats_single();

    LOG_INFO("=== OptionalInc Tests (MH_Has + MH_GetDeref) ===");
    test_class_dispatch_optional_inc_default();
    test_class_dispatch_optional_inc_custom();

    LOG_INFO("=== RawCopy Tests (MH_Set raw, MH_Require, struct) ===");
    test_class_dispatch_raw_copy_int();
    test_class_dispatch_raw_copy_struct();
    test_class_dispatch_raw_copy_missing();

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
    test_class_payload_set_get();
    test_class_payload_multiple_types();
    test_class_payload_duplicate_key();
    test_class_payload_dispatch_roundtrip();
}
