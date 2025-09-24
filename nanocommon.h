#pragma once
#include <inttypes.h>
#include <malloc.h>

#ifndef NC_ALLOCATE
    #define NC_ALLOCATE malloc
#endif
#ifndef NC_REALLOCATE
    #define NC_REALLOCATE realloc
#endif
#ifndef NC_FREE
    #define NC_FREE free
#endif

#define A_VEC(Type) struct { Type* start; size_t count; size_t capacity; } // anonymous vector
#define VEC(Type) typedef struct Type##_Vec { Type* start; size_t count; size_t capacity; };
#define VEC_Push(vec, value) if(vec.count >= vec.capacity) { \
                                    vec.capacity = vec.capacity == 0 ? 8 : vec.capacity * 1.5; \
                                    vec.start = NC_REALLOCATE((void*)vec.start, vec.capacity*sizeof(*vec.start));} \
                            memcpy((void*)(vec.start + vec.count++), (void*)value, sizeof(*value))

#define VEC_Push_Ptr(vec, value) if(vec->count >= vec->capacity) { \
                                    vec->capacity = vec->capacity == 0 ? 8 : vec->capacity * 1.5; \
                                    vec->start = NC_REALLOCATE((void*)vec->start, vec->capacity*sizeof(*vec->start));} \
                            memcpy((void*)(vec->start + vec->count++), (void*)value, sizeof(*value))

#define VEC_Get(vec, index) vec.start + index
#define VEC_Remove(vec, index) if(index < vec.count - 1) {memmove((void*)(vec.start + index), (void*)(vec.start + (index+1)), (vec.count - 1 - index)*sizeof(*vec.start));}; vec.count--
#define VEC_Free(vec) if(vec.start != NULL) NC_FREE(vec.start)

#define copystrn(dest, src, length) snprintf(dest, length, "%s", src)

typedef struct StrView {
    char* start;
    size_t length;
} StrView;

size_t to_buffer(StrView* str, char* buffer, size_t max_size);

typedef void* (*allocator_allocate_t)(void*, size_t);
typedef void* (*allocator_reallocate_t)(void*, void*, size_t);
typedef bool (*allocator_free_t)(void*, void*);

typedef struct allocator_t {
    void* context;
    allocator_allocate_t alloc;
    allocator_reallocate_t realloc;
    allocator_free_t free;
} allocator_t;

typedef allocator_t arena_allocator;
typedef allocator_t system_allocator;

void n_arena_allocator_init(arena_allocator* allocator, size_t max_memory);
void* n_arena_allocator_realloc(void* allocator, void* start, size_t size);
void* n_arena_allocator_alloc(void* allocator, size_t size);
bool n_arena_allocator_free(void* allocator, void* start);

void n_system_allocator_init(system_allocator* allocator);

#ifdef NC_IMPLEMENTATION
#ifndef NC_IMPLEMENTATION_GUARD
#define NC_IMPLEMENTATION_GUARD

size_t to_buffer(StrView* str, char* buffer, size_t max_size){
    size_t copy_size = max_size > str->length +1 ? str->length + 1 : max_size;
    memcpy(buffer, (const void*)str->start, copy_size - 1);
    buffer[copy_size] = '\0';
    return copy_size;
}

typedef struct {
    void* start;
    size_t total_memory;
} arena_allocator_context;

static void* n_system_wrapper_alloc(void* allocator, size_t size){ if(allocator != NULL) return NULL; return malloc(size); }
static void* n_system_wrapper_realloc(void* allocator, void* start, size_t size){ if(allocator != NULL) return NULL; return realloc(start, size); }
static bool n_system_wrapper_free(void* allocator, void* start){ free(start); return allocator == NULL; }

void n_system_allocator_init(system_allocator* allocator){
    allocator->alloc = n_arena_allocator_alloc;
    allocator->realloc = n_arena_allocator_realloc;
    allocator->free = n_arena_allocator_free;
}


void n_arena_allocator_init(arena_allocator* allocator, size_t max_memory){
    arena_allocator_context* c = malloc(sizeof(arena_allocator_context));
    c->start = malloc(max_memory);
    c->total_memory = max_memory;
    *(int*)c->start = (int)max_memory;
    allocator->context = c;
    allocator->alloc = n_arena_allocator_alloc;
    allocator->realloc = n_arena_allocator_realloc;
    allocator->free = n_arena_allocator_free;
}

void* n_arena_allocator_realloc(void* allocator, void* start, size_t size){
    if(allocator == NULL)
        return NULL;

    if (start == NULL){
        return n_arena_allocator_alloc(allocator, size);
    }

    void* cur = (uint32_t*)start - 1;
    uint32_t to_skip = ((*(uint32_t*)cur) ^ (1 << 31));

    void* after_start = ((char*)cur + to_skip);
    if((((((*(uint32_t*)after_start) & (1 << 31)) > 0) &&
            (size - to_skip) + sizeof(uint32_t) < (*(uint32_t*)after_start & (~(1 << 31)))))
        || (size + sizeof(uint32_t) < to_skip)){
        uint32_t free_after = *(uint32_t*)after_start;
        *(uint32_t*)after_start = 0;
        *(uint32_t*)((char*)cur + size + sizeof(uint32_t)) = free_after - size - sizeof(uint32_t);
        return start;
    }

    void* new_start = n_arena_allocator_alloc(allocator, size);
    if(new_start == NULL)
        return NULL;
    
    uint32_t free_after = 0;
    if((((*(uint32_t*)after_start) & (1 << 31)) == 0)){
        free_after = *(uint32_t*)after_start;
        *(uint32_t*)after_start = 0;
    }
    *(uint32_t*)cur = to_skip + free_after;

    memcpy(new_start, start, to_skip - sizeof(uint32_t));
    return new_start;
}

void* n_arena_allocator_alloc(void* allocator, size_t size){
    if(allocator == NULL)
        return NULL;
    arena_allocator_context* c = (arena_allocator_context*)allocator;
    
    void* cur = c->start;
    void* alloc_end = (char*)c->start + c->total_memory; 
    size_t size_to_alloc = size + sizeof(uint32_t);

    while((cur < alloc_end && (((*(uint32_t*)cur) & (1 << 31)) > 0)) || ((*(uint32_t*)cur) & (~(1 << 31))) < size_to_alloc){
        uint32_t to_skip = ((*(uint32_t*)cur) ^ (1 << 31));
        cur = (char*)cur + to_skip;
    }
    uint32_t cur_free = *(uint32_t*)cur;
    if((char*)alloc_end - (char*)cur < (ptrdiff_t)size_to_alloc)
        return NULL; // not enough memory
    
    *(uint32_t*)cur = ((uint32_t)(1 << 31)) | size_to_alloc;
    uint32_t free_after_alloc = cur_free - size_to_alloc;
    if(free_after_alloc > 0){
        *(uint32_t*)((char*)cur + size_to_alloc) = free_after_alloc;
    } 
    return (void*)((char*)cur + sizeof(uint32_t));
}

bool n_arena_allocator_free(void* allocator, void* start){
    if(allocator == NULL || start != NULL)
        return false;
    arena_allocator_context* c = (arena_allocator_context*)allocator;
    free(c->start);
    free(c);
    return true;
}
#endif
#endif