#pragma once

#include "UnsafeArray.h"

typedef int  (*UnsafePredicate)(const void *element);
typedef int  (*UnsafeComparator)(const void *a, const void *b);
typedef void (*UnsafeAction)(void *element);
typedef void (*UnsafeTransform)(const void *src, void *dst);
typedef void (*UnsafeAccumulator)(void *accumulator, const void *element);

// ============================================================
//  Querying
// ============================================================

// Returns a new array containing only elements that match the predicate.
static UnsafeArray *UnsafeArray_Where(UnsafeArray *arr, UnsafePredicate predicate) {
    UnsafeArray *result = UnsafeArray_Create(arr->element_size, arr->count > 0 ? arr->count : 1);
    for (uint32_t i = 0; i < arr->count; i++) {
        void *elem = UnsafeArray_Get(arr, i);
        if (predicate(elem)) UnsafeArray_Add(result, elem);
    }
    return result;
}

// Returns a new array with each element transformed. dst_element_size is the size of the output type.
static UnsafeArray *UnsafeArray_Select(UnsafeArray *arr, UnsafeTransform transform, uint32_t dst_element_size) {
    UnsafeArray *result = UnsafeArray_Create(dst_element_size, arr->count > 0 ? arr->count : 1);
    uint8_t tmp[256];
    for (uint32_t i = 0; i < arr->count; i++) {
        transform(UnsafeArray_Get(arr, i), tmp);
        UnsafeArray_Add(result, tmp);
    }
    return result;
}

// Returns pointer to the first element matching the predicate, or NULL.
static void *UnsafeArray_First(UnsafeArray *arr, UnsafePredicate predicate) {
    for (uint32_t i = 0; i < arr->count; i++) {
        void *elem = UnsafeArray_Get(arr, i);
        if (predicate(elem)) return elem;
    }
    return NULL;
}

// Returns pointer to the last element matching the predicate, or NULL.
static void *UnsafeArray_Last(UnsafeArray *arr, UnsafePredicate predicate) {
    for (uint32_t i = arr->count; i > 0; i--) {
        void *elem = UnsafeArray_Get(arr, i - 1);
        if (predicate(elem)) return elem;
    }
    return NULL;
}

// Returns the index of the first element matching the predicate, or -1.
static int32_t UnsafeArray_IndexOf(UnsafeArray *arr, UnsafePredicate predicate) {
    for (uint32_t i = 0; i < arr->count; i++) {
        if (predicate(UnsafeArray_Get(arr, i))) return (int32_t)i;
    }
    return -1;
}

// Returns the index of the last element matching the predicate, or -1.
static int32_t UnsafeArray_LastIndexOf(UnsafeArray *arr, UnsafePredicate predicate) {
    for (uint32_t i = arr->count; i > 0; i--) {
        if (predicate(UnsafeArray_Get(arr, i - 1))) return (int32_t)(i - 1);
    }
    return -1;
}

// ============================================================
//  Predicates
// ============================================================

// Returns 1 if any element matches the predicate.
static int UnsafeArray_Any(UnsafeArray *arr, UnsafePredicate predicate) {
    for (uint32_t i = 0; i < arr->count; i++) {
        if (predicate(UnsafeArray_Get(arr, i))) return 1;
    }
    return 0;
}

// Returns 1 if all elements match the predicate.
static int UnsafeArray_All(UnsafeArray *arr, UnsafePredicate predicate) {
    for (uint32_t i = 0; i < arr->count; i++) {
        if (!predicate(UnsafeArray_Get(arr, i))) return 0;
    }
    return 1;
}

// Returns 1 if the array contains an element equal to value (memcmp).
static int UnsafeArray_Contains(UnsafeArray *arr, const void *value) {
    for (uint32_t i = 0; i < arr->count; i++) {
        if (memcmp(UnsafeArray_Get(arr, i), value, arr->element_size) == 0) return 1;
    }
    return 0;
}

// Returns the number of elements matching the predicate.
static uint32_t UnsafeArray_Count(UnsafeArray *arr, UnsafePredicate predicate) {
    uint32_t n = 0;
    for (uint32_t i = 0; i < arr->count; i++) {
        if (predicate(UnsafeArray_Get(arr, i))) n++;
    }
    return n;
}

// ============================================================
//  Aggregation
// ============================================================

// Folds all elements into an accumulator. Accumulator is initialized by the caller.
static void UnsafeArray_Aggregate(UnsafeArray *arr, void *accumulator, UnsafeAccumulator func) {
    for (uint32_t i = 0; i < arr->count; i++) {
        func(accumulator, UnsafeArray_Get(arr, i));
    }
}

// Returns pointer to the min element according to comparator, or NULL if empty.
static void *UnsafeArray_Min(UnsafeArray *arr, UnsafeComparator cmp) {
    if (arr->count == 0) return NULL;
    void *best = UnsafeArray_Get(arr, 0);
    for (uint32_t i = 1; i < arr->count; i++) {
        void *elem = UnsafeArray_Get(arr, i);
        if (cmp(elem, best) < 0) best = elem;
    }
    return best;
}

// Returns pointer to the max element according to comparator, or NULL if empty.
static void *UnsafeArray_Max(UnsafeArray *arr, UnsafeComparator cmp) {
    if (arr->count == 0) return NULL;
    void *best = UnsafeArray_Get(arr, 0);
    for (uint32_t i = 1; i < arr->count; i++) {
        void *elem = UnsafeArray_Get(arr, i);
        if (cmp(elem, best) > 0) best = elem;
    }
    return best;
}

// ============================================================
//  Mutation
// ============================================================

// Executes an action on every element.
static void UnsafeArray_ForEach(UnsafeArray *arr, UnsafeAction action) {
    for (uint32_t i = 0; i < arr->count; i++) {
        action(UnsafeArray_Get(arr, i));
    }
}

// Removes all elements matching the predicate (order-preserving). Returns number removed.
static uint32_t UnsafeArray_RemoveAll(UnsafeArray *arr, UnsafePredicate predicate) {
    uint32_t write = 0;
    for (uint32_t read = 0; read < arr->count; read++) {
        if (!predicate(UnsafeArray_Get(arr, read))) {
            if (write != read) {
                memcpy(UnsafeArray_Get(arr, write), UnsafeArray_Get(arr, read), arr->element_size);
            }
            write++;
        }
    }
    uint32_t removed = arr->count - write;
    arr->count = write;
    return removed;
}

// Sorts the array in place using the given comparator.
static void UnsafeArray_OrderBy(UnsafeArray *arr, UnsafeComparator cmp) {
    if (arr->count < 2) return;
    qsort(arr->data, arr->count, arr->element_size, cmp);
}

// Reverses the array in place.
static void UnsafeArray_Reverse(UnsafeArray *arr) {
    if (arr->count < 2) return;
    uint8_t tmp[256];
    for (uint32_t i = 0, j = arr->count - 1; i < j; i++, j--) {
        memcpy(tmp, UnsafeArray_Get(arr, i), arr->element_size);
        memcpy(UnsafeArray_Get(arr, i), UnsafeArray_Get(arr, j), arr->element_size);
        memcpy(UnsafeArray_Get(arr, j), tmp, arr->element_size);
    }
}

// Shuffles the array in place (Fisher-Yates). Call srand() beforehand for varied results.
static void UnsafeArray_Shuffle(UnsafeArray *arr) {
    if (arr->count < 2) return;
    uint8_t tmp[256];
    for (uint32_t i = arr->count - 1; i > 0; i--) {
        uint32_t j = (uint32_t)(rand() % (int)(i + 1));
        if (i != j) {
            memcpy(tmp, UnsafeArray_Get(arr, i), arr->element_size);
            memcpy(UnsafeArray_Get(arr, i), UnsafeArray_Get(arr, j), arr->element_size);
            memcpy(UnsafeArray_Get(arr, j), tmp, arr->element_size);
        }
    }
}

// Removes duplicate elements (memcmp). Order-preserving. Returns number removed.
static uint32_t UnsafeArray_Distinct(UnsafeArray *arr) {
    if (arr->count < 2) return 0;
    uint32_t write = 1;
    for (uint32_t read = 1; read < arr->count; read++) {
        int dup = 0;
        for (uint32_t k = 0; k < write; k++) {
            if (memcmp(UnsafeArray_Get(arr, read), UnsafeArray_Get(arr, k), arr->element_size) == 0) {
                dup = 1;
                break;
            }
        }
        if (!dup) {
            if (write != read) {
                memcpy(UnsafeArray_Get(arr, write), UnsafeArray_Get(arr, read), arr->element_size);
            }
            write++;
        }
    }
    uint32_t removed = arr->count - write;
    arr->count = write;
    return removed;
}

// ============================================================
//  Subsequence
// ============================================================

// Returns a new array skipping the first n elements.
static UnsafeArray *UnsafeArray_Skip(UnsafeArray *arr, uint32_t n) {
    UnsafeArray *result = UnsafeArray_Create(arr->element_size, arr->count > n ? arr->count - n : 1);
    for (uint32_t i = n; i < arr->count; i++) {
        UnsafeArray_Add(result, UnsafeArray_Get(arr, i));
    }
    return result;
}

// Returns a new array with only the first n elements.
static UnsafeArray *UnsafeArray_Take(UnsafeArray *arr, uint32_t n) {
    uint32_t take = n < arr->count ? n : arr->count;
    UnsafeArray *result = UnsafeArray_Create(arr->element_size, take > 0 ? take : 1);
    for (uint32_t i = 0; i < take; i++) {
        UnsafeArray_Add(result, UnsafeArray_Get(arr, i));
    }
    return result;
}

// Skips elements while predicate is true, returns the rest as a new array.
static UnsafeArray *UnsafeArray_SkipWhile(UnsafeArray *arr, UnsafePredicate predicate) {
    uint32_t start = 0;
    while (start < arr->count && predicate(UnsafeArray_Get(arr, start))) start++;
    UnsafeArray *result = UnsafeArray_Create(arr->element_size, arr->count - start > 0 ? arr->count - start : 1);
    for (uint32_t i = start; i < arr->count; i++) {
        UnsafeArray_Add(result, UnsafeArray_Get(arr, i));
    }
    return result;
}

// Takes elements while predicate is true, returns them as a new array.
static UnsafeArray *UnsafeArray_TakeWhile(UnsafeArray *arr, UnsafePredicate predicate) {
    UnsafeArray *result = UnsafeArray_Create(arr->element_size, arr->count > 0 ? arr->count : 1);
    for (uint32_t i = 0; i < arr->count; i++) {
        if (!predicate(UnsafeArray_Get(arr, i))) break;
        UnsafeArray_Add(result, UnsafeArray_Get(arr, i));
    }
    return result;
}

// Returns a new array that is the concatenation of two arrays (must have same element_size).
static UnsafeArray *UnsafeArray_Concat(UnsafeArray *a, UnsafeArray *b) {
    UnsafeArray *result = UnsafeArray_Create(a->element_size, a->count + b->count);
    for (uint32_t i = 0; i < a->count; i++) UnsafeArray_Add(result, UnsafeArray_Get(a, i));
    for (uint32_t i = 0; i < b->count; i++) UnsafeArray_Add(result, UnsafeArray_Get(b, i));
    return result;
}

// ============================================================
//  LINQ Macros — inline expressions, no function pointers needed
//
//  Usage pattern:
//    LINQ_WHERE(arr, int, x, x > 5 && x % 2 == 0)
//    LINQ_FIRST(arr, int, x, x == 42)
//    LINQ_SUM(arr, int)
//    LINQ_FOREACH(arr, int, x, { printf("%d\n", x); })
// ============================================================

// Returns a new UnsafeArray* containing elements where expr is true.
//   UnsafeArray *evens = LINQ_WHERE(arr, int, x, x % 2 == 0);
#define LINQ_WHERE(arr, type, var, expr) ({ \
    UnsafeArray *_lw_src = (arr); \
    UnsafeArray *_lw_dst = UnsafeArray_Create(sizeof(type), _lw_src->count > 0 ? _lw_src->count : 1); \
    for (uint32_t _lw_i = 0; _lw_i < _lw_src->count; _lw_i++) { \
        type var = *(type *)UnsafeArray_Get(_lw_src, _lw_i); \
        if (expr) UnsafeArray_Add(_lw_dst, &var); \
    } \
    _lw_dst; \
})

// Returns a pointer to the first matching element, or NULL.
//   int *found = LINQ_FIRST(arr, int, x, x > 10);
#define LINQ_FIRST(arr, type, var, expr) ({ \
    UnsafeArray *_lf_src = (arr); \
    type *_lf_result = NULL; \
    for (uint32_t _lf_i = 0; _lf_i < _lf_src->count; _lf_i++) { \
        type *_lf_ptr = (type *)UnsafeArray_Get(_lf_src, _lf_i); \
        type var = *_lf_ptr; \
        if (expr) { _lf_result = _lf_ptr; break; } \
    } \
    _lf_result; \
})

// Returns a pointer to the last matching element, or NULL.
//   int *found = LINQ_LAST(arr, int, x, x < 0);
#define LINQ_LAST(arr, type, var, expr) ({ \
    UnsafeArray *_ll_src = (arr); \
    type *_ll_result = NULL; \
    for (uint32_t _ll_i = _ll_src->count; _ll_i > 0; _ll_i--) { \
        type *_ll_ptr = (type *)UnsafeArray_Get(_ll_src, _ll_i - 1); \
        type var = *_ll_ptr; \
        if (expr) { _ll_result = _ll_ptr; break; } \
    } \
    _ll_result; \
})

// Returns the index of the first match, or -1.
//   int32_t idx = LINQ_INDEXOF(arr, int, x, x == 42);
#define LINQ_INDEXOF(arr, type, var, expr) ({ \
    UnsafeArray *_li_src = (arr); \
    int32_t _li_result = -1; \
    for (uint32_t _li_i = 0; _li_i < _li_src->count; _li_i++) { \
        type var = *(type *)UnsafeArray_Get(_li_src, _li_i); \
        if (expr) { _li_result = (int32_t)_li_i; break; } \
    } \
    _li_result; \
})

// Returns 1 if any element matches, 0 otherwise.
//   if (LINQ_ANY(arr, int, x, x < 0)) { ... }
#define LINQ_ANY(arr, type, var, expr) ({ \
    UnsafeArray *_la_src = (arr); \
    int _la_result = 0; \
    for (uint32_t _la_i = 0; _la_i < _la_src->count; _la_i++) { \
        type var = *(type *)UnsafeArray_Get(_la_src, _la_i); \
        if (expr) { _la_result = 1; break; } \
    } \
    _la_result; \
})

// Returns 1 if all elements match, 0 otherwise.
//   if (LINQ_ALL(arr, int, x, x > 0)) { ... }
#define LINQ_ALL(arr, type, var, expr) ({ \
    UnsafeArray *_lal_src = (arr); \
    int _lal_result = 1; \
    for (uint32_t _lal_i = 0; _lal_i < _lal_src->count; _lal_i++) { \
        type var = *(type *)UnsafeArray_Get(_lal_src, _lal_i); \
        if (!(expr)) { _lal_result = 0; break; } \
    } \
    _lal_result; \
})

// Returns count of elements matching the expression.
//   uint32_t n = LINQ_COUNT(arr, int, x, x % 3 == 0);
#define LINQ_COUNT(arr, type, var, expr) ({ \
    UnsafeArray *_lc_src = (arr); \
    uint32_t _lc_n = 0; \
    for (uint32_t _lc_i = 0; _lc_i < _lc_src->count; _lc_i++) { \
        type var = *(type *)UnsafeArray_Get(_lc_src, _lc_i); \
        if (expr) _lc_n++; \
    } \
    _lc_n; \
})

// Returns a new UnsafeArray* with each element transformed by expr.
//   UnsafeArray *doubled = LINQ_SELECT(arr, int, int, x, x * 2);
#define LINQ_SELECT(arr, src_type, dst_type, var, expr) ({ \
    UnsafeArray *_ls_src = (arr); \
    UnsafeArray *_ls_dst = UnsafeArray_Create(sizeof(dst_type), _ls_src->count > 0 ? _ls_src->count : 1); \
    for (uint32_t _ls_i = 0; _ls_i < _ls_src->count; _ls_i++) { \
        src_type var = *(src_type *)UnsafeArray_Get(_ls_src, _ls_i); \
        dst_type _ls_val = (dst_type)(expr); \
        UnsafeArray_Add(_ls_dst, &_ls_val); \
    } \
    _ls_dst; \
})

// Executes a statement block for each element.
//   LINQ_FOREACH(arr, int, x, { printf("%d\n", x); });
#define LINQ_FOREACH(arr, type, var, body) do { \
    UnsafeArray *_le_src = (arr); \
    for (uint32_t _le_i = 0; _le_i < _le_src->count; _le_i++) { \
        type var = *(type *)UnsafeArray_Get(_le_src, _le_i); \
        body \
    } \
} while (0)

// Executes a statement block for each element with mutable pointer access.
//   LINQ_FOREACH_REF(arr, int, p, { *p *= 2; });
#define LINQ_FOREACH_REF(arr, type, var, body) do { \
    UnsafeArray *_ler_src = (arr); \
    for (uint32_t _ler_i = 0; _ler_i < _ler_src->count; _ler_i++) { \
        type *var = (type *)UnsafeArray_Get(_ler_src, _ler_i); \
        body \
    } \
} while (0)

// Removes all elements where expr is true (in-place). Returns number removed.
//   uint32_t n = LINQ_REMOVEALL(arr, int, x, x < 0);
#define LINQ_REMOVEALL(arr, type, var, expr) ({ \
    UnsafeArray *_lr_src = (arr); \
    uint32_t _lr_w = 0; \
    for (uint32_t _lr_r = 0; _lr_r < _lr_src->count; _lr_r++) { \
        type var = *(type *)UnsafeArray_Get(_lr_src, _lr_r); \
        if (!(expr)) { \
            if (_lr_w != _lr_r) memcpy(UnsafeArray_Get(_lr_src, _lr_w), UnsafeArray_Get(_lr_src, _lr_r), sizeof(type)); \
            _lr_w++; \
        } \
    } \
    uint32_t _lr_removed = _lr_src->count - _lr_w; \
    _lr_src->count = _lr_w; \
    _lr_removed; \
})

// Folds all elements into a single value.
//   int sum = LINQ_AGGREGATE(arr, int, int, acc, x, 0, acc + x);
#define LINQ_AGGREGATE(arr, type, acc_type, acc, var, seed, expr) ({ \
    UnsafeArray *_lag_src = (arr); \
    acc_type acc = (seed); \
    for (uint32_t _lag_i = 0; _lag_i < _lag_src->count; _lag_i++) { \
        type var = *(type *)UnsafeArray_Get(_lag_src, _lag_i); \
        acc = (expr); \
    } \
    acc; \
})

// Sum all elements. Type must support +.
//   int total = LINQ_SUM(arr, int);
#define LINQ_SUM(arr, type) LINQ_AGGREGATE(arr, type, type, _s_acc, _s_val, (type)0, _s_acc + _s_val)

// Product of all elements. Type must support *.
//   int product = LINQ_PRODUCT(arr, int);
#define LINQ_PRODUCT(arr, type) LINQ_AGGREGATE(arr, type, type, _p_acc, _p_val, (type)1, _p_acc * _p_val)

// Min value via expression. Returns the type directly (not a pointer). Array must not be empty.
//   int lo = LINQ_MIN(arr, int);
#define LINQ_MIN(arr, type) ({ \
    UnsafeArray *_lmn_src = (arr); \
    type _lmn_best = *(type *)UnsafeArray_Get(_lmn_src, 0); \
    for (uint32_t _lmn_i = 1; _lmn_i < _lmn_src->count; _lmn_i++) { \
        type _lmn_cur = *(type *)UnsafeArray_Get(_lmn_src, _lmn_i); \
        if (_lmn_cur < _lmn_best) _lmn_best = _lmn_cur; \
    } \
    _lmn_best; \
})

// Max value via expression. Returns the type directly (not a pointer). Array must not be empty.
//   int hi = LINQ_MAX(arr, int);
#define LINQ_MAX(arr, type) ({ \
    UnsafeArray *_lmx_src = (arr); \
    type _lmx_best = *(type *)UnsafeArray_Get(_lmx_src, 0); \
    for (uint32_t _lmx_i = 1; _lmx_i < _lmx_src->count; _lmx_i++) { \
        type _lmx_cur = *(type *)UnsafeArray_Get(_lmx_src, _lmx_i); \
        if (_lmx_cur > _lmx_best) _lmx_best = _lmx_cur; \
    } \
    _lmx_best; \
})
