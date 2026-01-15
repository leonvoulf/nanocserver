Json: A truth/false/null value with an unexpected continuation after them will probably be undetected (due to the way detection of true/false/null is implemented)

Some JSON utils may streamline usage:
    1. Json utils - adding nodes to nodes.
    2. Parsing simply values out of fields, without predefined types

Json: maybe using hashmaps will be faster for larger objects, benchmark it in the future

Server: maybe usage of an arena allocator would be quicker - but it would require initialization of many arena allocators, maybe only create "small" arenas for predictable things like header sizes

Server: SSL is currently not supported (add libssl support)
Server: maybe add a basic http client