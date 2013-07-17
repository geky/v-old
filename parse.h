#ifndef V_PARSE
#define V_PARSE

#include "var.h"

var_t parse_single(var_t input);

// parses vcode and returns the result
var_t vparse(var_t code, var_t scope);


// builtin function used both internally and externally 
// to split code on keywords
var_t v_fn_split(var_t args);

#endif
