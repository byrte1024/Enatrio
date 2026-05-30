#pragma once

#include "../system/tests.h"
#include "../system/cts/ByteStream.h"

static void test_bytestream_create_destroy(void) {
    TEST("bytestream: create and destroy");
    ByteStream *s = ByteStream_Create(32);
    ASSERT(s != NULL);
    ASSERT(s->capacity == 32);
    ASSERT(s->length == 0);
    ASSERT(s->cursor == 0);
    ByteStream_Destroy(s);
    PASS();
}

static void test_bytestream_write_read_int(void) {
    TEST("bytestream: write and read int");
    ByteStream *s = ByteStream_Create(16);
    ByteStream_WriteValue(s, int, 42);
    ByteStream_WriteValue(s, int, -7);
    ASSERT(s->length == sizeof(int) * 2);
    ASSERT(s->cursor == sizeof(int) * 2);

    ByteStream_Rewind(s);
    int a = ByteStream_ReadDeref(s, int);
    int b = ByteStream_ReadDeref(s, int);
    ASSERT(a == 42);
    ASSERT(b == -7);
    ByteStream_Destroy(s);
    PASS();
}

static void test_bytestream_write_read_mixed_types(void) {
    TEST("bytestream: write and read mixed types");
    ByteStream *s = ByteStream_Create(64);
    ByteStream_WriteValue(s, uint8_t, 0xFF);
    ByteStream_WriteValue(s, float, 3.14f);
    ByteStream_WriteValue(s, uint32_t, 1000000);
    ByteStream_WriteValue(s, int16_t, -123);

    ByteStream_Rewind(s);
    ASSERT(ByteStream_ReadDeref(s, uint8_t) == 0xFF);
    float f = ByteStream_ReadDeref(s, float);
    ASSERT(f > 3.13f && f < 3.15f);
    ASSERT(ByteStream_ReadDeref(s, uint32_t) == 1000000);
    ASSERT(ByteStream_ReadDeref(s, int16_t) == -123);
    ByteStream_Destroy(s);
    PASS();
}

static void test_bytestream_write_raw_bytes(void) {
    TEST("bytestream: write raw bytes");
    ByteStream *s = ByteStream_Create(16);
    const char *msg = "hello";
    ByteStream_Write(s, msg, 5);
    ASSERT(s->length == 5);

    ByteStream_Rewind(s);
    char buf[6] = {0};
    ASSERT(ByteStream_Read(s, buf, 5) == 0);
    ASSERT(memcmp(buf, "hello", 5) == 0);
    ByteStream_Destroy(s);
    PASS();
}

static void test_bytestream_auto_grow(void) {
    TEST("bytestream: auto grow");
    ByteStream *s = ByteStream_Create(4);
    ASSERT(s->capacity == 4);
    for (int i = 0; i < 100; i++) {
        ByteStream_WriteValue(s, int, i);
    }
    ASSERT(s->length == sizeof(int) * 100);
    ASSERT(s->capacity >= s->length);

    ByteStream_Rewind(s);
    for (int i = 0; i < 100; i++) {
        ASSERT(ByteStream_ReadDeref(s, int) == i);
    }
    ByteStream_Destroy(s);
    PASS();
}

static void test_bytestream_read_past_length(void) {
    TEST("bytestream: read past length fails");
    ByteStream *s = ByteStream_Create(16);
    ByteStream_WriteValue(s, int, 42);
    ByteStream_Rewind(s);

    int val;
    ASSERT(ByteStream_Read(s, &val, sizeof(int)) == 0);
    ASSERT(val == 42);
    ASSERT(ByteStream_Read(s, &val, sizeof(int)) == -1);
    ASSERT(s->cursor == sizeof(int));
    ByteStream_Destroy(s);
    PASS();
}

static void test_bytestream_seek(void) {
    TEST("bytestream: seek");
    ByteStream *s = ByteStream_Create(32);
    ByteStream_WriteValue(s, int, 10);
    ByteStream_WriteValue(s, int, 20);
    ByteStream_WriteValue(s, int, 30);

    ByteStream_Seek(s, sizeof(int));
    ASSERT(s->cursor == sizeof(int));
    ASSERT(ByteStream_ReadDeref(s, int) == 20);

    ByteStream_Seek(s, 0);
    ASSERT(ByteStream_ReadDeref(s, int) == 10);

    ByteStream_Seek(s, 99999);
    ASSERT(s->cursor == s->length);
    ByteStream_Destroy(s);
    PASS();
}

static void test_bytestream_skip(void) {
    TEST("bytestream: skip");
    ByteStream *s = ByteStream_Create(32);
    ByteStream_WriteValue(s, int, 10);
    ByteStream_WriteValue(s, int, 20);
    ByteStream_WriteValue(s, int, 30);

    ByteStream_Rewind(s);
    ByteStream_Skip(s, sizeof(int) * 2);
    ASSERT(ByteStream_ReadDeref(s, int) == 30);

    ByteStream_Rewind(s);
    ByteStream_Skip(s, 99999);
    ASSERT(s->cursor == s->length);
    ByteStream_Destroy(s);
    PASS();
}

static void test_bytestream_remaining(void) {
    TEST("bytestream: remaining");
    ByteStream *s = ByteStream_Create(32);
    ByteStream_WriteValue(s, int, 1);
    ByteStream_WriteValue(s, int, 2);
    ByteStream_Rewind(s);
    ASSERT(ByteStream_Remaining(s) == sizeof(int) * 2);
    ByteStream_ReadDeref(s, int);
    ASSERT(ByteStream_Remaining(s) == sizeof(int));
    ByteStream_ReadDeref(s, int);
    ASSERT(ByteStream_Remaining(s) == 0);
    ByteStream_Destroy(s);
    PASS();
}

static void test_bytestream_clear(void) {
    TEST("bytestream: clear");
    ByteStream *s = ByteStream_Create(16);
    ByteStream_WriteValue(s, int, 42);
    ASSERT(s->length > 0);
    ByteStream_Clear(s);
    ASSERT(s->length == 0);
    ASSERT(s->cursor == 0);
    ASSERT(s->capacity >= 16);
    ByteStream_Destroy(s);
    PASS();
}

static void test_bytestream_overwrite(void) {
    TEST("bytestream: overwrite existing data");
    ByteStream *s = ByteStream_Create(16);
    ByteStream_WriteValue(s, int, 100);
    ByteStream_WriteValue(s, int, 200);
    uint32_t original_length = s->length;

    ByteStream_Seek(s, 0);
    ByteStream_WriteValue(s, int, 999);
    ASSERT(s->length == original_length);

    ByteStream_Seek(s, 0);
    ASSERT(ByteStream_ReadDeref(s, int) == 999);
    ASSERT(ByteStream_ReadDeref(s, int) == 200);
    ByteStream_Destroy(s);
    PASS();
}

static void test_bytestream_peek(void) {
    TEST("bytestream: peek does not advance cursor");
    ByteStream *s = ByteStream_Create(16);
    ByteStream_WriteValue(s, int, 42);
    ByteStream_Rewind(s);

    int peeked = ByteStream_PeekDeref(s, int);
    ASSERT(peeked == 42);
    ASSERT(s->cursor == 0);

    int read = ByteStream_ReadDeref(s, int);
    ASSERT(read == 42);
    ASSERT(s->cursor == sizeof(int));
    ByteStream_Destroy(s);
    PASS();
}

static void test_bytestream_save_load(void) {
    TEST("bytestream: save and load from file");
    ByteStream *s = ByteStream_Create(32);
    ByteStream_WriteValue(s, uint32_t, 0xDEADBEEF);
    ByteStream_WriteValue(s, float, 2.718f);
    ByteStream_WriteValue(s, int16_t, -42);
    const char *tag = "test";
    ByteStream_Write(s, tag, 4);

    const char *path = "/tmp/enatrio_bytestream_test.ebsf";
    ASSERT(ByteStream_SaveToFile(s, path) == 0);

    ByteStream *loaded = ByteStream_LoadFromFile(path);
    ASSERT(loaded != NULL);
    ASSERT(loaded->length == s->length);
    ASSERT(loaded->cursor == 0);
    ASSERT(memcmp(loaded->data, s->data, s->length) == 0);

    ASSERT(ByteStream_ReadDeref(loaded, uint32_t) == 0xDEADBEEF);
    float f = ByteStream_ReadDeref(loaded, float);
    ASSERT(f > 2.717f && f < 2.719f);
    ASSERT(ByteStream_ReadDeref(loaded, int16_t) == -42);
    char buf[5] = {0};
    ByteStream_Read(loaded, buf, 4);
    ASSERT(memcmp(buf, "test", 4) == 0);

    ByteStream_Destroy(s);
    ByteStream_Destroy(loaded);
    remove(path);
    PASS();
}

static void test_bytestream_load_bad_magic(void) {
    TEST("bytestream: load rejects bad magic");
    const char *path = "/tmp/enatrio_bytestream_badmagic.bin";
    FILE *f = fopen(path, "wb");
    ASSERT(f != NULL);
    fwrite("JUNK", 1, 4, f);
    fclose(f);

    ByteStream *loaded = ByteStream_LoadFromFile(path);
    ASSERT(loaded == NULL);
    remove(path);
    PASS();
}

static void test_bytestream_load_empty(void) {
    TEST("bytestream: save and load empty stream");
    ByteStream *s = ByteStream_Create(8);
    const char *path = "/tmp/enatrio_bytestream_empty.ebsf";
    ASSERT(ByteStream_SaveToFile(s, path) == 0);

    ByteStream *loaded = ByteStream_LoadFromFile(path);
    ASSERT(loaded != NULL);
    ASSERT(loaded->length == 0);
    ASSERT(loaded->cursor == 0);

    ByteStream_Destroy(s);
    ByteStream_Destroy(loaded);
    remove(path);
    PASS();
}

static void test_bytestream_struct_roundtrip(void) {
    TEST("bytestream: struct round-trip");
    typedef struct { float x; float y; float z; } Vec3;

    ByteStream *s = ByteStream_Create(64);
    Vec3 v1 = {1.0f, 2.0f, 3.0f};
    Vec3 v2 = {4.0f, 5.0f, 6.0f};
    ByteStream_Write(s, &v1, sizeof(Vec3));
    ByteStream_Write(s, &v2, sizeof(Vec3));

    ByteStream_Rewind(s);
    Vec3 out1, out2;
    ByteStream_Read(s, &out1, sizeof(Vec3));
    ByteStream_Read(s, &out2, sizeof(Vec3));
    ASSERT(out1.x == 1.0f && out1.y == 2.0f && out1.z == 3.0f);
    ASSERT(out2.x == 4.0f && out2.y == 5.0f && out2.z == 6.0f);
    ByteStream_Destroy(s);
    PASS();
}

static void test_bytestream_readderef_past_length(void) {
    TEST("bytestream: ReadDeref past length returns zero");
    ByteStream *s = ByteStream_Create(16);
    ByteStream_WriteValue(s, int, 42);
    ByteStream_Rewind(s);

    ASSERT(ByteStream_ReadDeref(s, int) == 42);
    int past = ByteStream_ReadDeref(s, int);
    ASSERT(past == 0);
    ByteStream_Destroy(s);
    PASS();
}

static void test_bytestream_peekderef_past_length(void) {
    TEST("bytestream: PeekDeref past length returns zero");
    ByteStream *s = ByteStream_Create(16);
    ByteStream_WriteValue(s, uint8_t, 0xAB);

    ByteStream_Seek(s, 0);
    ASSERT(ByteStream_PeekDeref(s, uint8_t) == 0xAB);
    ASSERT(s->cursor == 0);

    ByteStream_Seek(s, 1);
    int past = ByteStream_PeekDeref(s, int);
    ASSERT(past == 0);
    ByteStream_Destroy(s);
    PASS();
}

static void test_bytestream_write_returns_error_on_overflow(void) {
    TEST("bytestream: Write rejects uint32 overflow");
    ByteStream *s = ByteStream_Create(16);
    ByteStream_WriteValue(s, int, 1);

    s->cursor = UINT32_MAX - 2;
    int result = ByteStream_WriteValue(s, int, 99);
    ASSERT(result == -1);
    ByteStream_Destroy(s);
    PASS();
}

static void test_bytestream_read_rejects_overflow(void) {
    TEST("bytestream: Read rejects uint32 overflow");
    ByteStream *s = ByteStream_Create(16);
    ByteStream_WriteValue(s, int, 1);
    s->cursor = UINT32_MAX - 2;
    int val;
    ASSERT(ByteStream_Read(s, &val, sizeof(int)) == -1);
    ByteStream_Destroy(s);
    PASS();
}

static void test_bytestream_skip_clamps(void) {
    TEST("bytestream: Skip clamps to length");
    ByteStream *s = ByteStream_Create(16);
    ByteStream_WriteValue(s, int, 1);
    ByteStream_Rewind(s);
    ByteStream_Skip(s, UINT32_MAX);
    ASSERT(s->cursor == s->length);
    ByteStream_Destroy(s);
    PASS();
}

static void test_bytestream_load_truncated_header(void) {
    TEST("bytestream: load rejects truncated header");
    const char *path = "/tmp/enatrio_bytestream_trunc.bin";
    FILE *f = fopen(path, "wb");
    ASSERT(f != NULL);
    fwrite("EBSF\x01", 1, 5, f);
    fclose(f);

    ByteStream *loaded = ByteStream_LoadFromFile(path);
    ASSERT(loaded == NULL);
    remove(path);
    PASS();
}

static void test_bytestream_load_rejects_huge_length(void) {
    TEST("bytestream: load rejects length > BYTESTREAM_MAX_SIZE");
    const char *path = "/tmp/enatrio_bytestream_huge.bin";
    FILE *f = fopen(path, "wb");
    ASSERT(f != NULL);
    fwrite("EBSF", 1, 4, f);
    uint8_t ver[4] = {1, 0, 0, 0};
    fwrite(ver, 1, 4, f);
    uint8_t len[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    fwrite(len, 1, 4, f);
    fclose(f);

    ByteStream *loaded = ByteStream_LoadFromFile(path);
    ASSERT(loaded == NULL);
    remove(path);
    PASS();
}

static void test_bytestream_load_truncated_data(void) {
    TEST("bytestream: load rejects truncated data");
    const char *path = "/tmp/enatrio_bytestream_truncdata.bin";
    FILE *f = fopen(path, "wb");
    ASSERT(f != NULL);
    fwrite("EBSF", 1, 4, f);
    uint8_t ver[4] = {1, 0, 0, 0};
    fwrite(ver, 1, 4, f);
    uint8_t len[4] = {100, 0, 0, 0};
    fwrite(len, 1, 4, f);
    uint8_t partial[10] = {0};
    fwrite(partial, 1, 10, f);
    fclose(f);

    ByteStream *loaded = ByteStream_LoadFromFile(path);
    ASSERT(loaded == NULL);
    remove(path);
    PASS();
}

static void test_bytestream_create_zero_capacity(void) {
    TEST("bytestream: Create with zero capacity succeeds");
    ByteStream *s = ByteStream_Create(0);
    ASSERT(s != NULL);
    ASSERT(s->capacity >= 1);
    ByteStream_WriteValue(s, int, 42);
    ByteStream_Rewind(s);
    ASSERT(ByteStream_ReadDeref(s, int) == 42);
    ByteStream_Destroy(s);
    PASS();
}

static void run_bytestream_tests(void) {
    LOG_INFO("=== ByteStream Tests ===");
    test_bytestream_create_destroy();
    test_bytestream_write_read_int();
    test_bytestream_write_read_mixed_types();
    test_bytestream_write_raw_bytes();
    test_bytestream_auto_grow();
    test_bytestream_read_past_length();
    test_bytestream_seek();
    test_bytestream_skip();
    test_bytestream_remaining();
    test_bytestream_clear();
    test_bytestream_overwrite();
    test_bytestream_peek();
    test_bytestream_save_load();
    test_bytestream_load_bad_magic();
    test_bytestream_load_empty();
    test_bytestream_struct_roundtrip();
    test_bytestream_readderef_past_length();
    test_bytestream_peekderef_past_length();
    test_bytestream_write_returns_error_on_overflow();
    test_bytestream_read_rejects_overflow();
    test_bytestream_skip_clamps();
    test_bytestream_load_truncated_header();
    test_bytestream_load_rejects_huge_length();
    test_bytestream_load_truncated_data();
    test_bytestream_create_zero_capacity();
}
