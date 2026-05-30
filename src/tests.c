#ifdef INTESTING

#include <stdio.h>

#define VERBOSE

#include "system/tests.h"
#include "tests/UnsafeArrayTests.h"
#include "tests/UnsafeDictionaryTests.h"
#include "tests/UnsafeArrayLINQTests.h"
#include "tests/UnsafeHashMapTests.h"
#include "tests/UnsafeVariedDictionaryTests.h"
#include "tests/UnsafeVariedHashMapTests.h"
#include "tests/UtilsTests.h"
#include "tests/ClassTests.h"
#include "tests/SelfTests.h"
#include "tests/SerializationTests.h"
#include "tests/ByteStreamTests.h"

int main(void) {
    START_LOGGING("tests", LOG_INFO);
    LOG_BUILD_INFO();

    run_unsafe_array_tests();
    run_unsafe_dictionary_tests();
    run_unsafe_array_linq_tests();
    run_unsafe_hashmap_tests();
    run_unsafe_varied_dictionary_tests();
    run_unsafe_varied_hashmap_tests();
    run_utils_tests();
    run_class_tests();
    run_self_tests();
    run_serialization_tests();
    run_bytestream_tests();

    LOG_INFO("=== Results: %d/%d passed ===", tests_passed, tests_run);

    END_LOGGING();
    return tests_run - tests_passed;
}

#endif
