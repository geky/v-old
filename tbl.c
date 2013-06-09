#include "tbl.h"

#include "str.h"

#include <string.h>
#include <stdio.h>


// the collision resolution is mostly based on
// Python's implementation.


static inline uint8_t tbl_npw2(int v) {
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

static inline int tbl_lcap(int v) {
    return v + (v >> 1);
}


var_t tbl_create(uint32_t len) {
    var_t var;
    tbl_t *tbl;
    ref_t *newval = var_alloc(sizeof(ref_t) + sizeof(tbl_t));

    *newval = 1;
    var.ref = newval;
    var.type = TYPE_TBL;

    tbl = (tbl_t*)(newval+1);
    var.tbl.ptr = tbl;

    tbl->size = tbl_npw2(tbl_lcap(len));
    len = ((uint64_t)1) << tbl->size;

    tbl->entries = var_alloc(sizeof(entry_t) * len);
    memset(tbl->entries, 0, sizeof(entry_t) * len);

    tbl->super = 0;
    tbl->nulls = 0;
    tbl->count = 0;

    return var;
}


void tbl_free(tbl_t *tbl) {
    int i, len = ((uint64_t)1) << tbl->size;
    entry_t *entries = tbl->entries;

    for (i=0; i<len; i++) {
        if (entries[i].key.type != TYPE_NULL) {
            var_dec_ref(entries[i].key);
            var_dec_ref(entries[i].val);
        }
    }

    var_free(entries);
}



static uint32_t tbl_resize(tbl_t *tbl, uint32_t len) {
#ifndef NDEBUG
    printf("resizing to %d for cap %d for %d\n",
           1 << tbl_npw2(tbl_lcap(len)), 
           tbl_lcap(len),
           len);
#endif

    int j, oldsize = ((uint64_t)1) << tbl->size;
    entry_t *oldent = tbl->entries;

    tbl->size = tbl_npw2(tbl_lcap(len));
    len = ((uint64_t)1) << tbl->size;
    uint32_t msize = len - 1;

    tbl->entries = var_alloc(sizeof(entry_t) * len);
    memset(tbl->entries, 0, sizeof(entry_t) * len);

    tbl->nulls = 0;
    tbl->count = 0;

    for (j=0; j<oldsize; j++) {
        if (oldent[j].key.type == TYPE_NULL ||
            oldent[j].val.type == TYPE_NULL)
            continue;

        uint32_t i = var_hash(oldent[j].key);

        for (;; i = (i<<2) + i + 1) {
            entry_t *look = &tbl->entries[i & msize];

            if (look->key.type == TYPE_NULL) {
                look->key = oldent[j].key;
                look->val = oldent[j].val;
                tbl->count++;

                break;
            }
        }
    }

    var_free(oldent);
    return len;
}


static void tbl_set_super(tbl_t *tbl, var_t val) {
    if (tbl->size == 0xff)
        tbl_resize(tbl, 1);

    var_t key_o = tbl->entries->key;
    tbl->entries->key = str_var("super");

    var_t val_o = tbl->entries->val;
    tbl->entries->val = val;
    var_inc_ref(val);

    if ((tbl->super & 0x2) == 0) {
        if (key_o.type == TYPE_NULL)
            tbl->count++;
        else if (val_o.type == TYPE_NULL)
            tbl->nulls--;
        else 
            tbl_assign(tbl, key_o, val_o);
    }

    var_dec_ref(key_o);
    var_dec_ref(val_o);

    tbl->super = (val.type == TYPE_NULL) ? 
                    0x0 : 
                    (0x2 | (val.type == TYPE_TBL));
}



var_t tbl_lookup(tbl_t *tbl, var_t key) {
    if (key.type == TYPE_NULL)
        return null_var;

    uint32_t i, hash = var_hash(key);

    while (tbl->size != 0xff) {
        uint32_t msize = (1 << tbl->size) - 1;

        for (i = hash;; i = (i<<2) + i + 1) {
            entry_t *look = &tbl->entries[hash & msize];

            if (look->key.type == TYPE_NULL)
                break;

            if (var_equals(key, look->key)) {
                if (look->val.type == TYPE_NULL)
                    break;

                return look->val;
            }
        }

        if (!(tbl->super & 0x1))
            break;

        tbl = var_tbl(tbl->entries->val);
    }

    return null_var;
}


void tbl_assign(tbl_t *tbl, var_t key, var_t val) { 
    if (key.type == TYPE_NULL)
        return;

    uint32_t i = var_hash(key);
    uint32_t msize = ((uint64_t)1) << tbl->size;

    if (i == 0 && var_equals(key, str_var("super"))) {
        tbl_set_super(tbl, val);
        return;
    }


    if (tbl_lcap(tbl->count+1) > msize)
        msize = tbl_resize(tbl, tbl->count+1 - tbl->nulls);

    for (msize--;; i = (i<<2) + i + 1) {
        entry_t *look = &tbl->entries[i & msize];

        if (look->key.type == TYPE_NULL) {
            if (val.type != TYPE_NULL) {
                look->key = key;
                var_inc_ref(key);

                look->val = val;
                var_inc_ref(val);

                tbl->count++;
            }

            return;
        }

        if (var_equals(look->key, key)) {
            if (look->val.type == TYPE_NULL)
                continue;

            var_dec_ref(look->val);
            look->val = val;

            if (val.type == TYPE_NULL)
                tbl->nulls++;
            else
                var_inc_ref(val);

            return;
        }
    }
}


void tbl_set(tbl_t *tbl, var_t key, var_t val) {
    if (key.type == TYPE_NULL)
        return;

    uint32_t i, hash = var_hash(key);
    tbl_t *link = tbl;

    if (hash == 0 && var_equals(key, str_var("super"))) {
        tbl_set_super(tbl, val);
        return;
    }


    while (1) {
        uint32_t j, len = ((uint64_t)1) << link->size;
        uint32_t msize = len - 1;

        for (i = hash, j = 0; j < len; i = (i<<2) + i + 1, j++) {
            entry_t *look = &link->entries[hash & msize];

            if (look->key.type == TYPE_NULL)
                break;

            if (var_equals(look->key, key)) {
                if (look->val.type == TYPE_NULL)
                    break;

                var_dec_ref(look->val);
                look->val = val;

                if (val.type == TYPE_NULL)
                    tbl->nulls++;
                else
                    var_inc_ref(val);

                return;
            }
        }

        if (!(link->super & 0x1))
            break;

        link = var_tbl(link->entries->val);
    }


    if (val.type == TYPE_NULL)
        return;

    uint32_t msize = ((uint64_t)1) << tbl->size;
    tbl->count++;

    if (tbl_lcap(tbl->count) > msize)
        msize = tbl_resize(tbl, tbl->count - tbl->nulls);

    for (i = hash, msize--;; i = (i<<2) + i + 1) {
        entry_t *look = &tbl->entries[i & msize];

        if (look->key.type == TYPE_NULL) {
            look->key = key;
            var_inc_ref(key);

            look->val = val;
            var_inc_ref(val);

            return;
        }
    }
}


var_t light_tbl(var_t *v, int n) {
    if (n < 1)
        return tbl_create(0);

    switch (v->type) {
        case TYPE_NUM:
            return tbl_create(ceil(var_num(*v)));

        case TYPE_CSTR:
        case TYPE_STR: {
            uint32_t i, len = v->str.len;

            var_t out = tbl_create(len);

            for (i=0; i<len; i++)
                tbl_assign(var_tbl(out), num_var(i), str_substr(*v, i, i+1));

            return out;
        }

        case TYPE_TBL: {
            tbl_t *tbl = var_tbl(*v);
            uint32_t i, len = ((uint64_t)1) << tbl->size;
            
            var_t out = tbl_create(tbl->count - tbl->nulls);

            for (i=0; i<len; i++) {
                entry_t *look = &tbl->entries[i];

                tbl_assign(var_tbl(out), look->key, look->val);
            }

            return out;
        }

        default:
            return tbl_create(0);
    }
}
