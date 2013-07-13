#ifndef V_NUM
#define V_NUM

#include "var.h"

// function for creating a num var from c doubles
var_t num_create(double);

// builtin for creating numbers
var_t light_num(var_t *, int);
/*
// parses a number literal
var_t num_parse(uint16_t *off, char *str, uint16_t len);
*/

#endif
