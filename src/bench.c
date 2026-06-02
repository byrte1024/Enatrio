#ifdef INBENCH

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
#include "classes/exploder.h"
#include "classes/scene_demo.h"

// ============================================================
// Benchmark harness
// ============================================================

static double _now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

static int _bench_count = 0;

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

static void bench_dispatch(void) {
    _header("Class System: Message Dispatch");
    int NB = 100000;
    double a, b;

    BeginClassRegistrations();
    RegisterClass(Object_ClassDef());
    RegisterClass(GameObject_ClassDef());
    RegisterClass(BouncingBox_ClassDef());
    RegisterClass(SpinningCircle_ClassDef());
    RegisterClass(Player_ClassDef());
    RegisterClass(Exploder_ClassDef());
    RegisterClass(Window_ClassDef());
    EndClassRegistrations();

    // Pure PreparePayload + FreePayload (pool recycle)
    { a=_now_ns();
      for(int i=0;i<NB;i++){
          MessagePayload p = PreparePayload(CID_Exploder, MID_Exploder_ShimmiShimmiYea);
          FreePayload(&p);
      } b=_now_ns();
      _pb("PreparePayload + FreePayload (pool)", NB, b-a); }

    // Dispatch (stateless, no Self)
    { a=_now_ns();
      for(int i=0;i<NB;i++){
          MessagePayload p = PreparePayload(CID_Exploder, MID_Exploder_ShimmiShimmiYea);
          Payload_SetValue(&p, "Strength", float, 5.0f);
          DispatchMessage(&p);
          FreePayload(&p);
      } b=_now_ns();
      _pb("Dispatch (Exploder, stateless)", NB, b-a); }

    // SelfDispatch (object lifecycle)
    { ExternalReference obj = Object_CreateRef(CID_GameObject);
      a=_now_ns();
      for(int i=0;i<NB;i++){
          MessagePayload p = PrepareSelfPayload(ObjectContainer_TempFrom(obj), MID_GameObject_SELF_Update);
          DispatchMessage(&p);
          FreePayload(&p);
      } b=_now_ns();
      _pb("SelfDispatch (GameObject Update, no-op)", NB, b-a);
      ObjectContainer_UnRef_External(&obj); }

    // SELF_DISPATCH macro
    { ExternalReference obj = Object_CreateRef(CID_GameObject);
      TempObjectReference t = ObjectContainer_TempFrom(obj);
      a=_now_ns();
      for(int i=0;i<NB;i++){
          SELF_DISPATCH(t, MID_GameObject_SELF_Update, {}, {});
      } b=_now_ns();
      _pb("SELF_DISPATCH macro (GameObject Update)", NB, b-a);
      ObjectContainer_UnRef_External(&obj); }

    // Payload_SetValue + Payload_GetDeref
    { MessagePayload p = PreparePayload(CID_Exploder, MID_Exploder_ShimmiShimmiYea);
      a=_now_ns();
      for(int i=0;i<NB;i++){
          Payload_SetValue(&p, "val", int, i);
      } b=_now_ns();
      _pb("Payload_SetValue (upsert, same key)", NB, b-a);
      a=_now_ns();
      volatile int v;
      for(int i=0;i<NB;i++){
          v = Payload_GetDeref(&p, "val", int);
      } b=_now_ns();
      (void)v;
      _pb("Payload_GetDeref (existing key)", NB, b-a);
      FreePayload(&p); }

    // Inheritance chain walk: dispatch to grandchild (3-level chain)
    { ExternalReference obj = Object_CreateRef(CID_BouncingBox);
      TempObjectReference t = ObjectContainer_TempFrom(obj);
      a=_now_ns();
      for(int i=0;i<NB;i++){
          SELF_DISPATCH(t, MID_Object_SELF_Create, {}, {});
      } b=_now_ns();
      _pb("Chain walk dispatch (BouncingBox->GO->Object)", NB, b-a);
      ObjectContainer_UnRef_External(&obj); }

    // CanDispatchMessage
    { a=_now_ns();
      volatile int r;
      for(int i=0;i<NB;i++){
          r = CanDispatchMessage(MID_GameObject_SELF_Update, CID_BouncingBox);
      } b=_now_ns();
      (void)r;
      _pb("CanDispatchMessage (chain walk, 2 levels)", NB, b-a); }
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
    { ExternalReference root = GameObject_CreateRootRef(CID_BouncingBox);
      TempObjectReference r = ObjectContainer_TempFrom(root);
      for(int i=0;i<10;i++) GameObject_CreateChild(r, CID_BouncingBox);
      a=_now_ns();
      for(int i=0;i<NB;i++){
          GAMEOBJECT_DISPATCH(r, MID_GameObject_SELF_Update, SPREAD_DOWN, {}, {});
      } b=_now_ns();
      _pb("SPREAD_DOWN (root + 10 children)", NB, b-a);
      ObjectContainer_UnRef_External(&root); }

    // Spread to 100 children
    { ExternalReference root = GameObject_CreateRootRef(CID_BouncingBox);
      TempObjectReference r = ObjectContainer_TempFrom(root);
      for(int i=0;i<100;i++) GameObject_CreateChild(r, CID_BouncingBox);
      a=_now_ns();
      for(int i=0;i<NB;i++){
          GAMEOBJECT_DISPATCH(r, MID_GameObject_SELF_Update, SPREAD_DOWN, {}, {});
      } b=_now_ns();
      _pb("SPREAD_DOWN (root + 100 children)", NB, b-a);
      ObjectContainer_UnRef_External(&root); }

    // Spread to deep tree (3 levels, 5 children each = 1+5+25+125 = 156 nodes)
    { ExternalReference root = GameObject_CreateRootRef(CID_BouncingBox);
      TempObjectReference r = ObjectContainer_TempFrom(root);
      for(int i=0;i<5;i++){
          TempObjectReference mid = GameObject_CreateChild(r, CID_BouncingBox);
          for(int j=0;j<5;j++){
              TempObjectReference leaf = GameObject_CreateChild(mid, CID_BouncingBox);
              for(int k=0;k<5;k++) GameObject_CreateChild(leaf, CID_BouncingBox);
          }
      }
      a=_now_ns();
      for(int i=0;i<1000;i++){
          GAMEOBJECT_DISPATCH(r, MID_GameObject_SELF_Update, SPREAD_DOWN, {}, {});
      } b=_now_ns();
      _pb("SPREAD_DOWN (156-node tree, 4 levels)", 1000, b-a);
      ObjectContainer_UnRef_External(&root); }

    // SPREAD_UP for comparison
    { ExternalReference root = GameObject_CreateRootRef(CID_BouncingBox);
      TempObjectReference r = ObjectContainer_TempFrom(root);
      for(int i=0;i<10;i++) GameObject_CreateChild(r, CID_BouncingBox);
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

    // Warmup: prime the payload pool and CPU caches
    {
        for (int i = 0; i < 1000; i++) {
            MessagePayload p = PreparePayload(CID_Untyped, MESSAGEID_EMPTY);
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
    bench_lifecycle();
    bench_spread();

    printf("\n%d benchmarks complete.\n", _bench_count);

    END_LOGGING();
    return 0;
}

#endif
