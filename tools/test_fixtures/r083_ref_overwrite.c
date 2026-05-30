void bad(void) {
    ExternalReference obj = Object_CreateRef(0x1234);
    obj = Object_CreateRef(0x5678);
}
