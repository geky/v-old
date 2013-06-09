#include "num.h"


var_t num_create(double val) {
    var_t num;

    num.num = val;
    num.type = TYPE_NUM;

    return num;
}



