#ifndef LIGHT_TABLE
#define LIGHT_TABLE

#include "var.h"


// definition of entry and table type
typedef struct entry {
    var_t key;
    var_t val;
} entry_t;

struct table {
    uint32_t size;    // size of the entries array
    uint32_t count;   // number of keys in use

    entry_t *entries; // array of entries
};


// looks up a key in the table and 
// returns either its value or null
var_t table_lookup(var_t table, var_t key);


// sets a value in the table with the given key
void table_set(var_t table, var_t key, var_t val);


#endif
