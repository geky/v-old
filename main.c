#include "var.h"
#include "str.h"
#include "table.h"

#include "ldebug.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main() {
    var_t test, testb;
    var_t result;
    int i;


    test = table_create(0);
    testb = table_create(0);


//    table_set(var_table(test), str_var("a"), str_var("true"));
//    table_set(var_table(test), num_var(0), str_var("true"));
//    table_set(var_table(test), str_var("2"), str_var("c"));
//    table_set(var_table(test), str_var("1"), null_var);

    for (i=0; i<10000000; i++) {
        table_assign(var_table(test), num_var(i), str_var("a"));
    }



    printf("\n");
    //d_print_var(test);
    printf("\n");

//    result = light_repr(&test, 1);

//    printf("Result: %.*s\n\n", var_len(result), var_str(result));

    var_dec_ref(test);
    var_dec_ref(testb);
//    var_dec_ref(result);

    d_print_space();

    return 0;
}
