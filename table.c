#include "table.h"

#include "str.h"

#include <string.h>
#include <stdio.h>


// the collision resolution is mostly based on
// Python's implementation.


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

    table->super = 0;
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
    printf("resizing to %d for cap %d for %d\n",
           1 << table_npw2(table_lcap(len)), 
           table_lcap(len),
           len);

    int j, oldsize = ((uint64_t)1) << table->size;
    entry_t *oldent = table->entries;

    table->size = table_npw2(table_lcap(len));
    len = ((uint64_t)1) << table->size;
    uint32_t msize = len - 1;

    table->entries = var_alloc(sizeof(entry_t) * len);
    memset(table->entries, 0, sizeof(entry_t) * len);

    table->nulls = 0;
    table->count = 0;

    for (j=0; j<oldsize; j++) {
        if (var_type(oldent[j].key) == TYPE_NULL ||
            var_type(oldent[j].val) == TYPE_NULL)
            continue;

        hash_t i = var_hash(oldent[j].key);

        for (;; i = (i<<2) + i + 1) {
            entry_t *look = &table->entries[i & msize];

            if (var_type(look->key) == TYPE_NULL) {
                look->key = oldent[j].key;
                look->val = oldent[j].val;
                table->count++;

                break;
            }
        }
    }

    var_free(oldent);
    return len;
}


static void table_set_super(table_t *table, var_t val) {
    if (table->size == 0xff)
        table_resize(table, 1);

    var_t key_o = table->entries->key;
    table->entries->key = str_var("super");

    var_t val_o = table->entries->val;
    table->entries->val = val;
    var_inc_ref(val);

    if ((table->super & 0x2) == 0) {
        if (var_type(key_o) == TYPE_NULL)
            table->count++;
        else if (var_type(val_o) == TYPE_NULL)
            table->nulls--;
        else 
            table_assign(table, key_o, val_o);
    }

    var_dec_ref(key_o);
    var_dec_ref(val_o);

    table->super = (var_type(val) == TYPE_NULL) ? 0x0 : 
                   0x2 | (var_type(val) == TYPE_TABLE);
}



var_t table_lookup(table_t *table, var_t key) {
    if (var_type(key) == TYPE_NULL)
        return null_var;

    hash_t i, hash = var_hash(key);

    while (table->size != 0xff) {
        uint32_t msize = (1 << table->size) - 1;

        for (i = hash;; i = (i<<2) + i + 1) {
            entry_t *look = &table->entries[hash & msize];

            if (var_type(look->key) == TYPE_NULL)
                break;

            if (var_equals(key, look->key)) {
                if (var_type(look->val) == TYPE_NULL)
                    break;

                return look->val;
            }
        }

        if (!(table->super & 0x1))
            break;

        table = var_table(table->entries->val);
    }

    return null_var;
}


void table_assign(table_t *table, var_t key, var_t val) { 
    if (var_type(key) == TYPE_NULL)
        return;

    hash_t i = var_hash(key);
    uint32_t msize = ((uint64_t)1) << table->size;

    if (i == 0 && var_equals(key, str_var("super"))) {
        table_set_super(table, val);
        return;
    }


    if (table_lcap(table->count+1) > msize)
        msize = table_resize(table, table->count+1 - table->nulls);

    for (msize--;; i = (i<<2) + i + 1) {
        entry_t *look = &table->entries[i & msize];

        if (var_type(look->key) == TYPE_NULL) {
            if (var_type(val) != TYPE_NULL) {
                look->key = key;
                var_inc_ref(key);

                look->val = val;
                var_inc_ref(val);

                table->count++;
            }

            return;
        }

        if (var_equals(look->key, key)) {
            if (var_type(look->val) == TYPE_NULL)
                continue;

            var_dec_ref(look->val);
            look->val = val;

            if (var_type(val) == TYPE_NULL)
                table->nulls++;
            else
                var_inc_ref(val);

            return;
        }
    }
}


void table_set(table_t *table, var_t key, var_t val) {
    if (var_type(key) == TYPE_NULL)
        return;

    hash_t i, hash = var_hash(key);
    table_t *link = table;

    if (hash == 0 && var_equals(key, str_var("super"))) {
        table_set_super(table, val);
        return;
    }


    while (1) {
        uint32_t j, len = ((uint64_t)1) << link->size;
        uint32_t msize = len - 1;

        for (i = hash, j = 0; j < len; i = (i<<2) + i + 1, j++) {
            entry_t *look = &link->entries[hash & msize];

            if (var_type(look->key) == TYPE_NULL)
                break;

            if (var_equals(look->key, key)) {
                if (var_type(look->val) == TYPE_NULL)
                    break;

                var_dec_ref(look->val);
                look->val = val;

                if (var_type(val) == TYPE_NULL)
                    table->nulls++;
                else
                    var_inc_ref(val);

                return;
            }
        }

        if (!(link->super & 0x1))
            break;

        link = var_table(link->entries->val);
    }


    if (var_type(val) == TYPE_NULL)
        return;

    uint32_t msize = ((uint64_t)1) << table->size;
    table->count++;

    if (table_lcap(table->count) > msize)
        msize = table_resize(table, table->count - table->nulls);

    for (i = hash, msize--;; i = (i<<2) + i + 1) {
        entry_t *look = &table->entries[i & msize];

        if (var_type(look->key) == TYPE_NULL) {
            look->key = key;
            var_inc_ref(key);

            look->val = val;
            var_inc_ref(val);

            return;
        }
    }
}


var_t light_table(var_t *v, int n) {
    if (n < 1)
        return table_create(0);

    switch (var_type(*v)) {
        case TYPE_NUM:
            return table_create(ceil(var_num(*v)));

        case TYPE_STR: {
            uint32_t i, length = var_len(*v);

            var_t out = table_create(var_len(*v));

            for (i = 0; i < length; i++)
                table_assign(var_table(out), num_var(i), str_substr(*v, i, i+1));

            return out;
        }

        case TYPE_TABLE: {
            table_t *table = var_table(*v);
            uint32_t i, length = ((uint64_t)1) << table->size;
            
            var_t out = table_create(table->count - table->nulls);

            for (i = 0; i < length; i++) {
                entry_t *look = &table->entries[i];

                table_assign(var_table(out), look->key, look->val);
            }

            return out;
        }

        default:
            return table_create(0);
    }
}
