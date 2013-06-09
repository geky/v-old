#include "var.h"
#include "str.h"
#include "tbl.h"

#include "ldebug.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main() {
    var_t test, testb;
    var_t result;
    int i;


//    test = tbl_create(0);
//    testb = tbl_create(0);


//    tbl_set(var_tbl(test), str_var("a"), str_var("true"));
//    tbl_set(var_tbl(test), num_var(0), str_var("true"));
//    tbl_set(var_tbl(test), str_var("2"), str_var("c"));
//    tbl_set(var_tbl(test), str_var("1"), null_var);

    testb = str_var("Hello World!");
    test = light_tbl(&testb, 1);

    var_dec_ref(testb);


    printf("\n");
    //d_print_var(test);
    printf("\n");

    result = light_repr(&test, 1);

    printf("Result: %.*s\n\n", result.str.len, var_str(result));

    var_dec_ref(test);
//    var_dec_ref(testb);
    var_dec_ref(result);

    d_print_space();

    return 0;
}
