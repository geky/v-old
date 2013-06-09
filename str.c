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
    val.str.off = 0;
    val.str.len = len;

    return val;
}


var_t str_concat(var_t a, var_t b) {
    var_t val;

    uint16_t len = a.str.len + b.str.len;
    val.ref = var_alloc(sizeof(ref_t) + len);
    *val.ref = 1;

    char *str = (char*)(val.ref+1);
    memcpy(str, var_str(a), a.str.len);
    memcpy(str+a.str.len, var_str(b), b.str.len);

    val.type = TYPE_STR;
    val.str.off = 0;
    val.str.len = len;

    return val;
}

var_t str_substr(var_t a, int start, int end) {
    var_t val;

    val.str.len = end-start;
    val.str.off = a.str.off + start;
    val.v.meta = a.v.meta;

    var_inc_ref(val);
    return val;
}
    

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
