#ifndef VDEBUG
#define VDEBUG

#ifdef NDEBUG
#error vdebug is included in non prototyping build
#endif

#include "tbl.h"
#include "str.h"

#include <stdio.h>
#include <assert.h>

void d_add_heap(int64_t);
void d_sub_heap(int64_t);
void d_set_stack(void);
void d_poke_stack(void);

void d_print_space(void);

#endif
