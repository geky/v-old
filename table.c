#include "var.h"
#include "table.h"


// the collision resolution is mostly based on
// Python's implementation. Including the 
// perturb shift applied as well as the recurrence

#define MIN_TABLE_SIZE 8
#define PERTURB_SHIFT 5


static void table_resize(table_t table, uint32_t size) {
    uint32_t oldsize = 1 << table->size;
    entry_t *oldent = table->entries;

    int i = 0;

    table->size = size;
    table->entries = var_alloc(sizeof(entry_t) * (1 << size));

    for (; i < oldsize; i++)
        table_set(table, oldent[i].key, oldent[i].val);

    var_free(oldent);
}


var_t table_lookup(table_t table, var_t key) {
    uint32_t msize = (1 << table->size) - 1;
    entry_t *entries = table->entries;
    entry_t *entry;

    hash_t hash = var_hash(key);
    uint32_t i = hash;

    if (var_type(key) == NULL_VAR)
        return null_var;


    while (1) {
        entry = &entries[i & msize];

        if (var_type(entry->key) == NULL_VAR)
            return null_var; 

        if (var_equals(key, entry->key))
            return entry->val;

        i = (i << 2) + i + hash + 1;
        hash >>= PERTURB_SHIFT;
    }
}


void table_set(table_t table, var_t key, var_t val) {
    uint32_t msize = 1 << table->size;
    entry_t *entries = table->entries;
    entry_t *entry;

    hash_t hash = var_hash(key);
    uint32_t i = hash;

    if (var_type(key) == NULL_VAR)
        return;


    // check if count is > 0.75 * msize
    if (table->count + 1 > (msize >> 1) + (msize >> 2)) {
        table_resize(table, table->size+1);
        msize <<= 1;
    }


    while (1) {
        entry = &entries[i & msize];

        if (var_type(entry->key) == NULL_VAR) {
            if (var_type(val) != NULL_VAR) {
                entry->key = key;
                entry->val = val;

                var_inc_ref(key);
                var_inc_ref(val);

                table->count++;
            }
            return;
        }

        if (var_type(entry->val) == NULL_VAR) {
            if (var_type(val) != NULL_VAR) {
                entry->key = key;
                entry->val = val;

                var_inc_ref(key);
                var_inc_ref(val);

                table->nulls--;
            }
            return;
        }

        if (var_equals(key, entry->key)) {
            if (var_type(val) == NULL_VAR)
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




