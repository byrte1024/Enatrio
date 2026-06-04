#pragma once

#include "../system/tests.h"
#include "../system/object/Self.h"

// Tests deliberately manipulate refs and registration order.
#define LINTNORE

// ============================================================
// Test class: Animal (CID 0x1001) -- inherits Object
// Has "legs" value (default 4), declares Speak MID
// ============================================================

#define TYPE Animal

BEGIN_CLASS(0x1001);
INHERITS(Object);

DECLARE_MID(Speak, 0x01);

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Create)
    CALL_BASE();
    Self_SetValue("legs", int, 4);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Destroy)
    CALL_BASE();
MESSAGE_HANDLER_END()

MESSAGE_HANDLER_BEGIN(Speak)
    MH_SetValue(sound, int, 0);
MESSAGE_HANDLER_END()

CAN_RECEIVE_BEGIN()
    SELF_CAN_RECEIVE_MID_EXTERN(Object, Create)
    SELF_CAN_RECEIVE_MID_EXTERN(Object, Destroy)
    CAN_RECEIVE_MID(Speak)
CAN_RECEIVE_END()

RECEIVE_MESSAGE_BEGIN()
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Object, Create)
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Object, Destroy)
    RECEIVE_MESSAGE_ROUTE(Speak)
RECEIVE_MESSAGE_END()

CLASSDEF_INHERITS(Object)

#undef TYPE

// ============================================================
// Test class: Dog (CID 0x1002) -- inherits Animal
// Overrides Create (CALL_BASE, adds "breed"=1)
// Overrides Speak (IGNORE_BASE, returns sound=1)
// Overrides Destroy (CALL_BASE)
// ============================================================

#define TYPE Dog

BEGIN_CLASS(0x1002);
INHERITS(Animal);

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Create)
    CALL_BASE();
    Self_SetValue("breed", int, 1);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Destroy)
    CALL_BASE();
MESSAGE_HANDLER_END()

MESSAGE_HANDLER_BEGIN_EXTERN(Animal, Speak)
    IGNORE_BASE();
    MH_SetValue(sound, int, 1);
MESSAGE_HANDLER_END()

CAN_RECEIVE_BEGIN()
    SELF_CAN_RECEIVE_MID_EXTERN(Object, Create)
    SELF_CAN_RECEIVE_MID_EXTERN(Object, Destroy)
    CAN_RECEIVE_MID_EXTERN(Animal, Speak)
CAN_RECEIVE_END()

RECEIVE_MESSAGE_BEGIN()
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Object, Create)
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Object, Destroy)
    RECEIVE_MESSAGE_ROUTE_EXTERN(Animal, Speak)
RECEIVE_MESSAGE_END()

CLASSDEF_INHERITS(Animal)

#undef TYPE

// ============================================================
// Test class: Cat (CID 0x1003) -- inherits Animal
// Overrides Create (CALL_BASE, adds "indoor"=1)
// Overrides Destroy (CALL_BASE)
// Does NOT override Speak (inherits from Animal)
// ============================================================

#define TYPE Cat

BEGIN_CLASS(0x1003);
INHERITS(Animal);

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Create)
    CALL_BASE();
    Self_SetValue("indoor", int, 1);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Destroy)
    CALL_BASE();
MESSAGE_HANDLER_END()

CAN_RECEIVE_BEGIN()
    SELF_CAN_RECEIVE_MID_EXTERN(Object, Create)
    SELF_CAN_RECEIVE_MID_EXTERN(Object, Destroy)
CAN_RECEIVE_END()

RECEIVE_MESSAGE_BEGIN()
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Object, Create)
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Object, Destroy)
RECEIVE_MESSAGE_END()

CLASSDEF_INHERITS(Animal)

#undef TYPE

// ============================================================
// Test class: Plant (CID 0x1004) -- inherits Object directly
// Create sets "leaves"=10
// ============================================================

#define TYPE Plant

BEGIN_CLASS(0x1004);
INHERITS(Object);

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Create)
    CALL_BASE();
    Self_SetValue("leaves", int, 10);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Destroy)
    CALL_BASE();
MESSAGE_HANDLER_END()

CAN_RECEIVE_BEGIN()
    SELF_CAN_RECEIVE_MID_EXTERN(Object, Create)
    SELF_CAN_RECEIVE_MID_EXTERN(Object, Destroy)
CAN_RECEIVE_END()

RECEIVE_MESSAGE_BEGIN()
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Object, Create)
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Object, Destroy)
RECEIVE_MESSAGE_END()

CLASSDEF_INHERITS(Object)

#undef TYPE

// ============================================================
// Minimal no-parent class for testing
// ============================================================

#define TYPE InhTestNoParent
BEGIN_CLASS(0xF001);
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

// ============================================================
// Registration helper
// ============================================================

static void _inherit_register_all(void) {
    BeginClassRegistrations();
    RegisterClass(Object_ClassDef());
    RegisterClass(Animal_ClassDef());
    RegisterClass(Dog_ClassDef());
    RegisterClass(Cat_ClassDef());
    RegisterClass(Plant_ClassDef());
    EndClassRegistrations();
}

// ============================================================
// Tests
// ============================================================

static void test_inherit_animal_has_legs(void) {
    TEST("inherit: Animal has legs=4");
    TempObjectReference obj = Object_Create(CID_Animal);
    ASSERT(obj != NULL);
    ASSERT(obj->data != NULL);
    int *legs = (int*)_Object_GetValueData(obj->data->values, "legs", 4);
    ASSERT(legs != NULL);
    ASSERT(*legs == 4);
    Object_Destroy(obj);
    PASS();
}

static void test_inherit_dog_has_legs_and_breed(void) {
    TEST("inherit: Dog has legs=4 and breed=1 (CALL_BASE chained)");
    TempObjectReference obj = Object_Create(CID_Dog);
    ASSERT(obj != NULL);
    ASSERT(obj->data != NULL);
    int *legs = (int*)_Object_GetValueData(obj->data->values, "legs", 4);
    ASSERT(legs != NULL);
    ASSERT(*legs == 4);
    int *breed = (int*)_Object_GetValueData(obj->data->values, "breed", 5);
    ASSERT(breed != NULL);
    ASSERT(*breed == 1);
    Object_Destroy(obj);
    PASS();
}

static void test_inherit_dog_overrides_speak(void) {
    TEST("inherit: Dog overrides Speak, sound=1");
    TempObjectReference obj = Object_Create(CID_Dog);
    ASSERT(obj != NULL);
    MessagePayload p = PreparePayload(CID_Dog, MID_Animal_Speak);
    ASSERT(p.data != NULL);
    DispatchMessage(&p);
    ASSERT(MESSAGE_RESULT_ISOK(p.result));
    int *sound = (int*)Payload_Get(&p, "sound");
    ASSERT(sound != NULL);
    ASSERT(*sound == 1);
    FreePayload(&p);
    Object_Destroy(obj);
    PASS();
}

static void test_inherit_cat_inherits_speak_from_animal(void) {
    TEST("inherit: Cat inherits Speak from Animal, sound=0");
    TempObjectReference obj = Object_Create(CID_Cat);
    ASSERT(obj != NULL);
    MessagePayload p = PreparePayload(CID_Cat, MID_Animal_Speak);
    ASSERT(p.data != NULL);
    DispatchMessage(&p);
    ASSERT(MESSAGE_RESULT_ISOK(p.result));
    int *sound = (int*)Payload_Get(&p, "sound");
    ASSERT(sound != NULL);
    ASSERT(*sound == 0);
    FreePayload(&p);
    Object_Destroy(obj);
    PASS();
}

static void test_inherit_plant_independent(void) {
    TEST("inherit: Plant has leaves=10");
    TempObjectReference obj = Object_Create(CID_Plant);
    ASSERT(obj != NULL);
    ASSERT(obj->data != NULL);
    int *leaves = (int*)_Object_GetValueData(obj->data->values, "leaves", 6);
    ASSERT(leaves != NULL);
    ASSERT(*leaves == 10);
    Object_Destroy(obj);
    PASS();
}

static void test_inherit_registration_order(void) {
    TEST("inherit: registering Dog before Animal fails, then correct order succeeds");
    // Try wrong order: Dog before Animal
    BeginClassRegistrations();
    RegisterClass(Object_ClassDef());
    RegisterClass(Dog_ClassDef()); // should fail -- Animal not registered yet
    ASSERT(!CLASSID_ISREGISTERED(CID_Dog));
    EndClassRegistrations();

    // Correct order
    BeginClassRegistrations();
    RegisterClass(Object_ClassDef());
    RegisterClass(Animal_ClassDef());
    RegisterClass(Dog_ClassDef());
    ASSERT(CLASSID_ISREGISTERED(CID_Dog));
    EndClassRegistrations();

    // Re-register all for remaining tests
    _inherit_register_all();
    PASS();
}

static void test_inherit_dispatch_walks_chain(void) {
    TEST("inherit: CanDispatchMessage walks chain for Dog");
    // Dog should handle Object's Create, Animal's Speak, Object's Destroy
    ASSERT(CanDispatchMessage(MID_Object_SELF_Create, CID_Dog));
    ASSERT(CanDispatchMessage(MID_Animal_Speak, CID_Dog));
    ASSERT(CanDispatchMessage(MID_Object_SELF_Destroy, CID_Dog));
    PASS();
}

static void test_inherit_no_parent_class(void) {
    TEST("inherit: no-parent class handles own MIDs but not Object's");
    BeginClassRegistrations();
    RegisterClass(Object_ClassDef());
    RegisterClass(InhTestNoParent_ClassDef());
    EndClassRegistrations();

    ASSERT(CanDispatchMessage(MID_InhTestNoParent_Poke, CID_InhTestNoParent));
    ASSERT(!CanDispatchMessage(MID_Object_SELF_Create, CID_InhTestNoParent));
    ASSERT(!CanDispatchMessage(MID_Object_SELF_Destroy, CID_InhTestNoParent));

    // Re-register all for remaining tests
    _inherit_register_all();
    PASS();
}

static void test_inherit_deep_chain_3_levels(void) {
    TEST("inherit: Dog 3-level chain (Dog->Animal->Object), legs and breed exist");
    TempObjectReference obj = Object_Create(CID_Dog);
    ASSERT(obj != NULL);
    int *legs = (int*)_Object_GetValueData(obj->data->values, "legs", 4);
    ASSERT(legs != NULL);
    ASSERT(*legs == 4);
    int *breed = (int*)_Object_GetValueData(obj->data->values, "breed", 5);
    ASSERT(breed != NULL);
    ASSERT(*breed == 1);
    Object_Destroy(obj);
    PASS();
}

static void test_inherit_destroy_chains(void) {
    TEST("inherit: Dog ref create and unref, no crash");
    ExternalReference ref = Object_CreateRef(CID_Dog);
    ASSERT(ref != NULL);
    ASSERT(ref->external_refs == 1);
    ObjectContainer_UnRef_External(&ref);
    ASSERT(ref == NULL);
    PASS();
}

static void test_inherit_cat_has_indoor(void) {
    TEST("inherit: Cat has legs=4 and indoor=1");
    TempObjectReference obj = Object_Create(CID_Cat);
    ASSERT(obj != NULL);
    ASSERT(obj->data != NULL);
    int *legs = (int*)_Object_GetValueData(obj->data->values, "legs", 4);
    ASSERT(legs != NULL);
    ASSERT(*legs == 4);
    int *indoor = (int*)_Object_GetValueData(obj->data->values, "indoor", 6);
    ASSERT(indoor != NULL);
    ASSERT(*indoor == 1);
    Object_Destroy(obj);
    PASS();
}

// ============================================================
// Runner
// ============================================================

static void run_inheritance_tests(void) {
    _inherit_register_all();

    LOG_INFO("=== Inheritance Tests ===");
    test_inherit_animal_has_legs();
    test_inherit_dog_has_legs_and_breed();
    test_inherit_dog_overrides_speak();
    test_inherit_cat_inherits_speak_from_animal();
    test_inherit_plant_independent();
    test_inherit_registration_order();
    test_inherit_dispatch_walks_chain();
    test_inherit_no_parent_class();
    test_inherit_deep_chain_3_levels();
    test_inherit_destroy_chains();
    test_inherit_cat_has_indoor();
}

#undef LINTNORE
