#pragma once

#include "../system/tests.h"
#include "../system/object/Self.h"
// Minimal no-parent class for registration tests
#define TYPE GOTestStandalone
BEGIN_CLASS(0xF002);
DECLARE_MID(Poke, 0x01);
MESSAGE_HANDLER_BEGIN(Poke)
MESSAGE_HANDLER_END()
CAN_RECEIVE_BEGIN()
    CAN_RECEIVE_MID(Poke)
CAN_RECEIVE_END()
RECEIVE_MESSAGE_BEGIN()
    RECEIVE_MESSAGE_ROUTE(Poke)
RECEIVE_MESSAGE_END()
CLASSDEF()
#undef TYPE

#define LINTNORE

// ============================================================
// Shared execution log
// ============================================================

static int _go_exec_log[128];
static int _go_exec_count = 0;
static void _go_exec_reset(void) { _go_exec_count = 0; memset(_go_exec_log, 0, sizeof(_go_exec_log)); }

// ============================================================
// Test class: GOTestNode (CID 0x2001) -- inherits GameObject
// Records its "id" value into _go_exec_log on SELF_Update
// ============================================================

#define TYPE GOTestNode

BEGIN_CLASS(0x2001);
INHERITS(GameObject);

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Create)
    CALL_BASE();
    Self_SetValue("id", int, 0);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Destroy)
    CALL_BASE();
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(GameObject, Update)
    IGNORE_BASE();
    int id = Self_GetDeref("id", int);
    _go_exec_log[_go_exec_count++] = id;
MESSAGE_HANDLER_END()

CAN_RECEIVE_BEGIN()
    SELF_CAN_RECEIVE_MID_EXTERN(Object, Create)
    SELF_CAN_RECEIVE_MID_EXTERN(Object, Destroy)
    SELF_CAN_RECEIVE_MID_EXTERN(GameObject, Update)
CAN_RECEIVE_END()

RECEIVE_MESSAGE_BEGIN()
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Object, Create)
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Object, Destroy)
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(GameObject, Update)
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

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(GameObject, Update)
    IGNORE_BASE();
    int id = Self_GetDeref("id", int);
    _go_exec_log[_go_exec_count++] = id;
    SPREAD_CONSUME(payload);
MESSAGE_HANDLER_END()

CAN_RECEIVE_BEGIN()
    SELF_CAN_RECEIVE_MID_EXTERN(Object, Create)
    SELF_CAN_RECEIVE_MID_EXTERN(Object, Destroy)
    SELF_CAN_RECEIVE_MID_EXTERN(GameObject, Update)
CAN_RECEIVE_END()

RECEIVE_MESSAGE_BEGIN()
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Object, Create)
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Object, Destroy)
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(GameObject, Update)
RECEIVE_MESSAGE_END()

CLASSDEF_INHERITS(GameObject)

#undef TYPE

// ============================================================
// Test class: GOAdder (CID 0x2003) -- inherits GameObject
// On Update, adds a new child (once) to test snapshot safety
// ============================================================

#define TYPE GOAdder

BEGIN_CLASS(0x2003);
INHERITS(GameObject);

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Create)
    CALL_BASE();
    Self_SetValue("id", int, 0);
    Self_SetValue("did_add", int, 0);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Destroy)
    CALL_BASE();
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(GameObject, Update)
    IGNORE_BASE();
    int id = Self_GetDeref("id", int);
    _go_exec_log[_go_exec_count++] = id;
    int did_add = Self_GetDeref("did_add", int);
    if (!did_add) {
        Self_SetValue("did_add", int, 1);
        // Add a new child during spread
        TempObjectReference new_child = GameObject_CreateChild(Self, CID_GOTestNode);
        if (new_child) {
            _Object_StoreValue(new_child->data->values, "id", 2,
                               &(int){999}, sizeof(int), CID_GOTestNode, SER_SKIP, 0);
        }
    }
MESSAGE_HANDLER_END()

CAN_RECEIVE_BEGIN()
    SELF_CAN_RECEIVE_MID_EXTERN(Object, Create)
    SELF_CAN_RECEIVE_MID_EXTERN(Object, Destroy)
    SELF_CAN_RECEIVE_MID_EXTERN(GameObject, Update)
CAN_RECEIVE_END()

RECEIVE_MESSAGE_BEGIN()
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Object, Create)
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Object, Destroy)
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(GameObject, Update)
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
    RegisterClass(GOTestStandalone_ClassDef());
    RegisterClass(GOAdder_ClassDef());
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

    GAMEOBJECT_DISPATCH(root, MID_GameObject_SELF_Update, SPREAD_DOWN, {}, {});

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

    GAMEOBJECT_DISPATCH(root, MID_GameObject_SELF_Update, SPREAD_UP, {}, {});

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

    GAMEOBJECT_DISPATCH(root, MID_GameObject_SELF_Update, SPREAD_DOWN, {}, {});

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

    GAMEOBJECT_DISPATCH(root, MID_GameObject_SELF_Update, SPREAD_DOWN, {}, {});

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

    GAMEOBJECT_DISPATCH(root, MID_GameObject_SELF_Update, SPREAD_DOWN, {}, {});

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

    GAMEOBJECT_DISPATCH(root, MID_GameObject_SELF_Update, SPREAD_DOWN, {
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

    GAMEOBJECT_DISPATCH(root, MID_GameObject_SELF_Update, SPREAD_DOWN, {}, {});

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
    GAMEOBJECT_DISPATCH(root, MID_GameObject_SELF_Update, SPREAD_DOWN, {
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

    GAMEOBJECT_DISPATCH(root, MID_GameObject_SELF_Update, SPREAD_DOWN, {}, {});

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
// Edge case: empty tree
// ============================================================

static void test_go_spread_empty_tree(void) {
    TEST("go: SPREAD_DOWN on root with 0 children -- only root receives");
    _go_exec_reset();
    ExternalReference rref = GameObject_CreateRootRef(CID_GOTestNode);
    TempObjectReference root = ObjectContainer_TempFrom(rref);
    _Object_StoreValue(root->data->values, "id", 2, &(int){1}, sizeof(int), CID_GOTestNode, SER_SKIP, 0);

    GAMEOBJECT_DISPATCH(root, MID_GameObject_SELF_Update, SPREAD_DOWN, {}, {});

    ASSERT(_go_exec_count == 1);
    ASSERT(_go_exec_log[0] == 1);

    ObjectContainer_UnRef_External(&rref);
    PASS();
}

// ============================================================
// Edge case: single child
// ============================================================

static void test_go_spread_single_child(void) {
    TEST("go: root + 1 child, both receive in correct order");
    _go_exec_reset();
    ExternalReference rref = GameObject_CreateRootRef(CID_GOTestNode);
    TempObjectReference root = ObjectContainer_TempFrom(rref);
    _Object_StoreValue(root->data->values, "id", 2, &(int){1}, sizeof(int), CID_GOTestNode, SER_SKIP, 0);

    _go_create_node(root, 2, 0);

    // DOWN: root first, then child
    GAMEOBJECT_DISPATCH(root, MID_GameObject_SELF_Update, SPREAD_DOWN, {}, {});
    ASSERT(_go_exec_count == 2);
    ASSERT(_go_exec_log[0] == 1);
    ASSERT(_go_exec_log[1] == 2);

    // UP: child first, then root
    _go_exec_reset();
    GAMEOBJECT_DISPATCH(root, MID_GameObject_SELF_Update, SPREAD_UP, {}, {});
    ASSERT(_go_exec_count == 2);
    ASSERT(_go_exec_log[0] == 2);
    ASSERT(_go_exec_log[1] == 1);

    ObjectContainer_UnRef_External(&rref);
    PASS();
}

// ============================================================
// Remove nonexistent child
// ============================================================

static void test_go_remove_nonexistent_child(void) {
    TEST("go: RemoveChild on non-child returns NOT_FOUND, count unchanged");
    ExternalReference rref = GameObject_CreateRootRef(CID_GOTestNode);
    TempObjectReference root = ObjectContainer_TempFrom(rref);

    _go_create_node(root, 10, 0);

    // Create a separate node not in root's child list
    ExternalReference other_ref = GameObject_CreateRootRef(CID_GOTestNode);
    TempObjectReference other = ObjectContainer_TempFrom(other_ref);

    uint8_t result = SELF_DISPATCH(root, MID_GameObject_SELF_RemoveChild, {
        Payload_SetValue(msg, "child", TempObjectReference, other);
    }, {});

    ASSERT(result == MESSAGE_RESULT_NOT_FOUND);

    int *count = (int *)_Object_GetValueData(root->data->values, "child_count", 11);
    ASSERT(count != NULL);
    ASSERT(*count == 1);

    ObjectContainer_UnRef_External(&other_ref);
    ObjectContainer_UnRef_External(&rref);
    PASS();
}

// ============================================================
// Remove only child
// ============================================================

static void test_go_remove_only_child(void) {
    TEST("go: remove only child -> child_count=0, parent cleared");
    ExternalReference rref = GameObject_CreateRootRef(CID_GOTestNode);
    TempObjectReference root = ObjectContainer_TempFrom(rref);

    ExternalReference cref = GameObject_CreateChildRef(root, CID_GOTestNode);
    TempObjectReference child = ObjectContainer_TempFrom(cref);

    SELF_DISPATCH(root, MID_GameObject_SELF_RemoveChild, {
        Payload_SetValue(msg, "child", TempObjectReference, child);
    }, {});

    int *count = (int *)_Object_GetValueData(root->data->values, "child_count", 11);
    ASSERT(count != NULL);
    ASSERT(*count == 0);

    TempObjectReference parent = Object_SGetRef(child, "parent");
    ASSERT(parent == NULL);

    ObjectContainer_UnRef_External(&cref);
    ObjectContainer_UnRef_External(&rref);
    PASS();
}

// ============================================================
// Remove first child (re-indexing)
// ============================================================

static void test_go_remove_first_child(void) {
    TEST("go: remove first of 3 children, verify re-indexing");
    ExternalReference rref = GameObject_CreateRootRef(CID_GOTestNode);
    TempObjectReference root = ObjectContainer_TempFrom(rref);

    ExternalReference c1ref = GameObject_CreateChildRef(root, CID_GOTestNode);
    TempObjectReference c1 = ObjectContainer_TempFrom(c1ref);
    _Object_StoreValue(c1->data->values, "id", 2, &(int){10}, sizeof(int), CID_GOTestNode, SER_SKIP, 0);

    TempObjectReference c2 = _go_create_node(root, 20, 0);
    TempObjectReference c3 = _go_create_node(root, 30, 0);

    SELF_DISPATCH(root, MID_GameObject_SELF_RemoveChild, {
        Payload_SetValue(msg, "child", TempObjectReference, c1);
    }, {});

    int *count = (int *)_Object_GetValueData(root->data->values, "child_count", 11);
    ASSERT(*count == 2);

    char kbuf[_GO_CHILD_KEY_MAX];
    uint32_t klen = _go_child_key(kbuf, 0);
    TempObjectReference slot0 = Object_GetRef(root, kbuf, klen);
    ASSERT(slot0 == c2);

    klen = _go_child_key(kbuf, 1);
    TempObjectReference slot1 = Object_GetRef(root, kbuf, klen);
    ASSERT(slot1 == c3);

    ObjectContainer_UnRef_External(&c1ref);
    ObjectContainer_UnRef_External(&rref);
    PASS();
}

// ============================================================
// Remove last child
// ============================================================

static void test_go_remove_last_child(void) {
    TEST("go: remove last of 3 children, verify remaining");
    ExternalReference rref = GameObject_CreateRootRef(CID_GOTestNode);
    TempObjectReference root = ObjectContainer_TempFrom(rref);

    TempObjectReference c1 = _go_create_node(root, 10, 0);
    TempObjectReference c2 = _go_create_node(root, 20, 0);
    ExternalReference c3ref = GameObject_CreateChildRef(root, CID_GOTestNode);
    TempObjectReference c3 = ObjectContainer_TempFrom(c3ref);
    _Object_StoreValue(c3->data->values, "id", 2, &(int){30}, sizeof(int), CID_GOTestNode, SER_SKIP, 0);

    SELF_DISPATCH(root, MID_GameObject_SELF_RemoveChild, {
        Payload_SetValue(msg, "child", TempObjectReference, c3);
    }, {});

    int *count = (int *)_Object_GetValueData(root->data->values, "child_count", 11);
    ASSERT(*count == 2);

    char kbuf[_GO_CHILD_KEY_MAX];
    uint32_t klen = _go_child_key(kbuf, 0);
    TempObjectReference slot0 = Object_GetRef(root, kbuf, klen);
    ASSERT(slot0 == c1);

    klen = _go_child_key(kbuf, 1);
    TempObjectReference slot1 = Object_GetRef(root, kbuf, klen);
    ASSERT(slot1 == c2);

    ObjectContainer_UnRef_External(&c3ref);
    ObjectContainer_UnRef_External(&rref);
    PASS();
}

// ============================================================
// SetPriority on root (no parent) -- should not crash
// ============================================================

static void test_go_set_priority_no_parent(void) {
    TEST("go: SetPriority on root (no parent) succeeds");
    ExternalReference rref = GameObject_CreateRootRef(CID_GOTestNode);
    TempObjectReference root = ObjectContainer_TempFrom(rref);

    uint8_t result = SELF_DISPATCH(root, MID_GameObject_SELF_SetPriority, {
        Payload_SetValue(msg, "priority", int, 42);
    }, {});

    ASSERT(MESSAGE_RESULT_ISOK(result));

    int *pri = (int *)_Object_GetValueData(root->data->values, "priority", 8);
    ASSERT(pri != NULL);
    ASSERT(*pri == 42);

    ObjectContainer_UnRef_External(&rref);
    PASS();
}

// ============================================================
// SetPriority on child without external ref (guard ref test)
// ============================================================

static void test_go_set_priority_no_ext_ref(void) {
    TEST("go: SetPriority on child with no ext ref does not crash");
    ExternalReference rref = GameObject_CreateRootRef(CID_GOTestNode);
    TempObjectReference root = ObjectContainer_TempFrom(rref);

    TempObjectReference child = GameObject_CreateChild(root, CID_GOTestNode);
    ASSERT(child != NULL);
    _Object_StoreValue(child->data->values, "id", 2, &(int){5}, sizeof(int), CID_GOTestNode, SER_SKIP, 0);

    SELF_DISPATCH(child, MID_GameObject_SELF_SetPriority, {
        Payload_SetValue(msg, "priority", int, 30);
    }, {});

    // Verify child is still in parent's list
    int *count = (int *)_Object_GetValueData(root->data->values, "child_count", 11);
    ASSERT(count != NULL);
    ASSERT(*count == 1);

    char kbuf[_GO_CHILD_KEY_MAX];
    uint32_t klen = _go_child_key(kbuf, 0);
    TempObjectReference slot0 = Object_GetRef(root, kbuf, klen);
    ASSERT(slot0 == child);

    int *pri = (int *)_Object_GetValueData(child->data->values, "priority", 8);
    ASSERT(pri != NULL);
    ASSERT(*pri == 30);

    ObjectContainer_UnRef_External(&rref);
    PASS();
}

// ============================================================
// Consume at root -- children skipped
// ============================================================

static void test_go_consume_root(void) {
    TEST("go: root consumes -> children skipped");
    _go_exec_reset();
    ExternalReference rref = GameObject_CreateRootRef(CID_GOConsumer);
    TempObjectReference root = ObjectContainer_TempFrom(rref);
    _Object_StoreValue(root->data->values, "id", 2, &(int){1}, sizeof(int), CID_GOConsumer, SER_SKIP, 0);

    _go_create_node(root, 2, 0);
    _go_create_node(root, 3, 0);

    GAMEOBJECT_DISPATCH(root, MID_GameObject_SELF_Update, SPREAD_DOWN, {}, {});

    ASSERT(_go_exec_count == 1);
    ASSERT(_go_exec_log[0] == 1);

    ObjectContainer_UnRef_External(&rref);
    PASS();
}

// ============================================================
// Consume does not affect siblings
// ============================================================

static void test_go_consume_does_not_affect_siblings(void) {
    TEST("go: consumer child does not block sibling execution");
    _go_exec_reset();
    ExternalReference rref = GameObject_CreateRootRef(CID_GOTestNode);
    TempObjectReference root = ObjectContainer_TempFrom(rref);
    _Object_StoreValue(root->data->values, "id", 2, &(int){1}, sizeof(int), CID_GOTestNode, SER_SKIP, 0);

    // child_0: GOConsumer(id=2)
    TempObjectReference consumer = GameObject_CreateChild(root, CID_GOConsumer);
    _Object_StoreValue(consumer->data->values, "id", 2, &(int){2}, sizeof(int), CID_GOConsumer, SER_SKIP, 0);

    // child_1: GOTestNode(id=3)
    _go_create_node(root, 3, 0);
    // child_2: GOTestNode(id=4)
    _go_create_node(root, 4, 0);

    GAMEOBJECT_DISPATCH(root, MID_GameObject_SELF_Update, SPREAD_DOWN, {}, {});

    // root(1), consumer(2) consumes its subtree, but siblings 3 and 4 still run
    ASSERT(_go_exec_count == 4);
    ASSERT(_go_exec_log[0] == 1);
    ASSERT(_go_exec_log[1] == 2);
    ASSERT(_go_exec_log[2] == 3);
    ASSERT(_go_exec_log[3] == 4);

    ObjectContainer_UnRef_External(&rref);
    PASS();
}

// ============================================================
// Consume nested -- middle consumes, leaf skipped
// ============================================================

static void test_go_consume_nested(void) {
    TEST("go: nested consumer skips its subtree only");
    _go_exec_reset();
    ExternalReference rref = GameObject_CreateRootRef(CID_GOTestNode);
    TempObjectReference root = ObjectContainer_TempFrom(rref);
    _Object_StoreValue(root->data->values, "id", 2, &(int){1}, sizeof(int), CID_GOTestNode, SER_SKIP, 0);

    // middle: GOConsumer(id=2)
    TempObjectReference middle = GameObject_CreateChild(root, CID_GOConsumer);
    _Object_StoreValue(middle->data->values, "id", 2, &(int){2}, sizeof(int), CID_GOConsumer, SER_SKIP, 0);

    // leaf under middle: GOTestNode(id=3)
    _go_create_node(middle, 3, 0);

    GAMEOBJECT_DISPATCH(root, MID_GameObject_SELF_Update, SPREAD_DOWN, {}, {});

    // root(1) visited, middle(2) consumes -> leaf(3) skipped
    ASSERT(_go_exec_count == 2);
    ASSERT(_go_exec_log[0] == 1);
    ASSERT(_go_exec_log[1] == 2);

    ObjectContainer_UnRef_External(&rref);
    PASS();
}

// ============================================================
// SPREAD_UP deep tree
// ============================================================

static void test_go_spread_up_deep(void) {
    TEST("go: SPREAD_UP deep tree visits deepest first");
    _go_exec_reset();
    ExternalReference rref = GameObject_CreateRootRef(CID_GOTestNode);
    TempObjectReference root = ObjectContainer_TempFrom(rref);
    _Object_StoreValue(root->data->values, "id", 2, &(int){1}, sizeof(int), CID_GOTestNode, SER_SKIP, 0);

    TempObjectReference mid = _go_create_node(root, 2, 0);
    _go_create_node(mid, 3, 0);

    GAMEOBJECT_DISPATCH(root, MID_GameObject_SELF_Update, SPREAD_UP, {}, {});

    ASSERT(_go_exec_count == 3);
    ASSERT(_go_exec_log[0] == 3);
    ASSERT(_go_exec_log[1] == 2);
    ASSERT(_go_exec_log[2] == 1);

    ObjectContainer_UnRef_External(&rref);
    PASS();
}

// ============================================================
// SPREAD_UP: consume has no effect on siblings
// ============================================================

static void test_go_spread_up_consume_no_effect(void) {
    TEST("go: SPREAD_CONSUME in SPREAD_UP does not prevent siblings");
    _go_exec_reset();
    ExternalReference rref = GameObject_CreateRootRef(CID_GOTestNode);
    TempObjectReference root = ObjectContainer_TempFrom(rref);
    _Object_StoreValue(root->data->values, "id", 2, &(int){1}, sizeof(int), CID_GOTestNode, SER_SKIP, 0);

    // child_0: GOConsumer(id=2)
    TempObjectReference consumer = GameObject_CreateChild(root, CID_GOConsumer);
    _Object_StoreValue(consumer->data->values, "id", 2, &(int){2}, sizeof(int), CID_GOConsumer, SER_SKIP, 0);

    // child_1: GOTestNode(id=3)
    _go_create_node(root, 3, 0);

    GAMEOBJECT_DISPATCH(root, MID_GameObject_SELF_Update, SPREAD_UP, {}, {});

    // UP: children first (2, 3), then root (1). Consumer's consume has no effect.
    ASSERT(_go_exec_count == 3);
    ASSERT(_go_exec_log[0] == 2);
    ASSERT(_go_exec_log[1] == 3);
    ASSERT(_go_exec_log[2] == 1);

    ObjectContainer_UnRef_External(&rref);
    PASS();
}

// ============================================================
// Add child during spread (snapshot safety)
// ============================================================

static void test_go_add_child_during_spread(void) {
    TEST("go: child added during spread not visited (snapshot safety)");
    _go_exec_reset();
    ExternalReference rref = GameObject_CreateRootRef(CID_GOTestNode);
    TempObjectReference root = ObjectContainer_TempFrom(rref);
    _Object_StoreValue(root->data->values, "id", 2, &(int){1}, sizeof(int), CID_GOTestNode, SER_SKIP, 0);

    // child_0: GOAdder(id=2) -- will add a child(999) during its Update
    TempObjectReference adder = GameObject_CreateChild(root, CID_GOAdder);
    _Object_StoreValue(adder->data->values, "id", 2, &(int){2}, sizeof(int), CID_GOAdder, SER_SKIP, 0);

    // child_1: GOTestNode(id=3)
    _go_create_node(root, 3, 0);

    GAMEOBJECT_DISPATCH(root, MID_GameObject_SELF_Update, SPREAD_DOWN, {}, {});

    // Snapshot was taken before adder added child 999, so 999 should NOT be in the log
    ASSERT(_go_exec_count == 3);
    ASSERT(_go_exec_log[0] == 1);
    ASSERT(_go_exec_log[1] == 2);
    ASSERT(_go_exec_log[2] == 3);

    // Verify the new child exists on the adder
    int *adder_count = (int *)_Object_GetValueData(adder->data->values, "child_count", 11);
    ASSERT(adder_count != NULL);
    ASSERT(*adder_count == 1);

    // Verify the new child has id 999
    char kbuf[_GO_CHILD_KEY_MAX];
    uint32_t klen = _go_child_key(kbuf, 0);
    TempObjectReference new_child = Object_GetRef(adder, kbuf, klen);
    ASSERT(new_child != NULL);
    int *new_id = (int *)_Object_GetValueData(new_child->data->values, "id", 2);
    ASSERT(new_id != NULL);
    ASSERT(*new_id == 999);

    ObjectContainer_UnRef_External(&rref);
    PASS();
}

// ============================================================
// Ref counting: dropping root ext ref cleans up tree
// ============================================================

static void test_go_child_ref_count(void) {
    TEST("go: children held by parent internal refs, cleanup on root drop");
    ExternalReference rref = GameObject_CreateRootRef(CID_GOTestNode);
    TempObjectReference root = ObjectContainer_TempFrom(rref);

    TempObjectReference c1 = GameObject_CreateChild(root, CID_GOTestNode);
    TempObjectReference c2 = GameObject_CreateChild(root, CID_GOTestNode);
    ASSERT(c1 != NULL);
    ASSERT(c2 != NULL);

    // Children should have internal_refs > 0 (held by parent)
    ASSERT(c1->internal_refs > 0);
    ASSERT(c2->internal_refs > 0);

    // Drop root ext ref -- GC should collect the tree
    ObjectContainer_UnRef_External(&rref);
    ASSERT(rref == NULL);

    PASS();
}

// ============================================================
// CreateChildRef -- ext ref works, unref keeps child alive via parent
// ============================================================

static void test_go_create_child_ref_cleanup(void) {
    TEST("go: CreateChildRef ext ref works, child survives unref");
    ExternalReference rref = GameObject_CreateRootRef(CID_GOTestNode);
    TempObjectReference root = ObjectContainer_TempFrom(rref);

    ExternalReference cref = GameObject_CreateChildRef(root, CID_GOTestNode);
    ASSERT(cref != NULL);
    TempObjectReference child = ObjectContainer_TempFrom(cref);
    ASSERT(child != NULL);
    ASSERT(child->data != NULL);

    // Unref external -- child should survive via parent's internal ref
    ObjectContainer_UnRef_External(&cref);
    ASSERT(cref == NULL);

    // Child should still be accessible from parent
    char kbuf[_GO_CHILD_KEY_MAX];
    uint32_t klen = _go_child_key(kbuf, 0);
    TempObjectReference slot0 = Object_GetRef(root, kbuf, klen);
    ASSERT(slot0 == child);
    ASSERT(slot0->data != NULL);

    ObjectContainer_UnRef_External(&rref);
    PASS();
}

// ============================================================
// Multiple spreads accumulate in exec_log
// ============================================================

static void test_go_multiple_updates(void) {
    TEST("go: multiple Update spreads accumulate correctly");
    _go_exec_reset();
    ExternalReference rref = GameObject_CreateRootRef(CID_GOTestNode);
    TempObjectReference root = ObjectContainer_TempFrom(rref);
    _Object_StoreValue(root->data->values, "id", 2, &(int){1}, sizeof(int), CID_GOTestNode, SER_SKIP, 0);

    _go_create_node(root, 2, 0);

    GAMEOBJECT_DISPATCH(root, MID_GameObject_SELF_Update, SPREAD_DOWN, {}, {});
    GAMEOBJECT_DISPATCH(root, MID_GameObject_SELF_Update, SPREAD_DOWN, {}, {});
    GAMEOBJECT_DISPATCH(root, MID_GameObject_SELF_Update, SPREAD_DOWN, {}, {});

    ASSERT(_go_exec_count == 6);
    // Each spread: 1, 2
    ASSERT(_go_exec_log[0] == 1);
    ASSERT(_go_exec_log[1] == 2);
    ASSERT(_go_exec_log[2] == 1);
    ASSERT(_go_exec_log[3] == 2);
    ASSERT(_go_exec_log[4] == 1);
    ASSERT(_go_exec_log[5] == 2);

    ObjectContainer_UnRef_External(&rref);
    PASS();
}

// ============================================================
// Large child count (100 children)
// ============================================================

static void test_go_many_children(void) {
    TEST("go: 100 children all visited in spread");
    _go_exec_reset();
    ExternalReference rref = GameObject_CreateRootRef(CID_GOTestNode);
    TempObjectReference root = ObjectContainer_TempFrom(rref);
    _Object_StoreValue(root->data->values, "id", 2, &(int){0}, sizeof(int), CID_GOTestNode, SER_SKIP, 0);

    for (int i = 1; i <= 100; i++) {
        _go_create_node(root, i, 0);
    }

    GAMEOBJECT_DISPATCH(root, MID_GameObject_SELF_Update, SPREAD_DOWN, {}, {});

    ASSERT(_go_exec_count == 101);
    ASSERT(_go_exec_log[0] == 0); // root
    for (int i = 1; i <= 100; i++) {
        ASSERT(_go_exec_log[i] == i);
    }

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

    // New comprehensive tests
    test_go_spread_empty_tree();
    test_go_spread_single_child();
    test_go_remove_nonexistent_child();
    test_go_remove_only_child();
    test_go_remove_first_child();
    test_go_remove_last_child();
    test_go_set_priority_no_parent();
    test_go_set_priority_no_ext_ref();
    test_go_consume_root();
    test_go_consume_does_not_affect_siblings();
    test_go_consume_nested();
    test_go_spread_up_deep();
    test_go_spread_up_consume_no_effect();
    test_go_add_child_during_spread();
    test_go_child_ref_count();
    test_go_create_child_ref_cleanup();
    test_go_multiple_updates();
    test_go_many_children();
}

#undef LINTNORE
