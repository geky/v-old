#ifndef V_BUILTIN
#define V_BUILTIN

#include "var.h"

// Statement implementations
var_t v_if(var_t args);
var_t v_while(var_t args);
var_t v_for(var_t args);

// Operator implementations
var_t v_add(var_t args);
var_t v_mul(var_t args);

#endif
