#include "var.h"
#include <stdio.h>


int main() {
    union {double d; var_t v;} test;
    test.d = 2.5;
    var_meta(test.v) &= ~0x7;
    var_meta(test.v) |= NUM_VAR;
    
    printf("value: ");
    var_print(test.v);
    printf("\nhash: 0x%x\n", var_hash(test.v));
    printf("size: %d\n", sizeof test);

    return 0;
}
