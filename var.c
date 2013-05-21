#include "var.h"

#include <stdio.h>


int equals(var_t a, var_t b) {
    if (a.meta != b.meta)
        return 0;

    switch (var_type(a)) {
        case NULL_VAR:   return 1; // all nulls are equivalent
        case NUM_VAR:    return num_equals(a,b);
        case STRING_VAR: return string_equals(a,b);
        default:         return a.data == b.data;
    }
}


hash_t hash(var_t var) {
    switch (var_type(var)) {
        case NULL_VAR:   return 0; // nulls should never be hashed
        case NUM_VAR:    return num_hash(var);
        case STRING_VAR: return string_hash(var);
        default:         return (hash_t)var.data;
    }
}



// prints currently really just for debugging
// should either be removed or refactored, very hacky as is

static inline void print_table(var_t var) {
    table_t table = var_table(var);
                                                             
    int arr = 1;
    double arr_v = 0.0;

    int i = 0;
    int length = var_size(var);
 
    printf("[");
                                                             
    for (; i<length; i++) {
        var_t key = table[i].key;
        var_t val = table[i].val;

        if (var_type(key) == NULL_VAR) {
            arr = 0;
                                                             
        } else {

            if (var_type(key) == NUM_VAR) {
                if (arr && var_num(key) == arr_v) {
                    arr_v += 1.0;
                                                             
                } else {
                    print(key);
                    printf(": ");

                    arr = 1;
                    arr_v = var_num(key) + 1.0;
                }
                                                             
            } else {
                print(key);
                printf(": ");
                                                             
                arr = 0;
            }


            print(val);

            if (i < length-1)
                printf(", ");
            else
                printf("]");
        }
    }
}

void print(var_t var) {
    switch (var_type(var)) {
        case NULL_VAR:   printf("null");                                 break;
        case NUM_VAR:    printf("%f", var_num(var));                     break;
        case STRING_VAR: printf("%.*s", var_size(var), var_string(var)); break;
        case TABLE_VAR:  print_table(var);                               break;
        case FUNC_VAR:   printf("func { %p }", var.data);                break;
        default:         printf("BAD VAR");                              break;
    }
}
