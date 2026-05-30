#pragma once

// This line would trigger R040 but is suppressed
#define LINTNORE
void test(void *obj) {
    obj->internal_refs = 5;
}
#undef LINTNORE

// This line is outside LINTNORE and SHOULD trigger R040
void bad(void *obj) {
    obj->external_refs = 0;
}
