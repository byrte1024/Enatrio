#pragma once

#include <stdint.h>
#include <stdio.h>

#include "UnsafeDictionary.h"

typedef uint16_t ClassID;
#define CLASSID_MAX UINT16_MAX
#define CLASS_MAXNAMELENGTH 64

typedef const char MessageID[64];
#define MESSAGEID_EMPTY ((const char[64]) {0})

typedef struct MessageDefinition {

    MessageID mid; //ID of the message, must be globally unique. allows up to 64 characters.

} MessageDefinition;

typedef struct MessagePayload {
    MessageID mid;
    ClassID cid_target;
    UnsafeDictionary* data_dict; //KvP of data, must all be pointers or primitives since NO garbage collection is done
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

    payload.data_dict = UnsafeDictionary_Create(sizeof(void*), 8);
    if (payload.data_dict == NULL) {
        LOG_ERROR("Failed to allocate payload data dictionary.");
        return payload;
    }

    return payload;
}

// Sets a key to a pointer value directly. The argument must already be a pointer.
//   Payload_SetPointer(payload, "name", my_ptr);
#define Payload_SetPointer(payload, str_key, ptr) do { \
    void *_pp_tmp = (ptr); \
    UnsafeDictionary_SSet((payload)->data_dict, str_key, &_pp_tmp); \
} while (0)

// Sets a key to a pointer to the given variable (auto-adds &).
//   Payload_SetPointerTo(payload, "count", my_int);
#define Payload_SetPointerTo(payload, str_key, var) do { \
    void *_ppt_tmp = &(var); \
    UnsafeDictionary_SSet((payload)->data_dict, str_key, &_ppt_tmp); \
} while (0)

// Sets a key to a stack-local variable. Lives until the enclosing scope ends.
//   Payload_SetPointerToValue(payload, "a", int, 4);
#define Payload_SetPointerToValue(payload, str_key, type, value) \
    type BAT2(_pptv_, __LINE__) = (value); \
    Payload_SetPointerTo(payload, str_key, BAT2(_pptv_, __LINE__))

static inline void FreePayload(MessagePayload* payload) {
    if (payload->data_dict != NULL) {
        UnsafeDictionary_Destroy(payload->data_dict);
    }
}