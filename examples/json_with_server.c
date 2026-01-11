#define NJ_IMPLEMENTATION // for json
#define NJ_STRUCT_IMPLEMENTATION // for defining NJ_STRUCT related parsing and serializing functions, does not need to be in the same place as NJ_IMPLEMENTATION

#define NS_THREAD_POOL_SIZE 10 // Default value, 1 makes everything single threaded (otherwise we create new threads)
#define NS_IMPLEMENTATION // for server
#define NC_IMPLEMENTATION // Currently required - implementation of some helper functions and a basic arena allocator
#include "../nanojson.h"
#include "../nanoserv.h"

typedef char* string; // unfortunately required
NJ_STRUCT_WITH_VEC(A, a, int, b, string, c, float);
// An alternative definition that works with more complex types would be:
// struct A { int a; char* b; float c;};
// NJ_DEFINE_PARSE_SERIALIZE(A, a, int, b, string, c, float);
// NJ_DEFINE_PARSE_SERIALIZE_VECTOR(A);
// And in an header file you'd use NJ_DECLARE_PARSE_SERIALIZE(A) instead
NJ_STRUCT(B, as, NJ_VEC(A), e, int);

void post_a(const HTTPRequest* req, HTTPResponse* res, route_handler_param param){
    int err = -1;
    JsonParser parser; json_get_system_parser(&parser);
    JsonNode* parent = json_from_buffer(req->body, strlen(req->body), &err, &parser);
    if(err != -1){
        // json parser error occured
        ns_set_response_static_body_status(res, "Error: could not parse JSON", "text/plain", 400);
        return;
    }
    A a;
    NJ_PARSE_NODE(A, parent, &a, &parser);
    B b = (B){.e=3};
    VEC_Push(b.as, &a);
    VEC_Push(b.as, &a);
    VEC_Push(b.as, &a);

    char* body = malloc(512);
    float d = (float)a.a+a.c;
    int offset = snprintf(body, 512, "%f:%s,", d, a.b);

    // SERIALIZE B
    JsonNode b_node; 
    json_init_node(&b_node, OBJECT);
    NJ_SERIALIZE_NODE(B, &b_node, &b, &parser);
    json_output_node(&b_node, body + offset, 512 - offset, 0);

    ns_set_response_body_status(res, body, "application/json", 200); // response body is automatically freed by the server.
    // if you wish to use a different "free" method, modify res->body_dealloc and res->body_dealloc_context
    json_free(parent, true, &parser);
    json_free(&b_node, false, &parser);
    VEC_Free(b.as);
}


int main(){
    Server* serv = ns_create_server(NULL, 3000, true);
    if(serv == NULL){
        fprintf(stderr, "Failed to initialize server\n");
        return 1;
    }
    ns_serve_file(serv, "/example", "examples/json_with_server.c");
    ns_serve_directory(serv, "/ex_d*", "examples"); // only * is supported as a wildcard, NO REGEX
    ns_request(serv, POST, "/a", post_a, NULL);
    ns_start_server(serv);
    // BLOCKING 
    ns_stop_server(serv);
    ns_destroy_server(serv);
    // curl -X POST localhost:3000/a -d "{\"a\": 1, \"b\": \"abc\", \"c\": 2}"
}