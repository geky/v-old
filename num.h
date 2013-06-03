#ifndef LIGHT_NUM
#define LIGHT_NUM

#include "var.h"
// num_t declared as double


// function for creating a num var from c doubles
var_t num_create(double);

// builtin for creating numbers
var_t light_num(var_t *, int);

#endif
