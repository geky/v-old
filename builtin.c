
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

var_t v_while(var_t args) {
    var_t pred = tbl_lookup(args.tbl, str_var("parameters"));
    var_t code = tbl_lookup(args.tbl, str_var("c"));

    if (pred.type != TYPE_FN && pred.type != TYPE_BFN)
        return null_var;

    if (code.type != TYPE_FN && code.type != TYPE_BFN) {
        var_t scope = tbl_create(3);
        tbl_assign(scope.tbl, str_var("p"), pred);
        tbl_assign(scope.tbl, str_var("w"), fn_var(v_while));
        tbl_assign(scope.tbl, str_var(":"), v_assign);

        var_t res = fn_create(str_var("w(parameters:p,c:b)\0"), scope);
        tbl_assign(res.fn->args.tbl, str_var("block"), str_var("b"));

        return res;
    }

    while (1) {
        var_t res = fn_call(pred, null_var);

        var_print(res);

        if (res.type == TYPE_NULL)
            break;

        fn_call(code, null_var);
        break;
    }

    return null_var;
}

var_t v_for(var_t args) {
    var_t p = tbl_lookup(args.tbl, str_var("parameters"));
    tbl_assign(args.tbl, num_var(0), p);

    printf("!");
    var_print(v_fn_token(args));
    printf("!\n");
    return null_var;
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
