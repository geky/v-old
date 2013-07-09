#ifndef LIGHT_VAR
#define LIGHT_VAR

#include <stdint.h>
#include <stdlib.h>

#include <math.h>


/* definitions of variable type */

// used definitions

// variable identifiers

typedef enum type {
    TYPE_NULL = 0x0,

    TYPE_NUM  = 0x1,

    TYPE_STR  = 0x6,
    TYPE_CSTR = 0x2,

    TYPE_TBL  = 0x4,

    TYPE_FN   = 0x7,
    TYPE_BFN  = 0x3,
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
            };
        };

        double num;
    };
} var_t;


//typedef union var {
//    uint64_t bits;
//    uint8_t  bytes[8];
//
//    struct {
//        uint32_t meta;
//        uint32_t data;
//    } v;
//
//    uint32_t type : 3;
//    ref_t *ref;
//
//
//    double num;
//
//    struct {
//        ref_t *ref;
//
//        uint16_t off;
//        uint16_t len;
//    } str;
//
//    struct {
//        ref_t *ref;
//        struct tbl *ptr;
//    } tbl;
//
//    struct {
//        ref_t *ref;
//        struct fn *ptr;
//    } fn;
//
//    struct {
//        uint32_t _padd;
//        union var *(*ptr)(union var *, int);
//    } bfn;
//} var_t;
//

/* useful macro definitions */

// for retrieving components
#define var_ref(v) (*((var_t){{(v).bits & ~0x7}}).ref)

#define var_num(v) (((var_t){{(v).bits & ~0x7}}).num)
#define var_str(v) ((const char*)(&var_ref(v)+1) + ((v).off))
#define var_tbl(v) ((v).tbl)
#define var_fn(v)  ((v).fn)
#define var_bfn(v) ((v).bfn)

// macros for creating literal vars in c

#define null_var   ((var_t){{0}})

#define nan_var    ((var_t){{TYPE_NUM | (uint64_t)NAN}})

#define inf_var    ((var_t){{TYPE_NUM | (uint64_t)INFINITY}})

#define num_var(n) ((var_t){{TYPE_NUM | (~0x7 &  \
                    ((union { double d; uint64_t i; }){n}).i)}})

#define str_var(n) ({ const static struct {                  \
                        ref_t __attribute__((aligned(8))) r; \
                        char s[sizeof(n)-1];                 \
                      } str = { 1, {(n)} };                  \
                                                             \
                      ((var_t){{TYPE_CSTR |                  \
                        ((union { struct {                   \
                            const void *s;                   \
                            uint16_t off;                    \
                            uint16_t len;                    \
                        } s; uint64_t i; }                   \
                      ){{&str, 0, sizeof(n)-1}}).i}}); })

#define fn_var(n)  ((var_t){{TYPE_BFN | (~0x7 &         \
                       ((union { struct {               \
                           uint32_t padd;               \
                           var_t (*bfn)(var_t);         \
                       }; uint64_t i; }){{0, n}}).i)}})



/* function definitions for variable */

// Handlers for internal reference counting to 
// manage garbage collection. Deallocates immediatly
// when references hits zero.
void var_inc_ref(var_t var);
void var_dec_ref(var_t var);

// Internal wrapper for malloc that handles things like
// failures and alignment.
void *var_alloc(size_t size);
void var_free(void *ptr);

/*!! for debugging !!*/
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
