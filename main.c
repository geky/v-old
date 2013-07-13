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

    var_t code = str_var("{a = [0,1,2], a.[2] += 4, a}\0");
    var_t res, result;
    res = parse_single(code);

    result = light_repr(&res, 1);
    printf("Result: %.*s\n\n", result.len, var_str(result));

    d_print_space();

    return 0;
}
