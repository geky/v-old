#ifndef LIGHT_VAR
#define LIGHT_VAR


/* definitions that define variabls */


// type is always found in metadata
#define var_type(v) ((v).meta & 0x7)

// size is only valid for strings and tables
#define var_size(v) ((v).meta >> 3)

// var is struct which contains the data
// for all variables
typedef struct var {
    unsigned int meta;
    void *data;
} var_t;


// variable identifiers
enum {
    NULL_VAR   = 0,
    NUM_VAR    = 1,
    STRING_VAR = 2,
    TABLE_VAR  = 3,
    FUNC_VAR   = 4
};


// definition of hash type
typedef unsigned int hash_t;



/* specific var includes */


#include "stringvar.h"
#include "numvar.h"
#include "tablevar.h"
#include "funcvar.h"



/* function definitions for variable types */


// equals returns 1 if both variables are the
// same type and equivalent
int equals(var_t a, var_t b);


// Hash returns a c long hash value of the
// variable type. The hash value of equivalent
// will be equal.
hash_t hash(var_t var);


// Prints the variable to stdout. Really just for debugging
// Should either be removed or refactored, very hacky as is
void print(var_t var);


#endif
