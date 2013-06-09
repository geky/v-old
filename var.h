#ifndef LIGHT_VAR
#define LIGHT_VAR

#include <stdint.h>
#include <stdlib.h>

#include <math.h>


/* definitions of variable type */

// used definitions

typedef uint32_t hash_t;
typedef unsigned int ref_t;


// variable identifiers

enum {
    TYPE_NULL  = 0x0,
    TYPE_NUM   = 0x1,
    TYPE_STR   = 0x4,
    TYPE_CSTR  = 0x3,
    TYPE_TABLE = 0x5,
    TYPE_FUNC  = 0x6,
    TYPE_BIN   = 0x2,
};


// var_t is struct which contains the data
// for all variable types

typedef union var {
    uint64_t bits;
    double num;

    struct {
        union {
            uint32_t bits;
            ref_t *ref;
        } meta;

        union {
            void *ptr;
            struct table *table;
            struct func *func;
            union var *(*bin)(union var *, int);

            struct {
                uint16_t offset;
                uint16_t length;
            } str;
        } data;
    } var;
} var_t;


/* useful macro definitions */

// for retrieving components
#define var_meta(v) ((v).var.meta.bits)
#define var_data(v) ((v).var.data.ptr)
#define var_type(v) ((v).var.meta.bits & 0x7)
#define var_bits(v) ((v).bits & ~0x7)

#define var_num(v)   (((var_t)((v).bits & ~0x7)).num)
#define var_str(v)   (((char*)(((var_t)((v).bits & ~0x7)).var.meta.ref+1)) + \
                      ((v).var.data.str.offset))
#define var_table(v) ((v).var.data.table)
#define var_func(v)  ((v).var.data.func)
#define var_bin(v)   ((v).var.data.bin)

#define var_ref(v) (*((var_t)((v).bits & ~0x7)).var.meta.ref)
#define var_off(v) ((v).var.data.str.offset)
#define var_len(v) ((v).var.data.str.length)


// macros for creating literal vars in c

#define null_var    ((var_t){0})

#define nan_var     ((var_t){TYPE_NUM | (uint64_t)NAN})
#define inf_var     ((var_t){TYPE_NUM | (uint64_t)INFINITY})
#define num_var(n)  ((var_t){TYPE_NUM | (~0x7 &                   \
                        ((union { double d; uint64_t i; }){n}).i)})

#define str_var(n)  ({ const static union {             \
                        struct {                        \
                            ref_t r;                    \
                            char s[sizeof(n)-1];        \
                        } t;                            \
                        char s[0];                      \
                        uint64_t align;                 \
                       } __attribute__((aligned(8)))    \
                         str = {{ 1, {(n)} }};          \
                                                        \
                       ((var_t){ TYPE_CSTR |            \
                       ((union {                        \
                        struct {                        \
                            const char *s;              \
                            uint16_t off;               \
                            uint16_t len;               \
                        } t;                            \
                        uint64_t i;                     \
                       }){{str.s, 0, sizeof(n)-1}}).i}); }) 

#define func_var(n) ((var_t){TYPE_BIN | (~0x7 &          \
                        ((union { struct {               \
                            uint32_t padd;               \
                            var_t *(*bin)(var_t *, int); \
                        }; uint64_t i; }){0,n}).i)})



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
