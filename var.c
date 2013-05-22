#include "var.h"

#include "table.h"

#include <string.h>
#include <stdio.h>


int var_equals(var_t a, var_t b) {
    if (var_meta(a) != var_meta(b))
        return 0;

    switch (var_type(a)) {
        case NULL_VAR: return 1; // all nulls are equivalent
        case NUM_VAR:  return var_num(a) == var_num(b);
        case STR_VAR:  return !memcmp(var_str(a), var_str(b), var_size(a));
        default:       return var_data(a) == var_data(b);
    }
}


hash_t var_hash(var_t var) {
    switch (var_type(var)) {
        case NULL_VAR:
            return 0; // nulls should never be hashed

        case NUM_VAR: {
            var_meta(var) &= ~0x7;

            // take int value as base to keep
            // it linear for integers
            hash_t hash = (unsigned int)var.num;

            // move decimal part around to fit into
            // an int value to add to the hash
            var.num -= hash;
            var.num += 0x100000;

            return hash ^ var_meta(var);
        }

        case STR_VAR: {
            // based off the djb2 algorithm
            hash_t hash = 5381;
            str_t str = var_str(var);

            int len = var_size(var);
            int i = 0;

            // keep it from hashing overly long strings
            if (len > 32)
                len = 32;

            for (; i < len; i++) {
                // hash = 33*hash + str[i]
                hash = (hash << 5) + hash + str[i];
            }

            return hash;
        }
            
        default:
            return (hash_t)var_data(var);
    }
}



// prints currently really just for debugging
// should either be removed or refactored, very hacky as is

static inline void print_table(var_t var) {
    int arr = 1;
    double arr_v = 0.0;

    int i = 0;
    int length = var_table(var)->size;
    entry_t *entries = var_table(var)->entries;
 
    printf("[");
                                                             
    for (; i<length; i++) {
        var_t key = entries[i].key;
        var_t val = entries[i].val;

        if (var_type(key) == NULL_VAR || var_type(val) == NULL_VAR) {
            arr = 0;
                                                             
        } else {
            if (var_type(key) == NUM_VAR) {
                if (arr && var_num(key) == arr_v) {
                    arr_v += 1.0;
                                                             
                } else {
                    var_print(key);
                    printf(": ");

                    arr = 1;
                    arr_v = var_num(key) + 1.0;
                }
                                                             
            } else {
                var_print(key);
                printf(": ");
                                                             
                arr = 0;
            }


            var_print(val);

            if (i < length-1)
                printf(", ");
            else
                printf("]");
        }
    }
}

void var_print(var_t var) {
    switch (var_type(var)) {
        case NULL_VAR:  printf("null");                              break;
        case NUM_VAR:   printf("%f", var_num(var));                  break;
        case STR_VAR:   printf("%.*s", var_size(var), var_str(var)); break;
        case TABLE_VAR: print_table(var);                            break;
        case FUNC_VAR:  printf("func { %p }", var_data(var));        break;
        default:        printf("BAD VAR");                           break;
    }
}
