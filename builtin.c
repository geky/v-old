
#include "builtin.h"
#include "parse.h"

#include "tbl.h"
#include "fn.h"

#include <stdio.h>


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
    var_t first = tbl_lookup(args.tbl, num_var(0));
    var_t code = tbl_lookup(args.tbl, num_var(1));

    if (pred.type != TYPE_FN && pred.type != TYPE_BFN)
        return null_var;

    if (code.type != TYPE_FN && code.type != TYPE_BFN) {
        var_t scope = tbl_create(3);
        tbl_assign(scope.tbl, str_var("f"), first);
        tbl_assign(scope.tbl, str_var("p"), pred);
        tbl_assign(scope.tbl, str_var("w"), fn_var(v_while));
        tbl_assign(scope.tbl, str_var(":"), v_assign);

        var_t res = fn_create(str_var("w(f,b,parameters:p)\0"), scope);
        tbl_assign(res.fn->args.tbl, str_var("block"), str_var("b"));

        return res;
    }

    var_t res = tbl_create(0);
    double n = 0.0;

    if (first.type == TYPE_NULL)
        return res;

    while (1) {
        tbl_assign(res.tbl, num_var(n), fn_call(code, null_var));
        n++;

        var_t c = fn_call(pred, null_var);
        if (c.type == TYPE_NULL)
            break;
    }

    return res;
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

var_t v_gt(var_t args) {
    var_t a = tbl_lookup(args.tbl, num_var(0));
    var_t b = tbl_lookup(args.tbl, num_var(1));

    return var_num(a) > var_num(b) ? num_var(1) : null_var;
}


void add_builtins(tbl_t *tbl) {
    tbl_assign(tbl, str_var("."), v_dot);
    tbl_assign(tbl, str_var(":"), v_assign);
    tbl_assign(tbl, str_var("="), v_set);
    tbl_assign(tbl, str_var("&&"), v_and);
    tbl_assign(tbl, str_var("||"), v_or);

    tbl_assign(tbl, str_var("if"), fn_var(v_if));
    tbl_assign(tbl, str_var("while"), fn_var(v_while));
    tbl_assign(tbl, str_var("for"), fn_var(v_for));

    tbl_assign(tbl, str_var("+"), fn_var(v_add));
    tbl_assign(tbl, str_var("*"), fn_var(v_mul));
    tbl_assign(tbl, str_var(">"), fn_var(v_gt));
}
