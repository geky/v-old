#ifndef LIGHT_STR
#define LIGHT_STR

#include "var.h"


// functions for creating a str var from c strings
var_t str_create(const char *, uint16_t len);

// function for str concatenation
var_t str_concat(var_t a, var_t b);

// builtin for creating strings
var_t light_str(var_t *, int);


#endif