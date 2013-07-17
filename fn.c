#include "fn.h"

#include "tbl.h"
#include "parse.h"

#include <stdio.h>
#include <assert.h>

var_t fn_create(var_t code, var_t clos) {
    var_t var;
    fn_t *fn = var_alloc(sizeof(fn_t));

    fn->ref = 1;
    var.ref = &fn->ref;
    var.type = TYPE_FN;
    var.fn = fn;

    fn->code = code;
    var_inc_ref(code);
    fn->closure = clos;
    var_inc_ref(clos);
    
    fn->args = tbl_create(1);
    tbl_assign(fn->args.tbl, str_var("this"), num_var(1));

    return var;
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

            printf("calling ");
            var_print(fn->code);
            printf("\n");

            return vparse(fn->code, scope);
        }
    }

    return null_var;
}

var_t light_fn(var_t *v, int n) {return null_var;}
