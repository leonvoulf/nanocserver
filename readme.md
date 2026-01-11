A very basic HTTP Server + JSON parser/serializer header-only library with some useful macros for working with different types. Developed mostly for personal uses, and *should not be considered production-level or safe/stable*.

For a simple example see examples/json_with_server.c

Note that nanojson.h and nanoserv.h are separate header-only "libraries" and not dependent upon one another, but both depend on nanocommon.h

Ultimately, the .c file which includes the implementation must:
#define NC_IMPLEMENTATION

and either #define NJ_IMPLEMENTATION or #define NS_IMPLEMNTATION, depending on which header you used.

Documentation is lacking, and will (maybe) improve...

Should work on Windows 10 (MSVC), Linux (glibc 2.28, [gcc 9.0+/clang 14+]), . Requires C11 "threads.h", which is available only in newer versions of MSVC (2022+). Other stdlibs are not supported.