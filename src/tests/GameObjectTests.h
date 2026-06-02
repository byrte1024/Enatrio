#pragma once

#include "../system/tests.h"
#include "../system/object/Self.h"
#include "../classes/exploder.h"

#define LINTNORE

// ============================================================
// Shared execution log
// ============================================================

static int _go_exec_log[64];
static int _go_exec_count = 0;
static void _go_exec_reset(void) { _go_exec_count = 0; memset(_go_exec_log, 0, sizeof(_go_exec_log)); }

// ============================================================
// Test class: GOTestNode (CID 0x2001) -- inherits GameObject
// Records its "id" value into _go_exec_log on SELF_Update
// ============================================================

#define TYPE GOTestNode

BEGIN_CLASS(0x2001);
INHERITS(GameObject);

DECLARE_SELF_MID(Update);

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Create)
    CALL_BASE();
    Self_SetValue("id", int, 0);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Destroy)
    CALL_BASE();
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN(Update)
    int id = Self_GetDeref("id", int);
    _go_exec_log[_go_exec_count++] = id;
MESSAGE_HANDLER_END()

CAN_RECEIVE_BEGIN()
    SELF_CAN_RECEIVE_MID_EXTERN(Object, Create)
    SELF_CAN_RECEIVE_MID_EXTERN(Object, Destroy)
    SELF_CAN_RECEIVE_MID(Update)
CAN_RECEIVE_END()

RECEIVE_MESSAGE_BEGIN()
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Object, Create)
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Object, Destroy)
    SELF_RECEIVE_MESSAGE_ROUTE(Update)
RECEIVE_MESSAGE_END()

CLASSDEF_INHERITS(GameObject)

#undef TYPE

// ============================================================
// Test class: GOConsumer (CID 0x2002) -- inherits GameObject
// Same as GOTestNode but consumes the spread after logging
// ============================================================

#define TYPE GOConsumer

BEGIN_CLASS(0x2002);
INHERITS(GameObject);

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Create)
    CALL_BASE();
    Self_SetValue("id", int, 0);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Destroy)
    CALL_BASE();
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(GOTestNode, Update)
    int id = Self_GetDeref("id", int);
    _go_exec_log[_go_exec_count++] = id;
    SPREAD_CONSUME(payload);
MESSAGE_HANDLER_END()

CAN_RECEIVE_BEGIN()
    SELF_CAN_RECEIVE_MID_EXTERN(Object, Create)
    SELF_CAN_RECEIVE_MID_EXTERN(Object, Destroy)
    SELF_CAN_RECEIVE_MID_EXTERN(GOTestNode, Update)
CAN_RECEIVE_END()

RECEIVE_MESSAGE_BEGIN()
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Object, Create)
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Object, Destroy)
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(GOTestNode, Update)
RECEIVE_MESSAGE_END()

CLASSDEF_INHERITS(GameObject)

#undef TYPE

// ============================================================
// Helper: create a GOTestNode with given id and priority
// ============================================================

static TempObjectReference _go_create_node(TempObjectReference parent, int id, int priority) {
    TempObjectReference node;
    if (parent) {
        node = GameObject_CreateChild(parent, CID_GOTestNode);
    } else {
        node = GameObject_CreateRoot(CID_GOTestNode);
    }
    if (node == NULL) return NULL;
    _Object_StoreValue(node->data->values, "id", 2,
                       &id, sizeof(int), CID_GOTestNode, SER_SKIP, 0);
    if (priority != 0) {
        _Object_StoreValue(node->data->values, "priority", 8,
                           &priority, sizeof(int), CID_GOTestNode, SER_RAW, 0);
    }
    return node;
}

// ============================================================
// Registration helper
// ============================================================

static void _go_register_all(void) {
    BeginClassRegistrations();
    RegisterClass(Object_ClassDef());
    RegisterClass(GameObject_ClassDef());
    RegisterClass(GOTestNode_ClassDef());
    RegisterClass(GOConsumer_ClassDef());
    RegisterClass(Exploder_ClassDef());
    EndClassRegistrations();
}

// ============================================================
// Tests
// ============================================================

// 1. CreateRootRef, verify active=1, child_count=0
static void test_go_create_root(void) {
    TEST("go: CreateRootRef sets active=1, child_count=0");
    ExternalReference ref = GameObject_CreateRootRef(CID_GOTestNode);
    ASSERT(ref != NULL);
    TempObjectReference obj = ObjectContainer_TempFrom(ref);
    ASSERT(obj->data != NULL);

    int *active = (int *)_Object_GetValueData(obj->data->values, "active", 6);
    ASSERT(active != NULL);
    ASSERT(*active == 1);

    int *count = (int *)_Object_GetValueData(obj->data->values, "child_count", 11);
    ASSERT(count != NULL);
    ASSERT(*count == 0);

    ObjectContainer_UnRef_External(&ref);
    PASS();
}

// 2. CreateChild x2, verify child_count=2, verify parent back-ref
static void test_go_add_children(void) {
    TEST("go: AddChild x2, count=2, parent back-ref set");
    ExternalReference rref = GameObject_CreateRootRef(CID_GOTestNode);
    ASSERT(rref != NULL);
    TempObjectReference root = ObjectContainer_TempFrom(rref);

    TempObjectReference c1 = GameObject_CreateChild(root, CID_GOTestNode);
    ASSERT(c1 != NULL);
    TempObjectReference c2 = GameObject_CreateChild(root, CID_GOTestNode);
    ASSERT(c2 != NULL);

    int *count = (int *)_Object_GetValueData(root->data->values, "child_count", 11);
    ASSERT(count != NULL);
    ASSERT(*count == 2);

    // Verify parent back-ref on c1
    TempObjectReference p1 = Object_SGetRef(c1, "parent");
    ASSERT(p1 == root);

    // Verify parent back-ref on c2
    TempObjectReference p2 = Object_SGetRef(c2, "parent");
    ASSERT(p2 == root);

    ObjectContainer_UnRef_External(&rref);
    PASS();
}

// 3. Add 3 children (ids 10,20,30), remove middle, verify re-indexing
static void test_go_remove_child(void) {
    TEST("go: RemoveChild re-indexes remaining children");
    ExternalReference rref = GameObject_CreateRootRef(CID_GOTestNode);
    TempObjectReference root = ObjectContainer_TempFrom(rref);

    TempObjectReference c1 = _go_create_node(root, 10, 0);
    TempObjectReference c2 = _go_create_node(root, 20, 0);
    TempObjectReference c3 = _go_create_node(root, 30, 0);
    ASSERT(c1 != NULL);
    ASSERT(c2 != NULL);
    ASSERT(c3 != NULL);

    // Remove middle child (c2, id=20)
    SELF_DISPATCH(root, MID_GameObject_SELF_RemoveChild, {
        Payload_SetValue(msg, "child", TempObjectReference, c2);
    }, {});

    int *count = (int *)_Object_GetValueData(root->data->values, "child_count", 11);
    ASSERT(count != NULL);
    ASSERT(*count == 2);

    // child_0 should be c1 (id=10)
    char kbuf[_GO_CHILD_KEY_MAX];
    uint32_t klen = _go_child_key(kbuf, 0);
    TempObjectReference slot0 = Object_GetRef(root, kbuf, klen);
    ASSERT(slot0 == c1);
    int *id0 = (int *)_Object_GetValueData(slot0->data->values, "id", 2);
    ASSERT(id0 != NULL);
    ASSERT(*id0 == 10);

    // child_1 should be c3 (id=30)
    klen = _go_child_key(kbuf, 1);
    TempObjectReference slot1 = Object_GetRef(root, kbuf, klen);
    ASSERT(slot1 == c3);
    int *id1 = (int *)_Object_GetValueData(slot1->data->values, "id", 2);
    ASSERT(id1 != NULL);
    ASSERT(*id1 == 30);

    ObjectContainer_UnRef_External(&rref);
    PASS();
}

// 4. SPREAD_DOWN: root(1) + 2 children(2,3). Verify exec order: 1,2,3
static void test_go_spread_down(void) {
    TEST("go: SPREAD_DOWN visits root then children");
    _go_exec_reset();
    ExternalReference rref = GameObject_CreateRootRef(CID_GOTestNode);
    TempObjectReference root = ObjectContainer_TempFrom(rref);
    _Object_StoreValue(root->data->values, "id", 2, &(int){1}, sizeof(int), CID_GOTestNode, SER_SKIP, 0);

    _go_create_node(root, 2, 0);
    _go_create_node(root, 3, 0);

    GAMEOBJECT_DISPATCH(root, MID_GOTestNode_SELF_Update, SPREAD_DOWN, {}, {});

    ASSERT(_go_exec_count == 3);
    ASSERT(_go_exec_log[0] == 1);
    ASSERT(_go_exec_log[1] == 2);
    ASSERT(_go_exec_log[2] == 3);

    ObjectContainer_UnRef_External(&rref);
    PASS();
}

// 5. SPREAD_UP: same setup. Verify order: 2,3,1
static void test_go_spread_up(void) {
    TEST("go: SPREAD_UP visits children then root");
    _go_exec_reset();
    ExternalReference rref = GameObject_CreateRootRef(CID_GOTestNode);
    TempObjectReference root = ObjectContainer_TempFrom(rref);
    _Object_StoreValue(root->data->values, "id", 2, &(int){1}, sizeof(int), CID_GOTestNode, SER_SKIP, 0);

    _go_create_node(root, 2, 0);
    _go_create_node(root, 3, 0);

    GAMEOBJECT_DISPATCH(root, MID_GOTestNode_SELF_Update, SPREAD_UP, {}, {});

    ASSERT(_go_exec_count == 3);
    ASSERT(_go_exec_log[0] == 2);
    ASSERT(_go_exec_log[1] == 3);
    ASSERT(_go_exec_log[2] == 1);

    ObjectContainer_UnRef_External(&rref);
    PASS();
}

// 6. Consumer stops subtree: Root(1) -> GOConsumer(2) -> leaf(3), sibling(4)
static void test_go_consume_stops_subtree(void) {
    TEST("go: SPREAD_CONSUME stops subtree, siblings still visited");
    _go_exec_reset();
    ExternalReference rref = GameObject_CreateRootRef(CID_GOTestNode);
    TempObjectReference root = ObjectContainer_TempFrom(rref);
    _Object_StoreValue(root->data->values, "id", 2, &(int){1}, sizeof(int), CID_GOTestNode, SER_SKIP, 0);

    // Consumer child (id=2)
    TempObjectReference consumer = GameObject_CreateChild(root, CID_GOConsumer);
    ASSERT(consumer != NULL);
    _Object_StoreValue(consumer->data->values, "id", 2, &(int){2}, sizeof(int), CID_GOConsumer, SER_SKIP, 0);

    // Leaf under consumer (id=3)
    TempObjectReference leaf = _go_create_node(consumer, 3, 0);
    ASSERT(leaf != NULL);

    // Sibling of consumer (id=4)
    _go_create_node(root, 4, 0);

    GAMEOBJECT_DISPATCH(root, MID_GOTestNode_SELF_Update, SPREAD_DOWN, {}, {});

    // Root(1) visited, consumer(2) visited and consumes, leaf(3) skipped, sibling(4) visited
    ASSERT(_go_exec_count == 3);
    ASSERT(_go_exec_log[0] == 1);
    ASSERT(_go_exec_log[1] == 2);
    ASSERT(_go_exec_log[2] == 4);

    ObjectContainer_UnRef_External(&rref);
    PASS();
}

// 7. Inactive child skipped
static void test_go_inactive_skipped(void) {
    TEST("go: inactive child skipped during spread");
    _go_exec_reset();
    ExternalReference rref = GameObject_CreateRootRef(CID_GOTestNode);
    TempObjectReference root = ObjectContainer_TempFrom(rref);
    _Object_StoreValue(root->data->values, "id", 2, &(int){1}, sizeof(int), CID_GOTestNode, SER_SKIP, 0);

    TempObjectReference c1 = _go_create_node(root, 2, 0);
    _go_create_node(root, 3, 0);

    // Deactivate child 2
    SELF_DISPATCH(c1, MID_GameObject_SELF_SetActive, {
        Payload_SetValue(msg, "active", int, 0);
    }, {});

    GAMEOBJECT_DISPATCH(root, MID_GOTestNode_SELF_Update, SPREAD_DOWN, {}, {});

    ASSERT(_go_exec_count == 2);
    ASSERT(_go_exec_log[0] == 1);
    ASSERT(_go_exec_log[1] == 3);

    ObjectContainer_UnRef_External(&rref);
    PASS();
}

// 8. Priority ordering: children with priorities 30,10,20 -> exec order 10,20,30
static void test_go_priority_ordering(void) {
    TEST("go: children sorted by priority during spread");
    _go_exec_reset();
    ExternalReference rref = GameObject_CreateRootRef(CID_GOTestNode);
    TempObjectReference root = ObjectContainer_TempFrom(rref);
    _Object_StoreValue(root->data->values, "id", 2, &(int){0}, sizeof(int), CID_GOTestNode, SER_SKIP, 0);

    // Create 3 children as roots, set priority, then AddChild manually
    TempObjectReference c1 = GameObject_CreateRoot(CID_GOTestNode);
    _Object_StoreValue(c1->data->values, "id", 2, &(int){30}, sizeof(int), CID_GOTestNode, SER_SKIP, 0);
    _Object_StoreValue(c1->data->values, "priority", 8, &(int){30}, sizeof(int), CID_GOTestNode, SER_RAW, 0);
    SELF_DISPATCH(root, MID_GameObject_SELF_AddChild, { Payload_SetValue(msg, "child", TempObjectReference, c1); }, {});

    TempObjectReference c2 = GameObject_CreateRoot(CID_GOTestNode);
    _Object_StoreValue(c2->data->values, "id", 2, &(int){10}, sizeof(int), CID_GOTestNode, SER_SKIP, 0);
    _Object_StoreValue(c2->data->values, "priority", 8, &(int){10}, sizeof(int), CID_GOTestNode, SER_RAW, 0);
    SELF_DISPATCH(root, MID_GameObject_SELF_AddChild, { Payload_SetValue(msg, "child", TempObjectReference, c2); }, {});

    TempObjectReference c3 = GameObject_CreateRoot(CID_GOTestNode);
    _Object_StoreValue(c3->data->values, "id", 2, &(int){20}, sizeof(int), CID_GOTestNode, SER_SKIP, 0);
    _Object_StoreValue(c3->data->values, "priority", 8, &(int){20}, sizeof(int), CID_GOTestNode, SER_RAW, 0);
    SELF_DISPATCH(root, MID_GameObject_SELF_AddChild, { Payload_SetValue(msg, "child", TempObjectReference, c3); }, {});

    GAMEOBJECT_DISPATCH(root, MID_GOTestNode_SELF_Update, SPREAD_DOWN, {}, {});

    ASSERT(_go_exec_count == 4);
    ASSERT(_go_exec_log[0] == 0);
    ASSERT(_go_exec_log[1] == 10);
    ASSERT(_go_exec_log[2] == 20);
    ASSERT(_go_exec_log[3] == 30);

    ObjectContainer_UnRef_External(&rref);
    PASS();
}

// 9. Reverse flag: children visited in reverse order
static void test_go_reverse_flag(void) {
    TEST("go: spread_reverse visits children in reverse");
    _go_exec_reset();
    ExternalReference rref = GameObject_CreateRootRef(CID_GOTestNode);
    TempObjectReference root = ObjectContainer_TempFrom(rref);
    _Object_StoreValue(root->data->values, "id", 2, &(int){0}, sizeof(int), CID_GOTestNode, SER_SKIP, 0);

    _go_create_node(root, 1, 0);
    _go_create_node(root, 2, 0);
    _go_create_node(root, 3, 0);

    GAMEOBJECT_DISPATCH(root, MID_GOTestNode_SELF_Update, SPREAD_DOWN, {
        Payload_SetValue(msg, "spread_reverse", int, 1);
    }, {});

    ASSERT(_go_exec_count == 4);
    ASSERT(_go_exec_log[0] == 0);
    ASSERT(_go_exec_log[1] == 3);
    ASSERT(_go_exec_log[2] == 2);
    ASSERT(_go_exec_log[3] == 1);

    ObjectContainer_UnRef_External(&rref);
    PASS();
}

// 10. Deep tree: Root(1)->mid(2)->leaf(3)
static void test_go_deep_tree(void) {
    TEST("go: deep tree Root->mid->leaf all receive");
    _go_exec_reset();
    ExternalReference rref = GameObject_CreateRootRef(CID_GOTestNode);
    TempObjectReference root = ObjectContainer_TempFrom(rref);
    _Object_StoreValue(root->data->values, "id", 2, &(int){1}, sizeof(int), CID_GOTestNode, SER_SKIP, 0);

    TempObjectReference mid = _go_create_node(root, 2, 0);
    _go_create_node(mid, 3, 0);

    GAMEOBJECT_DISPATCH(root, MID_GOTestNode_SELF_Update, SPREAD_DOWN, {}, {});

    ASSERT(_go_exec_count == 3);
    ASSERT(_go_exec_log[0] == 1);
    ASSERT(_go_exec_log[1] == 2);
    ASSERT(_go_exec_log[2] == 3);

    ObjectContainer_UnRef_External(&rref);
    PASS();
}

// 11. Shared payload mutation: set test_marker=42, verify after spread
static void test_go_shared_payload_mutation(void) {
    TEST("go: payload data survives spread dispatch");
    _go_exec_reset();
    ExternalReference rref = GameObject_CreateRootRef(CID_GOTestNode);
    TempObjectReference root = ObjectContainer_TempFrom(rref);
    _Object_StoreValue(root->data->values, "id", 2, &(int){1}, sizeof(int), CID_GOTestNode, SER_SKIP, 0);

    _go_create_node(root, 2, 0);

    int found_marker = 0;
    GAMEOBJECT_DISPATCH(root, MID_GOTestNode_SELF_Update, SPREAD_DOWN, {
        Payload_SetValue(msg, "test_marker", int, 42);
    }, {
        int *m = (int *)Payload_Get(msg, "test_marker");
        if (m) found_marker = *m;
    });

    ASSERT(found_marker == 42);
    ASSERT(_go_exec_count == 2);

    ObjectContainer_UnRef_External(&rref);
    PASS();
}

// 12. SetPriority resorts: a(pri=10), b(pri=20). Change a to 30. Exec: 0,2,1
static void test_go_set_priority_resorts(void) {
    TEST("go: SetPriority re-sorts children");
    _go_exec_reset();
    ExternalReference rref = GameObject_CreateRootRef(CID_GOTestNode);
    TempObjectReference root = ObjectContainer_TempFrom(rref);
    _Object_StoreValue(root->data->values, "id", 2, &(int){0}, sizeof(int), CID_GOTestNode, SER_SKIP, 0);

    // Create a (id=1, pri=10) and b (id=2, pri=20) using root-then-add approach
    // Hold external refs so SetPriority's remove+re-add doesn't free them
    ExternalReference aref = GameObject_CreateRootRef(CID_GOTestNode);
    TempObjectReference a = ObjectContainer_TempFrom(aref);
    _Object_StoreValue(a->data->values, "id", 2, &(int){1}, sizeof(int), CID_GOTestNode, SER_SKIP, 0);
    _Object_StoreValue(a->data->values, "priority", 8, &(int){10}, sizeof(int), CID_GOTestNode, SER_RAW, 0);
    SELF_DISPATCH(root, MID_GameObject_SELF_AddChild, { Payload_SetValue(msg, "child", TempObjectReference, a); }, {});

    ExternalReference bref = GameObject_CreateRootRef(CID_GOTestNode);
    TempObjectReference b = ObjectContainer_TempFrom(bref);
    _Object_StoreValue(b->data->values, "id", 2, &(int){2}, sizeof(int), CID_GOTestNode, SER_SKIP, 0);
    _Object_StoreValue(b->data->values, "priority", 8, &(int){20}, sizeof(int), CID_GOTestNode, SER_RAW, 0);
    SELF_DISPATCH(root, MID_GameObject_SELF_AddChild, { Payload_SetValue(msg, "child", TempObjectReference, b); }, {});

    // Change a's priority to 30 (should move after b)
    SELF_DISPATCH(a, MID_GameObject_SELF_SetPriority, {
        Payload_SetValue(msg, "priority", int, 30);
    }, {});

    GAMEOBJECT_DISPATCH(root, MID_GOTestNode_SELF_Update, SPREAD_DOWN, {}, {});

    ASSERT(_go_exec_count == 3);
    ASSERT(_go_exec_log[0] == 0);
    ASSERT(_go_exec_log[1] == 2);
    ASSERT(_go_exec_log[2] == 1);

    ObjectContainer_UnRef_External(&aref);
    ObjectContainer_UnRef_External(&bref);
    ObjectContainer_UnRef_External(&rref);
    PASS();
}

// 13. Parent ref after remove: verify parent set, then cleared
static void test_go_parent_ref_after_remove(void) {
    TEST("go: parent ref set on add, cleared on remove");
    ExternalReference rref = GameObject_CreateRootRef(CID_GOTestNode);
    TempObjectReference root = ObjectContainer_TempFrom(rref);

    // Hold external ref so child survives RemoveChild
    ExternalReference cref = GameObject_CreateChildRef(root, CID_GOTestNode);
    ASSERT(cref != NULL);
    TempObjectReference child = ObjectContainer_TempFrom(cref);

    // Parent should be set
    TempObjectReference parent = Object_SGetRef(child, "parent");
    ASSERT(parent == root);

    // Remove child
    SELF_DISPATCH(root, MID_GameObject_SELF_RemoveChild, {
        Payload_SetValue(msg, "child", TempObjectReference, child);
    }, {});

    // Parent should be cleared
    TempObjectReference parent_after = Object_SGetRef(child, "parent");
    ASSERT(parent_after == NULL);

    ObjectContainer_UnRef_External(&cref);
    ObjectContainer_UnRef_External(&rref);
    PASS();
}

// 14. CreateChild and CreateChildRef both work, count=2
static void test_go_create_child_helpers(void) {
    TEST("go: CreateChild + CreateChildRef both add children, count=2");
    ExternalReference rref = GameObject_CreateRootRef(CID_GOTestNode);
    TempObjectReference root = ObjectContainer_TempFrom(rref);

    TempObjectReference c1 = GameObject_CreateChild(root, CID_GOTestNode);
    ASSERT(c1 != NULL);

    ExternalReference c2ref = GameObject_CreateChildRef(root, CID_GOTestNode);
    ASSERT(c2ref != NULL);

    int *count = (int *)_Object_GetValueData(root->data->values, "child_count", 11);
    ASSERT(count != NULL);
    ASSERT(*count == 2);

    ObjectContainer_UnRef_External(&c2ref);
    ObjectContainer_UnRef_External(&rref);
    PASS();
}

// ============================================================
// Runner
// ============================================================

static void run_gameobject_tests(void) {
    _go_register_all();

    LOG_INFO("=== GameObject Tests ===");
    test_go_create_root();
    test_go_add_children();
    test_go_remove_child();
    test_go_spread_down();
    test_go_spread_up();
    test_go_consume_stops_subtree();
    test_go_inactive_skipped();
    test_go_priority_ordering();
    test_go_reverse_flag();
    test_go_deep_tree();
    test_go_shared_payload_mutation();
    test_go_set_priority_resorts();
    test_go_parent_ref_after_remove();
    test_go_create_child_helpers();
}

#undef LINTNORE
