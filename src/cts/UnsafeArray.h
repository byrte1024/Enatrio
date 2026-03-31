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
    UnsafeArray *arr = (UnsafeArray *)malloc(sizeof(UnsafeArray));
    arr->element_size = element_size;
    arr->capacity = capacity;
    arr->count = 0;
    arr->data = (uint8_t *)malloc((size_t)capacity * element_size);
    return arr;
}

static void UnsafeArray_Destroy(UnsafeArray *arr) {
    free(arr->data);
    free(arr);
}

static void *UnsafeArray_Get(UnsafeArray *arr, uint32_t index) {
    return arr->data + (size_t)index * arr->element_size;
}

static void UnsafeArray_Set(UnsafeArray *arr, uint32_t index, const void *value) {
    memcpy(arr->data + (size_t)index * arr->element_size, value, arr->element_size);
}

static void UnsafeArray_Grow(UnsafeArray *arr) {
    arr->capacity *= 2;
    arr->data = (uint8_t *)realloc(arr->data, (size_t)arr->capacity * arr->element_size);
}

static void UnsafeArray_Add(UnsafeArray *arr, const void *value) {
    if (arr->count >= arr->capacity) UnsafeArray_Grow(arr);
    memcpy(arr->data + (size_t)arr->count * arr->element_size, value, arr->element_size);
    arr->count++;
}

// Removes element at index by swapping with the last element. O(1) but does not preserve order.
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

// Removes element at index by shifting subsequent elements down. O(n) but preserves order.
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

#define UnsafeArray_GetValue(arr, index, type) (*(type *)UnsafeArray_Get(arr, index))

#define UnsafeArray_SetValue(arr, index, type, value) do { \
    type _ua_tmp = (value); \
    UnsafeArray_Set(arr, index, &_ua_tmp); \
} while (0)

typedef void (*UnsafeArrayFormatter)(uint32_t index, const void *element, char *buf, uint32_t buf_size);

// Prints the array using a formatter callback that writes each element into buf.
static void UnsafeArray_Print(UnsafeArray *arr, UnsafeArrayFormatter fmt) {
    char buf[256];
    printf("UnsafeArray[%u/%u] (elem %u bytes) {\n", arr->count, arr->capacity, arr->element_size);
    for (uint32_t i = 0; i < arr->count; i++) {
        fmt(i, UnsafeArray_Get(arr, i), buf, sizeof(buf));
        printf("  [%u] %s\n", i, buf);
    }
    printf("}\n");
}

// Prints the array using a printf-style format and type.
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

// Logs the array using LOG_INFO for each line via a formatter callback.
static void UnsafeArray_Log(UnsafeArray *arr, UnsafeArrayFormatter fmt) {
    char buf[256];
    LOG_INFO("UnsafeArray[%u/%u] (elem %u bytes) {", arr->count, arr->capacity, arr->element_size);
    for (uint32_t i = 0; i < arr->count; i++) {
        fmt(i, UnsafeArray_Get(arr, i), buf, sizeof(buf));
        LOG_INFO("  [%u] %s", i, buf);
    }
    LOG_INFO("}");
}

// Logs the array using LOG_INFO with a printf-style format and type.
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
