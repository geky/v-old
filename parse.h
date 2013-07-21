#ifndef V_PARSE
#define V_PARSE

#include "var.h"

// parses vcode and returns the result
var_t vparse(var_t code, var_t scope);

var_t veval_str(var_t input);
var_t veval_cstr(const char *input);

// Syntatical constructs
enum {
    DOT_OP    = 0x11 << 3,
    ASSIGN_OP = 0x12 << 3,
    SET_OP    = 0x13 << 3,
    AND_OP    = 0x14 << 3,
    OR_OP     = 0x15 << 3,
};

/** builtin function used both internally and externally **/

// splits code on keywords
var_t v_fn_split(var_t args);
// grabs the next token string
var_t v_fn_token(var_t args);

#endif
