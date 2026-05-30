void bad_access(void *self_data) {
    UnsafeVariedHashMap_SSet(Self_Values, "key", &val, 4);
    UnsafeVariedHashMap_SGet(obj->data->values, "key");
}
