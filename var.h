#ifndef V_VAR
#define V_VAR

#include <stdint.h>
#include <stdlib.h>

#include <math.h>


/* definitions of variable type */

// used definitions

// variable identifiers

typedef enum type {
    TYPE_NULL = 0x0,

    TYPE_NUM  = 0x1,
    TYPE_STR  = 0x4,

    TYPE_TBL  = 0x5,

    TYPE_FN   = 0x6,
    TYPE_BFN  = 0x2,

    TYPE_ERR  = 0x7
} type_t;

typedef uint32_t hash_t;
typedef unsigned int ref_t;

typedef struct var {
    union {
        uint64_t bits;
        uint8_t  bytes[8];

        struct {
            union {
                uint32_t meta;
                type_t type : 3;

                ref_t *ref;
                const char *str;
            };

            union {
                uint32_t data;

                struct {
                    uint16_t off;
                    uint16_t len;
                };

                struct tbl *tbl;
                struct fn *fn;
                struct var (*bfn)(struct var);

                struct var *err;
            };
        };

        double num;
    };
} var_t;


/* useful macro definitions */

// for retrieving components
#define var_ref(v) (*((var_t){{(v).bits & ~0x7}}).ref)

#define var_num(v) (((var_t){{(v).bits & ~0x7}}).num)
#define var_str(v) ((v).str + (v).off)

// macros for creating literal vars in c

#define null_var     ((var_t){{0}})
#define null_flag(n) ((var_t){.meta=(~0x7 & (n))})
#define err_flag(n)  ((var_t){.meta=( 0x7 | (n))})

#define num_var(n)   ((var_t){{TYPE_NUM | (~0x7 & ((var_t){.num=(n)}).bits)}})
#define nan_var      num_var(NAN)
#define inf_var      num_var(INFINITY)

#define str_var(n)   ({ static struct {                        \
                          ref_t __attribute__((aligned(8))) r; \
                          const char s[sizeof(n)-1];           \
                        } str = { 1, {(n)} };                  \
                                                               \
                        ((var_t){{TYPE_STR |                   \
                          (~0x7 & ((var_t){                    \
                            .ref = (ref_t*)&str,               \
                            .off = 0,                          \
                            .len = sizeof(n)-1                 \
                          }).bits)    }});     })

#define fn_var(n)   ((var_t){.type=TYPE_BFN, .bfn=(n)})




/* function definitions for variable */

// Handlers for internal reference counting to 
// manage garbage collection. Deallocates immediatly
// when references hits zero.
void var_inc_ref(var_t var);
void var_dec_ref(var_t var);

// Function for manually freeing variables
void var_free(var_t var);

// Internal wrapper for malloc that handles things like
// failures and alignment.
void *var_alloc(size_t size);
void var_dealloc(void *ptr);

// Function for printing variables easily for debugging
void var_print(var_t v);

// Equals returns true if both variables are the
// same type and equivalent.
int var_equals(var_t a, var_t b);

// Hash returns a hash value of the given variable. 
hash_t var_hash(var_t var);

// Type returns the function representing the type
var_t light_type(var_t *, int);

// Repr returns a string representation of a var
var_t light_repr(var_t *, int);

// builtin for creating nulls
var_t light_null(var_t *, int);


#endif
