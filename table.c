#include "table.h"

#include <string.h>
#include <stdio.h>


// the collision resolution is mostly based on
// Python's implementation. Including the 
// perturb shift applied as well as the recurrence

#ifndef PERTURB_SHIFT
#define PERTURB_SHIFT 5
#endif


static inline uint8_t table_npw2(int v) {
    uint32_t r;
    uint32_t s;

    if (!v) return 0xff;

    v--;
    r = (v > 0xffff) << 4; v >>= r;
    s = (v > 0xff  ) << 3; v >>= s; r |= s;
    s = (v > 0xf   ) << 2; v >>= s; r |= s;
    s = (v > 0x3   ) << 1; v >>= s; r |= s;

    return  (r | (v >> 1)) + 1;
}

static inline int table_lcap(int v) {
    return v + (v >> 1);
}


var_t table_create(uint32_t len) {
    var_t var;
    table_t *table;
    ref_t *newval = var_alloc(sizeof(ref_t) + sizeof(table_t));

    *newval = 1;
    var_meta(var) = TYPE_TABLE | (uint32_t)newval;
    table = (table_t*)(newval+1);
    var_table(var) = table;

    table->size = table_npw2(table_lcap(len));
    len = ((uint64_t)1) << table->size;

    table->entries = var_alloc(sizeof(entry_t) * len);
    memset(table->entries, 0, sizeof(entry_t) * len);

    table->nulls = 0;
    table->count = 0;

    return var;
}


void table_free(table_t *table) {
    int i, length = ((uint64_t)1) << table->size;
    entry_t *entries = table->entries;

    for (i=0; i<length; i++) {
        if (var_type(entries[i].key) != TYPE_NULL) {
            var_dec_ref(entries[i].key);
            var_dec_ref(entries[i].val);
        }
    }

    var_free(entries);
}



static uint32_t table_resize(table_t *table, uint32_t len) {
    int i, oldsize = ((uint64_t)1) << table->size;
    entry_t *oldent = table->entries;

    table->size = table_npw2(table_lcap(len));
    len = ((uint64_t)1) << table->size;

    table->entries = var_alloc(sizeof(entry_t) * len);
    memset(table->entries, 0, sizeof(entry_t) * len);

    table->nulls = 0;
    table->count = 0;

    for (i=0; i<oldsize; i++)
        table_set(table, oldent[i].key, oldent[i].val);

    var_free(oldent);
    return len;
}


var_t table_lookup(table_t *table, var_t key) {
    uint32_t msize = (((uint64_t)1) << table->size) - 1;
    entry_t *look;

    hash_t hash = var_hash(key);
    uint32_t i = hash;

    if (table->size == 0xff || var_type(key) == TYPE_NULL)
        return null_var;

    while (1) {
        look = &table->entries[i & msize];

        if (var_type(look->key) == TYPE_NULL)
            return null_var; 

        if (var_equals(key, look->key))
            return look->val;

        i = (i << 2) + i + hash + 1;
        hash >>= PERTURB_SHIFT;
    }
}


void table_set(table_t *table, var_t key, var_t val) {
    uint32_t msize = (((uint64_t)1) << table->size);
    entry_t *entry;

    hash_t hash = var_hash(key);
    uint32_t i = hash;

    if (var_type(key) == TYPE_NULL)
        return;

    if (table->size == 0xff || table_lcap(table->count+1) > msize)
        msize = table_resize(table, table->count+1 - table->nulls);

    msize--;

    while (1) {
        entry = &table->entries[i & msize];

        if (var_type(entry->key) == TYPE_NULL) {
            if (var_type(val) != TYPE_NULL) {
                entry->key = key;
                entry->val = val;

                var_inc_ref(key);
                var_inc_ref(val);

                table->count++;
            }
            return;
        }

        if (var_type(entry->val) == TYPE_NULL) {
            if (var_type(val) != TYPE_NULL) {
                var_dec_ref(entry->key);

                entry->key = key;
                entry->val = val;

                var_inc_ref(key);
                var_inc_ref(val);

                table->nulls--;
            }
            return;
        }

        if (var_equals(key, entry->key)) {
            if (var_type(val) == TYPE_NULL)
                table->nulls++;
            else
                var_inc_ref(val);

            var_dec_ref(entry->val);
            entry->val = val;

            return;
        }

        i = (i << 2) + i + hash + 1;
        hash >>= PERTURB_SHIFT;
    }
}




