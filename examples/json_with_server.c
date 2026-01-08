#define NJ_IMPLEMENTATION
#define NJ_STRUCT_IMPLEMENTATION
#define NS_IMPLEMENTATION
#define NC_IMPLEMENTATION
#include "../nanojson.h"
#include "../nanoserv.h"

typedef char* string;
NJ_STRUCT(A, a, int, b, string, c, float);
// An alternative definition would be
// struct A { int a; char* b; float c;};
// NJ_DEFINE_PARSE_SERIALIZE(A, a, int, b, string, c, float);
// And in an header file you'd use NJ_DECLARE_PARSE_SERIALIZE(A)

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
    char* body = malloc(64);
    float d = (float)a.a+a.c;
    snprintf(body, 64, "%f:%s", d, a.b);
    ns_set_response_body_status(res, body, "application/json", 200); // response body is automatically freed by the server.
    // if you wish to use a different "free" method, modify res->body_dealloc and res->body_dealloc_context
    json_free(parent, true, &parser);
}


int main(){
    Server* serv = ns_create_server(NULL, 3000, true);
    if(serv == NULL){
        fprintf(stderr, "Failed to initialize server\n");
        return 1;
    }
    ns_request(serv, POST, "/a", post_a, NULL);
    ns_start_server(serv);
    // curl -X POST localhost:3000/a -d "{\"a\": 1, \"b\": \"abc\", \"c\": 2}"
}