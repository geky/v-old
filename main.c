#include "var.h"
#include <stdio.h>


int main() {
    numvar_t test;
    test.num = 2.5;
    test.var.meta &= ~0x7;
    test.var.meta |= NUM_VAR;
    
    printf("value: ");
    print(test.var);
    printf("\nhash: 0x%x\n", hash(test.var));
    printf("size: %d\n", sizeof test);

    return 0;
}
