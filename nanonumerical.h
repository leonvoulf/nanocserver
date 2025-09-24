#include <stdint.h>
#include "nanocommon.h"

typedef struct numerical_t {
    A_VEC(size_t) words;
    size_t mantissa;
} numerical_t;

void numerical_init(numerical_t* output){
    
}

void numerical_add(numerical_t* first, numerical_t* second, numerical_t* output){
    size_t min = first->words.count < second->words.count ? first->words.count : second->words.count;
    size_t max = first->words.count == min ? second->words.count : first->words.count;
    bool prev_carry = false;
    bool carry = false;
    size_t last_bit = 1 << (sizeof(last_bit)*8-1);
    for(size_t i = 0; i < min; i++){
        carry = (first->words.start[i] & last_bit) && (second->words.start[i] & last_bit);
        output->words.start[i] = first->words.start[i] + second->words.start[i] + (size_t)prev_carry;
        prev_carry = carry;
    }

}

void numerical_sub(numerical_t* first, numerical_t* second, numerical_t* output){

}