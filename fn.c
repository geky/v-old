#include "fn.h"

#include "tbl.h"
#include "parse.h"

#include <stdio.h>
#include <assert.h>

var_t fn_create(var_t code, var_t clos) {
    return null_var;
}

var_t fn_call(var_t vfn, var_t args) {
    assert(args.type == TYPE_TBL);

    switch (vfn.type) {
        case TYPE_BFN: 
            return vfn.bfn(args);

        case TYPE_FN: {
            fn_t *fn = vfn.fn;

            // minimum scope is [args, this, super]
            var_t scope = tbl_create(3); 
            tbl_assign(scope.tbl, str_var("args"), args);
            tbl_assign(scope.tbl, str_var("super"), fn->closure);

            for_tbl(key, val, args.tbl, {
                var_t temp = tbl_lookup(fn->args.tbl, key);

                if (temp.meta)
                    tbl_assign(scope.tbl, temp, val);
            });

            return vparse(fn->code, scope);
        }
    }

    return null_var;
}

var_t light_fn(var_t *v, int n) {return null_var;}
