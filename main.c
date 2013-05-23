#include "var.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main() {
    var_t test = nan_var;

    printf("value: ");
    var_print(test);
    printf("\nhash: 0x%x\n", var_hash(test));
    printf("size: %d\n", sizeof test);
    printf("loc: %p\n", &test);
    printf("ref: %p\n", &var_ref(test));

    var_dec_ref(test);

    return 0;
}
