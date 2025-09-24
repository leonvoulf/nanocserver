#define NS_DEBUG
#define NS_THREAD_POOL_SIZE 10
#define NC_IMPLEMENTATION
#define NJ_IMPLEMENTATION
#define NS_IMPLEMENTATION
#include "nanoserv.h"
#define NJ_INTEGRAL_ARRAY_DEFINITIONS
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

//void json_parse_A(Node* node, void* out_raw, size_t elem_size) { _Bool field_marked[256] = {0}; A* out = (A*)out_raw; for(size_t i = 0; i < node->children.count; i++){ if(!field_marked[0] && strcmp(node->children.start[i]->key, "b") == 0) { json_parse_integral(node->children.start[i], (void*)&out->b, sizeof(out->b)); field_marked[0] = 1; } if(!field_marked[1] && strcmp(node->children.start[i]->key, "c") == 0) { json_parse_string(node->children.start[i], (void*)&out->c, sizeof(out->c)); field_marked[1] = 1; } if(!field_marked[2] && strcmp(node->children.start[i]->key, "d") == 0) { json_parse__Bool(node->children.start[i], (void*)&out->d, sizeof(out->d)); field_marked[2] = 1; } } } inline void json_parse_nj_array_A(Node* node, void* out_raw, size_t elem_size) { if(node->type == FIELD){ json_parse_nj_array_A(node->children.start[0], out_raw, elem_size); return; } (void)( (!!(node->type == ARRAY && "Non array passed to array parser")) || (_wassert(L"node->type == ARRAY && \"Non array passed to array parser\"", L"C:\\Users\\Leon\\Downloads\\cserver\\nanoserv\\serv.c", (unsigned)(12)), 0) ); for(size_t i = 0; i < node->children.count; i++) json_parse_A(node->children.start[i], (void*)(*(char**)((char*)out_raw+i*sizeof(A))), sizeof(A)); }

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

int main(int argc, char** argv){

    arena_allocator arena;
    n_arena_allocator_init(&arena, 256);
    char* al = arena.alloc(arena.context, 32);
    strcpy(al, "This is a string");
    int* somenum = arena.alloc(arena.context, sizeof(int));
    *somenum = 7;
    printf("%s, %d, %p\n", al, *somenum, al);
    al = arena.realloc(arena.context, al, 64);
    strcat(al, "maybe");
    printf("%s, %d, %p\n", al, *somenum, al);

    uint64_t* somenum2 = arena.alloc(arena.context, sizeof(uint64_t));
    *somenum2 = 10;
    printf("%s, %zu, %p\n", al, *somenum2, somenum2);

    const char* filename = "./json_check/r.json";
    FILE* file = fopen(filename, "r");
    char buffer[4096] = {0};
    size_t r = fread(buffer, 1, sizeof(buffer), file);
    buffer[r] = '\0';
    int errc = -1;
    Node* parent = create_nodes_from_parent(buffer, strlen(buffer), &errc);
    if(errc != -1){
        printf("Error on parsing: %d", errc);
        return 1;
    }
    buffer[0] = '\0';
    r = output_node(parent, buffer, 4096, 0, 0, 2);
    buffer[r] = '\0';
    printf("%s", buffer);
    fclose(file);

    memset(buffer, 0, sizeof(buffer));
    file = fopen("./json_check/a.json", "r");
    size_t r2 = fread(buffer, 1, sizeof(buffer), file);
    parent = create_nodes_from_parent(buffer, r2, &errc);
    if(errc != -1){
        printf("Error on parsing: %d", errc);
        return 1;
    }
    char some[128];
    A a_to_be_parsed = {.c=some};
    NJ_PARSE_NODE(A, parent, &a_to_be_parsed);
    printf("%s\n", a_to_be_parsed.c);

    a_to_be_parsed.b = 43;
    B b = {.a=&a_to_be_parsed, .c=22};
    Node node = {.type=OBJECT};
    NJ_SERIALIZE_NODE(B, &node, &b);
    memset(buffer, 0, 4096);
    r = output_node(&node, buffer, 4096, 0, 0, 2);
    buffer[r] = '\0';
    printf("%s\n\n", buffer);

    node = (Node){.type=OBJECT};
    C c1 = {.a=true, .b=3}; C c2 = {.a=false, .b=6}; C c3 = {.a=true, .b=2};
    D d = {.e="abc"};
    VEC_Push(d.c_vec, &c1); VEC_Push(d.c_vec, &c2); VEC_Push(d.c_vec, &c3);
    NJ_SERIALIZE_NODE(D, &node, &d);
    memset(buffer, 0, 4096);
    r = output_node(&node, buffer, 4096, 0, 0, 2);
    buffer[r] = '\0';
    printf("%s\n", buffer);

    Server* serv = ns_create_server(NULL, 3000, true);
    if(serv == NULL){
        fprintf(stderr, "Failed to initialize server\n");
        return 1;
    }
    ns_request(serv, GET, "/a", gen_handle);
    ns_start_server(serv);
    return 0;
}