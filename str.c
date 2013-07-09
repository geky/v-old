#include "str.h"
#include "tbl.h"
#include "fn.h"


#include <string.h>


var_t str_create(const char *string, uint16_t len) {
    var_t val;

    val.ref = var_alloc(sizeof(ref_t) + len);
    *val.ref = 1;

    memcpy(val.ref+1, string, len);

    val.type = TYPE_STR;
    val.off = 0;
    val.len = len;

    return val;
}


var_t str_concat(var_t a, var_t b) {
    var_t val;

    uint16_t len = a.len + b.len;
    val.ref = var_alloc(sizeof(ref_t) + len);
    *val.ref = 1;

    char *str = (char*)(val.ref+1);
    memcpy(str, var_str(a), a.len);
    memcpy(str+a.len, var_str(b), b.len);

    val.type = TYPE_STR;
    val.off = 0;
    val.len = len;

    return val;
}

var_t str_substr(var_t a, int start, int end) {
    var_t val;

    val.len = end - start;
    val.off = a.off + start;
    val.meta = a.meta;

    var_inc_ref(val);
    return val;
}
    
/*
var_t light_str(var_t *v, int n) {
    if (n < 1)
        return str_var("");

    switch (v->type) {
        case TYPE_STR:
            return *v;

        case TYPE_TBL: {
            var_t to_str = tbl_lookup(var_tbl(*v), str_var("to_str"));
            var_t temp;

            switch (to_str.type) {
                case TYPE_FN:
                case TYPE_BFN:
                    temp = fn_call(to_str, 0, 0);
                    return light_str(&temp, 1);
            }
        }

        default:
            return light_repr(v, n);
    }
}
*/
/*
var_t str_parse(uint16_t *offp, char *str, uint16_t len) {
    char quote = str[(*offp)++];
    uint16_t off = *offp;
    uint16_t end = off+len;

    while (str[off++] != quote) {
        if (off >= end)
            // TODO return error
            return null_var;
    }
    
    var_t val;
    val.v.meta = TYPE_STR | (uint32_t)((ref_t*)str - 1);
    val.str.off = *offp;
    val.str.len = off - 2 - val.str.off;
    *offp = off;

    return val;
}
*/
