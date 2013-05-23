#ifndef LIGHT_VAR
#define LIGHT_VAR

#include <stdint.h>
#include <stdlib.h>


/* definitions of variables */


// type definitions

typedef double num_t;
typedef char *str_t;
typedef struct table *table_t;
typedef void *func_t;

// definition of useful types

typedef uint32_t hash_t;
typedef unsigned int ref_t;


// variable identifiers

enum {
    NULL_VAR  = 0,
    NUM_VAR   = 1,
    STR_VAR   = 4,
    TABLE_VAR = 5,
    FUNC_VAR  = 6,
    BIF_VAR   = 7
};


// var_t is struct which contains the data
// for all variable types

typedef union {
    uint64_t bits;
    num_t num;

    struct var {
        union {
            uint32_t bits;
            ref_t *ref;
        } meta;

        union {
            void *ptr;
            table_t table;

            struct {
                uint16_t offset;
                uint16_t length;
            } str;
        } data;
    } var;
} var_t;



/* useful macros */


// for retrieving components
#define var_meta(v) ((v).var.meta.bits)

#define var_data(v) ((v).var.data.ptr)

#define var_type(v) ((v).var.meta.bits & 0x7)


#define var_num(v)   (((var_t)((v).bits & ~0x7)).num)

#define var_str(v)   (((str_t)(((var_t)((v).bits & ~0x7)).var.meta.ref+1)) + \
                      ((v).var.data.str.offset))

#define var_table(v) ((table_t)(v).var.data.ptr)

#define var_func(v)  ((v).var.data.ptr)


#define var_ref(v) (*((var_t)((v).bits & ~0x7)).var.meta.ref)

#define var_off(v) ((v).var.data.str.offset)

#define var_len(v) ((v).var.data.str.length)


// definition of some constants
#define null_var ((var_t){0})

#define nan_var  ((var_t){0x7ffffffffffffff8 | NUM_VAR})

#define inf_var  ((var_t){0x7ff0000000000000 | NUM_VAR})



/* function definitions for variable types */

// equals returns 1 if both variables are the
// same type and equivalent
int var_equals(var_t a, var_t b);


// Hash returns a c long hash value of the
// variable type. The hash value of equivalent
// will be equal.
hash_t var_hash(var_t var);


// Prints the variable to stdout. Really just for debugging
// Should either be removed or refactored, very hacky as is
void var_print(var_t var);



// Increments the internal reference counter on types 
// that are garbage collected
void var_inc_ref(var_t var);


// Decrements the internal reference counter, may
// also perform deallocation if references reach zero
void var_dec_ref(var_t var);


// Internal wrapper for malloc that handles things like
// failures and alignment
void *var_alloc(size_t size);


// Internal wrapper for free in case it needs to be
// extended in the future
void var_free(void *ptr);


#endif
