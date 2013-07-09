#include "num.h"


var_t num_create(double val) {
    var_t num;

    num.num = val;
    num.type = TYPE_NUM;

    return num;
}


var_t light_num(var_t *v, int n) {
    return null_var;
    // TODO
}


/*
var_t num_parse(uint16_t *offp, char *str, uint16_t len) {
    unsigned char i;
    uint16_t off = *offp;
    double acc = 0;

    struct {
        double radix;
        double exp;
        char exp_c;
        char exp_C;
    } base = { 10.0, 10.0, 'e', 'E' };    

    if (str[off] == 0) {
        off++; // discard inital zero

        switch (str[off]) {
            case 'B': case 'b':
                base = {  2.0, 2.0, 'p', 'P' };
                off++;
                break;

            case 'O': case 'o':
                base = {  8.0, 2.0, 'p', 'P' };
                off++;
                break;

            case 'X': case 'x':
                base = { 16.0, 2.0, 'p', 'P' };
                off++;
                break;
        }
    }


    while (1) {
        i = str[off++];

        if (i == base.exp_c || i == base.exp_C)
            goto exp;

        i -= (i >= 'A') ? ('A' + 10) :
             (i >= 'a') ? ('a' + 10) : '0';

        if (i > base.radix) { 
            if (i == '.') {
                break;
            } else {
                *offp = off-1;
                return num_create(acc);
            }
        }

        acc *= base.radix;
        acc += i;
    }

    off++; // hit a period
    double scale = 1.0;

    while (1) {
        i = str[off++];

        if (i == base.exp_c || i == base.exp_C)
            goto exp;

        i -= (i >= 'A') ? ('A' + 10) :
             (i >= 'a') ? ('a' + 10) : '0';

        if (i > base.radix) {
            *offp = off-1;
            return num_create(acc);
        }

        scale /= base.radix;
        acc += scale * i;
    }

exp:
    off++; // hit an exponent symbol
    double sign = 1.0;
    double expt = 0.0;

    if (str[off] == '+') {
        off++;
    } else if (str[off] == '-') {
        sign = -1.0;
        off++;
    }

    while (1) {
        i = str[off++];

        i -= (i >= 'A') ? ('A' + 10) :
             (i >= 'a') ? ('a' + 10) : '0'

        if (i > base.radix) {
            acc *= pow(base.radix, expt)
            *offp = off-1;
            return num_create(acc);
        }

        expt *= base.radix;
        expt += i;
    }
}
*/  

