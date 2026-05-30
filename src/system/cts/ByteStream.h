#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../utils.h"

#ifndef BYTESTREAM_MAX_SIZE
#define BYTESTREAM_MAX_SIZE (1024u * 1024u * 1024u)
#endif

typedef struct ByteStream {
    uint8_t *data;
    uint32_t capacity;
    uint32_t length;
    uint32_t cursor;
} ByteStream;

static ByteStream *ByteStream_Create(uint32_t capacity) {
    if (capacity == 0) capacity = 1;
    ByteStream *stream = (ByteStream *)malloc(sizeof(ByteStream));
    if (!stream) return NULL;
    stream->data = (uint8_t *)malloc(capacity);
    if (!stream->data) { free(stream); return NULL; }
    stream->capacity = capacity;
    stream->length = 0;
    stream->cursor = 0;
    return stream;
}

static void ByteStream_Destroy(ByteStream *stream) {
    free(stream->data);
    free(stream);
}

static int _ByteStream_Grow(ByteStream *stream, uint32_t needed) {
    if (needed > BYTESTREAM_MAX_SIZE) return -1;
    uint32_t cap = stream->capacity;
    while (cap < needed) cap *= 2;
    if (cap > BYTESTREAM_MAX_SIZE) cap = BYTESTREAM_MAX_SIZE;
    uint8_t *newbuf = (uint8_t *)realloc(stream->data, cap);
    if (!newbuf) return -1;
    stream->data = newbuf;
    stream->capacity = cap;
    return 0;
}

// Overflow-safe add. Returns 1 if a + b would wrap uint32_t.
static int _ByteStream_WouldOverflow(uint32_t a, uint32_t b) {
    return a > UINT32_MAX - b;
}

static int ByteStream_Write(ByteStream *stream, const void *data, uint32_t size) {
    if (_ByteStream_WouldOverflow(stream->cursor, size)) return -1;
    uint32_t end = stream->cursor + size;
    if (end > stream->capacity) {
        if (_ByteStream_Grow(stream, end) != 0) return -1;
    }
    memcpy(stream->data + stream->cursor, data, size);
    stream->cursor = end;
    if (end > stream->length) stream->length = end;
    return 0;
}

static int ByteStream_Read(ByteStream *stream, void *out, uint32_t size) {
    if (_ByteStream_WouldOverflow(stream->cursor, size)) return -1;
    if (stream->cursor + size > stream->length) return -1;
    memcpy(out, stream->data + stream->cursor, size);
    stream->cursor += size;
    return 0;
}

// Returns a pointer into the buffer and advances the cursor,
// or NULL if the read would go past length.
static void *_ByteStream_ReadInPlace(ByteStream *stream, uint32_t size) {
    if (_ByteStream_WouldOverflow(stream->cursor, size)) return NULL;
    if (stream->cursor + size > stream->length) return NULL;
    void *ptr = stream->data + stream->cursor;
    stream->cursor += size;
    return ptr;
}

static void ByteStream_Seek(ByteStream *stream, uint32_t position) {
    stream->cursor = position < stream->length ? position : stream->length;
}

static void ByteStream_Skip(ByteStream *stream, uint32_t bytes) {
    uint32_t remaining = stream->length - stream->cursor;
    stream->cursor += bytes < remaining ? bytes : remaining;
}

static void ByteStream_Rewind(ByteStream *stream) {
    stream->cursor = 0;
}

static uint32_t ByteStream_Remaining(ByteStream *stream) {
    return stream->length - stream->cursor;
}

static void ByteStream_Clear(ByteStream *stream) {
    stream->length = 0;
    stream->cursor = 0;
}

#define ByteStream_WriteValue(stream, type, value) \
    ByteStream_Write(stream, &(type){value}, sizeof(type))

// Returns a zero-initialized value of the given type if the read fails.
#define ByteStream_ReadDeref(stream, type) ({ \
    void *_brd_ptr = _ByteStream_ReadInPlace(stream, sizeof(type)); \
    _brd_ptr ? *(type*)_brd_ptr : (type){0}; \
})

#define ByteStream_PeekDeref(stream, type) ({ \
    type _bpd_val = (type){0}; \
    if ((stream)->cursor + sizeof(type) <= (stream)->length) \
        _bpd_val = *(type*)((stream)->data + (stream)->cursor); \
    _bpd_val; \
})

// ============================================================
// Print / Log
// ============================================================

#define LINTNORE
static void ByteStream_PrintHex(ByteStream *stream) {
    printf("ByteStream[%u/%u bytes, cursor=%u] hex {\n  ", stream->length, stream->capacity, stream->cursor);
    for (uint32_t i = 0; i < stream->length; i++) {
        printf("%02X ", stream->data[i]);
        if ((i + 1) % 16 == 0 && i + 1 < stream->length) printf("\n  ");
    }
    printf("\n}\n");
}

static void ByteStream_PrintBinary(ByteStream *stream) {
    printf("ByteStream[%u/%u bytes, cursor=%u] binary {\n  ", stream->length, stream->capacity, stream->cursor);
    for (uint32_t i = 0; i < stream->length; i++) {
        for (int b = 7; b >= 0; b--) printf("%c", (stream->data[i] >> b) & 1 ? '1' : '0');
        printf(" ");
        if ((i + 1) % 8 == 0 && i + 1 < stream->length) printf("\n  ");
    }
    printf("\n}\n");
}

static void ByteStream_PrintAscii(ByteStream *stream) {
    printf("ByteStream[%u/%u bytes, cursor=%u] ascii {\n  ", stream->length, stream->capacity, stream->cursor);
    for (uint32_t i = 0; i < stream->length; i++) {
        uint8_t c = stream->data[i];
        printf("%c", (c >= 32 && c <= 126) ? c : '?');
    }
    printf("\n}\n");
}
#undef LINTNORE

static void ByteStream_LogHex(ByteStream *stream) {
    LOG_INFO("ByteStream[%u/%u bytes, cursor=%u] hex {", stream->length, stream->capacity, stream->cursor);
    char line[64];
    uint32_t pos = 0;
    for (uint32_t i = 0; i < stream->length; i++) {
        pos += (uint32_t)snprintf(line + pos, sizeof(line) - pos, "%02X ", stream->data[i]);
        if ((i + 1) % 16 == 0 || i + 1 == stream->length) {
            LOG_INFO("  %s", line);
            pos = 0;
        }
    }
    LOG_INFO("}");
}

static void ByteStream_LogBinary(ByteStream *stream) {
    LOG_INFO("ByteStream[%u/%u bytes, cursor=%u] binary {", stream->length, stream->capacity, stream->cursor);
    char line[128];
    uint32_t pos = 0;
    for (uint32_t i = 0; i < stream->length; i++) {
        for (int b = 7; b >= 0; b--) line[pos++] = (stream->data[i] >> b) & 1 ? '1' : '0';
        line[pos++] = ' ';
        if ((i + 1) % 8 == 0 || i + 1 == stream->length) {
            line[pos] = '\0';
            LOG_INFO("  %s", line);
            pos = 0;
        }
    }
    LOG_INFO("}");
}

static void ByteStream_LogAscii(ByteStream *stream) {
    LOG_INFO("ByteStream[%u/%u bytes, cursor=%u] ascii {", stream->length, stream->capacity, stream->cursor);
    char line[80];
    uint32_t pos = 0;
    for (uint32_t i = 0; i < stream->length; i++) {
        uint8_t c = stream->data[i];
        line[pos++] = (c >= 32 && c <= 126) ? c : '?';
        if (pos >= 64 || i + 1 == stream->length) {
            line[pos] = '\0';
            LOG_INFO("  %s", line);
            pos = 0;
        }
    }
    LOG_INFO("}");
}

// ============================================================
// File I/O
// ============================================================

// File format:
//   4 bytes: magic "EBSF" (Enatrio ByteStream File)
//   4 bytes: format version (uint32_t, little-endian)
//   4 bytes: data length in bytes (uint32_t, little-endian)
//   N bytes: raw data
//
// Little-endian is written/read byte-by-byte so the file is
// portable across platforms regardless of host endianness.

#define _BYTESTREAM_MAGIC_0 'E'
#define _BYTESTREAM_MAGIC_1 'B'
#define _BYTESTREAM_MAGIC_2 'S'
#define _BYTESTREAM_MAGIC_3 'F'
#define _BYTESTREAM_VERSION 1

static void _ByteStream_WriteLE32(FILE *f, uint32_t val) {
    uint8_t buf[4];
    buf[0] = (uint8_t)(val);
    buf[1] = (uint8_t)(val >> 8);
    buf[2] = (uint8_t)(val >> 16);
    buf[3] = (uint8_t)(val >> 24);
    fwrite(buf, 1, 4, f);
}

// Returns 0 on success, -1 if fread got fewer than 4 bytes.
static int _ByteStream_ReadLE32(FILE *f, uint32_t *out) {
    uint8_t buf[4];
    if (fread(buf, 1, 4, f) != 4) return -1;
    *out = (uint32_t)buf[0]
         | ((uint32_t)buf[1] << 8)
         | ((uint32_t)buf[2] << 16)
         | ((uint32_t)buf[3] << 24);
    return 0;
}

static int ByteStream_SaveToFile(ByteStream *stream, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        LOG_ERROR("ByteStream: failed to open '%s' for writing", path);
        return -1;
    }

    uint8_t magic[4] = { _BYTESTREAM_MAGIC_0, _BYTESTREAM_MAGIC_1,
                          _BYTESTREAM_MAGIC_2, _BYTESTREAM_MAGIC_3 };
    fwrite(magic, 1, 4, f);
    _ByteStream_WriteLE32(f, _BYTESTREAM_VERSION);
    _ByteStream_WriteLE32(f, stream->length);
    if (stream->length > 0) {
        fwrite(stream->data, 1, stream->length, f);
    }

    fclose(f);
    return 0;
}

// Returns a new ByteStream with cursor at 0, or NULL on failure.
// Rejects files with data length exceeding BYTESTREAM_MAX_SIZE.
static ByteStream *ByteStream_LoadFromFile(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        LOG_ERROR("ByteStream: failed to open '%s' for reading", path);
        return NULL;
    }

    uint8_t magic[4];
    if (fread(magic, 1, 4, f) != 4) {
        LOG_ERROR("ByteStream: '%s' too short for header", path);
        fclose(f);
        return NULL;
    }
    if (magic[0] != _BYTESTREAM_MAGIC_0 || magic[1] != _BYTESTREAM_MAGIC_1 ||
        magic[2] != _BYTESTREAM_MAGIC_2 || magic[3] != _BYTESTREAM_MAGIC_3) {
        LOG_ERROR("ByteStream: '%s' bad magic (expected EBSF)", path);
        fclose(f);
        return NULL;
    }

    uint32_t version;
    if (_ByteStream_ReadLE32(f, &version) != 0) {
        LOG_ERROR("ByteStream: '%s' truncated version field", path);
        fclose(f);
        return NULL;
    }
    if (version != _BYTESTREAM_VERSION) {
        LOG_ERROR("ByteStream: '%s' unsupported version %u (expected %u)", path, version, _BYTESTREAM_VERSION);
        fclose(f);
        return NULL;
    }

    uint32_t length;
    if (_ByteStream_ReadLE32(f, &length) != 0) {
        LOG_ERROR("ByteStream: '%s' truncated length field", path);
        fclose(f);
        return NULL;
    }
    if (length > BYTESTREAM_MAX_SIZE) {
        LOG_ERROR("ByteStream: '%s' data length %u exceeds max (%u)", path, length, (uint32_t)BYTESTREAM_MAX_SIZE);
        fclose(f);
        return NULL;
    }

    ByteStream *stream = ByteStream_Create(length > 0 ? length : 1);
    if (!stream) {
        LOG_ERROR("ByteStream: '%s' allocation failed for %u bytes", path, length);
        fclose(f);
        return NULL;
    }
    if (length > 0) {
        if (fread(stream->data, 1, length, f) != length) {
            LOG_ERROR("ByteStream: '%s' truncated data (expected %u bytes)", path, length);
            ByteStream_Destroy(stream);
            fclose(f);
            return NULL;
        }
    }
    stream->length = length;
    stream->cursor = 0;

    fclose(f);
    return stream;
}
