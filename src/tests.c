#ifdef INTESTING

#include <stdio.h>

#define VERBOSE

#include "tests.h"
#include "cts/UnsafeArrayTests.h"
#include "cts/UnsafeDictionaryTests.h"
#include "cts/UnsafeArrayLINQTests.h"
#include "cts/UnsafeHashMapTests.h"

int main(void) {
    START_LOGGING("tests.log", LOG_INFO);
    LOG_BUILD_INFO();

    run_unsafe_array_tests();
    run_unsafe_dictionary_tests();
    run_unsafe_array_linq_tests();
    run_unsafe_hashmap_tests();

    LOG_INFO("=== Results: %d/%d passed ===", tests_passed, tests_run);

    END_LOGGING();
    return tests_run - tests_passed;
}

#endif
