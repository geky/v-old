#ifndef LIGHT_TABLE
#define LIGHT_TABLE


// definition of entry and table type
typedef struct entry {
    var_t key;
    var_t val;
} entry_t;

typedef entry_t *table_t;


// extracts entry array from var_t
#define var_table(v) (*(entry_t**)((v).data))


// looks up a key in the table and 
// returns either its value or null
var_t table_lookup(var_t table, var_t key);


// sets a value in the table with the given key
void table_set(var_t table, var_t key, var_t val);


#endif
