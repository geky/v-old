#ifndef LIGHT_STRING
#define LIGHT_STRING

#include <string.h>

// string_t definition
typedef char *string_t;


// extracts char *from var_t
#define var_string(v) ((char*)(v).data)


// tests for equivalence of strings
static inline int string_equals(var_t a, var_t b) {
    return !memcmp(a.data, b.data, a.meta >> 3);
}


// returns a hash value of the string
// based off the djb2 algorithm
static inline hash_t string_hash(var_t var) {
    hash_t hash = 5381;
    string_t string = var_string(var);

    int length = var_size(var);
    int i = 0;

    for (; i < length; i++) {
        // equivalent to hash = 33*hash + string[i]
        hash = (hash << 5) + hash + string[i];
    }

    // selected so hash("length") == 0
    return hash + 0xf4d21539;
}



#endif
