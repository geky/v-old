#include "str.h"
#include "table.h"
#include "func.h"


#include <string.h>


var_t str_create(const char *string, uint16_t len) {
    var_t val;

    char *str = var_alloc(sizeof(ref_t) + len);

    memcpy(str+4, string, len);

    var_meta(val) = TYPE_STR | (uint32_t)str;
    var_off(val) = 0;
    var_len(val) = len;

    return val;
}


var_t str_concat(var_t a, var_t b) {
    var_t val;

    uint16_t len = var_len(a) + var_len(b);
    char *str = var_alloc(sizeof(ref_t) + len);

    memcpy(str+4, var_str(a), var_len(a));
    memcpy(str+4+var_len(a), var_str(b), var_len(b));

    var_meta(val) = TYPE_STR | (uint32_t)str;
    var_off(val) = 0;
    var_len(val) = len;

    return val;
}


var_t light_str(var_t *v, int n) {
    if (n < 1)
        return str_var("");

    switch (var_type(*v)) {
        case TYPE_STR:
            return *v;

        case TYPE_TABLE: {
            var_t to_str = table_lookup(var_table(*v), str_var("to_str"));
            var_t temp;

            switch (var_type(to_str)) {
                case TYPE_FUNC:
                case TYPE_BIN:
                    temp = func_call(to_str, 0, 0);
                    return light_str(&temp, 1);
            }
        }

        default:
            return light_repr(v, n);
    }
}
