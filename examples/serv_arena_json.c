#define NS_DEBUG
#define NS_THREAD_POOL_SIZE 10
#define NC_IMPLEMENTATION
#define NJ_IMPLEMENTATION
#define NS_IMPLEMENTATION
#include "nanoserv.h"
#include "nanojson.h"

typedef struct A {
    int b;
    char* c;
    bool d;
    int e[64];
} A;

typedef struct B {
    A* a;
    int c;
} B;

typedef struct C {
    bool a;
    int b;
} C;

typedef struct D {
    A_VEC(C) c_vec;
    char* e;
} D;


NJ_DEFINE_PARSE_SERIALIZE(C, a, bool, b, integral);
NJ_DEFINE_VECTOR_PARSE_SERIALIZE(C);

NJ_DEFINE_PARSE_SERIALIZE(D, c_vec, NJ_VEC(C), e, string);

NJ_DEFINE_PARSE_SERIALIZE(A, b, integral, c, string, d, bool, e, NJ_ARRAY(int));
NJ_DEFINE_PARSE_SERIALIZE(B, a, NJ_PTR(A), c, integral);

void gen_handle(HTTPRequest* req, HTTPResponse* res){
    res->body = malloc(7);
    strcpy((char *)res->body, "abc""\0");
    res->status_code = 200;
}

int check_arena_json_server(int argc, char** argv){

    arena_allocator arena;
    n_arena_allocator_init(&arena, 256, false);
    char* al = arena.alloc(arena.context, 32);
    strcpy(al, "This is a string");
    int* somenum = arena.alloc(arena.context, sizeof(int));
    *somenum = 7;
    printf("%s, %d, %p\n", al, *somenum, al);
    al = arena.realloc(arena.context, al, 64);
    strcat(al, "maybe");
    printf("%s, %d, %p\n", al, *somenum, al);

    JsonParser parser = {.allocator=arena};

    uint64_t* somenum2 = arena.alloc(arena.context, sizeof(uint64_t));
    *somenum2 = 10;
    printf("%s, %zu, %p\n", al, *somenum2, somenum2);

    const char* filename = "./json_check/r.json";
    FILE* file = fopen(filename, "r");
    char buffer[4096] = {0};
    size_t r = fread(buffer, 1, sizeof(buffer), file);
    buffer[r] = '\0';
    int errc = -1;
    JsonNode* parent = json_from_buffer(buffer, strlen(buffer), &errc, &parser);
    if(errc != -1){
        printf("Error on parsing: %d", errc);
        return 1;
    }
    buffer[0] = '\0';
    r = json_output_node(parent, buffer, 4096, 2);
    buffer[r] = '\0';
    printf("%s", buffer);
    fclose(file);

    memset(buffer, 0, sizeof(buffer));
    file = fopen("./json_check/a.json", "r");
    size_t r2 = fread(buffer, 1, sizeof(buffer), file);
    parent = json_from_buffer(buffer, r2, &errc, NULL);
    if(errc != -1){
        printf("Error on parsing: %d", errc);
        return 1;
    }
    char some[128];
    A a_to_be_parsed = {.c=some};
    NJ_PARSE_NODE(A, parent, &a_to_be_parsed, &parser);
    printf("%s\n", a_to_be_parsed.c);

    a_to_be_parsed.b = 43;
    B b = {.a=&a_to_be_parsed, .c=22};
    JsonNode node = {.type=OBJECT};
    NJ_SERIALIZE_NODE(B, &node, &b, &parser);
    memset(buffer, 0, 4096);
    r = json_output_node(&node, buffer, 4096, 2);
    buffer[r] = '\0';
    printf("%s\n\n", buffer);

    node = (JsonNode){.type=OBJECT};
    C c1 = {.a=true, .b=3}; C c2 = {.a=false, .b=6}; C c3 = {.a=true, .b=2};
    D d = {.e="abc"};
    VEC_Push(d.c_vec, &c1); VEC_Push(d.c_vec, &c2); VEC_Push(d.c_vec, &c3);
    NJ_SERIALIZE_NODE(D, &node, &d, &parser);
    memset(buffer, 0, 4096);
    r = json_output_node(&node, buffer, 4096, 2);
    buffer[r] = '\0';
    printf("%s\n", buffer);

    Server* serv = ns_create_server(NULL, 3000, true);
    if(serv == NULL){
        fprintf(stderr, "Failed to initialize server\n");
        return 1;
    }
    ns_request(serv, GET, "/a", gen_handle, NULL);
    ns_start_server(serv);
    return 0;
}