#pragma once

#include "../system/tests.h"
#include "../system/object/Serialization.h"
#include "SelfTests.h"

// ============================================================
// Helpers
// ============================================================

static const char *_sertest_path(const char *name) {
    static char buf[512];
    snprintf(buf, sizeof(buf), "%s/%s", AppLocal, name);
    return buf;
}
#define _SERTEST_PATH(name) _sertest_path(name)

// Helper to set up class registrations for each test group
static void _sertest_init_classes(void) {
    BeginClassRegistrations();
    RegisterClass(Counter_ClassDef());
    RegisterClass(Node_ClassDef());
    EndClassRegistrations();
}

// Helper: increment a Counter N times and return the count
static int _sertest_increment_n(TempObjectReference counter, int n) {
    for (int i = 0; i < n; i++) {
        MessagePayload m = _counter_dispatch(counter, MID_Counter_SELF_Increment);
        FreePayload(&m);
    }
    int count;
    SELF_DISPATCH(ObjectContainer_TempFrom(counter), MID_Counter_SELF_GetCount, {}, {
        count = Payload_GetDeref(msg, "result", int);
    });
    return count;
}

// Helper: get count from a Counter
static int _sertest_get_count(TempObjectReference counter) {
    int count;
    SELF_DISPATCH(ObjectContainer_TempFrom(counter), MID_Counter_SELF_GetCount, {}, {
        count = Payload_GetDeref(msg, "result", int);
    });
    return count;
}

// Helper: delete a file
static void _sertest_delete_file(const char *path) {
    remove(path);
}

// ============================================================
// Registry tests
// ============================================================

static void test_ser_builtins_registered(void) {
    TEST("ser: builtins SER_RAW, SER_SKIP, SER_DEREF, SER_STRING registered");
    _Ser_InitBuiltins();

    SerEntry *raw = Ser_Get(SER_RAW);
    ASSERT(raw != NULL);
    ASSERT(raw->serialize != NULL);
    ASSERT(raw->deserialize != NULL);

    SerEntry *skip = Ser_Get(SER_SKIP);
    ASSERT(skip != NULL);
    ASSERT(skip->serialize == NULL);
    ASSERT(skip->deserialize == NULL);

    SerEntry *deref = Ser_Get(SER_DEREF);
    ASSERT(deref != NULL);
    ASSERT(deref->serialize != NULL);
    ASSERT(deref->deserialize != NULL);

    SerEntry *str = Ser_Get(SER_STRING);
    ASSERT(str != NULL);
    ASSERT(str->serialize != NULL);
    ASSERT(str->deserialize != NULL);

    PASS();
}

static void test_ser_register_custom(void) {
    TEST("ser: register custom ser function at SER_USER_START");
    _Ser_InitBuiltins();

    // Use SER_USER_START + 1 to avoid conflict with test_ser_custom_function_roundtrip
    uint16_t test_id = SER_USER_START + 1;

    // If already registered from a prior run, skip re-registration check
    if (Ser_Get(test_id) == NULL) {
        int result = Ser_Register(test_id, _ser_raw, _ser_raw, "TEST_CUSTOM");
        ASSERT(result == 0);
    }

    SerEntry *entry = Ser_Get(test_id);
    ASSERT(entry != NULL);
    ASSERT(entry->registered == true);

    // Registering same ID again should fail
    int dup = Ser_Register(test_id, _ser_raw, _ser_raw, "DUPLICATE");
    ASSERT(dup == -1);

    // Registering below SER_USER_START should fail
    int below = Ser_Register(SER_USER_START - 1, _ser_raw, _ser_raw, "BELOW");
    ASSERT(below == -1);

    PASS();
}

// Custom XOR obfuscation ser/deser pair for test_ser_custom_function_roundtrip
static void *_ser_xor(const void *data, uint32_t size, uint16_t arg, uint32_t *out_size) {
    (void)arg;
    uint8_t *copy = (uint8_t *)malloc(size);
    if (!copy) return NULL;
    const uint8_t *src = (const uint8_t *)data;
    for (uint32_t i = 0; i < size; i++) {
        copy[i] = src[i] ^ 0xAA;
    }
    *out_size = size;
    return copy;
}

static void *_deser_xor(const void *data, uint32_t size, uint16_t arg, uint32_t *out_size) {
    (void)arg;
    uint8_t *copy = (uint8_t *)malloc(size);
    if (!copy) return NULL;
    const uint8_t *src = (const uint8_t *)data;
    for (uint32_t i = 0; i < size; i++) {
        copy[i] = src[i] ^ 0xAA;
    }
    *out_size = size;
    return copy;
}

static void test_ser_custom_function_roundtrip(void) {
    TEST("ser: custom XOR ser/deser roundtrips correctly");
    _sertest_init_classes();

    uint16_t xor_id = SER_USER_START;
    if (Ser_Get(xor_id) == NULL) {
        int r = Ser_Register(xor_id, _ser_xor, _deser_xor, "XOR");
        ASSERT(r == 0);
    }

    // Create a Counter object
    ExternalReference counter = Object_CreateRef(CID_Counter);
    ASSERT(counter != NULL);
    TempObjectReference ct = ObjectContainer_TempFrom(counter);

    // Store a value with custom serialization
    int secret = 12345;
    _Object_StoreValue(ct->data->values, "secret", 6,
                       &secret, sizeof(int),
                       CID_Counter, xor_id, 0);

    // Serialize
    ByteStream *stream = Object_Serialize(&counter, 1);
    ASSERT(stream != NULL);

    // Deserialize
    int root_count = 0;
    ExternalReference *roots = Object_Deserialize(stream, &root_count);
    ASSERT(roots != NULL);
    ASSERT(root_count == 1);
    ASSERT(roots[0] != NULL);

    // Verify the value round-tripped
    int *restored = (int *)_Object_GetValueData(roots[0]->data->values, "secret", 6);
    ASSERT(restored != NULL);
    ASSERT(*restored == 12345);

    // Cleanup
    ByteStream_Destroy(stream);
    ObjectContainer_UnRef_External(&counter);
    for (int i = 0; i < root_count; i++) {
        ObjectContainer_UnRef_External(&roots[i]);
    }
    free(roots);
    Object_GarbageCollect();
    ASSERT(_registry_count() == 0);
    PASS();
}

// ============================================================
// Value header tests
// ============================================================

static void test_ser_value_header_raw(void) {
    TEST("ser: value header for SER_RAW has correct fields");
    _sertest_init_classes();

    ExternalReference counter = Object_CreateRef(CID_Counter);
    ASSERT(counter != NULL);
    TempObjectReference ct = ObjectContainer_TempFrom(counter);

    // "count" was stored by SELF_Create via Self_SetValue -> SER_RAW
    ObjectValueHeader *hdr = _Object_GetValueHeader(ct->data->values, "count", 5);
    ASSERT(hdr != NULL);
    ASSERT(hdr->owner == CID_Counter);
    ASSERT(hdr->ser_id == SER_RAW);
    ASSERT(hdr->ser_arg == 0);

    ObjectContainer_UnRef_External(&counter);
    Object_GarbageCollect();
    ASSERT(_registry_count() == 0);
    PASS();
}

static void test_ser_value_header_transient(void) {
    TEST("ser: value header for SER_SKIP has correct fields");
    _sertest_init_classes();

    ExternalReference counter = Object_CreateRef(CID_Counter);
    ASSERT(counter != NULL);
    TempObjectReference ct = ObjectContainer_TempFrom(counter);

    // Store a transient value directly
    int cache_val = 999;
    _Object_StoreValue(ct->data->values, "cache", 5,
                       &cache_val, sizeof(int),
                       CID_Counter, SER_SKIP, 0);

    ObjectValueHeader *hdr = _Object_GetValueHeader(ct->data->values, "cache", 5);
    ASSERT(hdr != NULL);
    ASSERT(hdr->owner == CID_Counter);
    ASSERT(hdr->ser_id == SER_SKIP);
    ASSERT(hdr->ser_arg == 0);

    ObjectContainer_UnRef_External(&counter);
    Object_GarbageCollect();
    ASSERT(_registry_count() == 0);
    PASS();
}

static void test_ser_value_header_custom(void) {
    TEST("ser: value header for custom ser_id and arg");
    _sertest_init_classes();

    ExternalReference counter = Object_CreateRef(CID_Counter);
    ASSERT(counter != NULL);
    TempObjectReference ct = ObjectContainer_TempFrom(counter);

    uint16_t custom_id = SER_USER_START;
    uint16_t custom_arg = 42;
    int val = 777;
    _Object_StoreValue(ct->data->values, "custom", 6,
                       &val, sizeof(int),
                       CID_Counter, custom_id, custom_arg);

    ObjectValueHeader *hdr = _Object_GetValueHeader(ct->data->values, "custom", 6);
    ASSERT(hdr != NULL);
    ASSERT(hdr->owner == CID_Counter);
    ASSERT(hdr->ser_id == custom_id);
    ASSERT(hdr->ser_arg == custom_arg);

    ObjectContainer_UnRef_External(&counter);
    Object_GarbageCollect();
    ASSERT(_registry_count() == 0);
    PASS();
}

// ============================================================
// Single object round-trip
// ============================================================

static void test_ser_single_object_roundtrip(void) {
    TEST("ser: single Counter roundtrips with correct count");
    _sertest_init_classes();

    ExternalReference counter = Object_CreateRef(CID_Counter);
    ASSERT(counter != NULL);
    _sertest_increment_n(ObjectContainer_TempFrom(counter), 5);
    ASSERT(_sertest_get_count(ObjectContainer_TempFrom(counter)) == 5);

    ByteStream *stream = Object_Serialize(&counter, 1);
    ASSERT(stream != NULL);

    int root_count = 0;
    ExternalReference *roots = Object_Deserialize(stream, &root_count);
    ASSERT(roots != NULL);
    ASSERT(root_count == 1);
    ASSERT(roots[0] != NULL);

    int loaded_count = _sertest_get_count(ObjectContainer_TempFrom(roots[0]));
    ASSERT(loaded_count == 5);

    ByteStream_Destroy(stream);
    ObjectContainer_UnRef_External(&counter);
    for (int i = 0; i < root_count; i++) {
        ObjectContainer_UnRef_External(&roots[i]);
    }
    free(roots);
    Object_GarbageCollect();
    ASSERT(_registry_count() == 0);
    PASS();
}

static void test_ser_single_object_byte_match(void) {
    TEST("ser: same object serialized twice produces identical bytes");
    _sertest_init_classes();

    ExternalReference counter = Object_CreateRef(CID_Counter);
    ASSERT(counter != NULL);
    _sertest_increment_n(ObjectContainer_TempFrom(counter), 3);

    ByteStream *s1 = Object_Serialize(&counter, 1);
    ASSERT(s1 != NULL);

    ByteStream *s2 = Object_Serialize(&counter, 1);
    ASSERT(s2 != NULL);

    ASSERT(s1->length == s2->length);
    ASSERT(memcmp(s1->data, s2->data, s1->length) == 0);

    ByteStream_Destroy(s1);
    ByteStream_Destroy(s2);
    ObjectContainer_UnRef_External(&counter);
    Object_GarbageCollect();
    ASSERT(_registry_count() == 0);
    PASS();
}

static void test_ser_empty_object(void) {
    TEST("ser: Counter with initial values roundtrips");
    _sertest_init_classes();

    ExternalReference counter = Object_CreateRef(CID_Counter);
    ASSERT(counter != NULL);
    // count == 0 from SELF_Create, no increments

    ByteStream *stream = Object_Serialize(&counter, 1);
    ASSERT(stream != NULL);

    int root_count = 0;
    ExternalReference *roots = Object_Deserialize(stream, &root_count);
    ASSERT(roots != NULL);
    ASSERT(root_count == 1);
    ASSERT(roots[0] != NULL);

    int loaded_count = _sertest_get_count(ObjectContainer_TempFrom(roots[0]));
    ASSERT(loaded_count == 0);

    ByteStream_Destroy(stream);
    ObjectContainer_UnRef_External(&counter);
    for (int i = 0; i < root_count; i++) {
        ObjectContainer_UnRef_External(&roots[i]);
    }
    free(roots);
    Object_GarbageCollect();
    ASSERT(_registry_count() == 0);
    PASS();
}

// ============================================================
// Transient values
// ============================================================

static void test_ser_transient_skipped(void) {
    TEST("ser: SER_SKIP values are excluded from serialized data");
    _sertest_init_classes();

    ExternalReference counter = Object_CreateRef(CID_Counter);
    ASSERT(counter != NULL);
    TempObjectReference ct = ObjectContainer_TempFrom(counter);

    // Store a raw value
    int health = 100;
    _Object_StoreValue(ct->data->values, "health", 6,
                       &health, sizeof(int),
                       CID_Counter, SER_RAW, 0);

    // Store a transient value
    int cache = 999;
    _Object_StoreValue(ct->data->values, "cache", 5,
                       &cache, sizeof(int),
                       CID_Counter, SER_SKIP, 0);

    ByteStream *stream = Object_Serialize(&counter, 1);
    ASSERT(stream != NULL);

    int root_count = 0;
    ExternalReference *roots = Object_Deserialize(stream, &root_count);
    ASSERT(roots != NULL);
    ASSERT(root_count == 1);
    ASSERT(roots[0] != NULL);

    // "health" should exist
    int *h = (int *)_Object_GetValueData(roots[0]->data->values, "health", 6);
    ASSERT(h != NULL);
    ASSERT(*h == 100);

    // "cache" should NOT exist
    void *c = _Object_GetValueData(roots[0]->data->values, "cache", 5);
    ASSERT(c == NULL);

    ByteStream_Destroy(stream);
    ObjectContainer_UnRef_External(&counter);
    for (int i = 0; i < root_count; i++) {
        ObjectContainer_UnRef_External(&roots[i]);
    }
    free(roots);
    Object_GarbageCollect();
    ASSERT(_registry_count() == 0);
    PASS();
}

// ============================================================
// Multi-object round-trip
// ============================================================

static void test_ser_two_objects_no_refs(void) {
    TEST("ser: two Counters with different counts roundtrip");
    _sertest_init_classes();

    ExternalReference c1 = Object_CreateRef(CID_Counter);
    ExternalReference c2 = Object_CreateRef(CID_Counter);
    ASSERT(c1 != NULL && c2 != NULL);

    _sertest_increment_n(ObjectContainer_TempFrom(c1), 3);
    _sertest_increment_n(ObjectContainer_TempFrom(c2), 7);

    ExternalReference roots_arr[2] = { c1, c2 };
    ByteStream *stream = Object_Serialize(roots_arr, 2);
    ASSERT(stream != NULL);

    int root_count = 0;
    ExternalReference *loaded = Object_Deserialize(stream, &root_count);
    ASSERT(loaded != NULL);
    ASSERT(root_count == 2);

    int count1 = _sertest_get_count(ObjectContainer_TempFrom(loaded[0]));
    int count2 = _sertest_get_count(ObjectContainer_TempFrom(loaded[1]));
    ASSERT(count1 == 3);
    ASSERT(count2 == 7);

    ByteStream_Destroy(stream);
    ObjectContainer_UnRef_External(&c1);
    ObjectContainer_UnRef_External(&c2);
    for (int i = 0; i < root_count; i++) {
        ObjectContainer_UnRef_External(&loaded[i]);
    }
    free(loaded);
    Object_GarbageCollect();
    ASSERT(_registry_count() == 0);
    PASS();
}

static void test_ser_object_with_ref(void) {
    TEST("ser: object A refs B, B reachable via ref, roundtrips");
    _sertest_init_classes();

    ExternalReference a = Object_CreateRef(CID_Counter);
    ExternalReference b = Object_CreateRef(CID_Counter);
    ASSERT(a != NULL && b != NULL);

    _sertest_increment_n(ObjectContainer_TempFrom(a), 2);
    _sertest_increment_n(ObjectContainer_TempFrom(b), 9);

    // A refs B
    Object_SStoreRef(ObjectContainer_TempFrom(a), "child", ObjectContainer_TempFrom(b));

    // Serialize with only A as root (B reachable via ref)
    ByteStream *stream = Object_Serialize(&a, 1);
    ASSERT(stream != NULL);

    int root_count = 0;
    ExternalReference *loaded = Object_Deserialize(stream, &root_count);
    ASSERT(loaded != NULL);
    ASSERT(root_count == 1);
    ASSERT(loaded[0] != NULL);

    // Verify A's count
    int count_a = _sertest_get_count(ObjectContainer_TempFrom(loaded[0]));
    ASSERT(count_a == 2);

    // Verify A has ref to B
    TempObjectReference loaded_b = Object_SGetRef(ObjectContainer_TempFrom(loaded[0]), "child");
    ASSERT(loaded_b != NULL);
    int count_b = _sertest_get_count(loaded_b);
    ASSERT(count_b == 9);

    ByteStream_Destroy(stream);
    ObjectContainer_UnRef_External(&a);
    ObjectContainer_UnRef_External(&b);
    for (int i = 0; i < root_count; i++) {
        ObjectContainer_UnRef_External(&loaded[i]);
    }
    free(loaded);
    Object_GarbageCollect();
    ASSERT(_registry_count() == 0);
    PASS();
}

static void test_ser_mutual_refs(void) {
    TEST("ser: A refs B, B refs A (cycle), roundtrips");
    _sertest_init_classes();

    ExternalReference a = Object_CreateRef(CID_Counter);
    ExternalReference b = Object_CreateRef(CID_Counter);
    ASSERT(a != NULL && b != NULL);

    _sertest_increment_n(ObjectContainer_TempFrom(a), 4);
    _sertest_increment_n(ObjectContainer_TempFrom(b), 6);

    Object_SStoreRef(ObjectContainer_TempFrom(a), "peer", ObjectContainer_TempFrom(b));
    Object_SStoreRef(ObjectContainer_TempFrom(b), "peer", ObjectContainer_TempFrom(a));

    ByteStream *stream = Object_Serialize(&a, 1);
    ASSERT(stream != NULL);

    int root_count = 0;
    ExternalReference *loaded = Object_Deserialize(stream, &root_count);
    ASSERT(loaded != NULL);
    ASSERT(root_count == 1);

    TempObjectReference la = ObjectContainer_TempFrom(loaded[0]);
    ASSERT(_sertest_get_count(la) == 4);

    TempObjectReference lb = Object_SGetRef(la, "peer");
    ASSERT(lb != NULL);
    ASSERT(_sertest_get_count(lb) == 6);

    // B refs A
    TempObjectReference lb_peer = Object_SGetRef(lb, "peer");
    ASSERT(lb_peer != NULL);
    ASSERT(lb_peer == la);

    ByteStream_Destroy(stream);
    ObjectContainer_UnRef_External(&a);
    ObjectContainer_UnRef_External(&b);
    for (int i = 0; i < root_count; i++) {
        ObjectContainer_UnRef_External(&loaded[i]);
    }
    free(loaded);
    Object_GarbageCollect();
    ASSERT(_registry_count() == 0);
    PASS();
}

static void test_ser_self_ref(void) {
    TEST("ser: object refs itself, roundtrips");
    _sertest_init_classes();

    ExternalReference a = Object_CreateRef(CID_Counter);
    ASSERT(a != NULL);
    _sertest_increment_n(ObjectContainer_TempFrom(a), 3);
    Object_SStoreRef(ObjectContainer_TempFrom(a), "self", ObjectContainer_TempFrom(a));

    ByteStream *stream = Object_Serialize(&a, 1);
    ASSERT(stream != NULL);

    int root_count = 0;
    ExternalReference *loaded = Object_Deserialize(stream, &root_count);
    ASSERT(loaded != NULL);
    ASSERT(root_count == 1);

    TempObjectReference la = ObjectContainer_TempFrom(loaded[0]);
    ASSERT(_sertest_get_count(la) == 3);

    TempObjectReference self_ref = Object_SGetRef(la, "self");
    ASSERT(self_ref != NULL);
    ASSERT(self_ref == la);

    ByteStream_Destroy(stream);
    ObjectContainer_UnRef_External(&a);
    for (int i = 0; i < root_count; i++) {
        ObjectContainer_UnRef_External(&loaded[i]);
    }
    free(loaded);
    Object_GarbageCollect();
    ASSERT(_registry_count() == 0);
    PASS();
}

static void test_ser_diamond_refs(void) {
    TEST("ser: A->B, A->C, B->D, C->D diamond, D is shared");
    _sertest_init_classes();

    ExternalReference ea = Object_CreateRef(CID_Counter);
    ExternalReference eb = Object_CreateRef(CID_Counter);
    ExternalReference ec = Object_CreateRef(CID_Counter);
    ExternalReference ed = Object_CreateRef(CID_Counter);
    ASSERT(ea && eb && ec && ed);

    _sertest_increment_n(ObjectContainer_TempFrom(ea), 1);
    _sertest_increment_n(ObjectContainer_TempFrom(eb), 2);
    _sertest_increment_n(ObjectContainer_TempFrom(ec), 3);
    _sertest_increment_n(ObjectContainer_TempFrom(ed), 4);

    TempObjectReference a = ObjectContainer_TempFrom(ea);
    TempObjectReference b = ObjectContainer_TempFrom(eb);
    TempObjectReference c = ObjectContainer_TempFrom(ec);
    TempObjectReference d = ObjectContainer_TempFrom(ed);

    Object_SStoreRef(a, "left", b);
    Object_SStoreRef(a, "right", c);
    Object_SStoreRef(b, "target", d);
    Object_SStoreRef(c, "target", d);

    ByteStream *stream = Object_Serialize(&ea, 1);
    ASSERT(stream != NULL);

    int root_count = 0;
    ExternalReference *loaded = Object_Deserialize(stream, &root_count);
    ASSERT(loaded != NULL);
    ASSERT(root_count == 1);

    TempObjectReference la = ObjectContainer_TempFrom(loaded[0]);
    ASSERT(_sertest_get_count(la) == 1);

    TempObjectReference lb = Object_SGetRef(la, "left");
    TempObjectReference lc = Object_SGetRef(la, "right");
    ASSERT(lb != NULL && lc != NULL);
    ASSERT(_sertest_get_count(lb) == 2);
    ASSERT(_sertest_get_count(lc) == 3);

    TempObjectReference ld_from_b = Object_SGetRef(lb, "target");
    TempObjectReference ld_from_c = Object_SGetRef(lc, "target");
    ASSERT(ld_from_b != NULL);
    ASSERT(ld_from_c != NULL);
    ASSERT(ld_from_b == ld_from_c); // D is shared
    ASSERT(_sertest_get_count(ld_from_b) == 4);

    ByteStream_Destroy(stream);
    ObjectContainer_UnRef_External(&ea);
    ObjectContainer_UnRef_External(&eb);
    ObjectContainer_UnRef_External(&ec);
    ObjectContainer_UnRef_External(&ed);
    for (int i = 0; i < root_count; i++) {
        ObjectContainer_UnRef_External(&loaded[i]);
    }
    free(loaded);
    Object_GarbageCollect();
    ASSERT(_registry_count() == 0);
    PASS();
}

// ============================================================
// File I/O
// ============================================================

static void test_ser_save_load_file(void) {
    TEST("ser: SaveToFile + LoadFromFile roundtrips");
    _sertest_init_classes();

    const char *path = _SERTEST_PATH("test_ser_save_load.cob");

    ExternalReference counter = Object_CreateRef(CID_Counter);
    ASSERT(counter != NULL);
    _sertest_increment_n(ObjectContainer_TempFrom(counter), 8);

    int result = Object_SaveToFile(path, &counter, 1);
    ASSERT(result == 0);

    int root_count = 0;
    ExternalReference *loaded = Object_LoadFromFile(path, &root_count);
    ASSERT(loaded != NULL);
    ASSERT(root_count == 1);
    ASSERT(loaded[0] != NULL);

    int loaded_count = _sertest_get_count(ObjectContainer_TempFrom(loaded[0]));
    ASSERT(loaded_count == 8);

    ObjectContainer_UnRef_External(&counter);
    for (int i = 0; i < root_count; i++) {
        ObjectContainer_UnRef_External(&loaded[i]);
    }
    free(loaded);
    _sertest_delete_file(path);
    Object_GarbageCollect();
    ASSERT(_registry_count() == 0);
    PASS();
}

static void test_ser_load_bad_file(void) {
    TEST("ser: LoadFromFile with garbage data returns NULL");
    _sertest_init_classes();

    const char *path = _SERTEST_PATH("test_ser_bad.cob");

    // Write garbage to the file
    FILE *f = fopen(path, "wb");
    ASSERT(f != NULL);
    const char *garbage = "THIS IS NOT A VALID COB FILE AT ALL";
    fwrite(garbage, 1, strlen(garbage), f);
    fclose(f);

    int root_count = 0;
    ExternalReference *loaded = Object_LoadFromFile(path, &root_count);
    ASSERT(loaded == NULL);

    _sertest_delete_file(path);
    Object_GarbageCollect();
    ASSERT(_registry_count() == 0);
    PASS();
}

static void test_ser_save_load_byte_match(void) {
    TEST("ser: save to file, load, re-serialize, byte-for-byte match");
    _sertest_init_classes();

    const char *path = _SERTEST_PATH("test_ser_byte_match.cob");

    ExternalReference counter = Object_CreateRef(CID_Counter);
    ASSERT(counter != NULL);
    _sertest_increment_n(ObjectContainer_TempFrom(counter), 5);

    // Serialize to stream (for comparison)
    ByteStream *s1 = Object_Serialize(&counter, 1);
    ASSERT(s1 != NULL);

    // Save to file
    int result = Object_SaveToFile(path, &counter, 1);
    ASSERT(result == 0);

    // Load from file
    int root_count = 0;
    ExternalReference *loaded = Object_LoadFromFile(path, &root_count);
    ASSERT(loaded != NULL);
    ASSERT(root_count == 1);

    // Re-serialize loaded objects
    ByteStream *s2 = Object_Serialize(loaded, 1);
    ASSERT(s2 != NULL);

    // Compare
    ASSERT(s1->length == s2->length);
    ASSERT(memcmp(s1->data, s2->data, s1->length) == 0);

    ByteStream_Destroy(s1);
    ByteStream_Destroy(s2);
    ObjectContainer_UnRef_External(&counter);
    for (int i = 0; i < root_count; i++) {
        ObjectContainer_UnRef_External(&loaded[i]);
    }
    free(loaded);
    _sertest_delete_file(path);
    Object_GarbageCollect();
    ASSERT(_registry_count() == 0);
    PASS();
}

// ============================================================
// Edge cases
// ============================================================

static void test_ser_no_roots(void) {
    TEST("ser: Serialize with root_count=0 returns NULL");
    ExternalReference dummy;
    ByteStream *stream = Object_Serialize(&dummy, 0);
    ASSERT(stream == NULL);
    PASS();
}

static void test_ser_null_roots(void) {
    TEST("ser: Serialize with NULL roots returns NULL");
    ByteStream *stream = Object_Serialize(NULL, 5);
    ASSERT(stream == NULL);
    PASS();
}

static void test_ser_many_objects(void) {
    TEST("ser: 50 Counters in a chain, roundtrip all");
    _sertest_init_classes();

    ExternalReference counters[50];
    for (int i = 0; i < 50; i++) {
        counters[i] = Object_CreateRef(CID_Counter);
        ASSERT(counters[i] != NULL);
        _sertest_increment_n(ObjectContainer_TempFrom(counters[i]), i);
    }

    // Chain: each refs the next
    for (int i = 0; i < 49; i++) {
        Object_SStoreRef(ObjectContainer_TempFrom(counters[i]),
                         "next", ObjectContainer_TempFrom(counters[i + 1]));
    }

    // Serialize with first as root
    ByteStream *stream = Object_Serialize(&counters[0], 1);
    ASSERT(stream != NULL);

    int root_count = 0;
    ExternalReference *loaded = Object_Deserialize(stream, &root_count);
    ASSERT(loaded != NULL);
    ASSERT(root_count == 1);

    // Walk the chain and verify counts
    TempObjectReference current = ObjectContainer_TempFrom(loaded[0]);
    for (int i = 0; i < 50; i++) {
        ASSERT(current != NULL);
        int count = _sertest_get_count(current);
        ASSERT(count == i);
        if (i < 49) {
            current = Object_SGetRef(current, "next");
        }
    }

    ByteStream_Destroy(stream);
    for (int i = 0; i < 50; i++) {
        ObjectContainer_UnRef_External(&counters[i]);
    }
    for (int i = 0; i < root_count; i++) {
        ObjectContainer_UnRef_External(&loaded[i]);
    }
    free(loaded);
    Object_GarbageCollect();
    ASSERT(_registry_count() == 0);
    PASS();
}

static void test_ser_object_no_values_only_refs(void) {
    TEST("ser: object with refs but only default SELF_Create values");
    _sertest_init_classes();

    ExternalReference a = Object_CreateRef(CID_Counter);
    ExternalReference b = Object_CreateRef(CID_Counter);
    ASSERT(a != NULL && b != NULL);

    // A has only the default "count" = 0 from SELF_Create, plus a ref to B
    _sertest_increment_n(ObjectContainer_TempFrom(b), 10);
    Object_SStoreRef(ObjectContainer_TempFrom(a), "buddy", ObjectContainer_TempFrom(b));

    ByteStream *stream = Object_Serialize(&a, 1);
    ASSERT(stream != NULL);

    int root_count = 0;
    ExternalReference *loaded = Object_Deserialize(stream, &root_count);
    ASSERT(loaded != NULL);
    ASSERT(root_count == 1);

    TempObjectReference la = ObjectContainer_TempFrom(loaded[0]);
    ASSERT(_sertest_get_count(la) == 0);

    TempObjectReference lb = Object_SGetRef(la, "buddy");
    ASSERT(lb != NULL);
    ASSERT(_sertest_get_count(lb) == 10);

    ByteStream_Destroy(stream);
    ObjectContainer_UnRef_External(&a);
    ObjectContainer_UnRef_External(&b);
    for (int i = 0; i < root_count; i++) {
        ObjectContainer_UnRef_External(&loaded[i]);
    }
    free(loaded);
    Object_GarbageCollect();
    ASSERT(_registry_count() == 0);
    PASS();
}

static void test_ser_multiple_values(void) {
    TEST("ser: 10 different typed values on one object roundtrip");
    _sertest_init_classes();

    ExternalReference counter = Object_CreateRef(CID_Counter);
    ASSERT(counter != NULL);
    TempObjectReference ct = ObjectContainer_TempFrom(counter);

    // Store various typed values
    int v_int = -42;
    _Object_StoreValue(ct->data->values, "v_int", 5,
                       &v_int, sizeof(int), CID_Counter, SER_RAW, 0);

    float v_float = 3.14f;
    _Object_StoreValue(ct->data->values, "v_flt", 5,
                       &v_float, sizeof(float), CID_Counter, SER_RAW, 0);

    uint8_t v_u8 = 255;
    _Object_StoreValue(ct->data->values, "v_u8\0", 4,
                       &v_u8, sizeof(uint8_t), CID_Counter, SER_RAW, 0);

    int32_t v_i32 = -100000;
    _Object_StoreValue(ct->data->values, "v_i32", 5,
                       &v_i32, sizeof(int32_t), CID_Counter, SER_RAW, 0);

    uint32_t v_u32 = 4000000000u;
    _Object_StoreValue(ct->data->values, "v_u32", 5,
                       &v_u32, sizeof(uint32_t), CID_Counter, SER_RAW, 0);

    float v_f2 = -0.001f;
    _Object_StoreValue(ct->data->values, "v_f_2", 5,
                       &v_f2, sizeof(float), CID_Counter, SER_RAW, 0);

    int v_big = 2147483647;
    _Object_StoreValue(ct->data->values, "v_big", 5,
                       &v_big, sizeof(int), CID_Counter, SER_RAW, 0);

    uint8_t v_zero = 0;
    _Object_StoreValue(ct->data->values, "v_zer", 5,
                       &v_zero, sizeof(uint8_t), CID_Counter, SER_RAW, 0);

    int v_neg = -2147483647;
    _Object_StoreValue(ct->data->values, "v_neg", 5,
                       &v_neg, sizeof(int), CID_Counter, SER_RAW, 0);

    float v_f3 = 99999.5f;
    _Object_StoreValue(ct->data->values, "v_f_3", 5,
                       &v_f3, sizeof(float), CID_Counter, SER_RAW, 0);

    // Serialize + deserialize
    ByteStream *stream = Object_Serialize(&counter, 1);
    ASSERT(stream != NULL);

    int root_count = 0;
    ExternalReference *loaded = Object_Deserialize(stream, &root_count);
    ASSERT(loaded != NULL);
    ASSERT(root_count == 1);
    TempObjectReference lt = ObjectContainer_TempFrom(loaded[0]);

    // Verify all values
    int *ri = (int *)_Object_GetValueData(lt->data->values, "v_int", 5);
    ASSERT(ri != NULL && *ri == -42);

    float *rf = (float *)_Object_GetValueData(lt->data->values, "v_flt", 5);
    ASSERT(rf != NULL && *rf == 3.14f);

    uint8_t *ru8 = (uint8_t *)_Object_GetValueData(lt->data->values, "v_u8\0", 4);
    ASSERT(ru8 != NULL && *ru8 == 255);

    int32_t *ri32 = (int32_t *)_Object_GetValueData(lt->data->values, "v_i32", 5);
    ASSERT(ri32 != NULL && *ri32 == -100000);

    uint32_t *ru32 = (uint32_t *)_Object_GetValueData(lt->data->values, "v_u32", 5);
    ASSERT(ru32 != NULL && *ru32 == 4000000000u);

    float *rf2 = (float *)_Object_GetValueData(lt->data->values, "v_f_2", 5);
    ASSERT(rf2 != NULL && *rf2 == -0.001f);

    int *rbig = (int *)_Object_GetValueData(lt->data->values, "v_big", 5);
    ASSERT(rbig != NULL && *rbig == 2147483647);

    uint8_t *rzer = (uint8_t *)_Object_GetValueData(lt->data->values, "v_zer", 5);
    ASSERT(rzer != NULL && *rzer == 0);

    int *rneg = (int *)_Object_GetValueData(lt->data->values, "v_neg", 5);
    ASSERT(rneg != NULL && *rneg == -2147483647);

    float *rf3 = (float *)_Object_GetValueData(lt->data->values, "v_f_3", 5);
    ASSERT(rf3 != NULL && *rf3 == 99999.5f);

    ByteStream_Destroy(stream);
    ObjectContainer_UnRef_External(&counter);
    for (int i = 0; i < root_count; i++) {
        ObjectContainer_UnRef_External(&loaded[i]);
    }
    free(loaded);
    Object_GarbageCollect();
    ASSERT(_registry_count() == 0);
    PASS();
}

// ============================================================
// Forward/backward byte match
// ============================================================

static void test_ser_forward_backward_match(void) {
    TEST("ser: serialize -> deserialize -> re-serialize is byte-identical");
    _sertest_init_classes();

    // Create a complex graph: 3 objects with mixed refs
    ExternalReference ea = Object_CreateRef(CID_Counter);
    ExternalReference eb = Object_CreateRef(CID_Counter);
    ExternalReference ec = Object_CreateRef(CID_Counter);
    ASSERT(ea && eb && ec);

    _sertest_increment_n(ObjectContainer_TempFrom(ea), 10);
    _sertest_increment_n(ObjectContainer_TempFrom(eb), 20);
    _sertest_increment_n(ObjectContainer_TempFrom(ec), 30);

    TempObjectReference a = ObjectContainer_TempFrom(ea);
    TempObjectReference b = ObjectContainer_TempFrom(eb);
    TempObjectReference c = ObjectContainer_TempFrom(ec);

    Object_SStoreRef(a, "b", b);
    Object_SStoreRef(a, "c", c);
    Object_SStoreRef(b, "c", c);

    // Serialize to ByteStream A
    ByteStream *sa = Object_Serialize(&ea, 1);
    ASSERT(sa != NULL);

    // Deserialize A
    int root_count = 0;
    ExternalReference *loaded = Object_Deserialize(sa, &root_count);
    ASSERT(loaded != NULL);
    ASSERT(root_count == 1);

    // Re-serialize to ByteStream B
    ByteStream *sb = Object_Serialize(loaded, root_count);
    ASSERT(sb != NULL);

    // Compare A and B byte-for-byte
    ASSERT(sa->length == sb->length);
    ASSERT(memcmp(sa->data, sb->data, sa->length) == 0);

    ByteStream_Destroy(sa);
    ByteStream_Destroy(sb);
    ObjectContainer_UnRef_External(&ea);
    ObjectContainer_UnRef_External(&eb);
    ObjectContainer_UnRef_External(&ec);
    for (int i = 0; i < root_count; i++) {
        ObjectContainer_UnRef_External(&loaded[i]);
    }
    free(loaded);
    Object_GarbageCollect();
    ASSERT(_registry_count() == 0);
    PASS();
}

// ============================================================
// Security tests -- crafted .cob payloads
// ============================================================

static ByteStream *_sec_make_header(uint32_t obj_count, uint32_t root_count) {
    ByteStream *s = ByteStream_Create(128);
    uint8_t magic[4] = { 'E', 'C', 'O', 'B' };
    ByteStream_Write(s, magic, 4);
    ByteStream_WriteValue(s, uint32_t, 1);
    ByteStream_WriteValue(s, uint32_t, obj_count);
    ByteStream_WriteValue(s, uint32_t, root_count);
    return s;
}

static void test_ser_sec_huge_object_count(void) {
    TEST("ser security: huge object_count rejected");
    ByteStream *s = _sec_make_header(0xFFFFFFFF, 0);
    int rc = 0;
    ExternalReference *res = Object_Deserialize(s, &rc);
    ASSERT(res == NULL);
    ByteStream_Destroy(s);
    PASS();
}

static void test_ser_sec_huge_root_count(void) {
    TEST("ser security: huge root_count rejected");
    ByteStream *s = _sec_make_header(0, 0xFFFFFFFF);
    int rc = 0;
    ExternalReference *res = Object_Deserialize(s, &rc);
    ASSERT(res == NULL);
    ByteStream_Destroy(s);
    PASS();
}

static void test_ser_sec_ser_id_oob(void) {
    TEST("ser security: out-of-bounds ser_id rejected");
    BeginClassRegistrations();
    RegisterClass(Counter_ClassDef());
    EndClassRegistrations();

    ByteStream *s = _sec_make_header(1, 1);
    ByteStream_WriteValue(s, uint32_t, 0);
    ByteStream_WriteValue(s, int32_t, 1);
    // Object with ser_id = 999 (out of range for 1 object)
    ByteStream_WriteValue(s, uint32_t, 999);
    ByteStream_WriteValue(s, uint16_t, CID_Counter);
    ByteStream_WriteValue(s, uint32_t, 0);
    ByteStream_WriteValue(s, uint32_t, 0);

    ByteStream_Rewind(s);
    int rc = 0;
    ExternalReference *res = Object_Deserialize(s, &rc);
    ASSERT(res == NULL);
    ByteStream_Destroy(s);
    PASS();
}

static void test_ser_sec_key_len_overflow(void) {
    TEST("ser security: key_len > 256 rejected");
    BeginClassRegistrations();
    RegisterClass(Counter_ClassDef());
    EndClassRegistrations();

    ByteStream *s = _sec_make_header(1, 1);
    ByteStream_WriteValue(s, uint32_t, 0);
    ByteStream_WriteValue(s, int32_t, 1);
    // Object 0
    ByteStream_WriteValue(s, uint32_t, 0);
    ByteStream_WriteValue(s, uint16_t, CID_Counter);
    // 1 value with key_len = 9999
    ByteStream_WriteValue(s, uint32_t, 1);
    ByteStream_WriteValue(s, uint32_t, 9999);
    // Pad with enough junk
    for (int i = 0; i < 2500; i++) ByteStream_WriteValue(s, uint32_t, 0);

    ByteStream_Rewind(s);
    int rc = 0;
    ExternalReference *res = Object_Deserialize(s, &rc);
    ASSERT(res == NULL);
    ByteStream_Destroy(s);
    PASS();
}

static void test_ser_sec_huge_data_size(void) {
    TEST("ser security: data_size > SER_MAX_DATA_SIZE rejected");
    BeginClassRegistrations();
    RegisterClass(Counter_ClassDef());
    EndClassRegistrations();

    ByteStream *s = _sec_make_header(1, 1);
    ByteStream_WriteValue(s, uint32_t, 0);
    ByteStream_WriteValue(s, int32_t, 1);
    // Object 0
    ByteStream_WriteValue(s, uint32_t, 0);
    ByteStream_WriteValue(s, uint16_t, CID_Counter);
    // 1 value with valid key but huge data_size
    ByteStream_WriteValue(s, uint32_t, 1);
    ByteStream_WriteValue(s, uint32_t, 1);
    uint8_t key = 'x';
    ByteStream_Write(s, &key, 1);
    ByteStream_WriteValue(s, uint16_t, 0);
    ByteStream_WriteValue(s, uint16_t, SER_RAW);
    ByteStream_WriteValue(s, uint16_t, 0);
    ByteStream_WriteValue(s, uint32_t, 0xFFFFFFFF);

    ByteStream_Rewind(s);
    int rc = 0;
    ExternalReference *res = Object_Deserialize(s, &rc);
    ASSERT(res == NULL);
    ByteStream_Destroy(s);
    PASS();
}

static void test_ser_sec_truncated_stream(void) {
    TEST("ser security: truncated stream rejected");
    ByteStream *s = _sec_make_header(5, 1);
    // Root table for 1 root
    ByteStream_WriteValue(s, uint32_t, 0);
    ByteStream_WriteValue(s, int32_t, 1);
    // Only 1 object header instead of 5
    ByteStream_WriteValue(s, uint32_t, 0);
    ByteStream_WriteValue(s, uint16_t, 0x0010);
    // Truncated -- no value_count

    ByteStream_Rewind(s);
    int rc = 0;
    ExternalReference *res = Object_Deserialize(s, &rc);
    ASSERT(res == NULL);
    ByteStream_Destroy(s);
    PASS();
}

static void test_ser_sec_huge_value_count(void) {
    TEST("ser security: value_count > SER_MAX_VALUES_PER_OBJECT rejected");
    BeginClassRegistrations();
    RegisterClass(Counter_ClassDef());
    EndClassRegistrations();

    ByteStream *s = _sec_make_header(1, 1);
    ByteStream_WriteValue(s, uint32_t, 0);
    ByteStream_WriteValue(s, int32_t, 1);
    ByteStream_WriteValue(s, uint32_t, 0);
    ByteStream_WriteValue(s, uint16_t, CID_Counter);
    ByteStream_WriteValue(s, uint32_t, 0xFFFFFFFF);

    ByteStream_Rewind(s);
    int rc = 0;
    ExternalReference *res = Object_Deserialize(s, &rc);
    ASSERT(res == NULL);
    ByteStream_Destroy(s);
    PASS();
}

static void test_ser_sec_target_id_oob_in_refs(void) {
    TEST("ser security: target_id >= object_count in refs is skipped");
    BeginClassRegistrations();
    RegisterClass(Counter_ClassDef());
    EndClassRegistrations();

    // Create a valid serialized stream manually with 1 object, 0 values,
    // 1 ref with target_id = 999
    ByteStream *s = _sec_make_header(1, 1);
    ByteStream_WriteValue(s, uint32_t, 0);
    ByteStream_WriteValue(s, int32_t, 1);
    // Object 0
    ByteStream_WriteValue(s, uint32_t, 0);
    ByteStream_WriteValue(s, uint16_t, CID_Counter);
    ByteStream_WriteValue(s, uint32_t, 0); // 0 values
    // 1 ref
    ByteStream_WriteValue(s, uint32_t, 1);
    uint32_t kl = 3;
    ByteStream_Write(s, &kl, sizeof(uint32_t));
    ByteStream_Write(s, "bad", 3);
    ByteStream_WriteValue(s, uint32_t, 999); // OOB target

    ByteStream_Rewind(s);
    int rc = 0;
    ExternalReference *res = Object_Deserialize(s, &rc);
    // Should succeed but skip the bad ref
    ASSERT(res != NULL);
    ASSERT(rc == 1);
    ASSERT(res[0] != NULL);
    // The bad ref should not exist
    ASSERT(!UnsafeHashMap_Has(res[0]->data->references, "bad", 3));

    ObjectContainer_UnRef_External(&res[0]);
    Object_GarbageCollect();
    free(res);
    PASS();
}

// ============================================================
// Runner
// ============================================================

static void run_serialization_tests(void) {
    LOG_INFO("=== Serialization Tests ===");

    LOG_INFO("--- Ser Registry ---");
    test_ser_builtins_registered();
    test_ser_register_custom();
    test_ser_custom_function_roundtrip();

    LOG_INFO("--- Ser Value Headers ---");
    test_ser_value_header_raw();
    test_ser_value_header_transient();
    test_ser_value_header_custom();

    LOG_INFO("--- Single Object Roundtrip ---");
    test_ser_single_object_roundtrip();
    test_ser_single_object_byte_match();
    test_ser_empty_object();

    LOG_INFO("--- Transient Values ---");
    test_ser_transient_skipped();

    LOG_INFO("--- Multi-Object Roundtrip ---");
    test_ser_two_objects_no_refs();
    test_ser_object_with_ref();
    test_ser_mutual_refs();
    test_ser_self_ref();
    test_ser_diamond_refs();

    LOG_INFO("--- File I/O ---");
    test_ser_save_load_file();
    test_ser_load_bad_file();
    test_ser_save_load_byte_match();

    LOG_INFO("--- Edge Cases ---");
    test_ser_no_roots();
    test_ser_null_roots();
    test_ser_many_objects();
    test_ser_object_no_values_only_refs();
    test_ser_multiple_values();

    LOG_INFO("--- Forward/Backward Byte Match ---");
    test_ser_forward_backward_match();

    LOG_INFO("--- Security: Malicious .cob ---");
    test_ser_sec_huge_object_count();
    test_ser_sec_huge_root_count();
    test_ser_sec_ser_id_oob();
    test_ser_sec_key_len_overflow();
    test_ser_sec_huge_data_size();
    test_ser_sec_truncated_stream();
    test_ser_sec_huge_value_count();
    test_ser_sec_target_id_oob_in_refs();
}
