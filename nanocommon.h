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
#define VEC_Free_Al(vec, allocator) if(vec.start != NULL) allocator->free(allocator->context, vec.start)

#define VEC_Init_Reserve(vec, cap) do { vec.start = NC_ALLOCATE(cap*sizeof(*vec.start)); vec.capacity=cap; } while(0); 
#define VEC_Init_Reserve_Al(vec, cap, allocator) do { vec.start = allocator->alloc(allocator->context, cap*sizeof(*vec.start)); vec.capacity=cap; } while(0); 

#define copystrn(dest, src, length) snprintf(dest, length, "%s", src)

typedef struct StrView {
    const char* start;
    size_t length;
} StrView;

VEC(StrView);

StrView_Vec split_str(const char* buffer, size_t length, char delimiter);
void replace_in_str(char* dest, const char* source, const char* from, const char* to, size_t max_buffer_len, int occurences);
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

#ifndef _Noreturn // TEMPORARY
#define _Noreturn __declspec(noreturn)
#endif
#include <threads.h>

#ifdef NC_ATOMIC_EXTENSION
#ifdef _WIN32
#include <winnt.h>
#endif

void atomic_store_64(volatile uint64_t* target, uint64_t value){
    #ifdef _WIN32
        _ReadWriteBarrier();
        InterlockedExchange64(target, value);
    #elif defined(__GNUC__)
        __sync_synchronize();
        *target = value;
    #endif
}

uint64_t atomic_load_64(volatile uint64_t* target){
    #ifdef _WIN32
        _ReadWriteBarrier();
    #elif defined(__GNUC__)
        __sync_synchronize();
    #endif
    return *target;
} 

bool compare_and_swap_64(volatile uint64_t* target, uint64_t comparand, uint64_t value){
    #ifdef _WIN32
        return InterlockedCompareExchange64(target, value, comparand) == comparand;
    #elif defined(__GNUC__)
        return __atomic_compare_exchange_n(target, &comparand, value, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    #endif
    return false;
}


#endif

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

void replace_in_str(char* dest, const char* src, const char* from, const char* to, size_t max_buffer_len, int occurences){
    size_t in_from = 0;
    size_t in_dest = 0;
    size_t last_copy_position = 0;
    size_t from_l = strnlen(from, max_buffer_len);
    size_t to_l = strnlen(to, max_buffer_len);
    size_t dest_l = strnlen(dest, max_buffer_len);
    int remaining = occurences;
    for(size_t i = 0; src != NULL && src[i] != '\0' && i < max_buffer_len && remaining != 0; i++){
        if(src[i] != from[in_from++]){
            in_from = 0;
        }
        if(in_from == from_l){
            in_dest += strcpy_tn(dest + in_dest, i-from_l-last_copy_position, src + last_copy_position);
            in_dest += strcpy_tn(dest + in_dest, to_l, to);
            
            last_copy_position = i + 1;
            remaining--;
        }
    }
    size_t src_l = strnlen(src, max_buffer_len);
    if(last_copy_position < src_l){
        strcpy_tn(dest + in_dest, src_l - last_copy_position + 1, src + last_copy_position);
    } else {
        dest[in_dest] = '\0';
    }
}

#if !defined(__STDC_LIB_EXT1__) && !defined(strcpy_s)
int strcpy_tn(char *_Dst, size_t _SizeInBytes, const char *_Src){
    #if defined(strlcpy)
        return strlcpy(_Dst, _Src, _SizeInBytes);
    #else
        size_t i;
        for(i = 0; i < _SizeInBytes && _Src[i] != '\0'; i++)
            _Dst[i] = _Src[i];
        if(i > _SizeInBytes){
            _Dst[i-1] = '\0';
            return i;
        }
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

    volatile uint64_t block_allocation_lock;
    mtx_t* mtx;
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
    c->mtx = NC_ALLOCATE(sizeof(mtx_t));
    mtx_init(c->mtx, mtx_plain);
    
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

void* n_arena_allocator_realloc(void* allocator, void* start, size_t size){ // proceed only if not locked
    if(allocator == NULL)
        return NULL;

    if (start == NULL){
        return n_arena_allocator_alloc(allocator, size);
    }

    arena_allocator_context* c = (arena_allocator_context*)allocator;

    mtx_lock(c->mtx);

    void* cur = ((uint32_t*)start - 1);
    uint32_t to_skip = ((*(uint32_t*)cur) ^ (uint32_t)(1 << 31));

    void* after_start = ((char*)cur + to_skip);
    if((((((*(uint32_t*)after_start) & (uint32_t)(1 << 31)) == 0) &&
            (size - to_skip) + 2*sizeof(uint32_t) < (*(uint32_t*)after_start & (uint32_t)(~(1 << 31)))))
        || (size + 2*sizeof(uint32_t) < to_skip)){
        uint32_t free_after = *(uint32_t*)after_start;
        if(to_skip >= size + sizeof(uint32_t)){
            to_skip += 0;
        }

        *(uint32_t*)after_start = 0; // cas?
        char* after_new = (char*)cur + size + sizeof(uint32_t);
        *(uint32_t*)(after_new) = (to_skip + free_after) - size - sizeof(uint32_t); // cas?
        *(uint32_t*)cur = (size + sizeof(uint32_t)) | (1 << 31); // cas?
        if((char*)cur < (char*)c->start + c->block_size && (char*)cur >= (char*)c->start)
            c->current = after_new; // cas?

        mtx_unlock(c->mtx);
        
        return start;
    }

    uint32_t free_after_old = *(uint32_t*)after_start;

    mtx_unlock(c->mtx);

    void* new_start = n_arena_allocator_alloc(allocator, size);
    if(new_start == NULL)
        return NULL;
    
    mtx_lock(c->mtx);

    uint32_t free_after = 0;
    if((((*(uint32_t*)after_start) & (uint32_t)(1 << 31)) == 0)){
        free_after = *(uint32_t*)after_start;
        if(free_after == free_after_old)
            *(uint32_t*)after_start = 0; // cas?
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
    c->current = cur; // cas?
    if(new_start < (void*)((char*)start + to_skip)) {
        memmove(new_start, start, to_skip - sizeof(uint32_t));
    } else {
        memcpy(new_start, start, to_skip - sizeof(uint32_t));
    }

    mtx_unlock(c->mtx);

    return new_start;
}

void* n_arena_allocator_alloc(void* allocator, size_t size){ // proceed only if not locked
    if(allocator == NULL)
        return NULL;
    arena_allocator_context* c = (arena_allocator_context*)allocator;
    
    mtx_lock(c->mtx);

    void* cur = c->current;
    void* alloc_end = (char*)c->start + c->block_size; 
    size_t size_to_alloc = size + sizeof(uint32_t);

    while(cur < alloc_end && ((((*(uint32_t*)cur) & (uint32_t)(1 << 31)) > 0) || (((*(uint32_t*)cur) & (uint32_t)(~(1 << 31))) < size_to_alloc + sizeof(uint32_t)))){
        uint32_t to_skip = ((*(uint32_t*)cur) & (uint32_t)(~(1 << 31)));
        assert(to_skip > (sizeof(uint32_t)-1) && "Arena Memory Corruption");
        cur = (char*)cur + to_skip;
    }


    if((char*)alloc_end - (char*)cur < (ptrdiff_t)size_to_alloc){ // locked
        if(!c->flexible_memory)
            return NULL; // not enough memory
        size_t* end_of_block = (size_t*)((char*)c->start + c->block_size);
        if(*(uintptr_t*)end_of_block != (uintptr_t)NULL){
            c->start = *(void**)end_of_block;
            c->block_size = *(size_t*)c->start;
            c->start = (size_t*)c->start + 1;
            c->current = c->start;

            mtx_unlock(c->mtx);

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

        mtx_unlock(c->mtx);

        return n_arena_allocator_alloc(allocator, size);
    }
    uint32_t cur_free = *(uint32_t*)cur;

    if((((char*)cur - (char*)c->start) > 2229100) && (((char*)cur - (char*)c->start) <= 2229250)){
        size_to_alloc += 0;
    }

    *(uint32_t*)cur = ((uint32_t)(1 << 31)) | size_to_alloc; // cas linked to two below

    uint32_t free_after_alloc = cur_free - size_to_alloc;
    if(free_after_alloc > 0){
        //printf("free after current arena alloc %lu\n", free_after_alloc);
        *(uint32_t*)((char*)cur + size_to_alloc) = free_after_alloc; // cas
    }
    c->current = (char*)cur + size_to_alloc; // cas

    mtx_unlock(c->mtx);

    return (void*)((char*)cur + sizeof(uint32_t));
}

bool n_arena_allocator_free(void* allocator, void* start){ // locked
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