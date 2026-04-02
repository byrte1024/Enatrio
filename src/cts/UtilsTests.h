#pragma once

#include "../tests.h"
#include "Class.h"

// -- CAT macro tests --

#define _UT_PREFIX foo
#define _UT_SUFFIX bar
#define _UT_A x
#define _UT_B y

static void test_utils_cat1(void) {
    TEST("utils: CAT1 identity");
    int CAT1(myvar) = 42;
    ASSERT(myvar == 42);
    PASS();
}

static void test_utils_cat2(void) {
    TEST("utils: CAT2 concatenates two tokens");
    int CAT2(foo, bar) = 10;
    ASSERT(foobar == 10);
    PASS();
}

static void test_utils_cat3(void) {
    TEST("utils: CAT3 concatenates three tokens");
    int CAT3(a, b, c) = 7;
    ASSERT(abc == 7);
    PASS();
}

static void test_utils_cat4(void) {
    TEST("utils: CAT4 concatenates four tokens");
    int CAT4(t, e, s, t) = 99;
    ASSERT(test == 99);
    PASS();
}

// -- BAT macro tests (indirect, expands args first) --

static void test_utils_bat2_expands(void) {
    TEST("utils: BAT2 expands macros before concat");
    int BAT2(_UT_PREFIX, _UT_SUFFIX) = 55;
    ASSERT(foobar == 55);
    PASS();
}

static void test_utils_bat3_expands(void) {
    TEST("utils: BAT3 expands macros before concat");
    int BAT3(_UT_PREFIX, _UT_A, _UT_B) = 77;
    ASSERT(fooxy == 77);
    PASS();
}

static void test_utils_bat2_with_line(void) {
    TEST("utils: BAT2 with __LINE__ produces unique names");
    // Two BAT2 calls on different lines create different variable names.
    // If they were the same, this would fail to compile (redefinition).
    int BAT2(uniq_, __LINE__) = 10;
    int BAT2(uniq_, __LINE__) = 20;
    // Compilation success proves uniqueness.
    PASS();
}

// -- CAT vs BAT difference --

static void test_utils_cat_vs_bat(void) {
    TEST("utils: CAT2 does not expand, BAT2 does");
    // CAT2(_UT_PREFIX, _UT_SUFFIX) produces _UT_PREFIX_UT_SUFFIX (literal concat, no expansion)
    int CAT2(_UT_PREFIX, _UT_SUFFIX) = 1;
    ASSERT(_UT_PREFIX_UT_SUFFIX == 1);
    // BAT2(_UT_PREFIX, _UT_SUFFIX) expands macros first, producing foobar
    int BAT2(_UT_PREFIX, _UT_SUFFIX) = 2;
    ASSERT(foobar == 2);
    // They are different variables
    ASSERT(&_UT_PREFIX_UT_SUFFIX != &foobar);
    PASS();
}

// -- STR / BSTR macro tests --

static void test_utils_str(void) {
    TEST("utils: STR stringifies token");
    const char *s = STR(hello);
    ASSERT(strcmp(s, "hello") == 0);
    PASS();
}

static void test_utils_bstr_expands(void) {
    TEST("utils: BSTR expands macro then stringifies");
    #define _UT_TESTVAL 42
    const char *s = BSTR(_UT_TESTVAL);
    ASSERT(strcmp(s, "42") == 0);
    #undef _UT_TESTVAL
    PASS();
}

static void test_utils_str_does_not_expand(void) {
    TEST("utils: STR does not expand macros");
    #define _UT_TESTVAL2 999
    const char *s = STR(_UT_TESTVAL2);
    ASSERT(strcmp(s, "_UT_TESTVAL2") == 0);
    #undef _UT_TESTVAL2
    PASS();
}

// -- ASIS macro test --

static void test_utils_asis(void) {
    TEST("utils: ASIS passes value through");
    int v = ASIS(42);
    ASSERT(v == 42);
    int a = 10;
    ASSERT(ASIS(a + 5) == 15);
    PASS();
}

// -- MESSAGE_RESULT macros --

static void test_utils_message_result_isok(void) {
    TEST("utils: MESSAGE_RESULT_ISOK");
    ASSERT(MESSAGE_RESULT_ISOK(MESSAGE_RESULT_SUCCESS));
    ASSERT(!MESSAGE_RESULT_ISOK(MESSAGE_RESULT_OOM));
    ASSERT(!MESSAGE_RESULT_ISOK(MESSAGE_RESULT_PENDING));
    ASSERT(!MESSAGE_RESULT_ISOK(MESSAGE_RESULT_NOTSENT));
    PASS();
}

static void test_utils_message_result_codes_unique(void) {
    TEST("utils: all MESSAGE_RESULT codes are unique");
    uint8_t codes[] = {
        MESSAGE_RESULT_SUCCESS, MESSAGE_RESULT_OOM,
        MESSAGE_RESULT_MISSING_PARAMS, MESSAGE_RESULT_INVALID_PARAMS,
        MESSAGE_RESULT_UNKNOWN_ERROR, MESSAGE_RESULT_INVALID_CID,
        MESSAGE_RESULT_INVALID_MID, MESSAGE_RESULT_NO_PAYLOAD,
        MESSAGE_RESULT_INTERNAL_ERROR, MESSAGE_RESULT_NOT_SUPPORTED,
        MESSAGE_RESULT_BUSY, MESSAGE_RESULT_TIMEOUT,
        MESSAGE_RESULT_DENIED, MESSAGE_RESULT_DUPLICATE,
        MESSAGE_RESULT_NOT_FOUND, MESSAGE_RESULT_OVERFLOW,
        MESSAGE_RESULT_NOT_READY, MESSAGE_RESULT_CANCELLED,
        MESSAGE_RESULT_PENDING, MESSAGE_RESULT_IGNORED,
        MESSAGE_RESULT_NOTSENT
    };
    int count = (int)(sizeof(codes) / sizeof(codes[0]));
    for (int i = 0; i < count; i++) {
        for (int j = i + 1; j < count; j++) {
            ASSERT(codes[i] != codes[j]);
        }
    }
    PASS();
}

static void test_utils_message_result_name(void) {
    TEST("utils: MESSAGE_RESULT_NAME returns correct strings");
    ASSERT(strcmp(MESSAGE_RESULT_NAME(MESSAGE_RESULT_SUCCESS), "SUCCESS") == 0);
    ASSERT(strcmp(MESSAGE_RESULT_NAME(MESSAGE_RESULT_OOM), "OOM") == 0);
    ASSERT(strcmp(MESSAGE_RESULT_NAME(MESSAGE_RESULT_NOTSENT), "NOTSENT") == 0);
    ASSERT(strcmp(MESSAGE_RESULT_NAME(MESSAGE_RESULT_IGNORED), "IGNORED") == 0);
    ASSERT(strcmp(MESSAGE_RESULT_NAME(255), "UNDEFINED") == 0);
    PASS();
}

static void test_utils_message_result_desc(void) {
    TEST("utils: MESSAGE_RESULT_DESC returns descriptions");
    ASSERT(strlen(MESSAGE_RESULT_DESC(MESSAGE_RESULT_SUCCESS)) > 0);
    ASSERT(strlen(MESSAGE_RESULT_DESC(MESSAGE_RESULT_OOM)) > 0);
    ASSERT(strlen(MESSAGE_RESULT_DESC(MESSAGE_RESULT_NOTSENT)) > 0);
    ASSERT(strcmp(MESSAGE_RESULT_DESC(255), "Undefined result code") == 0);
    PASS();
}

// -- CLASSID macros --

static void test_utils_classid_isuntyped(void) {
    TEST("utils: CLASSID_ISUNTYPED");
    ASSERT(CLASSID_ISUNTYPED(0));
    ASSERT(!CLASSID_ISUNTYPED(1));
    ASSERT(!CLASSID_ISUNTYPED(0xFFFF));
    PASS();
}

// -- Runner --

static void run_utils_tests(void) {
    LOG_INFO("=== Utils Macro Tests ===");
    test_utils_cat1();
    test_utils_cat2();
    test_utils_cat3();
    test_utils_cat4();
    test_utils_bat2_expands();
    test_utils_bat3_expands();
    test_utils_bat2_with_line();
    test_utils_cat_vs_bat();
    test_utils_str();
    test_utils_bstr_expands();
    test_utils_str_does_not_expand();
    test_utils_asis();

    LOG_INFO("=== Message Result Macro Tests ===");
    test_utils_message_result_isok();
    test_utils_message_result_codes_unique();
    test_utils_message_result_name();
    test_utils_message_result_desc();
    test_utils_classid_isuntyped();
}
