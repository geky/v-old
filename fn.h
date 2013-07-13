#ifndef V_FN
#define V_FN

#include "var.h"

typedef struct fn {
    var_t args;    // tbl
    var_t closure; // tbl
    var_t code;    // str
} fn_t;


// function for creating function values
var_t fn_create(var_t code, var_t clos);

// function for calling functions 
var_t fn_call(var_t fn, var_t args);

// builtin for creating functions
var_t light_fn(var_t *, int);


#endif
