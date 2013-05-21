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


#endif
