#ifndef LIGHT_TBL
#define LIGHT_TBL

#include "var.h"

// definition of entry and table type
typedef struct entry {
    var_t key;
    var_t val;
} entry_t;

typedef struct tbl {
    uint8_t super;    // indicates if super is present
    uint8_t size;     // log 2 size of the entries
    uint16_t nulls;   // count of null entries
    uint32_t count;   // number of keys in use

    entry_t *entries; // array of entries
} tbl_t;


// functions for creating tables in c
var_t tbl_create(uint32_t len);
void tbl_free(tbl_t *);

// looks up a key in the table recursively
// and returns either its value or null
var_t tbl_lookup(tbl_t *, var_t key);

// sets a value in the table with the given key
// without decending down the super chain
void tbl_assign(tbl_t *, var_t key, var_t val);

// sets a value in the table with the given key
// decends down the super chain unless its not found
void tbl_set(tbl_t *, var_t key, var_t val);

// builin for creating tables
var_t light_tbl(var_t *, int);

#endif
