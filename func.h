#ifndef LIGHT_FUNC
#define LIGHT_FUNC

#include "var.h"

typedef var_t *(*bin_t)(var_t *, int);

typedef struct func {
    var_t scope;
    var_t code;
} func_t;


// function for creating function values
var_t func_create(char *);

// function for calling functions 
var_t func_call(var_t, var_t *args, int nargs);

// builtin for creating functions
var_t light_func(var_t *, int);


#endif
