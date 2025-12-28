#pragma once
#include <stdio.h>

#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "nanocommon.h"

#ifdef _WIN32
#define WIN32_MEAN_AND_LEAN
#define SOCKET_SHUTDOWN_BOTH_DIRECTIONS SD_BOTH
#include <io.h>
#include <winsock2.h>
#undef DELETE


#else
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
typedef int SOCKET;
#define SOCKET_SHUTDOWN_BOTH_DIRECTIONS SHUT_RDWR

#endif

#ifndef SO_REUSEPORT
    #define SO_REUSEPORT 0 // disgusting fix
#endif

#ifndef _Noreturn
#define _Noreturn __declspec(noreturn)
#endif
#include <threads.h>

#define MAX_HEADERS 64
#define MAX_FD_SET 64
#define MAX_SOCKETS 
#define MAX_READ 8192
#ifndef NS_THREAD_POOL_SIZE
#define NS_THREAD_POOL_SIZE 10
#endif

#ifdef NS_DEBUG
#define LOG_DEBUG(x, ...) printf(x, __VA_ARGS__)
#else
#define LOG_DEBUG(x, ...)
#endif


#define CODE_TO_NAME(status_code) (((status_code*99999)%166)-3) // some magix

static const char* status_names[] = {"Service Unavailable", "", "Unavailable For Legal Reasons", "Loop Detected", "", "", "", "Not Found", "", "",
     "Conflict", "Multiple Choices", "", "URI Too Long", "Use Proxy", "", "", "", "Created", "Failed Dependency", "", "Partial Content",
      "Too Many Requests", "", "", "Processing", "", "", "", "", "", "", "Not Implemented", "IM Used", "", "Variant Also Negotiates", "", "",
       "Network Authentication Required", "Payment Required", "", "", "Proxy Authentication Required", "", "", "Precondition Failed", "See Other",
        "", "Expectation Failed", "Permanent Redirect", "", "Unprocessable Entity", "", "No Content", "", "", "", "Continue", "", "", "", "", "", "",
         "", "", "", "Gateway Timeout", "", "", "", "Bad Request", "", "", "Method Not Allowed", "", "", "Gone", "Moved Permanently", "",
          "Unsupported Media Type", "Switch Proxy", "", "", "", "Accepted", "Too Early", "", "Multi-Status", "", "", "", "Early Hints", "",
           "Early Hints", "Processing", "Switching protocols", "Continue", "", "Bad Gateway", "", "", "Insufficient Storage", "", "", "",
            "Forbidden", "", "", "Request Timeout", "", "", "Payload Too Large", "Not Modified", "", "I'm a Teapot", "", "OK", "Locked", "",
             "Reset Content", "Precondition Required", "", "", "Switching protocols", "", "", "", "", "", "", "Internal Server Error", "", "",
             "", "Range Not Satisfiable", "Temporary Redirect", "", "Unauthorized", "", "Non-Authoritative Information", "Upgrade Required",
             "HTTP Version Not Supported", "", "", "Not Extended", "Misdirected Request", "", "", "Not Acceptable", "", "", "Length Required", "Found",
                "OK", "Already Reported", "Request Header Fields Too Large"};

static const char* content_types[] = {"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "application/javascript", "", "", "", "", "application/zip", "", "", "text/plain", "", "", "", "", "", "", "text/css", "", "", "", "application/vnd.rar", "text/csv", "", "", "application/xml", "image/svg+xml", "application/x-tar", "audio/wav", "", "", "", "", "image/jpeg", "", "", "font/ttf", "image/png", "", "image/x-icon", "application/octet-stream", "application/octet-stream", "application/vnd.ms-fontobject", "application/json", "", "application/wasm", "", "", "application/pdf", "", "", "", "image/gif", "application/ogg", "application/x-7z-compressed", "", "text/html", "", "", "", "", "", "", "image/jpeg", "", "", "video/webm", "font/woff", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "audio/mpeg", "", "", "", "", "", "", "video/mp4", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""};

static const char* possible_methods[] = {"GET", "POST", "UPDATE", "PATCH", "DELETE", "PUT"};

//SUPPORT FOR HTTP STATUS CODE NAMES

typedef enum HTTPMethod {
    GET, 
    POST,
    UPDATE,
    PATCH,
    DELETE,
    PUT,
    OPTIONS,
    ALL_METHODS,
    METHOD_UNKNOWN
} HTTPMethod;

typedef enum {
    NS_CLIENT_RESULT_ERROR = 0,
    NS_CLIENT_EMPTY = 1,
    NS_CLIENT_WOULD_BLOCK = 2,
    NS_CLIENT_SUCCESSFUL = 3
} ClientResult;

typedef struct Header {
    const char* key;
    const char* value;
} Header;

typedef struct HTTPRequest {
    HTTPMethod method;
    A_VEC(Header) headers;
    const char* path;
    const char* body;
} HTTPRequest;

typedef void(*body_dealloc_t)(void*);

typedef struct HTTPResponse {
    int client_socket;
    A_VEC(Header) headers;
    int status_code;
    const char* content_type;
    const char* body;
    size_t body_length;
    bool borrowed;
    body_dealloc_t body_dealloc;

} HTTPResponse;

typedef void* route_handler_param;
typedef void(*request_handler_t)(const HTTPRequest*, HTTPResponse*, route_handler_param);

typedef struct RouteHandler {
    const char* match;
    HTTPMethod method;
    request_handler_t handler;
    route_handler_param param;
    bool middle_route_handler;
} RouteHandler;


typedef struct Server {
    struct sockaddr_in address;
    int port;
    SOCKET listening_socket;
    A_VEC(SOCKET) alive_sockets;
    uint64_t last_communication_time;
    A_VEC(RouteHandler) handlers;
    bool shutdown_pending;

    A_VEC(SOCKET) handler_params_queue;
    A_VEC(SOCKET) sockets_to_remove;
    mtx_t handler_params_m;
    mtx_t server_handler_m;
    cnd_t server_cv;
} Server;

// API

void ns_free_header(Header* header);
Server* ns_create_server(const char* server_address, int port, bool server_socket_non_blocking);
void ns_stop_server(Server* server);
void ns_destroy_server(Server* server);
void ns_request(Server* server, HTTPMethod method, const char* match, request_handler_t handler, route_handler_param param);
void ns_middle_handler(Server* server, HTTPMethod method, const char* match, request_handler_t handler, route_handler_param param);
void ns_serve_directory(Server* server, const char* match, const char* directory_path);
void ns_serve_file(Server* server, const char* match, const char* path);
char** ns_parse_query_params(const HTTPRequest* request, char* buffer, size_t max_buffer_size, size_t max_params);
long ns_calc_sleep(Server* server);
void ns_listen_incoming(Server* server);
bool ns_breakdown_request(HTTPRequest* req, SOCKET socket, char* rbuff, size_t count);
char* ns_glue_response(HTTPResponse* res, size_t* response_size);
void ns_send_response(HTTPResponse* res);
HTTPResponse* ns_borrow_response(HTTPResponse* res); // gives up memory control of the response to the caller
void ns_set_response_body(HTTPResponse* res, const char* body, const char* content_type);
void ns_set_response_body_status(HTTPResponse* res, const char* body, const char* content_type, int status);
void ns_free_request_response(HTTPRequest* req, HTTPResponse* res);
RouteHandler* ns_match_handler(Server* server, const HTTPRequest* req);
ClientResult ns_handle_client(Server* server, SOCKET client_socket); // socket is ready for rw
void ns_put_header(HTTPResponse* res, const char* key, const char* value);
const char* ns_get_header(const HTTPRequest* res, const char* key);
void ns_start_server(Server* server);
void socket_startup();
void close_socket(SOCKET socket);

#ifdef NS_IMPLEMENTATION
#ifndef NS_IMPLEMENTATION_GUARD
#define NS_IMPLEMENTATION_GUARD

void socket_startup(){
    #ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(0x0202, &wsaData);
    #endif
}

void close_socket(SOCKET socket){
    #ifdef _WIN32
        closesocket(socket);
    #else
        close(socket);
    #endif
}

void sleep_msec(int msec){
    #ifdef _WIN32
        Sleep(msec);
    #else
        usleep(msec*1000);
    #endif
}

bool handle_socket_error(){
    int err = -1;
    #ifdef _WIN32
        err = WSAGetLastError();
        char *s = NULL;
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 
                    NULL, err,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPSTR)&s, 0, NULL);
        LOG_DEBUG("Socket error: %s\n", s);
        LocalFree(s);
        return err != WSAEWOULDBLOCK;
    #else
        err = errno;
        LOG_DEBUG("Socket error: %s\n", strerror(err));
        return err != EAGAIN && err != EWOULDBLOCK;
    #endif
}

int set_non_blocking(SOCKET socket){
    #ifdef _WIN32
        u_long mode = 1; // 1 for non-blocking, 0 for blocking
        return ioctlsocket(socket, FIONBIO, &mode);
    #else
        int flags = fcntl(socket, F_GETFL, 0);
        flags |= O_NONBLOCK;
        return fcntl(socket, F_SETFL, flags);
    #endif
}

uint64_t get_system_time(){
    #ifdef _WIN32
        return GetTickCount64();
    #else
        struct timeval te;
        gettimeofday(&te, NULL); // Get current time
        uint64_t milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000; // Calculate milliseconds
        return milliseconds;
    #endif
}



void ns_free_header(Header* header){
    free((void*)header->key); free((void*)header->value);
}

Server* ns_create_server(const char* server_address, int port, bool server_socket_non_blocking){
    socket_startup();
    SOCKET listen_socket;
    struct sockaddr_in address;
    #ifdef _WIN32
        char opt = 1;
    #else
        int opt = 1;
    #endif
    if ((listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) < 0) {
        LOG_DEBUG("Failed to set socket %d", (int)listen_socket);
        handle_socket_error();
        return NULL;
    }
    if (setsockopt(listen_socket, SOL_SOCKET,
                   SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt))) {
        LOG_DEBUG("Failed to set socket option %d, ", (int)listen_socket);
        handle_socket_error();
        return NULL;
    }

    if(server_socket_non_blocking)
        if(set_non_blocking(listen_socket)){
            LOG_DEBUG("Failed to set server socket %d as non blocking, ", (int)listen_socket);
            handle_socket_error();
            return NULL;
        }

    address.sin_family = AF_INET;
    if(server_address == NULL)
        address.sin_addr.s_addr = INADDR_ANY;
    else {
        address.sin_addr.s_addr = inet_addr(server_address);
        if(address.sin_addr.s_addr == ((uint32_t)-1)){
            LOG_DEBUG("Invalid address passed as parameter %s", server_address);
            return NULL;
        }
    }
    address.sin_port = htons(port);

    if (bind(listen_socket, (struct sockaddr*)&address,
             sizeof(address))
        < 0) {
        LOG_DEBUG("Failed to bind to listening socket %d, ", (int)listen_socket);
        handle_socket_error();
        return NULL;
    }
    if (listen(listen_socket, 3) < 0) {
        LOG_DEBUG("Failed to start socket listen %d, ", (int)listen_socket);
        handle_socket_error();
        return NULL;
    }
    Server* server = (Server*)malloc(sizeof(Server));
    memset((void*)server, 0, sizeof(Server));
    memcpy((void*)&server->address, (void*)&address, sizeof(struct sockaddr_in));
    server->port = port;
    server->last_communication_time = get_system_time();
    server->listening_socket = listen_socket;
    return server;
}

void ns_stop_server(Server* server){
    server->shutdown_pending = true;
}

void ns_destroy_server(Server* server){
    if(server->listening_socket)
        close_socket(server->listening_socket);
    for(size_t i = 0; i < server->alive_sockets.count; i++)
        close_socket(server->alive_sockets.start[i]);
    VEC_Free(server->alive_sockets);
    VEC_Free(server->handlers);
    VEC_Free(server->handler_params_queue);
    VEC_Free(server->sockets_to_remove);
    free(server);
}


void ns_request(Server* server, HTTPMethod method, const char* match, request_handler_t handler, route_handler_param param){
    RouteHandler r = {.match = match, .method = method, .handler = handler, .param=param};
    VEC_Push(server->handlers, &r);
}

void ns_middle_handler(Server* server, HTTPMethod method, const char* match, request_handler_t handler, route_handler_param param){
    RouteHandler r = {.match = match, .method = method, .handler = handler, .param=param, .middle_route_handler=true};
    VEC_Push(server->handlers, &r);
}

char** ns_parse_query_params(const HTTPRequest* request, char* buffer, size_t max_buffer_size, size_t max_params){
    memset(buffer, 0, max_buffer_size);
    size_t offset = 0;
    size_t path_l = strlen(request->path);
    size_t allocated = 0;
    for(; offset < path_l && request->path[offset] != '?'; offset++);
    if(offset == path_l)
        return NULL;
    offset += 1;
    size_t total_params = 0;
    for(size_t i = 0; offset + i < path_l; i++)
        if(request->path[offset + i] == '=')
            total_params += 1;
    total_params = total_params < max_params ? total_params : max_params;
    
    char* actual_buffer_start = buffer + ((2 * total_params) + 1)*sizeof(char**);
    max_buffer_size -= (actual_buffer_start - buffer);
    char* buffer_position = actual_buffer_start;

    char** params_position = (char**)buffer;

    const char* cur = request->path + offset;

    while(cur < request->path + path_l){
        const char* key_start = cur;
        for(; cur < request->path + path_l && (*cur != '='); cur++);
        if((buffer_position + (cur+1 - key_start)) - actual_buffer_start >= (ptrdiff_t)max_buffer_size){
            break;
        }
        memcpy(buffer_position, key_start, cur-key_start);
        buffer_position[cur-key_start] = '\0';
        params_position[allocated++] = buffer_position;

        buffer_position += cur-key_start+1;
        const char* val_start = cur+1;
        for(; cur < request->path + path_l && (*cur != '&'); cur++);
        if((buffer_position + (cur - val_start)) - actual_buffer_start >= (ptrdiff_t)max_buffer_size){
            allocated--;
            break;
        }
        memcpy(buffer_position, val_start, cur-val_start);
        buffer_position[cur-val_start] = '\0';
        params_position[allocated++] = buffer_position;

        buffer_position += cur-val_start+1;
        cur += 1;
    }
    params_position[allocated] = NULL;
    return (char**)buffer;
}

long ns_calc_sleep(Server* server){
    uint64_t current_time = get_system_time();
    if(current_time - server->last_communication_time < 64)
        return 0;
    
    return (current_time - server->last_communication_time) < 16384 ? (current_time - server->last_communication_time) : 16384;
}

void ns_listen_incoming(Server* server){
    struct timeval tv = {0};
    #ifdef _WIN32
        fd_set readfds;
        FD_ZERO(&readfds); 
        FD_SET(server->listening_socket, &readfds);
    #else
        struct pollfd readfds = (struct pollfd){.fd=server->listening_socket, .events=POLLIN, .revents=0};
    #endif

    tv.tv_sec = 0;
    tv.tv_usec = ns_calc_sleep(server); //calc_sleep(server); // Server listening socket is blocking, to avoid total busywaiting

    mtx_lock(&server->server_handler_m);
    if(server->sockets_to_remove.count > 0){
        for(int i = 0; i < (int)server->sockets_to_remove.count; i++){
            bool found_socket = false;
            for(size_t j = 0; j < server->alive_sockets.count; j++){
                if(server->sockets_to_remove.start[i] == server->alive_sockets.start[j]){
                    shutdown(server->alive_sockets.start[j], SOCKET_SHUTDOWN_BOTH_DIRECTIONS);
                    close_socket(server->alive_sockets.start[j]); 
                    VEC_Remove(server->sockets_to_remove, (size_t)i);
                    VEC_Remove(server->alive_sockets, j);
                    found_socket = true;
                    break;
                }
            }
            if(!found_socket){
                VEC_Remove(server->sockets_to_remove, (size_t)i);
                i--;
            }
        }
    }
    mtx_unlock(&server->server_handler_m);

    #ifdef _WIN32
    int retval = select(server->listening_socket + 1, &readfds, NULL, NULL, &tv);
    if(retval > 0){
    #else
        int retval = poll(&readfds, 1, (int)(tv.tv_usec > 16 ? 16 : tv.tv_usec));
        if(retval > 0){
    #endif
        SOCKET new_sock = accept(server->listening_socket, NULL, NULL);
        retval = set_non_blocking(new_sock);
        if(retval < 0){
            handle_socket_error();
            return;
        }
        VEC_Push(server->alive_sockets, &new_sock);
        // assert(server->alive_sockets.count < MAX_FD_SET); // CHANGE: splitting FDSETS by threads - currently busy waiting
    } else if (retval == 0){
//        sleep_msec(5); // 16msec or so on Win32 (without changing tick duration)
    } else {
        handle_socket_error();
    }
}

// probably place these somewhere else
char* str_space(char* str, size_t length, size_t* new_length){
    size_t i = 0;
    for(; i < length && str[i] != '\0' && !isspace(str[i]); i++);
    if(new_length != NULL)
        *new_length = length - i;
    return str + i;
}

char* str_word(char* str, size_t length, size_t* new_length){
    size_t i = 0;
    for(; i < length && str[i] != '\0' && isspace(str[i]); i++);
    if(new_length != NULL)
        *new_length = length - i;
    return str + i;
}

char* str_search(char search_char, char* str, size_t length, size_t* new_length){
    size_t i = 0;
    for(; i < length && str[i] != '\0' && str[i] != search_char; i++);
    if(new_length != NULL)
        *new_length = length - i;
    return str + i;
}

bool ns_breakdown_request(HTTPRequest* req, SOCKET socket, char* rbuff, size_t count){
    HTTPMethod method = METHOD_UNKNOWN;
    size_t cur_length = count;
    char* method_word = str_space(rbuff, cur_length, &cur_length);
    for(size_t i = 0; i < sizeof(possible_methods)/sizeof(possible_methods[0]); i++)
        if(strncmp(rbuff, possible_methods[i], method_word-rbuff) == 0)
            method = (HTTPMethod)i;

    if(method == METHOD_UNKNOWN || cur_length == 0){
        LOG_DEBUG("Unknown HTTP Method detected on socket %d", (int)socket);
        return false;
    }
    req->method = method;
    method_word = str_word(method_word, cur_length, &cur_length);

    char* query_word_end = str_space(method_word, cur_length, &cur_length);

    if(query_word_end-method_word == 0){
        LOG_DEBUG("No path passed to socket %d", (int)socket);
        return false;
    }
    char* path = (char*)malloc(query_word_end-method_word+1); // ALLOCATION
    copystrn(path, method_word, query_word_end-method_word+1);
    req->path = path;
    
    query_word_end = str_word(query_word_end, cur_length, &cur_length);
    char* http_version_end = str_space(query_word_end, cur_length, &cur_length);
    if(http_version_end-query_word_end == 0){
        LOG_DEBUG("No http headers passed to socket %d", (int)socket);
        return false;
    }
    //headers until blank line - may begin with blank line
    bool blank_line_found = false;
    char* cur_position = str_word(http_version_end, cur_length, &cur_length);
    while(!blank_line_found && cur_length > 0){
        char* header_name_end = str_search(':', cur_position, cur_length, &cur_length);
        cur_length -= 1;
        char* header_value_start = str_word(header_name_end+1, cur_length, &cur_length);
        char* header_value_end = str_search('\n', header_value_start, cur_length, &cur_length);

        char* key = (char*)malloc(header_name_end-cur_position+1);
        copystrn(key, cur_position, header_name_end-cur_position+1);
        assert(header_name_end-cur_position+1 > 0);
        
        char* value = (char*)malloc(header_value_end-header_value_start+1);
        assert(header_value_end-header_value_start+1 > 0);
        size_t copy_total = header_value_end-header_value_start;
        copystrn(value, header_value_start, header_value_end[-1] == '\r' ? copy_total : copy_total+1);

        Header h = {.key=key, .value=value};
        VEC_Push(req->headers, &h);

        //bool word_found = false;
        for(size_t i = 1; i < cur_length; i++)
            if(!isspace(header_value_end[i])){
                cur_position = (header_value_end + i);
                cur_length -= i;
                break;
            } else if(header_value_end[i] == '\n'){ // an empty line
                cur_position = str_word(header_value_end+i, cur_length-i, &cur_length);
                blank_line_found = true;
                break;
            }
    }

    if(cur_length == 0 || cur_length >= (1 << 31)){
        req->body = NULL;
        return true; // nobody
    } else {
        char* body = (char*)malloc(cur_length+1); // ALLOCATION
        copystrn(body, cur_position, cur_length+1);
        req->body = body;
    }
    return true;
}

char* ns_glue_response(HTTPResponse* res, size_t* response_size){
    size_t content_length = res->body != NULL ? res->body_length != 0 ? res->body_length : strlen(res->body) : 0;
    size_t content_type_length = res->content_type != NULL ? strlen(res->content_type) : 0;
    size_t approx_size = res->headers.count * 512 + content_type_length + content_length + 64;
    char* glued_resp = (char*)malloc(approx_size); // ALLOCATION
    size_t cur_loc = 0;
    cur_loc += snprintf(glued_resp + cur_loc, 48, "HTTP/1.1 %d %s\r\n", res->status_code, status_names[CODE_TO_NAME(res->status_code)]);
    cur_loc += snprintf(glued_resp + cur_loc, 48, "Content-Length: %zd\r\n", content_length);
    if(content_type_length > 0)
        cur_loc += snprintf(glued_resp + cur_loc, 32 + content_type_length, "Content-Type: %s\r\n", res->content_type);
    for(size_t i = 0; i < res->headers.count; i++)
        cur_loc += snprintf(glued_resp + cur_loc, 512, "%s: %s\r\n", res->headers.start[i].key, res->headers.start[i].value);
    
    glued_resp[cur_loc++] = '\r';
    glued_resp[cur_loc++] = '\n';
    if(content_length > 0){
        memcpy(glued_resp + cur_loc, res->body, content_length);
        glued_resp[cur_loc + content_length] = '\0';
    }
    *response_size = cur_loc + content_length;
    return glued_resp;
}  

void ns_send_response(HTTPResponse* res){
    size_t response_size = 0;
    char* full_resp = ns_glue_response(res, &response_size);
    int r = send(res->client_socket, full_resp, response_size, 0);
    free(full_resp); // ALLOCATION
    if(r < 0){
        handle_socket_error();
    }
}

HTTPResponse* ns_borrow_response(HTTPResponse* res){ // gives up memory control of the response to the caller
    res->borrowed = true;
    HTTPResponse* new_resp = (HTTPResponse*)malloc(sizeof(HTTPResponse));
    memcpy(new_resp, res, sizeof(HTTPResponse));
    return new_resp;
}

void ns_set_response_body(HTTPResponse* res, const char* body, const char* content_type){
    res->content_type = content_type;
    res->body = body;
}

void ns_set_response_body_status(HTTPResponse* res, const char* body, const char* content_type, int status){
    ns_set_response_body(res, body, content_type);
    res->status_code = status;
}

void ns_free_request_response(HTTPRequest* req, HTTPResponse* res){
    if(req != NULL){
        if(req->body != NULL)
            free((void*)req->body);
        free((void*)req->path);
        for(size_t i = 0; i < req->headers.count; i++){
            ns_free_header(&req->headers.start[i]);
        }
        VEC_Free(req->headers);
    }
    if(res != NULL){
        if(res->body != NULL && res->body_dealloc != NULL)
            res->body_dealloc((void*)res->body);
        
        for(size_t i = 0; i < res->headers.count; i++){
            ns_free_header(&res->headers.start[i]);
        }
        VEC_Free(res->headers);
    }
}

RouteHandler* ns_match_handler(Server* server, const HTTPRequest* req){
    for(size_t i = 0; i < server->handlers.count; i++){
        if(server->handlers.start[i].middle_route_handler)
            continue;
        if(server->handlers.start[i].method != req->method && server->handlers.start[i].method != ALL_METHODS)
            continue;
        if(server->handlers.start[i].match == NULL || server->handlers.start[i].match[0] != req->path[0])
            continue;
        const char* until_wildcard = strchr(server->handlers.start[i].match, '*');
        size_t l = until_wildcard - server->handlers.start[i].match;
        if(until_wildcard == NULL)
            l = strlen(server->handlers.start[i].match);
        if(strncmp(server->handlers.start[i].match, req->path, l) == 0)
            return (server->handlers.start + i);
    }
    return NULL;
}

void ns_call_all_middle_route_handlers(Server* server, const HTTPRequest* req, HTTPResponse* res){
    for(size_t i = 0; i < server->handlers.count; i++){
        if(!server->handlers.start[i].middle_route_handler)
            continue;
        if(server->handlers.start[i].method != req->method && server->handlers.start[i].method != ALL_METHODS)
            continue;
        if(server->handlers.start[i].match == NULL || server->handlers.start[i].match[0] != req->path[0])
            continue;
        const char* until_wildcard = strchr(server->handlers.start[i].match, '*');
        size_t l = until_wildcard - server->handlers.start[i].match;
        if(until_wildcard == NULL)
            l = strlen(server->handlers.start[i].match);
        if(strncmp(server->handlers.start[i].match, req->path, l) == 0)
            server->handlers.start[i].handler(req, res, server->handlers.start[i].param);
    }
}


ClientResult ns_handle_client(Server* server, SOCKET client_socket){ // socket is ready for rw
    HTTPRequest req = {0};
    HTTPResponse res = {.client_socket=client_socket, .body_dealloc=free};
    ClientResult c_r = NS_CLIENT_SUCCESSFUL;
    
    char initial_read_buffer[MAX_READ+1];
    char* rbuff = initial_read_buffer;
    int r = recv(client_socket, rbuff, MAX_READ, 0);
    if(r < 0){
        bool e = handle_socket_error();
        return e ? NS_CLIENT_RESULT_ERROR : NS_CLIENT_WOULD_BLOCK;
    }
    else if(r == 0){
        LOG_DEBUG("Empty read on socket %d", (int)client_socket);
        return NS_CLIENT_EMPTY;
    }
    else if(r > 0){
        size_t cur_read = MAX_READ;
        if(r >= cur_read){
            rbuff = NC_ALLOCATE(cur_read*2+1);
            strncpy(rbuff, initial_read_buffer, MAX_READ);
            r = recv(client_socket, rbuff + r, cur_read, 0);
            while(r >= cur_read){
                cur_read *= 2;
                rbuff = NC_REALLOCATE(rbuff, cur_read*2+1);
                r = recv(client_socket, rbuff+cur_read, cur_read, 0);
            }
            cur_read += r;
        } else {
            cur_read = r;
        }
        rbuff[cur_read] = '\0';

        if(!ns_breakdown_request(&req, client_socket, rbuff, (size_t)cur_read)){
            c_r = NS_CLIENT_RESULT_ERROR;
            goto post_request_process;
        }

        ns_call_all_middle_route_handlers(server, &req, &res);
        RouteHandler* handle = ns_match_handler(server, &req);
        if(handle == NULL){
            LOG_DEBUG("Received HTTP request without appropriate handler on socket %d", (int)client_socket);
            c_r = NS_CLIENT_RESULT_ERROR;
            goto post_request_process;
        }
        handle->handler(&req, &res, handle->param);
        if(!res.borrowed){ // if it was borrowed - then the caller has to send it
            ns_send_response(&res);
            ns_free_request_response(&req, &res);
        }
    }

    post_request_process:
    if(rbuff != initial_read_buffer)
        NC_FREE(rbuff);
    return c_r;
}

void ns_put_header(HTTPResponse* res, const char* key, const char* value){
    size_t keyl = strlen(key);
    char* nk = (char*)malloc(keyl+1); // ALLOCATION
    copystrn(nk, key, keyl+1);
    
    size_t val_l = strlen(value);
    char* nv = (char*)malloc(val_l+1); // ALLOCATION
    copystrn(nv, value, val_l+1);
    Header h = {.key=nk, .value=nv};
    VEC_Push(res->headers, &h);
}

const char* ns_get_header(const HTTPRequest* res, const char* key){
    for(size_t i = 0; i < res->headers.count; i++)
        if(strcmp(res->headers.start[i].key, key) == 0)
            return res->headers.start[i].value;
    return NULL;
}

static const char* ns_file_extension_content_type(const char* ext){
	size_t start = 0;
	for(size_t i = 0; ext[i] != '\0'; i++){
		start += ((uint8_t)ext[i] ^ (uint8_t)101 + 22);
    }
    start = start % (sizeof(content_types)/sizeof(content_types[0]));
    return content_types[start];
}


static void ns_file_handler(const HTTPRequest* req, HTTPResponse* res, route_handler_param param){
    const char* filename = (const char*)param;
    size_t l = 0;
    if(filename == NULL || (l = strnlen(filename, 256)) < 1){
        res->status_code = 404;
        return;
    }
    
    FILE* file = fopen(filename, "rb"); // IMPLEMENTATION: can make it so filesize is read upon configuration and stored as a param
    if(file == NULL){
        res->status_code = 404;
        return;
    }
    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0L, SEEK_SET);

    res->body = (const char*)malloc(file_size+1);
    fread((void*)res->body, sizeof(char), file_size, file);
    ((char *)res->body)[file_size] = '\0';
    fclose(file);

    char* extension = (char*)filename + l - 1;
    for(; *extension != '.'; --extension);
    const char* content_type = ns_file_extension_content_type(extension);
    if(strlen(content_type) == 0){
        content_type = "application/octet-stream";
    }
    res->body_length = file_size;
    res->content_type = content_type;
    res->status_code = 200;
}

typedef struct { const char* match; const char* dirn; } ns_directory_params;

static void ns_directory_handler(const HTTPRequest* req, HTTPResponse* res, route_handler_param param){
    ns_directory_params* directory_params = (ns_directory_params*)param;
    if(directory_params == NULL){
        res->status_code = 404;
        return;
    }

    if(strstr(req->path, "..") != NULL){
        res->status_code = 400;
        return;
    }

    char* until_wildcard = strchr(directory_params->match, '*');
    if(until_wildcard == NULL){
        res->status_code = 404;
        return;
    }
    const char* directory_name_start = req->path + (until_wildcard - directory_params->match);
    char path[256];
    path[0] = '\0';
    size_t l1 = strnlen(directory_params->dirn, sizeof(path)); size_t l2 = strnlen(directory_name_start, sizeof(path));
    if(l1 + l2 >= sizeof(path)){
        res->status_code = 500;
        return;
    }
    strncat(path, directory_params->dirn, l1+1);
    strncat(path, directory_name_start, l2+1);
    ns_file_handler(req, res, (route_handler_param)path);
}


void ns_serve_directory(Server* server, const char* match, const char* directory_path){
    ns_directory_params* directory_params = (ns_directory_params*)malloc(sizeof(ns_directory_params));
    *directory_params = (ns_directory_params){.match=match, .dirn=directory_path};
    assert(strchr(match, '*') != NULL && "Directory match must include a wildcard to allow for filenames");
    RouteHandler r = {.match = match, .method = GET, .handler = ns_directory_handler, .param=(route_handler_param)directory_params};
    VEC_Push(server->handlers, &r);
}

void ns_serve_file(Server* server, const char* match, const char* path){
    RouteHandler r = {.match = match, .method = GET, .handler = ns_file_handler, .param=(route_handler_param)path};
    VEC_Push(server->handlers, &r);
}

int ns_run_thread_pool(void* server_arg){
    Server* server = (Server*)server_arg;
    if(server == NULL)
        return 1;
    
    while(true){
        struct timespec tms = (struct timespec){.tv_sec=time(NULL)+30};
        mtx_lock(&server->handler_params_m);
        if(server->handler_params_queue.count > 0){
            SOCKET client_socket = server->handler_params_queue.start[server->handler_params_queue.count-1];
            #ifdef _WIN32
                LOG_DEBUG("th handle %d", thrd_current()._Tid);
            #else
                LOG_DEBUG("th handle %ld", server->handler_params_queue.count);
            #endif

            ClientResult cr = ns_handle_client(server, client_socket);
            if(cr == NS_CLIENT_RESULT_ERROR || cr == NS_CLIENT_EMPTY){ // destroy the socket
                mtx_lock(&server->server_handler_m);
                VEC_Push(server->sockets_to_remove, &client_socket);
                mtx_unlock(&server->server_handler_m);
            }
            (&server->handler_params_queue)->count = server->handler_params_queue.count-1;
        }
        mtx_unlock(&server->handler_params_m);
        mtx_lock(&server->server_handler_m);
        cnd_timedwait(&server->server_cv, &server->server_handler_m, &tms);
        if(server->shutdown_pending){
            mtx_unlock(&server->server_handler_m);
            return 0;
        }
        mtx_unlock(&server->server_handler_m);
    }
    return 1;
}


void ns_issue_handling_request(Server* server, SOCKET socket){
    mtx_lock(&server->handler_params_m);
    bool already_in_queue = false;
    for(size_t i = 0; i < server->handler_params_queue.count; i++){
        if(server->handler_params_queue.start[i] == socket){
            already_in_queue = true;
            break;
        }
    }
    if(!already_in_queue){
        VEC_Push(server->handler_params_queue, &socket);
        // if(server->handler_params_queue.count >= server->handler_params_queue.capacity) {
        //      server->handler_params_queue.capacity = server->handler_params_queue.capacity == 0 ? 8 : server->handler_params_queue.capacity * 1.5;
        //       server->handler_params_queue.start = realloc((void*)server->handler_params_queue.start,
        //        server->handler_params_queue.capacity*sizeof(*server->handler_params_queue.start));
        //     } 
        //     memcpy((void*)(server->handler_params_queue.start + server->handler_params_queue.count++), (void*)&socket, sizeof(*&socket));
    }
    mtx_unlock(&server->handler_params_m);
}

void ns_check_clients(Server* server){
    #ifdef _WIN32
        fd_set readfds;
    #else
        struct pollfd readfds[MAX_FD_SET];
    #endif
    struct timeval tv = {0};

    size_t cur_sock = 0;
    while(cur_sock < server->alive_sockets.count){
        #ifdef _WIN32
            FD_ZERO(&readfds);
        #else
            memset(readfds, 0, sizeof(readfds));
        #endif
        int selected_sockets = 0;
        size_t until_full_sock = cur_sock;
        // breakdown all listening sockets to max_fd chunks, and select each of them
        for(; until_full_sock < server->alive_sockets.count; until_full_sock++){
            #ifdef _WIN32
                FD_SET(server->alive_sockets.start[until_full_sock], &readfds);
            #else
                readfds[selected_sockets].fd = server->alive_sockets.start[until_full_sock];
                readfds[selected_sockets].events = POLLIN;
                readfds[selected_sockets].revents = 0;
            #endif
            selected_sockets++;
            if(selected_sockets >= MAX_FD_SET-1)
                break;
        }

        if(selected_sockets == 0)
            return;

        #ifdef _WIN32
            int retval = select(selected_sockets + 1, // PROBABLY SHOULD BE THE MAX SOCKET + 1
                            &readfds, NULL, NULL, &tv); // maybe it will break without a timeout
        #else
            int retval = poll(readfds, selected_sockets, 0);
        #endif
        if(retval > 0){
            server->last_communication_time = get_system_time();
            for(size_t i = cur_sock; i < until_full_sock && i < server->alive_sockets.count; i++){
                #ifdef _WIN32
                if(FD_ISSET(server->alive_sockets.start[i], &readfds)){
                #else
                if(readfds[i-cur_sock].revents & POLLIN){
                #endif
                    if(NS_THREAD_POOL_SIZE == 1){ // single thread
                        ClientResult cr = ns_handle_client(server, server->alive_sockets.start[i]);
                        if(cr == NS_CLIENT_EMPTY || cr == NS_CLIENT_RESULT_ERROR){ // destroy the socket || 
                            VEC_Push(server->sockets_to_remove, &server->alive_sockets.start[i]);
                        }
                    } else {
                        ns_issue_handling_request(server, server->alive_sockets.start[i]);
                        cnd_signal(&server->server_cv);
                    }
                }
                
            }
        } else if(retval < 0){
            handle_socket_error();
        }
        cur_sock = until_full_sock;
    }
}

void ns_start_server(Server* server) {
    int r1 = mtx_init(&server->server_handler_m, mtx_plain);
    int r2 = mtx_init(&server->handler_params_m, mtx_plain);
    int r3 = cnd_init(&server->server_cv);
    assert(r1 == thrd_success && r2 == thrd_success && r3 == thrd_success);
    thrd_t threads[NS_THREAD_POOL_SIZE] = {0};

    if(NS_THREAD_POOL_SIZE > 1)
        for(size_t i = 0; i < NS_THREAD_POOL_SIZE; i++){
            thrd_create(&threads[i], ns_run_thread_pool, server);
        }
    while(!server->shutdown_pending){
        ns_listen_incoming(server);
        ns_check_clients(server);
    }
    cnd_broadcast(&server->server_cv);
    if(NS_THREAD_POOL_SIZE > 1)
        for(size_t i = 0; i < NS_THREAD_POOL_SIZE; i++){
            int j_res;
            thrd_join(threads[i], &j_res);
        }
    cnd_destroy(&server->server_cv);
    mtx_destroy(&server->server_handler_m);
    mtx_destroy(&server->handler_params_m);
}

#endif
#endif