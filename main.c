#include "var.h"
#include "parse.h"
#include "str.h"
#include "tbl.h"

#include "vdbg.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main() {
    d_set_stack();

    var_t res, result;
    res = veval_cstr("2+5");

    result = light_repr(&res, 1);
    printf("Result: %.*s\n", result.len, var_str(result));

    d_print_space();

    return 0;
}
