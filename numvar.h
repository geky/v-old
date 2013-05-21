#ifndef LIGHT_NUM
#define LIGHT_NUM


// num_t definition
typedef double num_t;

typedef union numvar {
    num_t num;
    var_t var;
} numvar_t;


// returns double representation for internal use
static inline double var_num(var_t var) {
    var.meta &= ~0x7;
    return ((numvar_t)var).num;
}


// tests for num equivalence
static inline int num_equals(var_t a, var_t b) {
    return var_num(a) == var_num(b);
}


// returns hash value of a num
static inline hash_t num_hash(var_t var) {
    // add one to prevent collisions at 0
    // and prevent -0 from being an issue
    numvar_t val = (numvar_t)var;
    val.num += 1.0;

    // take int value as base to keep it
    // linear for integers
    hash_t hash = (unsigned int)val.num;

    // move the decimal part around to fit
    // into an int value to add to the hash
    val.num -= hash;
    val.num += 0x100000;

    return hash ^ val.var.meta;
}

#endif
