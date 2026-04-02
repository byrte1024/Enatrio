#pragma once

#include <stdint.h>
#include <stdio.h>

#include "UnsafeDictionary.h"
#include "UnsafeHashMap.h"

typedef uint16_t ClassID;
#define CLASSID_MAX UINT16_MAX
#define CLASS_MAXNAMELENGTH 64

typedef const char MessageID[64];
#define MESSAGEID_EMPTY ((const char[64]) {0})

typedef struct MessagePayload {
    MessageID mid;
    ClassID cid_target;
    UnsafeVariedHashMap* data; //KvP of data, values stored directly by copy. NO garbage collection.
    uint8_t result; //any result not 0 is a failure, different result values are different fail reasons
    
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
    "Undefined result code")


typedef struct ClassDefinition {
    
    ClassID cid; //ID of the class, must be globally unique. allows up to CLASSID_MAX different classes
    char classname[CLASS_MAXNAMELENGTH]; //Name of the class, should be unique but not enforced. allows up to CLASS_MAXNAMELENGTH characters

    bool (*CanReceiveMID)(MessageID mid);
    void (*ReceiveMessage)(MessagePayload* payload);


} ClassDefinition;


inline ClassDefinition ClassDefinitions[CLASSID_MAX] = { 0 };
inline bool ClassRegistrationsOpen = false;

#define CLASSID_ISUNTYPED(c) (c) == ((uint16_t)(0x0000))
#define CLASSID_ISREGISTERED(c) (ClassDefinitions[(c)].cid == c)

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

    //search classes to see if name is a duplicate
    for(int i = 0; i < CLASSID_MAX; i++) {
        if(strcmp(ClassDefinitions[i].classname, def.classname) == 0) {
            LOG_ERROR("Class name %s is already registered.", def.classname);
            return;
        }
    }

    //register class
    ClassDefinitions[def.cid] = def;
    LOG_INFO("Class %s registered with ID %d.", def.classname, def.cid);
}

static void EndClassRegistrations() {
    ClassRegistrationsOpen = false;

    LOG_INFO("Class system registration completed.");
    LOG_INFO("========================================");
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

    if(!ClassDefinitions[payload->cid_target].CanReceiveMID(payload->mid)){
        LOG_ERROR("Class %d does not support message ID %s.", payload->cid_target, payload->mid);
        payload->result = MESSAGE_RESULT_NOT_SUPPORTED;
        return payload;
    }

    payload->result = MESSAGE_RESULT_PENDING;
    ClassDefinitions[payload->cid_target].ReceiveMessage(payload);

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

// Stores a value by copy into the payload. Any type, any size.
//   Payload_SetValue(payload, "health", int, 100);
#define Payload_SetValue(payload, str_key, type, value) do { \
    type _psv_tmp = (value); \
    UnsafeVariedHashMap_SSet((payload)->data, str_key, &_psv_tmp, sizeof(type)); \
} while (0)

// Stores a pointer (void*) into the payload by copy.
//   Payload_SetPointer(payload, "out", &result);
#define Payload_SetPointer(payload, str_key, ptr) do { \
    void *_pp_tmp = (ptr); \
    UnsafeVariedHashMap_SSet((payload)->data, str_key, &_pp_tmp, sizeof(void*)); \
} while (0)

// Creates a stack-local variable and stores a pointer to it in the payload.
// The variable lives until the enclosing scope ends.
//   Payload_SetLocalPointer(payload, "out", int, 0);
#define Payload_SetLocalPointer(payload, str_key, type, value) \
    type BAT2(_pslp_, __LINE__) = (value); \
    Payload_SetPointer(payload, str_key, &BAT2(_pslp_, __LINE__))

// ============================================================
// Class definition macros
// ============================================================

// Declares class ID and name globals.
//   BEGIN_CLASS(Exploder, 0x22AB)
// Expands to:
//   inline const ClassID CID_Exploder = (ClassID)(0x22AB);
//   inline const char CLASSNAME_Exploder[CLASS_MAXNAMELENGTH] = "Exploder";
#define BEGIN_CLASS(id) \
    inline const ClassID BAT2(CID_, TYPE) = (ClassID)(id); \
    inline const char BAT2(CLASSNAME_, TYPE)[CLASS_MAXNAMELENGTH] = BSTR(TYPE)

// Declares a static MessageID for a class. Uses TYPE.
//   DECLARE_MID(Detonate)
// Expands to (with #define TYPE Exploder):
//   static MessageID MID_Exploder_Detonate = "Exploder.Detonate";
#define DECLARE_MID(msgname) \
    static MessageID BAT4(MID_, TYPE, _, msgname) = BSTR(TYPE) "." BSTR(msgname)

// ============================================================
// Message handler macros
// ============================================================

// Opens a message handler function. Uses TYPE.
//   MESSAGE_HANDLER_BEGIN(ShimmiShimmiYea)
// Expands to (with #define TYPE Exploder):
//   static void MESSAGE_HANDLER_Exploder_ShimmiShimmiYea(MessagePayload* payload) {
//       payload->result = MESSAGE_RESULT_SUCCESS;
#define MESSAGE_HANDLER_BEGIN(handlername) \
    static void BAT4(MESSAGE_HANDLER_, TYPE, _, handlername)(MessagePayload* payload) { \
        payload->result = MESSAGE_RESULT_SUCCESS;

// Closes a message handler function.
//   MESSAGE_HANDLER_END()
#define MESSAGE_HANDLER_END() }

// Extracts a pointer from the payload dict.
//   MH_EXTRACT_POINTER(Strength, float)
// Expands to:
// Extracts a pointer to the stored value (direct, value is in the map).
//   MH_EXTRACT_POINTER(Strength, float)
// Expands to:
//   float* MH_Strength_Ptr = (float*)UnsafeVariedHashMap_SGet(payload->data, "Strength");
#define MH_EXTRACT_POINTER(paramname, type) \
    type* BAT3(MH_, paramname, _Ptr) = \
        (type*)UnsafeVariedHashMap_SGet(payload->data, STR(paramname))

// Extracts a copy of the stored value.
// Returns INVALID_PARAMS if the key has no data.
//   MH_EXTRACT_VALUE(Strength, float)
#define MH_EXTRACT_VALUE(paramname, type) \
    type* BAT3(_mhev_, paramname, _ptr) = \
        (type*)UnsafeVariedHashMap_SGet(payload->data, STR(paramname)); \
    if (BAT3(_mhev_, paramname, _ptr) == NULL) { \
        payload->result = MESSAGE_RESULT_INVALID_PARAMS; \
        return; \
    } \
    type BAT3(MH_, paramname, _Val) = *BAT3(_mhev_, paramname, _ptr)

// Extracts a stored void* pointer and casts it to type*.
// Use when the caller stored a pointer via Payload_SetPointer.
//   MH_EXTRACT_STORED_PTR(out, int)
// Gives: int* MH_out_Ptr = *(void**)data, cast to int*
#define MH_EXTRACT_STORED_PTR(paramname, type) \
    type* BAT3(MH_, paramname, _Ptr) = \
        (type*)UnsafeVariedHashMap_SGetValue(payload->data, STR(paramname), void*)

// Checks that an extracted pointer is not NULL, returns INVALID_PARAMS if so.
//   MH_REQUIRE_POINTER(Strength)
#define MH_REQUIRE_POINTER(paramname) \
    if (BAT3(MH_, paramname, _Ptr) == NULL) { \
        payload->result = MESSAGE_RESULT_INVALID_PARAMS; \
        return; \
    }

// Bool expression: true if the key exists in the payload data.
//   if (MH_HAS_KEY(Strength)) { ... }
#define MH_HAS_KEY(paramname) \
    (UnsafeVariedHashMap_SHas(payload->data, STR(paramname)))

// Bool expression: true if the key exists and has non-NULL data.
//   if (MH_HAS_VALID_PTR(Strength)) { ... }
#define MH_HAS_VALID_PTR(paramname) \
    (MH_HAS_KEY(paramname) && \
     UnsafeVariedHashMap_SGet(payload->data, STR(paramname)) != NULL)

// Checks that the payload has a key, returns MISSING_PARAMS if not.
//   MH_REQUIRE_KEY(Strength)
#define MH_REQUIRE_KEY(paramname) \
    if (!UnsafeVariedHashMap_SHas(payload->data, STR(paramname))) { \
        payload->result = MESSAGE_RESULT_MISSING_PARAMS; \
        return; \
    }

// Extracts a pointer to stored value and requires the key.
//   MH_REQUIRE_PARAM(Strength, float)
#define MH_REQUIRE_PARAM(paramname, type) \
    MH_REQUIRE_KEY(paramname) \
    MH_EXTRACT_POINTER(paramname, type)

// Extracts a value copy and requires the key.
//   MH_REQUIRE_VALUE(Strength, float)
#define MH_REQUIRE_VALUE(paramname, type) \
    MH_REQUIRE_KEY(paramname) \
    MH_EXTRACT_VALUE(paramname, type)

// Extracts a stored pointer and requires the key + non-NULL.
//   MH_REQUIRE_STORED_PTR(out, int)
#define MH_REQUIRE_STORED_PTR(paramname, type) \
    MH_REQUIRE_KEY(paramname) \
    MH_EXTRACT_STORED_PTR(paramname, type); \
    MH_REQUIRE_POINTER(paramname)

// ============================================================
// CanReceiveMID helper macros
// ============================================================

// Opens a CanReceiveMID function. Uses TYPE.
//   CAN_RECEIVE_BEGIN()
#define CAN_RECEIVE_BEGIN() \
    static bool BAT2(TYPE, _CanReceiveMID)(MessageID mid) {

// Adds a MID check inside CanReceiveMID. Uses TYPE.
//   CAN_RECEIVE_MID(Detonate)
#define CAN_RECEIVE_MID(msgname) \
    if (strcmp(mid, BAT4(MID_, TYPE, _, msgname)) == 0) return true;

// Closes CanReceiveMID (returns false for unrecognized).
//   CAN_RECEIVE_END()
#define CAN_RECEIVE_END() \
        return false; \
    }

// ============================================================
// ReceiveMessage router macros
// ============================================================

// Opens a ReceiveMessage function. Uses TYPE.
//   RECEIVE_MESSAGE_BEGIN()
#define RECEIVE_MESSAGE_BEGIN() \
    static void BAT2(TYPE, _ReceiveMessage)(MessagePayload* payload) { \
        if (strcmp(payload->mid, "\0") == 0) { (void)0; }

// Routes a MID to its handler (else-if chain). Uses TYPE.
//   RECEIVE_MESSAGE_ROUTE(Detonate)
#define RECEIVE_MESSAGE_ROUTE(msgname) \
        else if (strcmp(payload->mid, BAT4(MID_, TYPE, _, msgname)) == 0) { \
            BAT4(MESSAGE_HANDLER_, TYPE, _, msgname)(payload); \
        }

// Closes ReceiveMessage (else branch sets NOT_SUPPORTED for unmatched).
//   RECEIVE_MESSAGE_END()
#define RECEIVE_MESSAGE_END() \
        else { payload->result = MESSAGE_RESULT_NOT_SUPPORTED; } \
    }

// ============================================================
// ClassDef builder macro
// ============================================================

// Creates a ClassDefinition builder function. Uses TYPE.
//   CLASSDEF()
// Expands to a static function Exploder_ClassDef() (with #define TYPE Exploder).
#define CLASSDEF() \
    static ClassDefinition BAT2(TYPE, _ClassDef)(void) { \
        ClassDefinition _cd = {0}; \
        _cd.cid = BAT2(CID_, TYPE); \
        strncpy(_cd.classname, BAT2(CLASSNAME_, TYPE), CLASS_MAXNAMELENGTH - 1); \
        _cd.CanReceiveMID = BAT2(TYPE, _CanReceiveMID); \
        _cd.ReceiveMessage = BAT2(TYPE, _ReceiveMessage); \
        return _cd; \
    }

static inline void FreePayload(MessagePayload* payload) {
    if (payload->data != NULL) {
        UnsafeVariedHashMap_Destroy(payload->data);
    }
}