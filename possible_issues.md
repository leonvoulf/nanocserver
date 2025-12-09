A truth/false/null value with an unexpected continuation after them will probably be undetected (due to the way detection of true/false/null is implemented)

Some JSON utils may streamline usage:
    1. A macro for both struct definition, and parse/serialize definition (another macrohell)
    2. Json utils - adding nodes to nodes.
    3. A static system allocator-parser

Server: maybe usage of an arena allocator would be quicker - but it would require initialization of many arena allocators, maybe only create "small" arenas for predictable things like header sizes

