#ifndef LIGHT_FN
#define LIGHT_FN

#include "var.h"

typedef var_t *(*bin_t)(var_t *, int);

typedef struct fn {
    var_t scope;
    var_t code;
} fn_t;


// function for creating function values
var_t fn_create(char *);

// function for calling functions 
var_t fn_call(var_t, var_t *args, int nargs);

// builtin for creating functions
var_t light_fn(var_t *, int);


#endif
