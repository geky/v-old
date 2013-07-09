#ifndef LIGHT_STR
#define LIGHT_STR

#include "var.h"


// functions for creating a str var from c strings
var_t str_create(const char *, uint16_t len);

// function for str concatenation
var_t str_concat(var_t a, var_t b);

// function for str substring
var_t str_substr(var_t a, int start, int end);

// builtin for creating strings
var_t light_str(var_t *, int);
/*
// parses a string literal
var_t str_parse(uint16_t *off, char *str, uint16_t len);
*/

#endif
