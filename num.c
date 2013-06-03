#include "num.h"


var_t num_create(double val) {
    var_t num;

    num.num = val;
    var_meta(num) &= ~0x7;
    var_meta(num) |= TYPE_NUM;

    return num;
}



