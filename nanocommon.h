#pragma once
#include <stdbool.h>
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
#define VEC(Type) typedef struct Type##_Vec { Type* start; size_t count; size_t capacity; } Type##_Vec;
#define VEC_Push(vec, value) if(vec.count >= vec.capacity) { \
                                    vec.capacity =  vec.capacity == 0 ? 1 : vec.capacity < 16 ? vec.capacity << 1 : vec.capacity * 1.5; \
                                    vec.start = NC_REALLOCATE((void*)vec.start, vec.capacity*sizeof(*vec.start));} \
                            memcpy((void*)(vec.start + vec.count++), (void*)value, sizeof(*value))
#define VEC_Push_Al(vec, value, allocator) if(vec.count >= vec.capacity) { \
                                    vec.capacity =  vec.capacity == 0 ? 1 : vec.capacity < 16 ? vec.capacity << 1 : vec.capacity * 1.5; \
                                    vec.start = allocator->realloc(allocator->context, (void*)vec.start, vec.capacity*sizeof(*vec.start));} \
                            memcpy((void*)(vec.start + vec.count++), (void*)value, sizeof(*value))

#define VEC_Push_Ptr(vec, value) if(vec->count >= vec->capacity) { \
                                    vec->capacity =  vec->capacity == 0 ? 1 : vec->capacity < 16 ? vec->capacity << 1 : vec->capacity * 1.5; \
                                    vec->start = NC_REALLOCATE((void*)vec->start, vec->capacity*sizeof(*vec->start));} \
                            memcpy((void*)(vec->start + vec->count++), (void*)value, sizeof(*value))

#define VEC_Push_Ptr_Al(vec, value, allocator) if(vec->count >= vec->capacity) { \
                                    vec->capacity =  vec->capacity == 0 ? 1 : vec->capacity < 16 ? vec->capacity << 1 : vec->capacity * 1.5; \
                                    vec->start = allocator->realloc(allocator->context, (void*)vec->start, vec->capacity*sizeof(*vec->start));} \
                            memcpy((void*)(vec->start + vec->count++), (void*)value, sizeof(*value))

#define VEC_Get(vec, index) vec.start + index
#define VEC_Remove(vec, index) if(index < vec.count - 1) {memmove((void*)(vec.start + index), (void*)(vec.start + (index+1)), (vec.count - 1 - index)*sizeof(*vec.start));}; vec.count--
#define VEC_Free(vec) if(vec.start != NULL) NC_FREE(vec.start)

#define VEC_Init_Reserve(vec, capacity) do { vec.start = NC_ALLOCATE(capacity*sizeof(*vec.start)); vec->capacity=capacity; } while(0); 
#define VEC_Init_Reserve_Al(vec, capacity, allocator) do { vec.start = allocator.allocate(allocator->context, capacity*sizeof(*vec.start)); vec->capacity=capacity; } while(0); 

#define copystrn(dest, src, length) snprintf(dest, length, "%s", src)

typedef struct StrView {
    const char* start;
    size_t length;
} StrView;

VEC(StrView);

StrView_Vec split_str(const char* buffer, size_t length, char delimiter);
size_t to_buffer(StrView* str, char* buffer, size_t max_size);
int strcpy_tn(char *_Dst, size_t _SizeInBytes, const char *_Src);

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

void n_arena_allocator_init(arena_allocator* allocator, size_t max_memory, bool flexible_memory);
void* n_arena_allocator_realloc(void* allocator, void* start, size_t size);
void* n_arena_allocator_alloc(void* allocator, size_t size);
bool n_arena_allocator_free(void* allocator, void* start);

void n_system_allocator_init(system_allocator* allocator);

#ifdef NC_IMPLEMENTATION
#ifndef NC_IMPLEMENTATION_GUARD
#define NC_IMPLEMENTATION_GUARD
#include <assert.h>
#include <string.h>

size_t to_buffer(StrView* str, char* buffer, size_t max_size){
    size_t copy_size = max_size > str->length +1 ? str->length + 1 : max_size;
    memcpy(buffer, (const void*)str->start, copy_size - 1);
    buffer[copy_size] = '\0';
    return copy_size;
}

StrView_Vec split_str(const char* buffer, size_t length, char delimiter){
    StrView_Vec views = {0};
    size_t start = 0;
    for(size_t i = 0; i < length; i++){
        if(buffer[i] == delimiter){
            StrView s = (StrView){.start=(buffer + start), .length = i - start};
            VEC_Push(views, &s);
            start = i+1;
        }
    }
    if(start < length){
        StrView s = (StrView){.start=(buffer + start), .length = length - start};
        VEC_Push(views, &s);
    }
    return views;
}

#if !defined(__STDC_LIB_EXT1__) && !defined(strcpy_s)
int strcpy_tn(char *_Dst, size_t _SizeInBytes, const char *_Src){
    #if defined(strlcpy)
        return strlcpy(_Dst, _Src, _SizeInBytes);
    #else
        size_t i;
        for(i = 0; i < _SizeInBytes && _Src[i] != '\0'; i++)
            _Dst[i] = _Src[i];
        if(i >= _SizeInBytes)
            return i;
        _Dst[i] = '\0';
        return i+1;
    #endif
}
#endif

typedef struct {
    void* initial_start;
    void* start;
    size_t total_memory;
    size_t block_size;
    void* current;
    bool flexible_memory;
} arena_allocator_context;

static void* n_system_wrapper_alloc(void* allocator, size_t size){ if(allocator != NULL) return NULL; return malloc(size); }
static void* n_system_wrapper_realloc(void* allocator, void* start, size_t size){ if(allocator != NULL) return NULL; return realloc(start, size); }
static bool n_system_wrapper_free(void* allocator, void* start){ free(start); return allocator == NULL; }

void n_system_allocator_init(system_allocator* allocator){
    allocator->context = NULL;
    allocator->alloc = n_system_wrapper_alloc;
    allocator->realloc = n_system_wrapper_realloc;
    allocator->free = n_system_wrapper_free;
}


void n_arena_allocator_init(arena_allocator* allocator, size_t max_memory, bool flexible_memory){
    arena_allocator_context* c = NC_ALLOCATE(sizeof(arena_allocator_context));
    c->initial_start = NC_ALLOCATE(2*sizeof(size_t)+max_memory);
    c->start = (size_t*)c->initial_start+1;
    memset(c->start, 0, max_memory+sizeof(size_t));
    c->total_memory = max_memory;
    *(int*)c->start = (int)max_memory;
    *(size_t*)c->initial_start = (size_t)max_memory;
    c->initial_start = c->start;
    c->current = c->start;
    c->flexible_memory = flexible_memory;
    c->block_size = max_memory;
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

    arena_allocator_context* c = (arena_allocator_context*)allocator;

    void* cur = ((uint32_t*)start - 1);
    uint32_t to_skip = ((*(uint32_t*)cur) ^ (uint32_t)(1 << 31));

    void* after_start = ((char*)cur + to_skip);
    if((((((*(uint32_t*)after_start) & (uint32_t)(1 << 31)) == 0) &&
            (size - to_skip) + 2*sizeof(uint32_t) < (*(uint32_t*)after_start & (uint32_t)(~(1 << 31)))))
        || (size + 2*sizeof(uint32_t) < to_skip)){
        uint32_t free_after = *(uint32_t*)after_start;
        *(uint32_t*)after_start = 0;
        char* after_new = (char*)cur + size + sizeof(uint32_t);
        *(uint32_t*)(after_new) = (to_skip + free_after) - size - sizeof(uint32_t);
        *(uint32_t*)cur = (size + sizeof(uint32_t)) | (1 << 31);
        if((char*)cur < (char*)c->start + c->block_size && (char*)cur >= (char*)c->start)
            c->current = after_new;
        return start;
    }

    uint32_t free_after_old = *(uint32_t*)after_start;

    void* new_start = n_arena_allocator_alloc(allocator, size);
    if(new_start == NULL)
        return NULL;
    
    uint32_t free_after = 0;
    if((((*(uint32_t*)after_start) & (uint32_t)(1 << 31)) == 0)){
        free_after = *(uint32_t*)after_start;
        if(free_after == free_after_old)
            *(uint32_t*)after_start = 0;
        else {
            free_after += 0;
        }
    }
    *(uint32_t*)cur = to_skip + free_after;

    if((char*)cur >= (char*)c->start + c->block_size || (char*)cur < (char*)c->start){
        c->start = c->initial_start;
        c->block_size = *((size_t*)c->start-1);
        char* end_of_block = (char*)c->start + c->block_size;
        while((char*)cur >= end_of_block || (char*)cur < (char*)c->start){
            c->start = ((*(size_t**)end_of_block)+1);
            c->block_size = *((size_t*)c->start-1);
            end_of_block = (char*)c->start + c->block_size;
        }
    }
    c->current = cur;
    if(new_start < (void*)((char*)start + to_skip)) {
        memmove(new_start, start, to_skip - sizeof(uint32_t));
    } else {
        memcpy(new_start, start, to_skip - sizeof(uint32_t));
    }
    return new_start;
}

void* n_arena_allocator_alloc(void* allocator, size_t size){
    if(allocator == NULL)
        return NULL;
    arena_allocator_context* c = (arena_allocator_context*)allocator;
    
    void* cur = c->current;
    void* alloc_end = (char*)c->start + c->block_size; 
    size_t size_to_alloc = size + sizeof(uint32_t);

    while(cur < alloc_end && ((((*(uint32_t*)cur) & (uint32_t)(1 << 31)) > 0) || (((*(uint32_t*)cur) & (uint32_t)(~(1 << 31))) < size_to_alloc + sizeof(uint32_t)))){
        uint32_t to_skip = ((*(uint32_t*)cur) & (uint32_t)(~(1 << 31)));
        assert(to_skip > (sizeof(uint32_t)-1) && "Arena Memory Corruption");
        cur = (char*)cur + to_skip;
    }


    if((char*)alloc_end - (char*)cur < (ptrdiff_t)size_to_alloc){
        if(!c->flexible_memory)
            return NULL; // not enough memory
        size_t* end_of_block = (size_t*)((char*)c->start + c->block_size);
        if(*(uintptr_t*)end_of_block != (uintptr_t)NULL){
            c->start = *(void**)end_of_block;
            c->block_size = *(size_t*)c->start;
            c->start = (size_t*)c->start + 1;
            c->current = c->start;
            return n_arena_allocator_alloc(allocator, size);
        }
        
        size_t old_size = c->total_memory;
        c->total_memory *= 2;
        size_t offset = (char*)cur - (char*)c->start;
        void* new_seg = NC_ALLOCATE(2*sizeof(size_t)+old_size);
        *(uintptr_t*)end_of_block = (uintptr_t)new_seg;
        c->start = ((size_t*)new_seg+1);
        c->current = c->start;
        c->block_size = old_size;
        *((size_t*)c->start-1) = old_size;
        memset((void*)((char*)c->current), 0, old_size+sizeof(size_t));
        *(uint32_t*)c->current = (uint32_t)old_size;
        return n_arena_allocator_alloc(allocator, size);
    }
    uint32_t cur_free = *(uint32_t*)cur;

    if((((char*)cur - (char*)c->start) > 2229100) && (((char*)cur - (char*)c->start) <= 2229250)){
        size_to_alloc += 0;
    }

    *(uint32_t*)cur = ((uint32_t)(1 << 31)) | size_to_alloc;

    uint32_t free_after_alloc = cur_free - size_to_alloc;
    if(free_after_alloc > 0){
        //printf("free after current arena alloc %lu\n", free_after_alloc);
        *(uint32_t*)((char*)cur + size_to_alloc) = free_after_alloc;
    }
    c->current = (char*)cur + size_to_alloc;

    return (void*)((char*)cur + sizeof(uint32_t));
}

bool n_arena_allocator_free(void* allocator, void* start){
    if(allocator == NULL || start != NULL)
        return false;
    arena_allocator_context* c = (arena_allocator_context*)allocator;
    if(c->flexible_memory){
        c->start = c->initial_start;
        c->block_size = *((size_t*)c->start-1);
        char* end_of_block = (char*)c->start + c->block_size;
        char* next_block = *(char**)end_of_block;
        while(next_block != NULL){
            c->start = ((size_t*)next_block+1);
            c->block_size = *((size_t*)next_block);
            end_of_block = (char*)c->start + c->block_size;
            next_block = *(char**)end_of_block;
            free((void*)((size_t*)c->start-1));
        }
    }
    free((void*)((size_t*)c->initial_start-1));
    free(c);
    return true;
}
#endif
#endif