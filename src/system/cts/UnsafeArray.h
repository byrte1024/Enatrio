#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../utils.h"

typedef struct UnsafeArray {
    uint8_t *data;
    uint32_t element_size;
    uint32_t capacity;
    uint32_t count;
} UnsafeArray;

static UnsafeArray *UnsafeArray_Create(uint32_t element_size, uint32_t capacity) {
    if (capacity == 0) capacity = 1;
    UnsafeArray *arr = (UnsafeArray *)malloc(sizeof(UnsafeArray));
    if (!arr) return NULL;
    arr->data = (uint8_t *)malloc((size_t)capacity * element_size);
    if (!arr->data) { free(arr); return NULL; }
    arr->element_size = element_size;
    arr->capacity = capacity;
    arr->count = 0;
    return arr;
}

static void UnsafeArray_Destroy(UnsafeArray *arr) {
    free(arr->data);
    free(arr);
}

// Caller MUST ensure index < arr->count. No bounds check ("Unsafe" contract).
static void *UnsafeArray_Get(UnsafeArray *arr, uint32_t index) {
    if (index >= arr->count) return NULL;
    return arr->data + (size_t)index * arr->element_size;
}
static void UnsafeArray_Set(UnsafeArray *arr, uint32_t index, const void *value) {
    memcpy(arr->data + (size_t)index * arr->element_size, value, arr->element_size);
}

static int _UnsafeArray_Grow(UnsafeArray *arr) {
    uint32_t new_cap = arr->capacity;
    if (new_cap > UINT32_MAX / 2) new_cap = UINT32_MAX;
    else new_cap *= 2;
    uint8_t *newbuf = (uint8_t *)realloc(arr->data, (size_t)new_cap * arr->element_size);
    if (!newbuf) return -1;
    arr->data = newbuf;
    arr->capacity = new_cap;
    return 0;
}

static int UnsafeArray_Add(UnsafeArray *arr, const void *value) {
    if (arr->count >= arr->capacity) {
        if (_UnsafeArray_Grow(arr) != 0) return -1;
    }
    memcpy(arr->data + (size_t)arr->count * arr->element_size, value, arr->element_size);
    arr->count++;
    return 0;
}

// O(1) removal by swapping with the last element -- use when order does not matter.
static int UnsafeArray_RemoveSwap(UnsafeArray *arr, uint32_t index) {
    if (index >= arr->count) return -1;
    arr->count--;
    if (index != arr->count) {
        memcpy(arr->data + (size_t)index * arr->element_size,
               arr->data + (size_t)arr->count * arr->element_size,
               arr->element_size);
    }
    return 0;
}

// O(n) removal that preserves element order -- use when index stability matters.
static int UnsafeArray_Remove(UnsafeArray *arr, uint32_t index) {
    if (index >= arr->count) return -1;
    arr->count--;
    if (index != arr->count) {
        memmove(arr->data + (size_t)index * arr->element_size,
                arr->data + (size_t)(index + 1) * arr->element_size,
                (size_t)(arr->count - index) * arr->element_size);
    }
    return 0;
}

static void UnsafeArray_Clear(UnsafeArray *arr) {
    arr->count = 0;
}

static int UnsafeArray_AddBulk(UnsafeArray *arr, const void *data, uint32_t count) {
    if (count == 0) return 0;
    while (arr->count + count > arr->capacity) {
        if (_UnsafeArray_Grow(arr) != 0) return -1;
    }
    memcpy(arr->data + (size_t)arr->count * arr->element_size,
           data, (size_t)count * arr->element_size);
    arr->count += count;
    return 0;
}

static int UnsafeArray_ShrinkToFit(UnsafeArray *arr) {
    if (arr->count == 0) return 0;
    if (arr->count == arr->capacity) return 0;
    uint8_t *newbuf = (uint8_t *)realloc(arr->data, (size_t)arr->count * arr->element_size);
    if (!newbuf) return -1;
    arr->data = newbuf;
    arr->capacity = arr->count;
    return 0;
}

// Caller MUST ensure index < arr->count. No bounds check ("Unsafe" contract).
#define UnsafeArray_GetDeref(arr, index, type) (*(type *)UnsafeArray_Get(arr, index))

#define UnsafeArray_SetValue(arr, index, type, value) \
    UnsafeArray_Set(arr, index, &(type){value})

typedef void (*UnsafeArrayFormatter)(uint32_t index, const void *element, char *buf, uint32_t buf_size);

#define LINTNORE
// Prints via a formatter callback that writes each element into buf.
static void UnsafeArray_Print(UnsafeArray *arr, UnsafeArrayFormatter fmt) {
    char buf[256];
    printf("UnsafeArray[%u/%u] (elem %u bytes) {\n", arr->count, arr->capacity, arr->element_size);
    for (uint32_t i = 0; i < arr->count; i++) {
        fmt(i, UnsafeArray_Get(arr, i), buf, sizeof(buf));
        printf("  [%u] %s\n", i, buf);
    }
    printf("}\n");
}

// Printf-style variant -- type must match the format specifier.
//   UnsafeArray_PrintF(arr, int, "%d");
//   UnsafeArray_PrintF(arr, float, "%.2f");
#define UnsafeArray_PrintF(arr, type, fmt) do { \
    UnsafeArray *_up_a = (arr); \
    printf("UnsafeArray[%u/%u] (elem %u bytes) {\n", _up_a->count, _up_a->capacity, _up_a->element_size); \
    for (uint32_t _up_i = 0; _up_i < _up_a->count; _up_i++) { \
        printf("  [%u] " fmt "\n", _up_i, *(type *)UnsafeArray_Get(_up_a, _up_i)); \
    } \
    printf("}\n"); \
} while (0)
#undef LINTNORE

// Logs via LOG_INFO using a formatter callback.
static void UnsafeArray_Log(UnsafeArray *arr, UnsafeArrayFormatter fmt) {
    char buf[256];
    LOG_INFO("UnsafeArray[%u/%u] (elem %u bytes) {", arr->count, arr->capacity, arr->element_size);
    for (uint32_t i = 0; i < arr->count; i++) {
        fmt(i, UnsafeArray_Get(arr, i), buf, sizeof(buf));
        LOG_INFO("  [%u] %s", i, buf);
    }
    LOG_INFO("}");
}

// LOG_INFO printf-style variant -- type must match the format specifier.
//   UnsafeArray_LogF(arr, int, "%d");
//   UnsafeArray_LogF(arr, float, "%.2f");
#define UnsafeArray_LogF(arr, type, fmt) do { \
    UnsafeArray *_ul_a = (arr); \
    LOG_INFO("UnsafeArray[%u/%u] (elem %u bytes) {", _ul_a->count, _ul_a->capacity, _ul_a->element_size); \
    for (uint32_t _ul_i = 0; _ul_i < _ul_a->count; _ul_i++) { \
        LOG_INFO("  [%u] " fmt, _ul_i, *(type *)UnsafeArray_Get(_ul_a, _ul_i)); \
    } \
    LOG_INFO("}"); \
} while (0)

// ============================================================
// Shared formatter helper for PrintF/LogF macros in collection types.
// Inspects the conversion specifier to choose float vs integer dispatch,
// since void* values need to be reinterpreted before snprintf.
// ============================================================

static void _unsafe_fmt_snprintf(const void *v, char *b, uint32_t s, const char *fmt, uint32_t elem_size) {
    // Find the last conversion character in the format string
    const char *p = fmt;
    char conv = 'd';
    while (*p) {
        if (*p == '%' && *(p + 1) != '%') {
            const char *q = p + 1;
            while (*q == '-' || *q == '+' || *q == ' ' || *q == '0' || *q == '#') q++;
            while (*q >= '0' && *q <= '9') q++;
            if (*q == '.') { q++; while (*q >= '0' && *q <= '9') q++; }
            while (*q == 'h' || *q == 'l' || *q == 'L' || *q == 'j' || *q == 'z' || *q == 't') q++;
            if (*q) conv = *q;
        }
        p++;
    }
    if (conv == 'f' || conv == 'e' || conv == 'g' || conv == 'a' ||
        conv == 'F' || conv == 'E' || conv == 'G' || conv == 'A') {
        if (elem_size <= sizeof(float)) {
            float f = 0; memcpy(&f, v, sizeof(f));
            snprintf(b, (size_t)s, fmt, (double)f);
        } else {
            double d = 0; memcpy(&d, v, elem_size < sizeof(d) ? elem_size : sizeof(d));
            snprintf(b, (size_t)s, fmt, d);
        }
    } else {
        long long i = 0; memcpy(&i, v, elem_size < sizeof(i) ? elem_size : sizeof(i));
        if (elem_size <= sizeof(int)) snprintf(b, (size_t)s, fmt, (int)i);
        else                         snprintf(b, (size_t)s, fmt, i);
    }
}
