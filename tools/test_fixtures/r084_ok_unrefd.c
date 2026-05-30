void good(void) {
    ExternalReference obj = Object_CreateRef(0x1234);
    ObjectContainer_UnRef_External(&obj);
}
