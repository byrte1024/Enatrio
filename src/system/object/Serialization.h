#pragma once

#include "Self.h"
#include "../cts/ByteStream.h"

// ============================================================
// Serialization registry types
// ============================================================

typedef void *(*SerFn)(const void *data, uint32_t size, uint16_t arg, uint32_t *out_size);
typedef void *(*DeserFn)(const void *data, uint32_t size, uint16_t arg, uint32_t *out_size);

typedef struct {
    SerFn serialize;
    DeserFn deserialize;
    char name[32];
    bool registered;
} SerEntry;

inline SerEntry _ser_registry[SER_MAX_ID] = {0};
inline bool _ser_initialized = false;

// ============================================================
// Built-in serialize/deserialize functions
// ============================================================

// SER_RAW (ID 0): copy bytes as-is
static void *_ser_raw(const void *data, uint32_t size, uint16_t arg, uint32_t *out_size) {
    (void)arg;
    void *copy = malloc(size);
    if (!copy) return NULL;
    memcpy(copy, data, size);
    *out_size = size;
    return copy;
}

// SER_SKIP (ID 1): serialize returns NULL (value is skipped)
// Deserialize should never be called but returns NULL as safety.

// SER_DEREF (ID 2): dereference pointer and copy arg bytes
static void *_ser_deref(const void *data, uint32_t size, uint16_t arg, uint32_t *out_size) {
    (void)size;
    if (arg == 0) return NULL;
    void *ptr = *(void **)data;
    if (!ptr) { *out_size = 0; return NULL; }
    void *copy = malloc(arg);
    if (!copy) return NULL;
    memcpy(copy, ptr, arg);
    *out_size = arg;
    return copy;
}

static void *_deser_deref(const void *data, uint32_t size, uint16_t arg, uint32_t *out_size) {
    (void)arg;
    void *block = malloc(size);
    if (!block) return NULL;
    memcpy(block, data, size);
    void *result = malloc(sizeof(void *));
    if (!result) { free(block); return NULL; }
    *(void **)result = block;
    *out_size = (uint32_t)sizeof(void *);
    return result;
}

// SER_STRING (ID 3): serialize reads char* pointer, writes strlen+1 bytes
static void *_ser_string(const void *data, uint32_t size, uint16_t arg, uint32_t *out_size) {
    (void)size; (void)arg;
    char *str = *(char **)data;
    if (!str) { *out_size = 0; return NULL; }
    uint32_t len = (uint32_t)strlen(str) + 1;
    char *copy = (char *)malloc(len);
    if (!copy) return NULL;
    memcpy(copy, str, len);
    *out_size = len;
    return copy;
}

static void *_deser_string(const void *data, uint32_t size, uint16_t arg, uint32_t *out_size) {
    (void)arg;
    char *str = (char *)malloc(size);
    if (!str) return NULL;
    memcpy(str, data, size);
    void *result = malloc(sizeof(char *));
    if (!result) { free(str); return NULL; }
    *(char **)result = str;
    *out_size = (uint32_t)sizeof(char *);
    return result;
}

// ============================================================
// Registration API
// ============================================================

static void _Ser_InitBuiltins(void) {
    if (_ser_initialized) return;
    _ser_initialized = true;
    _ser_registry[SER_RAW] = (SerEntry){ _ser_raw, _ser_raw, "RAW", true };
    _ser_registry[SER_SKIP] = (SerEntry){ NULL, NULL, "SKIP", true };
    _ser_registry[SER_DEREF] = (SerEntry){ _ser_deref, _deser_deref, "DEREF", true };
    _ser_registry[SER_STRING] = (SerEntry){ _ser_string, _deser_string, "STRING", true };
}

static int Ser_Register(uint16_t id, SerFn serialize, DeserFn deserialize, const char *name) {
    _Ser_InitBuiltins();
    if (id >= SER_MAX_ID) return -1;
    if (id < SER_USER_START) {
        LOG_ERROR("Ser_Register: ID %u is reserved (use >= %u)", id, SER_USER_START);
        return -1;
    }
    if (_ser_registry[id].registered) {
        LOG_ERROR("Ser_Register: ID %u already registered as '%s'", id, _ser_registry[id].name);
        return -1;
    }
    _ser_registry[id].serialize = serialize;
    _ser_registry[id].deserialize = deserialize;
    _ser_registry[id].registered = true;
    strncpy(_ser_registry[id].name, name ? name : "USER", 31);
    _ser_registry[id].name[31] = '\0';
    return 0;
}

static SerEntry *Ser_Get(uint16_t id) {
    _Ser_InitBuiltins();
    if (id >= SER_MAX_ID) return NULL;
    if (!_ser_registry[id].registered) return NULL;
    return &_ser_registry[id];
}

// ============================================================
// File format constants
// ============================================================

#define _SER_MAGIC_0 'E'
#define _SER_MAGIC_1 'C'
#define _SER_MAGIC_2 'O'
#define _SER_MAGIC_3 'B'
#define _SER_VERSION 1

// ============================================================
// Serialization helpers
// ============================================================

static int32_t _ser_find_id(UnsafeArray *visited, TempObjectReference target) {
    for (uint32_t i = 0; i < visited->count; i++) {
        if (*(TempObjectReference *)UnsafeArray_Get(visited, i) == target) return (int32_t)i;
    }
    return -1;
}

// File-scope statics for ForEach trampolines
static ByteStream *_ser_stream = NULL;
static UnsafeArray *_ser_visited = NULL;
static uint32_t _ser_value_count = 0;

static void _ser_write_value(const void *key, uint32_t key_len, void *value, uint32_t value_size) {
    if (value_size < sizeof(ObjectValueHeader)) return;
    ObjectValueHeader *hdr = (ObjectValueHeader *)value;
    if (hdr->ser_id == SER_SKIP) return;

    SerEntry *entry = Ser_Get(hdr->ser_id);
    uint32_t data_size = value_size - (uint32_t)sizeof(ObjectValueHeader);
    void *data = (uint8_t *)value + sizeof(ObjectValueHeader);

    void *serialized = NULL;
    uint32_t ser_size = 0;
    if (entry && entry->serialize) {
        serialized = entry->serialize(data, data_size, hdr->ser_arg, &ser_size);
        if (!serialized) return;
    } else {
        // No serialize function (should not happen for non-SKIP), write raw
        serialized = data;
        ser_size = data_size;
    }

    ByteStream_Write(_ser_stream, &key_len, sizeof(uint32_t));
    ByteStream_Write(_ser_stream, key, key_len);
    ByteStream_Write(_ser_stream, &hdr->owner, sizeof(ClassID));
    ByteStream_Write(_ser_stream, &hdr->ser_id, sizeof(uint16_t));
    ByteStream_Write(_ser_stream, &hdr->ser_arg, sizeof(uint16_t));
    ByteStream_Write(_ser_stream, &ser_size, sizeof(uint32_t));
    ByteStream_Write(_ser_stream, serialized, ser_size);

    if (entry && entry->serialize) free(serialized);
    _ser_value_count++;
}

static void _ser_write_ref(const void *key, uint32_t key_len, void *value) {
    ObjectReference *ref = (ObjectReference *)value;
    if (!ref || !*ref) return;
    TempObjectReference target = ObjectContainer_TempFrom(*ref);
    int32_t target_id = _ser_find_id(_ser_visited, target);
    if (target_id < 0) return;

    ByteStream_Write(_ser_stream, &key_len, sizeof(uint32_t));
    ByteStream_Write(_ser_stream, key, key_len);
    uint32_t tid = (uint32_t)target_id;
    ByteStream_Write(_ser_stream, &tid, sizeof(uint32_t));
    _ser_value_count++;
}

// ============================================================
// Object_Serialize
// ============================================================

static ByteStream *Object_Serialize(ExternalReference *roots, int root_count) {
    _Ser_InitBuiltins();
    if (!roots || root_count <= 0) return NULL;

    // BFS to discover all reachable objects
    UnsafeArray *visited = UnsafeArray_Create(sizeof(TempObjectReference), 64);
    UnsafeArray *worklist = UnsafeArray_Create(sizeof(TempObjectReference), 64);

    for (int i = 0; i < root_count; i++) {
        if (!roots[i]) continue;
        TempObjectReference t = ObjectContainer_TempFrom(roots[i]);
        if (_ser_find_id(visited, t) < 0) {
            UnsafeArray_Add(visited, &t);
            UnsafeArray_Add(worklist, &t);
        }
    }

    uint32_t idx = 0;
    while (idx < worklist->count) {
        TempObjectReference node = *(TempObjectReference *)UnsafeArray_Get(worklist, idx);
        idx++;
        if (!node->data || !node->data->references) continue;
        UnsafeHashMap *refs = node->data->references;
        for (uint32_t b = 0; b < refs->bucket_count; b++) {
            UnsafeHashEntry *e = &refs->buckets[b];
            if (e->value < 0) continue;
            ObjectReference *ref = (ObjectReference *)UnsafeArray_Get(refs->values, (uint32_t)e->value);
            if (!ref || !*ref) continue;
            TempObjectReference target = ObjectContainer_TempFrom(*ref);
            if (_ser_find_id(visited, target) < 0) {
                UnsafeArray_Add(visited, &target);
                UnsafeArray_Add(worklist, &target);
            }
        }
    }
    UnsafeArray_Destroy(worklist);

    // Write to ByteStream
    ByteStream *stream = ByteStream_Create(1024);
    _ser_stream = stream;
    _ser_visited = visited;

    // Header
    uint8_t magic[4] = { _SER_MAGIC_0, _SER_MAGIC_1, _SER_MAGIC_2, _SER_MAGIC_3 };
    ByteStream_Write(stream, magic, 4);
    ByteStream_WriteValue(stream, uint32_t, _SER_VERSION);
    ByteStream_WriteValue(stream, uint32_t, visited->count);
    ByteStream_WriteValue(stream, uint32_t, (uint32_t)root_count);

    // Root table
    for (int i = 0; i < root_count; i++) {
        TempObjectReference t = roots[i] ? ObjectContainer_TempFrom(roots[i]) : NULL;
        int32_t sid = t ? _ser_find_id(visited, t) : -1;
        ByteStream_WriteValue(stream, uint32_t, (uint32_t)sid);
        ByteStream_WriteValue(stream, int32_t, t ? t->external_refs : 0);
    }

    // Object table
    for (uint32_t i = 0; i < visited->count; i++) {
        TempObjectReference obj = *(TempObjectReference *)UnsafeArray_Get(visited, i);
        ByteStream_WriteValue(stream, uint32_t, i);
        ByteStream_WriteValue(stream, uint16_t, obj->cid);

        // Values -- write placeholder count, then values, then fix count
        uint32_t count_pos = stream->cursor;
        ByteStream_WriteValue(stream, uint32_t, 0);
        _ser_value_count = 0;
        if (obj->data && obj->data->values) {
            UnsafeVariedHashMap_ForEach(obj->data->values, _ser_write_value);
        }
        uint32_t end_pos = stream->cursor;
        ByteStream_Seek(stream, count_pos);
        ByteStream_WriteValue(stream, uint32_t, _ser_value_count);
        ByteStream_Seek(stream, end_pos);

        // References -- same pattern
        count_pos = stream->cursor;
        ByteStream_WriteValue(stream, uint32_t, 0);
        _ser_value_count = 0;
        if (obj->data && obj->data->references) {
            UnsafeHashMap_ForEach(obj->data->references, _ser_write_ref);
        }
        end_pos = stream->cursor;
        ByteStream_Seek(stream, count_pos);
        ByteStream_WriteValue(stream, uint32_t, _ser_value_count);
        ByteStream_Seek(stream, end_pos);
    }

    _ser_stream = NULL;
    _ser_visited = NULL;
    UnsafeArray_Destroy(visited);
    ByteStream_Rewind(stream);
    return stream;
}

// ============================================================
// Object_Deserialize
// ============================================================

static ExternalReference *Object_Deserialize(ByteStream *stream, int *out_root_count) {
    _Ser_InitBuiltins();
    if (!stream || !out_root_count) return NULL;
    ByteStream_Rewind(stream);

    // Read and validate header
    uint8_t magic[4];
    if (ByteStream_Read(stream, magic, 4) != 0) return NULL;
    if (magic[0] != _SER_MAGIC_0 || magic[1] != _SER_MAGIC_1 ||
        magic[2] != _SER_MAGIC_2 || magic[3] != _SER_MAGIC_3) {
        LOG_ERROR("Object_Deserialize: bad magic");
        return NULL;
    }

    uint32_t version = ByteStream_ReadDeref(stream, uint32_t);
    if (version != _SER_VERSION) {
        LOG_ERROR("Object_Deserialize: unsupported version %u", version);
        return NULL;
    }

    uint32_t object_count = ByteStream_ReadDeref(stream, uint32_t);
    uint32_t root_count = ByteStream_ReadDeref(stream, uint32_t);

    // Read root table
    uint32_t *root_ids = (uint32_t *)malloc(root_count * sizeof(uint32_t));
    int32_t *root_ext_refs = (int32_t *)malloc(root_count * sizeof(int32_t));
    if (!root_ids || !root_ext_refs) {
        free(root_ids); free(root_ext_refs);
        return NULL;
    }
    for (uint32_t i = 0; i < root_count; i++) {
        root_ids[i] = ByteStream_ReadDeref(stream, uint32_t);
        root_ext_refs[i] = ByteStream_ReadDeref(stream, int32_t);
    }

    // Phase 1: Create all objects, skip value/ref data
    TempObjectReference *id_map = (TempObjectReference *)malloc(object_count * sizeof(TempObjectReference));
    if (!id_map) { free(root_ids); free(root_ext_refs); return NULL; }

    uint32_t data_start = stream->cursor;

    for (uint32_t i = 0; i < object_count; i++) {
        uint32_t ser_id = ByteStream_ReadDeref(stream, uint32_t);
        uint16_t cid = ByteStream_ReadDeref(stream, uint16_t);

        TempObjectReference obj = Object_Create(cid);
        if (!obj) {
            LOG_ERROR("Object_Deserialize: failed to create object %u (CID 0x%04X)", ser_id, cid);
            for (uint32_t j = 0; j < i; j++) {
                if (id_map[j] && ObjectContainer_TotalRefs(id_map[j]) == 0) {
                    Object_Destroy(id_map[j]);
                }
            }
            free(id_map); free(root_ids); free(root_ext_refs);
            return NULL;
        }
        id_map[ser_id] = obj;

        // Skip values
        uint32_t value_count = ByteStream_ReadDeref(stream, uint32_t);
        for (uint32_t v = 0; v < value_count; v++) {
            uint32_t key_len = ByteStream_ReadDeref(stream, uint32_t);
            ByteStream_Skip(stream, key_len);
            ByteStream_Skip(stream, sizeof(ClassID));
            ByteStream_Skip(stream, sizeof(uint16_t));
            ByteStream_Skip(stream, sizeof(uint16_t));
            uint32_t data_size = ByteStream_ReadDeref(stream, uint32_t);
            ByteStream_Skip(stream, data_size);
        }

        // Skip refs
        uint32_t ref_count = ByteStream_ReadDeref(stream, uint32_t);
        for (uint32_t r = 0; r < ref_count; r++) {
            uint32_t key_len = ByteStream_ReadDeref(stream, uint32_t);
            ByteStream_Skip(stream, key_len);
            ByteStream_Skip(stream, sizeof(uint32_t));
        }
    }

    // Phase 2: Restore values (re-read from data_start)
    ByteStream_Seek(stream, data_start);

    for (uint32_t i = 0; i < object_count; i++) {
        uint32_t ser_id = ByteStream_ReadDeref(stream, uint32_t);
        ByteStream_Skip(stream, sizeof(uint16_t));
        TempObjectReference obj = id_map[ser_id];

        uint32_t value_count = ByteStream_ReadDeref(stream, uint32_t);
        for (uint32_t v = 0; v < value_count; v++) {
            uint32_t key_len = ByteStream_ReadDeref(stream, uint32_t);
            uint8_t key_buf[256];
            ByteStream_Read(stream, key_buf, key_len);

            ClassID owner = ByteStream_ReadDeref(stream, ClassID);
            uint16_t sid = ByteStream_ReadDeref(stream, uint16_t);
            uint16_t sarg = ByteStream_ReadDeref(stream, uint16_t);
            uint32_t data_size = ByteStream_ReadDeref(stream, uint32_t);

            void *ser_data = malloc(data_size);
            ByteStream_Read(stream, ser_data, data_size);

            // Deserialize
            SerEntry *entry = Ser_Get(sid);
            void *restored = NULL;
            uint32_t restored_size = 0;
            if (entry && entry->deserialize) {
                restored = entry->deserialize(ser_data, data_size, sarg, &restored_size);
            } else {
                restored = ser_data;
                restored_size = data_size;
                ser_data = NULL;
            }
            free(ser_data);

            if (restored && obj->data) {
                _Object_StoreValue(obj->data->values, key_buf, key_len,
                                   restored, restored_size, owner, sid, sarg);
            }
            if (restored) free(restored);
        }

        // Skip refs (phase 3 handles them)
        uint32_t ref_count = ByteStream_ReadDeref(stream, uint32_t);
        for (uint32_t r = 0; r < ref_count; r++) {
            uint32_t key_len = ByteStream_ReadDeref(stream, uint32_t);
            ByteStream_Skip(stream, key_len);
            ByteStream_Skip(stream, sizeof(uint32_t));
        }
    }

    // Phase 3: Restore references (re-read from data_start)
    ByteStream_Seek(stream, data_start);

    for (uint32_t i = 0; i < object_count; i++) {
        uint32_t ser_id = ByteStream_ReadDeref(stream, uint32_t);
        ByteStream_Skip(stream, sizeof(uint16_t));
        TempObjectReference obj = id_map[ser_id];

        // Skip values
        uint32_t value_count = ByteStream_ReadDeref(stream, uint32_t);
        for (uint32_t v = 0; v < value_count; v++) {
            uint32_t key_len = ByteStream_ReadDeref(stream, uint32_t);
            ByteStream_Skip(stream, key_len);
            ByteStream_Skip(stream, sizeof(ClassID));
            ByteStream_Skip(stream, sizeof(uint16_t));
            ByteStream_Skip(stream, sizeof(uint16_t));
            uint32_t data_size = ByteStream_ReadDeref(stream, uint32_t);
            ByteStream_Skip(stream, data_size);
        }

        // Restore refs
        uint32_t ref_count = ByteStream_ReadDeref(stream, uint32_t);
        for (uint32_t r = 0; r < ref_count; r++) {
            uint32_t key_len = ByteStream_ReadDeref(stream, uint32_t);
            uint8_t key_buf[256];
            ByteStream_Read(stream, key_buf, key_len);
            uint32_t target_id = ByteStream_ReadDeref(stream, uint32_t);

            if (target_id < object_count && obj->data) {
                TempObjectReference target = id_map[target_id];
                if (UnsafeHashMap_Has(obj->data->references, key_buf, key_len)) {
                    ObjectReference *old = (ObjectReference *)UnsafeHashMap_Get(obj->data->references, key_buf, key_len);
                    ObjectContainer_UnRef_Internal(old);
                    UnsafeHashMap_Remove(obj->data->references, key_buf, key_len);
                }
                ObjectReference iref = ObjectContainer_InternalRef_From_Temp(target);
                UnsafeHashMap_Set(obj->data->references, key_buf, key_len, &iref);
            }
        }
    }

    // Phase 4: Build return array with external refs
    ExternalReference *result = (ExternalReference *)malloc(root_count * sizeof(ExternalReference));
    if (!result) { free(id_map); free(root_ids); free(root_ext_refs); return NULL; }

    for (uint32_t i = 0; i < root_count; i++) {
        if (root_ids[i] < object_count) {
            TempObjectReference obj = id_map[root_ids[i]];
            obj->external_refs = root_ext_refs[i];
            result[i] = (ExternalReference)obj;
        } else {
            result[i] = NULL;
        }
    }

    *out_root_count = (int)root_count;
    free(id_map);
    free(root_ids);
    free(root_ext_refs);
    return result;
}

// ============================================================
// File I/O wrappers
// ============================================================

static int Object_SaveToFile(const char *path, ExternalReference *roots, int root_count) {
    ByteStream *stream = Object_Serialize(roots, root_count);
    if (!stream) return -1;
    int result = ByteStream_SaveToFile(stream, path);
    ByteStream_Destroy(stream);
    return result;
}

static ExternalReference *Object_LoadFromFile(const char *path, int *out_root_count) {
    ByteStream *stream = ByteStream_LoadFromFile(path);
    if (!stream) return NULL;
    ExternalReference *result = Object_Deserialize(stream, out_root_count);
    ByteStream_Destroy(stream);
    return result;
}
