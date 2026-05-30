void bad_function(void *obj) {
    ((void*)obj)->internal_refs = 5;
    ((void*)obj)->external_refs += 1;
}
