
#include <stdint.h>
#include <stdio.h>

#include "UnsafeDictionary.h"

typedef uint16_t ClassID;
#define CLASSID_MAX UINT16_MAX
#define CLASS_MAXNAMELENGTH 64

typedef char MessageID[64];


typedef struct MessageDefinition {

    MessageID mid; //ID of the message, must be globally unique. allows up to 64 characters.

} MessageDefinition;

typedef struct MessagePayload {
    UnsafeDictionary* data_dict; //KvP of data, must all be pointers or primitives since NO garbage collection is done
    
    
} MessagePayload;

typedef struct ClassDefinition {
    
    ClassID cid; //ID of the class, must be globally unique. allows up to CLASSID_MAX different classes
    char classname[CLASS_MAXNAMELENGTH]; //Name of the class, should be unique but not enforced. allows up to CLASS_MAXNAMELENGTH characters

    bool (*CanRecieveMID)(MessageID mid);



} ClassDefinition;

