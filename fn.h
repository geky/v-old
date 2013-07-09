#ifndef LIGHT_FN
#define LIGHT_FN

#include "var.h"

typedef struct fn {
    var_t *args;   // str
    int nargs;

    var_t closure; // tbl
    var_t code;    // str
} fn_t;


// function for creating function values
var_t fn_create(char *);

// function for calling functions 
var_t fn_call(var_t fn, var_t args);

// builtin for creating functions
var_t light_fn(var_t *, int);


#endif
