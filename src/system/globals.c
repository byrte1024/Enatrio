#include "class/Class.h"
#include "object/ObjectContainer.h"
#include "object/ObjectRef.h"
#include "object/Serialization.h"

// Class system
ClassDefinition ClassDefinitions[CLASSID_MAX + 1] = {0};
bool ClassRegistrationsOpen = false;

// Payload pool
UnsafeVariedHashMap **_payload_pool = NULL;
uint32_t _payload_pool_count = 0;
uint32_t _payload_pool_capacity = 0;

// Object registry
UnsafeArray *_object_registry = NULL;

// GC state
bool _gc_running = false;
int _gc_recursion_depth = 0;

// Serialization registry
SerEntry _ser_registry[SER_MAX_ID] = {0};
bool _ser_initialized = false;
