
#include "builtin.h"
#include "parse.h"

#include "tbl.h"
#include "fn.h"


static var_t v_if_true(var_t args) {
    var_t doif = tbl_lookup(args.tbl, str_var("block"));

    if (doif.type != TYPE_FN)
        return null_var;

    tbl_assign(args.tbl, num_var(0), doif);
    tbl_assign(args.tbl, num_var(1), str_var("else"));

    var_t res = tbl_lookup(v_fn_split(args).tbl, num_var(0));

    if (res.type == TYPE_FN)
        return fn_call(res, tbl_create(0));

    return null_var;
}

static var_t v_if_false(var_t args) {
    var_t doif = tbl_lookup(args.tbl, str_var("block"));

    if (doif.type != TYPE_FN)
        return null_var;

    tbl_assign(args.tbl, num_var(0), doif);
    tbl_assign(args.tbl, num_var(1), str_var("else"));

    var_t res = tbl_lookup(v_fn_split(args).tbl, num_var(1));

    if (res.type == TYPE_FN)
        return fn_call(res, tbl_create(0));

    return null_var;
}

var_t v_if(var_t args) {
    var_t a = tbl_lookup(args.tbl, num_var(0));

    if (a.type != TYPE_NULL)
        return fn_var(v_if_true);
    else
        return fn_var(v_if_false);
}



var_t v_add(var_t args) {
    var_t a = tbl_lookup(args.tbl, num_var(0));
    var_t b = tbl_lookup(args.tbl, num_var(1));

    return num_var(var_num(a) + var_num(b));
}

var_t v_mul(var_t args) {
    var_t a = tbl_lookup(args.tbl, num_var(0));
    var_t b = tbl_lookup(args.tbl, num_var(1));

    return num_var(var_num(a) * var_num(b));
}
