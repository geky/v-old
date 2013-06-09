#include "var.h"

#include "table.h"

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>


#ifndef NDEBUG
int64_t d_space_used;
int64_t d_space_left;
#endif



void var_inc_ref(var_t var) {
    if (var_type(var) & 0x4)
        var_ref(var)++;
}


void var_dec_ref(var_t var) {
    ref_t *ref;

    if (!(var_type(var) & 0x4))
        return;

    ref = &var_ref(var);
    assert(*ref > 0);

    if (--(*ref) == 0) {
        if (var_type(var) == TYPE_TABLE)
            table_free(var_table(var));

        var_free(ref);
    }
}



void *var_alloc(size_t size) {
    union { 
        void *ptr; 
        uint32_t bits; 
    } temp;

    if (size == 0) return 0;

#ifndef NDEBUG
    size += 8;
#endif

    temp.ptr = (void*)malloc(size);


    assert(temp.ptr != 0); // out of memory
    assert(sizeof temp == 4); // size constraint
    assert((temp.bits & 0x7) == 0); // alignment

#ifndef NDEBUG
    *(int64_t*)temp.ptr = size;
    d_space_used += size;
    d_space_left += size;
    temp.ptr = ((int64_t*)temp.ptr) + 1;
#endif

    return temp.ptr;
}

void var_free(void *ptr) {
#ifndef NDEBUG
    if (ptr) {
        ptr = ((int64_t*)ptr) - 1;
        d_space_left -= *(int64_t*)ptr;
    }
#endif

    free(ptr);
}



int var_equals(var_t a, var_t b) {
    if (var_type(a) != var_type(b))
        return 0;

    switch (var_type(a)) {
        case TYPE_NULL: return 1; // all nulls are equivalent
        case TYPE_NUM:  return var_num(a) == var_num(b);
        case TYPE_CSTR:
        case TYPE_STR:  return !memcmp(var_str(a), var_str(b), var_len(a));
        default:        return var_data(a) == var_data(b);
    }
}


hash_t var_hash(var_t var) {
    switch (var_type(var)) {
        case TYPE_NULL:
            return 0; // nulls should never be hashed

        case TYPE_NUM: {
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

        case TYPE_CSTR:
        case TYPE_STR: {
            // based off the djb2 algorithm
            hash_t hash = 5381;
            char *str = var_str(var);

            int len = var_len(var);
            int i = 0;

            // keep it from hashing overly long strings
            if (len > 32)
                len = 32;

            for (; i < len; i++) {
                // hash = 33*hash + str[i]
                hash = (hash << 5) + hash + str[i];
            }

            return hash - 274863188;
        }
            
        default:
            return (hash_t)var_data(var);
    }
}




/*
var_t light_num(int n, var_t var, var_t rad) {
    if (n < 1)
        return num_var(0);

    switch (var_type(var)) {
        case TYPE_NUM:
            return var;

        case TYPE_STR: {
            if (n > 1)
                return num_parse(var_str(var), (int)var_num(rad));
            else
                return num_parse(var_str(var), 10);
        }

        case TYPE_TABLE: {
            var_t temp = table_lookup(var_table(var), str_var("to_num"));

            if (var_type(temp) == TYPE_FUNC || var_type(temp) == TYPE_BIN)
                temp = func_call(temp)

            if (var_type(temp) == TYPE_NUM)
                return temp;
            else
                return num_var(0);
        }

        default:
            return num_var(0);
    }
}


var_t light_str(int n, var_t var) {
    if (n < 1)
        return str_var("");

    switch (var_type(var)) {
        case TYPE_NULL:  return str_var("null");
        case TYPE_NUM:   //TODO
        case TYPE_STR:   return var;
        case TYPE_TABLE: //TODO
        case TYPE_FUNC:  //TODO
        case TYPE_BIN:   return str_var("func() <builtin>");
        default:         return str_var("");
    }
}


var_t light_table(int n, var_t var) {
    if (n < 1)
        return table_create();

    switch (var_type(var)) {
        case TYPE_NUM:
            return table_create_len((int)var_num(len));

        case TYPE_STR:
            // TODO turn into list

        case TYPE_TABLE: {
            var_t temp = table_create(table_len(var));



            return var;

        default:
            return table_create();
    }
}



var_t light_func(int n, var_t var) {
    if (n < 1)
        return nofunc;

    switch (var_type(var)) {
        case TYPE_TABLE:
            // TODO have it return tabled function

        default:
            // TODO have it return function that returns var
    }
}

*/
/*
var_t light_type(var_t *v, int n) {
    if (n < 1)
        return null_var;

    switch (var_type(*v)) {
        case TYPE_NULL:  return func_var(light_null);
        case TYPE_NUM:   return func_var(light_num);
        case TYPE_CSTR:
        case TYPE_STR:   return func_var(light_str);
        case TYPE_TABLE: return func_var(light_table);
        case TYPE_FUNC
        case TYPE_BIN:   return func_var(light_func);
        default:         return null_var;
    }
}
*/

var_t light_repr(var_t *v, int n) {
    if (n < 1)
        return null_var;

    switch (var_type(*v)) {
        case TYPE_NULL:
            return str_var("null");

        case TYPE_NUM: {
            double num = var_num(*v);

            if (isnan(num)) {
                return str_var("nan");

            } else if (isinf(num)) {
                var_t val = str_var("-inf");

                if (num > 0.0)
                    var_off(val)++;

                return val;

            } else if (num == 0.0) {
                return str_var("0");

            } else {
                var_t val;
                char *str = var_alloc(sizeof(ref_t) + 16);

                var_meta(val) = (uint32_t)str;
                str += 4;

                if (num < 0.0) {
                    num = -num;
                    *(str++) = '-';
                }

                int exp = (int)log10(num);
                double digit = pow(10.0, exp);

                int expform = (exp > 14 || exp < -13);
                if (expform) {
                    num /= digit;
                    digit = 1.0;
                }

                if (digit < 1.0)
                    digit = 1.0;

                int i, length = expform ? 10 : 15;

                for (i=0; (num || digit >= 1.0) && i<length; i++) {
                    if (digit == 0.1)
                        *(str++) = '.';

                    int d = (int)(num / digit);
                    *(str++) = '0' + d;

                    num -= d * digit;
                    digit /= 10.0;
                }

                if (expform) {
                    *(str++) = 'e';

                    if (exp > 0.0) {
                        *(str++) = '+';
                    } else {
                        *(str++) = '-';
                        exp = -exp;
                    }

                    if (exp > 100.0) 
                        *(str++) = '0' + (int)(exp / 100.0);

                    // exp will always be greater than 10 here
                    *(str++) = '0' + (int)(exp / 10.0) % 10;
                    *(str++) = '0' + (int)(exp / 1.0) % 10;
                    
                }

                var_len(val) = (uint32_t)(str - var_meta(val) - 4);
                var_off(val) = 0;
                var_meta(val) |= TYPE_STR;
                var_ref(val) = 1;

                return val;
            }
        }

        case TYPE_CSTR:
        case TYPE_STR: {
            var_t val;

            var_off(val) = 0;
            var_len(val) = var_len(*v) + 2;

            char *str = var_alloc(sizeof(ref_t) + var_len(val));

            var_meta(val) = TYPE_STR | (uint32_t)str;
            var_ref(val) = 1;
            str += 4;

            str[0] = '\'';
            str[var_len(val)-1] = '\'';
            memcpy(str+1, var_str(*v), var_len(*v));

            return val;
        }

        case TYPE_TABLE: {
            table_t *table = var_table(*v);
            int i, length = ((uint64_t)1) << table->size;
            int space = 2; // make space for braces

            entry_t *entries = table->entries;
            entry_t *entstrs = var_alloc(sizeof(entry_t) * length);

            int valid_n = 1;
            int next_n = 0;

            for (i=0; i<length; i++) {
                int key_t = var_type(entries[i].key);
                int val_t = var_type(entries[i].val);

                if (key_t == TYPE_NULL || val_t == TYPE_NULL)
                    continue;

                if (key_t == TYPE_STR)
                    entstrs[i].key = entries[i].key;
                else
                    entstrs[i].key = light_repr(&entries[i].key, 1);

                entstrs[i].val = light_repr(&entries[i].val, 1);

                space += var_len(entstrs[i].key);
                space += var_len(entstrs[i].val);
                space += 4; // make space for colon and commas
            }

            var_t val;
            char *str = var_alloc(sizeof(ref_t) + space);

            var_meta(val) = (uint32_t)str;
            str += 4;

            *(str++) = '[';

            for (i=0; i<length; i++) {
                int key_t = var_type(entries[i].key);
                int val_t = var_type(entries[i].val);
                double key_n = var_num(entries[i].key);

                if (key_t == TYPE_NULL || val_t == TYPE_NULL)
                    continue;

                if (valid_n && key_t == TYPE_NUM && key_n == next_n) {
                    next_n++;
                } else {
                    next_n = (int)key_n;
                    valid_n = (key_t == TYPE_NUM && next_n == key_n);

                    memcpy(str, var_str(entstrs[i].key), var_len(entstrs[i].key));
                    str += var_len(entstrs[i].key);

                    *(str++) = ':';
                    *(str++) = ' ';
                }

                memcpy(str, var_str(entstrs[i].val), var_len(entstrs[i].val));
                str += var_len(entstrs[i].val);

                *(str++) = ',';
                *(str++) = ' ';

                var_dec_ref(entstrs[i].key);
                var_dec_ref(entstrs[i].val);
            }

            var_len(val) = (uint32_t)(str - var_meta(val) - 4);
            var_off(val) = 0;
            var_meta(val) |= TYPE_STR;
            var_ref(val) = 1;

            if (var_len(val) == 1) {
                var_len(val)++;
                *str = ']';
            } else {
                var_len(val)--;
                *(str-2) = ']';
            }

            var_free(entstrs);

            return val;
        }

        case TYPE_FUNC:
            return str_var(""); // TODO

        case TYPE_BIN:
            return str_var("func() <builtin>");

        default:
            assert(0); // Unable to represent bad value
            return null_var;
    }
}


var_t light_null(var_t *v, int n) {
    return null_var;
}

