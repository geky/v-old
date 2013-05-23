#ifndef LIGHT_TABLE
#define LIGHT_TABLE

#include "var.h"

// table_t declared as struct table *

// definition of entry and table type
typedef struct entry {
    var_t key;
    var_t val;
} entry_t;

struct table {
    uint8_t size;     // log 2 size of the entries
    uint16_t nulls;   // count of null entries
    uint32_t count;   // number of keys in use

    entry_t *entries; // array of entries
};


// looks up a key in the table and 
// returns either its value or null
var_t table_lookup(table_t table, var_t key);


// sets a value in the table with the given key
void table_set(table_t table, var_t key, var_t val);


#endif
