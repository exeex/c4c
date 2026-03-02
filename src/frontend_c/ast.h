/* ast.h – AST node definitions for the C frontend
 *
 * Each node is a `Node` struct with a `NodeKind` tag and a union `u` that
 * holds kind-specific data.  Nodes are allocated from an Arena so there is no
 * individual free; arena_free_all() releases everything at once.
 *
 * Type strings follow the same convention as the Python frontend:
 *   - base_type: "int", "char", "void", "unsigned long long", "struct foo", …
 *   - ptr_lvl:   number of pointer stars (0 = not a pointer)
 *   - arr_sz:    >0 for arrays, 0 otherwise
 */
#pragma once
#include "arena.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* ------------------------------------------------------------------ */
/* NodeKind – discriminant for the union                               */
/* ------------------------------------------------------------------ */
typedef enum {
    /* --- expressions --- */
    NK_INT_LIT,       /* integer literal                         */
    NK_FLOAT_LIT,     /* floating-point literal                  */
    NK_STR_LIT,       /* string literal (decoded bytes)          */
    NK_VAR,           /* variable / identifier reference         */
    NK_BINOP,         /* binary operator                         */
    NK_UNARY,         /* unary prefix operator                   */
    NK_CALL,          /* function call (direct or indirect)      */
    NK_INDEX,         /* a[i]                                    */
    NK_MEMBER,        /* a.f  or  a->f                           */
    NK_CAST,          /* (type)expr                              */
    NK_TERNARY,       /* cond ? then : else                      */
    NK_SIZEOF,        /* sizeof(type) or sizeof expr             */
    NK_ASSIGN_EXPR,   /* =  +=  -=  *=  /=  %=  &=  |=  ^=  <<= >>= */
    NK_COMPOUND_LIT,  /* (type){ init }                          */
    NK_STMT_EXPR,     /* ({ stmts… })                            */
    /* --- statements --- */
    NK_DECL,          /* local variable declaration               */
    NK_RETURN,        /* return [expr];                           */
    NK_EXPR_STMT,     /* expression statement                     */
    NK_BLOCK,         /* { stmts… }                              */
    NK_IF,            /* if (cond) then [else els]               */
    NK_WHILE,         /* while (cond) body                       */
    NK_FOR,           /* for (init; cond; post) body             */
    NK_DO_WHILE,      /* do body while (cond);                   */
    NK_SWITCH,        /* switch (expr) body                      */
    NK_CASE,          /* case val: stmt                          */
    NK_DEFAULT,       /* default: stmt                           */
    NK_BREAK,         /* break;                                  */
    NK_CONTINUE,      /* continue;                               */
    NK_GOTO,          /* goto label;                             */
    NK_LABEL,         /* label: stmt                             */
    NK_EMPTY,         /* ;   (empty statement)                   */
    /* --- top-level --- */
    NK_FUNCTION,      /* function definition or declaration       */
    NK_GLOBAL_VAR,    /* global variable                         */
    NK_STRUCT_DEF,    /* struct / union definition               */
    NK_ENUM_DEF,      /* enum definition                         */
    NK_PROGRAM,       /* root of the translation unit            */
} NodeKind;

/* ------------------------------------------------------------------ */
/* Param – function parameter (name + type)                           */
/* ------------------------------------------------------------------ */
typedef struct {
    char *base_type;   /* e.g. "int", "char", "struct Foo" */
    int   ptr_lvl;     /* 0 = not a pointer */
    char *name;        /* may be NULL for anonymous params */
} Param;

/* ------------------------------------------------------------------ */
/* StructField – one field in a struct / union definition             */
/* ------------------------------------------------------------------ */
typedef struct {
    char *base_type;
    int   ptr_lvl;
    char *name;
    int   arr_sz;      /* 0 = not an array */
    int  *extra_dims;  /* for multi-dimensional arrays (may be NULL) */
    int   n_edims;
} StructField;

/* ------------------------------------------------------------------ */
/* Node – tagged union                                                 */
/* ------------------------------------------------------------------ */
typedef struct Node Node;

struct Node {
    NodeKind kind;
    int      line, col;

    union {
        /* NK_INT_LIT */
        struct {
            int64_t val;
            char    sfx[8];   /* "L", "LL", "U", "UL", "ULL", or "" */
        } ival;

        /* NK_FLOAT_LIT */
        struct {
            double val;
        } flt;

        /* NK_STR_LIT */
        struct {
            char  *bytes;
            size_t len;
        } str;

        /* NK_VAR */
        struct {
            char *name;
        } var;

        /* NK_BINOP */
        struct {
            char  op[8];   /* "+", "-", "*", "/", "%", "&", "|", "^",
                              "<<", ">>", "&&", "||",
                              "==", "!=", "<", "<=", ">", ">=" */
            Node *lhs;
            Node *rhs;
        } binop;

        /* NK_UNARY */
        struct {
            char  op[4];   /* "-", "+", "!", "~", "*", "&", "++", "--" */
            Node *expr;
            bool  prefix;  /* true for prefix forms */
        } unary;

        /* NK_CALL */
        struct {
            Node  *func;    /* NK_VAR for direct, any expr for indirect */
            Node **args;
            int    nargs;
        } call;

        /* NK_INDEX */
        struct {
            Node *base;
            Node *idx;
        } index;

        /* NK_MEMBER */
        struct {
            Node *base;
            char *field;
            bool  arrow;   /* true for ->, false for . */
        } mbr;

        /* NK_CAST */
        struct {
            char *base_type;
            int   ptr_lvl;
            Node *expr;
        } cast;

        /* NK_TERNARY */
        struct {
            Node *cond;
            Node *then_e;
            Node *else_e;
        } tern;

        /* NK_SIZEOF */
        struct {
            char *base_type;  /* non-NULL for sizeof(type) */
            int   ptr_lvl;
            Node *expr;       /* non-NULL for sizeof expr  */
        } szof;

        /* NK_ASSIGN_EXPR */
        struct {
            Node *tgt;
            char  op[8];    /* "=", "+=", "-=", … */
            Node *expr;
            bool  postfix;  /* true for x++ / x-- (return old value) */
        } asgn;

        /* NK_COMPOUND_LIT */
        struct {
            char *base_type;
            int   ptr_lvl;
            Node *init;
        } clit;

        /* NK_STMT_EXPR */
        struct {
            Node **stmts;
            int    n;
        } se;

        /* NK_DECL */
        struct {
            char *name;
            char *base_type;
            int   ptr_lvl;
            int   arr_sz;
            int  *extra_dims;
            int   n_edims;
            Node *init;
            bool  is_static;
            bool  is_const;
        } decl;

        /* NK_RETURN / NK_EXPR_STMT */
        struct {
            Node *expr;    /* may be NULL for bare return */
        } expr_wrap;

        /* NK_BLOCK */
        struct {
            Node **stmts;
            int    n;
        } block;

        /* NK_IF */
        struct {
            Node *cond;
            Node *then_s;
            Node *else_s;  /* may be NULL */
        } iff;

        /* NK_WHILE */
        struct {
            Node *cond;
            Node *body;
        } whl;

        /* NK_FOR */
        struct {
            Node *init;    /* may be NULL */
            Node *cond;    /* may be NULL */
            Node *post;    /* may be NULL */
            Node *body;
        } forr;

        /* NK_DO_WHILE */
        struct {
            Node *body;
            Node *cond;
        } dowh;

        /* NK_SWITCH */
        struct {
            Node *expr;
            Node *body;
        } sw;

        /* NK_CASE */
        struct {
            Node *val;
            Node *stmt;
        } cas;

        /* NK_DEFAULT */
        struct {
            Node *stmt;
        } dft;

        /* NK_GOTO */
        struct {
            char *name;
        } got;

        /* NK_LABEL */
        struct {
            char *name;
            Node *stmt;
        } lbl;

        /* NK_FUNCTION */
        struct {
            char   *ret_type;
            int     ret_ptr_lvl;
            char   *name;
            Param  *params;
            int     nparams;
            bool    variadic;
            Node   *body;       /* NULL = forward declaration */
            bool    is_extern;
            bool    is_static;
            bool    is_inline;
        } fn;

        /* NK_GLOBAL_VAR */
        struct {
            char *name;
            char *base_type;
            int   ptr_lvl;
            int   arr_sz;
            Node *init;
            bool  is_extern;
            bool  is_static;
        } gvar;

        /* NK_STRUCT_DEF */
        struct {
            char        *tag;       /* NULL for anonymous */
            bool         is_union;
            StructField *fields;
            int          nfields;
        } sdef;

        /* NK_ENUM_DEF */
        struct {
            char    *name;          /* NULL for anonymous enum */
            char   **cst_names;
            int64_t *cst_vals;
            int      nconsts;
        } edef;

        /* NK_PROGRAM */
        struct {
            Node **items;   /* mix of NK_FUNCTION, NK_GLOBAL_VAR, NK_STRUCT_DEF, NK_ENUM_DEF */
            int    nitems;
        } prog;
    } u;
};

/* ------------------------------------------------------------------ */
/* Convenience constructors (allocate from arena)                     */
/* ------------------------------------------------------------------ */

static inline Node *node_new(Arena *a, NodeKind kind, int line, int col) {
    Node *n = (Node *)arena_alloc(a, sizeof(Node));
    n->kind = kind;
    n->line = line;
    n->col  = col;
    return n;
}

#define NODE(a, kind, line, col) node_new((a), (kind), (line), (col))

/* ------------------------------------------------------------------ */
/* AST dump (for --parse-only debugging)                              */
/* ------------------------------------------------------------------ */
void ast_dump(Node *n, int indent);
