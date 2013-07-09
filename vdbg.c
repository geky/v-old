#include "vdbg.h"
#include "var.h"
#include <math.h>

static volatile int64_t space_used = 0;
static volatile int64_t space_left = 0;

static volatile void *start_stack;
static volatile int64_t max_stack = 0;

void d_add_heap(int64_t s) {
    space_used += s;
    space_left += s;
}

void d_sub_heap(int64_t s) {
    space_left -= s;
}

void d_set_stack(void) {
    int top;
    start_stack = &top;
}

void d_poke_stack(void) {
    int64_t top;
    top = (int16_t)(intptr_t)(&top - max_stack);
    top = (top < 0) ? -top : top;

    if (top > max_stack)
        max_stack = top;
}   

void d_print_space() {
    printf("\nTotal Heap Used: %lld\n", space_used);
    printf(  "Total Heap Left: %lld\n", space_left);
    printf("\nStack Position: %p\n", start_stack);
    printf(  "Total Stack Used: %lld\n", max_stack);
}
