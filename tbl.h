#ifndef LIGHT_TBL
#define LIGHT_TBL

#include "var.h"

// definition of entry and table type
typedef struct entry {
    var_t key;
    var_t val;
} entry_t;

typedef struct tbl {
    ref_t ref;

    uint8_t super;    // indicates if super is present
    uint8_t size;     // log 2 size of the entries
    uint16_t nulls;   // count of null entries
    uint32_t count;   // number of keys in use

    entry_t *entries; // array of entries
} tbl_t;


// macro for looping through all values in a table
#define for_tbl(k, v, tbl, block) {                         \
        int i, l = ((uint64_t)1) << (tbl)->size;            \
        var_t k, v;                                         \
                                                            \
        for (i=0; i<l; i++) {                               \
            k = (tbl)->entries[i].key;                      \
            v = (tbl)->entries[i].val;                      \
            if ( k.type != TYPE_NULL &&                     \
                 v.type != TYPE_NULL ) {                    \
                block                                       \
            }                                               \
        }                                                   \
    }


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
