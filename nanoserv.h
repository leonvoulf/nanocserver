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


#define CODE_TO_NAME(status_code) (((status_code*99999)%166)-3) // some magixs

const char* status_names[] = {"Service Unavailable", "", "Unavailable For Legal Reasons", "Loop Detected", "", "", "", "Not Found", "", "",
     "Conflict", "Multiple Choices", "", "URI Too Long", "Use Proxy", "", "", "", "Created", "Failed Dependency", "", "Partial Content",
      "Too Many Requests", "", "", "Processing", "", "", "", "", "", "", "Not Implemented", "IM Used", "", "Variant Also Negotiates", "", "",
       "Network Authentication Required", "Payment Required", "", "", "Proxy Authentication Required", "", "", "Precondition Failed", "See Other",
        "", "Expectation Failed", "Permanent Redirect", "", "Unprocessable Entity", "", "No Content", "", "", "", "Continue", "", "", "", "", "", "",
         "", "", "", "Gateway Timeout", "", "", "", "Bad Request", "", "", "Method Not Allowed", "", "", "Gone", "Moved Permanently", "",
          "Unsupported Media Type", "Switch Proxy", "", "", "", "Accepted", "Too Early", "", "Multi-Status", "", "", "", "Early Hints", "",
           "Early Hints", "Processing", "Switching protocols", "Continue", "", "Bad Gateway", "", "", "Insufficient Storage", "", "", "",
            "Forbidden", "", "", "Request Timeout", "", "", "Payload Too Large", "Not Modified", "", "I'm a Teapot", "", "OK", "Locked", "",
             "Reset Content", "Precondition Required", "", "", "Switching protocols", "", "", "", "", "", "", "Internal Server Error", "", "",
             "", "Range Not Satisfiable", "Temporary Redirect", "", "Misdirected Request", "", "Non-Authoritative Information", "Upgrade Required",
             "HTTP Version Not Supported", "", "", "Not Extended", "Unauthorized", "", "", "Not Acceptable", "", "", "Length Required", "Found",
                "OK", "Already Reported", "Request Header Fields Too Large"};

const char* possible_methods[] = {"GET", "POST", "UPDATE", "PATCH", "DELETE", "PUT"};

//SUPPORT FOR HTTP STATUS CODE NAMES

typedef enum HTTPMethod {
    GET, 
    POST,
    UPDATE,
    PATCH,
    DELETE,
    PUT,
    UNKNOWN
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
    bool borrowed;
    body_dealloc_t body_dealloc;

} HTTPResponse;

typedef void(*request_handler_t)(HTTPRequest*, HTTPResponse*);

typedef struct RouteHandler {
    const char* match;
    HTTPMethod method;
    request_handler_t handler;
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
void ns_destroy_server(Server* server);
void ns_request(Server* server, HTTPMethod method, const char* match, request_handler_t handler);
char** ns_parse_query_params(HTTPRequest* request, char* buffer, size_t max_buffer_size, size_t max_params);
long ns_calc_sleep(Server* server);
void ns_listen_incoming(Server* server);
bool ns_breakdown_request(HTTPRequest* req, SOCKET socket, char* rbuff, size_t count);
char* ns_glue_response(HTTPResponse* res);
void ns_send_response(HTTPResponse* res);
HTTPResponse* ns_borrow_response(HTTPResponse* res); // gives up memory control of the response to the caller
void ns_set_response_body(HTTPResponse* res, const char* content_type, const char* body);
void ns_free_request_response(HTTPRequest* req, HTTPResponse* res);
request_handler_t ns_match_handler(Server* server, const HTTPRequest* req);
ClientResult ns_handle_client(Server* server, SOCKET client_socket); // socket is ready for rw
void ns_put_header(HTTPResponse* res, const char* key, const char* value);
const char* ns_get_header(HTTPRequest* res, const char* key);
void ns_start_server(Server* server);

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


void ns_request(Server* server, HTTPMethod method, const char* match, request_handler_t handler){
    RouteHandler r = {.match = match, .method = method, .handler = handler};
    VEC_Push(server->handlers, &r);
}

char** ns_parse_query_params(HTTPRequest* request, char* buffer, size_t max_buffer_size, size_t max_params){
    memset(buffer, 0, max_buffer_size);
    size_t offset = 0;
    size_t path_l = strlen(request->path);
    size_t allocated = 0;
    for(; offset < path_l && request->path[offset] != '?'; offset++);
    if(offset == path_l)
        return NULL;
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
        if((buffer_position + (cur - key_start)) - actual_buffer_start >= (ptrdiff_t)max_buffer_size){
            break;
        }
        memcpy(buffer_position, key_start, cur-key_start);
        buffer_position[cur-key_start] = '\0';
        params_position[allocated++] = buffer_position;

        buffer_position += cur-key_start+1;
        const char* val_start = cur;
        for(; cur < request->path + path_l && (*cur != '&'); cur++);
        if((buffer_position + (cur - val_start)) - actual_buffer_start >= (ptrdiff_t)max_buffer_size){
            allocated--;
            break;
        }
        memcpy(buffer_position, val_start, cur-val_start);
        buffer_position[cur-val_start] = '\0';
        params_position[allocated++] = buffer_position;

        buffer_position += cur-val_start+1;
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
    HTTPMethod method = UNKNOWN;
    size_t cur_length = count;
    char* method_word = str_space(rbuff, cur_length, &cur_length);
    for(size_t i = 0; i < sizeof(possible_methods)/sizeof(possible_methods[0]); i++)
        if(strncmp(rbuff, possible_methods[i], method_word-rbuff) == 0)
            method = (HTTPMethod)i;

    if(method == UNKNOWN || cur_length == 0){
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
        char* header_value_start = str_word(header_name_end+1, cur_length, &cur_length);
        char* header_value_end = str_search('\n', header_value_start, cur_length, &cur_length);

        char* key = (char*)malloc(header_name_end-cur_position+1);
        copystrn(key, cur_position, header_name_end-cur_position+1);
        assert(header_name_end-cur_position+1 > 0);
        
        char* value = (char*)malloc(header_value_end-header_value_start+1);
        copystrn(value, header_value_start, header_value_end-header_value_start+1);
        assert(header_value_end-header_value_start+1 > 0);

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
                cur_length -= i;
                blank_line_found = true;
                break;
            }
    }

    if(cur_length == 0){
        req->body = NULL;
        return true; // nobody
    } else {
        char* body = (char*)malloc(cur_length+1); // ALLOCATION
        copystrn(body, cur_position, cur_length+1);
        req->body = body;
    }
    return true;
}

char* ns_glue_response(HTTPResponse* res){
    size_t content_length = res->body != NULL ? strlen(res->body) : 0;
    size_t content_type_length = res->content_type != NULL ? strlen(res->content_type) : 0;
    size_t approx_size = res->headers.count * 512 + content_type_length + content_length + 64;
    char* glued_resp = (char*)malloc(approx_size); // ALLOCATION
    size_t cur_loc = 0;
    cur_loc += snprintf(glued_resp + cur_loc, 32, "HTTP/1.1 %d %s\n", res->status_code, status_names[CODE_TO_NAME(res->status_code)]);
    cur_loc += snprintf(glued_resp + cur_loc, 32, "Content-Length: %zd\n", content_length);
    if(content_type_length > 0)
        cur_loc += snprintf(glued_resp + cur_loc, 32 + content_type_length, "Content-Type: %s\n", res->content_type);
    for(size_t i = 0; i < res->headers.count; i++)
        cur_loc += snprintf(glued_resp + cur_loc, 512, "%s: %s\n", res->headers.start[i].key, res->headers.start[i].value);
    
    if(content_length > 0){
        glued_resp[cur_loc++] = '\n';
        strcpy(glued_resp + cur_loc, res->body);
    }
    return glued_resp;
}  

void ns_send_response(HTTPResponse* res){
    char* full_resp = ns_glue_response(res);
    int r = send(res->client_socket, full_resp, strlen(full_resp), 0);
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

void ns_set_response_body(HTTPResponse* res, const char* content_type, const char* body){
    res->content_type = content_type;
    res->body = body;
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
        VEC_Free(res->headers);
        for(size_t i = 0; i < res->headers.count; i++){
            ns_free_header(&res->headers.start[i]);
        }
        VEC_Free(res->headers);
    }
}

request_handler_t ns_match_handler(Server* server, const HTTPRequest* req){
    for(size_t i = 0; i < server->handlers.count; i++){
        if(server->handlers.start[i].method != req->method)
            continue;
        if(server->handlers.start[i].match == NULL || server->handlers.start[i].match[0] != req->path[0])
            continue;
        const char* until_wildcard = strchr(server->handlers.start[i].match, '*');
        size_t l = until_wildcard - 1 - server->handlers.start[i].match;
        if(until_wildcard == NULL)
            l = strlen(server->handlers.start[i].match);
        if(strncmp(server->handlers.start[i].match, req->path, l) == 0)
            return server->handlers.start[i].handler;
    }
    return NULL;
}


ClientResult ns_handle_client(Server* server, SOCKET client_socket){ // socket is ready for rw
    HTTPRequest req = {0};
    HTTPResponse res = {.client_socket=client_socket, .body_dealloc=free};
    char rbuff[MAX_READ];
    int r = recv(client_socket, rbuff, MAX_READ-1, 0);
    if(r < 0){
        bool e = handle_socket_error();
        return e ? NS_CLIENT_RESULT_ERROR : NS_CLIENT_WOULD_BLOCK;
    }
    else if(r == 0){
        LOG_DEBUG("Empty read on socket %d", (int)client_socket);
        return NS_CLIENT_EMPTY;
    }
    else if(r > 0){
        if(!ns_breakdown_request(&req, client_socket, rbuff, (size_t)r))
            return NS_CLIENT_RESULT_ERROR;

        request_handler_t handle = ns_match_handler(server, &req);
        if(handle == NULL){
            LOG_DEBUG("Received HTTP request without appropriate handler on socket %d", (int)client_socket);
            return NS_CLIENT_RESULT_ERROR;
        }
        handle(&req, &res);
        if(!res.borrowed){ // if it was borrowed - then the caller has to send it
            ns_send_response(&res);
            ns_free_request_response(&req, &res);
        }
    }
    return NS_CLIENT_SUCCESSFUL;
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

const char* ns_get_header(HTTPRequest* res, const char* key){
    for(size_t i = 0; i < res->headers.count; i++)
        if(strcmp(res->headers.start[i].key, key) == 0)
            return res->headers.start[i].value;
    return NULL;
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