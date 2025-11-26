#pragma once
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <assert.h>
#include <stdlib.h>
#include "nanocommon.h"

#define MAX_EXPRESSION_LENGTH 2048
#define MAX_NUMERICAL_EXPRESSION_DIGITS 20 // maybe set 40 for 2^128+ support

#ifndef NJ_ALLOCATE
    #ifdef NC_ALLOCATE
        #define NJ_ALLOCATE NC_ALLOCATE
    #else
        #define NJ_ALLOCATE malloc
    #endif
#endif
#ifndef NJ_FREE
    #ifdef NC_FREE
        #define NJ_FREE NC_FREE
    #else
        #define NJ_FREE free
    #endif
#endif



typedef enum {
    VALUE = 0,
    FIELD = 1,
    OBJECT = 2,
    ARRAY = 3,
    UNKNOWN_TYPE = 4
}  NodeType;

typedef enum {
    NONE = 0,
    NUMERICAL = 8,
    TRUE_VALUE = 5,
    FALSE_VALUE = 6,
    NULL_VALUE = 7,
    STRING = 9
} NodeFlags;

typedef struct Node {
    NodeType type;
    const char* key;
    bool static_key;
    A_VEC(struct Node) children;
    struct Node* parent;
    NodeFlags flags;
} Node;

typedef struct JsonParser {
    allocator_t allocator;
} JsonParser;

typedef void(*json_parser_t)(Node* node, void* out, size_t elem_s, JsonParser* parser);


// API
Node* create_nodes_from_parent(const char* buffer, size_t buffer_len, int* error_code, JsonParser* parser);
void free_json(Node* output_node, bool free_root, JsonParser* parser);
size_t output_node(Node* node, char* buffer, size_t buffer_max, size_t cur_pos, size_t cur_indent, size_t add_indent);
size_t json_approximate_size(Node* node, size_t add_indent);
void json_init_node(Node* node, NodeType type);
void json_parse_array(Node* node, void* out, json_parser_t parse_cb, size_t elem_s, JsonParser* parser);
void json_parse_integral(Node* node, void* out, size_t elem_s, JsonParser* parser);
void json_parse_floating(Node* node, void* out, size_t elem_s, JsonParser* parser);
void json_parse_string(Node* node, void* out, size_t elem_s, JsonParser* parser);
void json_parse__Bool(Node* node, void* out, size_t elem_s, JsonParser* parser);
void json_serialize_integral(Node* node, void* buf, size_t elem_s, JsonParser* parser);
void json_serialize_floating(Node* node, void* buf, size_t elem_s, JsonParser* parser);
void json_serialize_string(Node* node, void* buf, size_t elem_s, JsonParser* parser);
void json_serialize__Bool(Node* node, void* buf, size_t elem_s, JsonParser* parser);

#define NJ_ARRAY(type) nj_array_ ##type
#define NJ_PTR(type) nj_ptr_ ##type
#define NJ_VEC(type) nj_vector_ ##type
#define NJ_ENUM(type) nj_enum_ ##type
#define NJ_MAPPING(type) nj_mapping_ ##type

#define NJ_KEY_MAPPING(type) struct {char* key; type value;}

// For some weird compiler differences
#define json_serialize_bool json_serialize__Bool
#define json_parse_bool json_parse__Bool


#define NJ_CHECK_PARSE(field, type, ordinal) if(!field_marked[ordinal] && strcmp(node->children.start[i].key, #field) == 0) {\
        json_parse_ ##type(&node->children.start[i], (void*)&out->field, sizeof(out->field), parser); \
        field_marked[ordinal] = true; \
    }

#define NJ_PARSE_BREAKDOWN_1(v1, v2) NJ_CHECK_PARSE(v1, v2, 0)
#define NJ_PARSE_BREAKDOWN_2(v1, v2, v3, v4) NJ_PARSE_BREAKDOWN_1(v1, v2) NJ_CHECK_PARSE(v3, v4, 1)
#define NJ_PARSE_BREAKDOWN_3(v1, v2, v3, v4, v5, v6) NJ_PARSE_BREAKDOWN_2(v1, v2, v3, v4) NJ_CHECK_PARSE(v5, v6, 2)
#define NJ_PARSE_BREAKDOWN_4(v1, v2, v3, v4, v5, v6, v7, v8) NJ_PARSE_BREAKDOWN_3(v1, v2, v3, v4, v5, v6) NJ_CHECK_PARSE(v7, v8, 3)
#define NJ_PARSE_BREAKDOWN_5(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10) NJ_PARSE_BREAKDOWN_4(v1, v2, v3, v4, v5, v6, v7, v8) NJ_CHECK_PARSE(v9, v10, 4)
#define NJ_PARSE_BREAKDOWN_6(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12) NJ_PARSE_BREAKDOWN_5(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10) NJ_CHECK_PARSE(v11, v12, 5)
#define NJ_PARSE_BREAKDOWN_7(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14) NJ_PARSE_BREAKDOWN_6(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12) NJ_CHECK_PARSE(v13, v14, 6)
#define NJ_PARSE_BREAKDOWN_8(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16) NJ_PARSE_BREAKDOWN_7(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14) NJ_CHECK_PARSE(v15, v16, 7)
#define NJ_PARSE_BREAKDOWN_9(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18) NJ_PARSE_BREAKDOWN_8(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16) NJ_CHECK_PARSE(v17, v18, 8)
#define NJ_PARSE_BREAKDOWN_10(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20) NJ_PARSE_BREAKDOWN_9(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18) NJ_CHECK_PARSE(v19, v20, 9)
#define NJ_PARSE_BREAKDOWN_11(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22) NJ_PARSE_BREAKDOWN_10(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20) NJ_CHECK_PARSE(v21, v22, 10)
#define NJ_PARSE_BREAKDOWN_12(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24) NJ_PARSE_BREAKDOWN_11(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22) NJ_CHECK_PARSE(v23, v24, 11)
#define NJ_PARSE_BREAKDOWN_13(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26) NJ_PARSE_BREAKDOWN_12(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24) NJ_CHECK_PARSE(v25, v26, 12)
#define NJ_PARSE_BREAKDOWN_14(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28) NJ_PARSE_BREAKDOWN_13(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26) NJ_CHECK_PARSE(v27, v28, 13)
#define NJ_PARSE_BREAKDOWN_15(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30) NJ_PARSE_BREAKDOWN_14(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28) NJ_CHECK_PARSE(v29, v30, 14)
#define NJ_PARSE_BREAKDOWN_16(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32) NJ_PARSE_BREAKDOWN_15(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30) NJ_CHECK_PARSE(v31, v32, 15)
#define NJ_PARSE_BREAKDOWN_17(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34) NJ_PARSE_BREAKDOWN_16(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32) NJ_CHECK_PARSE(v33, v34, 16)
#define NJ_PARSE_BREAKDOWN_18(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36) NJ_PARSE_BREAKDOWN_17(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34) NJ_CHECK_PARSE(v35, v36, 17)
#define NJ_PARSE_BREAKDOWN_19(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38) NJ_PARSE_BREAKDOWN_18(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36) NJ_CHECK_PARSE(v37, v38, 18)
#define NJ_PARSE_BREAKDOWN_20(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40) NJ_PARSE_BREAKDOWN_19(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38) NJ_CHECK_PARSE(v39, v40, 19)
#define NJ_PARSE_BREAKDOWN_21(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42) NJ_PARSE_BREAKDOWN_20(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40) NJ_CHECK_PARSE(v41, v42, 20)
#define NJ_PARSE_BREAKDOWN_22(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44) NJ_PARSE_BREAKDOWN_21(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42) NJ_CHECK_PARSE(v43, v44, 21)
#define NJ_PARSE_BREAKDOWN_23(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46) NJ_PARSE_BREAKDOWN_22(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44) NJ_CHECK_PARSE(v45, v46, 22)
#define NJ_PARSE_BREAKDOWN_24(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48) NJ_PARSE_BREAKDOWN_23(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46) NJ_CHECK_PARSE(v47, v48, 23)
#define NJ_PARSE_BREAKDOWN_25(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50) NJ_PARSE_BREAKDOWN_24(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48) NJ_CHECK_PARSE(v49, v50, 24)
#define NJ_PARSE_BREAKDOWN_26(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52) NJ_PARSE_BREAKDOWN_25(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50) NJ_CHECK_PARSE(v51, v52, 25)
#define NJ_PARSE_BREAKDOWN_27(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54) NJ_PARSE_BREAKDOWN_26(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52) NJ_CHECK_PARSE(v53, v54, 26)
#define NJ_PARSE_BREAKDOWN_28(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56) NJ_PARSE_BREAKDOWN_27(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54) NJ_CHECK_PARSE(v55, v56, 27)
#define NJ_PARSE_BREAKDOWN_29(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58) NJ_PARSE_BREAKDOWN_28(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56) NJ_CHECK_PARSE(v57, v58, 28)
#define NJ_PARSE_BREAKDOWN_30(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58, v59, v60) NJ_PARSE_BREAKDOWN_29(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58) NJ_CHECK_PARSE(v59, v60, 29)
#define NJ_PARSE_BREAKDOWN_31(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58, v59, v60, v61, v62) NJ_PARSE_BREAKDOWN_30(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58, v59, v60) NJ_CHECK_PARSE(v61, v62, 30)

#define NJ_M_EXPAND(x) x
#define NJ_GET_M(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, NAME, ...) NAME
#define NJ_PARSE_BREAKDOWN(...) NJ_M_EXPAND(NJ_GET_M(__VA_ARGS__, NJ_PARSE_BREAKDOWN_31, NJ_PARSE_BREAKDOWN_31, NJ_PARSE_BREAKDOWN_30, NJ_PARSE_BREAKDOWN_30, NJ_PARSE_BREAKDOWN_29, NJ_PARSE_BREAKDOWN_29, NJ_PARSE_BREAKDOWN_28, NJ_PARSE_BREAKDOWN_28, NJ_PARSE_BREAKDOWN_27, NJ_PARSE_BREAKDOWN_27, NJ_PARSE_BREAKDOWN_26, NJ_PARSE_BREAKDOWN_26, NJ_PARSE_BREAKDOWN_25, NJ_PARSE_BREAKDOWN_25, NJ_PARSE_BREAKDOWN_24, NJ_PARSE_BREAKDOWN_24, NJ_PARSE_BREAKDOWN_23, NJ_PARSE_BREAKDOWN_23, NJ_PARSE_BREAKDOWN_22, NJ_PARSE_BREAKDOWN_22, NJ_PARSE_BREAKDOWN_21, NJ_PARSE_BREAKDOWN_21, NJ_PARSE_BREAKDOWN_20, NJ_PARSE_BREAKDOWN_20, NJ_PARSE_BREAKDOWN_19, NJ_PARSE_BREAKDOWN_19, NJ_PARSE_BREAKDOWN_18, NJ_PARSE_BREAKDOWN_18, NJ_PARSE_BREAKDOWN_17, NJ_PARSE_BREAKDOWN_17, NJ_PARSE_BREAKDOWN_16, NJ_PARSE_BREAKDOWN_16, NJ_PARSE_BREAKDOWN_15, NJ_PARSE_BREAKDOWN_15, NJ_PARSE_BREAKDOWN_14, NJ_PARSE_BREAKDOWN_14, NJ_PARSE_BREAKDOWN_13, NJ_PARSE_BREAKDOWN_13, NJ_PARSE_BREAKDOWN_12, NJ_PARSE_BREAKDOWN_12, NJ_PARSE_BREAKDOWN_11, NJ_PARSE_BREAKDOWN_11, NJ_PARSE_BREAKDOWN_10, NJ_PARSE_BREAKDOWN_10, NJ_PARSE_BREAKDOWN_9, NJ_PARSE_BREAKDOWN_9, NJ_PARSE_BREAKDOWN_8, NJ_PARSE_BREAKDOWN_8, NJ_PARSE_BREAKDOWN_7, NJ_PARSE_BREAKDOWN_7, NJ_PARSE_BREAKDOWN_6, NJ_PARSE_BREAKDOWN_6, NJ_PARSE_BREAKDOWN_5, NJ_PARSE_BREAKDOWN_5, NJ_PARSE_BREAKDOWN_4, NJ_PARSE_BREAKDOWN_4, NJ_PARSE_BREAKDOWN_3, NJ_PARSE_BREAKDOWN_3, NJ_PARSE_BREAKDOWN_2, NJ_PARSE_BREAKDOWN_2, NJ_PARSE_BREAKDOWN_1, NJ_PARSE_BREAKDOWN_1)(__VA_ARGS__))


// ALLOCATION
#define NJ_CHECK_SERIALIZE(field, stype, ordinal) \
    n_node = (Node) {.key = #field, .type=FIELD, .static_key=true}; \
    VEC_Push_Al(node->children, &n_node, (&(parser->allocator))); \
    json_serialize_ ##stype(&node->children.start[node->children.count-1], (void*)&(buf->field), sizeof(buf->field), parser);


#define NJ_SERIALIZE_BREAKDOWN_1(v1, v2) NJ_CHECK_SERIALIZE(v1, v2, 0)
#define NJ_SERIALIZE_BREAKDOWN_2(v1, v2, v3, v4) NJ_SERIALIZE_BREAKDOWN_1(v1, v2) NJ_CHECK_SERIALIZE(v3, v4, 1)
#define NJ_SERIALIZE_BREAKDOWN_3(v1, v2, v3, v4, v5, v6) NJ_SERIALIZE_BREAKDOWN_2(v1, v2, v3, v4) NJ_CHECK_SERIALIZE(v5, v6, 2)
#define NJ_SERIALIZE_BREAKDOWN_4(v1, v2, v3, v4, v5, v6, v7, v8) NJ_SERIALIZE_BREAKDOWN_3(v1, v2, v3, v4, v5, v6) NJ_CHECK_SERIALIZE(v7, v8, 3)
#define NJ_SERIALIZE_BREAKDOWN_5(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10) NJ_SERIALIZE_BREAKDOWN_4(v1, v2, v3, v4, v5, v6, v7, v8) NJ_CHECK_SERIALIZE(v9, v10, 4)
#define NJ_SERIALIZE_BREAKDOWN_6(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12) NJ_SERIALIZE_BREAKDOWN_5(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10) NJ_CHECK_SERIALIZE(v11, v12, 5)
#define NJ_SERIALIZE_BREAKDOWN_7(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14) NJ_SERIALIZE_BREAKDOWN_6(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12) NJ_CHECK_SERIALIZE(v13, v14, 6)
#define NJ_SERIALIZE_BREAKDOWN_8(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16) NJ_SERIALIZE_BREAKDOWN_7(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14) NJ_CHECK_SERIALIZE(v15, v16, 7)
#define NJ_SERIALIZE_BREAKDOWN_9(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18) NJ_SERIALIZE_BREAKDOWN_8(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16) NJ_CHECK_SERIALIZE(v17, v18, 8)
#define NJ_SERIALIZE_BREAKDOWN_10(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20) NJ_SERIALIZE_BREAKDOWN_9(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18) NJ_CHECK_SERIALIZE(v19, v20, 9)
#define NJ_SERIALIZE_BREAKDOWN_11(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22) NJ_SERIALIZE_BREAKDOWN_10(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20) NJ_CHECK_SERIALIZE(v21, v22, 10)
#define NJ_SERIALIZE_BREAKDOWN_12(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24) NJ_SERIALIZE_BREAKDOWN_11(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22) NJ_CHECK_SERIALIZE(v23, v24, 11)
#define NJ_SERIALIZE_BREAKDOWN_13(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26) NJ_SERIALIZE_BREAKDOWN_12(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24) NJ_CHECK_SERIALIZE(v25, v26, 12)
#define NJ_SERIALIZE_BREAKDOWN_14(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28) NJ_SERIALIZE_BREAKDOWN_13(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26) NJ_CHECK_SERIALIZE(v27, v28, 13)
#define NJ_SERIALIZE_BREAKDOWN_15(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30) NJ_SERIALIZE_BREAKDOWN_14(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28) NJ_CHECK_SERIALIZE(v29, v30, 14)
#define NJ_SERIALIZE_BREAKDOWN_16(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32) NJ_SERIALIZE_BREAKDOWN_15(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30) NJ_CHECK_SERIALIZE(v31, v32, 15)
#define NJ_SERIALIZE_BREAKDOWN_17(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34) NJ_SERIALIZE_BREAKDOWN_16(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32) NJ_CHECK_SERIALIZE(v33, v34, 16)
#define NJ_SERIALIZE_BREAKDOWN_18(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36) NJ_SERIALIZE_BREAKDOWN_17(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34) NJ_CHECK_SERIALIZE(v35, v36, 17)
#define NJ_SERIALIZE_BREAKDOWN_19(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38) NJ_SERIALIZE_BREAKDOWN_18(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36) NJ_CHECK_SERIALIZE(v37, v38, 18)
#define NJ_SERIALIZE_BREAKDOWN_20(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40) NJ_SERIALIZE_BREAKDOWN_19(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38) NJ_CHECK_SERIALIZE(v39, v40, 19)
#define NJ_SERIALIZE_BREAKDOWN_21(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42) NJ_SERIALIZE_BREAKDOWN_20(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40) NJ_CHECK_SERIALIZE(v41, v42, 20)
#define NJ_SERIALIZE_BREAKDOWN_22(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44) NJ_SERIALIZE_BREAKDOWN_21(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42) NJ_CHECK_SERIALIZE(v43, v44, 21)
#define NJ_SERIALIZE_BREAKDOWN_23(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46) NJ_SERIALIZE_BREAKDOWN_22(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44) NJ_CHECK_SERIALIZE(v45, v46, 22)
#define NJ_SERIALIZE_BREAKDOWN_24(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48) NJ_SERIALIZE_BREAKDOWN_23(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46) NJ_CHECK_SERIALIZE(v47, v48, 23)
#define NJ_SERIALIZE_BREAKDOWN_25(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50) NJ_SERIALIZE_BREAKDOWN_24(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48) NJ_CHECK_SERIALIZE(v49, v50, 24)
#define NJ_SERIALIZE_BREAKDOWN_26(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52) NJ_SERIALIZE_BREAKDOWN_25(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50) NJ_CHECK_SERIALIZE(v51, v52, 25)
#define NJ_SERIALIZE_BREAKDOWN_27(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54) NJ_SERIALIZE_BREAKDOWN_26(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52) NJ_CHECK_SERIALIZE(v53, v54, 26)
#define NJ_SERIALIZE_BREAKDOWN_28(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56) NJ_SERIALIZE_BREAKDOWN_27(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54) NJ_CHECK_SERIALIZE(v55, v56, 27)
#define NJ_SERIALIZE_BREAKDOWN_29(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58) NJ_SERIALIZE_BREAKDOWN_28(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56) NJ_CHECK_SERIALIZE(v57, v58, 28)
#define NJ_SERIALIZE_BREAKDOWN_30(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58, v59, v60) NJ_SERIALIZE_BREAKDOWN_29(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58) NJ_CHECK_SERIALIZE(v59, v60, 29)
#define NJ_SERIALIZE_BREAKDOWN_31(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58, v59, v60, v61, v62) NJ_SERIALIZE_BREAKDOWN_30(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58, v59, v60) NJ_CHECK_SERIALIZE(v61, v62, 30)

#define NJ_SERIALIZE_BREAKDOWN(...) NJ_M_EXPAND(NJ_GET_M(__VA_ARGS__, NJ_SERIALIZE_BREAKDOWN_31, NJ_SERIALIZE_BREAKDOWN_31, NJ_SERIALIZE_BREAKDOWN_30, NJ_SERIALIZE_BREAKDOWN_30, NJ_SERIALIZE_BREAKDOWN_29, NJ_SERIALIZE_BREAKDOWN_29, NJ_SERIALIZE_BREAKDOWN_28, NJ_SERIALIZE_BREAKDOWN_28, NJ_SERIALIZE_BREAKDOWN_27, NJ_SERIALIZE_BREAKDOWN_27, NJ_SERIALIZE_BREAKDOWN_26, NJ_SERIALIZE_BREAKDOWN_26, NJ_SERIALIZE_BREAKDOWN_25, NJ_SERIALIZE_BREAKDOWN_25, NJ_SERIALIZE_BREAKDOWN_24, NJ_SERIALIZE_BREAKDOWN_24, NJ_SERIALIZE_BREAKDOWN_23, NJ_SERIALIZE_BREAKDOWN_23, NJ_SERIALIZE_BREAKDOWN_22, NJ_SERIALIZE_BREAKDOWN_22, NJ_SERIALIZE_BREAKDOWN_21, NJ_SERIALIZE_BREAKDOWN_21, NJ_SERIALIZE_BREAKDOWN_20, NJ_SERIALIZE_BREAKDOWN_20, NJ_SERIALIZE_BREAKDOWN_19, NJ_SERIALIZE_BREAKDOWN_19, NJ_SERIALIZE_BREAKDOWN_18, NJ_SERIALIZE_BREAKDOWN_18, NJ_SERIALIZE_BREAKDOWN_17, NJ_SERIALIZE_BREAKDOWN_17, NJ_SERIALIZE_BREAKDOWN_16, NJ_SERIALIZE_BREAKDOWN_16, NJ_SERIALIZE_BREAKDOWN_15, NJ_SERIALIZE_BREAKDOWN_15, NJ_SERIALIZE_BREAKDOWN_14, NJ_SERIALIZE_BREAKDOWN_14, NJ_SERIALIZE_BREAKDOWN_13, NJ_SERIALIZE_BREAKDOWN_13, NJ_SERIALIZE_BREAKDOWN_12, NJ_SERIALIZE_BREAKDOWN_12, NJ_SERIALIZE_BREAKDOWN_11, NJ_SERIALIZE_BREAKDOWN_11, NJ_SERIALIZE_BREAKDOWN_10, NJ_SERIALIZE_BREAKDOWN_10, NJ_SERIALIZE_BREAKDOWN_9, NJ_SERIALIZE_BREAKDOWN_9, NJ_SERIALIZE_BREAKDOWN_8, NJ_SERIALIZE_BREAKDOWN_8, NJ_SERIALIZE_BREAKDOWN_7, NJ_SERIALIZE_BREAKDOWN_7, NJ_SERIALIZE_BREAKDOWN_6, NJ_SERIALIZE_BREAKDOWN_6, NJ_SERIALIZE_BREAKDOWN_5, NJ_SERIALIZE_BREAKDOWN_5, NJ_SERIALIZE_BREAKDOWN_4, NJ_SERIALIZE_BREAKDOWN_4, NJ_SERIALIZE_BREAKDOWN_3, NJ_SERIALIZE_BREAKDOWN_3, NJ_SERIALIZE_BREAKDOWN_2, NJ_SERIALIZE_BREAKDOWN_2, NJ_SERIALIZE_BREAKDOWN_1, NJ_SERIALIZE_BREAKDOWN_1)(__VA_ARGS__))

#define NJ_DECLARE_PARSE(Type) void json_parse_ ##Type(Node* node, void* out_raw, size_t elem_s, JsonParser* parser);  void json_parse_nj_array_ ##Type(Node* node, void* out_raw, size_t elem_s, JsonParser* parser); void json_parse_nj_ptr_ ##Type(Node* node, void* out_raw, size_t elem_s, JsonParser* parser);
#define NJ_DECLARE_VECTOR_PARSE(Type) void json_parse_nj_vector_ ##Type(Node* node, void* out_raw, size_t elem_s, JsonParser* parser);

// ASSERT
#define NJ_DEFINE_PARSE(Type, ...) void json_parse_ ##Type(Node* node, void* out_raw, size_t elem_s, JsonParser* parser) { \
        if(node->type == FIELD){ \
            json_parse_ ##Type(node->children.start, out_raw, elem_s, parser); \
            return; \
        } \
        bool field_marked[256] = {0};\
        Type* out = (Type*)out_raw; \
        for(size_t i = 0; i < node->children.count; i++){ \
            NJ_PARSE_BREAKDOWN(__VA_ARGS__) \
        } \
    } \
    void json_parse_nj_array_ ##Type(Node* node, void* out_raw, size_t elem_s, JsonParser* parser) { \
        if(node->type == FIELD){ \
            json_parse_nj_array_ ##Type(node->children.start, out_raw, elem_s, parser); \
            return; \
        } \
        assert(node->type == ARRAY && "Non array passed to array parser"); \
        for(size_t i = 0; i < node->children.count; i++) \
            json_parse_ ##Type(&node->children.start[i], (void*)(*(char**)((char*)out_raw+i*sizeof(Type))), sizeof(Type), parser);    \
    } \
    void json_parse_nj_ptr_ ##Type(Node* node, void* out_raw, size_t elem_s, JsonParser* parser) { \
        Type* t = parser->allocator.alloc(parser->allocator.context, sizeof(Type)); \
        (*(Type**)out_raw) = t; \
        json_parse_ ##Type(node, (void*)t, elem_s, parser); \
    }

#define NJ_DEFINE_VECTOR_PARSE(Type) void json_parse_nj_vector_ ##Type(Node* node, void* out_raw, size_t elem_s, JsonParser* parser) { \
        if(node->type == FIELD){ \
            json_parse_nj_vector_ ##Type(node->children.start, out_raw, elem_s, parser); \
            return; \
        } \
        assert(node->type == ARRAY && "Non array passed to array parser"); \
        typedef A_VEC(Type)* Type_vec_ptr; \
        Type_vec_ptr vec = (Type_vec_ptr)out_raw; \
        for(size_t i = 0; i < node->children.count; i++) { \
            Type t = {0}; \
            json_parse_ ##Type(&node->children.start[i], (void*)&t, sizeof(Type), parser); \
            VEC_Push_Ptr_Al(vec, &t, (&(parser->allocator))); \
        }   \
    }

#define NJ_DEFINE_MAPPING_PARSE(Type) void json_parse_nj_mapping_ ##Type(Node* node, void *out_raw, size_t elem_s, JsonParser* parser){ \
    if(node->type == FIELD){ \
        json_parse_nj_mapping_ ##Type(&node->children.start[0], out_raw, elem_s, parser); \
        return; \
    } \
    typedef struct { \
        char* key; Type value; \
    } mapping_type; \
    typedef A_VEC(mapping_type)* Type_vec_ptr; \
    Type_vec_ptr vec = (Type_vec_ptr)out_raw; \
    for(size_t i = 0; i < node->children.count; i++){ \
        mapping_type t = {0}; \
        json_parse_ ##Type(&node->children.start[i], (void*)&t.value, sizeof(Type), parser); \
        size_t l = strlen(node->children.start[i].key); \
        t.key = parser->allocator.alloc(parser->allocator.context, l+1); \
        strcpy_tn(t.key, l+1, node->children.start[i].key); \
        VEC_Push_Ptr_Al(vec, &t, (&(parser->allocator))); \
    } \
}

#define NJ_DECLARE_SERIALIZE(Type) void json_serialize_ ##Type(Node* node, void* buf_raw, size_t elem_s, JsonParser* parser); void json_serialize_nj_array_ ##Type(Node* node, void* buf_raw, size_t elem_s, JsonParser* parser); void json_serialize_nj_ptr_ ##Type(Node* node, void* buf_raw, size_t elem_s, JsonParser* parser);
#define NJ_DECLARE_VECTOR_SERIALIZE(Type) void json_serialize_nj_vector_ ##Type(Node* node, void* out_raw, size_t elem_s, JsonParser* parser);

#define NJ_DEFINE_SERIALIZE(Type, ...) void json_serialize_ ##Type(Node* node, void* buf_raw, size_t elem_s, JsonParser* parser) { \
        if(node->type == FIELD || node->type == ARRAY){ \
            Node n = (Node){.type=OBJECT}; \
            VEC_Push_Al(node->children, &n, (&(parser->allocator))); \
            json_serialize_ ##Type(node->children.start, buf_raw, elem_s, parser); \
            return; \
        } \
        Type* buf = (Type*)buf_raw; \
        Node n_node; \
        NJ_SERIALIZE_BREAKDOWN(__VA_ARGS__) \
    } \
    void json_serialize_nj_array_ ##Type(Node* node, void* buf_raw, size_t elem_s, JsonParser* parser) { \
        if(node->type == FIELD){ \
            Node n = (Node){.type=ARRAY}; \
            VEC_Push_Al(node->children, &n, (&(parser->allocator))); \
            json_serialize_nj_array_ ##Type(node->children.start, buf_raw, elem_s, parser); \
            return; \
        } \
        size_t elems = elem_s/sizeof(Type); \
        for(size_t i = 0; i < elems; i++) { \
            Node n = {.type = OBJECT}; \
            json_serialize_ ##Type(&n, (void*)((char*)buf_raw+i*elem_s), sizeof(Type), parser); \
            VEC_Push_Al(node->children, &n, (&(parser->allocator))); \
        } \
    } \
    void json_serialize_nj_ptr_ ##Type(Node* node, void* buf_raw, size_t elem_s, JsonParser* parser) { \
        json_serialize_ ##Type(node, (void*)(*(Type**)buf_raw), elem_s, parser); \
    }

#define NJ_DEFINE_VECTOR_SERIALIZE(Type) void json_serialize_nj_vector_ ##Type(Node* node, void* buf_raw, size_t elem_s, JsonParser* parser) { \
        if(node->type == FIELD){ \
            Node n = (Node){.type=ARRAY}; \
            VEC_Push_Al(node->children, &n, (&(parser->allocator))); \
            json_serialize_nj_vector_ ##Type(node->children.start, buf_raw, elem_s, parser); \
            return; \
        } \
        typedef A_VEC(Type)* Type_vec_ptr; \
        Type_vec_ptr buf = (Type_vec_ptr)buf_raw; \
        size_t elems = buf->count; \
        for(size_t i = 0; i < elems; i++) { \
            Node n = {.type = OBJECT}; \
            json_serialize_ ##Type(&n, (void*)(&buf->start[i]), sizeof(Type), parser); \
            VEC_Push_Al(node->children, &n, (&(parser->allocator))); \
        } \
    }

#define NJ_DEFINE_MAPPING_SERIALIZE(Type) void json_serialize_nj_mapping_ ##Type(Node* node, void *buf_raw, size_t elem_s, JsonParser* parser){ \
    if(node->type == FIELD){ \
        Node n = (Node){.type=OBJECT}; \
        VEC_Push_Al(node->children, &n, (&(parser->allocator))); \
        json_serialize_nj_mapping_ ##Type(node->children.start, buf_raw, elem_s, parser); \
        return; \
    } \
    typedef struct { \
        char* key; Type value; \
    } mapping_type; \
    typedef A_VEC(mapping_type)* Type_vec_ptr; \
    Type_vec_ptr buf = (Type_vec_ptr)buf_raw; \
    size_t elems = buf->count; \
    for(size_t i = 0; i < elems; i++) { \
        Node n = {.type = FIELD}; \
        Node n_c = {.type = OBJECT}; \
        mapping_type b = buf->start[i]; \
        size_t l = strlen(b.key)+1; \
        n.key = parser->allocator.alloc(parser->allocator.context, l); \
        strcpy_tn((char *)n.key, l, b.key); \
        json_serialize_ ##Type(&n_c, (void*)(&buf->start[i].value), sizeof(Type), parser); \
        VEC_Push_Al(n.children, &n_c, (&(parser->allocator))); \
        VEC_Push_Al(node->children, &n, (&(parser->allocator))); \
    } \
}

#define NJ_DEFINE_PARSE_SERIALIZE(Type, ...) NJ_DEFINE_PARSE(Type, __VA_ARGS__) NJ_DEFINE_SERIALIZE(Type, __VA_ARGS__)
#define NJ_DECLARE_PARSE_SERIALIZE(Type) NJ_DECLARE_PARSE(Type) NJ_DECLARE_SERIALIZE(Type)
#define NJ_DEFINE_VECTOR_PARSE_SERIALIZE(Type) NJ_DEFINE_VECTOR_PARSE(Type) NJ_DEFINE_VECTOR_SERIALIZE(Type)
#define NJ_DEFINE_MAPPING_PARSE_SERIALIZE(Type) NJ_DEFINE_MAPPING_PARSE(Type) NJ_DEFINE_MAPPING_SERIALIZE(Type)

#define NJ_DEFINE_BASIC_TYPE_ARRAY_PARSE(Type, underlying_parse) void json_parse_nj_array_ ##Type(Node* node, void* out_raw, size_t elem_s, JsonParser* parser) { \
        if(node->type == FIELD){ \
            json_parse_nj_array_ ##Type(node->children.start, out_raw, elem_s, parser); \
            return; \
        } \
        assert(node->type == ARRAY && "Non array passed to array parser"); \
        for(size_t i = 0; i < node->children.count; i++) \
            underlying_parse(&node->children.start[i], (void*)((char*)out_raw+i*sizeof(Type)), sizeof(Type), parser);    \
    }

#define NJ_DEFINE_BASIC_TYPE_ARRAY_SERIALIZE(Type, underlying_serialize) void json_serialize_nj_array_ ##Type(Node* node, void* buf_raw, size_t elem_s, JsonParser* parser) { \
        if(node->type == FIELD){ \
            Node node_n = {.type=ARRAY, .key="NR", .static_key=true}; \
            VEC_Push_Al(node->children, &node_n, (&(parser->allocator))); \
            json_serialize_nj_array_ ##Type(node->children.start, buf_raw, elem_s, parser); \
            return; \
        } \
        assert(node->type == ARRAY && "Non array passed to array parser"); \
        for(size_t i = 0; i < elem_s/sizeof(Type); i++) { \
            Node node_n = {0}; \
            VEC_Push_Al(node->children, &node_n, (&(parser->allocator))); \
            underlying_serialize(&node->children.start[i], (void*)((char*)buf_raw+i*sizeof(Type)), sizeof(Type), parser); \
        } \
    }

#define NJ_DECLARE_PARSE_SERIALIZE_ENUM(Type) void json_parse_nj_enum_##Type(Node* node, void* out_raw, size_t elem_s, JsonParser* parser);  json_serialize_nj_enum_##Type(Node* node, void* buf_raw, size_t elem_s, JsonParser* parser);
#define NJ_DEFINE_PARSE_SERIALIZE_ENUM(Type, names_ordered) void json_parse_nj_enum_##Type(Node* node, void* out, size_t elem_s, JsonParser* parser) { \
     if(node->type == FIELD){ \
        json_parse_nj_enum_##Type(&node->children.start[0], out, elem_s, parser); \
        return; \
    } \
    assert(node->flags == STRING && node->key != NULL && "Non strings cannot be parsed as string values"); \
    int val = 0; \
    for(int i = 0; i < sizeof(names_ordered)/sizeof(names_ordered[0]); i++){ \
        if(strcmp(names_ordered[i], node->key) == 0)\
            val = i;\
    }\
    *(int*)out = val; \
} \
void json_serialize_nj_enum_##Type(Node* node, void* buf_raw, size_t elem_s, JsonParser* parser) { \
    int ind = *(int*)buf_raw; \
    if(ind < 0 || ind > sizeof(names_ordered)/sizeof(names_ordered[0])) \
        ind = 0; \
    json_serialize_string(node, &names_ordered[ind], elem_s, parser); \
    \
} \


#define NJ_PARSE_NODE(Type, node, out, parser) json_parse_##Type(node, (void*)out, sizeof(Type), parser)
#define NJ_PARSE_NODE_VECTOR(Type, node, out, parser) json_parse_nj_vector_##Type(node, (void*)out, sizeof(Type), parser)

#define NJ_SERIALIZE_NODE(Type, node, buf_raw, parser) json_serialize_##Type(node, (void*)buf_raw, sizeof(Type), parser)
#define NJ_SERIALIZE_NODE_VECTOR(Type, node, out, parser) json_serialize_nj_vector_##Type(node, (void*)out, sizeof(Type), parser)

// DUBIOUS CODE GENERATION
#ifdef NJ_INTEGRAL_ARRAY_DEFINITIONS
#define json_serialize_int json_serialize_integral
#define json_parse_int json_parse_integral
#define NJ_DEFINE_INTEGRAL_ARRAY_TWO_WAY(x) NJ_DEFINE_BASIC_TYPE_ARRAY_PARSE(x, json_parse_integral) NJ_DEFINE_BASIC_TYPE_ARRAY_SERIALIZE(x, json_serialize_integral)

NJ_DEFINE_INTEGRAL_ARRAY_TWO_WAY(int);
NJ_DEFINE_INTEGRAL_ARRAY_TWO_WAY(short);
NJ_DEFINE_INTEGRAL_ARRAY_TWO_WAY(long);
NJ_DEFINE_INTEGRAL_ARRAY_TWO_WAY(uint8_t);
NJ_DEFINE_INTEGRAL_ARRAY_TWO_WAY(uint16_t);
NJ_DEFINE_INTEGRAL_ARRAY_TWO_WAY(uint32_t);
NJ_DEFINE_INTEGRAL_ARRAY_TWO_WAY(uint64_t);
NJ_DEFINE_INTEGRAL_ARRAY_TWO_WAY(int8_t);
NJ_DEFINE_INTEGRAL_ARRAY_TWO_WAY(int16_t);
NJ_DEFINE_INTEGRAL_ARRAY_TWO_WAY(int32_t);
NJ_DEFINE_INTEGRAL_ARRAY_TWO_WAY(int64_t); 
NJ_DEFINE_INTEGRAL_ARRAY_TWO_WAY(time_t); 

NJ_DEFINE_BASIC_TYPE_ARRAY_PARSE(float, json_parse_floating);
NJ_DEFINE_BASIC_TYPE_ARRAY_PARSE(double, json_parse_floating);

NJ_DEFINE_BASIC_TYPE_ARRAY_SERIALIZE(float, json_serialize_floating);
NJ_DEFINE_BASIC_TYPE_ARRAY_SERIALIZE(double, json_serialize_floating);

#endif

// IMPLEMENTATION STARTS HERE

#ifdef NJ_IMPLEMENTATION
#ifndef NJ_IMPLEMENTATION_GUARD
#define NJ_IMPLEMENTATION_GUARD

void json_init_node(Node* node, NodeType type){
    *node = (Node){.type=type, .key="R",  .static_key=true};
}

bool json_is_comment(const char* c){
    return strncmp(c, "//", 2) == 0;
}

bool is_json_value(const char* buffer, size_t buffer_len){
    return (isalnum(buffer[0]) && isalnum(buffer_len-1)) || strcmp(buffer, "true") == 0 || strcmp(buffer, "false") == 0 || strcmp(buffer, "null") == 0;
}

typedef enum {
    ERR_INITIAL_NOT_OBJECT_OR_ARRAY,
    ERR_NUMBER_WRONG_EXPONENT,
    ERR_VALUE_CHILD_OF_NON_FIELD,
    ERR_FIELD_SEPARATOR_AFTER_VALUE,
    ERR_NUMBER_NON_NUMERICAL_VALUE,
    ERR_ARRAY_CHILD_NOT_VALUE,
    ERR_UNEXPECTED_TOKEN,
    ERR_SCOPE_CLOSURE_MISMATCH
} ParserError;

typedef enum {
    OBJECT_START = 0,
    OBJECT_END = 1,
    ARRAY_START = 2,
    ARRAY_END = 3,
    COMMA_SEPARATOR = 4,
    BOOLEAN_TRUE_VALUE = 5,
    BOOLEAN_FALSE_VALUE = 6,
    NULL_TOKEN = 7,
    NUMERICAL_EXPRESSION = 8,
    STRING_VALUE = 9,
    STRING_VALUE_END = 10,
    UNKNOWN_TOKEN = 11,
    FIELD_SEPARATOR = 12,
    COMMENT = 13
} State;

Node* create_nodes_from_parent(const char* buffer, size_t buffer_len, int* error_code, JsonParser* parser){
    Node* current_node = (Node*)parser->allocator.alloc(parser->allocator.context, sizeof(Node)); 
    (*current_node) = (Node){.type=OBJECT, .key="R", .static_key=true, .children={0}, .parent=NULL};
    State current_state = UNKNOWN_TOKEN;
    State prev_state = UNKNOWN_TOKEN;
    int quotes = 2;
    
    char expression[MAX_EXPRESSION_LENGTH] = {0};
    size_t expr_len = 0;
    bool escape = false;
    bool number_post_exponent = false;

    for(size_t i = 0; i < buffer_len; i++){
        char c = buffer[i];
        if(i == 5713)
            i += 0;
        if(current_state == UNKNOWN_TOKEN){
            if(isspace(c))
                continue;
            if(c != '{' && c != '['){
                if(error_code != NULL) *error_code = ERR_INITIAL_NOT_OBJECT_OR_ARRAY;
                return NULL; // SET ERROR: parent type must be an object or an array
            }
            if(c == '['){
                current_state = ARRAY_START;
                current_node->type = ARRAY;
            }
            else {
                current_state = OBJECT_START;
                current_node->type = OBJECT;
            }
            continue;
        }

        if(current_state != STRING_VALUE && current_state != COMMENT && json_is_comment((buffer+i))){
            prev_state = current_state;
            current_state = COMMENT;
        }
        if(current_state == COMMENT){
            if(c != '\n')
                continue;
            current_state = prev_state;
        }
        if(quotes == 2 && isspace(c))
            continue;
        
        if(current_state == OBJECT_START || current_state == ARRAY_START || current_state == FIELD_SEPARATOR || current_state == COMMA_SEPARATOR){
            if(c == '{' || c == '['){
                current_state = c == '{' ? OBJECT_START : c == '[' ? ARRAY_START : current_state;
                Node n_node = (Node) {.type = c == '{' ? OBJECT : c == '[' ? ARRAY : FIELD, .key="NR", .static_key=true, .children={0}, .parent=current_node};
                VEC_Push_Al(current_node->children, &n_node, (&(parser->allocator)));
                current_node = &(current_node->children.start[current_node->children.count - 1]);
            }

            else if((current_state == OBJECT_START || current_state == COMMA_SEPARATOR) && c == '}')
                current_state = OBJECT_END;
            

            else if((current_state == ARRAY_START || current_state == COMMA_SEPARATOR) && c == ']')
                current_state = ARRAY_END;
            
            
            else if(c == '"' && !escape){
                current_state = STRING_VALUE;
            }

            else if(c == 't')
                current_state = BOOLEAN_TRUE_VALUE;
            
            else if(c == 'f')
                current_state = BOOLEAN_FALSE_VALUE;
            
            else if(c == 'n')
                current_state = NULL_TOKEN;
            
            else if(('0' <= c && c <= '9') || c == '-')
                current_state = NUMERICAL_EXPRESSION;

            else if(c == ',') // expected comma 
                current_state = COMMA_SEPARATOR;
            else {
                if(error_code != NULL) *error_code = ERR_UNEXPECTED_TOKEN;
                return NULL; // SET ERROR: unidentified value type
            }
        }

        if(current_state == BOOLEAN_FALSE_VALUE || current_state == BOOLEAN_TRUE_VALUE || current_state == NULL_TOKEN || current_state == NUMERICAL_EXPRESSION){
            if(c == ',' || c == ']' || c == '}'){
                if(number_post_exponent || (current_state == NUMERICAL_EXPRESSION && (expression[expr_len-1] == '-' || expression[expr_len-1] == '+'))){
                    if(error_code != NULL) *error_code = ERR_NUMBER_WRONG_EXPONENT;
                    return NULL; // SET ERROR: no exponent value provided for numerical type
                }

                Node n_node = (Node) {.type = VALUE, .key=NULL, .children={0}, .parent=current_node};
                n_node.key = (const char *)parser->allocator.alloc(parser->allocator.context, expr_len+1); // ALLOCATION
                copystrn((char*)n_node.key, expression, expr_len+1);
                //printf("%.*s\n", expr_len, expression);
                number_post_exponent = false;
                expression[0] = '\0';
                expr_len = 0;

                n_node.flags = (NodeFlags)current_state;
                current_state = c == ',' ? COMMA_SEPARATOR : c == ']' ? ARRAY_END : OBJECT_END;
                VEC_Push_Al(current_node->children, &n_node, (&(parser->allocator)));

                if(current_node->type == OBJECT){
                    if(error_code != NULL) *error_code = ERR_VALUE_CHILD_OF_NON_FIELD;
                    return NULL; // SET ERROR, values cannot be direct children of objects
                }
                if(current_node->type == FIELD)
                   current_node = current_node->parent; 

            } else if(c == ':' || (c == '"' && !escape)){
                if(error_code != NULL) *error_code = ERR_FIELD_SEPARATOR_AFTER_VALUE;
                return NULL; // SET ERROR, cant nest fields without objects
            }
            else {
                if(current_state == NUMERICAL_EXPRESSION){
                    if(!(('0' <= c && c <= '9') || c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E')){
                        if(error_code != NULL) *error_code = ERR_NUMBER_NON_NUMERICAL_VALUE;
                        return NULL;

                    } else if(expr_len > 0 && !number_post_exponent && (c == '-' || c == '+')){
                        if(error_code != NULL) *error_code = ERR_NUMBER_NON_NUMERICAL_VALUE;
                        return NULL;
                    }
                    else if(c == 'e' || c == 'E') {
                        if(number_post_exponent){
                            if(error_code != NULL) *error_code = ERR_NUMBER_NON_NUMERICAL_VALUE;
                            return NULL;

                        }
                        number_post_exponent = true;
                    } else if(c == '+' && !number_post_exponent){
                        if(error_code != NULL) *error_code = ERR_NUMBER_NON_NUMERICAL_VALUE;
                        return NULL;
                    }
                    else if(number_post_exponent){
                        number_post_exponent = false;
                    }
                }
                expression[expr_len++] = c;
                // if(c == '\\')
                //     expression[expr_len++] = c;  
            }
        }
        if(current_state == STRING_VALUE){
            if(c == '"' && !escape)
                quotes -= 1;
            
            if(quotes == 0){
                current_state = STRING_VALUE_END;
                quotes = 2;
                continue;
            }

            else{
                expression[expr_len++] = c;
                if(c == '\\')
                    expression[expr_len++] = c;  
            }
        }
            
        if(current_state == STRING_VALUE_END){
            if (c == ',' || c == ']' || c == '}'){
                current_state = c == ',' ? COMMA_SEPARATOR : c == ']' ? ARRAY_END : OBJECT_END;

                Node n_node = (Node) {.type = VALUE, .key=NULL, .children={0}, .parent=current_node};
                n_node.key = (const char *)parser->allocator.alloc(parser->allocator.context, expr_len); // ALLOCATION
                copystrn((char*)n_node.key, (expression+1), expr_len);
                //printf("%.*s\n", expr_len, expression);
                expression[0] = '\0';
                expr_len = 0;

                n_node.flags = (NodeFlags)STRING;
                VEC_Push_Al(current_node->children, &n_node, (&(parser->allocator)));
                if(current_node->type == FIELD)
                   current_node = current_node->parent; 

            }
            else if(c == ':'){
                current_state = FIELD_SEPARATOR;
                if(current_node->type == ARRAY) {
                    if(error_code != NULL) *error_code = ERR_ARRAY_CHILD_NOT_VALUE;
                    return NULL; // SET ERROR: cannot define fields as direct children of an array
                }

                Node n_node = (Node) {.type = FIELD, .key=NULL, .children={0}, .parent=current_node};
                n_node.key = (const char *)parser->allocator.alloc(parser->allocator.context, expr_len); // ALLOCATION
                copystrn((char*)n_node.key, (expression+1), expr_len);
                //printf("%.*s\n", expr_len, expression);
                expression[0] = '\0';
                expr_len = 0;

                VEC_Push_Al(current_node->children, &n_node, (&(parser->allocator)));
                current_node = &(current_node->children.start[current_node->children.count - 1]);
            } else {
                if(error_code != NULL) *error_code = ERR_UNEXPECTED_TOKEN;
                return NULL; // SET ERROR: unknown token after string end 
            }

        }        

        if(current_state == OBJECT_END || current_state == ARRAY_END){
            do {
                // if(current_node->parent == NULL){
                //     if(error_code != NULL) *error_code = ERR_SCOPE_CLOSURE_MISMATCH;
                //     return NULL;
                // }

                if((current_state == OBJECT_END && current_node->type == ARRAY) ||
                    (current_state == ARRAY_END && current_node->type == OBJECT)){
                    if(error_code != NULL) *error_code = ERR_SCOPE_CLOSURE_MISMATCH;

                    return NULL;
                }

                if((current_node->type != OBJECT && current_node->type != ARRAY) || strcmp(current_node->key, "R") != 0){
                    current_node = current_node->parent;
                }

            } while(current_node->type == FIELD);
            current_state = COMMA_SEPARATOR;   
        }

        if(c == '\\')
            escape = !escape;
        else
            escape = false;
        
    }
    if(current_state != OBJECT_END && current_state != ARRAY_END && current_state != COMMA_SEPARATOR){
        if(error_code != NULL) *error_code = ERR_UNEXPECTED_TOKEN;
        return NULL; // SET ERROR: unexpected string in ending of json
    }
    
    // Maybe this actually should be an error
    assert((current_node->type == OBJECT || current_node->type == ARRAY) && (strcmp(current_node->key, "R") == 0) && "Scope closure error in parsing");
    return current_node;
}

void free_json(Node* parent, bool free_root, JsonParser* parser){
    for(size_t i = 0; i < parent->children.count; i++)
        free_json(&parent->children.start[i], false, parser);
    if(parent->children.start != NULL){
        parent->children.count = 0;
        VEC_Free(parent->children);
    }
    if(parent->key != NULL && !parent->static_key)
        parser->allocator.free(parser->allocator.context, (void*)parent->key);
    if(free_root)
        parser->allocator.free(parser->allocator.context, parent);
}

size_t json_approximate_size(Node* node, size_t add_indent){
    size_t total_size = add_indent;
    for(size_t i = 0; i < node->children.count; i++){
        total_size += json_approximate_size(&node->children.start[i], add_indent)+add_indent;
    }
    return strnlen(node->key, 1024) + total_size + 6;
}

size_t output_node(Node* node, char* buffer, size_t buffer_max, size_t cur_pos, size_t cur_indent, size_t add_indent){
    if(node->type == OBJECT){
        size_t child_pos = cur_pos + 1;
        buffer[cur_pos] = '{';
        if(add_indent > 0){
            child_pos += sprintf((buffer + child_pos), "\n%*.s", (int)(cur_indent+add_indent), " ");
        }
        for(size_t i = 0; i < node->children.count; i++){
            child_pos = output_node(&node->children.start[i], buffer, buffer_max-child_pos, child_pos, cur_indent+add_indent, add_indent);
            if(i < node->children.count - 1)
                buffer[child_pos++] = ',';
            if(add_indent > 0)
                child_pos += sprintf((buffer + child_pos), "\n%*.s", (int)(i < node->children.count - 1 ? cur_indent+add_indent : cur_indent), " ");
        }
        buffer[child_pos++] = '}';
        return child_pos;
    }
    if(node->type == ARRAY){
        size_t child_pos = cur_pos + 1;
        buffer[cur_pos] = '[';
        if(add_indent > 0){
            child_pos += sprintf((buffer + child_pos), "\n%*.s", (int)(cur_indent+1), " ");
        }
        for(size_t i = 0; i < node->children.count; i++){
            child_pos = output_node(&node->children.start[i], buffer, buffer_max-child_pos, child_pos, cur_indent+add_indent, add_indent);
            if(i < node->children.count - 1)
                buffer[child_pos++] = ',';
            if(add_indent > 0)
                child_pos += sprintf((buffer + child_pos), "\n%*.s", (int)(i < node->children.count - 1 ? cur_indent+add_indent : cur_indent), " ");
        }
        buffer[child_pos++] = ']';
        return child_pos;
    }

    if(node->type == FIELD){
        size_t l = strlen(node->key);
        snprintf((buffer+cur_pos), l+4, "\"%s\":", node->key);
        assert(node->children.count > 0 && "An orphaned field was unepexctedly encountered");
        size_t child_pos = output_node(&node->children.start[0], buffer, buffer_max-cur_pos-l-3, cur_pos+l+3, cur_indent, add_indent);
        return child_pos;
    }
    if(node->type == VALUE){
        size_t l = 0;
        if(node->key == NULL && node->flags >= NUMERICAL){
            copystrn((buffer + cur_pos), "\"\"", 3);
            return cur_pos + 2;
        } else if(node->key != NULL) {
            l = strlen(node->key);
        }
        switch(node->flags){
            case NUMERICAL:
                copystrn((buffer + cur_pos), node->key, l+1);
                return cur_pos + l;
            case NULL_VALUE:
                copystrn((buffer + cur_pos), "null", 5);
                return cur_pos + 4;
            case TRUE_VALUE:
                copystrn((buffer + cur_pos), "true", 5);
                return cur_pos + 4;
            case FALSE_VALUE:
                copystrn((buffer + cur_pos), "false", 6);
                return cur_pos + 5;
            default:
                snprintf((buffer+cur_pos), l+3, "\"%s\"", node->key);
            
        }
        return cur_pos+l+2;
    }
    return 0; // SET ERROR
}

void json_parse_array(Node* node, void* out, json_parser_t parse_cb, size_t elem_s, JsonParser* parser){
    if(node->type == FIELD){
        json_parse_array(&node->children.start[0], out, parse_cb, elem_s, parser);
        return;
    }
    assert(node->type == ARRAY && "Non array passed to array parser");
    for(size_t i = 0; i < node->children.count; i++)
        parse_cb(&node->children.start[i], (void*)((char*)out+i*elem_s), elem_s, parser);
}

void json_parse_integral(Node* node, void* out, size_t elem_s, JsonParser* parser){
    if(node->type == FIELD){
        json_parse_integral(&node->children.start[0], out, elem_s, parser);
        return;
    }
    assert(node->flags == NUMERICAL && "Non numerical expressions cannot be parsed as numerical types");
    if(elem_s == sizeof(int8_t)){
        *(int8_t*)out = (int8_t)(atoi(node->key) & 0xFF);
    }
    if(elem_s == sizeof(int16_t)){
        *(int16_t*)out = (int16_t)(atoi(node->key) & 0xFFFF);
    }
    if(elem_s == sizeof(int32_t)){
        *(int32_t*)out = (int32_t)(atoll(node->key)); // atoll MAY BE UNSAFE!!!
    }
    if(elem_s == sizeof(int64_t)){
        *(int64_t*)out = (int64_t)(atoll(node->key));
    }
}

void json_parse_floating(Node* node, void* out, size_t elem_s, JsonParser* parser){
    if(node->type == FIELD){
        json_parse_floating(&node->children.start[0], out, elem_s, parser);
        return;
    }
    assert(node->flags == NUMERICAL && "Non numerical expressions cannot be parsed as numerical types");
    if(elem_s == sizeof(float)){
        *(float*)out = (float)(strtod(node->key, NULL));
    }
    if(elem_s == sizeof(double)){
        *(double*)out = (double)(strtod(node->key, NULL));
    }
}

void json_parse_string(Node* node, void* out, size_t elem_s, JsonParser* parser){
    if(node->type == FIELD){
        json_parse_string(&node->children.start[0], out, elem_s, parser);
        return;
    }
    assert(node->flags == STRING && "Non strings cannot be parsed as string values");
    size_t l = strlen(node->key)+1;
    if(*(char**)out == NULL){
        *(char**)out = parser->allocator.alloc(parser->allocator.context, l);
    }
    strcpy_tn(*(char**)out, l, node->key); // also potentially unsafe
}

void json_parse__Bool(Node* node, void* out, size_t elem_s, JsonParser* parser){
    if(node->type == FIELD){
        json_parse__Bool(&node->children.start[0], out, elem_s, parser);
        return;
    }
    assert((node->flags == TRUE_VALUE || node->flags == FALSE_VALUE) && "Non boolean cannot be parsed as boolean");
    *(bool*)out = (node->flags == TRUE_VALUE);
}

void json_serialize_integral(Node* node, void* buf, size_t elem_s, JsonParser* parser){
    if(node->type == FIELD){
        VEC_Push_Al(node->children, &((Node){0}), (&(parser->allocator))); // ALLOCATION (IMPLICIT, (&(parser->allocator)))
        json_serialize_integral(&node->children.start[0], buf, elem_s, parser);
        return;
    }
    node->flags = NUMERICAL;
    node->type = VALUE;
    node->key = parser->allocator.alloc(parser->allocator.context, MAX_NUMERICAL_EXPRESSION_DIGITS); // ALLOCATION

    assert(node->flags == NUMERICAL && "Non numerical expressions cannot be parsed as numerical types");
    if(elem_s == sizeof(int8_t)){
        int8_t val = *(int8_t*)buf;
        sprintf((char *)node->key, "%"PRIi8, val);
    }
    if(elem_s == sizeof(int16_t)){
        int16_t val = *(int16_t*)buf;
        sprintf((char *)node->key, "%"PRIi16, val);
    }
    if(elem_s == sizeof(int32_t)){
        int32_t val = *(int32_t*)buf;
        sprintf((char *)node->key, "%"PRIi32, val);
    }
    if(elem_s == sizeof(int64_t)){
        int64_t val = *(int64_t*)buf;
        sprintf((char *)node->key, "%"PRIi64, val);
    }
}

void json_serialize_floating(Node* node, void* buf, size_t elem_s, JsonParser* parser){
    if(node->type == FIELD){
        VEC_Push_Al(node->children, &((Node){0}), (&(parser->allocator))); // ALLOCATION (IMPLICIT, (&(parser->allocator)))
        json_serialize_floating(&node->children.start[0], buf, elem_s, parser);
        return;
    }
    node->flags = NUMERICAL;
    node->type = VALUE;
    node->key = parser->allocator.alloc(parser->allocator.context, MAX_NUMERICAL_EXPRESSION_DIGITS); // ALLOCATION

    assert(node->flags == NUMERICAL && "Non numerical expressions cannot be parsed as numerical types");
    if(elem_s == sizeof(float)){
        float val = *(float*)buf;
        snprintf((char *)node->key, MAX_NUMERICAL_EXPRESSION_DIGITS-1, "%f", val); // ADD PRECISION
    }
    if(elem_s == sizeof(double)){
        double val = *(double*)buf;
        snprintf((char *)node->key, MAX_NUMERICAL_EXPRESSION_DIGITS-1, "%f", val);
    }
}

void json_serialize_string(Node* node, void* buf, size_t elem_s, JsonParser* parser){
    if(node->type == FIELD){
        VEC_Push_Al(node->children, &((Node){0}), (&(parser->allocator))); // ALLOCATION (IMPLICIT, (&(parser->allocator)))
        json_serialize_string(&node->children.start[0], buf, elem_s, parser);
        return;
    }
    node->flags = STRING;
    node->type = VALUE;
    char* b = *(char**)buf;
    if(b == NULL)
        return;
    
    size_t l = strlen(b)+1;
    node->key = parser->allocator.alloc(parser->allocator.context, l); // ALLOCATION
    strcpy_tn((char *)node->key, l, b); // also potentially unsafe

}

void json_serialize__Bool(Node* node, void* buf, size_t elem_s, JsonParser* parser){
    if(node->type == FIELD){
        VEC_Push_Al(node->children, &((Node){0}), (&(parser->allocator))); // ALLOCATION (IMPLICIT, (&(parser->allocator)))
        json_serialize__Bool(&node->children.start[0], buf, elem_s, parser);
        return;
    }
    node->type = VALUE;
    node->flags = *(bool*)buf ? TRUE_VALUE : FALSE_VALUE;
    node->key = NULL; // No need to waste memory on text
}

void json_serialize_nj_mapping(Node* node, void* buf, size_t elem_s, JsonParser* parser){

}

#endif
#endif