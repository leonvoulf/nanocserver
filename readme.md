A very basic (and extremely broken!!!) HTTP Server + JSON parser/serializer header-only library with some useful macros for working with different types.
For a simple example see examples/json_with_server.c

Note that nanojson.h and nanoserv.h are separate and not dependent upon one another, but both depend upon nanocommon.h
Ultimately, the C file which includes the implementation must:
#define NC_IMPLEMENTATION

and either #define NJ_IMPLEMENTATION or #define NS_IMPLEMNTATION, depending on which header you used.

Documentation is lacking, and will (maybe) improve...

Should work on Windows 10 (MSVC), Linux Mint 20.04, WSL (gcc). Requires C11 "threads.h", which is available only in newer versions of MSVC (2017+).