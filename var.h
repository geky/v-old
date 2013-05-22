#ifndef LIGHT_VAR
#define LIGHT_VAR

#include <stdint.h>


/* definitions of variables */


// type definitions

typedef double num_t;
typedef char *str_t;
typedef struct table *table_t;
typedef void *func_t;

// definition of hash type

typedef uint32_t hash_t;


// variable identifiers

enum {
    NULL_VAR  = 0,
    NUM_VAR   = 1,
    STR_VAR   = 2,
    TABLE_VAR = 3,
    FUNC_VAR  = 4
};


// var_t is struct which contains the data
// for all variable types

typedef union {
    uint64_t bits;
    num_t num;

    struct var {
        uint32_t meta;
        void *data;
    } ptr;
} var_t;



/* useful macros */

#define var_meta(v)  ((v).ptr.meta)

#define var_data(v)  ((v).ptr.data)

#define var_type(v)  (var_meta(v) & 0x7)

#define var_size(v)  (var_meta(v) >> 3)

#define var_num(v)   (((var_t)((v).bits & ~0x7)).num)

#define var_str(v)   ((str_t)var_data(v))

#define var_table(v) ((table_t)var_data(v))

#define var_func(v)  var_data(v)



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


#endif
