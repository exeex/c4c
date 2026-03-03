/* parser.c – recursive-descent C parser for the frontend_c bootstrap
 *
 * Grammar coverage (C99 subset + common GNU extensions):
 *   Top-level  : function decls/defs, global vars, struct/union/enum defs,
 *                typedefs, extern declarations.
 *   Statements : if/else, while, for, do-while, switch/case/default,
 *                return, break, continue, goto, labels, blocks, local decls,
 *                expression statements.
 *   Expressions: all operators (Pratt table), function calls, array index,
 *                member access, cast, sizeof, ternary, compound literals,
 *                GCC statement expressions.
 *
 * Type representation: base type string + ptr_lvl integer, matching the
 * Python frontend convention so the IR builder can reuse the same mappings.
 */

#include "common.h"
#include "parser.h"
#include "token.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <inttypes.h>

/* ================================================================
 * Dynamic pointer array helpers
 * ================================================================ */
typedef struct { void **data; int n, cap; } PtrVec;

static void pv_init(PtrVec *v) { v->data = NULL; v->n = v->cap = 0; }

static void pv_push(PtrVec *v, void *p) {
    if (v->n >= v->cap) {
        v->cap = v->cap ? v->cap * 2 : 8;
        v->data = (void **)xrealloc(v->data, (size_t)v->cap * sizeof(void *));
    }
    v->data[v->n++] = p;
}

static void pv_free(PtrVec *v) { free(v->data); v->data = NULL; v->n = v->cap = 0; }

/* Copy a PtrVec into arena memory, write count to *out_n, return pointer */
static void **pv_to_arena(Arena *a, PtrVec *v, int *out_n) {
    *out_n = v->n;
    if (v->n == 0) return NULL;
    void **arr = (void **)arena_alloc(a, (size_t)v->n * sizeof(void *));
    memcpy(arr, v->data, (size_t)v->n * sizeof(void *));
    return arr;
}

/* ================================================================
 * Parser state
 * ================================================================ */
typedef struct {
    TokenVec *tv;
    size_t    pos;
    Arena    *arena;

    /* typedef name set – for cast disambiguation */
    char    **typedefs;
    char    **typedef_bases; /* parallel: underlying base type string, or NULL */
    int       ntypedefs, typedef_cap;

    /* Collect struct/union defs encountered anywhere (incl. inside functions).
     * These are prepended to the program items list so the IR builder can
     * emit type definitions before they are used. */
    PtrVec   struct_defs;
    int      anon_counter;  /* generates _anon_0, _anon_1, ... tags */

    int      braced_switch_depth; /* >0 when inside a braced switch body */
    bool had_error;
} Parser;

/* ================================================================
 * Token access helpers
 * ================================================================ */
static inline Token *cur_tok(Parser *p) {
    return &p->tv->tokens[p->pos < p->tv->count ? p->pos : p->tv->count - 1];
}
static inline Token *peek_tok(Parser *p, size_t n) {
    size_t idx = p->pos + n;
    return &p->tv->tokens[idx < p->tv->count ? idx : p->tv->count - 1];
}
static inline bool at(Parser *p, TokenKind k) { return cur_tok(p)->kind == k; }
static inline bool at2(Parser *p, TokenKind k, size_t n) { return peek_tok(p, n)->kind == k; }

static Token *advance(Parser *p) {
    Token *t = cur_tok(p);
    if (p->pos + 1 < p->tv->count) p->pos++;
    return t;
}

static void expect(Parser *p, TokenKind k) {
    if (!at(p, k)) {
        Token *t = cur_tok(p);
        fprintf(stderr, "parse error at %d:%d: expected %s, got %s ('%s')\n",
                t->line, t->col, tok_kind_str(k), tok_kind_str(t->kind), t->text);
        p->had_error = true;
        return;
    }
    advance(p);
}

static bool try_eat(Parser *p, TokenKind k) {
    if (at(p, k)) { advance(p); return true; }
    return false;
}

static void skip_to_semi(Parser *p) {
    while (!at(p, TK_EOF) && !at(p, TK_SEMI) && !at(p, TK_RBRACE)) advance(p);
    try_eat(p, TK_SEMI);
}

/* ================================================================
 * Typedef tracking
 * ================================================================ */
static void register_typedef(Parser *p, const char *name, const char *base_type) {
    for (int i = 0; i < p->ntypedefs; i++) {
        if (strcmp(p->typedefs[i], name) == 0) {
            if (base_type && !p->typedef_bases[i])
                p->typedef_bases[i] = xstrdup(base_type);
            return;
        }
    }
    if (p->ntypedefs >= p->typedef_cap) {
        p->typedef_cap = p->typedef_cap ? p->typedef_cap * 2 : 32;
        p->typedefs = (char **)xrealloc(p->typedefs,
                                        (size_t)p->typedef_cap * sizeof(char *));
        p->typedef_bases = (char **)xrealloc(p->typedef_bases,
                                              (size_t)p->typedef_cap * sizeof(char *));
    }
    p->typedefs[p->ntypedefs] = xstrdup(name);
    p->typedef_bases[p->ntypedefs] = base_type ? xstrdup(base_type) : NULL;
    p->ntypedefs++;
}

static const char *lookup_typedef_base(Parser *p, const char *name) {
    for (int i = 0; i < p->ntypedefs; i++)
        if (strcmp(p->typedefs[i], name) == 0)
            return p->typedef_bases[i];
    return NULL;
}

static bool is_typedef_name(Parser *p, const char *name) {
    for (int i = 0; i < p->ntypedefs; i++)
        if (strcmp(p->typedefs[i], name) == 0) return true;
    return false;
}

/* ================================================================
 * Type token check
 * ================================================================ */
static bool is_type_tok(Parser *p, Token *t) {
    switch (t->kind) {
        case TK_INT: case TK_CHAR: case TK_VOID: case TK_SHORT:
        case TK_LONG: case TK_SIGNED: case TK_UNSIGNED: case TK_DOUBLE:
        case TK_FLOAT: case TK_CONST: case TK_VOLATILE: case TK_RESTRICT:
        case TK_STATIC: case TK_EXTERN: case TK_INLINE: case TK_REGISTER:
        case TK_STRUCT: case TK_UNION: case TK_ENUM:
            return true;
        case TK_ID:
            return is_typedef_name(p, t->text);
        default:
            return false;
    }
}
static bool is_type_token(Parser *p) { return is_type_tok(p, cur_tok(p)); }

/* ================================================================
 * Forward declarations (mutual recursion)
 * ================================================================ */
static Node *parse_expr(Parser *p);
static Node *parse_assign_expr(Parser *p);
static Node *parse_unary(Parser *p);
static Node *parse_postfix(Parser *p);
static Node *parse_primary(Parser *p);
static Node *parse_stmt(Parser *p);
static Node *parse_block_body(Parser *p);
static char *parse_type_spec(Parser *p,
                              bool *is_static, bool *is_extern,
                              bool *is_inline, bool *is_const);

/* ================================================================
 * Qualifier / star consumption
 * ================================================================ */
static void skip_qualifiers(Parser *p) {
    while (at(p, TK_CONST) || at(p, TK_VOLATILE) || at(p, TK_RESTRICT))
        advance(p);
}

static int consume_stars(Parser *p) {
    int lvl = 0;
    for (;;) {
        skip_qualifiers(p);
        if (at(p, TK_STAR)) { advance(p); lvl++; }
        else break;
    }
    skip_qualifiers(p);
    return lvl;
}

/* Skip balanced paren group starting at '(' */
static void skip_paren_group(Parser *p) {
    if (!at(p, TK_LPAREN)) return;
    int d = 1; advance(p);
    while (!at(p, TK_EOF) && d > 0) {
        if      (at(p, TK_LPAREN)) { d++; advance(p); }
        else if (at(p, TK_RPAREN)) { d--; advance(p); }
        else advance(p);
    }
}

static void skip_brace_group(Parser *p) {
    if (!at(p, TK_LBRACE)) return;
    int d = 1; advance(p);
    while (!at(p, TK_EOF) && d > 0) {
        if      (at(p, TK_LBRACE)) { d++; advance(p); }
        else if (at(p, TK_RBRACE)) { d--; advance(p); }
        else advance(p);
    }
}

/* ================================================================
 * Struct body parsing
 * ================================================================ */
static void parse_struct_body(Parser *p, Node *sd) {
    expect(p, TK_LBRACE);
    PtrVec fv; pv_init(&fv);

    while (!at(p, TK_RBRACE) && !at(p, TK_EOF)) {
        if (at(p, TK_SEMI)) { advance(p); continue; }  /* stray semicolons */

        char *ftype = parse_type_spec(p, NULL, NULL, NULL, NULL);
        int   fptr  = consume_stars(p);

        /* Anonymous inner struct/union that was fully parsed by parse_type_spec */
        if (at(p, TK_SEMI)) {
            advance(p);
            /* Add as unnamed field so it appears in the LLVM struct type and
             * its sub-fields can be inlined for member-name lookup. */
            if (ftype && (strncmp(ftype,"struct ",7)==0 || strncmp(ftype,"union ",6)==0)) {
                StructField *sf = (StructField *)arena_alloc(p->arena, sizeof(StructField));
                memset(sf, 0, sizeof(*sf));
                sf->base_type = ftype;
                sf->name      = arena_strdup(p->arena, ""); /* empty = anonymous */
                pv_push(&fv, sf);
            }
            continue;
        }

        do {
            /* Function pointer field: (* [qualifiers] name)(...) */
            if (at(p, TK_LPAREN) && at2(p, TK_STAR, 1)) {
                advance(p); advance(p);
                /* Skip nullability/qualifier identifiers (_Nullable, _Nonnull, etc.)
                   i.e., any ID not immediately followed by ')' */
                while (at(p, TK_ID) && !at2(p, TK_RPAREN, 1))
                    advance(p);
                char *fname = NULL;
                if (at(p, TK_ID)) { fname = arena_strdup(p->arena, cur_tok(p)->text); advance(p); }
                expect(p, TK_RPAREN);
                skip_paren_group(p);   /* parameter list */
                if (fname) {
                    StructField *sf = (StructField *)arena_alloc(p->arena, sizeof(StructField));
                    memset(sf, 0, sizeof(*sf));
                    sf->base_type = arena_strdup(p->arena, "ptr");
                    sf->name      = fname;
                    pv_push(&fv, sf);
                }
                continue;
            }

            int dptr = fptr + consume_stars(p);
            char *dname = NULL;
            if (at(p, TK_ID)) { dname = arena_strdup(p->arena, cur_tok(p)->text); advance(p); }

            int arr_sz = 0;
            int edims[16]; int n_ed = 0;
            while (at(p, TK_LBRACK)) {
                advance(p);
                int sz = 0;
                if (!at(p, TK_RBRACK)) {
                    Node *se = parse_assign_expr(p);
                    if (se && se->kind == NK_INT_LIT) sz = (int)se->u.ival.val;
                    else sz = 1;
                }
                expect(p, TK_RBRACK);
                if (arr_sz == 0) arr_sz = (sz > 0) ? sz : 1;
                else if (n_ed < 16) edims[n_ed++] = (sz > 0) ? sz : 1;
            }
            /* Bitfield: ignore */
            if (at(p, TK_COLON)) { advance(p); parse_expr(p); }

            if (dname) {
                StructField *sf = (StructField *)arena_alloc(p->arena, sizeof(StructField));
                memset(sf, 0, sizeof(*sf));
                sf->base_type = ftype;
                sf->ptr_lvl   = dptr;
                sf->name      = dname;
                sf->arr_sz    = arr_sz;
                if (n_ed > 0) {
                    sf->extra_dims = (int *)arena_alloc(p->arena, (size_t)n_ed * sizeof(int));
                    memcpy(sf->extra_dims, edims, (size_t)n_ed * sizeof(int));
                    sf->n_edims = n_ed;
                }
                pv_push(&fv, sf);
            }
        } while (try_eat(p, TK_COMMA));

        expect(p, TK_SEMI);
    }
    expect(p, TK_RBRACE);

    sd->u.sdef.nfields = fv.n;
    if (fv.n > 0) {
        sd->u.sdef.fields = (StructField *)arena_alloc(p->arena,
                             (size_t)fv.n * sizeof(StructField));
        for (int i = 0; i < fv.n; i++)
            sd->u.sdef.fields[i] = *(StructField *)fv.data[i];
    }
    pv_free(&fv);
}

/* ================================================================
 * Constant expression evaluator for enum values
 * ================================================================ */
static int64_t eval_enum_const(Node *n, char **pnames, int64_t *pvals, int np) {
    if (!n) return 0;
    switch (n->kind) {
    case NK_INT_LIT: return n->u.ival.val;
    case NK_VAR:
        for (int i = 0; i < np; i++)
            if (strcmp(pnames[i], n->u.var.name) == 0)
                return pvals[i];
        return 0;
    case NK_UNARY:
        if (strcmp(n->u.unary.op,"-")==0)
            return -eval_enum_const(n->u.unary.expr, pnames, pvals, np);
        if (strcmp(n->u.unary.op,"~")==0)
            return ~eval_enum_const(n->u.unary.expr, pnames, pvals, np);
        return eval_enum_const(n->u.unary.expr, pnames, pvals, np);
    case NK_BINOP: {
        int64_t l = eval_enum_const(n->u.binop.lhs, pnames, pvals, np);
        int64_t r = eval_enum_const(n->u.binop.rhs, pnames, pvals, np);
        if (strcmp(n->u.binop.op,"+")==0)  return l + r;
        if (strcmp(n->u.binop.op,"-")==0)  return l - r;
        if (strcmp(n->u.binop.op,"*")==0)  return l * r;
        if (strcmp(n->u.binop.op,"/")==0)  return r ? l / r : 0;
        if (strcmp(n->u.binop.op,"<<")==0) return l << r;
        if (strcmp(n->u.binop.op,">>")==0) return l >> r;
        if (strcmp(n->u.binop.op,"|")==0)  return l | r;
        if (strcmp(n->u.binop.op,"&")==0)  return l & r;
        if (strcmp(n->u.binop.op,"^")==0)  return l ^ r;
        return l;
    }
    case NK_CAST:
        return eval_enum_const(n->u.cast.expr, pnames, pvals, np);
    default: return 0;
    }
}

/* ================================================================
 * Type specifier parser
 * ================================================================ */
static char *parse_type_spec(Parser *p,
                              bool *is_static, bool *is_extern,
                              bool *is_inline, bool *is_const) {
    bool st = false, ex = false, inl = false, con = false;
    bool unsign = false, sign = false, shrt = false;
    bool lng = false, llong = false;
    char base[128] = "";

    for (;;) {
        if (at(p, TK_STATIC))   { st  = true; advance(p); continue; }
        if (at(p, TK_EXTERN))   { ex  = true; advance(p); continue; }
        if (at(p, TK_INLINE))   { inl = true; advance(p); continue; }
        if (at(p, TK_REGISTER)) { advance(p); continue; }
        if (at(p, TK_CONST))    { con = true; advance(p); continue; }
        if (at(p, TK_VOLATILE)) { advance(p); continue; }
        if (at(p, TK_RESTRICT)) { advance(p); continue; }

        if (at(p, TK_UNSIGNED)) { unsign = true; advance(p); continue; }
        if (at(p, TK_SIGNED))   { sign   = true; advance(p); continue; }
        if (at(p, TK_SHORT))    { shrt   = true; advance(p); continue; }
        if (at(p, TK_LONG)) {
            advance(p);
            if (at(p, TK_LONG)) { llong = true; advance(p); }
            else                 { lng   = true; }
            continue;
        }

        /* Primitive types */
        if (at(p, TK_INT))    { strncpy(base, "int",    sizeof(base)-1); advance(p); break; }
        if (at(p, TK_CHAR))   { strncpy(base, "char",   sizeof(base)-1); advance(p); break; }
        if (at(p, TK_VOID))   { strncpy(base, "void",   sizeof(base)-1); advance(p); break; }
        if (at(p, TK_DOUBLE)) { strncpy(base, "double", sizeof(base)-1); advance(p); break; }
        if (at(p, TK_FLOAT))  { strncpy(base, "float",  sizeof(base)-1); advance(p); break; }

        /* struct / union */
        if (at(p, TK_STRUCT) || at(p, TK_UNION)) {
            bool is_union = at(p, TK_UNION);
            advance(p);
            char tag[128] = "";
            if (at(p, TK_ID)) { strncpy(tag, cur_tok(p)->text, sizeof(tag)-1); advance(p); }
            if (at(p, TK_LBRACE)) {
                /* Inline struct/union definition: generate synthetic tag if anonymous */
                if (!tag[0]) {
                    snprintf(tag, sizeof(tag), "_anon_%d", p->anon_counter++);
                }
                Node *sd = node_new(p->arena, NK_STRUCT_DEF,
                                    cur_tok(p)->line, cur_tok(p)->col);
                sd->u.sdef.is_union = is_union;
                sd->u.sdef.tag = arena_strdup(p->arena, tag);
                parse_struct_body(p, sd);
                /* Collect for top-level emission (deduplication via tag) */
                bool already = false;
                for (int i = 0; i < p->struct_defs.n; i++) {
                    Node *prev = (Node *)p->struct_defs.data[i];
                    if (prev->u.sdef.tag && strcmp(prev->u.sdef.tag, tag) == 0) {
                        already = true; break;
                    }
                }
                if (!already) pv_push(&p->struct_defs, sd);
            }
            if (tag[0])
                snprintf(base, sizeof(base), "%s %s",
                         is_union ? "union" : "struct", tag);
            else
                snprintf(base, sizeof(base), "%s", is_union ? "union" : "struct");
            break;
        }

        /* enum */
        if (at(p, TK_ENUM)) {
            advance(p);
            char enum_tag[128] = "";
            if (at(p, TK_ID)) {
                strncpy(enum_tag, cur_tok(p)->text, sizeof(enum_tag)-1);
                advance(p);
                /* Register enum tag as typedef so enum E variables are recognised */
                register_typedef(p, enum_tag, NULL);
            }
            if (at(p, TK_LBRACE)) {
                /* Parse enum body and create NK_ENUM_DEF */
                Node *ed = node_new(p->arena, NK_ENUM_DEF,
                                    cur_tok(p)->line, cur_tok(p)->col);
                ed->u.edef.name = enum_tag[0] ? arena_strdup(p->arena, enum_tag) : NULL;
                advance(p); /* eat { */
                /* Use small stack for intra-enum constant references */
                char  *pnames[256]; int64_t pvals[256]; int np = 0;
                PtrVec cnames; pv_init(&cnames);
                int64_t *cvals_arr =
                    (int64_t *)xmalloc(256 * sizeof(int64_t));
                int nc = 0;
                int64_t next_val = 0;
                while (!at(p, TK_RBRACE) && !at(p, TK_EOF)) {
                    if (!at(p, TK_ID)) { advance(p); continue; }
                    char *cname = arena_strdup(p->arena, cur_tok(p)->text);
                    advance(p);
                    int64_t val = next_val;
                    if (try_eat(p, TK_ASSIGN)) {
                        /* Simple constant-expr evaluation */
                        Node *vn = parse_assign_expr(p);
                        /* Recursive helper: evaluate constant node */
                        #define EVAL_CONST(nn) eval_enum_const(nn, pnames, pvals, np)
                        val = eval_enum_const(vn, pnames, pvals, np);
                        #undef EVAL_CONST
                    }
                    next_val = val + 1;
                    if (np < 256) { pnames[np] = cname; pvals[np] = val; np++; }
                    if (nc < 256) { pv_push(&cnames, cname); cvals_arr[nc++] = val; }
                    try_eat(p, TK_COMMA);
                }
                expect(p, TK_RBRACE);
                ed->u.edef.nconsts = nc;
                if (nc > 0) {
                    ed->u.edef.cst_names = (char **)arena_alloc(p->arena,
                                           (size_t)nc * sizeof(char *));
                    ed->u.edef.cst_vals  = (int64_t *)arena_alloc(p->arena,
                                           (size_t)nc * sizeof(int64_t));
                    for (int i = 0; i < nc; i++) {
                        ed->u.edef.cst_names[i] = (char *)cnames.data[i];
                        ed->u.edef.cst_vals[i]  = cvals_arr[i];
                    }
                }
                free(cvals_arr);
                pv_free(&cnames);
                pv_push(&p->struct_defs, ed);
            }
            strncpy(base, "int", sizeof(base)-1);
            break;
        }

        /* typedef name */
        if (at(p, TK_ID) && is_typedef_name(p, cur_tok(p)->text)) {
            const char *tname = cur_tok(p)->text;
            /* Expand struct/union typedefs so the IR builder can resolve them */
            const char *expanded = lookup_typedef_base(p, tname);
            if (expanded)
                strncpy(base, expanded, sizeof(base)-1);
            else
                strncpy(base, tname, sizeof(base)-1);
            advance(p);
            break;
        }

        /* __va_list / va_list */
        if (at(p, TK_ID) &&
            (strcmp(cur_tok(p)->text, "__va_list") == 0 ||
             strcmp(cur_tok(p)->text, "va_list")   == 0 ||
             strcmp(cur_tok(p)->text, "__builtin_va_list") == 0)) {
            strncpy(base, "__va_list", sizeof(base)-1);
            advance(p);
            break;
        }

        /* Unknown __xxx identifier: lenient fallback */
        if (at(p, TK_ID) && cur_tok(p)->text[0] == '_' && cur_tok(p)->text[1] == '_') {
            strncpy(base, "int", sizeof(base)-1);
            advance(p);
            break;
        }

        break;   /* no more type tokens */
    }

    /* Infer missing base */
    if (base[0] == '\0') strncpy(base, "int", sizeof(base)-1);

    /* Build multi-word type string */
    char full[256];
    if (sign && strcmp(base, "char") == 0) {
        snprintf(full, sizeof(full), "signed char");
    } else if (unsign && llong) {
        snprintf(full, sizeof(full), "unsigned long long");
    } else if (unsign && lng) {
        snprintf(full, sizeof(full), "unsigned long");
    } else if (unsign && shrt) {
        snprintf(full, sizeof(full), "unsigned short");
    } else if (unsign && strcmp(base,"char")==0) {
        snprintf(full, sizeof(full), "unsigned char");
    } else if (unsign) {
        snprintf(full, sizeof(full), "unsigned");
    } else if (llong) {
        snprintf(full, sizeof(full), "long long");
    } else if (lng && strcmp(base,"double")==0) {
        snprintf(full, sizeof(full), "double");
    } else if (lng) {
        snprintf(full, sizeof(full), "long");
    } else if (shrt) {
        snprintf(full, sizeof(full), "short");
    } else {
        snprintf(full, sizeof(full), "%s", base);
    }

    if (is_static) *is_static = st;
    if (is_extern) *is_extern = ex;
    if (is_inline) *is_inline = inl;
    if (is_const)  *is_const  = con;

    return arena_strdup(p->arena, full);
}

/* ================================================================
 * Number literal
 * ================================================================ */
static Node *parse_num_lit(Parser *p) {
    Token *t = advance(p);
    Node *n  = node_new(p->arena, NK_INT_LIT, t->line, t->col);

    const char *text = t->text;
    size_t tlen = strlen(text);

    /* Strip suffix: u U l L f F */
    const char *end = text + tlen;
    while (end > text) {
        char c = *(end - 1);
        if (c=='u'||c=='U'||c=='l'||c=='L'||c=='f'||c=='F') end--;
        else break;
    }
    int sfx_len = (int)(text + tlen - end);

    bool is_float = false;
    for (const char *q = text; q < end; q++) {
        if (*q=='.' || *q=='e' || *q=='E') { is_float = true; break; }
    }

    if (is_float) {
        n->kind = NK_FLOAT_LIT;
        n->u.flt.val = strtod(text, NULL);
    } else {
        char num_only[64];
        size_t nl = (size_t)(end - text);
        if (nl >= sizeof(num_only)) nl = sizeof(num_only)-1;
        memcpy(num_only, text, nl); num_only[nl] = '\0';
        n->u.ival.val = (int64_t)strtoull(num_only, NULL, 0);

        /* Classify suffix */
        const char *sfx = end;
        int has_u = 0, ll = 0;
        for (int i = 0; i < sfx_len; i++) {
            char c = sfx[i];
            if (c=='u'||c=='U') has_u++;
            if (c=='l'||c=='L') ll++;
        }
        if      (has_u && ll>=2) strncpy(n->u.ival.sfx, "ULL", 4);
        else if (has_u && ll==1) strncpy(n->u.ival.sfx, "UL",  3);
        else if (has_u)          strncpy(n->u.ival.sfx, "U",   2);
        else if (ll>=2)          strncpy(n->u.ival.sfx, "LL",  3);
        else if (ll==1)          strncpy(n->u.ival.sfx, "L",   2);
    }
    return n;
}

/* ================================================================
 * Primary expression
 * ================================================================ */
static Node *parse_primary(Parser *p) {
    Token *t = cur_tok(p);
    int line = t->line, col = t->col;

    /* Numeric literal */
    if (at(p, TK_NUM)) return parse_num_lit(p);

    /* String literal */
    if (at(p, TK_STR)) {
        advance(p);
        Node *n = node_new(p->arena, NK_STR_LIT, line, col);
        n->u.str.bytes = (char *)arena_alloc(p->arena, t->text_len + 1);
        memcpy(n->u.str.bytes, t->text, t->text_len + 1);
        n->u.str.len   = t->text_len;
        return n;
    }

    /* Identifier */
    if (at(p, TK_ID)) {
        advance(p);
        Node *n = node_new(p->arena, NK_VAR, line, col);
        n->u.var.name = arena_strdup(p->arena, t->text);
        return n;
    }

    /* sizeof */
    if (at(p, TK_SIZEOF)) {
        advance(p);
        Node *n = node_new(p->arena, NK_SIZEOF, line, col);
        if (at(p, TK_LPAREN)) {
            advance(p);   /* eat '(' */
            if (is_type_token(p)) {
                char *st = parse_type_spec(p, NULL, NULL, NULL, NULL);
                int   sp = consume_stars(p);
                /* handle sizeof(T[N]) */
                int arr_sz = 0;
                if (at(p, TK_LBRACK)) {
                    advance(p);
                    if (at(p, TK_NUM)) { arr_sz = (int)strtol(cur_tok(p)->text, NULL, 0); advance(p); }
                    else if (!at(p, TK_RBRACK)) parse_expr(p);
                    expect(p, TK_RBRACK);
                }
                expect(p, TK_RPAREN);
                n->u.szof.base_type = st;
                n->u.szof.ptr_lvl   = sp;
                n->u.szof.expr      = NULL;
                (void)arr_sz;
            } else {
                Node *inner = parse_expr(p);
                expect(p, TK_RPAREN);
                n->u.szof.base_type = NULL;
                n->u.szof.expr      = inner;
            }
        } else {
            n->u.szof.base_type = NULL;
            n->u.szof.expr      = parse_unary(p);
        }
        return n;
    }

    /* _Generic: skip, return 0 */
    if (at(p, TK_ID) && strcmp(cur_tok(p)->text, "_Generic") == 0) {
        advance(p);
        skip_paren_group(p);
        Node *n = node_new(p->arena, NK_INT_LIT, line, col);
        n->u.ival.val = 0;
        return n;
    }

    /* Parenthesised expression, cast, or GCC stmt expr */
    if (at(p, TK_LPAREN)) {
        advance(p);   /* eat '(' */

        /* GCC statement expression: ({ stmts }) */
        if (at(p, TK_LBRACE)) {
            Node *body = parse_block_body(p);
            expect(p, TK_RPAREN);
            Node *n = node_new(p->arena, NK_STMT_EXPR, line, col);
            n->u.se.stmts = body->u.block.stmts;
            n->u.se.n     = body->u.block.n;
            return n;
        }

        /* Check for cast: if the token after '(' is a type specifier, try cast */
        if (is_type_token(p)) {
            size_t saved = p->pos;
            char *ctype = parse_type_spec(p, NULL, NULL, NULL, NULL);
            int   cptr  = consume_stars(p);

            /* Function pointer cast: (type (*)(params)) */
            if (at(p, TK_LPAREN) && at2(p, TK_STAR, 1)) {
                advance(p); advance(p);     /* eat ( * */
                if (at(p, TK_RPAREN)) advance(p);  /* eat ) */
                skip_paren_group(p);        /* param list */
                /* must see ')' to close the cast */
                if (at(p, TK_RPAREN)) {
                    advance(p);
                    Node *expr = parse_unary(p);
                    Node *n = node_new(p->arena, NK_CAST, line, col);
                    n->u.cast.base_type = ctype;
                    n->u.cast.ptr_lvl   = cptr;
                    n->u.cast.expr      = expr;
                    return n;
                }
                /* Restore – not a cast */
                p->pos = saved;
                goto parse_paren_expr;
            }

            if (at(p, TK_RPAREN)) {
                /* It IS a cast (type)expr */
                advance(p);   /* eat ')' */
                /* Compound literal? (type){ ... } */
                if (at(p, TK_LBRACE)) {
                    skip_brace_group(p);
                    Node *n = node_new(p->arena, NK_COMPOUND_LIT, line, col);
                    n->u.clit.base_type = ctype;
                    n->u.clit.ptr_lvl   = cptr;
                    n->u.clit.init      = NULL;
                    return n;
                }
                Node *expr = parse_unary(p);
                Node *n = node_new(p->arena, NK_CAST, line, col);
                n->u.cast.base_type = ctype;
                n->u.cast.ptr_lvl   = cptr;
                n->u.cast.expr      = expr;
                return n;
            }
            /* Restore – it was not a cast after all */
            p->pos = saved;
        }

        parse_paren_expr:;
        Node *inner = parse_expr(p);
        expect(p, TK_RPAREN);
        return inner;
    }

    /* Error: unexpected token */
    fprintf(stderr, "parse error at %d:%d: unexpected %s ('%s') in expression\n",
            t->line, t->col, tok_kind_str(t->kind), t->text);
    p->had_error = true;
    advance(p);   /* consume to avoid infinite loop */
    Node *dummy = node_new(p->arena, NK_INT_LIT, line, col);
    dummy->u.ival.val = 0;
    return dummy;
}

/* ================================================================
 * Postfix expression  (call, index, member, postfix ++ --)
 * ================================================================ */
static Node *parse_postfix(Parser *p) {
    Node *base = parse_primary(p);

    for (;;) {
        int line = cur_tok(p)->line, col = cur_tok(p)->col;

        /* Function call */
        if (at(p, TK_LPAREN)) {
            advance(p);
            PtrVec args; pv_init(&args);
            while (!at(p, TK_RPAREN) && !at(p, TK_EOF)) {
                pv_push(&args, parse_assign_expr(p));
                if (!try_eat(p, TK_COMMA)) break;
            }
            expect(p, TK_RPAREN);
            Node *n = node_new(p->arena, NK_CALL, line, col);
            n->u.call.func  = base;
            n->u.call.args  = (Node **)pv_to_arena(p->arena, &args, &n->u.call.nargs);
            pv_free(&args);
            base = n;
            continue;
        }

        /* Array subscript */
        if (at(p, TK_LBRACK)) {
            advance(p);
            Node *idx = parse_expr(p);
            expect(p, TK_RBRACK);
            Node *n = node_new(p->arena, NK_INDEX, line, col);
            n->u.index.base = base;
            n->u.index.idx  = idx;
            base = n;
            continue;
        }

        /* Member access: . and -> */
        if (at(p, TK_DOT) || at(p, TK_ARROW)) {
            bool arrow = at(p, TK_ARROW);
            advance(p);
            char *field = NULL;
            if (at(p, TK_ID)) { field = arena_strdup(p->arena, cur_tok(p)->text); advance(p); }
            else { field = arena_strdup(p->arena, cur_tok(p)->text); advance(p); }
            Node *n = node_new(p->arena, NK_MEMBER, line, col);
            n->u.mbr.base  = base;
            n->u.mbr.field = field;
            n->u.mbr.arrow = arrow;
            base = n;
            continue;
        }

        /* Postfix ++ -- */
        if (at(p, TK_PLUSPLUS) || at(p, TK_MINUSMINUS)) {
            const char *op = at(p, TK_PLUSPLUS) ? "++" : "--";
            advance(p);
            Node *n = node_new(p->arena, NK_UNARY, line, col);
            strncpy(n->u.unary.op, op, sizeof(n->u.unary.op)-1);
            n->u.unary.expr   = base;
            n->u.unary.prefix = false;
            base = n;
            continue;
        }

        break;
    }
    return base;
}

/* ================================================================
 * Unary expression  (prefix)
 * ================================================================ */
static Node *parse_unary(Parser *p) {
    int line = cur_tok(p)->line, col = cur_tok(p)->col;

    /* Prefix ++ -- */
    if (at(p, TK_PLUSPLUS) || at(p, TK_MINUSMINUS)) {
        const char *op = at(p, TK_PLUSPLUS) ? "++" : "--";
        advance(p);
        Node *e = parse_unary(p);
        Node *n = node_new(p->arena, NK_UNARY, line, col);
        strncpy(n->u.unary.op, op, sizeof(n->u.unary.op)-1);
        n->u.unary.expr   = e;
        n->u.unary.prefix = true;
        return n;
    }

    static const struct { TokenKind tk; const char op; } unops[] = {
        {TK_MINUS, '-'}, {TK_PLUS, '+'}, {TK_BANG, '!'},
        {TK_TILDE, '~'}, {TK_STAR, '*'}, {TK_AMP,  '&'},
        {TK_EOF,    0 }
    };
    for (int i = 0; unops[i].op; i++) {
        if (at(p, unops[i].tk)) {
            char opch = unops[i].op;
            advance(p);
            Node *e = parse_unary(p);
            Node *n = node_new(p->arena, NK_UNARY, line, col);
            n->u.unary.op[0] = opch; n->u.unary.op[1] = '\0';
            n->u.unary.expr   = e;
            n->u.unary.prefix = true;
            return n;
        }
    }

    return parse_postfix(p);
}

/* ================================================================
 * Binary expression – precedence climbing
 * ================================================================ */
static int binop_prec(TokenKind tk) {
    switch (tk) {
        case TK_PIPEPIPE:  return 4;
        case TK_AMPAMP:    return 5;
        case TK_PIPE:      return 6;
        case TK_CARET:     return 7;
        case TK_AMP:       return 8;
        case TK_EQ: case TK_NE: return 9;
        case TK_LT: case TK_LE: case TK_GT: case TK_GE: return 10;
        case TK_LSHIFT: case TK_RSHIFT: return 11;
        case TK_PLUS: case TK_MINUS:    return 12;
        case TK_STAR: case TK_SLASH: case TK_PERCENT: return 13;
        default: return -1;
    }
}

static const char *binop_str(TokenKind tk) {
    switch (tk) {
        case TK_PIPEPIPE: return "||"; case TK_AMPAMP:  return "&&";
        case TK_PIPE:     return "|";  case TK_CARET:   return "^";
        case TK_AMP:      return "&";  case TK_EQ:      return "==";
        case TK_NE:       return "!="; case TK_LT:      return "<";
        case TK_LE:       return "<="; case TK_GT:      return ">";
        case TK_GE:       return ">="; case TK_LSHIFT:  return "<<";
        case TK_RSHIFT:   return ">>"; case TK_PLUS:    return "+";
        case TK_MINUS:    return "-";  case TK_STAR:    return "*";
        case TK_SLASH:    return "/";  case TK_PERCENT: return "%";
        default: return "?";
    }
}

static Node *parse_binop(Parser *p, int min_prec) {
    Node *lhs = parse_unary(p);
    for (;;) {
        int prec = binop_prec(cur_tok(p)->kind);
        if (prec < min_prec) break;
        int line = cur_tok(p)->line, col = cur_tok(p)->col;
        const char *op = binop_str(cur_tok(p)->kind);
        advance(p);
        Node *rhs = parse_binop(p, prec + 1);
        Node *n = node_new(p->arena, NK_BINOP, line, col);
        strncpy(n->u.binop.op, op, sizeof(n->u.binop.op)-1);
        n->u.binop.lhs = lhs;
        n->u.binop.rhs = rhs;
        lhs = n;
    }
    return lhs;
}

/* Ternary: cond ? then : else  (right-assoc, prec 3) */
static Node *parse_ternary(Parser *p) {
    Node *cond = parse_binop(p, 4);
    if (!at(p, TK_QUESTION)) return cond;
    int line = cur_tok(p)->line, col = cur_tok(p)->col;
    advance(p);
    Node *then_e = parse_expr(p);
    expect(p, TK_COLON);
    Node *else_e = parse_ternary(p);
    Node *n = node_new(p->arena, NK_TERNARY, line, col);
    n->u.tern.cond   = cond;
    n->u.tern.then_e = then_e;
    n->u.tern.else_e = else_e;
    return n;
}

/* Assignment expression (right-assoc, prec 2) */
static Node *parse_assign_expr(Parser *p) {
    Node *lhs = parse_ternary(p);
    static const struct { TokenKind tk; const char *op; } aops[] = {
        {TK_ASSIGN,    "="},   {TK_PLUSEQ,    "+="},  {TK_MINUSEQ,   "-="},
        {TK_STAREQ,    "*="},  {TK_SLASHEQ,   "/="},  {TK_PERCENTEQ, "%="},
        {TK_AMPEQ,    "&="},   {TK_PIPEEQ,    "|="},  {TK_CARETEQ,   "^="},
        {TK_LSHIFTEQ, "<<="},  {TK_RSHIFTEQ,  ">>="},
        {TK_EOF, NULL}
    };
    for (int i = 0; aops[i].op; i++) {
        if (at(p, aops[i].tk)) {
            int line = cur_tok(p)->line, col = cur_tok(p)->col;
            advance(p);
            Node *rhs = parse_assign_expr(p);
            Node *n = node_new(p->arena, NK_ASSIGN_EXPR, line, col);
            n->u.asgn.tgt  = lhs;
            strncpy(n->u.asgn.op, aops[i].op, sizeof(n->u.asgn.op)-1);
            n->u.asgn.expr = rhs;
            return n;
        }
    }
    return lhs;
}

static Node *parse_expr(Parser *p) { return parse_assign_expr(p); }

/* ================================================================
 * Statement parsing
 * ================================================================ */

static Node *parse_block_body(Parser *p) {
    int line = cur_tok(p)->line, col = cur_tok(p)->col;
    expect(p, TK_LBRACE);
    PtrVec sv; pv_init(&sv);
    while (!at(p, TK_RBRACE) && !at(p, TK_EOF)) {
        Node *s = parse_stmt(p);
        if (s) pv_push(&sv, s);
    }
    expect(p, TK_RBRACE);
    Node *n = node_new(p->arena, NK_BLOCK, line, col);
    n->u.block.stmts = (Node **)pv_to_arena(p->arena, &sv, &n->u.block.n);
    pv_free(&sv);
    return n;
}

static Node *parse_local_decl(Parser *p) {
    int line = cur_tok(p)->line, col = cur_tok(p)->col;
    bool is_st = false, is_con = false;
    char *btype = parse_type_spec(p, &is_st, NULL, NULL, &is_con);
    int ptr_lvl = consume_stars(p);

    /* Function pointer local: type (*name)(params) */
    if (at(p, TK_LPAREN) && at2(p, TK_STAR, 1)) {
        advance(p); advance(p);
        char *name = NULL;
        if (at(p, TK_ID)) { name = arena_strdup(p->arena, cur_tok(p)->text); advance(p); }
        expect(p, TK_RPAREN);
        skip_paren_group(p);   /* param list */
        Node *init_node = NULL;
        if (try_eat(p, TK_ASSIGN)) init_node = parse_assign_expr(p);
        /* multiple declarators */
        while (try_eat(p, TK_COMMA)) {
            consume_stars(p);
            if (at(p, TK_LPAREN) && at2(p, TK_STAR,1)) {
                advance(p); advance(p);
                if (at(p,TK_ID)) advance(p);
                expect(p, TK_RPAREN);
                skip_paren_group(p);
            } else {
                if (at(p, TK_ID)) advance(p);
            }
            if (try_eat(p, TK_ASSIGN)) parse_assign_expr(p);
        }
        expect(p, TK_SEMI);
        Node *n = node_new(p->arena, NK_DECL, line, col);
        n->u.decl.name      = name;
        n->u.decl.base_type = arena_strdup(p->arena, "ptr");
        n->u.decl.is_static = is_st;
        n->u.decl.is_const  = is_con;
        n->u.decl.init      = init_node;
        return n;
    }

    /* Bare struct/union/typedef declaration with no variable */
    if (at(p, TK_SEMI)) { advance(p); return NULL; }

    char *name = NULL;
    if (at(p, TK_ID)) { name = arena_strdup(p->arena, cur_tok(p)->text); advance(p); }

    int arr_sz = 0; int edims[16]; int n_ed = 0;
    while (at(p, TK_LBRACK)) {
        advance(p);
        int sz = 0;
        if (!at(p, TK_RBRACK)) {
            Node *se = parse_assign_expr(p);
            if (se && se->kind == NK_INT_LIT) sz = (int)se->u.ival.val;
            else sz = 1;
        }
        expect(p, TK_RBRACK);
        if (arr_sz == 0) arr_sz = (sz > 0) ? sz : -1;
        else if (n_ed < 16) edims[n_ed++] = (sz > 0) ? sz : 1;
    }

    Node *init_node = NULL;
    Node **init_list = NULL;
    char **init_fields = NULL;
    int    nInits = 0;
    if (try_eat(p, TK_ASSIGN)) {
        if (at(p, TK_LBRACE)) {
            /* Parse brace initializer list and store for codegen */
            advance(p); /* eat { */
            PtrVec vals; pv_init(&vals);
            PtrVec flds; pv_init(&flds);
            while (!at(p, TK_RBRACE) && !at(p, TK_EOF)) {
                char *field_name = NULL;
                /* Designated field: .field = expr */
                if (at(p, TK_DOT)) {
                    advance(p);
                    if (at(p, TK_ID)) {
                        field_name = arena_strdup(p->arena, cur_tok(p)->text);
                        advance(p);
                    }
                    expect(p, TK_ASSIGN);
                }
                /* Designated index: [N] = expr */
                else if (at(p, TK_LBRACK)) {
                    advance(p);
                    Node *idx = parse_assign_expr(p);
                    expect(p, TK_RBRACK);
                    char fbuf[32] = "[?]";
                    if (idx && idx->kind == NK_INT_LIT)
                        snprintf(fbuf, sizeof(fbuf), "[%" PRId64 "]", idx->u.ival.val);
                    field_name = arena_strdup(p->arena, fbuf);
                    expect(p, TK_ASSIGN);
                }
                Node *val;
                if (at(p, TK_LBRACE)) { skip_brace_group(p); val = NULL; }
                else val = parse_assign_expr(p);
                pv_push(&vals, val);
                pv_push(&flds, (void *)field_name);
                if (!try_eat(p, TK_COMMA)) break;
                if (at(p, TK_RBRACE)) break; /* trailing comma */
            }
            expect(p, TK_RBRACE);
            int nn = 0, nf = 0;
            init_list   = (Node **)pv_to_arena(p->arena, &vals, &nn);
            nInits = nn;
            init_fields = (char **)pv_to_arena(p->arena, &flds, &nf);
            pv_free(&vals); pv_free(&flds);
        } else {
            init_node = parse_assign_expr(p);
        }
    }

    /* Collect first declarator */
    PtrVec decls; pv_init(&decls);
    {
        Node *n = node_new(p->arena, NK_DECL, line, col);
        n->u.decl.name        = name;
        n->u.decl.base_type   = btype;
        n->u.decl.ptr_lvl     = ptr_lvl;
        n->u.decl.arr_sz      = arr_sz;
        n->u.decl.is_static   = is_st;
        n->u.decl.is_const    = is_con;
        n->u.decl.init        = init_node;
        n->u.decl.init_list   = init_list;
        n->u.decl.init_fields = init_fields;
        n->u.decl.nInits      = nInits;
        if (n_ed > 0) {
            n->u.decl.extra_dims = (int *)arena_alloc(p->arena,
                                        (size_t)n_ed * sizeof(int));
            memcpy(n->u.decl.extra_dims, edims, (size_t)n_ed * sizeof(int));
            n->u.decl.n_edims = n_ed;
        }
        pv_push(&decls, n);
    }

    /* Additional declarators: int a = 0, *b, c[3]; — emit one NK_DECL per name */
    while (try_eat(p, TK_COMMA)) {
        int dline = cur_tok(p)->line, dcol = cur_tok(p)->col;
        int dptr = consume_stars(p);
        char *dname = NULL;
        int darr = 0;
        /* Pointer-to-array: (*name)[size] — e.g. char arr[2][4], (*p)[4] */
        if (dptr == 0 && at(p, TK_LPAREN) && at2(p, TK_STAR, 1)) {
            advance(p); /* eat ( */
            advance(p); /* eat * */
            dptr = 1;
            if (at(p, TK_ID)) { dname = arena_strdup(p->arena, cur_tok(p)->text); advance(p); }
            expect(p, TK_RPAREN);
            while (at(p, TK_LBRACK)) {
                advance(p);
                int sz = 0;
                if (!at(p, TK_RBRACK)) {
                    Node *se = parse_assign_expr(p);
                    if (se && se->kind == NK_INT_LIT) sz = (int)se->u.ival.val;
                    else sz = 1;
                }
                expect(p, TK_RBRACK);
                if (darr == 0) darr = (sz > 0) ? sz : -1;
            }
        } else {
            if (at(p, TK_ID)) { dname = arena_strdup(p->arena, cur_tok(p)->text); advance(p); }
            while (at(p, TK_LBRACK)) {
                advance(p);
                int sz = 0;
                if (!at(p, TK_RBRACK)) {
                    Node *se = parse_assign_expr(p);
                    if (se && se->kind == NK_INT_LIT) sz = (int)se->u.ival.val;
                    else sz = 1;
                }
                try_eat(p, TK_RBRACK);
                if (darr == 0) darr = (sz > 0) ? sz : -1;
            }
        }
        Node *dinit = NULL;
        if (try_eat(p, TK_ASSIGN)) {
            if (at(p, TK_LBRACE)) skip_brace_group(p);
            else dinit = parse_assign_expr(p);
        }
        Node *dn = node_new(p->arena, NK_DECL, dline, dcol);
        dn->u.decl.name      = dname;
        dn->u.decl.base_type = btype;
        dn->u.decl.ptr_lvl   = dptr;
        dn->u.decl.arr_sz    = darr;
        dn->u.decl.is_static = is_st;
        dn->u.decl.is_const  = is_con;
        dn->u.decl.init      = dinit;
        pv_push(&decls, dn);
    }

    expect(p, TK_SEMI);

    /* Return single decl or a block of decls */
    if (decls.n == 1) {
        Node *result = (Node *)decls.data[0];
        pv_free(&decls);
        return result;
    }
    Node *blk = node_new(p->arena, NK_BLOCK, line, col);
    blk->u.block.stmts = (Node **)pv_to_arena(p->arena, &decls, &blk->u.block.n);
    pv_free(&decls);
    return blk;
}

static Node *parse_stmt(Parser *p) {
    Token *t = cur_tok(p);
    int line = t->line, col = t->col;

    if (at(p, TK_SEMI))   { advance(p); return node_new(p->arena, NK_EMPTY, line, col); }
    if (at(p, TK_LBRACE)) return parse_block_body(p);

    if (at(p, TK_IF)) {
        advance(p);
        expect(p, TK_LPAREN);
        Node *cond = parse_expr(p);
        expect(p, TK_RPAREN);
        Node *then_s = parse_stmt(p);
        Node *else_s = NULL;
        if (try_eat(p, TK_ELSE)) else_s = parse_stmt(p);
        Node *n = node_new(p->arena, NK_IF, line, col);
        n->u.iff.cond = cond; n->u.iff.then_s = then_s; n->u.iff.else_s = else_s;
        return n;
    }

    if (at(p, TK_WHILE)) {
        advance(p);
        expect(p, TK_LPAREN); Node *cond = parse_expr(p); expect(p, TK_RPAREN);
        Node *body = parse_stmt(p);
        Node *n = node_new(p->arena, NK_WHILE, line, col);
        n->u.whl.cond = cond; n->u.whl.body = body;
        return n;
    }

    if (at(p, TK_FOR)) {
        advance(p);
        expect(p, TK_LPAREN);
        Node *init = NULL, *cond = NULL, *post = NULL;
        if (!at(p, TK_SEMI)) {
            if (is_type_token(p)) init = parse_local_decl(p);   /* eats ';' */
            else { init = parse_expr(p); expect(p, TK_SEMI); }
        } else advance(p);
        if (!at(p, TK_SEMI)) cond = parse_expr(p);
        expect(p, TK_SEMI);
        if (!at(p, TK_RPAREN)) post = parse_expr(p);
        expect(p, TK_RPAREN);
        Node *body = parse_stmt(p);
        Node *n = node_new(p->arena, NK_FOR, line, col);
        n->u.forr.init=init; n->u.forr.cond=cond; n->u.forr.post=post; n->u.forr.body=body;
        return n;
    }

    if (at(p, TK_DO)) {
        advance(p);
        Node *body = parse_stmt(p);
        expect(p, TK_WHILE); expect(p, TK_LPAREN);
        Node *cond = parse_expr(p);
        expect(p, TK_RPAREN); expect(p, TK_SEMI);
        Node *n = node_new(p->arena, NK_DO_WHILE, line, col);
        n->u.dowh.body = body; n->u.dowh.cond = cond;
        return n;
    }

    if (at(p, TK_SWITCH)) {
        advance(p);
        expect(p, TK_LPAREN); Node *expr = parse_expr(p); expect(p, TK_RPAREN);
        Node *body;
        if (at(p, TK_LBRACE)) {
            p->braced_switch_depth++;
            body = parse_block_body(p);
            p->braced_switch_depth--;
        } else {
            /* C allows braceless switch body: parse a single statement */
            body = parse_stmt(p);
            if (!body) {
                body = node_new(p->arena, NK_BLOCK, line, col);
                body->u.block.n = 0;
            } else if (body->kind != NK_BLOCK) {
                Node *blk = node_new(p->arena, NK_BLOCK, line, col);
                PtrVec sv2; pv_init(&sv2); pv_push(&sv2, body);
                blk->u.block.stmts = (Node **)pv_to_arena(p->arena, &sv2, &blk->u.block.n);
                pv_free(&sv2);
                body = blk;
            }
        }
        Node *n = node_new(p->arena, NK_SWITCH, line, col);
        n->u.sw.expr = expr; n->u.sw.body = body;
        return n;
    }

    if (at(p, TK_CASE)) {
        advance(p);
        Node *val = parse_expr(p);
        if (at(p, TK_ELLIPSIS)) { advance(p); parse_expr(p); } /* range */
        expect(p, TK_COLON);
        PtrVec sv; pv_init(&sv);
        if (p->braced_switch_depth > 0) {
            /* Inside braced switch: collect until next case/default/rbrace */
            while (!at(p, TK_CASE) && !at(p, TK_DEFAULT) &&
                   !at(p, TK_RBRACE) && !at(p, TK_EOF)) {
                Node *s = parse_stmt(p);
                if (s) pv_push(&sv, s);
            }
        } else {
            /* Braceless switch body: exactly one sub-statement */
            Node *s = parse_stmt(p);
            if (s) pv_push(&sv, s);
        }
        Node *body;
        if (sv.n == 1) { body = (Node *)sv.data[0]; }
        else {
            body = node_new(p->arena, NK_BLOCK, line, col);
            body->u.block.stmts = (Node **)pv_to_arena(p->arena, &sv, &body->u.block.n);
        }
        pv_free(&sv);
        Node *n = node_new(p->arena, NK_CASE, line, col);
        n->u.cas.val = val; n->u.cas.stmt = body;
        return n;
    }

    if (at(p, TK_DEFAULT)) {
        advance(p); expect(p, TK_COLON);
        PtrVec sv; pv_init(&sv);
        if (p->braced_switch_depth > 0) {
            while (!at(p, TK_CASE) && !at(p, TK_DEFAULT) &&
                   !at(p, TK_RBRACE) && !at(p, TK_EOF)) {
                Node *s = parse_stmt(p);
                if (s) pv_push(&sv, s);
            }
        } else {
            Node *s = parse_stmt(p);
            if (s) pv_push(&sv, s);
        }
        Node *body;
        if (sv.n == 1) { body = (Node *)sv.data[0]; }
        else {
            body = node_new(p->arena, NK_BLOCK, line, col);
            body->u.block.stmts = (Node **)pv_to_arena(p->arena, &sv, &body->u.block.n);
        }
        pv_free(&sv);
        Node *n = node_new(p->arena, NK_DEFAULT, line, col);
        n->u.dft.stmt = body;
        return n;
    }

    if (at(p, TK_RETURN)) {
        advance(p);
        Node *expr = NULL;
        if (!at(p, TK_SEMI)) expr = parse_expr(p);
        expect(p, TK_SEMI);
        Node *n = node_new(p->arena, NK_RETURN, line, col);
        n->u.expr_wrap.expr = expr;
        return n;
    }

    if (at(p, TK_BREAK))    { advance(p); expect(p, TK_SEMI); return node_new(p->arena, NK_BREAK,    line, col); }
    if (at(p, TK_CONTINUE)) { advance(p); expect(p, TK_SEMI); return node_new(p->arena, NK_CONTINUE, line, col); }

    if (at(p, TK_GOTO)) {
        advance(p);
        char *lname = NULL;
        if (at(p, TK_ID)) { lname = arena_strdup(p->arena, cur_tok(p)->text); advance(p); }
        expect(p, TK_SEMI);
        Node *n = node_new(p->arena, NK_GOTO, line, col);
        n->u.got.name = lname;
        return n;
    }

    /* Label: id ':' */
    if (at(p, TK_ID) && at2(p, TK_COLON, 1)) {
        char *lname = arena_strdup(p->arena, cur_tok(p)->text);
        advance(p); advance(p);   /* id : */
        Node *stmt = parse_stmt(p);
        Node *n = node_new(p->arena, NK_LABEL, line, col);
        n->u.lbl.name = lname; n->u.lbl.stmt = stmt;
        return n;
    }

    /* Local declaration */
    if (is_type_token(p)) return parse_local_decl(p);

    /* Expression statement */
    Node *expr = parse_expr(p);
    expect(p, TK_SEMI);
    Node *n = node_new(p->arena, NK_EXPR_STMT, line, col);
    n->u.expr_wrap.expr = expr;
    return n;
}

/* ================================================================
 * Parameter list
 * ================================================================ */
static void parse_param_list(Parser *p, Node *fn_node) {
    expect(p, TK_LPAREN);
    PtrVec pv; pv_init(&pv);
    bool variadic = false;

    while (!at(p, TK_RPAREN) && !at(p, TK_EOF)) {
        if (at(p, TK_ELLIPSIS)) { variadic = true; advance(p); break; }
        if (at(p, TK_VOID) && at2(p, TK_RPAREN, 1)) { advance(p); break; }

        Param *pm = (Param *)arena_alloc(p->arena, sizeof(Param));
        memset(pm, 0, sizeof(*pm));
        pm->base_type = parse_type_spec(p, NULL, NULL, NULL, NULL);
        pm->ptr_lvl   = consume_stars(p);

        /* Function pointer or const-pointer param: type (* [qualifiers] name)(params)
         * Also handles int (* const x) — const/volatile pointer declarations */
        if (at(p, TK_LPAREN) && at2(p, TK_STAR, 1)) {
            advance(p); advance(p);
            /* Skip any qualifiers (const, volatile, restrict, or identifier qualifiers) */
            while ((at(p, TK_ID) || at(p, TK_CONST) || at(p, TK_VOLATILE) || at(p, TK_RESTRICT))
                   && !at2(p, TK_RPAREN, 1)) advance(p);
            if (at(p, TK_ID)) { pm->name = arena_strdup(p->arena, cur_tok(p)->text); advance(p); }
            expect(p, TK_RPAREN);
            /* If followed by (, this is truly a function pointer; otherwise const pointer */
            if (at(p, TK_LPAREN)) skip_paren_group(p);
            pm->base_type = arena_strdup(p->arena, "ptr");
            pm->ptr_lvl   = 0;
        } else {
            if (at(p, TK_ID)) { pm->name = arena_strdup(p->arena, cur_tok(p)->text); advance(p); }
            /* Array decay: name[] → pointer.
             * Skip optional qualifiers like 'static', 'const', 'volatile',
             * 'restrict' that may appear inside brackets per C99 §6.7.5.3. */
            if (at(p, TK_LBRACK)) {
                advance(p);
                /* Skip any qualifier keywords: static, const, volatile, restrict */
                while (at(p, TK_STATIC) || at(p, TK_CONST) || at(p, TK_VOLATILE) ||
                       at(p, TK_RESTRICT))
                    advance(p);
                if (!at(p, TK_RBRACK)) parse_assign_expr(p);
                expect(p, TK_RBRACK);
                pm->ptr_lvl++;
            }
        }

        pv_push(&pv, pm);
        if (!try_eat(p, TK_COMMA)) break;
    }
    expect(p, TK_RPAREN);

    /* Copy Param values into a contiguous arena-allocated array.
     * pv stores Param* pointers; we need a Param[] array. */
    fn_node->u.fn.nparams = pv.n;
    if (pv.n > 0) {
        fn_node->u.fn.params = (Param *)arena_alloc(p->arena,
                                                     (size_t)pv.n * sizeof(Param));
        for (int i = 0; i < pv.n; i++)
            fn_node->u.fn.params[i] = *(Param *)pv.data[i];
    }
    fn_node->u.fn.variadic = variadic;
    pv_free(&pv);
}

/* ================================================================
 * Top-level item
 * ================================================================ */
static void parse_typedef(Parser *p) {
    advance(p);   /* eat 'typedef' */
    char *btype = parse_type_spec(p, NULL, NULL, NULL, NULL);
    int ptr_lvl = consume_stars(p);
    (void)ptr_lvl;

    /* Function pointer typedef: typedef ret (*Name)(params); */
    if (at(p, TK_LPAREN) && at2(p, TK_STAR, 1)) {
        advance(p); advance(p);
        if (at(p, TK_ID)) {
            /* Register with "ptr" base so lltype_of resolves it to ptr */
            register_typedef(p, cur_tok(p)->text, "ptr");
            advance(p);
        }
        expect(p, TK_RPAREN);
        skip_paren_group(p);
        expect(p, TK_SEMI);
        return;
    }
    /* Simple typedef: typedef type [*] Name; */
    if (at(p, TK_ID)) {
        /* Store base type only for struct/union typedefs (to allow expansion later) */
        const char *base = (btype && (strncmp(btype,"struct ",7)==0 ||
                                      strncmp(btype,"union ",6)==0)) ? btype : NULL;
        register_typedef(p, cur_tok(p)->text, base);
        advance(p);
        while (at(p, TK_LBRACK)) {
            advance(p);
            if (!at(p, TK_RBRACK)) parse_assign_expr(p);
            expect(p, TK_RBRACK);
        }
    }
    expect(p, TK_SEMI);
}

static Node *finish_global_var(Parser *p, char *btype, int ptr_lvl, char *name,
                                bool is_ext, bool is_st, int line, int col) {
    int arr_sz = 0;
    while (at(p, TK_LBRACK)) {
        advance(p);
        int sz = 0;
        if (!at(p, TK_RBRACK)) {
            Node *se = parse_assign_expr(p);
            if (se && se->kind == NK_INT_LIT) sz = (int)se->u.ival.val;
            else sz = 1;
        }
        expect(p, TK_RBRACK);
        if (arr_sz == 0) arr_sz = (sz > 0) ? sz : -1;
    }
    Node *init_node = NULL;
    Node **init_list = NULL;
    char **init_fields = NULL;
    int    nInits = 0;
    if (try_eat(p, TK_ASSIGN)) {
        if (at(p, TK_LBRACE)) {
            /* Parse struct/array brace initializer */
            advance(p); /* eat { */
            PtrVec vals; pv_init(&vals);
            PtrVec flds; pv_init(&flds);
            while (!at(p, TK_RBRACE) && !at(p, TK_EOF)) {
                char *field_name = NULL;
                /* Designated initializer: .field = expr */
                if (at(p, TK_DOT)) {
                    advance(p);
                    if (at(p, TK_ID)) {
                        field_name = arena_strdup(p->arena, cur_tok(p)->text);
                        advance(p);
                    }
                    expect(p, TK_ASSIGN);
                }
                /* Designated index initializer: [N] = expr  (C99 §6.7.9) */
                else if (at(p, TK_LBRACK)) {
                    advance(p);
                    Node *idx = parse_assign_expr(p);
                    expect(p, TK_RBRACK);
                    /* Build a field name like "[N]" for tracking */
                    char fbuf[32] = "[?]";
                    if (idx && idx->kind == NK_INT_LIT)
                        snprintf(fbuf, sizeof(fbuf), "[%" PRId64 "]", idx->u.ival.val);
                    field_name = arena_strdup(p->arena, fbuf);
                    expect(p, TK_ASSIGN);
                }
                Node *val;
                if (at(p, TK_LBRACE)) {
                    /* Nested brace initializer: record it recursively */
                    advance(p);
                    PtrVec nvals; pv_init(&nvals);
                    PtrVec nflds; pv_init(&nflds);
                    while (!at(p, TK_RBRACE) && !at(p, TK_EOF)) {
                        char *nf = NULL;
                        if (at(p, TK_DOT)) {
                            advance(p);
                            if (at(p, TK_ID)) { nf = arena_strdup(p->arena, cur_tok(p)->text); advance(p); }
                            expect(p, TK_ASSIGN);
                        }
                        Node *nv;
                        if (at(p, TK_LBRACE)) {
                            /* Deeper nesting (e.g., sub-array initializer): recurse */
                            advance(p);
                            PtrVec n2vals; pv_init(&n2vals);
                            PtrVec n2flds; pv_init(&n2flds);
                            while (!at(p, TK_RBRACE) && !at(p, TK_EOF)) {
                                char *n2f = NULL;
                                if (at(p, TK_DOT)) {
                                    advance(p);
                                    if (at(p, TK_ID)) { n2f = arena_strdup(p->arena, cur_tok(p)->text); advance(p); }
                                    expect(p, TK_ASSIGN);
                                }
                                Node *n2v;
                                if (at(p, TK_LBRACE)) {
                                    /* 4th+ level: skip for now */
                                    skip_brace_group(p);
                                    n2v = node_new(p->arena, NK_INT_LIT, 0, 0);
                                } else {
                                    n2v = parse_assign_expr(p);
                                }
                                pv_push(&n2vals, n2v);
                                pv_push(&n2flds, (void *)n2f);
                                if (!try_eat(p, TK_COMMA)) break;
                                if (at(p, TK_RBRACE)) break;
                            }
                            expect(p, TK_RBRACE);
                            nv = node_new(p->arena, NK_GLOBAL_VAR, 0, 0);
                            nv->u.gvar.name = NULL;
                            nv->u.gvar.base_type = NULL;
                            int nn3 = 0;
                            nv->u.gvar.init_list   = (Node **)pv_to_arena(p->arena, &n2vals, &nn3);
                            nv->u.gvar.init_fields = (char **)pv_to_arena(p->arena, &n2flds, &nn3);
                            nv->u.gvar.nInits      = n2vals.n;
                            pv_free(&n2vals); pv_free(&n2flds);
                        } else {
                            nv = parse_assign_expr(p);
                        }
                        pv_push(&nvals, nv);
                        pv_push(&nflds, (void *)nf);
                        if (!try_eat(p, TK_COMMA)) break;
                        if (at(p, TK_RBRACE)) break; /* trailing comma */
                    }
                    expect(p, TK_RBRACE);
                    /* Build a compound-lit-like placeholder node storing the nested list */
                    val = node_new(p->arena, NK_GLOBAL_VAR, 0, 0);
                    val->u.gvar.name = NULL;
                    val->u.gvar.base_type = NULL;
                    int nn2 = 0;
                    val->u.gvar.init_list   = (Node **)pv_to_arena(p->arena, &nvals, &nn2);
                    val->u.gvar.init_fields = (char **)pv_to_arena(p->arena, &nflds, &nn2);
                    val->u.gvar.nInits      = nvals.n;
                    pv_free(&nvals); pv_free(&nflds);
                } else {
                    val = parse_assign_expr(p);
                }
                pv_push(&vals, val);
                pv_push(&flds, (void *)field_name);
                if (!try_eat(p, TK_COMMA)) break;
                /* Allow trailing comma */
                if (at(p, TK_RBRACE)) break;
            }
            expect(p, TK_RBRACE);
            int n2 = 0, n3 = 0;
            init_list   = (Node **)pv_to_arena(p->arena, &vals, &n2);
            nInits = n2;
            init_fields = (char **)pv_to_arena(p->arena, &flds, &n3);
            pv_free(&vals); pv_free(&flds);
        } else {
            init_node = parse_assign_expr(p);
        }
    }
    /* Collect first + extra declarators: int x, *y, z[3]; — one NK_GLOBAL_VAR per name */
    PtrVec gvars; pv_init(&gvars);
    {
        Node *n = node_new(p->arena, NK_GLOBAL_VAR, line, col);
        n->u.gvar.name        = name;
        n->u.gvar.base_type   = btype;
        n->u.gvar.ptr_lvl     = ptr_lvl;
        n->u.gvar.arr_sz      = arr_sz;
        n->u.gvar.init        = init_node;
        n->u.gvar.is_extern   = is_ext;
        n->u.gvar.is_static   = is_st;
        n->u.gvar.init_list   = init_list;
        n->u.gvar.init_fields = init_fields;
        n->u.gvar.nInits      = nInits;
        pv_push(&gvars, n);
    }
    /* extra declarators */
    while (try_eat(p, TK_COMMA)) {
        int dline = cur_tok(p)->line, dcol = cur_tok(p)->col;
        int dptr = consume_stars(p);
        char *dname = NULL;
        if (at(p, TK_ID)) { dname = arena_strdup(p->arena, cur_tok(p)->text); advance(p); }
        int darr = 0;
        while (at(p, TK_LBRACK)) {
            advance(p);
            int sz = 0;
            if (!at(p, TK_RBRACK)) {
                Node *se = parse_assign_expr(p);
                if (se && se->kind == NK_INT_LIT) sz = (int)se->u.ival.val;
                else sz = 1;
            }
            try_eat(p, TK_RBRACK);
            if (darr == 0) darr = (sz > 0) ? sz : -1;
        }
        Node *dinit = NULL;
        if (try_eat(p, TK_ASSIGN)) {
            if (at(p, TK_LBRACE)) skip_brace_group(p);
            else dinit = parse_assign_expr(p);
        }
        Node *dn = node_new(p->arena, NK_GLOBAL_VAR, dline, dcol);
        dn->u.gvar.name      = dname;
        dn->u.gvar.base_type = btype;
        dn->u.gvar.ptr_lvl   = dptr;
        dn->u.gvar.arr_sz    = darr;
        dn->u.gvar.init      = dinit;
        dn->u.gvar.is_extern = is_ext;
        dn->u.gvar.is_static = is_st;
        pv_push(&gvars, dn);
    }
    expect(p, TK_SEMI);

    /* Return single node or NK_BLOCK wrapping multiple global var nodes */
    if (gvars.n == 1) {
        Node *single = (Node *)gvars.data[0];
        pv_free(&gvars);
        return single;
    }
    Node *blk = node_new(p->arena, NK_BLOCK, line, col);
    int nn = 0;
    blk->u.block.stmts = (Node **)pv_to_arena(p->arena, &gvars, &nn);
    blk->u.block.n     = nn;
    pv_free(&gvars);
    return blk;
}

static Node *parse_top_level(Parser *p) {
    if (at(p, TK_EOF)) return NULL;
    if (at(p, TK_SEMI)) { advance(p); return NULL; }
    if (at(p, TK_TYPEDEF)) { parse_typedef(p); return NULL; }

    int line = cur_tok(p)->line, col = cur_tok(p)->col;
    bool is_st = false, is_ext = false, is_inl = false;
    char *btype = parse_type_spec(p, &is_st, &is_ext, &is_inl, NULL);

    /* Bare struct/union/enum def followed by ';' */
    if (at(p, TK_SEMI)) { advance(p); return NULL; }

    int ptr_lvl = consume_stars(p);

    /* Global function pointer: type (*name)(params) */
    if (at(p, TK_LPAREN) && at2(p, TK_STAR, 1)) {
        advance(p); advance(p);
        char *name = NULL;
        if (at(p, TK_ID)) { name = arena_strdup(p->arena, cur_tok(p)->text); advance(p); }
        expect(p, TK_RPAREN);
        skip_paren_group(p);
        return finish_global_var(p, arena_strdup(p->arena, "ptr"), 0, name,
                                 is_ext, is_st, line, col);
    }

    if (!at(p, TK_ID)) {
        fprintf(stderr, "parse error at %d:%d: expected identifier, got %s ('%s')\n",
                cur_tok(p)->line, cur_tok(p)->col,
                tok_kind_str(cur_tok(p)->kind), cur_tok(p)->text);
        p->had_error = true;
        skip_to_semi(p);
        return NULL;
    }

    char *name = arena_strdup(p->arena, cur_tok(p)->text);
    advance(p);

    if (at(p, TK_LPAREN)) {
        Node *n = node_new(p->arena, NK_FUNCTION, line, col);
        n->u.fn.ret_type    = btype;
        n->u.fn.ret_ptr_lvl = ptr_lvl;
        n->u.fn.name        = name;
        n->u.fn.is_extern   = is_ext;
        n->u.fn.is_static   = is_st;
        n->u.fn.is_inline   = is_inl;
        parse_param_list(p, n);
        if (at(p, TK_SEMI)) { advance(p); }
        else if (at(p, TK_LBRACE)) { n->u.fn.body = parse_block_body(p); }
        else { fprintf(stderr, "parse error: expected '{' or ';' after function head\n"); p->had_error = true; skip_to_semi(p); }
        return n;
    }

    return finish_global_var(p, btype, ptr_lvl, name, is_ext, is_st, line, col);
}

/* ================================================================
 * parse() – public entry point
 * ================================================================ */
Node *parse(TokenVec *tv, Arena *arena) {
    Parser p;
    memset(&p, 0, sizeof(p));
    p.tv    = tv;
    p.arena = arena;

    /* Seed well-known typedef names */
    static const char *builtin_typedefs[] = {
        "size_t","ssize_t","ptrdiff_t","intptr_t","uintptr_t",
        "int8_t","int16_t","int32_t","int64_t",
        "uint8_t","uint16_t","uint32_t","uint64_t",
        "int_least8_t","int_least16_t","int_least32_t","int_least64_t",
        "uint_least8_t","uint_least16_t","uint_least32_t","uint_least64_t",
        "int_fast8_t","int_fast16_t","int_fast32_t","int_fast64_t",
        "uint_fast8_t","uint_fast16_t","uint_fast32_t","uint_fast64_t",
        "intmax_t","uintmax_t",
        /* Darwin/BSD private typedefs (double-underscore prefix) */
        "__int8_t","__uint8_t","__int16_t","__uint16_t",
        "__int32_t","__uint32_t","__int64_t","__uint64_t",
        "__darwin_blkcnt_t","__darwin_blksize_t","__darwin_dev_t",
        "__darwin_gid_t","__darwin_id_t","__darwin_ino64_t","__darwin_ino_t",
        "__darwin_mode_t","__darwin_off_t","__darwin_pid_t","__darwin_uid_t",
        "__darwin_sigset_t","__darwin_socklen_t","__darwin_suseconds_t",
        "__darwin_time_t","__darwin_clock_t","__darwin_ptrdiff_t",
        "__darwin_size_t","__darwin_ssize_t","__darwin_wchar_t","__darwin_wint_t",
        "__darwin_intptr_t","__darwin_natural_t","__darwin_ct_rune_t",
        "off_t","pid_t","uid_t","gid_t","mode_t","nlink_t","ino_t","dev_t",
        "FILE","DIR","time_t","clock_t","wchar_t","wint_t",
        "va_list","__va_list","socklen_t","sigset_t",
        "bool","_Bool",
        "TokenKind","Token","TokenVec","Arena","ArenaBlock",
        "Node","NodeKind","Param","StructField",
        "Lx","Parser","PtrVec",
        NULL
    };
    for (int i = 0; builtin_typedefs[i]; i++)
        register_typedef(&p, builtin_typedefs[i], NULL);

    Node *prog = node_new(arena, NK_PROGRAM, 1, 1);
    PtrVec iv; pv_init(&iv);
    pv_init(&p.struct_defs);

    while (!at(&p, TK_EOF)) {
        Node *item = parse_top_level(&p);
        if (!item) continue;
        /* NK_BLOCK at top-level means a multi-declarator global: unpack children */
        if (item->kind == NK_BLOCK) {
            for (int ci = 0; ci < item->u.block.n; ci++)
                pv_push(&iv, item->u.block.stmts[ci]);
        } else {
            pv_push(&iv, item);
        }
    }

    /* Build final items list: struct defs first (so IR builder sees types before use),
     * then everything else.  Struct defs that also appear in 'iv' are deduplicated
     * by the IR builder's lookup_struct (it skips already-registered tags). */
    PtrVec final; pv_init(&final);
    for (int i = 0; i < p.struct_defs.n; i++)
        pv_push(&final, p.struct_defs.data[i]);
    for (int i = 0; i < iv.n; i++)
        pv_push(&final, iv.data[i]);
    pv_free(&p.struct_defs);

    prog->u.prog.items  = (Node **)pv_to_arena(arena, &final, &prog->u.prog.nitems);
    pv_free(&final);
    pv_free(&iv);

    for (int i = 0; i < p.ntypedefs; i++) {
        free(p.typedefs[i]);
        if (p.typedef_bases[i]) free(p.typedef_bases[i]);
    }
    free(p.typedefs);
    free(p.typedef_bases);

    if (p.had_error)
        fprintf(stderr, "parse: errors occurred\n");
    return prog;
}

/* ================================================================
 * AST dump (for --parse-only debugging)
 * ================================================================ */
void ast_dump(Node *n, int ind) {
    if (!n) { for(int i=0;i<ind*2;i++) putchar(' '); puts("(null)"); return; }

#define IP(...) do { for(int _i=0;_i<ind*2;_i++) putchar(' '); printf(__VA_ARGS__); putchar('\n'); } while(0)

    switch (n->kind) {
        case NK_INT_LIT:   IP("IntLit(%lld%s)", (long long)n->u.ival.val, n->u.ival.sfx); break;
        case NK_FLOAT_LIT: IP("FloatLit(%g)", n->u.flt.val); break;
        case NK_STR_LIT:   IP("StrLit(\"%s\")", n->u.str.bytes ? n->u.str.bytes : ""); break;
        case NK_VAR:       IP("Var(%s)", n->u.var.name); break;
        case NK_BINOP:
            IP("BinOp(%s)", n->u.binop.op);
            ast_dump(n->u.binop.lhs, ind+1);
            ast_dump(n->u.binop.rhs, ind+1);
            break;
        case NK_UNARY:
            IP("Unary(%s%s)", n->u.unary.op, n->u.unary.prefix ? "[pre]" : "[post]");
            ast_dump(n->u.unary.expr, ind+1);
            break;
        case NK_CALL:
            IP("Call(%d args)", n->u.call.nargs);
            ast_dump(n->u.call.func, ind+1);
            for (int i = 0; i < n->u.call.nargs; i++) ast_dump(n->u.call.args[i], ind+1);
            break;
        case NK_INDEX:
            IP("Index"); ast_dump(n->u.index.base, ind+1); ast_dump(n->u.index.idx, ind+1);
            break;
        case NK_MEMBER:
            IP("Member(%s%s)", n->u.mbr.arrow ? "->" : ".", n->u.mbr.field);
            ast_dump(n->u.mbr.base, ind+1);
            break;
        case NK_CAST:
            IP("Cast(%s%s)", n->u.cast.base_type, n->u.cast.ptr_lvl ? "*" : "");
            ast_dump(n->u.cast.expr, ind+1);
            break;
        case NK_TERNARY:
            IP("Ternary");
            ast_dump(n->u.tern.cond,   ind+1);
            ast_dump(n->u.tern.then_e, ind+1);
            ast_dump(n->u.tern.else_e, ind+1);
            break;
        case NK_SIZEOF:
            if (n->u.szof.base_type) IP("Sizeof(type=%s)", n->u.szof.base_type);
            else { IP("Sizeof(expr)"); ast_dump(n->u.szof.expr, ind+1); }
            break;
        case NK_ASSIGN_EXPR:
            IP("Assign(%s)", n->u.asgn.op);
            ast_dump(n->u.asgn.tgt, ind+1);
            ast_dump(n->u.asgn.expr, ind+1);
            break;
        case NK_DECL:
            IP("Decl(name=%s, type=%s ptr=%d arr=%d%s)",
               n->u.decl.name ? n->u.decl.name : "?",
               n->u.decl.base_type, n->u.decl.ptr_lvl, n->u.decl.arr_sz,
               n->u.decl.is_static ? " static" : "");
            if (n->u.decl.init) ast_dump(n->u.decl.init, ind+1);
            break;
        case NK_RETURN:
            IP("Return");
            if (n->u.expr_wrap.expr) ast_dump(n->u.expr_wrap.expr, ind+1);
            break;
        case NK_EXPR_STMT:
            IP("ExprStmt"); ast_dump(n->u.expr_wrap.expr, ind+1);
            break;
        case NK_BLOCK:
            IP("Block(%d)", n->u.block.n);
            for (int i = 0; i < n->u.block.n; i++) ast_dump(n->u.block.stmts[i], ind+1);
            break;
        case NK_IF:
            IP("If");
            ast_dump(n->u.iff.cond,   ind+1);
            ast_dump(n->u.iff.then_s, ind+1);
            if (n->u.iff.else_s) ast_dump(n->u.iff.else_s, ind+1);
            break;
        case NK_WHILE:
            IP("While"); ast_dump(n->u.whl.cond, ind+1); ast_dump(n->u.whl.body, ind+1);
            break;
        case NK_FOR:
            IP("For");
            if (n->u.forr.init) ast_dump(n->u.forr.init, ind+1);
            if (n->u.forr.cond) ast_dump(n->u.forr.cond, ind+1);
            if (n->u.forr.post) ast_dump(n->u.forr.post, ind+1);
            ast_dump(n->u.forr.body, ind+1);
            break;
        case NK_DO_WHILE:
            IP("DoWhile"); ast_dump(n->u.dowh.body, ind+1); ast_dump(n->u.dowh.cond, ind+1);
            break;
        case NK_SWITCH:
            IP("Switch"); ast_dump(n->u.sw.expr, ind+1); ast_dump(n->u.sw.body, ind+1);
            break;
        case NK_CASE:
            IP("Case"); ast_dump(n->u.cas.val, ind+1); ast_dump(n->u.cas.stmt, ind+1);
            break;
        case NK_DEFAULT:
            IP("Default"); ast_dump(n->u.dft.stmt, ind+1);
            break;
        case NK_BREAK:    IP("Break");    break;
        case NK_CONTINUE: IP("Continue"); break;
        case NK_GOTO:     IP("Goto(%s)", n->u.got.name ? n->u.got.name : "?"); break;
        case NK_LABEL:
            IP("Label(%s)", n->u.lbl.name);
            ast_dump(n->u.lbl.stmt, ind+1);
            break;
        case NK_EMPTY:    IP("Empty"); break;
        case NK_FUNCTION:
            IP("Function(%s ret=%s%s params=%d%s%s%s%s)",
               n->u.fn.name, n->u.fn.ret_type,
               n->u.fn.ret_ptr_lvl ? "*" : "",
               n->u.fn.nparams,
               n->u.fn.variadic  ? " ..."     : "",
               n->u.fn.is_extern ? " extern"  : "",
               n->u.fn.is_static ? " static"  : "",
               n->u.fn.body      ? ""          : " [decl]");
            for (int i = 0; i < n->u.fn.nparams; i++) {
                Param *pm = &n->u.fn.params[i];
                IP("  Param(%s%s %s)", pm->base_type,
                   pm->ptr_lvl ? "*" : "",
                   pm->name ? pm->name : "?");
            }
            if (n->u.fn.body) ast_dump(n->u.fn.body, ind+1);
            break;
        case NK_GLOBAL_VAR:
            IP("GlobalVar(%s type=%s%s arr=%d%s%s)",
               n->u.gvar.name, n->u.gvar.base_type,
               n->u.gvar.ptr_lvl ? "*" : "",
               n->u.gvar.arr_sz,
               n->u.gvar.is_extern ? " extern" : "",
               n->u.gvar.is_static ? " static" : "");
            if (n->u.gvar.init) ast_dump(n->u.gvar.init, ind+1);
            break;
        case NK_STRUCT_DEF:
            IP("StructDef(%s%s %d fields)",
               n->u.sdef.is_union ? "union " : "struct ",
               n->u.sdef.tag ? n->u.sdef.tag : "<anon>",
               n->u.sdef.nfields);
            for (int i = 0; i < n->u.sdef.nfields; i++) {
                StructField *f = &n->u.sdef.fields[i];
                IP("  Field(%s%s %s arr=%d)", f->base_type,
                   f->ptr_lvl ? "*" : "", f->name, f->arr_sz);
            }
            break;
        case NK_ENUM_DEF:
            IP("EnumDef(%s)", n->u.edef.name ? n->u.edef.name : "<anon>"); break;
        case NK_PROGRAM:
            IP("Program(%d items)", n->u.prog.nitems);
            for (int i = 0; i < n->u.prog.nitems; i++) ast_dump(n->u.prog.items[i], ind+1);
            break;
        case NK_COMPOUND_LIT:
            IP("CompoundLit(%s%s)", n->u.clit.base_type, n->u.clit.ptr_lvl ? "*" : "");
            break;
        case NK_STMT_EXPR:
            IP("StmtExpr(%d)", n->u.se.n); break;
        default:
            IP("Node(kind=%d)", (int)n->kind); break;
    }
#undef IP
}
