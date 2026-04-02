#pragma once

#include <stdint.h>
#include <stdio.h>

#include "Class.h"
#include "UnsafeDictionary.h"
#include "UnsafeHashMap.h"

typedef struct ObjectData {
    //Must ONLY contain primitive vals/pointers, NOTHING that needs nested garbage collection, cause it wont be done.
    UnsafeVariedHashMap* values;

    //References to other ObjectContainer's, will add/remove from reference counter when destroyed.
    UnsafeHashMap* references;
} ObjectData;

typedef ObjectData* ObjectDataPTR;

typedef struct ObjectContainer {
    ObjectDataPTR data;
    ClassID cid;
    
    int reference_counter; //how many references to us currently exist
} ObjectContainer;

