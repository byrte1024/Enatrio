#pragma once

#include <stdint.h>
#include <stdio.h>

#include "../cts/UnsafeDictionary.h"
#include "../cts/UnsafeHashMap.h"

// uint16_t bounds the registration table size -- 65535 classes is plenty
// for any realistic project, and keeps ClassDefinitions array small.
typedef uint16_t ClassID;
#define CLASSID_MAX UINT16_MAX
#define CLASS_MAXNAMELENGTH 64
#define CID_Untyped ((ClassID)(0))

// Fixed char[64] instead of a pointer -- enables stack allocation and
// memcpy without heap, avoids lifetime issues with dynamic strings.
typedef char MessageID[64];
#define MESSAGEID_EMPTY ((const char[64]) {0})

typedef struct MessagePayload {
    MessageID mid;
    ClassID cid_target;
    UnsafeVariedHashMap* data;
    uint8_t result;

} MessagePayload;

#define MESSAGE_RESULT_ISOK(r) ((r) == MESSAGE_RESULT_SUCCESS)

#define MESSAGE_RESULT_SUCCESS ((uint8_t)(0))
#define MESSAGE_RESULT_OOM ((uint8_t)(1))
#define MESSAGE_RESULT_MISSING_PARAMS ((uint8_t)(2))
#define MESSAGE_RESULT_INVALID_PARAMS ((uint8_t)(3))
#define MESSAGE_RESULT_UNKNOWN_ERROR ((uint8_t)(4))
#define MESSAGE_RESULT_INVALID_CID ((uint8_t)(5))
#define MESSAGE_RESULT_INVALID_MID ((uint8_t)(6))
#define MESSAGE_RESULT_NO_PAYLOAD ((uint8_t)(7))
#define MESSAGE_RESULT_INTERNAL_ERROR ((uint8_t)(8))
#define MESSAGE_RESULT_NOT_SUPPORTED ((uint8_t)(9))
#define MESSAGE_RESULT_BUSY ((uint8_t)(10))
#define MESSAGE_RESULT_TIMEOUT ((uint8_t)(11))
#define MESSAGE_RESULT_DENIED ((uint8_t)(12))
#define MESSAGE_RESULT_DUPLICATE ((uint8_t)(13))
#define MESSAGE_RESULT_NOT_FOUND ((uint8_t)(14))
#define MESSAGE_RESULT_OVERFLOW ((uint8_t)(15))
#define MESSAGE_RESULT_NOT_READY ((uint8_t)(16))
#define MESSAGE_RESULT_CANCELLED ((uint8_t)(17))
#define MESSAGE_RESULT_PENDING ((uint8_t)(18))
#define MESSAGE_RESULT_IGNORED ((uint8_t)(19))
#define MESSAGE_RESULT_NOTSENT ((uint8_t)(20))
#define MESSAGE_RESULT_INVALID_SELF ((uint8_t)(21))

#define MESSAGE_RESULT_NAME(r) ( \
    (r) == MESSAGE_RESULT_SUCCESS        ? "SUCCESS" : \
    (r) == MESSAGE_RESULT_OOM            ? "OOM" : \
    (r) == MESSAGE_RESULT_MISSING_PARAMS ? "MISSING_PARAMS" : \
    (r) == MESSAGE_RESULT_INVALID_PARAMS ? "INVALID_PARAMS" : \
    (r) == MESSAGE_RESULT_UNKNOWN_ERROR  ? "UNKNOWN_ERROR" : \
    (r) == MESSAGE_RESULT_INVALID_CID    ? "INVALID_CID" : \
    (r) == MESSAGE_RESULT_INVALID_MID    ? "INVALID_MID" : \
    (r) == MESSAGE_RESULT_NO_PAYLOAD     ? "NO_PAYLOAD" : \
    (r) == MESSAGE_RESULT_INTERNAL_ERROR ? "INTERNAL_ERROR" : \
    (r) == MESSAGE_RESULT_NOT_SUPPORTED  ? "NOT_SUPPORTED" : \
    (r) == MESSAGE_RESULT_BUSY           ? "BUSY" : \
    (r) == MESSAGE_RESULT_TIMEOUT        ? "TIMEOUT" : \
    (r) == MESSAGE_RESULT_DENIED         ? "DENIED" : \
    (r) == MESSAGE_RESULT_DUPLICATE      ? "DUPLICATE" : \
    (r) == MESSAGE_RESULT_NOT_FOUND      ? "NOT_FOUND" : \
    (r) == MESSAGE_RESULT_OVERFLOW       ? "OVERFLOW" : \
    (r) == MESSAGE_RESULT_NOT_READY      ? "NOT_READY" : \
    (r) == MESSAGE_RESULT_CANCELLED      ? "CANCELLED" : \
    (r) == MESSAGE_RESULT_PENDING        ? "PENDING" : \
    (r) == MESSAGE_RESULT_IGNORED       ? "IGNORED" : \
    (r) == MESSAGE_RESULT_NOTSENT       ? "NOTSENT" : \
    (r) == MESSAGE_RESULT_INVALID_SELF  ? "INVALID_SELF" : \
    "UNDEFINED")

#define MESSAGE_RESULT_DESC(r) ( \
    (r) == MESSAGE_RESULT_SUCCESS        ? "Operation completed successfully" : \
    (r) == MESSAGE_RESULT_OOM            ? "Out of memory" : \
    (r) == MESSAGE_RESULT_MISSING_PARAMS ? "Required parameters are missing" : \
    (r) == MESSAGE_RESULT_INVALID_PARAMS ? "Parameters are invalid or out of range" : \
    (r) == MESSAGE_RESULT_UNKNOWN_ERROR  ? "An unknown error occurred" : \
    (r) == MESSAGE_RESULT_INVALID_CID    ? "Target class ID does not exist" : \
    (r) == MESSAGE_RESULT_INVALID_MID    ? "Message ID is not recognized" : \
    (r) == MESSAGE_RESULT_NO_PAYLOAD     ? "Message payload is NULL" : \
    (r) == MESSAGE_RESULT_INTERNAL_ERROR ? "Internal processing error" : \
    (r) == MESSAGE_RESULT_NOT_SUPPORTED  ? "Target does not support this message" : \
    (r) == MESSAGE_RESULT_BUSY           ? "Target is busy and cannot process now" : \
    (r) == MESSAGE_RESULT_TIMEOUT        ? "Operation timed out" : \
    (r) == MESSAGE_RESULT_DENIED         ? "Operation was denied by the target" : \
    (r) == MESSAGE_RESULT_DUPLICATE      ? "A duplicate entry already exists" : \
    (r) == MESSAGE_RESULT_NOT_FOUND      ? "Requested resource was not found" : \
    (r) == MESSAGE_RESULT_OVERFLOW       ? "Data exceeded maximum capacity" : \
    (r) == MESSAGE_RESULT_NOT_READY      ? "Target is not yet initialized" : \
    (r) == MESSAGE_RESULT_CANCELLED      ? "Operation was cancelled" : \
    (r) == MESSAGE_RESULT_PENDING        ? "Message has not yet been acknowledged" : \
    (r) == MESSAGE_RESULT_IGNORED       ? "Message was never acknowledged" : \
    (r) == MESSAGE_RESULT_NOTSENT       ? "Message has not been sent" : \
    (r) == MESSAGE_RESULT_INVALID_SELF  ? "Invalid 'Self' object" : \
    "Undefined result code")


typedef struct ClassDefinition {

    ClassID cid;
    char classname[CLASS_MAXNAMELENGTH];
    ClassID parent_cid;

    bool (*CanReceiveMID)(MessageID mid);
    void (*ReceiveMessage)(MessagePayload* payload);


} ClassDefinition;


inline ClassDefinition ClassDefinitions[CLASSID_MAX + 1] = { 0 };

// Gate prevents late registration from corrupting running dispatch --
// once classes are locked in, the dispatch table is stable and safe to
// call from any context without synchronization.
inline bool ClassRegistrationsOpen = false;

#define CLASSID_ISUNTYPED(c) (c) == ((uint16_t)(0x0000))
#define CLASSID_ISREGISTERED(c) (ClassDefinitions[(c)].cid == c)

#define CLASSID_TOSTRING(c) (ClassDefinitions[(c)].classname)
#define CLASSID_TODEF(c) (ClassDefinitions[(c)])

static void BeginClassRegistrations() {
    ClassRegistrationsOpen = true;
    memset(ClassDefinitions, 0, sizeof(ClassDefinitions));

    LOG_INFO("========================================");
    LOG_INFO("Class system cleared and initialized");
    LOG_INFO("Begin registration now:");

}

static void RegisterClass(ClassDefinition def) {
    if (!ClassRegistrationsOpen) {
        LOG_ERROR("Class system is not open for registrations.");
        return;
    }
    if (CLASSID_ISREGISTERED((def.cid))) {
        LOG_ERROR("Class ID %d is already registered.", def.cid);
        return;
    }
    if (CLASSID_ISUNTYPED((def.cid))) {
        LOG_ERROR("Class ID %d is reserved for untyped classes.", def.cid);
        return;
    }

    if(def.CanReceiveMID == NULL) {
        LOG_ERROR("Class %s must implement CanReceiveMID.", def.classname);
        return;
    }
    if(def.ReceiveMessage == NULL) {
        LOG_ERROR("Class %s must implement ReceiveMessage.", def.classname);
        return;
    }

    for(int i = 0; i < CLASSID_MAX + 1; i++) {
        if(strcmp(ClassDefinitions[i].classname, def.classname) == 0) {
            LOG_ERROR("Class name %s is already registered.", def.classname);
            return;
        }
    }

    // Parent validation
    if (!CLASSID_ISUNTYPED(def.parent_cid)) {
        if (!CLASSID_ISREGISTERED(def.parent_cid)) {
            LOG_ERROR("Class %s: parent CID %d is not registered. Register parents first.",
                def.classname, def.parent_cid);
            return;
        }
        ClassID walk = def.parent_cid;
        while (!CLASSID_ISUNTYPED(walk)) {
            if (walk == def.cid) {
                LOG_ERROR("Class %s: circular inheritance detected.", def.classname);
                return;
            }
            walk = ClassDefinitions[walk].parent_cid;
        }
    }

    ClassDefinitions[def.cid] = def;
    LOG_INFO("Class %s registered with ID %d.", def.classname, def.cid);
}

static void EndClassRegistrations() {
    ClassRegistrationsOpen = false;

    LOG_INFO("Class system registration completed.");
    LOG_INFO("========================================");
}

static inline bool CanDispatchMessage(MessageID mid, ClassID cid){
    ClassID walk = cid;
    while (!CLASSID_ISUNTYPED(walk)) {
        if (!CLASSID_ISREGISTERED(walk)) return false;
        if (ClassDefinitions[walk].CanReceiveMID(mid)) return true;
        walk = ClassDefinitions[walk].parent_cid;
    }
    return false;
}

static inline MessagePayload* DispatchMessage(MessagePayload* payload) {
    if (payload == NULL) {
        LOG_ERROR("Cannot dispatch NULL payload.");
        return NULL;
    }
    if (payload->mid[0] == '\0') {
        LOG_ERROR("Message ID is empty, cannot dispatch.");
        payload->result = MESSAGE_RESULT_INVALID_MID;
        return payload;
    }
    if(CLASSID_ISUNTYPED(payload->cid_target)){
        LOG_ERROR("Untyped class ID %d cannot receive messages.", payload->cid_target);
        payload->result = MESSAGE_RESULT_INVALID_CID;
        return payload;
    }
    if(!CLASSID_ISREGISTERED(payload->cid_target)){
        LOG_ERROR("Class ID %d is not registered.", payload->cid_target);
        payload->result = MESSAGE_RESULT_INVALID_CID;
        return payload;
    }
    if (payload->data == NULL) {
        LOG_ERROR("Payload data is NULL (allocation failed?).");
        payload->result = MESSAGE_RESULT_NO_PAYLOAD;
        return payload;
    }

    // Walk the inheritance chain to find the first class that handles
    // this message. If no class in the chain supports it, return NOT_SUPPORTED.
    {
        ClassID walk = payload->cid_target;
        bool handled = false;
        while (!CLASSID_ISUNTYPED(walk)) {
            if (!CLASSID_ISREGISTERED(walk)) break;
            if (ClassDefinitions[walk].CanReceiveMID(payload->mid)) {
                payload->result = MESSAGE_RESULT_PENDING;
                ClassDefinitions[walk].ReceiveMessage(payload);
                handled = true;
                break;
            }
            walk = ClassDefinitions[walk].parent_cid;
        }
        if (!handled) {
            LOG_ERROR("Class %d does not support message ID %s.", payload->cid_target, payload->mid);
            payload->result = MESSAGE_RESULT_NOT_SUPPORTED;
            return payload;
        }
    }

    // Check for PENDING after handler returns -- catches handlers that
    // forgot to set a result code, which would silently swallow errors.
    if(payload->result == MESSAGE_RESULT_PENDING){
        LOG_ERROR("Class %d did not acknowledge message %s.", payload->cid_target, payload->mid);
        payload->result = MESSAGE_RESULT_IGNORED;
    }

    if(!MESSAGE_RESULT_ISOK(payload->result)){
        LOG_ERROR("Class %d returned result code %s : %s", payload->cid_target, MESSAGE_RESULT_NAME(payload->result), MESSAGE_RESULT_DESC(payload->result));
    }

    return payload;
}

static inline MessagePayload PreparePayload(ClassID cid_target, MessageID mid) {
    MessagePayload payload = {0};

    payload.cid_target = cid_target;
    memcpy(payload.mid, mid, sizeof(MessageID));
    payload.result = MESSAGE_RESULT_NOTSENT;

    payload.data = UnsafeVariedHashMap_Create(8);
    if (payload.data == NULL) {
        LOG_ERROR("Failed to allocate payload data map.");
        return payload;
    }

    return payload;
}

// Stores raw bytes into the payload (create or update).
//   Payload_Set(payload, "data", &my_struct, sizeof(MyStruct));
#define Payload_Set(payload, str_key, value_ptr, value_size) \
    UnsafeVariedHashMap_SUpsert((payload)->data, str_key, value_ptr, value_size)

// Stores a typed value (create or update).
//   Payload_SetValue(payload, "health", int, 100);
#define Payload_SetValue(payload, str_key, type, value) \
    Payload_Set(payload, str_key, &(type){value}, sizeof(type))

// Returns a void* to the stored data, or NULL if not found.
//   int *hp = (int*)Payload_Get(payload, "health");
#define Payload_Get(payload, str_key) \
    UnsafeVariedHashMap_SGet((payload)->data, str_key)

// Dereferences the stored data as the given type.
//   int hp = Payload_GetDeref(payload, "health", int);
#define Payload_GetDeref(payload, str_key, type) \
    UnsafeVariedHashMap_SGetDeref((payload)->data, str_key, type)

// Returns 1 if the key exists, 0 otherwise.
//   if (Payload_Has(payload, "health")) { ... }
#define Payload_Has(payload, str_key) \
    UnsafeVariedHashMap_SHas((payload)->data, str_key)

// Returns the byte size of the stored value, or 0 if not found.
//   uint32_t sz = Payload_GetSize(payload, "health");
#define Payload_GetSize(payload, str_key) \
    UnsafeVariedHashMap_SGetSize((payload)->data, str_key)

// Removes a key from the payload. Returns 0 on success, -1 if not found.
//   Payload_Remove(payload, "health");
#define Payload_Remove(payload, str_key) \
    UnsafeVariedHashMap_SRemove((payload)->data, str_key)

// Creates a stack-local variable and stores a pointer to it in the payload.
// The variable lives until the enclosing scope ends.
//   Payload_SetLocalValue(payload, "out", int, 0);
#define Payload_SetLocalValue(payload, str_key, type, value) \
    type BAT2(_pslp_, __LINE__) = (value); \
    Payload_SetValue(payload, str_key, void*, &BAT2(_pslp_, __LINE__))

// ============================================================
// Class definition macros
// ============================================================
#define LINTNORE

//   BEGIN_CLASS(Exploder, 0x22AB)
// Expands to:
//   inline const ClassID CID_Exploder = (ClassID)(0x22AB);
//   inline const char CLASSNAME_Exploder[CLASS_MAXNAMELENGTH] = "Exploder";
// Enum enumerators are TU-scoped, so a duplicate ID causes a redefinition error at compile time.
#define BEGIN_CLASS(id) \
    enum { BAT2(_CLASSID_RESERVED_, id) = (id) }; \
    inline const ClassID BAT2(CID_, TYPE) = (ClassID)(id); \
    inline const char BAT2(CLASSNAME_, TYPE)[CLASS_MAXNAMELENGTH] = BSTR(TYPE)

#define INHERITS(parentname) \
    static const ClassID BAT2(_INHERITS_FROM_, TYPE) = BAT2(CID_, parentname)

//   DECLARE_MID(Detonate)
// Expands to (with #define TYPE Exploder):
//   static MessageID MID_Exploder_Detonate = "Exploder.Detonate";
#define DECLARE_MID(msgname) \
    static MessageID BAT4(MID_, TYPE, _, msgname) = BSTR(TYPE) "." BSTR(msgname)

// ============================================================
// Message handler macros
// ============================================================

//   MESSAGE_HANDLER_BEGIN(ShimmiShimmiYea)
// Expands to (with #define TYPE Exploder):
//   static void MESSAGE_HANDLER_Exploder_ShimmiShimmiYea(MessagePayload* payload) {
//       payload->result = MESSAGE_RESULT_SUCCESS;
#define MESSAGE_HANDLER_BEGIN(handlername) \
    static void BAT4(MESSAGE_HANDLER_, TYPE, _, handlername)(MessagePayload* payload) { \
        payload->result = MESSAGE_RESULT_SUCCESS;

//   MESSAGE_HANDLER_BEGIN_EXTERN(Object, SELF_Create)
//   With TYPE=Counter -> MESSAGE_HANDLER_Counter_Object_SELF_Create
#define MESSAGE_HANDLER_BEGIN_EXTERN(classname, handlername) \
    static void BAT6(MESSAGE_HANDLER_, TYPE, _, classname, _, handlername)(MessagePayload* payload) { \
        payload->result = MESSAGE_RESULT_SUCCESS;

#define MESSAGE_HANDLER_END() }

// ---- Getters ----

// Returns a typed pointer to the stored value, or NULL if not found.
//   float* ptr = MH_Get(Strength, float);
#define MH_Get(paramname, type) \
    ((type*)UnsafeVariedHashMap_SGet(payload->data, STR(paramname)))

// Returns the stored value by copy (unsafe -- crashes if key missing).
//   float val = MH_GetDeref(Strength, float);
#define MH_GetDeref(paramname, type) \
    UnsafeVariedHashMap_SGetDeref(payload->data, STR(paramname), type)

// ---- Setters ----

// Stores raw bytes into the payload data (create or update).
//   MH_Set(result, &my_data, sizeof(my_data));
#define MH_Set(paramname, value_ptr, value_size) \
    UnsafeVariedHashMap_SUpsert(payload->data, STR(paramname), value_ptr, value_size)

// Stores a typed value into the payload data (create or update).
//   int result = a + b;
//   MH_SetValue(result, int, result);
#define MH_SetValue(paramname, type, var) \
    MH_Set(paramname, &(type){var}, sizeof(type))

// ---- Checks ----

// Bool expression: true if the key exists.
//   if (MH_Has(Strength)) { ... }
#define MH_Has(paramname) \
    (UnsafeVariedHashMap_SHas(payload->data, STR(paramname)))

// Fails with MISSING_PARAMS if the key does not exist.
//   MH_Require(Strength);
#define MH_Require(paramname) \
    if (!UnsafeVariedHashMap_SHas(payload->data, STR(paramname))) { \
        payload->result = MESSAGE_RESULT_MISSING_PARAMS; \
        return; \
    }

// ---- Extractions (safe -- require + declare variable) ----

// Requires key, then declares type* paramname = pointer to stored value.
//   MH_Extract(Strength, float);  // declares: float* Strength
#define MH_Extract(paramname, type) \
    MH_Require(paramname) \
    type* paramname = MH_Get(paramname, type)

// Requires key, then declares type paramname = copy of stored value.
//   MH_ExtractDeref(Strength, float);  // declares: float Strength
#define MH_ExtractDeref(paramname, type) \
    MH_Require(paramname) \
    type paramname = MH_GetDeref(paramname, type)

// ============================================================
// CanReceiveMID helper macros
// ============================================================

#define CAN_RECEIVE_BEGIN() \
    static bool BAT2(TYPE, _CanReceiveMID)(MessageID mid) {

#define CAN_RECEIVE_MID(msgname) \
    if (strcmp(mid, BAT4(MID_, TYPE, _, msgname)) == 0) return true;

#define CAN_RECEIVE_MID_EXTERN(classname, msgname) \
    if (strcmp(mid, BAT4(MID_, classname, _, msgname)) == 0) return true;

#define CAN_RECEIVE_END() \
        return false; \
    }

// ============================================================
// ReceiveMessage router macros
// ============================================================

#define RECEIVE_MESSAGE_BEGIN() \
    static void BAT2(TYPE, _ReceiveMessage)(MessagePayload* payload) { \
        if (strcmp(payload->mid, "\0") == 0) { (void)0; }

#define RECEIVE_MESSAGE_ROUTE(msgname) \
        else if (strcmp(payload->mid, BAT4(MID_, TYPE, _, msgname)) == 0) { \
            BAT4(MESSAGE_HANDLER_, TYPE, _, msgname)(payload); \
        }

//   RECEIVE_MESSAGE_ROUTE_EXTERN(Object, SELF_Create)
//   Matches MID_Object_SELF_Create, calls MESSAGE_HANDLER_Counter_Object_SELF_Create
#define RECEIVE_MESSAGE_ROUTE_EXTERN(classname, msgname) \
        else if (strcmp(payload->mid, BAT4(MID_, classname, _, msgname)) == 0) { \
            BAT6(MESSAGE_HANDLER_, TYPE, _, classname, _, msgname)(payload); \
        }

#define RECEIVE_MESSAGE_END() \
        else { payload->result = MESSAGE_RESULT_NOT_SUPPORTED; } \
    }

// ============================================================
// ClassDef builder macro
// ============================================================

// Expands to a static function Exploder_ClassDef() (with #define TYPE Exploder).
#define CLASSDEF() \
    static ClassDefinition BAT2(TYPE, _ClassDef)(void) { \
        ClassDefinition _cd = {0}; \
        _cd.cid = BAT2(CID_, TYPE); \
        strncpy(_cd.classname, BAT2(CLASSNAME_, TYPE), CLASS_MAXNAMELENGTH - 1); \
        _cd.parent_cid = CID_Untyped; \
        _cd.CanReceiveMID = BAT2(TYPE, _CanReceiveMID); \
        _cd.ReceiveMessage = BAT2(TYPE, _ReceiveMessage); \
        return _cd; \
    }

#define CLASSDEF_INHERITS(parentname) \
    static ClassDefinition BAT2(TYPE, _ClassDef)(void) { \
        ClassDefinition _cd = {0}; \
        _cd.cid = BAT2(CID_, TYPE); \
        strncpy(_cd.classname, BAT2(CLASSNAME_, TYPE), CLASS_MAXNAMELENGTH - 1); \
        _cd.parent_cid = BAT2(CID_, parentname); \
        _cd.CanReceiveMID = BAT2(TYPE, _CanReceiveMID); \
        _cd.ReceiveMessage = BAT2(TYPE, _ReceiveMessage); \
        return _cd; \
    }

#define CALL_BASE() do { \
    ClassID _base_cid = ClassDefinitions[BAT2(CID_, TYPE)].parent_cid; \
    while (!CLASSID_ISUNTYPED(_base_cid)) { \
        if (ClassDefinitions[_base_cid].CanReceiveMID(payload->mid)) { \
            ClassDefinitions[_base_cid].ReceiveMessage(payload); \
            break; \
        } \
        _base_cid = ClassDefinitions[_base_cid].parent_cid; \
    } \
} while (0)

#define IGNORE_BASE() ((void)0)

#undef LINTNORE

// Only frees data (the UnsafeVariedHashMap) -- the MessagePayload struct
// itself lives on the stack, so only the heap-allocated map needs cleanup.
static inline void FreePayload(MessagePayload* payload) {
    if (payload->data != NULL) {
        UnsafeVariedHashMap_Destroy(payload->data);
    }
}

// ============================================================
// Dispatch helpers
// ============================================================

static inline uint8_t Dispatch(ClassID cid, MessageID mid) {
    MessagePayload _p = PreparePayload(cid, mid);
    if (_p.data == NULL) { return MESSAGE_RESULT_OOM; }
    DispatchMessage(&_p);
    uint8_t _r = _p.result;
    FreePayload(&_p);
    return _r;
}

// msg is a MessagePayload* usable in both blocks. Payload is freed after out_block.
#define DISPATCH(cid, mid, params_block, out_block) ({ \
    MessagePayload _dp = PreparePayload(cid, mid); \
    MessagePayload *msg = &_dp; \
    uint8_t _dr = MESSAGE_RESULT_OOM; \
    if (msg->data != NULL) { \
        params_block \
        DispatchMessage(msg); \
        out_block \
        _dr = msg->result; \
    } \
    FreePayload(msg); \
    _dr; \
})
