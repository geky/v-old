#include "parse.h"

#include "num.h"
#include "str.h"
#include "tbl.h"
#include "fn.h"

#include "vdbg.h"

#include <stdio.h>
#include <assert.h>

//TODO create errors

#define MAX_SPACE 0xfffff

#define SYNTAX_ERROR(p) do { (p)->val = null_var;  \
                             (p)->ended = 1;       \
                             (p)->has_val = 1;     \
                             (p)->has_key = 0;     \
                             return 1;             } while (0)

#define HIDE_FLAG(h) ((var_t) { .meta = (h) })

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
};

extern int (*parsers[128])(struct parse *p);


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
    if (!p->has_key)
        p->key = null_var;

    if (p->has_val) {
        if (p->val.type != TYPE_TBL) {
            p->key = null_var;
            p->val = p->target;
        }
    } else {
        assert(p->target.type == TYPE_TBL);
        p->val = p->target;
    }

    p->has_val = 1;
    p->has_key = 1;
}

static inline void subparse(struct parse *p) {
#ifndef NDEBUG
    const char *s = p->code;
    d_poke_stack();
#endif

    while (!parsers[*p->code & 0x7f](p));
    presolve(p);

#ifndef NDEBUG
    printf("%.*s => ", p->code-s, s);
    var_print(p->val);
#endif
}


static var_t v_add(var_t args) {
    var_t a = tbl_lookup(args.tbl, num_var(0));
    var_t b = tbl_lookup(args.tbl, num_var(1));

    return num_var(var_num(a) + var_num(b));
}

static var_t v_mul(var_t args) {
    var_t a = tbl_lookup(args.tbl, num_var(0));
    var_t b = tbl_lookup(args.tbl, num_var(1));

    return num_var(var_num(a) * var_num(b));
}

var_t parse_single(var_t input) {
    if (input.type != TYPE_STR && input.type != TYPE_CSTR)
        return null_var;

    struct parse p = {
        .scope = tbl_create(0),
        .ops = tbl_create(0).tbl,

        .meta = input.meta,
        .start = var_str(input),
        .code = var_str(input),
        .end = var_str(input) + input.len,
    };

    tbl_assign(p.ops, str_var("+"), fn_var(v_add));
    tbl_assign(p.ops, str_var("*"), fn_var(v_mul));
    tbl_assign(p.ops, str_var(":"), HIDE_FLAG(ASSIGN_OP));
    tbl_assign(p.ops, str_var("="), HIDE_FLAG(ASSIGN_OP));

    subparse(&p);
    return p.val;
}

static int bad_parse(struct parse *p) {
    SYNTAX_ERROR(p);
}

static int space_parse(struct parse *p) {
    const char *s = p->code++;

    while (parsers[*p->code & 0x7f] == space_parse)
        p->code++;

    return p->in_op && (p->code - s > p->op_space);
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

        p->code++;
        return 0;
    }

    uint16_t c = 0;

    while (1) {
        if (++p->code == p->end)
            SYNTAX_ERROR(p);

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
    char quote = *p->code++;
    const char *s = p->code;

    while (*p->code != quote) {
        if (++p->code == p->end)
            SYNTAX_ERROR(p);
    }

    p->val.meta = p->meta;
    p->val.off = (uint16_t)(s - p->start);
    p->val.len = (uint16_t)(p->code++ - s);
    p->has_val = 1;

    return 0;
}

static int num_parse(struct parse *p) {
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
            if (i == '.') {
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

    p->code++; // hit a period
    scale = 1.0;

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
        SYNTAX_ERROR(p);

    const char *s = p->code;

    while (parsers[*p->code & 0x7f] == word_parse ||
           parsers[*p->code & 0x7f] == num_parse)
        p->code++;

    p->key.meta = p->meta;
    p->key.off = (uint16_t)(s - p->start);
    p->key.len = (uint16_t)(p->code - s);
    p->has_key = 1;

    return 0;
}

static int op_parse(struct parse *p) {
    var_t op;

    int in_op = p->in_op;
    unsigned int op_space = p->op_space;

    var_t lval;
    var_t lkey;
    tbl_t *ltbl;
    void (*tbl_op)(tbl_t*, var_t, var_t) = 0;

    {   const char *s = p->code;

        while (parsers[*p->code & 0x7f] == op_parse)
            p->code++;

        op.meta = p->meta;
        op.off = (uint16_t)(s - p->start);
        op.len = (uint16_t)(p->code - s);
    }

    while (1) {
        var_t op_v = tbl_lookup(p->ops, op);

        if (op_v.meta) {
            op = op_v;
            break;
        }

        op.len--;
        p->code--;

        if (op.len == 0)
            SYNTAX_ERROR(p);
    }

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
    } else {
        const char *s = p->code;

        while (parsers[*p->code & 0x7f] == space_parse ||
               parsers[*p->code & 0x7f] == term_parse)
            p->code++;

        p->op_space = p->code - s;
    }

    lkey = p->key;
    ltbl = p->val.tbl;

    presolve(p);
    lval = p->val;

    p->key = null_var;
    p->val = null_var;
    p->has_key = 0;
    p->has_val = 0;
    p->in_op = 1;

    subparse(p);

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

    p->in_op = in_op;
    p->op_space = op_space;

    return 0;
}

static void s_block_parse(struct parse *p) {
    int in_paren = p->in_paren;
    p->in_paren = 1;

    subparse(p);

    p->in_paren = in_paren;
    p->ended = 0;
}

static void m_block_parse(struct parse *p) {
    int in_paren = p->in_paren;
    p->in_paren = 0;

    p->n = 0.0;

    while (!p->ended) {
        p->has_asg = 0;

        subparse(p);

        if (!p->has_asg) {
            tbl_assign(var_tbl(p->target), num_var(p->n), p->val);
            p->n++;
        }

        p->val = null_var;
        p->has_val = 0;
    }

    p->val = p->target;
    p->has_val = 1;
    p->in_paren = in_paren;
    p->ended = 0;
};

static void b_block_parse(struct parse *p) {
}


static int paren_parse(struct parse *p) {
    p->code++;

    if (p->has_key || p->has_val) {
        presolve(p);
        var_t fn = p->val;

        p->val = null_var;
        p->has_val = 0;
        m_block_parse(p);
        // TODO fn call
    } else {
        s_block_parse(p);
    }

    if (*p->code++ != ')')
        SYNTAX_ERROR(p);
    else
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

    if (*p->code++ != ']')
        SYNTAX_ERROR(p);
    else
        return 0;
}

static int bracket_parse(struct parse *p) {
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


/** change this
var_t eval(var_t code, var_t scope) {
    struct parse p = {
        .scope = var_table(scope);
        .ops = ops;

        .str = var_str(code);
        .off = code.off;
        .end = code.end;
    };

    while (!parsers[p.str[p.off]](&p));

    return p.val;
}
**/

/*** remove all below

static var_t single_eval(uint16_t *offp, const char *str, uint16_t end,
                         var_t *key, var_t *scope, int inparen) {

    uint16_t off = *offp;

    var_t acc = null_var; int has_acc = 0;
    var_t key = null_var; int has_key = 0;

    while (off < end) {
        switch (str[off]) {
            case '`':
                comment_parse(&off, str, end);
                break;

            case 'a': case 'A':
            case 'b': case 'B':
            case 'c': case 'C':
            case 'd': case 'D':
            case 'e': case 'E':
            case 'f': case 'F':
            case 'g': case 'G':
            case 'h': case 'H':
            case 'i': case 'I':
            case 'j': case 'J':
            case 'k': case 'K':
            case 'l': case 'L':
            case 'm': case 'M':
            case 'n': case 'N':
            case 'o': case 'O':
            case 'p': case 'P':
            case 'q': case 'Q':
            case 'r': case 'R':
            case 's': case 'S':
            case 't': case 'T':
            case 'u': case 'U':
            case 'v': case 'V':
            case 'w': case 'W':
            case 'x': case 'X':
            case 'y': case 'Y':
            case 'z': case 'Z': case '_':
                key = word_parse(&off, str, end);

                if (has_acc || has_key)
                    return null_var; //TODO return error

                has_key = 1;
                break;

            case '!': case '#': case '$':
            case '%': case '&': case '*':
            case '+': case '-': case '/':
            case '<': case '>': case '?':
            case '@': case '~': case '^':
            case '|': case '\\':
            case '.': case ':': case '=': {
                var_t op_k = op_parse(&off, str, end);
                var_t op_v = null_var;
                int len = op_k.str.len++;

                while (!op_v.v.meta) {
                    op_k.str.len--;
                    op_v = tbl_lookup(ops, op_k);
                }

                switch (op_v.v.meta) {
                    case DOT_OP:
                    case ASSIGN_OP:
                    case SET_OP:
                    case AND_OP:
                    case OR_OP:
                }

                if (len > op_k.str.len) {
                    char ec = str_var(op_k)[op_k.str.len];
                    if (ec == '=' || ec == ':') {
                        
                }
            }

            case '(': {
                if (has_key) {
                    acc = tbl_lookup(has_acc ? acc : scope, key);
                    has_key = 0;
                    has_acc = 1;
                }

                if (has_acc) {
                    switch (acc.v.meta) {
                        case IF_ST:
                        case FOR_ST:
                        case WHILE_ST:
                        case FN_ST:
                    }

                }
            }

            case '[': {
            }

            case '{': {
                
            }

            case '0': case '1': case '2':
            case '3': case '4': case '5':
            case '6': case '7': case '8': case '9': 
                if (has_key || has_acc)
                    return null_var; // TODO return error

                acc = num_parse(&off, str, end);
                has_acc = 1; 

            case '"': case '\'':
                if (has_key || has_acc)
                    return null_var; // TODO return error

                acc = str_parse(&off, str, end);
                has_acc = 1;

            case ' ': case '\t':
                break;

            case '\n':
                if (inparen)
                    break;

                // fallthrough

            case ';': case ',':
                off++;

                // fallthrough

            case ']': case ')': case '}':
                *offp = off;

                if (has_key)
                    acc = tbl_lookup(has_acc ? acc : scope, key);

                return acc;

            default:
                return null_var; //TODO return error
        }
    }
}



// TODO this ---v
var_t light_eval(var_t *v, int n) {
    if (n > 0) {
        if (v[0].type != TYPE_STR)
            return null_var;

        var_inc_ref(v[0]);

        if (n > 1 && v[1].type == TYPE_TBL) {
            var_inc_ref(v[1]);
            return eval(v[0], v[1]);
        }

        return eval(v[0], table_create(0));
    }

    return null_var;
}

***/
