#include "parse.h"
#include "builtin.h"

#include "num.h"
#include "str.h"
#include "tbl.h"
#include "fn.h"

#include "vdbg.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

//TODO create errors

#define MAX_SPACE 0xfffff

#define HIDE_FLAG(h) ((var_t){.meta=(h)})

#define S_ASSERT(a) if (!(a)) {              \
                        p->val = null_var;   \
                        p->ended = 1;        \
                        p->has_val = 1;      \
                        p->has_key = 0;      \
                        return 1;            \
                    }

enum {
    DOT_OP        = ( 1<<3),
    ASSIGN_OP     = ( 2<<3),
    SET_OP        = ( 3<<3),
    AND_OP        = ( 4<<3),
    OR_OP         = ( 5<<3),

    IF_ST         = ( 8<<3),
    ELSE_ST       = ( 9<<3),
    SWITCH_ST     = (10<<3),
    FOR_ST        = (11<<3),
    WHILE_ST      = (12<<3),
    FN_ST         = (13<<3),
    RETURN_ST     = (14<<3),
    BREAK_ST      = (15<<3),
    CONTINUE_ST   = (16<<3),
};

struct parse {
    var_t scope;
    var_t target;
    tbl_t *ops;

    var_t val; 
    var_t key;

    double n;
    unsigned int op_space;
    unsigned int pre_space;

    uint32_t meta;
    const char *start;
    const char *code;
    const char *end;

    int has_val  : 1;
    int has_key  : 1;
    int has_asg  : 1;
    int in_paren : 1;
    int in_op    : 1;
    int ended    : 1;
    int dotted   : 1;
    int skip_to  : 1;
};

extern int (*parsers[128])(struct parse *p);
extern int (*skippers[128])(struct parse *p);


static inline void presolve(struct parse *p) {
    if (p->has_key) {
        if (p->has_val) {
            if (p->val.type == TYPE_TBL)
                p->val = tbl_lookup(p->val.tbl, p->key);
            else
                p->val = null_var;
        } else {
            assert(p->scope.type == TYPE_TBL);
            p->val = tbl_lookup(p->scope.tbl, p->key);
        }
    } else if (!p->has_val) {
        p->val = null_var;
    }

    p->has_val = 1;
    p->has_key = 0;
}

static inline void presolve_key(struct parse *p) {
    assert(p->target.type == TYPE_TBL);

    if (!p->has_key) {
        if (p->has_val) {
            p->key = p->val;
            p->val = p->target;
        } else {
            p->key = null_var;
        }
    } else {
        if (p->has_val) {
            if (p->val.type != TYPE_TBL) {
                presolve(p);
                p->key = p->val;
                p->val = p->target;
            }
        } else {
            p->val = p->target;
        }
    }

    p->has_val = 1;
    p->has_key = 1;
}

static inline void presolve_fn(struct parse *p) {
    assert(p->target.type == TYPE_TBL);

    if (p->has_key) {
        if (p->has_val) {
            if (p->val.type == TYPE_TBL) {
                var_t temp = p->val;
                p->val = tbl_lookup(p->val.tbl, p->key);
                p->key = temp;
            } else {
                p->key = null_var;
                p->val = null_var;
            }
        } else {
            assert(p->scope.type == TYPE_TBL);
            p->val = tbl_lookup(p->scope.tbl, p->key);
            p->key = p->scope;
        }
    } else {
        if (p->has_val) {
            p->key = p->scope;
        } else {
            p->key = null_var;
            p->val = null_var;
        }
    }

    p->has_val = 1;
    p->has_key = 1;
}

static inline void subparse(struct parse *p) {
#ifndef NDEBUG
    const char *s = p->code;
    d_poke_stack();
#endif

    while (!parsers[*p->code & 0x7f](p) &&
           p->code < p->end             );

#ifndef NDEBUG
    printf("|%.*s| => ", p->code-s, s);

    if (p->has_val) {
        var_print(p->val);
    }

    if (p->has_key) {
        printf(".");
        var_print(p->key);
    }

    printf("\n");
#endif
}

static inline void subskip(struct parse *p) {
#ifndef NDEBUG
    const char *s = p->code;
#endif

    while (!skippers[*p->code & 0x7f](p) &&
           p->code < p->end              );

#ifndef NDEBUG
    printf("skipping |%.*s|\n", p->code-s, s);
#endif
}


static int apply_block(struct parse *p) {
    presolve_fn(p);

    S_ASSERT(p->val.type == TYPE_FN || p->val.type == TYPE_BFN);

    var_t args = tbl_create(2);
    tbl_assign(args.tbl, str_var("this"), p->val);

    {   var_t block;
        const char *s = p->code;

        subskip(p);

        block.meta = p->meta;
        block.off = (uint16_t)(s - p->start);
        block.len = (uint16_t)(p->code - s);

        block = fn_create(block, p->scope);
        tbl_assign(args.tbl, str_var("block"), block);
    }

    p->val = fn_call(p->val, args);

    p->key = null_var;
    p->has_key = 0;
    return 1;
}


var_t parse_single(var_t input) {
    if (input.type != TYPE_STR && input.type != TYPE_CSTR)
        return null_var;

    var_t ops = tbl_create(0);

    struct parse p = {
        .scope = tbl_create(0),
        .ops = ops.tbl,

        .meta = input.meta,
        .start = var_str(input) - input.off,
        .code = var_str(input),
        .end = var_str(input) + input.len,
    };

    p.target = p.scope;

    tbl_assign(p.ops, str_var("+"), fn_var(v_add));
    tbl_assign(p.ops, str_var("*"), fn_var(v_mul));
    tbl_assign(p.ops, str_var(":"), HIDE_FLAG(ASSIGN_OP));
    tbl_assign(p.ops, str_var("="), HIDE_FLAG(SET_OP));
    tbl_assign(p.ops, str_var("."), HIDE_FLAG(DOT_OP));
    tbl_assign(p.ops, str_var("||"), HIDE_FLAG(OR_OP));
    tbl_assign(p.ops, str_var("&&"), HIDE_FLAG(AND_OP));

    tbl_assign(p.scope.tbl, str_var("ops"), ops);
    tbl_assign(p.scope.tbl, str_var("if"), fn_var(v_if));

    subparse(&p);
    presolve(&p);
    return p.val;
}

var_t vparse(var_t code, var_t scope) {
    assert(code.type == TYPE_STR || code.type == TYPE_CSTR);
    assert(scope.type == TYPE_TBL);

    struct parse p = {
        .scope = scope,
        .target = scope,

        .meta = code.meta,
        .start = var_str(code) - code.off,
        .code = var_str(code),
        .end = var_str(code) + code.len,
    };

    {   var_t ops = tbl_lookup(scope.tbl, str_var("ops"));
        if (ops.type == TYPE_TBL)
            p.ops = ops.tbl;
        else
            p.ops = tbl_create(0).tbl;
    }

    subparse(&p);
    presolve(&p);
    return p.val;
}


var_t v_fn_split(var_t args) {
    var_t fn1 = tbl_lookup(args.tbl, num_var(0));
    var_t code1 = fn1.fn->code;
    var_t word = tbl_lookup(args.tbl, num_var(1));

    if ( fn1.type != TYPE_FN || 
         (word.type != TYPE_STR && word.type != TYPE_CSTR) )
        return null_var;

    struct parse p = {
        .target = word,

        .meta = code1.meta,
        .start = var_str(code1) - code1.off,
        .code = var_str(code1),
        .end = var_str(code1) + code1.len,

        .skip_to = 1,
    };

    subskip(&p);

    var_t res = tbl_create(1);
    tbl_assign(res.tbl, num_var(0), fn1);

    if (!p.skip_to) {
        var_t code2;

        fn1.fn->code.len = p.code - (p.start + code1.off);

        code2.meta = p.meta;
        code2.off = (uint16_t)(p.code - p.start + word.len);
        code2.len = (uint16_t)(p.end - (p.code + word.len));

        var_t fn2 = fn_create(code2, fn1.fn->closure);
        tbl_assign(res.tbl, num_var(1), fn2);
    }

    return res;
}


static int bad_parse(struct parse *p) {
    S_ASSERT(0);
}

static int space_parse(struct parse *p) {
    const char *s = p->code++;

    while (parsers[*p->code & 0x7f] == space_parse)
        p->code++;

    if (p->in_op && (p->code - s >= p->op_space)) {
        p->code = s;
        return 1;
    } else {
        p->pre_space = p->code - s;
        return 0;
    }
}

static int term_parse(struct parse *p) {
    p->code += !p->in_op;
    return !p->in_paren || p->in_op;
}

static int end_parse(struct parse *p) {
    p->ended = 1;
    return 1;
}

static int comm_parse(struct parse *p) {
    const char *s = p->code;

    while (*p->code == '`')
        p->code++;

    uint16_t n = (uint16_t)(p->code - s);

    if (n == 1) {
        while (*p->code != '`' && *p->code != '\n') {
            if (++p->code == p->end)
                return 0;
        }

        p->code += (*p->code == '`');
        return 0;
    }

    uint16_t c = 0;

    while (1) {
        S_ASSERT(++p->code != p->end);

        if (*p->code == '`') {
            if (++c == n) {
                p->code++;
                return 0;
            }
        } else {
            c = 0;
        }
    }
}

static int str_parse(struct parse *p) {
    if (p->has_key || p->has_val)
        return apply_block(p);

    char quote = *p->code++;
    const char *s = p->code;

    while (*p->code != quote)
        S_ASSERT(++p->code != p->end);

    p->val.meta = p->meta;
    p->val.off = (uint16_t)(s - p->start);
    p->val.len = (uint16_t)(p->code++ - s);
    p->has_val = 1;

    return 0;
}

static int num_parse(struct parse *p) {
    if (p->has_key || p->has_val)
        return apply_block(p);

    unsigned char i; // unsigned for correct underflow
    double scale;
    double sign;

    p->val.num = 0.0;

    struct base {
        double radix;
        double exp;
        char exp_c;
        char exp_C;
    } base = { 10.0, 10.0, 'e', 'E' };


    if (*p->code == '0') {
        p->code++; // discard inital zero

        switch (*p->code) {
            case 'B': case 'b':
                base = (struct base){2.0, 2.0, 'p', 'P'};
                p->code++;
                break;

            case 'O': case 'o':
                base = (struct base){8.0, 2.0, 'p', 'P'};
                p->code++;
                break;

            case 'X': case 'x':
                base = (struct base){16.0, 2.0, 'p', 'P'};
                p->code++;
                break;
        }
    }

    while (1) {
        i = *p->code++;

        if (i == base.exp_c || i == base.exp_C)
            goto exp;

        i -= (i >= 'a') ? ('a' - 10) :
             (i >= 'A') ? ('A' - 10) : '0';

        if (i >= base.radix) {
            if (i == 254) { // '.' applied to the above operation
                break;
            } else {
                p->code--;
                p->val.type = TYPE_NUM;
                p->has_val = 1;
                return 0;
            }
        }

        p->val.num *= base.radix;
        p->val.num += i;
    }

    scale = 1.0; // hit a period

    while (1) {
        i = *p->code++;

        if (i == base.exp_c || i == base.exp_C)
            goto exp;

        i -= (i >= 'a') ? ('a' - 10) :
             (i >= 'A') ? ('A' - 10) : '0';

        if (i >= base.radix) {
            p->code--;
            p->val.type = TYPE_NUM;
            p->has_val = 1;
            return 0;
        }

        scale /= base.radix;
        p->val.num += scale * i;
    }

exp:
    scale = 0.0;
    sign = 1.0;

    if (*p->code == '+') {
        p->code++;
    } else if (*p->code == '-') {
        sign = -1.0;
        p->code++;
    }

    while (1) {
        i = *p->code++;

        i -= (i >= 'a') ? ('a' - 10) :
             (i >= 'A') ? ('A' - 10) : '0';

        if (i >= base.radix) {
            p->val.num *= pow(base.radix, sign*scale);
            p->code--;
            p->val.type = TYPE_NUM;
            p->has_val = 1;
            return 0;
        }

        scale *= base.radix;
        scale += i;
    }
}
    
static int word_parse(struct parse *p) {
    if (p->has_key || p->has_val)
        return apply_block(p);

    const char *s = p->code;

    while (parsers[*p->code & 0x7f] == word_parse ||
           parsers[*p->code & 0x7f] == num_parse)
        p->code++;

    p->key.meta = p->meta;
    p->key.off = (uint16_t)(s - p->start);
    p->key.len = (uint16_t)(p->code - s);
    p->has_key = 1;

    

    return p->dotted;
}

static int op_parse(struct parse *p) {
    if (p->in_op && p->has_val && p->op_space == 0)
        return 1;


    unsigned int op_space = p->op_space;

    var_t op;

    {   const char *s = p->code;

        while (parsers[*p->code & 0x7f] == op_parse)
            p->code++;

        op.meta = p->meta;
        op.off = (uint16_t)(s - p->start);
        op.len = (uint16_t)(p->code - s);

        while (parsers[*p->code & 0x7f] == space_parse ||
               parsers[*p->code & 0x7f] == term_parse)
            p->code++;

        p->op_space = p->code - (s + op.len);

        if (p->op_space < p->pre_space) {
            if (p->has_key || p->has_val) {
                p->code = s - p->pre_space;
                p->op_space = op_space;

                return apply_block(p);

            } else if (*s == '.') { 
                p->code = s;
                p->op_space = op_space;

                return num_parse(p);
            }
        }
    }


    while (1) {
        var_t op_v = tbl_lookup(p->ops, op);

        if (op_v.meta) {
            op = op_v;
            break;
        }

        op.len--;
        p->code--;

        S_ASSERT(op.len > 0);
    }


    void (*tbl_op)(tbl_t*, var_t, var_t) = 0;

    if (op.meta == ASSIGN_OP) {
        tbl_op = tbl_assign;
        presolve_key(p);
        p->op_space = MAX_SPACE;

    } else if (op.meta == SET_OP) {
        tbl_op = tbl_set;
        presolve_key(p);
        p->op_space = MAX_SPACE;

    } else if (*p->code == ':') {
        tbl_op = tbl_assign;
        presolve_key(p);
        p->code++;
        p->op_space = MAX_SPACE;

    } else if (*p->code == '=') {
        tbl_op = tbl_set;
        presolve_key(p);
        p->code++;
        p->op_space = MAX_SPACE;

    }

    int in_op = p->in_op;

    var_t lkey = p->key;
    tbl_t *ltbl = p->val.tbl;

    presolve(p);
    var_t lval = p->val;
    p->in_op = 1;

    if ( (op.meta == AND_OP && p->val.type == TYPE_NULL) ||
         (op.meta == OR_OP  && p->val.type != TYPE_NULL) ) {
        subskip(p);
        goto op_done;
    }

    p->key = null_var;
    p->val = null_var;
    p->has_key = 0;
    p->has_val = 0;
    p->dotted = (op.meta == DOT_OP);

    subparse(p);

    if (op.meta == DOT_OP) {
        presolve_key(p);
        p->val = lval;
    } else {
        presolve(p);
    }

    if (op.type != TYPE_NULL) {
        var_t args = tbl_create(2);
        tbl_assign(args.tbl, num_var(0), lval);
        tbl_assign(args.tbl, num_var(1), p->val);

        p->val = fn_call(op, args);
    }
 
    if (tbl_op) {
        tbl_op(ltbl, lkey, p->val);
        p->has_asg = 1;
    }

op_done:
    p->in_op = in_op;
    p->op_space = op_space;

    return 0;
}

// for blocks with single code block
static void s_block_parse(struct parse *p) {
    int in_paren = p->in_paren;
    int in_op = p->in_op;
    int op_space = p->op_space;
    p->in_paren = 1;
    p->in_op = 0;

    subparse(p);
    presolve(p);

    p->in_paren = in_paren;
    p->in_op = in_op;
    p->op_space = op_space;
    p->ended = 0;
}

// for blocks with multiple independent code blocks
static void m_block_parse(struct parse *p) {
    int in_paren = p->in_paren;
    int in_op = p->in_op;
    int op_space = p->op_space;
    double n = p->n;
    p->in_paren = 0;
    p->in_op = 0;
    p->n = 0.0;

    while (!p->ended) {
        p->has_asg = 0;

        subparse(p);
        presolve(p);

        if (!p->has_asg) {
            tbl_assign(p->target.tbl, num_var(p->n), p->val);
            p->n++;
        }

        p->val = null_var;
        p->has_val = 0;
    }

    p->val = p->target;
    p->has_val = 1;
    p->in_paren = in_paren;
    p->in_op = in_op;
    p->op_space = op_space;
    p->n = n;
    p->ended = 0;
};

// for blocks with multiple dependent code blocks
static void b_block_parse(struct parse *p) {
    int in_paren = p->in_paren;
    int in_op = p->in_op;
    int op_space = p->op_space;
    p->in_paren = 0;
    p->in_op = 0;

    while (!p->ended) {
        p->val = null_var;
        p->has_val = 0;

        subparse(p);
        presolve(p);
    }

    p->in_paren = in_paren;
    p->in_op = in_op;
    p->op_space = op_space;
    p->ended = 0;
}


static int paren_parse(struct parse *p) {
    p->code++;

    if (p->has_key || p->has_val) {
        presolve_fn(p);

        S_ASSERT(p->val.type == TYPE_FN || p->val.type == TYPE_BFN);
        var_t fn = p->val;

        var_t target = p->target;
        p->target = tbl_create(1);
        tbl_assign(p->target.tbl, str_var("this"), p->key);

        p->val = null_var;
        p->has_val = 0;
        p->key = null_var;
        p->has_key = 0;

        m_block_parse(p);

        p->val = fn_call(fn, p->val);
        p->target = target;

    } else {
        s_block_parse(p);
    }

    S_ASSERT(*p->code++ == ')');
    return 0;
}

static int brace_parse(struct parse *p) {
    p->code++;

    if (p->has_key || p->has_val) {
        presolve(p);
        var_t tbl = p->val;

        p->val = null_var;
        p->has_val = 0;
        s_block_parse(p);
        presolve(p);

        p->key = p->val;
        p->has_key = 1;
        p->val = tbl;

    } else {
        var_t target = p->target;
        p->target = tbl_create(0);

        m_block_parse(p);
        p->target = target;
    }

    S_ASSERT(*p->code++ == ']');
    return 0;
}

static int bracket_parse(struct parse *p) {
    if (p->has_key || p->has_val)
        return apply_block(p);

    p->code++;

    var_t scope = p->scope;
    var_t target = p->target;
    p->target = p->scope = tbl_create(1);
    tbl_assign(p->scope.tbl, str_var("super"), scope);

    b_block_parse(p); 
    p->target = target;
    p->scope = scope;

    S_ASSERT(*p->code++ == '}');
    return 0;
}


int (*parsers[128])(struct parse *) = {
    end_parse,      // 0x00     // \0
    bad_parse,      // 0x01     // \1
    bad_parse,      // 0x02     // \2
    bad_parse,      // 0x03     // \3
    bad_parse,      // 0x04     // \4
    bad_parse,      // 0x05     // \5
    bad_parse,      // 0x06     // \6
    bad_parse,      // 0x07     // \a
    bad_parse,      // 0x08     // \b
    space_parse,    // 0x09     // \t
    term_parse,     // 0x0a     // \n
    space_parse,    // 0x0b     // \v
    term_parse,     // 0x0c     // \f
    term_parse,     // 0x0d     // \r
    bad_parse,      // 0x0e     // \xe
    bad_parse,      // 0x0f     // \xf
    bad_parse,      // 0x10     // \x10
    bad_parse,      // 0x11     // \x11
    bad_parse,      // 0x12     // \x12
    bad_parse,      // 0x13     // \x13
    bad_parse,      // 0x14     // \x14
    bad_parse,      // 0x15     // \x15
    bad_parse,      // 0x16     // \x16
    bad_parse,      // 0x17     // \x17
    bad_parse,      // 0x18     // \x18
    bad_parse,      // 0x19     // \x19
    bad_parse,      // 0x1a     // \x1a
    bad_parse,      // 0x1b     // \x1b
    bad_parse,      // 0x1c     // \x1c
    bad_parse,      // 0x1d     // \x1d
    bad_parse,      // 0x1e     // \x1e
    bad_parse,      // 0x1f     // \x1f
    space_parse,    // 0x20     //  
    op_parse,       // 0x21     // !
    str_parse,      // 0x22     // "
    op_parse,       // 0x23     // #
    op_parse,       // 0x24     // $
    op_parse,       // 0x25     // %
    op_parse,       // 0x26     // &
    str_parse,      // 0x27     // '
    paren_parse,    // 0x28     // (
    end_parse,      // 0x29     // )
    op_parse,       // 0x2a     // *
    op_parse,       // 0x2b     // +
    term_parse,     // 0x2c     // ,
    op_parse,       // 0x2d     // -
    op_parse,       // 0x2e     // .
    op_parse,       // 0x2f     // /
    num_parse,      // 0x30     // 0
    num_parse,      // 0x31     // 1
    num_parse,      // 0x32     // 2
    num_parse,      // 0x33     // 3
    num_parse,      // 0x34     // 4
    num_parse,      // 0x35     // 5
    num_parse,      // 0x36     // 6
    num_parse,      // 0x37     // 7
    num_parse,      // 0x38     // 8
    num_parse,      // 0x39     // 9
    op_parse,       // 0x3a     // :
    term_parse,     // 0x3b     // ;
    op_parse,       // 0x3c     // <
    op_parse,       // 0x3d     // =
    op_parse,       // 0x3e     // >
    op_parse,       // 0x3f     // ?
    op_parse,       // 0x40     // @
    word_parse,     // 0x41     // A
    word_parse,     // 0x42     // B
    word_parse,     // 0x43     // C
    word_parse,     // 0x44     // D
    word_parse,     // 0x45     // E
    word_parse,     // 0x46     // F
    word_parse,     // 0x47     // G
    word_parse,     // 0x48     // H
    word_parse,     // 0x49     // I
    word_parse,     // 0x4a     // J
    word_parse,     // 0x4b     // K
    word_parse,     // 0x4c     // L
    word_parse,     // 0x4d     // M
    word_parse,     // 0x4e     // N
    word_parse,     // 0x4f     // O
    word_parse,     // 0x50     // P
    word_parse,     // 0x51     // Q
    word_parse,     // 0x52     // R
    word_parse,     // 0x53     // S
    word_parse,     // 0x54     // T
    word_parse,     // 0x55     // U
    word_parse,     // 0x56     // V
    word_parse,     // 0x57     // W
    word_parse,     // 0x58     // X
    word_parse,     // 0x59     // Y
    word_parse,     // 0x5a     // Z
    brace_parse,    // 0x5b     // [
    op_parse,       /* 0x5c     // \ */
    end_parse,      // 0x5d     // ]
    op_parse,       // 0x5e     // ^
    word_parse,     // 0x5f     // _
    comm_parse,     // 0x60     // `
    word_parse,     // 0x61     // a
    word_parse,     // 0x62     // b
    word_parse,     // 0x63     // c
    word_parse,     // 0x64     // d
    word_parse,     // 0x65     // e
    word_parse,     // 0x66     // f
    word_parse,     // 0x67     // g
    word_parse,     // 0x68     // h
    word_parse,     // 0x69     // i
    word_parse,     // 0x6a     // j
    word_parse,     // 0x6b     // k
    word_parse,     // 0x6c     // l
    word_parse,     // 0x6d     // m
    word_parse,     // 0x6e     // n
    word_parse,     // 0x6f     // o
    word_parse,     // 0x70     // p
    word_parse,     // 0x71     // q
    word_parse,     // 0x72     // r
    word_parse,     // 0x73     // s
    word_parse,     // 0x74     // t
    word_parse,     // 0x75     // u
    word_parse,     // 0x76     // v
    word_parse,     // 0x77     // w
    word_parse,     // 0x78     // x
    word_parse,     // 0x79     // y
    word_parse,     // 0x7a     // z
    bracket_parse,  // 0x7b     // {
    op_parse,       // 0x7c     // |
    end_parse,      // 0x7d     // }
    op_parse,       // 0x7e     // ~
    bad_parse,      // 0x7f     // \x7f
};


static int skip(struct parse *p) {
    p->code++;
    return 0;
}

static int paren_skip(struct parse *p) {
    while (*p->code != ')')
        S_ASSERT(++p->code != p->end);

    p->code++;
    return 0;
}

static int brace_skip(struct parse *p) {
    while (*p->code != ']')
        S_ASSERT(++p->code != p->end);

    p->code++;
    return 0;
}

static int bracket_skip(struct parse *p) {
    while (*p->code != '}')
        S_ASSERT(++p->code != p->end);

    p->code++;
    return 0;
}

static int str_skip(struct parse *p) {
    char quote = *p->code++;
    while (*p->code != quote)
        S_ASSERT(++p->code != p->end);

    p->code++;
    return 0;
}

static int op_skip(struct parse *p) {
    if (p->in_op && p->has_val && p->op_space == 0)
        return 1;

    p->code++;
    return 0;
}

static int word_skip(struct parse *p) {
    if (p->skip_to) {
        assert( p->target.type == TYPE_STR ||
                p->target.type == TYPE_CSTR );

        if ( p->end - p->code > p->target.len &&
             !memcmp(p->code, var_str(p->target), p->target.len) ) {
            p->skip_to = 0;
            return 1;
        }
    }

    p->code++;
    return 0;
}

int (*skippers [128])(struct parse *) = {
    end_parse,      // 0x00     // \0
    skip,           // 0x01     // \1
    skip,           // 0x02     // \2
    skip,           // 0x03     // \3
    skip,           // 0x04     // \4
    skip,           // 0x05     // \5
    skip,           // 0x06     // \6
    skip,           // 0x07     // \a
    skip,           // 0x08     // \b
    space_parse,    // 0x09     // \t
    term_parse,     // 0x0a     // \n
    space_parse,    // 0x0b     // \v
    term_parse,     // 0x0c     // \f
    term_parse,     // 0x0d     // \r
    skip,           // 0x0e     // \xe
    skip,           // 0x0f     // \xf
    skip,           // 0x10     // \x10
    skip,           // 0x11     // \x11
    skip,           // 0x12     // \x12
    skip,           // 0x13     // \x13
    skip,           // 0x14     // \x14
    skip,           // 0x15     // \x15
    skip,           // 0x16     // \x16
    skip,           // 0x17     // \x17
    skip,           // 0x18     // \x18
    skip,           // 0x19     // \x19
    skip,           // 0x1a     // \x1a
    skip,           // 0x1b     // \x1b
    skip,           // 0x1c     // \x1c
    skip,           // 0x1d     // \x1d
    skip,           // 0x1e     // \x1e
    skip,           // 0x1f     // \x1f
    space_parse,    // 0x20     //  
    op_skip,        // 0x21     // !
    str_skip,       // 0x22     // "
    op_skip,        // 0x23     // #
    op_skip,        // 0x24     // $
    op_skip,        // 0x25     // %
    op_skip,        // 0x26     // &
    str_skip,       // 0x27     // '
    paren_skip,     // 0x28     // (
    end_parse,      // 0x29     // )
    op_skip,        // 0x2a     // *
    op_skip,        // 0x2b     // +
    term_parse,     // 0x2c     // ,
    op_skip,        // 0x2d     // -
    op_skip,        // 0x2e     // .
    op_skip,        // 0x2f     // /
    skip,           // 0x30     // 0
    skip,           // 0x31     // 1
    skip,           // 0x32     // 2
    skip,           // 0x33     // 3
    skip,           // 0x34     // 4
    skip,           // 0x35     // 5
    skip,           // 0x36     // 6
    skip,           // 0x37     // 7
    skip,           // 0x38     // 8
    skip,           // 0x39     // 9
    op_skip,        // 0x3a     // :
    term_parse,     // 0x3b     // ;
    op_skip,        // 0x3c     // <
    op_skip,        // 0x3d     // =
    op_skip,        // 0x3e     // >
    op_skip,        // 0x3f     // ?
    skip,           // 0x40     // @
    word_skip,      // 0x41     // A
    word_skip,      // 0x42     // B
    word_skip,      // 0x43     // C
    word_skip,      // 0x44     // D
    word_skip,      // 0x45     // E
    word_skip,      // 0x46     // F
    word_skip,      // 0x47     // G
    word_skip,      // 0x48     // H
    word_skip,      // 0x49     // I
    word_skip,      // 0x4a     // J
    word_skip,      // 0x4b     // K
    word_skip,      // 0x4c     // L
    word_skip,      // 0x4d     // M
    word_skip,      // 0x4e     // N
    word_skip,      // 0x4f     // O
    word_skip,      // 0x50     // P
    word_skip,      // 0x51     // Q
    word_skip,      // 0x52     // R
    word_skip,      // 0x53     // S
    word_skip,      // 0x54     // T
    word_skip,      // 0x55     // U
    word_skip,      // 0x56     // V
    word_skip,      // 0x57     // W
    word_skip,      // 0x58     // X
    word_skip,      // 0x59     // Y
    word_skip,      // 0x5a     // Z
    brace_skip,     // 0x5b     // [
    op_skip,        /* 0x5c     // \ */
    end_parse,      // 0x5d     // ]
    op_skip,        // 0x5e     // ^
    word_skip,      // 0x5f     // _
    comm_parse,     // 0x60     // `
    word_skip,      // 0x61     // a
    word_skip,      // 0x62     // b
    word_skip,      // 0x63     // c
    word_skip,      // 0x64     // d
    word_skip,      // 0x65     // e
    word_skip,      // 0x66     // f
    word_skip,      // 0x67     // g
    word_skip,      // 0x68     // h
    word_skip,      // 0x69     // i
    word_skip,      // 0x6a     // j
    word_skip,      // 0x6b     // k
    word_skip,      // 0x6c     // l
    word_skip,      // 0x6d     // m
    word_skip,      // 0x6e     // n
    word_skip,      // 0x6f     // o
    word_skip,      // 0x70     // p
    word_skip,      // 0x71     // q
    word_skip,      // 0x72     // r
    word_skip,      // 0x73     // s
    word_skip,      // 0x74     // t
    word_skip,      // 0x75     // u
    word_skip,      // 0x76     // v
    word_skip,      // 0x77     // w
    word_skip,      // 0x78     // x
    word_skip,      // 0x79     // y
    word_skip,      // 0x7a     // z
    bracket_skip,   // 0x7b     // {
    op_skip,        // 0x7c     // |
    end_parse,      // 0x7d     // }
    op_skip,        // 0x7e     // ~
    skip,           // 0x7f     // \x7f
};
