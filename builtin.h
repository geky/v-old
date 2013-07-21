#ifndef V_BUILTIN
#define V_BUILTIN

#include "var.h"
#include "parse.h"

#include "tbl.h"

// Syntatical contructs
#define v_dot    null_flag(DOT_OP)
#define v_assign null_flag(ASSIGN_OP)
#define v_set    null_flag(SET_OP)
#define v_and    null_flag(AND_OP)
#define v_or     null_flag(OR_OP)

// Statement implementations
var_t v_if(var_t args);
var_t v_while(var_t args);
var_t v_for(var_t args);

// Operator implementations
var_t v_add(var_t args);
var_t v_mul(var_t args);
var_t v_gt(var_t args);

// Adds all builtins to a given table
void add_builtins(tbl_t *tbl);

#endif
