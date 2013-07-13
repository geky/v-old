#ifndef LIGHT_PARSE
#define LIGHT_PARSE

#include "var.h"

var_t parse_single(var_t input);

// parses vcode and returns the result
var_t vparse(var_t code, var_t scope);

#endif
