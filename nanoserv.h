#pragma once
#include <stdio.h>

#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "nanocommon.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define SOCKET_SHUTDOWN_BOTH_DIRECTIONS SD_BOTH
#include <io.h>
#include <winsock2.h>
#undef DELETE
#define poll WSAPoll


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
#define MAX_ALLOWED_CONTENT_LENGTH_CLIENT (1 << 23)

#ifdef NS_DEBUG
#define LOG_DEBUG(x, ...) printf(x, __VA_ARGS__)
#else
#define LOG_DEBUG(x, ...)
#endif

#ifdef NS_OPENSSL_SUPPORT
    typedef struct ssl_st SSL;
    typedef struct ssl_ctx_st SSL_CTX;
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
                "OK", "Already Reported", "Request Header Fields Too Large", "", "", "", "", "", "", "Unknown"};

static const char* content_types[] = {"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "application/javascript", "", "", "", "", "application/zip", "", "", "text/plain", "", "", "", "", "", "", "text/css", "", "", "", "application/vnd.rar", "text/csv", "", "", "application/xml", "image/svg+xml", "application/x-tar", "audio/wav", "", "", "", "", "image/jpeg", "", "", "font/ttf", "image/png", "", "image/x-icon", "application/octet-stream", "application/octet-stream", "application/vnd.ms-fontobject", "application/json", "", "application/wasm", "", "", "application/pdf", "", "", "", "image/gif", "application/ogg", "application/x-7z-compressed", "", "text/html", "", "", "", "", "", "", "image/jpeg", "", "", "video/webm", "font/woff", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "audio/mpeg", "", "", "", "", "", "", "video/mp4", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""};

static const char* possible_methods[] = {"GET", "POST", "UPDATE", "PATCH", "DELETE", "PUT", "OPTIONS"};

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

typedef struct SocketState {
    SOCKET socket;
    #ifdef NS_OPENSSL_SUPPORT
    SSL* ssl;
    #endif
} SocketState;

typedef struct Header {
    const char* key;
    const char* value;
} Header;

typedef struct HTTPRequest {
    HTTPMethod method;
    A_VEC(Header) headers;
    const char* path;
    const char* body;
    size_t body_length;
} HTTPRequest;

typedef void(*body_dealloc_t)(void*, void*);

typedef struct HTTPResponse {
    SocketState client_socket_state;
    A_VEC(Header) headers;
    int status_code;
    const char* content_type;
    const char* body;
    size_t body_length;
    bool borrowed;
    body_dealloc_t body_dealloc;
    void* body_dealloc_context;

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

typedef struct StoredClientState {
    SocketState socket_state;
    char* read_buf;
    size_t r_read;
    size_t r_total;
    char* send_buf;
    size_t s_sent;
    size_t s_total;
    bool indicated_for_removal;
} StoredClientState;

#define NS_SUPPORTED_COMPRESSION_NONE 0
#define NS_SUPPORTED_COMPRESSION_GZIP 1

typedef struct Server {
    struct sockaddr_in address;
    int port;
    SOCKET listening_socket;
    A_VEC(SocketState) alive_sockets;
    uint64_t last_communication_time;
    A_VEC(RouteHandler) handlers;
    volatile uint64_t shutdown_pending;

    A_VEC(StoredClientState) handler_params_queue;
    A_VEC(SOCKET) sockets_to_remove;
    mtx_t handler_params_m;
    mtx_t server_handler_m;
    cnd_t server_cv;

    int supported_compression;

#ifdef NS_OPENSSL_SUPPORT
    SSL_CTX* ssl_ctx;
    A_VEC(SSL*) staging_ssl_sockets;
#endif
} Server;

// API

void ns_free_header(Header* header);
Server* ns_create_server(const char* server_address, int port, bool server_socket_non_blocking, int supported_compression);
void ns_stop_server(Server* server);
void ns_destroy_server(Server* server);
void ns_request(Server* server, HTTPMethod method, const char* match, request_handler_t handler, route_handler_param param);
void ns_middle_handler(Server* server, HTTPMethod method, const char* match, request_handler_t handler, route_handler_param param);
void ns_serve_directory(Server* server, const char* match, const char* directory_path);
void ns_serve_file(Server* server, const char* match, const char* path);
char** ns_parse_query_params(const HTTPRequest* request, char* buffer, size_t max_buffer_size, size_t max_params);
long us_calc_sleep(Server* server);
void ns_listen_incoming(Server* server);
bool ns_breakdown_request(HTTPRequest* req, SOCKET socket, char* rbuff, size_t count);
char* ns_glue_response(HTTPResponse* res, size_t* response_size);
int ns_send_response(HTTPResponse* res, StoredClientState* scs);
HTTPResponse* ns_borrow_response(HTTPResponse* res); // gives up memory control of the response to the caller
void ns_set_response_body(HTTPResponse* res, const char* body, const char* content_type);
void ns_set_response_body_status(HTTPResponse* res, const char* body, const char* content_type, int status);
void ns_set_response_static_body_status(HTTPResponse* res, const char* body, const char* content_type, int status);
void ns_free_request_response(HTTPRequest* req, HTTPResponse* res);
RouteHandler* ns_match_handler(Server* server, const HTTPRequest* req);
ClientResult ns_handle_client(Server* server, StoredClientState* client_st); // socket is ready for rw
void ns_put_header(HTTPResponse* res, const char* key, const char* value);
const char* ns_get_header(const HTTPRequest* res, const char* key);
void ns_start_server(Server* server);
void socket_startup();
void close_socket(SOCKET socket);

int ns_set_socket_blocking_mode(SOCKET socket, bool blocking);

#ifdef NS_IMPLEMENTATION
#ifndef NS_IMPLEMENTATION_GUARD
#define NS_IMPLEMENTATION_GUARD

#ifndef NS_THREAD_POOL_SIZE
#define NS_THREAD_POOL_SIZE 10
#endif

#ifndef NS_MINIMUM_SIZE_FOR_COMPRESSION
#define NS_MINIMUM_SIZE_FOR_COMPRESSION 256
#endif

#define NS_COMPRESSION_BUFFER_SIZE (1 << 15)

#ifdef NS_GZIP_SUPPORT
    #include <zlib.h>
#endif

#ifdef NS_OPENSSL_SUPPORT
    #include <openssl/ssl.h>
    #include <openssl/err.h>

#endif

#define NS_THREAD_WAIT_TIME 5

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
        return err != WSAEWOULDBLOCK && err != WSAEINPROGRESS;
    #else
        err = errno;
        LOG_DEBUG("Socket error: %s\n", strerror(err));
        return err != EAGAIN && err != EWOULDBLOCK && err != EINPROGRESS;
    #endif
}

int ns_set_socket_blocking_mode(SOCKET socket, bool blocking){
    #ifdef _WIN32
        u_long mode = blocking ? 0 : 1; // 1 for non-blocking, 0 for blocking
        return ioctlsocket(socket, FIONBIO, &mode);
    #else
        int flags = fcntl(socket, F_GETFL, 0);
        if(!blocking){
            flags |= O_NONBLOCK;
        } else {
            flags &= (~O_NONBLOCK);
        }
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

void ns_free_client_state(StoredClientState* scs){
    if(scs->read_buf != NULL)
        NC_FREE(scs->read_buf);
    if(scs->send_buf != NULL)
        NC_FREE(scs->send_buf);
}

Server* ns_create_server(const char* server_address, int port, bool server_socket_non_blocking, int supported_compression){
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
        if(ns_set_socket_blocking_mode(listen_socket, false)){
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
    Server* server = (Server*)NC_ALLOCATE(sizeof(Server));
    memset((void*)server, 0, sizeof(Server));
    memcpy((void*)&server->address, (void*)&address, sizeof(struct sockaddr_in));
    server->port = port;
    server->last_communication_time = get_system_time();
    server->listening_socket = listen_socket;

    server->supported_compression = supported_compression;

    return server;
}

bool ns_setup_ssl(Server* server, const char* cert_filename, const char* key_filename){
    #ifdef NS_OPENSSL_SUPPORT
        int r = OPENSSL_init_ssl(0, NULL);
        if(r == 0) {
            return false;
        }
        SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
        if(!ctx)
            return false;
        r = SSL_CTX_use_certificate_file(ctx, cert_filename, SSL_FILETYPE_PEM);
        r = r & SSL_CTX_use_PrivateKey_file(ctx, key_filename, SSL_FILETYPE_PEM);
        if(r != 1){
            SSL_CTX_free(ctx);
            return false;
        }
        SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
        server->ssl_ctx = ctx;
        return true;
    #else
        return false;
    #endif
}

void ns_stop_server(Server* server){
    atomic_store_64(&server->shutdown_pending, 1);
}

void ns_destroy_server(Server* server){
    if(server->listening_socket)
        close_socket(server->listening_socket);
    for(size_t i = 0; i < server->alive_sockets.count; i++){
#ifdef NS_OPENSSL_SUPPORT
        if(server->alive_sockets.start[i].ssl != NULL){
            SSL_shutdown(server->alive_sockets.start[i].ssl);
            SSL_free(server->alive_sockets.start[i].ssl);
            server->alive_sockets.start[i].ssl = NULL;
        }
#endif
        close_socket(server->alive_sockets.start[i].socket);
    }
    VEC_Free(server->alive_sockets);
    VEC_Free(server->handlers);
    for(size_t i = 0; i < server->handler_params_queue.count; i++){
        ns_free_client_state(server->handler_params_queue.start + i);
    }
    VEC_Free(server->handler_params_queue);
    VEC_Free(server->sockets_to_remove);
#ifdef NS_OPENSSL_SUPPORT
    SSL_CTX_free(server->ssl_ctx);
#endif
    NC_FREE(server);
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

long us_calc_sleep(Server* server){
    uint64_t current_time = get_system_time();
    if(current_time - server->last_communication_time < 64)
        return 0;
    
    return ((current_time - server->last_communication_time) < 999 ? (current_time - server->last_communication_time) : 999)*1000;
}

static bool ns_remove_socket_handling(Server* server, SocketState scst){
    mtx_lock(&server->handler_params_m);
    bool removed = false;
    for(size_t i = 0; i < server->handler_params_queue.count; i++){
        if(server->handler_params_queue.start[i].socket_state.socket == scst.socket){
            server->handler_params_queue.start[i].socket_state.socket = INVALID_SOCKET;
            VEC_Remove(server->handler_params_queue, i);
            ns_free_client_state(server->handler_params_queue.start + i);
            i--;
            removed = true;
        }
    }
    mtx_unlock(&server->handler_params_m);
    return removed;
}

static void ns_remove_alive_socket_internal(Server* server, int pos){
    #ifdef NS_OPENSSL_SUPPORT 
        if(server->alive_sockets.start[pos].ssl != NULL){
            ns_remove_socket_handling(server, server->alive_sockets.start[pos]);
            SSL_shutdown(server->alive_sockets.start[pos].ssl);
            memset(server->alive_sockets.start[pos].ssl, 0, 56);
            //SSL_free(server->alive_sockets.start[j].ssl);
            server->alive_sockets.start[pos].ssl = NULL;
        } else
    #else
        shutdown(server->alive_sockets.start[pos].socket, SOCKET_SHUTDOWN_BOTH_DIRECTIONS);
    #endif
        
    close_socket(server->alive_sockets.start[pos].socket); 
    VEC_Remove(server->alive_sockets, pos);
}

void ns_listen_incoming(Server* server){
    struct timeval tv = {0};
    struct pollfd readfds = (struct pollfd){.fd=server->listening_socket, .events=POLLIN, .revents=0};

    tv.tv_sec = 0;
    tv.tv_usec = us_calc_sleep(server); //calc_sleep(server); // Server listening socket is blocking, to avoid total busywaiting

    mtx_lock(&server->handler_params_m);
    for(int i = 0; i < (int)server->handler_params_queue.count; i++){
        if(server->handler_params_queue.start[i].indicated_for_removal)
            VEC_Push(server->sockets_to_remove, (&server->handler_params_queue.start[i].socket_state.socket));
    }
    mtx_unlock(&server->handler_params_m);

    if(server->sockets_to_remove.count > 0){
        for(int i = 0; i < (int)server->sockets_to_remove.count; i++){
            bool found_socket = false;
            for(size_t j = 0; j < server->alive_sockets.count; j++){
                if(server->sockets_to_remove.start[i] == server->alive_sockets.start[j].socket){
                    ns_remove_alive_socket_internal(server, j);
                    VEC_Remove(server->sockets_to_remove, (size_t)i);
                    found_socket = true;
                    i--;
                    break;
                }
            }
            if(!found_socket){
                VEC_Remove(server->sockets_to_remove, (size_t)i);
                i--;
            }
        }
    }

    int retval = poll(&readfds, 1, (int)((tv.tv_usec/1000 > 16) ? 16 : tv.tv_usec/1000));
    if(retval > 0){
        SOCKET new_sock = accept(server->listening_socket, NULL, NULL);
        SocketState sock_s = (SocketState){.socket=new_sock};
#ifdef NS_OPENSSL_SUPPORT
        if(server->ssl_ctx != NULL){
            SSL* ssl = SSL_new(server->ssl_ctx);
            SSL_set_fd(ssl, new_sock);
            int ret;
            if((ret = SSL_accept(ssl)) <= 0){
                int err = SSL_get_error(ssl, ret);
                if(err != SSL_ERROR_WANT_ACCEPT && err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE){
                    close_socket(new_sock);
                    return;
                } else {
                    VEC_Push(server->staging_ssl_sockets, &ssl);
                    goto check_staging_sockets;
                }
            }
            sock_s.ssl = ssl;
        }
#endif
        retval = ns_set_socket_blocking_mode(new_sock, false);
        if(retval < 0){
            handle_socket_error();
            return;
        }
        VEC_Push(server->alive_sockets, &sock_s);
        // assert(server->alive_sockets.count < MAX_FD_SET); // CHANGE: splitting FDSETS by threads - currently busy waiting
    } else if(retval < 0) {
        handle_socket_error();
    }

    #ifdef NS_OPENSSL_SUPPORT
        check_staging_sockets:
        for(size_t i = 0; i < server->staging_ssl_sockets.count; i++){
            SSL* ssl = server->staging_ssl_sockets.start[i];
            SOCKET new_sock = SSL_get_fd(ssl);
            int ret;
            if((ret = SSL_accept(ssl)) <= 0){
                int err = SSL_get_error(ssl, ret);
                if(err != SSL_ERROR_WANT_ACCEPT && err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE){
                    close_socket(new_sock);
                    VEC_Remove(server->staging_ssl_sockets, i);
                    i--;
                }
            } else {
                VEC_Remove(server->staging_ssl_sockets, i);
                i--;
                SocketState st = (SocketState){.socket=new_sock, .ssl=ssl};
                VEC_Push(server->alive_sockets, &st);
            }
        }
    #endif
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
    char* path = (char*)NC_ALLOCATE(query_word_end-method_word+1); // ALLOCATION
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

        char* key = (char*)NC_ALLOCATE(header_name_end-cur_position+1); // dont rely on dyn alloc
        copystrn(key, cur_position, header_name_end-cur_position+1);
        assert(header_name_end-cur_position+1 > 0);
        
        char* value = (char*)NC_ALLOCATE(header_value_end-header_value_start+1);
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
        char* body = (char*)NC_ALLOCATE(cur_length+1); // ALLOCATION
        copystrn(body, cur_position, cur_length+1);
        req->body = body;
        req->body_length = cur_length;
    }
    return true;
}

char* ns_glue_response(HTTPResponse* res, size_t* response_size){
    size_t content_length = res->body != NULL ? res->body_length != 0 ? res->body_length : strlen(res->body) : 0;
    size_t content_type_length = res->content_type != NULL ? strlen(res->content_type) : 0;
    size_t approx_size = res->headers.count * 512 + content_type_length + content_length + 64;
    char* glued_resp = (char*)NC_ALLOCATE(approx_size); // ALLOCATION
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

static void ns_compress_response(HTTPResponse* res, int compression){
    if(res->body == NULL || compression == NS_SUPPORTED_COMPRESSION_NONE)
        return;

    int actual_compression_level = NS_SUPPORTED_COMPRESSION_NONE;
#ifdef NS_GZIP_SUPPORT
    if(compression == NS_SUPPORTED_COMPRESSION_GZIP)
        actual_compression_level = compression;
#endif

    if(actual_compression_level == NS_SUPPORTED_COMPRESSION_NONE){
        return;
    }
    assert(actual_compression_level == NS_SUPPORTED_COMPRESSION_GZIP && "Attempted to compress with an unsupported compression method");

    #ifdef NS_GZIP_SUPPORT
        z_stream strm_ = {0};
        strm_.zalloc = Z_NULL;
        strm_.zfree = Z_NULL;
        strm_.opaque = Z_NULL;
        bool init_s = deflateInit2(&strm_, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 31, 8,
                           Z_DEFAULT_STRATEGY) == Z_OK;
        if(!init_s){
            deflateEnd(&strm_);
            return;
        }

        char compression_store[NS_COMPRESSION_BUFFER_SIZE];
        strm_.avail_in = (int)res->body_length;
        strm_.next_in = res->body;
        int new_body_length = (int)res->body_length;
        size_t in_body = 0;
        do {
            strm_.avail_out = NS_COMPRESSION_BUFFER_SIZE;
            strm_.next_out = compression_store;

            int ret = deflate(&strm_, Z_FINISH);
            if (ret == Z_STREAM_ERROR) { return; } // deformed response...

            if (strm_.avail_out == 0 && in_body + NS_COMPRESSION_BUFFER_SIZE > new_body_length) {
                new_body_length = new_body_length + NS_COMPRESSION_BUFFER_SIZE*2;
                (char*)res->body = NC_REALLOCATE((char*)res->body, new_body_length);
            }
            memcpy((char *)res->body + in_body, compression_store, NS_COMPRESSION_BUFFER_SIZE - strm_.avail_out);
            in_body += NS_COMPRESSION_BUFFER_SIZE - strm_.avail_out;
        } while (strm_.avail_out == 0);
        deflateEnd(&strm_);
        ns_put_header(res, "Content-Encoding", "gzip");

        res->body_length = in_body;
    #endif
    
}

static int ns_decompress_request(HTTPRequest* req){
    if(req->body == NULL)
        return 1;
    int compression = NS_SUPPORTED_COMPRESSION_NONE;
    const char* t_e = ns_get_header(req, "Transfer-Encoding");
    if(t_e != NULL && strstr(t_e, "gzip") != NULL)
        compression = NS_SUPPORTED_COMPRESSION_GZIP;

    int actual_compression_level = NS_SUPPORTED_COMPRESSION_NONE;
    #ifdef NS_GZIP_SUPPORT
        if(compression == NS_SUPPORTED_COMPRESSION_GZIP)
            actual_compression_level = compression;
    #endif

    if(actual_compression_level == NS_SUPPORTED_COMPRESSION_NONE){
        return actual_compression_level < compression ? 0 : 1;
    }
    assert(actual_compression_level == NS_SUPPORTED_COMPRESSION_GZIP && "Attempted to compress with an unsupported compression method");

    #ifdef NS_GZIP_SUPPORT
        z_stream strm_ = {0};
        strm_.zalloc = Z_NULL;
        strm_.zfree = Z_NULL;
        strm_.opaque = Z_NULL;
        bool init_s = inflateInit2(&strm_, 15+32) == Z_OK;
        if(!init_s){
            inflateEnd(&strm_);
            return 0;
        }

        char compression_store[NS_COMPRESSION_BUFFER_SIZE];
        strm_.avail_in = (int)req->body_length;
        strm_.next_in = req->body;
        int new_body_length = (int)req->body_length;
        size_t in_body = 0;
        while(strm_.avail_in > 0) {
            strm_.avail_out = NS_COMPRESSION_BUFFER_SIZE;
            strm_.next_out = compression_store;

            int ret = inflate(&strm_, Z_NO_FLUSH);
            if (ret == Z_STREAM_ERROR || ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) { return 0; } // deformed response...

            if (strm_.avail_in > 0 && in_body + NS_COMPRESSION_BUFFER_SIZE > new_body_length) {
                new_body_length = new_body_length + NS_COMPRESSION_BUFFER_SIZE*2;
                (char*)req->body = NC_REALLOCATE((char*)req->body, new_body_length);
            }
            memcpy((char*)req->body + in_body, compression_store, NS_COMPRESSION_BUFFER_SIZE - strm_.avail_out);
            in_body += NS_COMPRESSION_BUFFER_SIZE - strm_.avail_out;
        }
        inflateEnd(&strm_);
    
    #endif
    return 1;
}

static int s_read(SocketState* s, char* buffer, int read_size){
    #ifdef NS_OPENSSL_SUPPORT
        if(s->ssl != NULL){
            int read = SSL_read(s->ssl, buffer, read_size);
            if(read > 0)
                return read;

            int err = SSL_get_error(s->ssl, read);
            if(err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE){
                #ifdef _WIN32
                    WSASetLastError(WSAEWOULDBLOCK);
                #else
                    errno = EWOULDBLOCK;
                #endif
            }
            if(err == SSL_ERROR_SSL){
                #ifdef _WIN32
                    WSASetLastError(WSAEINVAL);
                #else
                    errno = EINVAL;
                #endif 
            }
            return -1;
        }
    #endif
    return recv(s->socket, buffer, read_size, 0);
}

static int s_write(SocketState* s, char* buffer, int write_size){
    #ifdef NS_OPENSSL_SUPPORT
        if(s->ssl != NULL){
            int write = SSL_write(s->ssl, buffer, write_size);
            if(write > 0)
                return write;

            int err = SSL_get_error(s->ssl, write);
            if(err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE){
                #ifdef _WIN32
                    WSASetLastError(WSAEWOULDBLOCK);
                #else
                    errno = EWOULDBLOCK;
                #endif
            }
            if(err == SSL_ERROR_SSL){
                #ifdef _WIN32
                    WSASetLastError(WSAEINVAL);
                #else
                    errno = EINVAL;
                #endif 
            }
            return -1;
        }
    #endif
    return send(s->socket, buffer, write_size, 0);
}

static int ns_read_request_internal(StoredClientState* scs){
    int r = s_read(&scs->socket_state, scs->read_buf + scs->r_read, scs->r_total - scs->r_read);
    if(r < 0){
        bool err = handle_socket_error();
        if(err){
            return -1;
        } else {
            r = 0;
        }
    }
    scs->r_read += r;
    return scs->r_read == scs->r_total ? 1 : 0;
}

static int ns_send_response_internal(StoredClientState* scs){
    int cur = s_write(&scs->socket_state, scs->send_buf + scs->s_sent, scs->s_total - scs->s_sent); // busy waiting?!
    if(cur < 0){
        bool err = handle_socket_error();
        if(err){
            return -1;
        } else {
            cur = 0;
        }
    }
    scs->s_sent += cur;
    return scs->s_sent == scs->s_total ? 1 : 0;
}

int ns_send_response(HTTPResponse* res, StoredClientState* scs){
    size_t response_size = 0;
    char* full_resp = ns_glue_response(res, &response_size);
    StoredClientState scs_local = (StoredClientState){.socket_state = res->client_socket_state};
    assert(scs != NULL);
    if(scs->socket_state.socket == INVALID_SOCKET){
        memcpy(scs, &scs_local, sizeof(scs_local));
    }

    scs->send_buf = full_resp;
    scs->s_total = response_size;
    int r = ns_send_response_internal(scs);
    if(r == 1){
        ns_free_client_state(scs);
    }
    return r;
}

HTTPResponse* ns_borrow_response(HTTPResponse* res){ // gives up memory control of the response to the caller
    res->borrowed = true;
    HTTPResponse* new_resp = (HTTPResponse*)NC_ALLOCATE(sizeof(HTTPResponse));
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

void ns_set_response_static_body_status(HTTPResponse* res, const char* body, const char* content_type, int status){
    ns_set_response_body_status(res, body, content_type, status);
    res->body_dealloc = NULL;
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
            res->body_dealloc(res->body_dealloc_context, (void*)res->body);
        
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

static void dealloc_wrapper(void* context, void* ptr){
    free(ptr);
}

static int ns_get_compression_level(const HTTPRequest* req, int max_level){
    int compression_level = NS_SUPPORTED_COMPRESSION_NONE;
    const char* enc;
    if((enc = ns_get_header(req, "Accept-Encoding")) == NULL)
        return compression_level;

    if(strstr(enc, "gzip") != NULL && max_level >= NS_SUPPORTED_COMPRESSION_GZIP)
        compression_level = NS_SUPPORTED_COMPRESSION_GZIP;
    return compression_level;
}

ClientResult ns_handle_client(Server* server, StoredClientState* client_st){ // socket is ready for rw
    SocketState* cl_st = &client_st->socket_state;
    SOCKET client_socket = cl_st->socket;
    HTTPRequest req = {0};
    HTTPResponse res = {.client_socket_state=*cl_st, .body_dealloc=dealloc_wrapper};
    ClientResult c_r = NS_CLIENT_SUCCESSFUL;
    
    if(client_st->send_buf != NULL){
        int r = ns_send_response_internal(client_st);
        switch(r) {
            case -1:
                return NS_CLIENT_RESULT_ERROR;
            case 0:
                return NS_CLIENT_WOULD_BLOCK;
            case 1:
                return NS_CLIENT_SUCCESSFUL;
        }
    }

    char initial_read_buffer[MAX_READ+1];
    char secondary_read_buffer[MAX_READ+1];
    char* rbuff = initial_read_buffer;
    int r;

    if(client_st->read_buf != NULL){
        rbuff = client_st->read_buf;
        int cur = ns_read_request_internal(client_st);
        switch(cur) {
            case -1:
                return NS_CLIENT_RESULT_ERROR;
            case 0:
                return NS_CLIENT_WOULD_BLOCK;
            case 1:
                r = client_st->r_total;
                goto resume_multipacket_processing;
        }
    }

    r = s_read(cl_st, rbuff, MAX_READ);
    if(r < 0){
        bool e = handle_socket_error();
        return e ? NS_CLIENT_RESULT_ERROR : NS_CLIENT_WOULD_BLOCK;
    }
    else if(r == 0){
        LOG_DEBUG("Empty read on socket %d", (int)client_socket);
        return NS_CLIENT_EMPTY;
    }
    else if(r > 0){

        if(!ns_breakdown_request(&req, client_socket, rbuff, (size_t)r)){ // this version is temporary, a better way to do it would be to allocate a buffer for every client. Here we are forcibly bound by the size of the recv buffer
            c_r = NS_CLIENT_RESULT_ERROR;
            goto post_request_process;
        }

        const char* content_length = ns_get_header(&req, "Content-Length");
        if(content_length == NULL){
            goto post_multipacket_processing;
        } else {
            int ct_length = strtol(content_length, NULL, 10);
            //size_t body_l = strlen(req->body); // CHANGE: replace with body length
            if(ct_length <= req.body_length){
                goto post_multipacket_processing;
            }

            size_t length_with_headers = ct_length + (r-req.body_length);

            if(MAX_READ + ct_length+1 > MAX_ALLOWED_CONTENT_LENGTH_CLIENT){
                return NS_CLIENT_RESULT_ERROR; // drop absurdly large requests
            }
            rbuff = NC_ALLOCATE(MAX_READ + ct_length+1);
            int new_r;
            memcpy(rbuff, initial_read_buffer, r);
            if((new_r = s_read(cl_st, rbuff + r, MAX_READ + ct_length+1)) < length_with_headers - r){
                if(new_r < 0){
                    if(handle_socket_error()){
                        ns_free_request_response(&req, &res);
                        NC_FREE(rbuff);
                        return NS_CLIENT_RESULT_ERROR;
                    } else {
                        new_r = 0;
                    }
                }
                client_st->read_buf = rbuff;
                client_st->r_read = r + new_r;
                client_st->r_total = length_with_headers;
                ns_free_request_response(&req, &res);
                return NS_CLIENT_WOULD_BLOCK;
            } else {
                r += new_r;
            }

        }

        resume_multipacket_processing:

        rbuff[r] = '\0';

        if(!ns_breakdown_request(&req, client_socket, rbuff, (size_t)r)){
            ns_free_request_response(&req, &res);
            return NS_CLIENT_RESULT_ERROR;
        }

        const char* t_e = NULL;

        int decompressed = ns_decompress_request(&req);
        if(!decompressed){
            ns_free_request_response(&req, &res);
            return NS_CLIENT_RESULT_ERROR;
        }

        post_multipacket_processing:

        ns_call_all_middle_route_handlers(server, &req, &res);
        if(res.body == NULL){
            RouteHandler* handle = ns_match_handler(server, &req);
            if(handle == NULL){
                LOG_DEBUG("Received HTTP request without appropriate handler on socket %d", (int)client_socket);
                c_r = NS_CLIENT_SUCCESSFUL;
                goto post_request_process;
            }
            handle->handler(&req, &res, handle->param);
        }
        if(!res.borrowed){ // if it was borrowed - then the caller has to send it
            int compression_level = ns_get_compression_level(&req, server->supported_compression);
            ns_compress_response(&res, res.body_length < NS_MINIMUM_SIZE_FOR_COMPRESSION ? NS_SUPPORTED_COMPRESSION_NONE : compression_level);
            int r = ns_send_response(&res, client_st);
            if(r < 1)
                res.body = NULL;
            c_r = r == 1 ? NS_CLIENT_SUCCESSFUL : r == 0 ? NS_CLIENT_WOULD_BLOCK : NS_CLIENT_RESULT_ERROR;
            ns_free_request_response(&req, &res);
        }
        return c_r;
    }

    post_request_process:
    if(rbuff != initial_read_buffer)
        NC_FREE(rbuff);
    return c_r;
}

void ns_put_header(HTTPResponse* res, const char* key, const char* value){
    size_t keyl = strlen(key);
    char* nk = (char*)NC_ALLOCATE(keyl+1); // ALLOCATION
    copystrn(nk, key, keyl+1);
    
    size_t val_l = strlen(value);
    char* nv = (char*)NC_ALLOCATE(val_l+1); // ALLOCATION
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

    res->body = (const char*)NC_ALLOCATE(file_size+1);
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
    ns_directory_params* directory_params = (ns_directory_params*)NC_ALLOCATE(sizeof(ns_directory_params));
    *directory_params = (ns_directory_params){.match=match, .dirn=directory_path};
    assert(strchr(match, '*') != NULL && "Directory match must include a wildcard to allow for filenames");
    RouteHandler r = {.match = match, .method = GET, .handler = ns_directory_handler, .param=(route_handler_param)directory_params};
    VEC_Push(server->handlers, &r);
}

void ns_serve_file(Server* server, const char* match, const char* path){
    RouteHandler r = {.match = match, .method = GET, .handler = ns_file_handler, .param=(route_handler_param)path};
    VEC_Push(server->handlers, &r);
}

void ns_thread_pool_pop_single(Server* server){
    mtx_lock(&server->handler_params_m);
    if(server->handler_params_queue.count > 0){
        StoredClientState* client_st = server->handler_params_queue.start + server->handler_params_queue.count - 1;
        while(client_st->indicated_for_removal && client_st >= server->handler_params_queue.start){
            client_st--;
        }
        if(client_st < server->handler_params_queue.start)
            goto post_handling;

        #ifdef _WIN32
            LOG_DEBUG("th handle %d", thrd_current()._Tid);
        #else
            LOG_DEBUG("th handle %ld", server->handler_params_queue.count);
        #endif

        ClientResult cr = ns_handle_client(server, client_st);
        if(cr == NS_CLIENT_RESULT_ERROR || cr == NS_CLIENT_EMPTY){ // destroy the socket
            LOG_DEBUG("Removing %d\n", (int)client_st->socket_state.socket);
            client_st->indicated_for_removal = true;
        } else if(cr == NS_CLIENT_WOULD_BLOCK){
            StoredClientState st_t = *client_st;
            server->handler_params_queue.start[server->handler_params_queue.count-1] = server->handler_params_queue.start[0];
            server->handler_params_queue.start[0] = st_t;
        } else { // remove from "stack" only if client hasn't finished sending data
            (&server->handler_params_queue)->count = server->handler_params_queue.count-1;
        }
    }

    post_handling:

    mtx_unlock(&server->handler_params_m);
}

int ns_run_thread_pool(void* server_arg){
    Server* server = (Server*)server_arg;
    if(server == NULL)
        return 1;
    
    while(true){
        struct timespec tms = (struct timespec){.tv_sec=time(NULL)+NS_THREAD_WAIT_TIME};
        ns_thread_pool_pop_single(server);
        mtx_lock(&server->server_handler_m);
        cnd_timedwait(&server->server_cv, &server->server_handler_m, &tms);
        if(atomic_load_64(&server->shutdown_pending)){
            mtx_unlock(&server->server_handler_m);
            return 0;
        }

        mtx_unlock(&server->server_handler_m);
    }
    return 1;
}


bool ns_issue_handling_request(Server* server, SocketState scst){
    LOG_DEBUG("adding %d", (int)scst.socket);
    mtx_lock(&server->handler_params_m);
    bool already_in_queue = false;
    for(size_t i = 0; i < server->handler_params_queue.count; i++){
        if(server->handler_params_queue.start[i].socket_state.socket == scst.socket){
            already_in_queue = true;
            break;
        }
    }

    if(!already_in_queue){
        StoredClientState scs = (StoredClientState){.socket_state = scst};
        VEC_Push(server->handler_params_queue, &scs);
    }
    mtx_unlock(&server->handler_params_m);
    return !already_in_queue;
}



void ns_check_clients(Server* server){
    struct pollfd readfds[MAX_FD_SET];
    struct timeval tv = {0};

    size_t cur_sock = 0;
    while(cur_sock < server->alive_sockets.count){
        memset(readfds, 0, sizeof(readfds));
        int selected_sockets = 0;
        size_t until_full_sock = cur_sock;
        // breakdown all listening sockets to max_fd chunks, and select each of them
        for(; until_full_sock < server->alive_sockets.count; until_full_sock++){
            readfds[selected_sockets].fd = server->alive_sockets.start[until_full_sock].socket;
            readfds[selected_sockets].events = POLLIN;
            readfds[selected_sockets].revents = 0;
            selected_sockets++;
            if(selected_sockets >= MAX_FD_SET-1)
                break;
        }

        if(selected_sockets == 0)
            return;
        int retval = poll(readfds, selected_sockets, 0);
        if(retval > 0){
            uint64_t system_time = get_system_time();
            for(size_t i = cur_sock; i < until_full_sock && i < server->alive_sockets.count; i++){
                if(readfds[i-cur_sock].revents & POLLIN){
                    if(ns_issue_handling_request(server, server->alive_sockets.start[i])){
                        server->last_communication_time = system_time;
                    }
                    if(NS_THREAD_POOL_SIZE != 1){
                        cnd_signal(&server->server_cv);
                    }
                } else if((readfds[i-cur_sock].revents & POLLHUP) || (readfds[i-cur_sock].revents & POLLERR)){
                    ns_remove_socket_handling(server, server->alive_sockets.start[i]);
                    VEC_Push(server->sockets_to_remove, &server->alive_sockets.start[i].socket);
                }
                
            }
        } else if(retval < 0){
            handle_socket_error();
        }
        if(NS_THREAD_POOL_SIZE == 1)
            ns_thread_pool_pop_single(server); // handle a single client in here, as we run in a single-threaded mode
        
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
    while(!atomic_load_64(&server->shutdown_pending)){
        ns_listen_incoming(server);
        ns_check_clients(server);
    }
    mtx_lock(&server->server_handler_m);
    cnd_broadcast(&server->server_cv);
    mtx_unlock(&server->server_handler_m);
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