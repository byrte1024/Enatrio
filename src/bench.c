#ifdef INBENCH
#define LINTNORE

#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define VERBOSE

#include <raylib.h>

#include "system/utils.h"
#include "system/object/Self.h"

#include "classes/window.h"

// ============================================================
// Benchmark-only classes
// ============================================================

// BenchGameObject: minimal GameObject subclass for spread benchmarks.
#define TYPE BenchGameObject
BEGIN_CLASS(0xB020);
INHERITS(GameObject);
SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Create)
    CALL_BASE();
MESSAGE_HANDLER_END()
SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Destroy)
    CALL_BASE();
MESSAGE_HANDLER_END()
SELF_MESSAGE_HANDLER_BEGIN_EXTERN(GameObject, Update)
    (void)Self;
MESSAGE_HANDLER_END()
SELF_MESSAGE_HANDLER_BEGIN_EXTERN(GameObject, Render)
    (void)Self;
MESSAGE_HANDLER_END()
CAN_RECEIVE_BEGIN()
    SELF_CAN_RECEIVE_MID_EXTERN(Object, Create)
    SELF_CAN_RECEIVE_MID_EXTERN(Object, Destroy)
    SELF_CAN_RECEIVE_MID_EXTERN(GameObject, Update)
    SELF_CAN_RECEIVE_MID_EXTERN(GameObject, Render)
CAN_RECEIVE_END()
RECEIVE_MESSAGE_BEGIN()
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Object, Create)
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Object, Destroy)
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(GameObject, Update)
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(GameObject, Render)
RECEIVE_MESSAGE_END()
CLASSDEF_INHERITS(GameObject)
#undef TYPE

// BenchStandalone: minimal class with no parent, for chain-walk tests.
#define TYPE BenchStandalone
BEGIN_CLASS(0xB021);
DECLARE_MID(Ping, 0x01);
MESSAGE_HANDLER_BEGIN(Ping)
MESSAGE_HANDLER_END()
CAN_RECEIVE_BEGIN()
    CAN_RECEIVE_MID(Ping)
CAN_RECEIVE_END()
RECEIVE_MESSAGE_BEGIN()
    RECEIVE_MESSAGE_ROUTE(Ping)
RECEIVE_MESSAGE_END()
CLASSDEF()
#undef TYPE

// BenchNoOp: minimal class, 1 MID, no-op handler.
#define TYPE BenchNoOp
BEGIN_CLASS(0xB001);
DECLARE_MID(Ping, 0x01);
MESSAGE_HANDLER_BEGIN(Ping)
MESSAGE_HANDLER_END()
CAN_RECEIVE_BEGIN()
    CAN_RECEIVE_MID(Ping)
CAN_RECEIVE_END()
RECEIVE_MESSAGE_BEGIN()
    RECEIVE_MESSAGE_ROUTE(Ping)
RECEIVE_MESSAGE_END()
CLASSDEF()
#undef TYPE

// BenchSelfNoOp: inherits Object, 1 SELF MID, no-op handler.
#define TYPE BenchSelfNoOp
BEGIN_CLASS(0xB003);
INHERITS(Object);
DECLARE_SELF_MID(Tick, 0x01);
SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Create)
    CALL_BASE();
MESSAGE_HANDLER_END()
SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Destroy)
    CALL_BASE();
MESSAGE_HANDLER_END()
SELF_MESSAGE_HANDLER_BEGIN(Tick)
    (void)Self;
MESSAGE_HANDLER_END()
CAN_RECEIVE_BEGIN()
    SELF_CAN_RECEIVE_MID_EXTERN(Object, Create)
    SELF_CAN_RECEIVE_MID_EXTERN(Object, Destroy)
    SELF_CAN_RECEIVE_MID(Tick)
CAN_RECEIVE_END()
RECEIVE_MESSAGE_BEGIN()
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Object, Create)
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Object, Destroy)
    SELF_RECEIVE_MESSAGE_ROUTE(Tick)
RECEIVE_MESSAGE_END()
CLASSDEF_INHERITS(Object)
#undef TYPE

// Generated extreme classes: 32, 100, 1000 MIDs each
#include "bench_classes.h"

// Generated parameter-heavy classes: 1, 5, 10, 100, 1000 params per handler
#include "bench_param_classes.h"

// ============================================================
// Benchmark harness
// ============================================================

static double _now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

static int _bench_count = 0;
static int _assert_count = 0;
static int _assert_fail = 0;

// Result storage for post-hoc assertions
#define BENCH_MAX_RESULTS 128
static struct { const char *name; double ns_per_op; } _bench_results[BENCH_MAX_RESULTS];
static int _bench_result_count = 0;

static double _bench_lookup(const char *name) {
    for (int i = 0; i < _bench_result_count; i++) {
        if (strcmp(_bench_results[i].name, name) == 0)
            return _bench_results[i].ns_per_op;
    }
    return -1.0;
}

#define BENCH_ASSERT(desc, cond) do { \
    _assert_count++; \
    if (!(cond)) { \
        printf("  FAIL: %s\n", desc); \
        _assert_fail++; \
    } else { \
        printf("  PASS: %s\n", desc); \
    } \
} while (0)

static void _pb(const char *name, int ops, double ns) {
    double us = ns / 1000.0;
    double ms = ns / 1e6;
    double ns_per_op = ns / ops;
    double ops_1ms  = (ns_per_op > 0.01) ? 1e6 / ns_per_op : 999999999.0;
    double ops_8ms  = (ns_per_op > 0.01) ? 8.33e6 / ns_per_op : 999999999.0;
    double ops_16ms = (ns_per_op > 0.01) ? 16.67e6 / ns_per_op : 999999999.0;
    char b1[16], b8[16], b16[16];
    if (ops_1ms > 1e6) snprintf(b1,sizeof(b1),"  inf"); else snprintf(b1,sizeof(b1),"%6.0fk",ops_1ms/1000);
    if (ops_8ms > 1e6) snprintf(b8,sizeof(b8),"  inf"); else snprintf(b8,sizeof(b8),"%6.0fk",ops_8ms/1000);
    if (ops_16ms > 1e6) snprintf(b16,sizeof(b16),"  inf"); else snprintf(b16,sizeof(b16),"%6.0fk",ops_16ms/1000);
    printf("  %-50s %7d ops  %8.1f us  %7.1f ns/op  | %7s  %7s  %7s\n",
           name, ops, us, ns_per_op, b1, b8, b16);
    if (_bench_result_count < BENCH_MAX_RESULTS) {
        _bench_results[_bench_result_count].name = name;
        _bench_results[_bench_result_count].ns_per_op = ns_per_op;
        _bench_result_count++;
    }
    _bench_count++;
}

static void _header(const char *section) {
    printf("\n=== %s ===\n", section);
    printf("  %-50s %7s      %8s  %10s  | %8s  %8s  %8s\n",
           "", "ops", "total", "per-op", "1ms", "8.33ms", "16.67ms");
    printf("  %-50s %7s      %8s  %10s  | %8s  %8s  %8s\n",
           "", "", "", "", "budget", "budget", "budget");
    printf("  %s\n",
    "------------------------------------------------------"
    "------------------------------------------------------");
}

// ============================================================
// CTS: UnsafeArray
// ============================================================

static void bench_array(void) {
    _header("UnsafeArray");
    int N = 10000;
    double a, b;

    { UnsafeArray *arr = UnsafeArray_Create(sizeof(int), 8);
      a=_now_ns(); for(int i=0;i<N;i++) UnsafeArray_Add(arr,&i); b=_now_ns();
      _pb("Add (sequential)", N, b-a); UnsafeArray_Destroy(arr); }

    { int bulk[10000]; for(int i=0;i<N;i++) bulk[i]=i;
      UnsafeArray *arr = UnsafeArray_Create(sizeof(int), 8);
      a=_now_ns(); UnsafeArray_AddBulk(arr,bulk,(uint32_t)N); b=_now_ns();
      _pb("AddBulk (10000 at once)", N, b-a); UnsafeArray_Destroy(arr); }

    { UnsafeArray *arr = UnsafeArray_Create(sizeof(int), (uint32_t)N);
      for(int i=0;i<N;i++) UnsafeArray_Add(arr,&i); volatile void *p;
      a=_now_ns(); for(int i=0;i<N;i++) p=UnsafeArray_Get(arr,(uint32_t)i); b=_now_ns();
      (void)p; _pb("Get (bounds-checked)", N, b-a); UnsafeArray_Destroy(arr); }

    { UnsafeArray *arr = UnsafeArray_Create(sizeof(int), (uint32_t)N);
      for(int i=0;i<N;i++) UnsafeArray_Add(arr,&i); volatile void *p;
      a=_now_ns(); for(int i=0;i<N;i++) p=UnsafeArray_GetFast(arr,(uint32_t)i); b=_now_ns();
      (void)p; _pb("GetFast (unchecked)", N, b-a); UnsafeArray_Destroy(arr); }

    { UnsafeArray *arr = UnsafeArray_Create(sizeof(int), 100);
      for(int i=0;i<100;i++) UnsafeArray_Add(arr,&i);
      a=_now_ns(); for(int i=0;i<N;i++){UnsafeArray_RemoveSwap(arr,0);UnsafeArray_Add(arr,&i);} b=_now_ns();
      _pb("RemoveSwap + Add cycle", N, b-a); UnsafeArray_Destroy(arr); }

    { UnsafeArray *arr = UnsafeArray_Create(sizeof(int), (uint32_t)N);
      for(int i=0;i<N;i++) UnsafeArray_Add(arr,&i);
      a=_now_ns(); for(int i=0;i<N;i++) UnsafeArray_Set(arr,(uint32_t)i,&i); b=_now_ns();
      _pb("Set (overwrite by index)", N, b-a); UnsafeArray_Destroy(arr); }
}

// ============================================================
// CTS: UnsafeHashMap (fixed-size)
// ============================================================

static void _bench_fe_noop(const void *k, uint32_t kl, void *v) {
    (void)k; (void)kl; (void)v;
}

static void bench_hashmap(void) {
    _header("UnsafeHashMap (fixed-size)");
    int N = 5000;
    int NB = 100000;
    double a, b;
    char key[16];

    { UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
      a=_now_ns(); for(int i=0;i<N;i++){int l=snprintf(key,sizeof(key),"k%d",i);UnsafeHashMap_Set(map,key,(uint32_t)l,&i);} b=_now_ns();
      _pb("Set (unique keys)", N, b-a); UnsafeHashMap_Destroy(map); }

    { UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int),(uint32_t)N);
      for(int i=0;i<N;i++){int l=snprintf(key,sizeof(key),"k%d",i);UnsafeHashMap_Set(map,key,(uint32_t)l,&i);}
      volatile void *p; a=_now_ns();
      for(int i=0;i<N;i++){int l=snprintf(key,sizeof(key),"k%d",i);p=UnsafeHashMap_Get(map,key,(uint32_t)l);} b=_now_ns();
      (void)p; _pb("Get (existing, incl. snprintf)", N, b-a); UnsafeHashMap_Destroy(map); }

    { UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
      int v=0; UnsafeHashMap_Set(map,"k",1,&v);
      a=_now_ns(); for(int i=0;i<NB;i++) UnsafeHashMap_Upsert(map,"k",1,&i); b=_now_ns();
      _pb("Upsert (same key, in-place)", NB, b-a); UnsafeHashMap_Destroy(map); }

    { UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
      int v=0; UnsafeHashMap_Set(map,"k",1,&v);
      a=_now_ns(); for(int i=0;i<NB;i++){UnsafeHashMap_Remove(map,"k",1);UnsafeHashMap_Set(map,"k",1,&i);} b=_now_ns();
      _pb("Remove+Set cycle (same key)", NB, b-a); UnsafeHashMap_Destroy(map); }

    { UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int), 8);
      int v=0; UnsafeHashMap_Set(map,"k",1,&v);
      a=_now_ns(); for(int i=0;i<NB;i++){volatile int r=UnsafeHashMap_Has(map,"k",1);(void)r;} b=_now_ns();
      _pb("Has (existing key)", NB, b-a); UnsafeHashMap_Destroy(map); }

    { UnsafeHashMap *map = UnsafeHashMap_Create(sizeof(int),(uint32_t)N);
      for(int i=0;i<N;i++){int l=snprintf(key,sizeof(key),"k%d",i);UnsafeHashMap_Set(map,key,(uint32_t)l,&i);}
      a=_now_ns();
      for(int iter=0;iter<100;iter++){
          UnsafeHashMap_ForEach(map, _bench_fe_noop);
      } b=_now_ns();
      _pb("ForEach (5000 entries, x100 iters)", 100, b-a); UnsafeHashMap_Destroy(map); }
}

// ============================================================
// CTS: UnsafeVariedHashMap
// ============================================================

static void bench_varied_hashmap(void) {
    _header("UnsafeVariedHashMap");
    int N = 5000;
    int NB = 100000;
    double a, b;
    char key[16];

    { UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);
      a=_now_ns(); for(int i=0;i<N;i++){int l=snprintf(key,sizeof(key),"k%d",i);UnsafeVariedHashMap_Set(map,key,(uint32_t)l,&i,sizeof(int));} b=_now_ns();
      _pb("Set (unique keys, int)", N, b-a); UnsafeVariedHashMap_Destroy(map); }

    { UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);
      int v=0; UnsafeVariedHashMap_Set(map,"k",1,&v,sizeof(int));
      a=_now_ns(); for(int i=0;i<NB;i++) UnsafeVariedHashMap_Upsert(map,"k",1,&i,sizeof(int)); b=_now_ns();
      _pb("Upsert same-size (int, in-place)", NB, b-a); UnsafeVariedHashMap_Destroy(map); }

    { UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);
      double d=1.0; UnsafeVariedHashMap_Set(map,"v",1,&d,sizeof(double));
      a=_now_ns(); for(int i=0;i<NB;i++){int sm=i;double bg=(double)i;UnsafeVariedHashMap_Upsert(map,"v",1,&sm,sizeof(int));UnsafeVariedHashMap_Upsert(map,"v",1,&bg,sizeof(double));} b=_now_ns();
      _pb("Upsert oscillating 4b/8b (pairs)", NB, b-a);
      printf("    data->count: %u -- %s\n", map->data->count, map->data->count==8?"ZERO GROWTH":"GREW");
      UnsafeVariedHashMap_Destroy(map); }

    { UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);
      int v=0; UnsafeVariedHashMap_Set(map,"k",1,&v,sizeof(int)); uint32_t bl=map->data->count;
      a=_now_ns(); for(int i=0;i<NB;i++){UnsafeVariedHashMap_Remove(map,"k",1);UnsafeVariedHashMap_Set(map,"k",1,&i,sizeof(int));} b=_now_ns();
      _pb("Remove+Set cycle (same key, int)", NB, b-a);
      printf("    data->count: %u (baseline %u) -- %s\n", map->data->count, bl, map->data->count==bl?"ZERO GROWTH":"GREW");
      UnsafeVariedHashMap_Destroy(map); }

    { UnsafeVariedHashMap *map = UnsafeVariedHashMap_Create(8);
      char bigval[256]; memset(bigval,'X',sizeof(bigval));
      UnsafeVariedHashMap_Set(map,"big",3,bigval,256);
      a=_now_ns(); for(int i=0;i<NB;i++) UnsafeVariedHashMap_Upsert(map,"big",3,bigval,256); b=_now_ns();
      _pb("Upsert same-size (256 bytes, in-place)", NB, b-a);
      UnsafeVariedHashMap_Destroy(map); }
}

// ============================================================
// CTS: UnsafeDictionary (trie)
// ============================================================

static void bench_dictionary(void) {
    _header("UnsafeDictionary (trie)");
    int N = 5000;
    int NB = 100000;
    double a, b;
    char key[16];

    { UnsafeDictionary *dict = UnsafeDictionary_Create(sizeof(int), 8);
      a=_now_ns(); for(int i=0;i<N;i++){int l=snprintf(key,sizeof(key),"k%d",i);UnsafeDictionary_Set(dict,key,(uint32_t)l,&i);} b=_now_ns();
      _pb("Set (unique keys)", N, b-a); UnsafeDictionary_Destroy(dict); }

    { UnsafeDictionary *dict = UnsafeDictionary_Create(sizeof(int),(uint32_t)N);
      for(int i=0;i<N;i++){int l=snprintf(key,sizeof(key),"k%d",i);UnsafeDictionary_Set(dict,key,(uint32_t)l,&i);}
      volatile void *p; a=_now_ns();
      for(int i=0;i<N;i++){int l=snprintf(key,sizeof(key),"k%d",i);p=UnsafeDictionary_Get(dict,key,(uint32_t)l);} b=_now_ns();
      (void)p; _pb("Get (existing, incl. snprintf)", N, b-a); UnsafeDictionary_Destroy(dict); }

    { UnsafeDictionary *dict = UnsafeDictionary_Create(sizeof(int), 8);
      int v=0; UnsafeDictionary_Set(dict,"k",1,&v);
      a=_now_ns(); for(int i=0;i<NB;i++) UnsafeDictionary_Upsert(dict,"k",1,&i); b=_now_ns();
      _pb("Upsert (same key, in-place)", NB, b-a); UnsafeDictionary_Destroy(dict); }
}

// ============================================================
// Scaling
// ============================================================

static void bench_scaling(void) {
    _header("Scaling: HashMap Get ns/op vs N entries");
    double a, b;
    char key[16];
    int sizes[] = {100, 500, 1000, 5000, 10000, 50000};
    for(int s=0;s<6;s++){
        int sz=sizes[s];
        UnsafeHashMap *map=UnsafeHashMap_Create(sizeof(int),(uint32_t)sz);
        for(int i=0;i<sz;i++){int l=snprintf(key,sizeof(key),"k%d",i);UnsafeHashMap_Set(map,key,(uint32_t)l,&i);}
        volatile void *p; a=_now_ns();
        for(int i=0;i<sz;i++){int l=snprintf(key,sizeof(key),"k%d",i);p=UnsafeHashMap_Get(map,key,(uint32_t)l);} b=_now_ns();
        (void)p; printf("  N=%6d  %8.1f ns/op\n", sz, (b-a)/sz);
        UnsafeHashMap_Destroy(map);
    }
}

// ============================================================
// Object System: Class dispatch
// ============================================================

static void _register_bench_classes(void) {
    BeginClassRegistrations();
    RegisterClass(Object_ClassDef());
    RegisterClass(GameObject_ClassDef());
    RegisterClass(BenchGameObject_ClassDef());
    RegisterClass(BenchStandalone_ClassDef());
    RegisterClass(Window_ClassDef());
    RegisterClass(BenchNoOp_ClassDef());
    RegisterClass(BenchSelfNoOp_ClassDef());
    RegisterClass(BenchN32_ClassDef());
    RegisterClass(BenchN100_ClassDef());
    RegisterClass(BenchN1000_ClassDef());
    RegisterClass(BenchParam1_ClassDef());
    RegisterClass(BenchParam5_ClassDef());
    RegisterClass(BenchParam10_ClassDef());
    RegisterClass(BenchParam100_ClassDef());
    RegisterClass(BenchParam1000_ClassDef());
    EndClassRegistrations();
}

static void bench_dispatch(void) {
    _header("Dispatch: Core (stable, no-op handlers)");
    int NB = 200000;
    double a, b;

    // Warmup: prime pool + caches
    for (int i = 0; i < 5000; i++) {
        MessagePayload p = PreparePayload(CID_BenchNoOp, MID_BenchNoOp_Ping);
        DispatchMessage(&p);
        FreePayload(&p);
    }

    // Pure payload alloc/free cycle (pool)
    { a=_now_ns();
      for(int i=0;i<NB;i++){
          MessagePayload p = PreparePayload(CID_BenchNoOp, MID_BenchNoOp_Ping);
          FreePayload(&p);
      } b=_now_ns();
      _pb("PreparePayload + FreePayload (pool)", NB, b-a); }

    // Minimal dispatch: 1-MID class, no-op handler
    { a=_now_ns();
      for(int i=0;i<NB;i++){
          MessagePayload p = PreparePayload(CID_BenchNoOp, MID_BenchNoOp_Ping);
          DispatchMessage(&p);
          FreePayload(&p);
      } b=_now_ns();
      _pb("Dispatch (1 MID, no-op handler)", NB, b-a); }

    // SelfDispatch: 1 MID on Self object, no-op
    { ExternalReference obj = Object_CreateRef(CID_BenchSelfNoOp);
      TempObjectReference t = ObjectContainer_TempFrom(obj);
      a=_now_ns();
      for(int i=0;i<NB;i++){
          SELF_DISPATCH(t, MID_BenchSelfNoOp_SELF_Tick, {}, {});
      } b=_now_ns();
      _pb("SELF_DISPATCH (1 SELF MID, no-op)", NB, b-a);
      ObjectContainer_UnRef_External(&obj); }

    // Payload Set + Get (shared payload, no dispatch)
    { MessagePayload p = PreparePayload(CID_BenchNoOp, MID_BenchNoOp_Ping);
      a=_now_ns();
      for(int i=0;i<NB;i++) Payload_SetValue(&p, "v", int, i);
      b=_now_ns();
      _pb("Payload_SetValue (upsert, same key)", NB, b-a);
      volatile int v;
      a=_now_ns();
      for(int i=0;i<NB;i++) v = Payload_GetDeref(&p, "v", int);
      b=_now_ns();
      (void)v;
      _pb("Payload_GetDeref (existing key)", NB, b-a);
      FreePayload(&p); }

    // CanDispatchMessage: direct (no chain walk)
    { volatile int r;
      a=_now_ns();
      for(int i=0;i<NB;i++) r = CanDispatchMessage(MID_BenchNoOp_Ping, CID_BenchNoOp);
      b=_now_ns();
      (void)r;
      _pb("CanDispatchMessage (direct, 1 MID)", NB, b-a); }

    // CanDispatchMessage: chain walk (BenchGameObject -> GO -> Object)
    { volatile int r;
      a=_now_ns();
      for(int i=0;i<NB;i++) r = CanDispatchMessage(MID_Object_SELF_Create, CID_BenchGameObject);
      b=_now_ns();
      (void)r;
      _pb("CanDispatchMessage (chain walk, 3 levels)", NB, b-a); }
}

static void bench_dispatch_scaling(void) {
    _header("Dispatch Scaling: first MID vs last MID vs N MIDs");
    int NB = 200000;
    double a, b;

    // Warmup
    for (int i = 0; i < 5000; i++) {
        MessagePayload p = PreparePayload(CID_BenchN32, MID_BenchN32_F0001);
        DispatchMessage(&p); FreePayload(&p);
    }

    // --- 32 MIDs ---
    { a=_now_ns();
      for(int i=0;i<NB;i++){
          MessagePayload p = PreparePayload(CID_BenchN32, MID_BenchN32_F0001);
          DispatchMessage(&p); FreePayload(&p);
      } b=_now_ns();
      _pb("32 MIDs: dispatch FIRST (F0001)", NB, b-a); }

    { a=_now_ns();
      for(int i=0;i<NB;i++){
          MessagePayload p = PreparePayload(CID_BenchN32, MID_BenchN32_F0032);
          DispatchMessage(&p); FreePayload(&p);
      } b=_now_ns();
      _pb("32 MIDs: dispatch LAST (F0032)", NB, b-a); }

    { volatile int r; a=_now_ns();
      for(int i=0;i<NB;i++) r = CanDispatchMessage(MID_BenchN32_F0032, CID_BenchN32);
      b=_now_ns(); (void)r;
      _pb("32 MIDs: CanDispatch LAST", NB, b-a); }

    // --- 100 MIDs ---
    { a=_now_ns();
      for(int i=0;i<NB;i++){
          MessagePayload p = PreparePayload(CID_BenchN100, MID_BenchN100_F0001);
          DispatchMessage(&p); FreePayload(&p);
      } b=_now_ns();
      _pb("100 MIDs: dispatch FIRST (F0001)", NB, b-a); }

    { a=_now_ns();
      for(int i=0;i<NB;i++){
          MessagePayload p = PreparePayload(CID_BenchN100, MID_BenchN100_F0100);
          DispatchMessage(&p); FreePayload(&p);
      } b=_now_ns();
      _pb("100 MIDs: dispatch LAST (F0100)", NB, b-a); }

    { volatile int r; a=_now_ns();
      for(int i=0;i<NB;i++) r = CanDispatchMessage(MID_BenchN100_F0100, CID_BenchN100);
      b=_now_ns(); (void)r;
      _pb("100 MIDs: CanDispatch LAST", NB, b-a); }

    // --- 1000 MIDs ---
    { a=_now_ns();
      for(int i=0;i<NB;i++){
          MessagePayload p = PreparePayload(CID_BenchN1000, MID_BenchN1000_F0001);
          DispatchMessage(&p); FreePayload(&p);
      } b=_now_ns();
      _pb("1000 MIDs: dispatch FIRST (F0001)", NB, b-a); }

    { a=_now_ns();
      for(int i=0;i<NB;i++){
          MessagePayload p = PreparePayload(CID_BenchN1000, MID_BenchN1000_F1000);
          DispatchMessage(&p); FreePayload(&p);
      } b=_now_ns();
      _pb("1000 MIDs: dispatch LAST (F1000)", NB, b-a); }

    { volatile int r; a=_now_ns();
      for(int i=0;i<NB;i++) r = CanDispatchMessage(MID_BenchN1000_F1000, CID_BenchN1000);
      b=_now_ns(); (void)r;
      _pb("1000 MIDs: CanDispatch LAST", NB, b-a); }
}

// ============================================================
// Object System: Object lifecycle
// ============================================================

// ============================================================
// Dispatch with N payload parameters
// ============================================================

static void _bench_param_dispatch(const char *label, ClassID cid, MessageID mid, int n_params, int iters) {
    double a, b;
    char key[16];

    // Warmup
    for (int w = 0; w < 100; w++) {
        MessagePayload p = PreparePayload(cid, mid);
        for (int j = 1; j <= n_params; j++) {
            int len = snprintf(key, sizeof(key), "v%d", j);
            UnsafeVariedHashMap_Set(p.data, key, (uint32_t)len, &j, sizeof(int));
        }
        DispatchMessage(&p);
        FreePayload(&p);
    }

    a = _now_ns();
    for (int i = 0; i < iters; i++) {
        MessagePayload p = PreparePayload(cid, mid);
        for (int j = 1; j <= n_params; j++) {
            int len = snprintf(key, sizeof(key), "v%d", j);
            UnsafeVariedHashMap_Set(p.data, key, (uint32_t)len, &j, sizeof(int));
        }
        DispatchMessage(&p);
        FreePayload(&p);
    }
    b = _now_ns();
    _pb(label, iters, b - a);
}

static void bench_param_scaling(void) {
    _header("Dispatch Scaling: N payload parameters (literal keys via S-macros)");
    int NB = 200000;
    double a, b;

    // 1 param with literal key
    { a=_now_ns();
      for(int i=0;i<NB;i++){
          MessagePayload p = PreparePayload(CID_BenchParam1, MID_BenchParam1_Sum1);
          Payload_SetValue(&p, "v1", int, i);
          DispatchMessage(&p);
          FreePayload(&p);
      } b=_now_ns();
      _pb("1 param (literal key)", NB, b-a); }

    // 5 params with literal keys
    { a=_now_ns();
      for(int i=0;i<NB;i++){
          MessagePayload p = PreparePayload(CID_BenchParam5, MID_BenchParam5_Sum5);
          Payload_SetValue(&p, "v1", int, i);
          Payload_SetValue(&p, "v2", int, i);
          Payload_SetValue(&p, "v3", int, i);
          Payload_SetValue(&p, "v4", int, i);
          Payload_SetValue(&p, "v5", int, i);
          DispatchMessage(&p);
          FreePayload(&p);
      } b=_now_ns();
      _pb("5 params (literal keys)", NB, b-a); }

    // 10 params with literal keys
    { a=_now_ns();
      for(int i=0;i<100000;i++){
          MessagePayload p = PreparePayload(CID_BenchParam10, MID_BenchParam10_Sum10);
          Payload_SetValue(&p, "v1", int, i);
          Payload_SetValue(&p, "v2", int, i);
          Payload_SetValue(&p, "v3", int, i);
          Payload_SetValue(&p, "v4", int, i);
          Payload_SetValue(&p, "v5", int, i);
          Payload_SetValue(&p, "v6", int, i);
          Payload_SetValue(&p, "v7", int, i);
          Payload_SetValue(&p, "v8", int, i);
          Payload_SetValue(&p, "v9", int, i);
          Payload_SetValue(&p, "v10", int, i);
          DispatchMessage(&p);
          FreePayload(&p);
      } b=_now_ns();
      _pb("10 params (literal keys)", 100000, b-a); }

    _header("Dispatch Scaling: N params (dynamic keys via snprintf -- worst case)");

    _bench_param_dispatch("1 param (dynamic key)",
        CID_BenchParam1, MID_BenchParam1_Sum1, 1, 200000);

    _bench_param_dispatch("5 params (dynamic keys)",
        CID_BenchParam5, MID_BenchParam5_Sum5, 5, 100000);

    _bench_param_dispatch("10 params (dynamic keys)",
        CID_BenchParam10, MID_BenchParam10_Sum10, 10, 50000);

    _bench_param_dispatch("100 params: full dispatch",
        CID_BenchParam100, MID_BenchParam100_Sum100, 100, 10000);

    _bench_param_dispatch("1000 params: full dispatch",
        CID_BenchParam1000, MID_BenchParam1000_Sum1000, 1000, 1000);
}

// ============================================================
// Object System: Object lifecycle
// ============================================================

static void bench_lifecycle(void) {
    _header("Object System: Lifecycle");
    int N = 10000;
    double a, b;

    // Object_Create + Object_Destroy
    { a=_now_ns();
      for(int i=0;i<N;i++){
          TempObjectReference obj = Object_Create(CID_GameObject);
          Object_Destroy(obj);
      } b=_now_ns();
      _pb("Object_Create + Object_Destroy (GameObject)", N, b-a); }

    // Object_CreateRef + UnRef
    { a=_now_ns();
      for(int i=0;i<N;i++){
          ExternalReference ref = Object_CreateRef(CID_GameObject);
          ObjectContainer_UnRef_External(&ref);
      } b=_now_ns();
      _pb("Object_CreateRef + UnRef_External", N, b-a); }

    // Self value Set + Get cycle
    { ExternalReference obj = Object_CreateRef(CID_GameObject);
      TempObjectReference t = ObjectContainer_TempFrom(obj);
      a=_now_ns();
      for(int i=0;i<N;i++){
          _Object_StoreValue(t->data->values, "bench", 5, &i, sizeof(int), CID_GameObject, SER_SKIP, 0);
      } b=_now_ns();
      _pb("_Object_StoreValue (upsert, same key)", N, b-a);
      a=_now_ns();
      volatile void *p;
      for(int i=0;i<N;i++){
          p = _Object_GetValueData(t->data->values, "bench", 5);
      } b=_now_ns();
      (void)p;
      _pb("_Object_GetValueData (existing key)", N, b-a);
      ObjectContainer_UnRef_External(&obj); }
}

// ============================================================
// GameObject: SpreadMessage
// ============================================================

static void bench_spread(void) {
    _header("GameObject: SpreadMessage");
    int NB = 10000;
    double a, b;

    // Spread to empty tree (root only)
    { ExternalReference root = GameObject_CreateRootRef(CID_GameObject);
      TempObjectReference r = ObjectContainer_TempFrom(root);
      a=_now_ns();
      for(int i=0;i<NB;i++){
          GAMEOBJECT_DISPATCH(r, MID_GameObject_SELF_Update, SPREAD_DOWN, {}, {});
      } b=_now_ns();
      _pb("SPREAD_DOWN (root only, 0 children)", NB, b-a);
      ObjectContainer_UnRef_External(&root); }

    // Spread to 10 children
    { ExternalReference root = GameObject_CreateRootRef(CID_BenchGameObject);
      TempObjectReference r = ObjectContainer_TempFrom(root);
      for(int i=0;i<10;i++) GameObject_CreateChild(r, CID_BenchGameObject);
      a=_now_ns();
      for(int i=0;i<NB;i++){
          GAMEOBJECT_DISPATCH(r, MID_GameObject_SELF_Update, SPREAD_DOWN, {}, {});
      } b=_now_ns();
      _pb("SPREAD_DOWN (root + 10 children)", NB, b-a);
      ObjectContainer_UnRef_External(&root); }

    // Spread to 100 children
    { ExternalReference root = GameObject_CreateRootRef(CID_BenchGameObject);
      TempObjectReference r = ObjectContainer_TempFrom(root);
      for(int i=0;i<100;i++) GameObject_CreateChild(r, CID_BenchGameObject);
      a=_now_ns();
      for(int i=0;i<NB;i++){
          GAMEOBJECT_DISPATCH(r, MID_GameObject_SELF_Update, SPREAD_DOWN, {}, {});
      } b=_now_ns();
      _pb("SPREAD_DOWN (root + 100 children)", NB, b-a);
      ObjectContainer_UnRef_External(&root); }

    // Spread to deep tree (3 levels, 5 children each = 1+5+25+125 = 156 nodes)
    { ExternalReference root = GameObject_CreateRootRef(CID_BenchGameObject);
      TempObjectReference r = ObjectContainer_TempFrom(root);
      for(int i=0;i<5;i++){
          TempObjectReference mid = GameObject_CreateChild(r, CID_BenchGameObject);
          for(int j=0;j<5;j++){
              TempObjectReference leaf = GameObject_CreateChild(mid, CID_BenchGameObject);
              for(int k=0;k<5;k++) GameObject_CreateChild(leaf, CID_BenchGameObject);
          }
      }
      a=_now_ns();
      for(int i=0;i<1000;i++){
          GAMEOBJECT_DISPATCH(r, MID_GameObject_SELF_Update, SPREAD_DOWN, {}, {});
      } b=_now_ns();
      _pb("SPREAD_DOWN (156-node tree, 4 levels)", 1000, b-a);
      ObjectContainer_UnRef_External(&root); }

    // SPREAD_UP for comparison
    { ExternalReference root = GameObject_CreateRootRef(CID_BenchGameObject);
      TempObjectReference r = ObjectContainer_TempFrom(root);
      for(int i=0;i<10;i++) GameObject_CreateChild(r, CID_BenchGameObject);
      a=_now_ns();
      for(int i=0;i<NB;i++){
          GAMEOBJECT_DISPATCH(r, MID_GameObject_SELF_Update, SPREAD_UP, {}, {});
      } b=_now_ns();
      _pb("SPREAD_UP (root + 10 children)", NB, b-a);
      ObjectContainer_UnRef_External(&root); }
}

// ============================================================
// Main
// ============================================================

int main(void) {
    START_LOGGING("bench", LOG_INFO);

    printf("\nEnatrio CTS & Object System Benchmark\n");
    printf("======================================\n");
    printf("Build: release (-O2), %d source files, %d lines\n", SRC_FILE_COUNT, SRC_LINE_COUNT);
    printf("Columns: ops | total time | per-op time | max ops within 1ms / 8.33ms / 16.67ms\n");

    _register_bench_classes();

    // Warmup: prime the payload pool and CPU caches
    {
        for (int i = 0; i < 1000; i++) {
            MessagePayload p = PreparePayload(CID_BenchNoOp, MID_BenchNoOp_Ping);
            FreePayload(&p);
        }
        printf("Payload pool primed: %u cached\n", _payload_pool_count);
    }

    bench_array();
    bench_hashmap();
    bench_varied_hashmap();
    bench_dictionary();
    bench_scaling();
    bench_dispatch();
    bench_dispatch_scaling();
    bench_param_scaling();
    bench_lifecycle();
    bench_spread();

    printf("\n%d benchmarks complete.\n\n", _bench_count);

    // ============================================================
    // Assertions -- calibrated to CI runner (ubuntu-latest, AMD EPYC)
    // Absolute caps use ~3x the CI baseline for noise margin.
    // Scaling caps verify O(1) behavior (ratio near 1.0).
    // ============================================================
    printf("=== Benchmark Assertions ===\n");

    // --- Scaling: MID dispatch must be O(1) ---
    // CI showed ratios of 1.0-1.1x. Cap at 3x for noise.
    {
        double first32 = _bench_lookup("32 MIDs: dispatch FIRST (F0001)");
        double last32  = _bench_lookup("32 MIDs: dispatch LAST (F0032)");
        if (first32 > 0 && last32 > 0) {
            double ratio = last32 / first32;
            printf("  32-MID first/last ratio: %.1fx\n", ratio);
            BENCH_ASSERT("32 MIDs: last/first ratio < 3x (O(1) dispatch)", ratio < 3.0);
        }
    }
    {
        double first100 = _bench_lookup("100 MIDs: dispatch FIRST (F0001)");
        double last100  = _bench_lookup("100 MIDs: dispatch LAST (F0100)");
        if (first100 > 0 && last100 > 0) {
            double ratio = last100 / first100;
            printf("  100-MID first/last ratio: %.1fx\n", ratio);
            BENCH_ASSERT("100 MIDs: last/first ratio < 3x (O(1) dispatch)", ratio < 3.0);
        }
    }
    {
        double first1000 = _bench_lookup("1000 MIDs: dispatch FIRST (F0001)");
        double last1000  = _bench_lookup("1000 MIDs: dispatch LAST (F1000)");
        if (first1000 > 0 && last1000 > 0) {
            double ratio = last1000 / first1000;
            printf("  1000-MID first/last ratio: %.1fx\n", ratio);
            BENCH_ASSERT("1000 MIDs: last/first ratio < 3x (O(1) dispatch)", ratio < 3.0);
        }
    }

    // --- Scaling: CanDispatchMessage must be O(1) across class sizes ---
    {
        double can32  = _bench_lookup("32 MIDs: CanDispatch LAST");
        double can1000 = _bench_lookup("1000 MIDs: CanDispatch LAST");
        if (can32 > 0 && can1000 > 0) {
            double ratio = can1000 / can32;
            printf("  CanDispatch 1000/32 ratio: %.1fx\n", ratio);
            BENCH_ASSERT("CanDispatch scales O(1): 1000/32 ratio < 3x", ratio < 3.0);
        }
    }

    // --- Ratios: faster operations must stay faster ---
    {
        double upsert = _bench_lookup("Upsert (same key, in-place)");
        double rmset  = _bench_lookup("Remove+Set cycle (same key)");
        if (upsert > 0 && rmset > 0) {
            printf("  Upsert vs Remove+Set: %.1fx faster\n", rmset / upsert);
            BENCH_ASSERT("Upsert faster than Remove+Set", upsert < rmset);
        }
    }
    {
        double bulk = _bench_lookup("AddBulk (10000 at once)");
        double seq  = _bench_lookup("Add (sequential)");
        if (bulk > 0 && seq > 0) {
            printf("  AddBulk vs Add: %.1fx faster\n", seq / bulk);
            BENCH_ASSERT("AddBulk faster than sequential Add", bulk < seq);
        }
    }
    {
        double pool = _bench_lookup("PreparePayload + FreePayload (pool)");
        double disp = _bench_lookup("Dispatch (1 MID, no-op handler)");
        if (pool > 0 && disp > 0) {
            printf("  Pool recycle vs full dispatch: %.1fx faster\n", disp / pool);
            BENCH_ASSERT("Pool recycle < full dispatch cost", pool < disp);
        }
    }

    // --- Absolute caps (3x CI baseline for noise margin) ---
    // CI baseline: pool=2.9, dispatch=7.0, get=40.4, upsert=4.9,
    //   candisp=3.2, selfdispatch=559, create+destroy=1670, spread10=5698

    {
        double v = _bench_lookup("PreparePayload + FreePayload (pool)");
        if (v > 0) BENCH_ASSERT("Payload pool < 10 ns (CI baseline: 2.9)", v < 10.0);
    }
    {
        double v = _bench_lookup("Dispatch (1 MID, no-op handler)");
        if (v > 0) BENCH_ASSERT("Dispatch (no-op) < 25 ns (CI baseline: 7.0)", v < 25.0);
    }
    {
        double v = _bench_lookup("Get (existing, incl. snprintf)");
        if (v > 0) BENCH_ASSERT("HashMap Get < 150 ns (CI baseline: 40.4)", v < 150.0);
    }
    {
        double v = _bench_lookup("Upsert (same key, in-place)");
        if (v > 0) BENCH_ASSERT("HashMap Upsert < 20 ns (CI baseline: 4.9)", v < 20.0);
    }
    {
        double v = _bench_lookup("CanDispatchMessage (direct, 1 MID)");
        if (v > 0) BENCH_ASSERT("CanDispatchMessage < 15 ns (CI baseline: 3.2)", v < 15.0);
    }
    {
        double v = _bench_lookup("SELF_DISPATCH (1 SELF MID, no-op)");
        if (v > 0) BENCH_ASSERT("SELF_DISPATCH < 150 ns (CI baseline: 39.7)", v < 150.0);
    }
    {
        double v = _bench_lookup("Object_Create + Object_Destroy (GameObject)");
        if (v > 0) BENCH_ASSERT("Object Create+Destroy < 5000 ns (CI baseline: 1670)", v < 5000.0);
    }
    {
        double v = _bench_lookup("SPREAD_DOWN (root + 10 children)");
        if (v > 0) BENCH_ASSERT("SpreadMessage (10 children) < 20000 ns (CI baseline: 5698)", v < 20000.0);
    }

    printf("\n=== Assertions: %d/%d passed ===\n",
        _assert_count - _assert_fail, _assert_count);

    END_LOGGING();
    return _assert_fail > 0 ? 1 : 0;
}

#undef LINTNORE
#endif
