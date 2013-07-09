#include "fn.h"

#include <stdio.h>
#include <assert.h>

var_t fn_create(char *s) {return null_var;}

var_t fn_call(var_t fn, var_t args) {
    assert(args.type == TYPE_TBL);

    switch (fn.type) {
        case TYPE_BFN: return fn.bfn(args);
        case TYPE_FN: printf("CALLED FN\n");
        default: return null_var;
    }
}

var_t light_fn(var_t *v, int n) {return null_var;}
