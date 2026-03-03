/* ir_builder.c – LLVM IR text emission (Phase 1: string-based)
 *
 * Translates the AST produced by parser.c to LLVM IR text.
 * Conventions (matching src/frontend/c2ll.py oracle):
 *   - All pointer types → `ptr`   (LLVM opaque pointers)
 *   - All locals:  alloca at function entry / load-store
 *   - Temps:       %t0, %t1, … (per-function counter reset to 0)
 *   - Block labels: label_<N>_<purpose>
 *   - Comparisons: i1 → zext to i32 for C boolean context
 *   - String literals: @.str<N> = private unnamed_addr constant [N x i8]
 *   - Integer promotion: i8/i16 arithmetic → sext to i32 first
 */

#include "ir_builder.h"
#include "ast.h"
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>

/* ================================================================
 * Type utilities
 * ================================================================ */

/* Write the LLVM type for C type (base, ptr_lvl) into buf[bufsz].
 * Returns a pointer to either buf or a string literal (never heap). */
static const char *lltype_of(const char *base, int ptr_lvl,
                              char *buf, size_t bufsz) {
    if (ptr_lvl > 0) return "ptr";
    if (!base) return "i32";
    if (strcmp(base,"ptr")==0) return "ptr"; /* explicit ptr base (function pointer fields) */
    if (strcmp(base,"int")==0 || strcmp(base,"unsigned int")==0 ||
        strcmp(base,"unsigned")==0 || strcmp(base,"signed")==0 ||
        strcmp(base,"_Bool")==0) return "i32";
    if (strcmp(base,"char")==0 || strcmp(base,"signed char")==0 ||
        strcmp(base,"unsigned char")==0) return "i8";
    if (strcmp(base,"short")==0 || strcmp(base,"unsigned short")==0 ||
        strcmp(base,"signed short")==0) return "i16";
    if (strcmp(base,"long")==0 || strcmp(base,"unsigned long")==0 ||
        strcmp(base,"long long")==0 || strcmp(base,"unsigned long long")==0 ||
        strcmp(base,"signed long long")==0) return "i64";
    if (strcmp(base,"double")==0) return "double";
    if (strcmp(base,"float")==0) return "float";
    if (strcmp(base,"void")==0) return "void";
    if (strcmp(base,"__va_list")==0 || strcmp(base,"__builtin_va_list")==0)
        return "i8";
    if (strncmp(base,"struct ",7)==0) {
        snprintf(buf, bufsz, "%%struct.%s", base+7);
        return buf;
    }
    if (strncmp(base,"union ",6)==0) {
        snprintf(buf, bufsz, "%%union.%s", base+6);
        return buf;
    }
    /* Enum, unknown typedef → i32 */
    return "i32";
}

static int type_size(const char *base, int ptr_lvl) {
    if (ptr_lvl > 0) return 8;
    if (!base) return 4;
    if (strcmp(base,"char")==0 || strcmp(base,"signed char")==0 ||
        strcmp(base,"unsigned char")==0) return 1;
    if (strcmp(base,"short")==0 || strcmp(base,"unsigned short")==0) return 2;
    if (strcmp(base,"int")==0 || strcmp(base,"unsigned int")==0 ||
        strcmp(base,"unsigned")==0 || strcmp(base,"float")==0) return 4;
    if (strcmp(base,"long")==0 || strcmp(base,"unsigned long")==0 ||
        strcmp(base,"long long")==0 || strcmp(base,"unsigned long long")==0 ||
        strcmp(base,"double")==0) return 8;
    if (strcmp(base,"void")==0) return 1;
    return 4;
}

static bool is_fp_lltype(const char *llt) {
    return strcmp(llt,"double")==0 || strcmp(llt,"float")==0;
}

/* Format a double as LLVM IR hex float (0xHHHHHHHHHHHHHHHH).
 * This avoids C99 hex float syntax (%a) which LLVM doesn't accept. */
static int fmt_double_llvm(char *buf, size_t bufsz, double val) {
    uint64_t bits;
    memcpy(&bits, &val, sizeof(bits));
    return snprintf(buf, bufsz, "0x%016" PRIX64, bits);
}

/* Format a float as LLVM IR hex float (0xHHHHHHHHHHHHHHHH - double-extended). */
static int fmt_float_llvm(char *buf, size_t bufsz, float val) {
    /* LLVM represents float literals as double-precision hex */
    return fmt_double_llvm(buf, bufsz, (double)val);
}

static int lltype_byte_size(const char *llt) {
    if (!llt) return 4;
    if (strcmp(llt,"i8")==0)  return 1;
    if (strcmp(llt,"i16")==0) return 2;
    if (strcmp(llt,"i64")==0||strcmp(llt,"double")==0||strcmp(llt,"ptr")==0) return 8;
    if (strcmp(llt,"float")==0) return 4;
    return 4;
}

/* Usual arithmetic conversions: return the promoted LLVM type */
static const char *promote_lltype(const char *a, const char *b) {
    if (strcmp(a,"double")==0 || strcmp(b,"double")==0) return "double";
    if (strcmp(a,"float")==0  || strcmp(b,"float")==0)  return "float";
    if (strcmp(a,"i64")==0    || strcmp(b,"i64")==0)    return "i64";
    return "i32";
}

/* ================================================================
 * Data structures
 * ================================================================ */

#define MAX_LOCALS       512
#define MAX_STRUCT_FIELDS 64
#define MAX_STRUCTS      128
#define MAX_FUNC_SIGS    512
#define MAX_ENUMS        512
#define MAX_STR_LITS     256
#define MAX_GLOBALS      256
#define MAX_LOOP_DEPTH    32
#define TMP_POOL_SZ   (512*1024)  /* 512 KB for temp names per function */

typedef struct {
    char name[128];
    char slot[128];     /* alloca register, e.g. "%a" */
    char lltype[64];    /* LLVM type of the C variable (e.g. "ptr" for int*) */
    char elt_lltype[64];/* Element type for pointers: "i32" for int*, "ptr" for int** */
    int  arr_sz;        /* 0 = scalar */
    bool is_ptr;        /* true if the C type is a pointer */
} LocalSym;

typedef struct {
    char name[128];
    char lltype[64];
    char elt_lltype[64]; /* element type for pointer fields (e.g. "%struct.S" for S*) */
    int  index;          /* field index in LLVM struct type */
    int  arr_sz;
    bool is_ptr;
    bool is_anon_logical; /* sub-field inlined from anonymous struct/union (not a real LLVM field) */
    int  byte_offset;    /* byte offset from start of containing struct (for anon GEP) */
} FieldInfo;

typedef struct {
    char      tag[128];
    bool      is_union;
    FieldInfo fields[MAX_STRUCT_FIELDS];
    int       nfields;
    int       byte_size;
} StructLayout;

typedef struct {
    char name[128];
    char ret_lltype[64];
    char ret_struct_tag[64]; /* struct tag if return is struct ptr, e.g. "S" for struct S * */
    bool variadic;
    char fixed_params_sig[256]; /* types of declared fixed params, e.g. "ptr" for printf */
} FuncSig;

typedef struct {
    char    name[128];
    int64_t val;
} EnumConst;

typedef struct {
    char  *bytes;
    size_t len;
    int    idx;
} StrLit;

typedef struct {
    char name[128];
    char lltype[64];      /* LLVM type of the global variable */
    char elt_lltype[64];  /* element type if it's a pointer */
    int  arr_sz;          /* >0 if it's an array */
    bool is_ptr;
} GlobalSym;

typedef struct {
    FILE *out;

    /* Global registries (valid for the whole program) */
    StructLayout structs[MAX_STRUCTS];
    int          n_structs;

    FuncSig      func_sigs[MAX_FUNC_SIGS];
    int          n_fsigs;

    EnumConst    enums[MAX_ENUMS];
    int          n_enums;

    StrLit       strlits[MAX_STR_LITS];
    int          n_strlits;

    GlobalSym    globals[MAX_GLOBALS];
    int          n_globals;

    /* Per-function state (reset at each function) */
    LocalSym locals[MAX_LOCALS];
    int      n_locals;

    int tmp_counter;
    int label_counter;

    /* Loop stack (break/continue targets) */
    char break_label[MAX_LOOP_DEPTH][128];
    char cont_label [MAX_LOOP_DEPTH][128];
    int  n_loop_stk;

    /* Switch state */
    char sw_end_label[128];
    char sw_def_label[128];  /* unique default label for current switch */
    char sw_case_pfx[64];    /* prefix for case labels in current switch */
    bool in_switch;
    bool sw_def_emitted;     /* true once NK_DEFAULT emits its label */

    /* Current function return type */
    char cur_ret_lltype[64];

    /* Current function name (for local static variable naming) */
    char cur_fn_name[128];

    /* Current basic block label (for phi predecessors in && / ||) */
    char cur_block[128];

    /* After ret/br, skip further instructions until next label */
    bool terminated;

    /* Output buffers (per function) */
    StrBuf alloca_buf;  /* alloca preamble */
    StrBuf body_buf;    /* instructions */
    StrBuf str_buf;     /* string literal globals (emitted before functions) */
    StrBuf *cur_buf;    /* NULL = direct to out */

    /* Bump pool for temp/label names (reset per function) */
    char   tmp_pool[TMP_POOL_SZ];
    size_t tmp_pool_used;
} IRCtx;

/* ================================================================
 * Emit helpers
 * ================================================================ */

/* Emit formatted text to the current buffer or directly to out. */
static void emit(IRCtx *ctx, const char *fmt, ...) {
    char buf[4096];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (ctx->cur_buf)
        sb_puts(ctx->cur_buf, buf);
    else
        fputs(buf, ctx->out);
}

/* Emit only when the current basic block is not yet terminated. */
static void emit_instr(IRCtx *ctx, const char *fmt, ...) {
    if (ctx->terminated) return;
    char buf[4096];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (ctx->cur_buf)
        sb_puts(ctx->cur_buf, buf);
    else
        fputs(buf, ctx->out);
}

/* Emit a terminator instruction (ret/br/switch) and set terminated flag. */
static void emit_term(IRCtx *ctx, const char *fmt, ...) {
    if (ctx->terminated) return;
    char buf[4096];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (ctx->cur_buf)
        sb_puts(ctx->cur_buf, buf);
    else
        fputs(buf, ctx->out);
    ctx->terminated = true;
}

/* Emit a basic block label and reset the terminated flag.
 * Always emits, even in dead code (labels are valid branch targets). */
static void emit_label(IRCtx *ctx, const char *name) {
    /* In LLVM IR, a label is written as "name:\n" at column 0. */
    char buf[256];
    snprintf(buf, sizeof(buf), "%s:\n", name);
    if (ctx->cur_buf)
        sb_puts(ctx->cur_buf, buf);
    else
        fputs(buf, ctx->out);
    ctx->terminated = false;
    strncpy(ctx->cur_block, name, sizeof(ctx->cur_block)-1);
}

/* ================================================================
 * Name allocators (bump-allocated per-function pool)
 * ================================================================ */

static const char *fresh_tmp(IRCtx *ctx) {
    char *p = &ctx->tmp_pool[ctx->tmp_pool_used];
    int n = snprintf(p, 32, "%%t%d", ctx->tmp_counter++);
    ctx->tmp_pool_used += (size_t)(n + 1);
    return p;
}

static const char *fresh_label(IRCtx *ctx, const char *purpose) {
    char *p = &ctx->tmp_pool[ctx->tmp_pool_used];
    int n = snprintf(p, 128, "label_%d_%s", ctx->label_counter++, purpose);
    ctx->tmp_pool_used += (size_t)(n + 1);
    return p;
}

/* Pool-allocate a copy of s (for keeping names alive during a function). */
static const char *pool_dup(IRCtx *ctx, const char *s) {
    size_t n = strlen(s) + 1;
    char *p = &ctx->tmp_pool[ctx->tmp_pool_used];
    memcpy(p, s, n);
    ctx->tmp_pool_used += n;
    return p;
}

/* ================================================================
 * Symbol table operations
 * ================================================================ */

static LocalSym *lookup_local(IRCtx *ctx, const char *name) {
    for (int i = ctx->n_locals - 1; i >= 0; i--)
        if (strcmp(ctx->locals[i].name, name) == 0)
            return &ctx->locals[i];
    return NULL;
}

static LocalSym *register_local(IRCtx *ctx, const char *name,
                                 const char *llt, const char *elt_llt,
                                 int arr_sz, bool is_ptr) {
    if (ctx->n_locals >= MAX_LOCALS) return NULL;
    LocalSym *s = &ctx->locals[ctx->n_locals++];
    strncpy(s->name, name, sizeof(s->name)-1);
    strncpy(s->slot, name[0]=='%' ? name : "", sizeof(s->slot)-1);
    /* Build alloca slot name: %<name> or disambiguate */
    snprintf(s->slot, sizeof(s->slot), "%%%s", name);
    /* disambiguate if collision */
    for (int i = 0; i < ctx->n_locals - 1; i++) {
        if (strcmp(ctx->locals[i].slot, s->slot) == 0) {
            snprintf(s->slot, sizeof(s->slot), "%%%s.%d", name, ctx->n_locals);
            break;
        }
    }
    strncpy(s->lltype, llt, sizeof(s->lltype)-1);
    strncpy(s->elt_lltype, elt_llt ? elt_llt : llt, sizeof(s->elt_lltype)-1);
    s->arr_sz  = arr_sz;
    s->is_ptr  = is_ptr;
    return s;
}

static StructLayout *lookup_struct(IRCtx *ctx, const char *tag) {
    for (int i = 0; i < ctx->n_structs; i++)
        if (strcmp(ctx->structs[i].tag, tag) == 0)
            return &ctx->structs[i];
    return NULL;
}

static FuncSig *lookup_funcsig(IRCtx *ctx, const char *name) {
    for (int i = 0; i < ctx->n_fsigs; i++)
        if (strcmp(ctx->func_sigs[i].name, name) == 0)
            return &ctx->func_sigs[i];
    return NULL;
}

static FuncSig *register_funcsig(IRCtx *ctx, const char *name,
                                  const char *ret_llt, bool variadic,
                                  const char *fixed_params_sig) {
    FuncSig *existing = lookup_funcsig(ctx, name);
    if (existing) return existing;
    if (ctx->n_fsigs >= MAX_FUNC_SIGS) return NULL;
    FuncSig *fs = &ctx->func_sigs[ctx->n_fsigs++];
    strncpy(fs->name, name, sizeof(fs->name)-1);
    strncpy(fs->ret_lltype, ret_llt, sizeof(fs->ret_lltype)-1);
    fs->variadic = variadic;
    if (fixed_params_sig)
        strncpy(fs->fixed_params_sig, fixed_params_sig, sizeof(fs->fixed_params_sig)-1);
    return fs;
}

static int64_t lookup_enum(IRCtx *ctx, const char *name, bool *found) {
    for (int i = 0; i < ctx->n_enums; i++)
        if (strcmp(ctx->enums[i].name, name) == 0) {
            *found = true;
            return ctx->enums[i].val;
        }
    *found = false;
    return 0;
}

static const char *lookup_strlit(IRCtx *ctx, const char *bytes, size_t len) {
    for (int i = 0; i < ctx->n_strlits; i++)
        if (ctx->strlits[i].len == len &&
            memcmp(ctx->strlits[i].bytes, bytes, len) == 0) {
            /* Return the global name */
            char *p = &ctx->tmp_pool[ctx->tmp_pool_used];
            int n = snprintf(p, 32, "@.str%d", ctx->strlits[i].idx);
            ctx->tmp_pool_used += (size_t)(n + 1);
            return p;
        }
    return NULL;
}

static const char *register_strlit(IRCtx *ctx, const char *bytes, size_t len) {
    const char *existing = lookup_strlit(ctx, bytes, len);
    if (existing) return existing;
    if (ctx->n_strlits >= MAX_STR_LITS) return "@.strOVF";
    int idx = ctx->n_strlits;
    StrLit *sl = &ctx->strlits[ctx->n_strlits++];
    sl->bytes = (char *)xmalloc(len);
    memcpy(sl->bytes, bytes, len);
    sl->len = len;
    sl->idx = idx;
    /* Emit the global declaration into str_buf */
    StrBuf *save = ctx->cur_buf;
    ctx->cur_buf = &ctx->str_buf;
    /* [len+1 x i8] = bytes + NUL */
    size_t total = len + 1;
    emit(ctx, "@.str%d = private unnamed_addr constant [%zu x i8] c\"", idx, total);
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)bytes[i];
        if (c == '"' || c == '\\' || c < 0x20 || c > 0x7E)
            emit(ctx, "\\%02X", c);
        else
            emit(ctx, "%c", (char)c);
    }
    emit(ctx, "\\00\", align 1\n");
    ctx->cur_buf = save;
    /* Return the name */
    char *p = &ctx->tmp_pool[ctx->tmp_pool_used];
    int n = snprintf(p, 32, "@.str%d", idx);
    ctx->tmp_pool_used += (size_t)(n + 1);
    return p;
}

/* ================================================================
 * Pre-scan: collect string literals before emitting anything
 * ================================================================ */

static void scan_strlits(IRCtx *ctx, Node *n) {
    if (!n) return;
    switch (n->kind) {
    case NK_STR_LIT:
        register_strlit(ctx, n->u.str.bytes, n->u.str.len);
        break;
    case NK_BINOP:
        scan_strlits(ctx, n->u.binop.lhs);
        scan_strlits(ctx, n->u.binop.rhs);
        break;
    case NK_UNARY:
        scan_strlits(ctx, n->u.unary.expr);
        break;
    case NK_CALL:
        scan_strlits(ctx, n->u.call.func);
        for (int i = 0; i < n->u.call.nargs; i++)
            scan_strlits(ctx, n->u.call.args[i]);
        break;
    case NK_INDEX:
        scan_strlits(ctx, n->u.index.base);
        scan_strlits(ctx, n->u.index.idx);
        break;
    case NK_MEMBER:
        scan_strlits(ctx, n->u.mbr.base);
        break;
    case NK_CAST:
        scan_strlits(ctx, n->u.cast.expr);
        break;
    case NK_TERNARY:
        scan_strlits(ctx, n->u.tern.cond);
        scan_strlits(ctx, n->u.tern.then_e);
        scan_strlits(ctx, n->u.tern.else_e);
        break;
    case NK_SIZEOF:
        scan_strlits(ctx, n->u.szof.expr);
        break;
    case NK_ASSIGN_EXPR:
        scan_strlits(ctx, n->u.asgn.tgt);
        scan_strlits(ctx, n->u.asgn.expr);
        break;
    case NK_COMPOUND_LIT:
        scan_strlits(ctx, n->u.clit.init);
        break;
    case NK_STMT_EXPR:
        for (int i = 0; i < n->u.se.n; i++)
            scan_strlits(ctx, n->u.se.stmts[i]);
        break;
    case NK_DECL:
        scan_strlits(ctx, n->u.decl.init);
        break;
    case NK_RETURN:
    case NK_EXPR_STMT:
        scan_strlits(ctx, n->u.expr_wrap.expr);
        break;
    case NK_BLOCK:
        for (int i = 0; i < n->u.block.n; i++)
            scan_strlits(ctx, n->u.block.stmts[i]);
        break;
    case NK_IF:
        scan_strlits(ctx, n->u.iff.cond);
        scan_strlits(ctx, n->u.iff.then_s);
        scan_strlits(ctx, n->u.iff.else_s);
        break;
    case NK_WHILE:
        scan_strlits(ctx, n->u.whl.cond);
        scan_strlits(ctx, n->u.whl.body);
        break;
    case NK_FOR:
        scan_strlits(ctx, n->u.forr.init);
        scan_strlits(ctx, n->u.forr.cond);
        scan_strlits(ctx, n->u.forr.post);
        scan_strlits(ctx, n->u.forr.body);
        break;
    case NK_DO_WHILE:
        scan_strlits(ctx, n->u.dowh.body);
        scan_strlits(ctx, n->u.dowh.cond);
        break;
    case NK_SWITCH:
        scan_strlits(ctx, n->u.sw.expr);
        scan_strlits(ctx, n->u.sw.body);
        break;
    case NK_CASE:
        scan_strlits(ctx, n->u.cas.val);
        scan_strlits(ctx, n->u.cas.stmt);
        break;
    case NK_DEFAULT:
        scan_strlits(ctx, n->u.dft.stmt);
        break;
    case NK_LABEL:
        scan_strlits(ctx, n->u.lbl.stmt);
        break;
    case NK_FUNCTION:
        scan_strlits(ctx, n->u.fn.body);
        break;
    case NK_GLOBAL_VAR:
        scan_strlits(ctx, n->u.gvar.init);
        break;
    case NK_PROGRAM:
        for (int i = 0; i < n->u.prog.nitems; i++)
            scan_strlits(ctx, n->u.prog.items[i]);
        break;
    default:
        break;
    }
}

/* ================================================================
 * First pass: collect struct defs, enum defs, function signatures
 * ================================================================ */

/* Recursively flatten all NAMED fields from sub-struct into dest, offsetting
 * each field's byte_offset by container_byte_offset.  All flattened entries
 * get index=container_llvm_idx (the LLVM field that holds the container). */
static void flatten_fields_into(StructLayout *dest, const StructLayout *sub,
                                  int container_byte_offset, int container_llvm_idx) {
    for (int j = 0; j < sub->nfields && dest->nfields < MAX_STRUCT_FIELDS; j++) {
        const FieldInfo *sf = &sub->fields[j];
        if (sf->name[0] == '\0') continue; /* skip physical containers */
        FieldInfo *df = &dest->fields[dest->nfields++];
        *df = *sf;
        df->index       = container_llvm_idx;
        df->byte_offset = container_byte_offset + sf->byte_offset;
        df->is_anon_logical = true;
    }
}

static void collect_struct_def(IRCtx *ctx, Node *n) {
    if (!n || n->kind != NK_STRUCT_DEF) return;
    const char *tag = n->u.sdef.tag;
    if (!tag) return;
    if (lookup_struct(ctx, tag)) return;  /* already registered */
    if (ctx->n_structs >= MAX_STRUCTS) return;

    StructLayout *sl = &ctx->structs[ctx->n_structs++];
    strncpy(sl->tag, tag, sizeof(sl->tag)-1);
    sl->is_union = n->u.sdef.is_union;
    sl->nfields  = 0;
    sl->byte_size = 0;

    char tb[64], etb[64];
    int llvm_idx = 0;  /* physical LLVM struct field index */
    for (int i = 0; i < n->u.sdef.nfields && sl->nfields < MAX_STRUCT_FIELDS; i++) {
        StructField *sf = &n->u.sdef.fields[i];
        bool is_anon = (!sf->name || sf->name[0] == '\0');

        if (is_anon && sf->ptr_lvl == 0 &&
            sf->base_type &&
            (strncmp(sf->base_type,"struct ",7)==0 || strncmp(sf->base_type,"union ",6)==0)) {
            /* Anonymous struct/union member: add a physical container entry then
             * expand its sub-fields (and their sub-fields) for member-name lookup. */
            const char *anon_base = sf->base_type;
            const char *anon_tag  = strncmp(anon_base,"struct ",7)==0 ? anon_base+7 : anon_base+6;
            const char *llt = lltype_of(anon_base, 0, tb, sizeof(tb));

            /* Byte offset of this container within the parent */
            int cont_byte_off = sl->is_union ? 0 : sl->byte_size;

            /* Physical container (empty name) */
            FieldInfo *pc = &sl->fields[sl->nfields++];
            memset(pc, 0, sizeof(*pc));
            strncpy(pc->lltype, llt, sizeof(pc->lltype)-1);
            pc->index = llvm_idx;
            pc->byte_offset = cont_byte_off;
            /* pc->name is already "" (zero-initialized) */

            /* Recursively flatten all named sub-fields (including deeply nested ones) */
            StructLayout *sub = lookup_struct(ctx, anon_tag);
            if (sub) {
                flatten_fields_into(sl, sub, cont_byte_off, llvm_idx);
            }

            /* accumulate size */
            int fsz = type_size(anon_base, 0);
            if (sl->is_union) {
                if (fsz > sl->byte_size) sl->byte_size = fsz;
            } else {
                sl->byte_size += fsz;
            }
            llvm_idx++;
            continue;
        }

        /* Normal named field */
        FieldInfo *fi = &sl->fields[sl->nfields++];
        strncpy(fi->name, sf->name ? sf->name : "", sizeof(fi->name)-1);
        const char *llt = lltype_of(sf->base_type, sf->ptr_lvl, tb, sizeof(tb));
        strncpy(fi->lltype, llt, sizeof(fi->lltype)-1);
        int elt_pl = sf->ptr_lvl > 0 ? sf->ptr_lvl - 1 : 0;
        const char *elt_llt = lltype_of(sf->base_type, elt_pl, etb, sizeof(etb));
        strncpy(fi->elt_lltype, elt_llt, sizeof(fi->elt_lltype)-1);
        fi->byte_offset = sl->is_union ? 0 : sl->byte_size;
        fi->index  = llvm_idx++;
        fi->arr_sz = sf->arr_sz;
        fi->is_ptr = sf->ptr_lvl > 0;
        fi->is_anon_logical = false;
        /* accumulate size */
        int fsz = type_size(sf->base_type, sf->ptr_lvl);
        if (sf->arr_sz > 0) fsz *= sf->arr_sz;
        if (sl->is_union) {
            if (fsz > sl->byte_size) sl->byte_size = fsz;
        } else {
            sl->byte_size += fsz;
        }
    }
}

static void collect_enum_def(IRCtx *ctx, Node *n) {
    if (!n || n->kind != NK_ENUM_DEF) return;
    for (int i = 0; i < n->u.edef.nconsts; i++) {
        if (ctx->n_enums >= MAX_ENUMS) break;
        EnumConst *ec = &ctx->enums[ctx->n_enums++];
        strncpy(ec->name, n->u.edef.cst_names[i], sizeof(ec->name)-1);
        ec->val = n->u.edef.cst_vals[i];
    }
}

static void collect_func_sig(IRCtx *ctx, Node *n) {
    if (!n || n->kind != NK_FUNCTION) return;
    char tb[64];
    const char *rlt = lltype_of(n->u.fn.ret_type, n->u.fn.ret_ptr_lvl, tb, sizeof(tb));
    FuncSig *fs = NULL;
    FuncSig *existing = lookup_funcsig(ctx, n->u.fn.name);
    if (existing) {
        fs = existing;
    } else if (ctx->n_fsigs < MAX_FUNC_SIGS) {
        fs = &ctx->func_sigs[ctx->n_fsigs++];
        strncpy(fs->name, n->u.fn.name, sizeof(fs->name)-1);
    }
    if (fs) {
        strncpy(fs->ret_lltype, rlt, sizeof(fs->ret_lltype)-1);
        fs->variadic = n->u.fn.variadic;
        /* Populate ret_struct_tag for functions returning struct pointer */
        fs->ret_struct_tag[0] = '\0';
        if (n->u.fn.ret_ptr_lvl > 0 && n->u.fn.ret_type &&
            strncmp(n->u.fn.ret_type, "struct ", 7) == 0) {
            strncpy(fs->ret_struct_tag, n->u.fn.ret_type + 7,
                    sizeof(fs->ret_struct_tag)-1);
        }
        /* Build fixed_params_sig: types of declared fixed parameters only.
         * This is used as the function type in variadic call instructions so
         * that the type matches the declaration (e.g. "ptr" for printf). */
        fs->fixed_params_sig[0] = '\0';
        int nwritten = 0;
        for (int i = 0; i < n->u.fn.nparams; i++) {
            Param *p = &n->u.fn.params[i];
            if (p->base_type && strcmp(p->base_type, "void") == 0 && p->ptr_lvl == 0)
                continue;
            char ptb[64];
            const char *plt = lltype_of(p->base_type, p->ptr_lvl, ptb, sizeof(ptb));
            if (nwritten > 0)
                strncat(fs->fixed_params_sig, ", ",
                        sizeof(fs->fixed_params_sig) - strlen(fs->fixed_params_sig) - 1);
            strncat(fs->fixed_params_sig, plt,
                    sizeof(fs->fixed_params_sig) - strlen(fs->fixed_params_sig) - 1);
            nwritten++;
        }
    }
}

static GlobalSym *lookup_global(IRCtx *ctx, const char *name) {
    for (int i = 0; i < ctx->n_globals; i++)
        if (strcmp(ctx->globals[i].name, name) == 0)
            return &ctx->globals[i];
    return NULL;
}

/* Compute actual array size for a global var with arr_sz == -1 (inferred from init). */
static int compute_global_arr_sz(Node *n) {
    if (!n || n->kind != NK_GLOBAL_VAR) return 0;
    int arr_sz = n->u.gvar.arr_sz;
    if (arr_sz != -1) return arr_sz;
    /* Infer from string literal initializer */
    if (n->u.gvar.init && n->u.gvar.init->kind == NK_STR_LIT)
        return (int)(n->u.gvar.init->u.str.len + 1);
    /* Infer from brace initializer list */
    if (n->u.gvar.nInits > 0) {
        int max_idx = -1, next_seq = 0;
        for (int j = 0; j < n->u.gvar.nInits; j++) {
            const char *f = n->u.gvar.init_fields ? n->u.gvar.init_fields[j] : NULL;
            int cur_idx;
            if (f && f[0] == '[') {
                cur_idx = (int)strtol(f + 1, NULL, 10);
                next_seq = cur_idx + 1;
            } else {
                cur_idx = next_seq++;
            }
            if (cur_idx > max_idx) max_idx = cur_idx;
        }
        return max_idx + 1;
    }
    return -1; /* still unknown */
}

static void collect_global_var(IRCtx *ctx, Node *n) {
    if (!n || n->kind != NK_GLOBAL_VAR) return;
    if (ctx->n_globals >= MAX_GLOBALS) return;
    char tb[64], etb[64];
    const char *llt = lltype_of(n->u.gvar.base_type, n->u.gvar.ptr_lvl, tb, sizeof(tb));
    int elt_pl = n->u.gvar.ptr_lvl > 0 ? n->u.gvar.ptr_lvl - 1 : 0;
    const char *elt_llt = lltype_of(n->u.gvar.base_type, elt_pl, etb, sizeof(etb));
    GlobalSym *gs = &ctx->globals[ctx->n_globals++];
    strncpy(gs->name, n->u.gvar.name, sizeof(gs->name)-1);
    strncpy(gs->lltype, llt, sizeof(gs->lltype)-1);
    strncpy(gs->elt_lltype, elt_llt, sizeof(gs->elt_lltype)-1);
    /* Normalize arr_sz: compute actual size when parser left it as -1 */
    gs->arr_sz = compute_global_arr_sz(n);
    gs->is_ptr = n->u.gvar.ptr_lvl > 0;
}

/* ================================================================
 * Expression type inference (for cast/promote decisions)
 * ================================================================ */

/* Forward declarations */
static const char *codegen_expr(IRCtx *ctx, Node *n);
static const char *codegen_lval(IRCtx *ctx, Node *n);
static void codegen_stmt(IRCtx *ctx, Node *n);
static const char *get_struct_tag(IRCtx *ctx, Node *n, bool arrow);

/* Determine the LLVM type that codegen_expr will produce for node n.
 * Returns a literal or pool-allocated string. buf/sz are scratch space
 * for struct name formatting. */
static const char *expr_lltype(IRCtx *ctx, Node *n, char *buf, size_t bufsz) {
    if (!n) return "i32";
    switch (n->kind) {
    case NK_INT_LIT: {
        const char *sfx = n->u.ival.sfx;
        if (strstr(sfx,"LL") || strstr(sfx,"ll") ||
            strstr(sfx,"L")  || strstr(sfx,"l"))
            return "i64";
        return "i32";
    }
    case NK_FLOAT_LIT: return "double";
    case NK_STR_LIT:   return "ptr";
    case NK_VAR: {
        /* Check enum first */
        bool found;
        lookup_enum(ctx, n->u.var.name, &found);
        if (found) return "i32";
        LocalSym *ls = lookup_local(ctx, n->u.var.name);
        if (ls) {
            if (ls->arr_sz != 0 || ls->is_ptr) return "ptr";
            return ls->lltype;
        }
        GlobalSym *gs = lookup_global(ctx, n->u.var.name);
        if (gs) {
            if (gs->arr_sz != 0 || gs->is_ptr) return "ptr";
            return gs->lltype;
        }
        /* Function name used as a value → ptr */
        if (lookup_funcsig(ctx, n->u.var.name)) return "ptr";
        return "i32";
    }
    case NK_BINOP: {
        const char *op = n->u.binop.op;
        /* Comparisons always return int */
        if (strcmp(op,"==")==0||strcmp(op,"!=")==0||strcmp(op,"<")==0||
            strcmp(op,"<=")==0||strcmp(op,">")==0||strcmp(op,">=")==0||
            strcmp(op,"&&")==0||strcmp(op,"||")==0) return "i32";
        /* Shift: result type is promoted left operand (i8/i16 → i32 per C integer promotions) */
        if (strcmp(op,"<<")==0||strcmp(op,">>")==0) {
            const char *lt = expr_lltype(ctx, n->u.binop.lhs, buf, bufsz);
            if (strcmp(lt,"i8")==0 || strcmp(lt,"i16")==0) return "i32";
            return lt;
        }
        const char *la = expr_lltype(ctx, n->u.binop.lhs, buf, bufsz);
        const char *lb = expr_lltype(ctx, n->u.binop.rhs, buf, bufsz);
        /* Pointer arithmetic: ptr+int, int+ptr → ptr; ptr-ptr → i64 */
        bool la_ptr = strcmp(la,"ptr")==0, lb_ptr = strcmp(lb,"ptr")==0;
        if ((strcmp(op,"+")==0 || strcmp(op,"-")==0) && (la_ptr || lb_ptr)) {
            if (la_ptr && lb_ptr) return "i64"; /* ptr - ptr = ptrdiff */
            return "ptr"; /* ptr ± int = ptr */
        }
        return promote_lltype(la, lb);
    }
    case NK_UNARY: {
        const char *op = n->u.unary.op;
        if (strcmp(op,"!")==0) return "i32";
        if (strcmp(op,"&")==0) return "ptr";
        if (strcmp(op,"*")==0) {
            /* Dereference: return element type of the pointer expression.
             * Strip pre/post ++ / -- to reach the actual pointer base node. */
            Node *inner = n->u.unary.expr;
            while (inner->kind == NK_UNARY &&
                   (strcmp(inner->u.unary.op,"++")==0 ||
                    strcmp(inner->u.unary.op,"--")==0)) {
                inner = inner->u.unary.expr;
            }
            if (inner->kind == NK_VAR) {
                LocalSym *ls = lookup_local(ctx, inner->u.var.name);
                if (ls && ls->is_ptr && ls->elt_lltype[0]) return ls->elt_lltype;
                GlobalSym *gs = lookup_global(ctx, inner->u.var.name);
                if (gs && gs->is_ptr && gs->elt_lltype[0]) return gs->elt_lltype;
            }
            if (inner->kind == NK_MEMBER) {
                const char *fn = inner->u.mbr.field;
                const char *tag = get_struct_tag(ctx, inner->u.mbr.base,
                                                 inner->u.mbr.arrow);
                if (tag) {
                    StructLayout *sl = lookup_struct(ctx, tag);
                    if (sl) {
                        for (int i = 0; i < sl->nfields; i++) {
                            if (strcmp(sl->fields[i].name, fn) == 0 &&
                                sl->fields[i].is_ptr && sl->fields[i].elt_lltype[0])
                                return sl->fields[i].elt_lltype;
                        }
                    }
                }
            }
            return "i32";
        }
        /* ++ / -- result is promoted (i8/i16 → i32 in our codegen) */
        if (strcmp(op,"++")==0 || strcmp(op,"--")==0) {
            const char *t = expr_lltype(ctx, n->u.unary.expr, buf, bufsz);
            if (strcmp(t,"i8")==0||strcmp(t,"i16")==0) return "i32";
            return t;
        }
        /* Unary -, +, ~ on i8/i16 promote to i32 (C integer promotion rules) */
        {
            const char *t = expr_lltype(ctx, n->u.unary.expr, buf, bufsz);
            if (strcmp(t,"i8")==0 || strcmp(t,"i16")==0) return "i32";
            return t;
        }
    }
    case NK_CALL: {
        if (n->u.call.func->kind == NK_VAR) {
            const char *fname = n->u.call.func->u.var.name;
            /* builtins with known return types (must match codegen special-cases) */
            if (strcmp(fname,"__builtin_bswap64")==0) return "i64";
            if (strcmp(fname,"__builtin_bswap32")==0) return "i32";
            if (strcmp(fname,"__builtin_bswap16")==0) return "i16";
            if (strcmp(fname,"__builtin_fabs")==0 ||
                strcmp(fname,"__builtin_inf")==0 || strcmp(fname,"__builtin_infl")==0)
                return "double";
            if (strcmp(fname,"__builtin_inff")==0) return "float";
            if (strcmp(fname,"__builtin_expect")==0 && n->u.call.nargs>=1)
                return expr_lltype(ctx, n->u.call.args[0], buf, bufsz);
            FuncSig *fs = lookup_funcsig(ctx, fname);
            if (fs) return fs->ret_lltype;
        }
        return "i32";
    }
    case NK_CAST:
        return lltype_of(n->u.cast.base_type, n->u.cast.ptr_lvl, buf, bufsz);
    case NK_SIZEOF:  return "i64";
    case NK_INDEX: {
        if (n->u.index.base->kind == NK_VAR) {
            LocalSym *ls = lookup_local(ctx, n->u.index.base->u.var.name);
            if (ls) {
                if (ls->arr_sz == 0 && ls->is_ptr)
                    return ls->elt_lltype[0] ? ls->elt_lltype : "i32";
                return ls->lltype;
            }
            GlobalSym *gs = lookup_global(ctx, n->u.index.base->u.var.name);
            if (gs) {
                if (gs->arr_sz == 0 && gs->is_ptr)
                    return gs->elt_lltype[0] ? gs->elt_lltype : "i32";
                return gs->lltype;
            }
        }
        /* NK_MEMBER base: array field subscript – element type from FieldInfo */
        if (n->u.index.base->kind == NK_MEMBER) {
            const char *field = n->u.index.base->u.mbr.field;
            const char *tag = get_struct_tag(ctx, n->u.index.base->u.mbr.base,
                                             n->u.index.base->u.mbr.arrow);
            if (tag) {
                StructLayout *sl = lookup_struct(ctx, tag);
                if (sl) {
                    for (int i = 0; i < sl->nfields; i++) {
                        if (strcmp(sl->fields[i].name, field) == 0) {
                            const char *elt = sl->fields[i].elt_lltype;
                            return (elt && elt[0]) ? elt : sl->fields[i].lltype;
                        }
                    }
                }
            }
        }
        return "i32";
    }
    case NK_MEMBER: {
        /* Use get_struct_tag to resolve the base struct (handles chained accesses). */
        const char *field = n->u.mbr.field;
        const char *tag = get_struct_tag(ctx, n->u.mbr.base, n->u.mbr.arrow);
        if (tag) {
            StructLayout *sl = lookup_struct(ctx, tag);
            if (sl) {
                for (int i = 0; i < sl->nfields; i++) {
                    if (strcmp(sl->fields[i].name, field) == 0)
                        return sl->fields[i].lltype;
                }
            }
        }
        /* Arrow access through unknown struct type: likely a pointer field */
        if (n->u.mbr.arrow) return "ptr";
        return "i32";
    }
    case NK_TERNARY: {
        /* If either branch is ptr, the overall type is ptr (C rule) */
        char tb2[64];
        const char *tlt = expr_lltype(ctx, n->u.tern.then_e, buf, bufsz);
        const char *elt = expr_lltype(ctx, n->u.tern.else_e, tb2, sizeof(tb2));
        if (strcmp(elt,"ptr")==0 && strcmp(tlt,"ptr")!=0) return "ptr";
        return tlt;
    }
    case NK_ASSIGN_EXPR:
        return expr_lltype(ctx, n->u.asgn.tgt, buf, bufsz);
    case NK_COMPOUND_LIT:
        return lltype_of(n->u.clit.base_type, n->u.clit.ptr_lvl, buf, bufsz);
    case NK_STMT_EXPR:
        if (n->u.se.n > 0)
            return expr_lltype(ctx, n->u.se.stmts[n->u.se.n-1], buf, bufsz);
        return "void";
    default:
        return "i32";
    }
}

/* ================================================================
 * Cast / promote helpers
 * ================================================================ */

/* Emit a cast instruction if from_llt != to_llt.
 * Returns either val (no cast needed) or a fresh temp with the cast result. */
static const char *cast_val(IRCtx *ctx, const char *val,
                              const char *from_llt, const char *to_llt) {
    if (!from_llt || !to_llt) return val;
    if (strcmp(from_llt, to_llt) == 0) return val;

    /* void → no-op */
    if (strcmp(to_llt,"void")==0) return val;

    /* int → ptr */
    if (strcmp(to_llt,"ptr")==0 && strcmp(from_llt,"ptr")!=0) {
        const char *t = fresh_tmp(ctx);
        emit_instr(ctx, "  %s = inttoptr %s %s to ptr\n", t, from_llt, val);
        return t;
    }
    /* ptr → int */
    if (strcmp(from_llt,"ptr")==0 && strcmp(to_llt,"ptr")!=0) {
        const char *t = fresh_tmp(ctx);
        emit_instr(ctx, "  %s = ptrtoint ptr %s to %s\n", t, val, to_llt);
        return t;
    }

    bool from_fp = is_fp_lltype(from_llt);
    bool to_fp   = is_fp_lltype(to_llt);

    if (from_fp && !to_fp) {
        /* float → int: fptosi */
        const char *t = fresh_tmp(ctx);
        emit_instr(ctx, "  %s = fptosi %s %s to %s\n", t, from_llt, val, to_llt);
        return t;
    }
    if (!from_fp && to_fp) {
        /* int → float: sitofp */
        const char *t = fresh_tmp(ctx);
        emit_instr(ctx, "  %s = sitofp %s %s to %s\n", t, from_llt, val, to_llt);
        return t;
    }
    if (from_fp && to_fp) {
        if (strcmp(from_llt,"float")==0 && strcmp(to_llt,"double")==0) {
            const char *t = fresh_tmp(ctx);
            emit_instr(ctx, "  %s = fpext float %s to double\n", t, val);
            return t;
        }
        if (strcmp(from_llt,"double")==0 && strcmp(to_llt,"float")==0) {
            const char *t = fresh_tmp(ctx);
            emit_instr(ctx, "  %s = fptrunc double %s to float\n", t, val);
            return t;
        }
        return val;
    }

    /* Both integers: determine widths */
    int fw = 32, tw = 32;
    if (strcmp(from_llt,"i64")==0) fw=64;
    else if (strcmp(from_llt,"i16")==0) fw=16;
    else if (strcmp(from_llt,"i8")==0)  fw=8;
    if (strcmp(to_llt,"i64")==0) tw=64;
    else if (strcmp(to_llt,"i16")==0) tw=16;
    else if (strcmp(to_llt,"i8")==0)  tw=8;

    if (fw == tw) return val;
    const char *t = fresh_tmp(ctx);
    if (fw > tw) {
        emit_instr(ctx, "  %s = trunc %s %s to %s\n", t, from_llt, val, to_llt);
    } else {
        emit_instr(ctx, "  %s = sext %s %s to %s\n", t, from_llt, val, to_llt);
    }
    return t;
}

/* Promote an i8/i16 value to i32 (C integer promotion rule). */
static const char *promote_to_i32(IRCtx *ctx, const char *val,
                                    const char *llt) {
    if (strcmp(llt,"i8")==0 || strcmp(llt,"i16")==0)
        return cast_val(ctx, val, llt, "i32");
    return val;
}

/* Emit a truthiness check: llt val → i1.
 * Handles ptr (icmp ne ptr val, null), double/float (fcmp une),
 * and integer types (icmp ne iN val, 0). Returns the i1 temp name. */
static const char *emit_truth_check(IRCtx *ctx, const char *val,
                                     const char *llt) {
    /* Promote i8/i16 first (unless it's a ptr) */
    const char *use_llt = llt;
    const char *use_val = val;
    if (strcmp(llt,"i8")==0 || strcmp(llt,"i16")==0) {
        use_val = promote_to_i32(ctx, val, llt);
        use_llt = "i32";
    }
    const char *ci = fresh_tmp(ctx);
    if (strcmp(use_llt,"ptr")==0) {
        emit_instr(ctx, "  %s = icmp ne ptr %s, null\n", ci, use_val);
    } else if (strcmp(use_llt,"double")==0 || strcmp(use_llt,"float")==0) {
        emit_instr(ctx, "  %s = fcmp une %s %s, 0.0\n", ci, use_llt, use_val);
    } else {
        emit_instr(ctx, "  %s = icmp ne %s %s, 0\n", ci, use_llt, use_val);
    }
    return ci;
}

/* ================================================================
 * codegen_lval – compute the *address* of an lvalue (returns ptr)
 * ================================================================ */

/* Return the struct tag (without %struct. prefix) for what expression n resolves to.
 * Used to determine the struct type for GEP in member/arrow access.
 * 'arrow' indicates whether this call is for a -> dereference (so we want elt type).
 * Returns NULL if unknown. */
static const char *get_struct_tag(IRCtx *ctx, Node *n, bool arrow) {
    if (!n) return NULL;
    if (n->kind == NK_VAR) {
        LocalSym *ls = lookup_local(ctx, n->u.var.name);
        if (ls) {
            const char *llt = (arrow && ls->is_ptr) ? ls->elt_lltype : ls->lltype;
            if (strncmp(llt, "%struct.", 8) == 0) return llt + 8;
            return NULL;
        }
        GlobalSym *gs = lookup_global(ctx, n->u.var.name);
        if (gs) {
            const char *llt = (arrow && gs->is_ptr) ? gs->elt_lltype : gs->lltype;
            if (strncmp(llt, "%struct.", 8) == 0) return llt + 8;
            return NULL;
        }
        return NULL;
    }
    if (n->kind == NK_MEMBER) {
        /* Get the struct tag of the base, then look up the field's type */
        const char *base_tag = get_struct_tag(ctx, n->u.mbr.base, n->u.mbr.arrow);
        if (!base_tag) return NULL;
        StructLayout *sl = lookup_struct(ctx, base_tag);
        if (!sl) return NULL;
        for (int i = 0; i < sl->nfields; i++) {
            if (strcmp(sl->fields[i].name, n->u.mbr.field) == 0) {
                /* For arrow we want what the field points to */
                const char *llt = sl->fields[i].elt_lltype;
                if (!arrow) llt = sl->fields[i].lltype;
                if (strncmp(llt, "%struct.", 8) == 0) return llt + 8;
                return NULL;
            }
        }
        return NULL;
    }
    if (n->kind == NK_CALL && n->u.call.func->kind == NK_VAR) {
        /* Direct named function call: use ret_struct_tag from FuncSig */
        FuncSig *fs = lookup_funcsig(ctx, n->u.call.func->u.var.name);
        if (fs && arrow && fs->ret_struct_tag[0])
            return fs->ret_struct_tag;
        return NULL;
    }
    if (n->kind == NK_INDEX) {
        /* Indexing into an array (e.g., jones[i].field): struct tag from array element type */
        if (n->u.index.base->kind == NK_VAR) {
            LocalSym *ls = lookup_local(ctx, n->u.index.base->u.var.name);
            if (ls) {
                const char *llt = ls->lltype;
                if (strncmp(llt, "%struct.", 8) == 0) return llt + 8;
                return NULL;
            }
            GlobalSym *gs = lookup_global(ctx, n->u.index.base->u.var.name);
            if (gs) {
                const char *llt = gs->lltype;
                if (strncmp(llt, "%struct.", 8) == 0) return llt + 8;
                return NULL;
            }
        }
        return NULL;
    }
    return NULL;
}

static const char *codegen_lval(IRCtx *ctx, Node *n) {
    if (!n) return "undef";
    char tb[64];
    switch (n->kind) {
    case NK_VAR: {
        LocalSym *ls = lookup_local(ctx, n->u.var.name);
        if (ls) return pool_dup(ctx, ls->slot);
        /* Fallback: global variable */
        char *p = &ctx->tmp_pool[ctx->tmp_pool_used];
        int sz = snprintf(p, 128, "@%s", n->u.var.name);
        ctx->tmp_pool_used += (size_t)(sz + 1);
        return p;
    }
    case NK_UNARY:
        if (strcmp(n->u.unary.op,"*")==0) {
            /* Dereference: lval = the pointer itself */
            return codegen_expr(ctx, n->u.unary.expr);
        }
        break;
    case NK_INDEX: {
        /* Determine element type and whether base is an array variable (not pointer).
         * For array vars: codegen_lval gives the array address (correct for GEP).
         * For pointer vars or expressions: codegen_expr gives the pointer VALUE. */
        const char *elt = "i32";
        bool base_is_array = false;
        if (n->u.index.base->kind == NK_VAR) {
            LocalSym *ls = lookup_local(ctx, n->u.index.base->u.var.name);
            if (ls) {
                if (ls->arr_sz != 0 && !ls->is_ptr) {
                    base_is_array = true;
                    elt = ls->lltype;
                } else if (ls->is_ptr) {
                    elt = ls->elt_lltype;
                }
            } else {
                GlobalSym *gs = lookup_global(ctx, n->u.index.base->u.var.name);
                if (gs) {
                    if (gs->arr_sz != 0 && !gs->is_ptr) {
                        base_is_array = true;
                        elt = gs->lltype;
                    } else if (gs->is_ptr) {
                        elt = gs->elt_lltype;
                    }
                }
            }
        } else if (n->u.index.base->kind == NK_MEMBER) {
            /* Struct array field subscript: get element type from FieldInfo */
            const char *field = n->u.index.base->u.mbr.field;
            const char *tag = get_struct_tag(ctx, n->u.index.base->u.mbr.base,
                                             n->u.index.base->u.mbr.arrow);
            if (tag) {
                StructLayout *sl = lookup_struct(ctx, tag);
                if (sl) {
                    for (int i = 0; i < sl->nfields; i++) {
                        if (strcmp(sl->fields[i].name, field) == 0) {
                            const char *e = sl->fields[i].elt_lltype;
                            elt = (e && e[0]) ? e : sl->fields[i].lltype;
                            break;
                        }
                    }
                }
            }
            /* base is NK_MEMBER of array type: codegen_expr returns the ptr already */
        }
        const char *base = base_is_array
                           ? codegen_lval(ctx, n->u.index.base)
                           : codegen_expr(ctx, n->u.index.base);
        const char *idx  = codegen_expr(ctx, n->u.index.idx);
        /* GEP index must be i64 */
        const char *idx_llt = expr_lltype(ctx, n->u.index.idx, tb, sizeof(tb));
        idx = cast_val(ctx, idx, strcmp(idx_llt,"i64")==0?"i64":"i32", "i64");
        const char *t = fresh_tmp(ctx);
        emit_instr(ctx, "  %s = getelementptr inbounds %s, ptr %s, i64 %s\n",
                   t, elt, base, idx);
        return t;
    }
    case NK_MEMBER: {
        bool arrow = n->u.mbr.arrow;
        const char *field = n->u.mbr.field;
        const char *base_ptr;
        if (arrow) {
            base_ptr = codegen_expr(ctx, n->u.mbr.base);
        } else {
            base_ptr = codegen_lval(ctx, n->u.mbr.base);
        }
        /* Find the struct tag for GEP: use recursive helper that handles
         * chained member accesses and arrow (pointer dereference) correctly. */
        const char *tag = get_struct_tag(ctx, n->u.mbr.base, arrow);
        if (!tag) {
            /* Can't determine struct type; use i8 GEP offset 0 as fallback */
            const char *t = fresh_tmp(ctx);
            emit_instr(ctx, "  %s = getelementptr i8, ptr %s, i64 0\n", t, base_ptr);
            return t;
        }
        StructLayout *sl = lookup_struct(ctx, tag);
        int field_idx = 0;
        int field_byte_off = 0;
        bool use_byte_gep = false;
        if (sl) {
            for (int i = 0; i < sl->nfields; i++) {
                if (strcmp(sl->fields[i].name, field)==0) {
                    field_idx      = sl->fields[i].index;
                    field_byte_off = sl->fields[i].byte_offset;
                    use_byte_gep   = sl->fields[i].is_anon_logical;
                    break;
                }
            }
        }
        const char *t = fresh_tmp(ctx);
        if (use_byte_gep) {
            /* Anonymous struct/union member: use byte-level GEP for correct offset */
            emit_instr(ctx, "  %s = getelementptr inbounds i8, ptr %s, i64 %d\n",
                       t, base_ptr, field_byte_off);
        } else {
            /* Regular named field: use LLVM struct GEP */
            char full_tag[128];
            snprintf(full_tag, sizeof(full_tag), "%%struct.%s", tag);
            emit_instr(ctx, "  %s = getelementptr inbounds %s, ptr %s, i32 0, i32 %d\n",
                       t, full_tag, base_ptr, field_idx);
        }
        return t;
    }
    default:
        break;
    }
    /* Fallback: codegen as expr (e.g. already a ptr) */
    (void)tb;
    return codegen_expr(ctx, n);
}

/* ================================================================
 * codegen_expr – emit expression code, return the SSA value name
 * ================================================================ */

static const char *codegen_expr(IRCtx *ctx, Node *n) {
    if (!n) return "0";
    char tb1[64], tb2[64];

    switch (n->kind) {
    /* ---- Literals ---- */
    case NK_INT_LIT: {
        char *p = &ctx->tmp_pool[ctx->tmp_pool_used];
        int sz = snprintf(p, 32, "%" PRId64, n->u.ival.val);
        ctx->tmp_pool_used += (size_t)(sz + 1);
        return p;
    }
    case NK_FLOAT_LIT: {
        char *p = &ctx->tmp_pool[ctx->tmp_pool_used];
        int sz = fmt_double_llvm(p, 32, n->u.flt.val);
        ctx->tmp_pool_used += (size_t)(sz + 1);
        return p;
    }
    case NK_STR_LIT: {
        const char *name = register_strlit(ctx, n->u.str.bytes, n->u.str.len);
        /* Decay to ptr (getelementptr [N x i8], ptr @.strX, i64 0, i64 0) */
        size_t total = n->u.str.len + 1;
        const char *t = fresh_tmp(ctx);
        emit_instr(ctx, "  %s = getelementptr [%zu x i8], ptr %s, i64 0, i64 0\n",
                   t, total, name);
        return t;
    }

    /* ---- Variable reference ---- */
    case NK_VAR: {
        /* Enum constant? */
        bool found;
        int64_t ev = lookup_enum(ctx, n->u.var.name, &found);
        if (found) {
            char *p = &ctx->tmp_pool[ctx->tmp_pool_used];
            int sz = snprintf(p, 32, "%" PRId64, ev);
            ctx->tmp_pool_used += (size_t)(sz + 1);
            return p;
        }
        LocalSym *ls = lookup_local(ctx, n->u.var.name);
        if (ls) {
            if (ls->arr_sz > 0) {
                /* Arrays decay to ptr; return the alloca address */
                return pool_dup(ctx, ls->slot);
            }
            /* Scalar: load */
            const char *t = fresh_tmp(ctx);
            emit_instr(ctx, "  %s = load %s, ptr %s\n",
                       t, ls->lltype, ls->slot);
            return t;
        }
        /* Global variable: load */
        char gname[128];
        snprintf(gname, sizeof(gname), "@%s", n->u.var.name);
        /* Function used as value (function pointer decay): return address directly */
        if (lookup_funcsig(ctx, n->u.var.name)) {
            return pool_dup(ctx, gname);
        }
        GlobalSym *gs = lookup_global(ctx, n->u.var.name);
        if (gs && gs->arr_sz > 0) {
            /* Array global decays to ptr; return address directly */
            return pool_dup(ctx, gname);
        }
        const char *gllt = gs ? gs->lltype : "i32";
        const char *t = fresh_tmp(ctx);
        emit_instr(ctx, "  %s = load %s, ptr %s\n", t, gllt, gname);
        return t;
    }

    /* ---- Binary operator ---- */
    case NK_BINOP: {
        const char *op  = n->u.binop.op;

        /* Short-circuit logical operators: evaluate LHS first, branch on result */
        if (strcmp(op,"&&")==0 || strcmp(op,"||")==0) {
            bool is_and = (op[0] == '&');
            const char *lrhs = pool_dup(ctx, fresh_label(ctx, is_and?"land_r":"lor_r"));
            const char *lend = pool_dup(ctx, fresh_label(ctx, is_and?"land_e":"lor_e"));
            /* Evaluate LHS */
            const char *llt_l0 = expr_lltype(ctx, n->u.binop.lhs, tb1, sizeof(tb1));
            const char *lhs0 = codegen_expr(ctx, n->u.binop.lhs);
            const char *c1 = emit_truth_check(ctx, lhs0, llt_l0);
            const char *lhs_block = pool_dup(ctx, ctx->cur_block);
            if (is_and)
                emit_term(ctx, "  br i1 %s, label %%%s, label %%%s\n", c1, lrhs, lend);
            else
                emit_term(ctx, "  br i1 %s, label %%%s, label %%%s\n", c1, lend, lrhs);
            /* Evaluate RHS in its own block */
            emit_label(ctx, lrhs);
            const char *llt_r0 = expr_lltype(ctx, n->u.binop.rhs, tb2, sizeof(tb2));
            const char *rhs0 = codegen_expr(ctx, n->u.binop.rhs);
            const char *c2 = emit_truth_check(ctx, rhs0, llt_r0);
            const char *rhs_block = pool_dup(ctx, ctx->cur_block);
            emit_term(ctx, "  br label %%%s\n", lend);
            /* Join block with phi */
            emit_label(ctx, lend);
            const char *t = fresh_tmp(ctx);
            /* For &&: false when LHS=false; for ||: true when LHS=true */
            if (is_and)
                emit_instr(ctx, "  %s = phi i1 [ false, %%%s ], [ %s, %%%s ]\n",
                           t, lhs_block, c2, rhs_block);
            else
                emit_instr(ctx, "  %s = phi i1 [ true, %%%s ], [ %s, %%%s ]\n",
                           t, lhs_block, c2, rhs_block);
            const char *t2 = fresh_tmp(ctx);
            emit_instr(ctx, "  %s = zext i1 %s to i32\n", t2, t);
            return t2;
        }

        const char *lhs = codegen_expr(ctx, n->u.binop.lhs);
        const char *rhs = codegen_expr(ctx, n->u.binop.rhs);
        const char *llt_l = expr_lltype(ctx, n->u.binop.lhs, tb1, sizeof(tb1));
        const char *llt_r = expr_lltype(ctx, n->u.binop.rhs, tb2, sizeof(tb2));

        /* Pointer arithmetic: ptr+int, int+ptr, ptr-int → getelementptr.
         * ptr-ptr → ptrtoint both, sub i64, divide by element size → i64. */
        bool lhs_ptr = strcmp(llt_l, "ptr") == 0;
        bool rhs_ptr = strcmp(llt_r, "ptr") == 0;
        if ((lhs_ptr || rhs_ptr) &&
            (strcmp(op,"+")==0 || strcmp(op,"-")==0)) {
            if (lhs_ptr && rhs_ptr) {
                /* ptr - ptr: compute byte difference */
                const char *i1 = fresh_tmp(ctx);
                const char *i2 = fresh_tmp(ctx);
                const char *d  = fresh_tmp(ctx);
                emit_instr(ctx, "  %s = ptrtoint ptr %s to i64\n", i1, lhs);
                emit_instr(ctx, "  %s = ptrtoint ptr %s to i64\n", i2, rhs);
                emit_instr(ctx, "  %s = sub i64 %s, %s\n", d, i1, i2);
                /* Divide by element size to get element-count difference.
                 * Determine element type from lhs base if possible. */
                const char *elt = "i32"; /* default element size = 4 */
                if (n->u.binop.lhs->kind == NK_UNARY &&
                    strcmp(n->u.binop.lhs->u.unary.op,"&")==0) {
                    /* &arr[i] form: element type from array base */
                }
                int esz = 4;
                if (strcmp(elt,"i8")==0) esz = 1;
                else if (strcmp(elt,"i16")==0) esz = 2;
                else if (strcmp(elt,"i64")==0 || strcmp(elt,"double")==0) esz = 8;
                if (esz == 1) return d;  /* byte difference = element difference */
                const char *t = fresh_tmp(ctx);
                emit_instr(ctx, "  %s = sdiv i64 %s, %d\n", t, d, esz);
                return t;
            }
            /* ptr ± int: use getelementptr i8 (byte arithmetic) */
            const char *ptr_val = lhs_ptr ? lhs : rhs;
            const char *int_val = lhs_ptr ? rhs : lhs;
            const char *int_llt = lhs_ptr ? llt_r : llt_l;
            /* Promote index to i64 */
            const char *idx = cast_val(ctx, int_val,
                strcmp(int_llt,"i64")==0?"i64":"i32", "i64");
            if (strcmp(op,"-")==0) {
                /* ptr - n: negate the index */
                const char *ni = fresh_tmp(ctx);
                emit_instr(ctx, "  %s = sub i64 0, %s\n", ni, idx);
                idx = ni;
            }
            /* Determine element type: look up elt_lltype if base is a VAR */
            const char *elt_llt = "i8"; /* default: byte arithmetic */
            Node *ptr_node = lhs_ptr ? n->u.binop.lhs : n->u.binop.rhs;
            if (ptr_node->kind == NK_VAR) {
                LocalSym *ls = lookup_local(ctx, ptr_node->u.var.name);
                if (ls && ls->is_ptr && ls->elt_lltype[0])
                    elt_llt = ls->elt_lltype;
            }
            const char *t = fresh_tmp(ctx);
            emit_instr(ctx, "  %s = getelementptr inbounds %s, ptr %s, i64 %s\n",
                       t, elt_llt, ptr_val, idx);
            return t;
        }

        /* Pointer comparison: == != < <= > >= between ptrs (or ptr vs int) */
        if ((lhs_ptr || rhs_ptr) &&
            (strcmp(op,"==")==0||strcmp(op,"!=")==0||strcmp(op,"<")==0||
             strcmp(op,"<=")==0||strcmp(op,">")==0||strcmp(op,">=")==0)) {
            /* Ensure both operands are ptr type */
            const char *lv = lhs_ptr ? lhs : cast_val(ctx, lhs,
                strcmp(llt_l,"i64")==0?"i64":"i32", "ptr");
            const char *rv = rhs_ptr ? rhs : cast_val(ctx, rhs,
                strcmp(llt_r,"i64")==0?"i64":"i32", "ptr");
            const char *pred;
            if (strcmp(op,"==")==0)      pred="eq";
            else if (strcmp(op,"!=")==0) pred="ne";
            else if (strcmp(op,"<")==0)  pred="ult";
            else if (strcmp(op,"<=")==0) pred="ule";
            else if (strcmp(op,">")==0)  pred="ugt";
            else                         pred="uge";
            const char *ci = fresh_tmp(ctx);
            const char *t2 = fresh_tmp(ctx);
            emit_instr(ctx, "  %s = icmp %s ptr %s, %s\n", ci, pred, lv, rv);
            emit_instr(ctx, "  %s = zext i1 %s to i32\n", t2, ci);
            return t2;
        }

        bool fp_op = is_fp_lltype(llt_l) || is_fp_lltype(llt_r);
        const char *promoted;
        bool is_shift = (strcmp(op,"<<")==0||strcmp(op,">>")==0);
        if (fp_op) promoted = promote_lltype(llt_l, llt_r);
        else if (is_shift) {
            /* Shift result type follows the left operand (C rule) */
            promoted = (strcmp(llt_l,"i64")==0) ? "i64" : "i32";
        } else promoted = promote_lltype(llt_l, llt_r);

        if (!fp_op) {
            lhs = promote_to_i32(ctx, lhs, llt_l);
            rhs = promote_to_i32(ctx, rhs, llt_r);
            lhs = cast_val(ctx, lhs, strcmp(llt_l,"i64")==0?"i64":"i32", promoted);
            /* For shifts, rhs must be same type as lhs in LLVM */
            const char *rhs_src = is_shift ? promoted : (strcmp(llt_r,"i64")==0?"i64":"i32");
            rhs = cast_val(ctx, rhs, strcmp(llt_r,"i64")==0?"i64":"i32", rhs_src);
        } else {
            lhs = cast_val(ctx, lhs, llt_l, promoted);
            rhs = cast_val(ctx, rhs, llt_r, promoted);
        }

        /* Comparisons */
        if (strcmp(op,"==")==0||strcmp(op,"!=")==0||strcmp(op,"<")==0||
            strcmp(op,"<=")==0||strcmp(op,">")==0||strcmp(op,">=")==0) {
            const char *pred = "";
            if (fp_op) {
                if (strcmp(op,"==")==0) pred="oeq";
                else if (strcmp(op,"!=")==0) pred="one";
                else if (strcmp(op,"<")==0)  pred="olt";
                else if (strcmp(op,"<=")==0) pred="ole";
                else if (strcmp(op,">")==0)  pred="ogt";
                else                          pred="oge";
            } else {
                if (strcmp(op,"==")==0) pred="eq";
                else if (strcmp(op,"!=")==0) pred="ne";
                else if (strcmp(op,"<")==0)  pred="slt";
                else if (strcmp(op,"<=")==0) pred="sle";
                else if (strcmp(op,">")==0)  pred="sgt";
                else                          pred="sge";
            }
            const char *ci = fresh_tmp(ctx);
            const char *t  = fresh_tmp(ctx);
            const char *cmp_instr = fp_op ? "fcmp" : "icmp";
            emit_instr(ctx, "  %s = %s %s %s %s, %s\n",
                       ci, cmp_instr, pred, promoted, lhs, rhs);
            emit_instr(ctx, "  %s = zext i1 %s to i32\n", t, ci);
            return t;
        }

        /* Arithmetic / bitwise */
        const char *llop = NULL;
        if (fp_op) {
            if (strcmp(op,"+")==0) llop="fadd";
            else if (strcmp(op,"-")==0) llop="fsub";
            else if (strcmp(op,"*")==0) llop="fmul";
            else if (strcmp(op,"/")==0) llop="fdiv";
        } else {
            if (strcmp(op,"+")==0) llop="add";
            else if (strcmp(op,"-")==0) llop="sub";
            else if (strcmp(op,"*")==0) llop="mul";
            else if (strcmp(op,"/")==0) llop="sdiv";
            else if (strcmp(op,"%")==0) llop="srem";
            else if (strcmp(op,"&")==0) llop="and";
            else if (strcmp(op,"|")==0) llop="or";
            else if (strcmp(op,"^")==0) llop="xor";
            else if (strcmp(op,"<<")==0) llop="shl";
            else if (strcmp(op,">>")==0) llop="ashr";
        }
        if (!llop) {
            /* Unknown operator: return 0 */
            return "0";
        }
        const char *t = fresh_tmp(ctx);
        emit_instr(ctx, "  %s = %s %s %s, %s\n", t, llop, promoted, lhs, rhs);
        return t;
    }

    /* ---- Unary operator ---- */
    case NK_UNARY: {
        const char *op = n->u.unary.op;

        if (strcmp(op,"&")==0) {
            return codegen_lval(ctx, n->u.unary.expr);
        }

        if (strcmp(op,"*")==0) {
            const char *ptr = codegen_expr(ctx, n->u.unary.expr);
            /* Determine what type is at the pointed-to location.
             * Strip pre/post ++ / -- to reach the actual pointer base. */
            const char *elt_llt = "i32";
            Node *ptr_base = n->u.unary.expr;
            while (ptr_base->kind == NK_UNARY &&
                   (strcmp(ptr_base->u.unary.op,"++")==0 ||
                    strcmp(ptr_base->u.unary.op,"--")==0)) {
                ptr_base = ptr_base->u.unary.expr;
            }
            if (ptr_base->kind == NK_VAR) {
                LocalSym *ls = lookup_local(ctx, ptr_base->u.var.name);
                if (ls && ls->is_ptr && ls->elt_lltype[0]) elt_llt = ls->elt_lltype;
                else {
                    GlobalSym *gs = lookup_global(ctx, ptr_base->u.var.name);
                    if (gs && gs->is_ptr && gs->elt_lltype[0]) elt_llt = gs->elt_lltype;
                }
            } else if (ptr_base->kind == NK_CAST) {
                /* Deref of cast: element type = what (cast_ptr_lvl - 1) points to */
                int remaining = ptr_base->u.cast.ptr_lvl - 1;
                if (remaining >= 1) {
                    elt_llt = "ptr";
                } else if (remaining == 0 && ptr_base->u.cast.base_type) {
                    char etb[64];
                    elt_llt = lltype_of(ptr_base->u.cast.base_type, 0, etb, sizeof(etb));
                    /* pool-copy since etb is on the stack */
                    elt_llt = pool_dup(ctx, elt_llt);
                }
            } else if (ptr_base->kind == NK_MEMBER) {
                const char *fn = ptr_base->u.mbr.field;
                const char *tag = get_struct_tag(ctx, ptr_base->u.mbr.base,
                                                 ptr_base->u.mbr.arrow);
                if (tag) {
                    StructLayout *sl = lookup_struct(ctx, tag);
                    if (sl) {
                        for (int i = 0; i < sl->nfields; i++) {
                            if (strcmp(sl->fields[i].name, fn) == 0 &&
                                sl->fields[i].is_ptr && sl->fields[i].elt_lltype[0]) {
                                elt_llt = sl->fields[i].elt_lltype;
                                break;
                            }
                        }
                    }
                }
            }
            const char *t = fresh_tmp(ctx);
            emit_instr(ctx, "  %s = load %s, ptr %s\n", t, elt_llt, ptr);
            return t;
        }

        if (strcmp(op,"++")==0 || strcmp(op,"--")==0) {
            bool is_pre = n->u.unary.prefix;
            bool is_inc = (op[0]=='+');
            const char *slot = codegen_lval(ctx, n->u.unary.expr);
            char tb[64];
            const char *raw_llt = expr_lltype(ctx, n->u.unary.expr, tb, sizeof(tb));
            /* Load at the actual slot type, then promote for arithmetic */
            const char *old_raw = fresh_tmp(ctx);
            emit_instr(ctx, "  %s = load %s, ptr %s\n", old_raw, raw_llt, slot);
            /* Promote i8/i16 to i32 for arithmetic */
            bool small_int = (strcmp(raw_llt,"i8")==0||strcmp(raw_llt,"i16")==0);
            const char *arith_llt = small_int ? "i32" : raw_llt;
            const char *old = small_int ? cast_val(ctx, old_raw, raw_llt, "i32") : old_raw;
            const char *one = (strcmp(arith_llt,"double")==0||strcmp(arith_llt,"float")==0)
                              ? "1.0" : "1";
            const char *add_op = (strcmp(arith_llt,"double")==0||strcmp(arith_llt,"float")==0)
                                 ? (is_inc?"fadd":"fsub") : (is_inc?"add":"sub");
            const char *newv = fresh_tmp(ctx);
            if (strcmp(raw_llt, "ptr") == 0) {
                /* Pointer inc/dec: use GEP with element type */
                const char *elt_llt = "i8";
                if (n->u.unary.expr->kind == NK_VAR) {
                    LocalSym *ls2 = lookup_local(ctx, n->u.unary.expr->u.var.name);
                    if (ls2 && ls2->is_ptr && ls2->elt_lltype[0])
                        elt_llt = ls2->elt_lltype;
                }
                const char *delta = is_inc ? "1" : "-1";
                emit_instr(ctx, "  %s = getelementptr inbounds %s, ptr %s, i64 %s\n",
                           newv, elt_llt, old_raw, delta);
                emit_instr(ctx, "  store ptr %s, ptr %s\n", newv, slot);
            } else {
                emit_instr(ctx, "  %s = %s %s %s, %s\n", newv, add_op, arith_llt, old, one);
                /* Truncate back to original type if needed */
                const char *store_val = small_int
                    ? cast_val(ctx, newv, "i32", raw_llt) : newv;
                emit_instr(ctx, "  store %s %s, ptr %s\n", raw_llt, store_val, slot);
            }
            return is_pre ? newv : old;
        }

        const char *val = codegen_expr(ctx, n->u.unary.expr);
        char tb[64];
        const char *llt = expr_lltype(ctx, n->u.unary.expr, tb, sizeof(tb));
        val = promote_to_i32(ctx, val, llt);
        llt = strcmp(llt,"i8")==0||strcmp(llt,"i16")==0 ? "i32" : llt;

        if (strcmp(op,"-")==0) {
            const char *t = fresh_tmp(ctx);
            if (is_fp_lltype(llt))
                emit_instr(ctx, "  %s = fneg %s %s\n", t, llt, val);
            else
                emit_instr(ctx, "  %s = sub %s 0, %s\n", t, llt, val);
            return t;
        }
        if (strcmp(op,"+")==0) return val;
        if (strcmp(op,"!")==0) {
            /* !val: check val == 0 (or null for ptr), return 1 or 0 */
            const char *c  = fresh_tmp(ctx);
            const char *t  = fresh_tmp(ctx);
            if (strcmp(llt,"ptr")==0)
                emit_instr(ctx, "  %s = icmp eq ptr %s, null\n", c, val);
            else if (is_fp_lltype(llt))
                emit_instr(ctx, "  %s = fcmp oeq %s %s, 0.0\n", c, llt, val);
            else
                emit_instr(ctx, "  %s = icmp eq %s %s, 0\n", c, llt, val);
            emit_instr(ctx, "  %s = zext i1 %s to i32\n", t, c);
            return t;
        }
        if (strcmp(op,"~")==0) {
            const char *t = fresh_tmp(ctx);
            emit_instr(ctx, "  %s = xor %s %s, -1\n", t, llt, val);
            return t;
        }
        return val;
    }

    /* ---- Function call ---- */
    case NK_CALL: {
        /* Evaluate arguments first */
        const char **argv_vals = (const char **)xmalloc(
            (size_t)(n->u.call.nargs + 1) * sizeof(char *));
        char **argv_llt = (char **)xmalloc(
            (size_t)(n->u.call.nargs + 1) * sizeof(char *));
        for (int i = 0; i < n->u.call.nargs; i++) {
            argv_vals[i] = codegen_expr(ctx, n->u.call.args[i]);
            char tbuf[64];
            argv_llt[i] = xstrdup(expr_lltype(ctx, n->u.call.args[i], tbuf, sizeof(tbuf)));
            /* Promote i8/i16 arguments */
            const char *promoted = promote_to_i32(ctx, argv_vals[i], argv_llt[i]);
            if (promoted != argv_vals[i]) {
                free(argv_llt[i]);
                argv_llt[i] = xstrdup("i32");
                argv_vals[i] = promoted;
            }
        }

        /* Determine callee and return type */
        const char *ret_llt = "i32";
        bool variadic = false;
        char call_name[256] = "";
        bool is_direct = false;
        const char *decl_params_sig = NULL; /* fixed-param types from declaration */

        if (n->u.call.func->kind == NK_VAR) {
            const char *fname = n->u.call.func->u.var.name;

            /* Map __builtin_bswap{16,32,64} → LLVM intrinsics */
            if (strcmp(fname,"__builtin_bswap32")==0 && n->u.call.nargs==1) {
                const char *t = fresh_tmp(ctx);
                emit_instr(ctx, "  %s = call i32 @llvm.bswap.i32(i32 %s)\n",
                           t, argv_vals[0]);
                for (int i=0;i<n->u.call.nargs;i++) free(argv_llt[i]);
                free(argv_vals); free(argv_llt);
                return t;
            }
            if (strcmp(fname,"__builtin_bswap64")==0 && n->u.call.nargs==1) {
                const char *av = cast_val(ctx, argv_vals[0], argv_llt[0], "i64");
                const char *t = fresh_tmp(ctx);
                emit_instr(ctx, "  %s = call i64 @llvm.bswap.i64(i64 %s)\n",
                           t, av);
                for (int i=0;i<n->u.call.nargs;i++) free(argv_llt[i]);
                free(argv_vals); free(argv_llt);
                return t;
            }
            if (strcmp(fname,"__builtin_bswap16")==0 && n->u.call.nargs==1) {
                const char *av = cast_val(ctx, argv_vals[0], argv_llt[0], "i16");
                const char *t = fresh_tmp(ctx);
                emit_instr(ctx, "  %s = call i16 @llvm.bswap.i16(i16 %s)\n",
                           t, av);
                for (int i=0;i<n->u.call.nargs;i++) free(argv_llt[i]);
                free(argv_vals); free(argv_llt);
                return t;
            }

            /* __builtin_expect(x, expected) → just return x */
            if (strcmp(fname,"__builtin_expect")==0 && n->u.call.nargs >= 1) {
                const char *res = argv_vals[0];
                for (int i=0;i<n->u.call.nargs;i++) free(argv_llt[i]);
                free(argv_vals); free(argv_llt);
                return res;
            }
            /* __builtin_object_size(ptr, type) → return -1 (unknown = max) */
            if (strcmp(fname,"__builtin_object_size")==0) {
                const char *t = fresh_tmp(ctx);
                emit_instr(ctx, "  %s = add i32 0, -1\n", t);
                for (int i=0;i<n->u.call.nargs;i++) free(argv_llt[i]);
                free(argv_vals); free(argv_llt);
                return t;
            }
            /* __builtin_fabs(x) → fabs(x) via llvm intrinsic */
            if (strcmp(fname,"__builtin_fabs")==0 && n->u.call.nargs==1) {
                const char *av = cast_val(ctx, argv_vals[0], argv_llt[0], "double");
                const char *t = fresh_tmp(ctx);
                emit_instr(ctx, "  %s = call double @llvm.fabs.f64(double %s)\n", t, av);
                for (int i=0;i<n->u.call.nargs;i++) free(argv_llt[i]);
                free(argv_vals); free(argv_llt);
                return t;
            }
            /* __builtin_inf() → double infinity constant */
            if (strcmp(fname,"__builtin_inf")==0 || strcmp(fname,"__builtin_inff")==0 ||
                strcmp(fname,"__builtin_infl")==0) {
                /* +infinity IEEE 754 */
                char *p = &ctx->tmp_pool[ctx->tmp_pool_used];
                int sz = snprintf(p, 32, "0x7FF0000000000000");
                ctx->tmp_pool_used += (size_t)(sz + 1);
                for (int i=0;i<n->u.call.nargs;i++) free(argv_llt[i]);
                free(argv_vals); free(argv_llt);
                return p;
            }
            /* __builtin_nan(s) → double NaN */
            if (strcmp(fname,"__builtin_nan")==0 || strcmp(fname,"__builtin_nanf")==0) {
                char *p = &ctx->tmp_pool[ctx->tmp_pool_used];
                int sz = snprintf(p, 32, "0x7FF8000000000000");
                ctx->tmp_pool_used += (size_t)(sz + 1);
                for (int i=0;i<n->u.call.nargs;i++) free(argv_llt[i]);
                free(argv_vals); free(argv_llt);
                return p;
            }
            /* __builtin___memcpy_chk(dst,src,n,size) → memcpy(dst,src,n) */
            if (strcmp(fname,"__builtin___memcpy_chk")==0 && n->u.call.nargs >= 3) {
                const char *t = fresh_tmp(ctx);
                const char *sn = cast_val(ctx, argv_vals[2], argv_llt[2], "i64");
                emit_instr(ctx, "  %s = call ptr @memcpy(ptr %s, ptr %s, i64 %s)\n",
                           t, argv_vals[0], argv_vals[1], sn);
                for (int i=0;i<n->u.call.nargs;i++) free(argv_llt[i]);
                free(argv_vals); free(argv_llt);
                return t;
            }
            /* __builtin___memmove_chk(dst,src,n,size) → memmove(dst,src,n) */
            if (strcmp(fname,"__builtin___memmove_chk")==0 && n->u.call.nargs >= 3) {
                const char *t = fresh_tmp(ctx);
                const char *sn = cast_val(ctx, argv_vals[2], argv_llt[2], "i64");
                emit_instr(ctx, "  %s = call ptr @memmove(ptr %s, ptr %s, i64 %s)\n",
                           t, argv_vals[0], argv_vals[1], sn);
                for (int i=0;i<n->u.call.nargs;i++) free(argv_llt[i]);
                free(argv_vals); free(argv_llt);
                return t;
            }
            /* __builtin___memset_chk(dst,c,n,size) → memset(dst,c,n) */
            if (strcmp(fname,"__builtin___memset_chk")==0 && n->u.call.nargs >= 3) {
                const char *t = fresh_tmp(ctx);
                const char *sn = cast_val(ctx, argv_vals[2], argv_llt[2], "i64");
                emit_instr(ctx, "  %s = call ptr @memset(ptr %s, i32 %s, i64 %s)\n",
                           t, argv_vals[0], argv_vals[1], sn);
                for (int i=0;i<n->u.call.nargs;i++) free(argv_llt[i]);
                free(argv_vals); free(argv_llt);
                return t;
            }
            /* __builtin___strncpy_chk(dst,src,n,size) → strncpy(dst,src,n) */
            if (strcmp(fname,"__builtin___strncpy_chk")==0 && n->u.call.nargs >= 3) {
                const char *t = fresh_tmp(ctx);
                const char *sn = cast_val(ctx, argv_vals[2], argv_llt[2], "i64");
                emit_instr(ctx, "  %s = call ptr @strncpy(ptr %s, ptr %s, i64 %s)\n",
                           t, argv_vals[0], argv_vals[1], sn);
                for (int i=0;i<n->u.call.nargs;i++) free(argv_llt[i]);
                free(argv_vals); free(argv_llt);
                return t;
            }
            /* __builtin___strncat_chk(dst,src,n,size) → strncat(dst,src,n) */
            if (strcmp(fname,"__builtin___strncat_chk")==0 && n->u.call.nargs >= 3) {
                const char *t = fresh_tmp(ctx);
                const char *sn = cast_val(ctx, argv_vals[2], argv_llt[2], "i64");
                emit_instr(ctx, "  %s = call ptr @strncat(ptr %s, ptr %s, i64 %s)\n",
                           t, argv_vals[0], argv_vals[1], sn);
                for (int i=0;i<n->u.call.nargs;i++) free(argv_llt[i]);
                free(argv_vals); free(argv_llt);
                return t;
            }
            /* __builtin___strcat_chk(dst,src,size) → strcat(dst,src) */
            if (strcmp(fname,"__builtin___strcat_chk")==0 && n->u.call.nargs >= 2) {
                const char *t = fresh_tmp(ctx);
                emit_instr(ctx, "  %s = call ptr @strcat(ptr %s, ptr %s)\n",
                           t, argv_vals[0], argv_vals[1]);
                for (int i=0;i<n->u.call.nargs;i++) free(argv_llt[i]);
                free(argv_vals); free(argv_llt);
                return t;
            }
            /* __builtin___strcpy_chk(dst,src,size) → strcpy(dst,src) */
            if (strcmp(fname,"__builtin___strcpy_chk")==0 && n->u.call.nargs >= 2) {
                const char *t = fresh_tmp(ctx);
                emit_instr(ctx, "  %s = call ptr @strcpy(ptr %s, ptr %s)\n",
                           t, argv_vals[0], argv_vals[1]);
                for (int i=0;i<n->u.call.nargs;i++) free(argv_llt[i]);
                free(argv_vals); free(argv_llt);
                return t;
            }
            /* __builtin___sprintf_chk(buf,flag,size,fmt,...) → sprintf(buf,fmt,...) */
            if (strcmp(fname,"__builtin___sprintf_chk")==0 && n->u.call.nargs >= 4) {
                /* args: buf=0, flag=1, size=2, fmt=3, variadic=4+ */
                const char *t = fresh_tmp(ctx);
                StrBuf sb2; sb_init(&sb2);
                StrBuf *save2 = ctx->cur_buf; ctx->cur_buf = &sb2;
                emit(ctx, "  %s = call i32 (ptr, ...) @sprintf(ptr %s, ptr %s",
                     t, argv_vals[0], argv_vals[3]);
                for (int i = 4; i < n->u.call.nargs; i++)
                    emit(ctx, ", %s %s", argv_llt[i], argv_vals[i]);
                emit(ctx, ")\n");
                ctx->cur_buf = save2;
                if (!ctx->terminated) {
                    if (ctx->cur_buf)
                        sb_puts(ctx->cur_buf, sb2.buf ? sb2.buf : "");
                    else
                        fputs(sb2.buf ? sb2.buf : "", ctx->out);
                }
                if (sb2.buf) free(sb2.buf);
                for (int i=0;i<n->u.call.nargs;i++) free(argv_llt[i]);
                free(argv_vals); free(argv_llt);
                return t;
            }

            /* If 'fname' is a local variable holding a function pointer (lltype == ptr),
             * use an indirect call instead of a direct @name call. */
            LocalSym *call_ls = lookup_local(ctx, fname);
            if (call_ls && (call_ls->is_ptr || strcmp(call_ls->lltype,"ptr")==0)) {
                /* is_direct remains false; fn_val will be set in !is_direct branch below */
            } else {
                FuncSig *fs = lookup_funcsig(ctx, fname);
                if (fs) {
                    ret_llt  = fs->ret_lltype;
                    variadic = fs->variadic;
                    if (fs->fixed_params_sig[0])
                        decl_params_sig = fs->fixed_params_sig;
                }
                snprintf(call_name, sizeof(call_name), "@%s", fname);
                is_direct = true;
            }
        }

        /* Build argument string (type+value pairs) and types-only string
         * (for the function type in variadic call instructions). */
        StrBuf arg_buf; sb_init(&arg_buf);
        StrBuf typ_buf; sb_init(&typ_buf);
        StrBuf *save = ctx->cur_buf; ctx->cur_buf = &arg_buf;
        for (int i = 0; i < n->u.call.nargs; i++) {
            if (i > 0) emit(ctx, ", ");
            emit(ctx, "%s %s", argv_llt[i], argv_vals[i]);
        }
        ctx->cur_buf = &typ_buf;
        for (int i = 0; i < n->u.call.nargs; i++) {
            if (i > 0) emit(ctx, ", ");
            emit(ctx, "%s", argv_llt[i]);
        }
        ctx->cur_buf = save;
        char *args_str  = sb_str(&arg_buf);
        char *types_str = sb_str(&typ_buf);

        for (int i = 0; i < n->u.call.nargs; i++) free(argv_llt[i]);
        free(argv_vals);
        free(argv_llt);

        /* Build function type for indirect calls */
        const char *ret_llt_use = ret_llt;
        const char *fn_val = "";
        if (!is_direct) {
            /* Strip redundant function-pointer dereference: (*fp)(args) == fp(args).
             * In C, dereferencing a function pointer is a no-op; just evaluate the inner expr. */
            Node *callee_node = n->u.call.func;
            if (callee_node->kind == NK_UNARY && strcmp(callee_node->u.unary.op,"*")==0) {
                callee_node = callee_node->u.unary.expr;
            }
            fn_val = codegen_expr(ctx, callee_node);
            /* For calls through a returned function pointer (NK_CALL callee returning ptr),
             * propagate "ptr" as the return type (covers fptr typedef chains like go()()). */
            if (callee_node->kind == NK_CALL) {
                char cltb[64];
                const char *callee_llt = expr_lltype(ctx, callee_node, cltb, sizeof(cltb));
                ret_llt_use = (strcmp(callee_llt, "ptr") == 0) ? "ptr" : "i32";
            } else {
                ret_llt_use = "i32";
            }
        }

        /* For variadic calls: function type uses declared fixed params only.
         * Use decl_params_sig if we have it (from FuncSig), else fall back
         * to types_str which includes all passed argument types. */
        const char *call_type_sig = (variadic && decl_params_sig) ? decl_params_sig : types_str;

        if (strcmp(ret_llt_use,"void")==0) {
            if (is_direct) {
                if (variadic)
                    emit_instr(ctx, "  call %s (%s, ...) %s(%s)\n",
                               ret_llt_use, call_type_sig, call_name, args_str);
                else
                    emit_instr(ctx, "  call %s %s(%s)\n",
                               ret_llt_use, call_name, args_str);
            } else {
                emit_instr(ctx, "  call %s %s(%s)\n",
                           ret_llt_use, fn_val, args_str);
            }
            free(args_str); free(types_str);
            return "0";
        }

        const char *t = fresh_tmp(ctx);
        if (is_direct) {
            if (variadic)
                emit_instr(ctx, "  %s = call %s (%s, ...) %s(%s)\n",
                           t, ret_llt_use, call_type_sig, call_name, args_str);
            else
                emit_instr(ctx, "  %s = call %s %s(%s)\n",
                           t, ret_llt_use, call_name, args_str);
        } else {
            emit_instr(ctx, "  %s = call %s %s(%s)\n",
                       t, ret_llt_use, fn_val, args_str);
        }
        free(args_str); free(types_str);
        return t;
    }

    /* ---- Array subscript ---- */
    case NK_INDEX: {
        const char *slot = codegen_lval(ctx, n);
        char tb[64];
        const char *elt_llt = expr_lltype(ctx, n, tb, sizeof(tb));
        if (strcmp(elt_llt,"ptr")==0) {
            /* Pointer array: load ptr element */
            const char *t = fresh_tmp(ctx);
            emit_instr(ctx, "  %s = load ptr, ptr %s\n", t, slot);
            return t;
        }
        const char *t = fresh_tmp(ctx);
        emit_instr(ctx, "  %s = load %s, ptr %s\n", t, elt_llt, slot);
        return t;
    }

    /* ---- Struct member ---- */
    case NK_MEMBER: {
        /* If the member field is an array, return the address (array decays to ptr).
         * Otherwise load the scalar value as usual. */
        const char *field_name = n->u.mbr.field;
        const char *tag = get_struct_tag(ctx, n->u.mbr.base, n->u.mbr.arrow);
        if (tag) {
            StructLayout *sl = lookup_struct(ctx, tag);
            if (sl) {
                for (int i = 0; i < sl->nfields; i++) {
                    if (strcmp(sl->fields[i].name, field_name) == 0) {
                        if (sl->fields[i].arr_sz > 0) {
                            /* Array field: decay to pointer, return address directly */
                            return codegen_lval(ctx, n);
                        }
                        break;
                    }
                }
            }
        }
        const char *slot = codegen_lval(ctx, n);
        char tb[64];
        const char *flt = expr_lltype(ctx, n, tb, sizeof(tb));
        const char *t = fresh_tmp(ctx);
        emit_instr(ctx, "  %s = load %s, ptr %s\n", t, flt, slot);
        return t;
    }

    /* ---- Cast ---- */
    case NK_CAST: {
        char tb[64];
        const char *to_llt = lltype_of(n->u.cast.base_type, n->u.cast.ptr_lvl,
                                        tb, sizeof(tb));
        const char *from_llt = expr_lltype(ctx, n->u.cast.expr, tb1, sizeof(tb1));
        const char *val = codegen_expr(ctx, n->u.cast.expr);
        val = promote_to_i32(ctx, val, from_llt);
        if (strcmp(from_llt,"i8")==0||strcmp(from_llt,"i16")==0) from_llt="i32";
        return cast_val(ctx, val, from_llt, to_llt);
    }

    /* ---- Sizeof ---- */
    case NK_SIZEOF: {
        int sz = 0;
        if (n->u.szof.base_type) {
            sz = type_size(n->u.szof.base_type, n->u.szof.ptr_lvl);
        } else if (n->u.szof.expr) {
            Node *se = n->u.szof.expr;
            /* sizeof(array_var): return total array size, not pointer size */
            if (se->kind == NK_VAR) {
                LocalSym *ls = lookup_local(ctx, se->u.var.name);
                if (ls && ls->arr_sz != 0) {
                    sz = ls->arr_sz * lltype_byte_size(ls->elt_lltype);
                } else if (!ls) {
                    GlobalSym *gs = lookup_global(ctx, se->u.var.name);
                    if (gs && gs->arr_sz != 0)
                        sz = gs->arr_sz * lltype_byte_size(gs->elt_lltype);
                }
            }
            if (sz == 0) {
                /* sizeof(expr): determine from expr type */
                char tb[64];
                const char *llt = expr_lltype(ctx, se, tb, sizeof(tb));
                if (strcmp(llt,"i64")==0||strcmp(llt,"double")==0||strcmp(llt,"ptr")==0)
                    sz = 8;
                else if (strcmp(llt,"i16")==0) sz = 2;
                else if (strcmp(llt,"i8")==0)  sz = 1;
                else sz = 4;
            }
        }
        char *p = &ctx->tmp_pool[ctx->tmp_pool_used];
        int n2 = snprintf(p, 16, "%d", sz);
        ctx->tmp_pool_used += (size_t)(n2 + 1);
        return p;
    }

    /* ---- Ternary ?: ---- */
    case NK_TERNARY: {
        const char *cond_val = codegen_expr(ctx, n->u.tern.cond);
        char tb[64];
        const char *cond_llt = expr_lltype(ctx, n->u.tern.cond, tb, sizeof(tb));
        const char *ci = emit_truth_check(ctx, cond_val, cond_llt);

        const char *lthen = fresh_label(ctx, "tern_then");
        const char *lelse = fresh_label(ctx, "tern_else");
        const char *lend  = fresh_label(ctx, "tern_end");

        emit_term(ctx, "  br i1 %s, label %%%s, label %%%s\n", ci, lthen, lelse);
        emit_label(ctx, lthen);
        const char *tv = codegen_expr(ctx, n->u.tern.then_e);
        char tb2[64];
        const char *then_llt = expr_lltype(ctx, n->u.tern.then_e, tb2, sizeof(tb2));
        const char *lthen_end = pool_dup(ctx, lthen);
        (void)lthen_end;
        emit_term(ctx, "  br label %%%s\n", lend);
        emit_label(ctx, lelse);
        const char *ev = codegen_expr(ctx, n->u.tern.else_e);
        const char *lelse_end = pool_dup(ctx, lelse);
        (void)lelse_end;
        emit_term(ctx, "  br label %%%s\n", lend);
        emit_label(ctx, lend);
        const char *t = fresh_tmp(ctx);
        /* Normalize values to phi type: for ptr phi, replace integer "0" with "null" */
        char tb3[64];
        const char *else_llt = expr_lltype(ctx, n->u.tern.else_e, tb3, sizeof(tb3));
        bool phi_is_ptr = (strcmp(then_llt,"ptr")==0);
        /* If types differ, use the pointer type for the phi */
        const char *phi_llt = then_llt;
        if (strcmp(else_llt,"ptr")==0 && strcmp(then_llt,"ptr")!=0) phi_llt = "ptr";
        const char *tv_use = tv, *ev_use = ev;
        if (strcmp(phi_llt,"ptr")==0) {
            /* Integer "0" → "null" for ptr phi */
            if (strcmp(tv,"0")==0) tv_use = "null";
            if (strcmp(ev,"0")==0) ev_use = "null";
        }
        (void)phi_is_ptr;
        emit_instr(ctx, "  %s = phi %s [ %s, %%%s ], [ %s, %%%s ]\n",
                   t, phi_llt, tv_use, lthen, ev_use, lelse);
        return t;
    }

    /* ---- Assignment expression ---- */
    case NK_ASSIGN_EXPR: {
        const char *op  = n->u.asgn.op;
        Node *tgt = n->u.asgn.tgt;
        const char *slot = codegen_lval(ctx, tgt);
        char tb[64];
        const char *tgt_llt = expr_lltype(ctx, tgt, tb, sizeof(tb));
        /* Keep the actual memory type for load/store (no i8→i32 promotion).
         * cast_val will trunc/sext as needed. */
        const char *arith_llt = (strcmp(tgt_llt,"i8")==0||strcmp(tgt_llt,"i16")==0)
                                 ? "i32" : tgt_llt;

        if (strcmp(op,"=")==0) {
            const char *rhs = codegen_expr(ctx, n->u.asgn.expr);
            const char *rlt = expr_lltype(ctx, n->u.asgn.expr, tb, sizeof(tb));
            rhs = promote_to_i32(ctx, rhs, rlt);
            rhs = cast_val(ctx, rhs,
                           (strcmp(rlt,"i8")==0||strcmp(rlt,"i16")==0)?"i32":rlt,
                           tgt_llt);
            emit_instr(ctx, "  store %s %s, ptr %s\n", tgt_llt, rhs, slot);
            return rhs;
        }

        /* Compound assignment: cur op= rhs.
         * Load/store uses tgt_llt (actual memory type: i8/i16/i32/...).
         * Arithmetic uses arith_llt (promoted: i8/i16 → i32). */
        const char *cur = fresh_tmp(ctx);
        emit_instr(ctx, "  %s = load %s, ptr %s\n", cur, tgt_llt, slot);
        /* Promote loaded value to arith type */
        cur = promote_to_i32(ctx, cur, tgt_llt);
        const char *rhs = codegen_expr(ctx, n->u.asgn.expr);
        const char *rlt = expr_lltype(ctx, n->u.asgn.expr, tb, sizeof(tb));

        /* Pointer arithmetic compound: use GEP instead of add/sub */
        if (strcmp(tgt_llt, "ptr") == 0 &&
            (strcmp(op,"+=")==0 || strcmp(op,"-=")==0)) {
            const char *elt_llt = "i8";
            if (tgt->kind == NK_VAR) {
                LocalSym *ls = lookup_local(ctx, tgt->u.var.name);
                if (ls && ls->is_ptr && ls->elt_lltype[0]) elt_llt = ls->elt_lltype;
            }
            const char *idx = cast_val(ctx, rhs,
                strcmp(rlt,"i64")==0?"i64":"i32", "i64");
            if (strcmp(op,"-=")==0) {
                const char *ni = fresh_tmp(ctx);
                emit_instr(ctx, "  %s = sub i64 0, %s\n", ni, idx);
                idx = ni;
            }
            const char *newv2 = fresh_tmp(ctx);
            emit_instr(ctx, "  %s = getelementptr inbounds %s, ptr %s, i64 %s\n",
                       newv2, elt_llt, cur, idx);
            emit_instr(ctx, "  store ptr %s, ptr %s\n", newv2, slot);
            return newv2;
        }

        rhs = promote_to_i32(ctx, rhs, rlt);
        rhs = cast_val(ctx, rhs,
                       (strcmp(rlt,"i8")==0||strcmp(rlt,"i16")==0)?"i32":rlt,
                       arith_llt);

        const char *llop = NULL;
        bool fp_op = is_fp_lltype(arith_llt);
        if (strcmp(op,"+=")==0) llop = fp_op?"fadd":"add";
        else if (strcmp(op,"-=")==0) llop = fp_op?"fsub":"sub";
        else if (strcmp(op,"*=")==0) llop = fp_op?"fmul":"mul";
        else if (strcmp(op,"/=")==0) llop = fp_op?"fdiv":"sdiv";
        else if (strcmp(op,"%=")==0) llop = "srem";
        else if (strcmp(op,"&=")==0) llop = "and";
        else if (strcmp(op,"|=")==0) llop = "or";
        else if (strcmp(op,"^=")==0) llop = "xor";
        else if (strcmp(op,"<<=")==0) llop = "shl";
        else if (strcmp(op,">>=")==0) llop = "ashr";

        if (!llop) return cur;
        const char *newv = fresh_tmp(ctx);
        emit_instr(ctx, "  %s = %s %s %s, %s\n", newv, llop, arith_llt, cur, rhs);
        /* Truncate arith result back to actual memory type before storing */
        const char *store_val = cast_val(ctx, newv, arith_llt, tgt_llt);
        emit_instr(ctx, "  store %s %s, ptr %s\n", tgt_llt, store_val, slot);
        return store_val;
    }

    /* ---- Statement expression ({ ... }) ---- */
    case NK_STMT_EXPR: {
        const char *last = "0";
        for (int i = 0; i < n->u.se.n; i++) {
            if (n->u.se.stmts[i]->kind == NK_EXPR_STMT)
                last = codegen_expr(ctx, n->u.se.stmts[i]->u.expr_wrap.expr);
            else
                codegen_stmt(ctx, n->u.se.stmts[i]);
        }
        return last;
    }

    /* ---- Compound literal (T){...} – not fully supported, return 0 ---- */
    case NK_COMPOUND_LIT:
        return "0";

    default:
        return "0";
    }
}

/* ================================================================
 * codegen_stmt – emit statement code
 * ================================================================ */

/* Forward-declare scan so codegen_stmt can call it for hoisting */
static void hoist_alloca(IRCtx *ctx, Node *n);

/* Recursively scan an expression node for NK_STMT_EXPR / NK_DECL nodes
 * and hoist their declarations.  This handles GNU ({...}) inside expressions. */
static void hoist_alloca_expr(IRCtx *ctx, Node *n) {
    if (!n) return;
    switch (n->kind) {
    case NK_STMT_EXPR:
        for (int i = 0; i < n->u.se.n; i++)
            hoist_alloca(ctx, n->u.se.stmts[i]);
        break;
    case NK_TERNARY:
        hoist_alloca_expr(ctx, n->u.tern.cond);
        hoist_alloca_expr(ctx, n->u.tern.then_e);
        hoist_alloca_expr(ctx, n->u.tern.else_e);
        break;
    case NK_BINOP:
        hoist_alloca_expr(ctx, n->u.binop.lhs);
        hoist_alloca_expr(ctx, n->u.binop.rhs);
        break;
    case NK_UNARY:
        hoist_alloca_expr(ctx, n->u.unary.expr);
        break;
    case NK_ASSIGN_EXPR:
        hoist_alloca_expr(ctx, n->u.asgn.tgt);
        hoist_alloca_expr(ctx, n->u.asgn.expr);
        break;
    case NK_CALL:
        hoist_alloca_expr(ctx, n->u.call.func);
        for (int i = 0; i < n->u.call.nargs; i++)
            hoist_alloca_expr(ctx, n->u.call.args[i]);
        break;
    default:
        break;
    }
}

static void hoist_alloca(IRCtx *ctx, Node *n) {
    if (!n) return;
    if (n->kind == NK_DECL) {
        char tb[64], etb[64];
        const char *llt = lltype_of(n->u.decl.base_type, n->u.decl.ptr_lvl,
                                     tb, sizeof(tb));
        /* Element type: what the pointer points to (ptr_lvl - 1) */
        int elt_pl = n->u.decl.ptr_lvl > 0 ? n->u.decl.ptr_lvl - 1 : 0;
        const char *elt_llt = lltype_of(n->u.decl.base_type, elt_pl, etb, sizeof(etb));
        bool is_ptr = n->u.decl.ptr_lvl > 0;

        if (n->u.decl.is_static) {
            /* Local static: emit as a global variable, register as global.
             * Use slot @fn_name.var_name so codegen uses global reference. */
            char gname[256];
            snprintf(gname, sizeof(gname), "%s.%s", ctx->cur_fn_name, n->u.decl.name);
            /* Compute initial value */
            /* Struct/union types require zeroinitializer, not "0" */
            const char *init_str = (llt[0] == '%' || llt[0] == '[') ? "zeroinitializer" : "0";
            char init_buf[64];
            if (n->u.decl.init) {
                Node *iv = n->u.decl.init;
                if (iv->kind == NK_INT_LIT) {
                    snprintf(init_buf, sizeof(init_buf), "%" PRId64, iv->u.ival.val);
                    init_str = init_buf;
                } else if (iv->kind == NK_FLOAT_LIT) {
                    fmt_double_llvm(init_buf, sizeof(init_buf), iv->u.flt.val);
                    init_str = init_buf;
                }
            }
            /* Emit global directly to output (hoist_alloca runs before the
             * function header is written, so this appears before the define). */
            fprintf(ctx->out, "@%s = internal global %s %s\n", gname, llt, init_str);
            /* Register as global and also as local with @gname slot */
            if (ctx->n_globals < MAX_GLOBALS) {
                GlobalSym *gs = &ctx->globals[ctx->n_globals++];
                strncpy(gs->name, n->u.decl.name, sizeof(gs->name)-1);
                strncpy(gs->lltype, llt, sizeof(gs->lltype)-1);
                strncpy(gs->elt_lltype, elt_llt, sizeof(gs->elt_lltype)-1);
                gs->arr_sz = n->u.decl.arr_sz;
                gs->is_ptr = is_ptr;
                /* Also register the global under its mangled name so @fn.var is findable */
                if (ctx->n_globals < MAX_GLOBALS) {
                    GlobalSym *gs2 = &ctx->globals[ctx->n_globals++];
                    strncpy(gs2->name, gname, sizeof(gs2->name)-1);
                    strncpy(gs2->lltype, llt, sizeof(gs2->lltype)-1);
                    strncpy(gs2->elt_lltype, elt_llt, sizeof(gs2->elt_lltype)-1);
                    gs2->arr_sz = n->u.decl.arr_sz;
                    gs2->is_ptr = is_ptr;
                }
            }
            /* Register local with slot = @gname so loads/stores use global ref */
            char slot_buf[256];
            snprintf(slot_buf, sizeof(slot_buf), "@%s", gname);
            LocalSym *ls2 = &ctx->locals[ctx->n_locals < MAX_LOCALS ? ctx->n_locals++ : ctx->n_locals-1];
            strncpy(ls2->name, n->u.decl.name, sizeof(ls2->name)-1);
            strncpy(ls2->slot, slot_buf, sizeof(ls2->slot)-1);
            strncpy(ls2->lltype, llt, sizeof(ls2->lltype)-1);
            strncpy(ls2->elt_lltype, elt_llt, sizeof(ls2->elt_lltype)-1);
            ls2->arr_sz = n->u.decl.arr_sz;
            ls2->is_ptr = is_ptr;
            return;
        }

        LocalSym *ls = register_local(ctx, n->u.decl.name, llt, elt_llt,
                                       n->u.decl.arr_sz, is_ptr);
        if (!ls) return;
        StrBuf *save = ctx->cur_buf;
        ctx->cur_buf = &ctx->alloca_buf;
        if (n->u.decl.arr_sz > 0) {
            emit(ctx, "  %s = alloca [%d x %s]\n",
                 ls->slot, n->u.decl.arr_sz, llt);
        } else {
            emit(ctx, "  %s = alloca %s\n", ls->slot, llt);
        }
        ctx->cur_buf = save;
        return;
    }
    /* Recurse into block-like nodes */
    switch (n->kind) {
    case NK_BLOCK:
        for (int i = 0; i < n->u.block.n; i++)
            hoist_alloca(ctx, n->u.block.stmts[i]);
        break;
    case NK_IF:
        hoist_alloca(ctx, n->u.iff.then_s);
        hoist_alloca(ctx, n->u.iff.else_s);
        break;
    case NK_WHILE:
        hoist_alloca(ctx, n->u.whl.body);
        break;
    case NK_FOR:
        hoist_alloca(ctx, n->u.forr.init);
        hoist_alloca(ctx, n->u.forr.body);
        break;
    case NK_DO_WHILE:
        hoist_alloca(ctx, n->u.dowh.body);
        break;
    case NK_SWITCH:
        hoist_alloca(ctx, n->u.sw.body);
        break;
    case NK_CASE:
        hoist_alloca(ctx, n->u.cas.stmt);
        break;
    case NK_DEFAULT:
        hoist_alloca(ctx, n->u.dft.stmt);
        break;
    case NK_LABEL:
        hoist_alloca(ctx, n->u.lbl.stmt);
        break;
    case NK_STMT_EXPR:
        for (int i = 0; i < n->u.se.n; i++)
            hoist_alloca(ctx, n->u.se.stmts[i]);
        break;
    case NK_TERNARY:
        hoist_alloca_expr(ctx, n->u.tern.then_e);
        hoist_alloca_expr(ctx, n->u.tern.else_e);
        break;
    case NK_EXPR_STMT:
        hoist_alloca_expr(ctx, n->u.expr_wrap.expr);
        break;
    default:
        break;
    }
}

/* ================================================================
 * Switch dispatch helpers
 * ================================================================ */

#define MAX_SW_CASES 128

/* Collect case integer values from a switch body AST.
 * Does NOT recurse into nested NK_SWITCH nodes.
 * Returns count of cases found (up to max_out). */
static int collect_switch_cases(Node *n, int64_t *out, int max_out) {
    if (!n || max_out <= 0) return 0;
    if (n->kind == NK_SWITCH) return 0; /* don't cross nested switch */
    if (n->kind == NK_CASE) {
        int64_t cv = 0;
        if (n->u.cas.val) {
            if (n->u.cas.val->kind == NK_INT_LIT)
                cv = n->u.cas.val->u.ival.val;
            else if (n->u.cas.val->kind == NK_INT_LIT && 0) /* char lits use NK_INT_LIT too */
                cv = n->u.cas.val->u.ival.val;
        }
        out[0] = cv;
        /* Also scan the case sub-statement for further nested cases */
        int extra = 0;
        if (max_out > 1 && n->u.cas.stmt)
            extra = collect_switch_cases(n->u.cas.stmt, out + 1, max_out - 1);
        return 1 + extra;
    }
    int count = 0;
    switch (n->kind) {
        case NK_BLOCK:
            for (int i = 0; i < n->u.block.n && count < max_out; i++)
                count += collect_switch_cases(n->u.block.stmts[i], out + count, max_out - count);
            break;
        case NK_LABEL:
            count += collect_switch_cases(n->u.lbl.stmt, out + count, max_out - count);
            break;
        case NK_IF:
            count += collect_switch_cases(n->u.iff.then_s, out + count, max_out - count);
            count += collect_switch_cases(n->u.iff.else_s, out + count, max_out - count);
            break;
        case NK_DO_WHILE:
            count += collect_switch_cases(n->u.dowh.body, out + count, max_out - count);
            break;
        case NK_WHILE:
            count += collect_switch_cases(n->u.whl.body, out + count, max_out - count);
            break;
        case NK_FOR:
            count += collect_switch_cases(n->u.forr.body, out + count, max_out - count);
            break;
        case NK_DEFAULT:
            count += collect_switch_cases(n->u.dft.stmt, out + count, max_out - count);
            break;
        default:
            break;
    }
    return count;
}

static void codegen_stmt(IRCtx *ctx, Node *n) {
    if (!n) return;
    char tb[64];

    switch (n->kind) {
    /* ---- Local declaration ---- */
    case NK_DECL: {
        /* Static locals are handled as globals (initialized at module load) */
        if (n->u.decl.is_static) break;
        LocalSym *ls = lookup_local(ctx, n->u.decl.name);
        if (!ls) break;  /* alloca already hoisted */
        const char *llt = ls->lltype;
        if (n->u.decl.nInits > 0 && ls->arr_sz != 0) {
            /* Brace-initialized array: emit per-element stores */
            int arr_sz = ls->arr_sz;
            const char *elt_llt = ls->elt_lltype[0] ? ls->elt_lltype : llt;
            /* Compute actual size from designators when -1 */
            if (arr_sz == -1) {
                int max_idx = -1, next_seq = 0;
                for (int j = 0; j < n->u.decl.nInits; j++) {
                    const char *f = n->u.decl.init_fields ? n->u.decl.init_fields[j] : NULL;
                    int cur_idx;
                    if (f && f[0] == '[') {
                        cur_idx = (int)strtol(f + 1, NULL, 10);
                        next_seq = cur_idx + 1;
                    } else {
                        cur_idx = next_seq++;
                    }
                    if (cur_idx > max_idx) max_idx = cur_idx;
                }
                arr_sz = max_idx + 1;
            }
            /* Build ordered array of values (positional + designated) */
            Node **ordered = (Node **)xcalloc((size_t)(arr_sz > 0 ? arr_sz : n->u.decl.nInits),
                                              sizeof(Node *));
            int next_seq = 0;
            for (int j = 0; j < n->u.decl.nInits; j++) {
                const char *f = n->u.decl.init_fields ? n->u.decl.init_fields[j] : NULL;
                int target;
                if (f && f[0] == '[') {
                    target = (int)strtol(f + 1, NULL, 10);
                    next_seq = target + 1;
                } else {
                    target = next_seq++;
                }
                if (arr_sz > 0 && target < arr_sz)
                    ordered[target] = n->u.decl.init_list[j];
            }
            int emit_sz = arr_sz > 0 ? arr_sz : n->u.decl.nInits;
            for (int j = 0; j < emit_sz; j++) {
                const char *idx_s = fresh_tmp(ctx);
                emit_instr(ctx, "  %s = getelementptr inbounds %s, ptr %s, i64 %d\n",
                           idx_s, elt_llt, ls->slot, j);
                if (ordered[j]) {
                    char ftb[64];
                    const char *val = codegen_expr(ctx, ordered[j]);
                    const char *from_llt2 = expr_lltype(ctx, ordered[j], ftb, sizeof(ftb));
                    val = promote_to_i32(ctx, val, from_llt2);
                    if (strcmp(from_llt2,"i8")==0||strcmp(from_llt2,"i16")==0) from_llt2="i32";
                    val = cast_val(ctx, val, from_llt2, elt_llt);
                    emit_instr(ctx, "  store %s %s, ptr %s\n", elt_llt, val, idx_s);
                } else {
                    /* No initializer for this position: store 0 */
                    emit_instr(ctx, "  store %s 0, ptr %s\n", elt_llt, idx_s);
                }
            }
            free(ordered);
        } else if (n->u.decl.nInits > 0) {
            /* Brace-initialized struct (arr_sz == 0): emit zeroinitializer or
             * per-field stores.  Common case: {0} → store T zeroinitializer. */
            emit_instr(ctx, "  store %s zeroinitializer, ptr %s\n", llt, ls->slot);
        } else if (n->u.decl.init) {
            if (ls->arr_sz != 0) {
                /* String literal init for char array */
                if (n->u.decl.init->kind == NK_STR_LIT) {
                    size_t slen = n->u.decl.init->u.str.len + 1;
                    int emit_sz = ls->arr_sz > 0 ? ls->arr_sz : (int)slen;
                    for (int j = 0; j < emit_sz; j++) {
                        unsigned char c = (j < (int)slen)
                                          ? (unsigned char)n->u.decl.init->u.str.bytes[j] : 0;
                        const char *idx_s = fresh_tmp(ctx);
                        emit_instr(ctx, "  %s = getelementptr inbounds i8, ptr %s, i64 %d\n",
                                   idx_s, ls->slot, j);
                        emit_instr(ctx, "  store i8 %d, ptr %s\n", (int)c, idx_s);
                    }
                }
                break;
            }
            const char *val = codegen_expr(ctx, n->u.decl.init);
            const char *from_llt = expr_lltype(ctx, n->u.decl.init, tb, sizeof(tb));
            val = promote_to_i32(ctx, val, from_llt);
            if (strcmp(from_llt,"i8")==0||strcmp(from_llt,"i16")==0) from_llt="i32";
            val = cast_val(ctx, val, from_llt, llt);
            emit_instr(ctx, "  store %s %s, ptr %s\n", llt, val, ls->slot);
        }
        break;
    }

    /* ---- Return ---- */
    case NK_RETURN: {
        if (!n->u.expr_wrap.expr) {
            emit_term(ctx, "  ret void\n");
        } else {
            const char *val = codegen_expr(ctx, n->u.expr_wrap.expr);
            const char *from_llt = expr_lltype(ctx, n->u.expr_wrap.expr, tb, sizeof(tb));
            val = promote_to_i32(ctx, val, from_llt);
            if (strcmp(from_llt,"i8")==0||strcmp(from_llt,"i16")==0) from_llt="i32";
            val = cast_val(ctx, val, from_llt, ctx->cur_ret_lltype);
            emit_term(ctx, "  ret %s %s\n", ctx->cur_ret_lltype, val);
        }
        break;
    }

    /* ---- Expression statement ---- */
    case NK_EXPR_STMT:
        codegen_expr(ctx, n->u.expr_wrap.expr);
        break;

    /* ---- Block ---- */
    case NK_BLOCK:
        for (int i = 0; i < n->u.block.n; i++)
            codegen_stmt(ctx, n->u.block.stmts[i]);
        break;

    /* ---- If / else ---- */
    case NK_IF: {
        const char *cval = codegen_expr(ctx, n->u.iff.cond);
        const char *cllt = expr_lltype(ctx, n->u.iff.cond, tb, sizeof(tb));
        const char *ci = emit_truth_check(ctx, cval, cllt);

        const char *lthen  = fresh_label(ctx, "then");
        const char *lelse  = fresh_label(ctx, "else");
        const char *lmerge = fresh_label(ctx, "ifend");

        if (n->u.iff.else_s) {
            emit_term(ctx, "  br i1 %s, label %%%s, label %%%s\n", ci, lthen, lelse);
        } else {
            emit_term(ctx, "  br i1 %s, label %%%s, label %%%s\n", ci, lthen, lmerge);
        }

        emit_label(ctx, lthen);
        codegen_stmt(ctx, n->u.iff.then_s);
        emit_term(ctx, "  br label %%%s\n", lmerge);

        if (n->u.iff.else_s) {
            emit_label(ctx, lelse);
            codegen_stmt(ctx, n->u.iff.else_s);
            emit_term(ctx, "  br label %%%s\n", lmerge);
        }

        emit_label(ctx, lmerge);
        break;
    }

    /* ---- While ---- */
    case NK_WHILE: {
        const char *lcond = fresh_label(ctx, "while_cond");
        const char *lbody = fresh_label(ctx, "while_body");
        const char *lend  = fresh_label(ctx, "while_end");

        if (ctx->n_loop_stk < MAX_LOOP_DEPTH) {
            strncpy(ctx->break_label[ctx->n_loop_stk], lend,  127);
            strncpy(ctx->cont_label [ctx->n_loop_stk], lcond, 127);
            ctx->n_loop_stk++;
        }

        emit_term(ctx, "  br label %%%s\n", lcond);
        emit_label(ctx, lcond);

        const char *cval = codegen_expr(ctx, n->u.whl.cond);
        const char *cllt = expr_lltype(ctx, n->u.whl.cond, tb, sizeof(tb));
        const char *ci = emit_truth_check(ctx, cval, cllt);
        emit_term(ctx, "  br i1 %s, label %%%s, label %%%s\n", ci, lbody, lend);

        emit_label(ctx, lbody);
        codegen_stmt(ctx, n->u.whl.body);
        emit_term(ctx, "  br label %%%s\n", lcond);

        emit_label(ctx, lend);
        if (ctx->n_loop_stk > 0) ctx->n_loop_stk--;
        break;
    }

    /* ---- For ---- */
    case NK_FOR: {
        const char *lcond = fresh_label(ctx, "for_cond");
        const char *lbody = fresh_label(ctx, "for_body");
        const char *lpost = fresh_label(ctx, "for_post");
        const char *lend  = fresh_label(ctx, "for_end");

        if (ctx->n_loop_stk < MAX_LOOP_DEPTH) {
            strncpy(ctx->break_label[ctx->n_loop_stk], lend,  127);
            strncpy(ctx->cont_label [ctx->n_loop_stk], lpost, 127);
            ctx->n_loop_stk++;
        }

        /* init */
        if (n->u.forr.init) {
            if (n->u.forr.init->kind == NK_DECL)
                codegen_stmt(ctx, n->u.forr.init);
            else
                codegen_expr(ctx, n->u.forr.init);
        }
        emit_term(ctx, "  br label %%%s\n", lcond);

        emit_label(ctx, lcond);
        if (n->u.forr.cond) {
            const char *cval = codegen_expr(ctx, n->u.forr.cond);
            const char *cllt = expr_lltype(ctx, n->u.forr.cond, tb, sizeof(tb));
            const char *ci = emit_truth_check(ctx, cval, cllt);
            emit_term(ctx, "  br i1 %s, label %%%s, label %%%s\n", ci, lbody, lend);
        } else {
            emit_term(ctx, "  br label %%%s\n", lbody);
        }

        emit_label(ctx, lbody);
        codegen_stmt(ctx, n->u.forr.body);
        emit_term(ctx, "  br label %%%s\n", lpost);

        emit_label(ctx, lpost);
        if (n->u.forr.post)
            codegen_expr(ctx, n->u.forr.post);
        emit_term(ctx, "  br label %%%s\n", lcond);

        emit_label(ctx, lend);
        if (ctx->n_loop_stk > 0) ctx->n_loop_stk--;
        break;
    }

    /* ---- Do-while ---- */
    case NK_DO_WHILE: {
        const char *lbody = fresh_label(ctx, "do_body");
        const char *lcond = fresh_label(ctx, "do_cond");
        const char *lend  = fresh_label(ctx, "do_end");

        if (ctx->n_loop_stk < MAX_LOOP_DEPTH) {
            strncpy(ctx->break_label[ctx->n_loop_stk], lend,  127);
            strncpy(ctx->cont_label [ctx->n_loop_stk], lcond, 127);
            ctx->n_loop_stk++;
        }

        emit_term(ctx, "  br label %%%s\n", lbody);
        emit_label(ctx, lbody);
        codegen_stmt(ctx, n->u.dowh.body);
        emit_term(ctx, "  br label %%%s\n", lcond);

        emit_label(ctx, lcond);
        const char *cval = codegen_expr(ctx, n->u.dowh.cond);
        const char *cllt = expr_lltype(ctx, n->u.dowh.cond, tb, sizeof(tb));
        const char *ci = emit_truth_check(ctx, cval, cllt);
        emit_term(ctx, "  br i1 %s, label %%%s, label %%%s\n", ci, lbody, lend);

        emit_label(ctx, lend);
        if (ctx->n_loop_stk > 0) ctx->n_loop_stk--;
        break;
    }

    /* ---- Switch ---- */
    case NK_SWITCH: {
        /* Collect case values from body AST (before emitting any IR) */
        int64_t case_vals[MAX_SW_CASES];
        int n_cases = collect_switch_cases(n->u.sw.body, case_vals, MAX_SW_CASES);

        /* Compute switch expression */
        const char *val = codegen_expr(ctx, n->u.sw.expr);
        const char *vlt = expr_lltype(ctx, n->u.sw.expr, tb, sizeof(tb));
        val = promote_to_i32(ctx, val, vlt);
        /* Determine actual type after promotion */
        const char *sw_llt = (strcmp(vlt,"i64")==0) ? "i64" : "i32";
        /* val must survive the function – it's already in tmp_pool */

        /* Generate unique labels for this switch instance */
        const char *lend  = pool_dup(ctx, fresh_label(ctx, "sw_end"));
        const char *ldef  = pool_dup(ctx, fresh_label(ctx, "sw_def"));
        const char *lcpfx = pool_dup(ctx, fresh_label(ctx, "sw_case"));

        /* Save/restore switch context for nesting */
        char old_sw_end[128], old_sw_def[128], old_case_pfx[64];
        bool old_in_sw       = ctx->in_switch;
        bool old_def_emitted = ctx->sw_def_emitted;
        strncpy(old_sw_end,   ctx->sw_end_label, sizeof(old_sw_end)-1);
        strncpy(old_sw_def,   ctx->sw_def_label, sizeof(old_sw_def)-1);
        strncpy(old_case_pfx, ctx->sw_case_pfx,  sizeof(old_case_pfx)-1);
        strncpy(ctx->sw_end_label, lend,  sizeof(ctx->sw_end_label)-1);
        strncpy(ctx->sw_def_label, ldef,  sizeof(ctx->sw_def_label)-1);
        strncpy(ctx->sw_case_pfx,  lcpfx, sizeof(ctx->sw_case_pfx)-1);
        ctx->in_switch      = true;
        ctx->sw_def_emitted = false;

        if (ctx->n_loop_stk < MAX_LOOP_DEPTH) {
            strncpy(ctx->break_label[ctx->n_loop_stk], lend, 127);
            ctx->cont_label[ctx->n_loop_stk][0] = '\0';
            ctx->n_loop_stk++;
        }

        /* Emit if-chain dispatch: jump to matching case label or default */
        const char *ldisp = pool_dup(ctx, fresh_label(ctx, "sw_disp"));
        emit_term(ctx, "  br label %%%s\n", ldisp);
        emit_label(ctx, ldisp);
        for (int i = 0; i < n_cases; i++) {
            char case_lbl[256];
            snprintf(case_lbl, sizeof(case_lbl), "%s_%" PRId64, lcpfx, case_vals[i]);
            /* Fallback label: next check block or default */
            char next_lbl[256];
            if (i + 1 < n_cases)
                snprintf(next_lbl, sizeof(next_lbl), "%s_chk%d", ldisp, i + 1);
            else
                strncpy(next_lbl, ldef, sizeof(next_lbl) - 1);
            const char *ct = fresh_tmp(ctx);
            emit(ctx, "  %s = icmp eq %s %s, %" PRId64 "\n", ct, sw_llt, val, case_vals[i]);
            emit_term(ctx, "  br i1 %s, label %%%s, label %%%s\n", ct, case_lbl, next_lbl);
            if (i + 1 < n_cases)
                emit_label(ctx, next_lbl);
        }
        if (n_cases == 0) {
            /* No cases → jump straight to default */
            emit_term(ctx, "  br label %%%s\n", ldef);
        }

        /* Dead body block: code before the first case is unreachable from dispatch.
         * We still need a valid block here so the body emits correct IR. */
        const char *body_lbl = fresh_label(ctx, "sw_body");
        emit_label(ctx, body_lbl);
        codegen_stmt(ctx, n->u.sw.body);

        /* Close out body flow */
        emit_term(ctx, "  br label %%%s\n", lend);

        /* Emit default label only if NK_DEFAULT didn't already emit it */
        if (!ctx->sw_def_emitted) {
            emit_label(ctx, ldef);
            emit_term(ctx, "  br label %%%s\n", lend);
        }
        emit_label(ctx, lend);

        strncpy(ctx->sw_end_label, old_sw_end,   sizeof(ctx->sw_end_label)-1);
        strncpy(ctx->sw_def_label, old_sw_def,   sizeof(ctx->sw_def_label)-1);
        strncpy(ctx->sw_case_pfx,  old_case_pfx, sizeof(ctx->sw_case_pfx)-1);
        ctx->in_switch      = old_in_sw;
        ctx->sw_def_emitted = old_def_emitted;
        if (ctx->n_loop_stk > 0) ctx->n_loop_stk--;
        break;
    }

    /* ---- Case ---- */
    case NK_CASE: {
        /* Emit a unique case label using the current switch's prefix */
        int64_t cv = 0;
        if (n->u.cas.val && n->u.cas.val->kind == NK_INT_LIT)
            cv = n->u.cas.val->u.ival.val;
        else if (n->u.cas.val && n->u.cas.val->kind == NK_VAR) {
            bool found;
            cv = lookup_enum(ctx, n->u.cas.val->u.var.name, &found);
        }
        char case_lbl[192];
        if (ctx->sw_case_pfx[0])
            snprintf(case_lbl, sizeof(case_lbl), "%s_%" PRId64, ctx->sw_case_pfx, cv);
        else
            snprintf(case_lbl, sizeof(case_lbl), "case_%" PRId64, cv);
        emit_term(ctx, "  br label %%%s\n", case_lbl);
        emit_label(ctx, case_lbl);
        codegen_stmt(ctx, n->u.cas.stmt);
        break;
    }

    /* ---- Default ---- */
    case NK_DEFAULT: {
        const char *dlbl = ctx->sw_def_label[0] ? ctx->sw_def_label : "sw_default";
        emit_term(ctx, "  br label %%%s\n", dlbl);
        emit_label(ctx, dlbl);
        ctx->sw_def_emitted = true;  /* NK_SWITCH must not re-emit this label */
        codegen_stmt(ctx, n->u.dft.stmt);
        break;
    }

    /* ---- Break ---- */
    case NK_BREAK:
        if (ctx->n_loop_stk > 0) {
            const char *bl = ctx->break_label[ctx->n_loop_stk - 1];
            if (bl[0]) emit_term(ctx, "  br label %%%s\n", bl);
        }
        break;

    /* ---- Continue ---- */
    case NK_CONTINUE:
        if (ctx->n_loop_stk > 0) {
            const char *cl = ctx->cont_label[ctx->n_loop_stk - 1];
            if (cl[0]) emit_term(ctx, "  br label %%%s\n", cl);
        }
        break;

    /* ---- Goto ---- */
    case NK_GOTO:
        /* Prefix goto label with "_goto_" to avoid name clash with local variables */
        emit_term(ctx, "  br label %%_goto_%s\n", n->u.got.name);
        break;

    /* ---- Label ---- */
    case NK_LABEL: {
        /* Use "_goto_" prefix to avoid name clash with local LLVM values */
        char lbl_buf[256];
        snprintf(lbl_buf, sizeof(lbl_buf), "_goto_%s", n->u.lbl.name);
        emit_term(ctx, "  br label %%%s\n", lbl_buf);
        emit_label(ctx, lbl_buf);
        codegen_stmt(ctx, n->u.lbl.stmt);
        break;
    }

    /* ---- Empty ---- */
    case NK_EMPTY:
        break;

    default:
        break;
    }
}

/* ================================================================
 * emit_function – emit a single function definition
 * ================================================================ */

static void emit_function(IRCtx *ctx, Node *n) {
    if (!n || n->kind != NK_FUNCTION) return;

    /* Reset per-function state */
    ctx->n_locals      = 0;
    ctx->tmp_counter   = 0;
    ctx->label_counter = 0;
    ctx->n_loop_stk    = 0;
    ctx->in_switch      = false;
    ctx->sw_def_emitted = false;
    ctx->terminated     = false;
    ctx->tmp_pool_used  = 0;
    sb_init(&ctx->alloca_buf);
    sb_init(&ctx->body_buf);

    char tb[64];
    const char *ret_llt = lltype_of(n->u.fn.ret_type, n->u.fn.ret_ptr_lvl,
                                     tb, sizeof(tb));
    strncpy(ctx->cur_ret_lltype, ret_llt, sizeof(ctx->cur_ret_lltype)-1);

    /* Build parameter string for the signature.
     * Skip void-typed params: int f(void) → no params in LLVM IR.
     * For forward declarations (no body), omit parameter names: LLVM 'declare'
     * statements must not have duplicate parameter names (e.g. multiple
     * _Nullable-stripped names colliding), and names are not required there. */
    bool is_decl = (n->u.fn.body == NULL);
    StrBuf param_buf; sb_init(&param_buf);
    int nemitted = 0;
    for (int i = 0; i < n->u.fn.nparams; i++) {
        Param *p = &n->u.fn.params[i];
        if (p->base_type && strcmp(p->base_type, "void") == 0 && p->ptr_lvl == 0)
            continue;  /* C (void) = no parameters */
        if (nemitted++ > 0) sb_puts(&param_buf, ", ");
        char ptb[64];
        const char *plt = lltype_of(p->base_type, p->ptr_lvl, ptb, sizeof(ptb));
        sb_puts(&param_buf, plt);
        if (!is_decl && p->name) { sb_puts(&param_buf, " %p_"); sb_puts(&param_buf, p->name); }
    }
    if (n->u.fn.variadic) {
        if (nemitted > 0) sb_puts(&param_buf, ", ");
        sb_puts(&param_buf, "...");
    }
    char *params_str = sb_str(&param_buf);

    /* If this is a declaration (no body), emit declare */
    if (!n->u.fn.body) {
        const char *linkage = n->u.fn.is_extern ? "" : "";
        (void)linkage;
        fprintf(ctx->out, "declare %s @%s(%s)\n", ret_llt, n->u.fn.name, params_str);
        free(params_str);
        return;
    }

    /* ---- Function definition ---- */
    const char *linkage = n->u.fn.is_static ? "internal " : "";

    /* First: hoist all local variable allocas by pre-scanning the body */
    /* Track current function name (needed for local static variable naming) */
    strncpy(ctx->cur_fn_name, n->u.fn.name, sizeof(ctx->cur_fn_name)-1);
    /* Register parameters as locals first */
    for (int i = 0; i < n->u.fn.nparams; i++) {
        Param *p = &n->u.fn.params[i];
        if (!p->name) continue;
        char ptb[64], eptb[64];
        const char *plt = lltype_of(p->base_type, p->ptr_lvl, ptb, sizeof(ptb));
        int elt_pl = p->ptr_lvl > 0 ? p->ptr_lvl - 1 : 0;
        const char *eplt = lltype_of(p->base_type, elt_pl, eptb, sizeof(eptb));
        bool is_ptr = p->ptr_lvl > 0;
        LocalSym *ls = register_local(ctx, p->name, plt, eplt, 0, is_ptr);
        if (!ls) continue;
        /* Emit alloca for parameter */
        StrBuf *save = ctx->cur_buf;
        ctx->cur_buf = &ctx->alloca_buf;
        emit(ctx, "  %s = alloca %s\n", ls->slot, plt);
        ctx->cur_buf = save;
    }

    /* Hoist all body declarations */
    hoist_alloca(ctx, n->u.fn.body);

    /* Now emit the body */
    ctx->cur_buf = &ctx->body_buf;
    strncpy(ctx->cur_block, "entry", sizeof(ctx->cur_block)-1);

    /* Store parameters into their alloca slots */
    for (int i = 0; i < n->u.fn.nparams; i++) {
        Param *p = &n->u.fn.params[i];
        if (!p->name) continue;
        char ptb[64];
        const char *plt = lltype_of(p->base_type, p->ptr_lvl, ptb, sizeof(ptb));
        LocalSym *ls = lookup_local(ctx, p->name);
        if (!ls) continue;
        emit_instr(ctx, "  store %s %%p_%s, ptr %s\n", plt, p->name, ls->slot);
    }

    /* Codegen the body */
    codegen_stmt(ctx, n->u.fn.body);

    /* Ensure function ends with a terminator */
    if (!ctx->terminated) {
        if (strcmp(ret_llt,"void")==0)
            emit_term(ctx, "  ret void\n");
        else
            emit_term(ctx, "  ret %s 0\n", ret_llt);
    }

    ctx->cur_buf = NULL;

    /* ---- Write output ---- */
    fprintf(ctx->out, "\ndefine %s%s @%s(%s) {\n", linkage, ret_llt,
            n->u.fn.name, params_str);
    fprintf(ctx->out, "entry:\n");
    /* alloca preamble */
    if (ctx->alloca_buf.buf && ctx->alloca_buf.len > 0)
        fwrite(ctx->alloca_buf.buf, 1, ctx->alloca_buf.len, ctx->out);
    /* body */
    if (ctx->body_buf.buf && ctx->body_buf.len > 0)
        fwrite(ctx->body_buf.buf, 1, ctx->body_buf.len, ctx->out);
    fprintf(ctx->out, "}\n");

    free(ctx->alloca_buf.buf); ctx->alloca_buf.buf = NULL;
    free(ctx->body_buf.buf);   ctx->body_buf.buf   = NULL;
    free(params_str);
}

/* ================================================================
 * Helpers for emitting global constant expressions
 * ================================================================ */

/* Emit an LLVM constant value for a global initializer element.
 * Returns the type string (e.g. "i32") for emission in struct literals. */
static const char *emit_gvar_const(IRCtx *ctx, Node *iv, const char *field_llt,
                                    char *outbuf, size_t outsz) {
    if (!iv) {
        /* No initializer: zero */
        if (strcmp(field_llt,"ptr")==0) { snprintf(outbuf,outsz,"null"); return "ptr"; }
        if (strcmp(field_llt,"i64")==0) { snprintf(outbuf,outsz,"0"); return "i64"; }
        if (strcmp(field_llt,"double")==0) { snprintf(outbuf,outsz,"0.0"); return "double"; }
        /* Struct/union/array types need zeroinitializer */
        if (field_llt[0] == '%' || (field_llt[0] == '[')) {
            snprintf(outbuf, outsz, "zeroinitializer");
            return field_llt[0] ? field_llt : "i32";
        }
        snprintf(outbuf,outsz,"0"); return field_llt[0] ? field_llt : "i32";
    }
    if (iv->kind == NK_INT_LIT) {
        snprintf(outbuf,outsz,"%" PRId64, iv->u.ival.val);
        if (strcmp(field_llt,"i64")==0) return "i64";
        if (strcmp(field_llt,"i8")==0)  return "i8";
        if (strcmp(field_llt,"i16")==0) return "i16";
        return "i32";
    }
    if (iv->kind == NK_FLOAT_LIT) {
        fmt_double_llvm(outbuf, outsz, iv->u.flt.val);
        return strcmp(field_llt,"float")==0 ? "float" : "double";
    }
    if (iv->kind == NK_UNARY && strcmp(iv->u.unary.op,"&")==0
        && iv->u.unary.expr->kind == NK_VAR) {
        snprintf(outbuf,outsz,"@%s", iv->u.unary.expr->u.var.name);
        return "ptr";
    }
    if (iv->kind == NK_STR_LIT) {
        const char *sname = register_strlit(ctx, iv->u.str.bytes, iv->u.str.len);
        snprintf(outbuf,outsz,"getelementptr ([%zu x i8], ptr %s, i64 0, i64 0)",
                 iv->u.str.len+1, sname);
        return "ptr";
    }
    /* Cast: strip the cast and evaluate the inner expression */
    if (iv->kind == NK_CAST) {
        return emit_gvar_const(ctx, iv->u.cast.expr, field_llt, outbuf, outsz);
    }
    /* Unary minus: negative integer constant */
    if (iv->kind == NK_UNARY && strcmp(iv->u.unary.op,"-")==0) {
        if (iv->u.unary.expr->kind == NK_INT_LIT) {
            snprintf(outbuf,outsz,"%" PRId64, -iv->u.unary.expr->u.ival.val);
            return field_llt[0] ? field_llt : "i32";
        }
    }
    /* Fallback: zero */
    snprintf(outbuf,outsz,"0");
    return field_llt[0] ? field_llt : "i32";
}

/* Emit an inline struct/union constant body (without @name = global prefix).
 * Writes "%struct.T { f1_type f1_val, f2_type f2_val, ... }" to out.
 * init_list/init_fields/nInits are the initializers from the C source. */
static void emit_struct_const_inline(IRCtx *ctx, const char *llt, StructLayout *sl,
                                     Node **init_list, char **init_fields, int nInits);

/* Build an index of physical (non-logical) fields in sl.
 * Returns count; phys_idx[] receives indices into sl->fields. */
static int phys_fields(StructLayout *sl, int *phys_idx, int max) {
    int cnt = 0;
    for (int f = 0; f < sl->nfields && cnt < max; f++) {
        if (!sl->fields[f].is_anon_logical)
            phys_idx[cnt++] = f;
    }
    return cnt;
}

static void emit_struct_const_inline(IRCtx *ctx, const char *llt, StructLayout *sl,
                                     Node **init_list, char **init_fields, int nInits) {
    /* Build physical field indices */
    int phys_idx[MAX_STRUCT_FIELDS];
    int phys_cnt = phys_fields(sl, phys_idx, MAX_STRUCT_FIELDS);

    /* Determine if designated or positional */
    bool designated = false;
    for (int i = 0; i < nInits; i++) {
        if (init_fields && init_fields[i]) { designated = true; break; }
    }

    /* Build value array indexed by physical field position */
    Node **fv = (Node **)xcalloc((size_t)phys_cnt, sizeof(Node *));
    if (designated) {
        for (int j = 0; j < nInits; j++) {
            const char *fname = init_fields ? init_fields[j] : NULL;
            if (!fname) {
                if (j < phys_cnt) fv[j] = init_list[j];
                continue;
            }
            /* Find field by name (may be a logical sub-field → use its llvm index) */
            for (int f = 0; f < sl->nfields; f++) {
                if (strcmp(sl->fields[f].name, fname) == 0) {
                    int target_idx = sl->fields[f].index;
                    /* Map target LLVM index to physical field slot */
                    for (int k = 0; k < phys_cnt; k++) {
                        if (sl->fields[phys_idx[k]].index == target_idx) {
                            fv[k] = init_list[j]; break;
                        }
                    }
                    break;
                }
            }
        }
    } else {
        for (int j = 0; j < nInits && j < phys_cnt; j++)
            fv[j] = init_list[j];
    }

    /* Emit struct constant */
    fprintf(ctx->out, "%s { ", llt);
    char vbuf[256];
    for (int k = 0; k < phys_cnt; k++) {
        if (k > 0) fprintf(ctx->out, ", ");
        FieldInfo *fi = &sl->fields[phys_idx[k]];
        /* Construct the actual LLVM field type (arrays have [N x T] not just T) */
        char arr_type_buf[96] = {0};
        const char *flt;
        if (fi->arr_sz > 0 && fi->name[0] != '\0') {
            snprintf(arr_type_buf, sizeof(arr_type_buf), "[%d x %s]", fi->arr_sz, fi->lltype);
            flt = arr_type_buf;
        } else {
            flt = fi->lltype;
        }
        Node *iv = fv[k];

        if (fi->name[0] == '\0') {
            /* Anonymous container field */
            if (strncmp(flt, "%union.", 7) == 0) {
                /* Union: emit as byte array constant */
                const char *utag = flt + 7;
                StructLayout *usl = lookup_struct(ctx, utag);
                int nbytes = usl ? usl->byte_size : 4;
                int64_t val = 0;
                if (iv && iv->kind == NK_INT_LIT) val = iv->u.ival.val;
                fprintf(ctx->out, "%s { [%d x i8] c\"", flt, nbytes);
                for (int b = 0; b < nbytes; b++) {
                    uint8_t byte = (uint8_t)((val >> (8*b)) & 0xFF);
                    fprintf(ctx->out, "\\%02X", byte);
                }
                fprintf(ctx->out, "\" }");
            } else if (strncmp(flt, "%struct.", 8) == 0 &&
                       iv && iv->kind == NK_GLOBAL_VAR && iv->u.gvar.nInits > 0) {
                /* Anonymous struct container with nested init: recurse */
                StructLayout *nsl = lookup_struct(ctx, flt + 8);
                if (nsl)
                    emit_struct_const_inline(ctx, flt, nsl,
                                             iv->u.gvar.init_list,
                                             iv->u.gvar.init_fields,
                                             iv->u.gvar.nInits);
                else
                    fprintf(ctx->out, "%s zeroinitializer", flt);
            } else {
                fprintf(ctx->out, "%s zeroinitializer", flt);
            }
        } else if (iv && iv->kind == NK_GLOBAL_VAR && iv->u.gvar.nInits > 0) {
            /* Named field with nested init: recurse for struct, expand for array */
            if (strncmp(flt, "%struct.", 8) == 0) {
                StructLayout *nsl = lookup_struct(ctx, flt + 8);
                if (nsl)
                    emit_struct_const_inline(ctx, flt, nsl,
                                             iv->u.gvar.init_list,
                                             iv->u.gvar.init_fields,
                                             iv->u.gvar.nInits);
                else
                    fprintf(ctx->out, "%s zeroinitializer", flt);
            } else if (fi->arr_sz > 0) {
                /* Array field: emit [N x T] [T v1, T v2, ...] */
                const char *elt_llt = fi->elt_lltype[0] ? fi->elt_lltype : "i32";
                int n = fi->arr_sz;
                Node **inits = iv->u.gvar.init_list;
                int nInits2 = iv->u.gvar.nInits;
                fprintf(ctx->out, "%s [", flt);
                for (int j = 0; j < n; j++) {
                    if (j > 0) fprintf(ctx->out, ", ");
                    Node *elem2 = (j < nInits2) ? inits[j] : NULL;
                    const char *evtype = emit_gvar_const(ctx, elem2, elt_llt, vbuf, sizeof(vbuf));
                    fprintf(ctx->out, "%s %s", evtype, vbuf);
                }
                fprintf(ctx->out, "]");
            } else {
                fprintf(ctx->out, "%s zeroinitializer", flt);
            }
        } else if (fi->arr_sz > 0) {
            /* Array field with a single scalar initializer (or no init):
             * emit zeroinitializer for the array – this avoids "[N x T] 0"
             * which LLVM rejects (array types can't use scalar constant 0). */
            const char *elt_llt = fi->elt_lltype[0] ? fi->elt_lltype : "i32";
            if (!iv || (iv->kind == NK_INT_LIT && iv->u.ival.val == 0)) {
                fprintf(ctx->out, "%s zeroinitializer", flt);
            } else {
                /* Single non-zero value: put it in element[0], rest zero */
                char ev[256];
                const char *evtype = emit_gvar_const(ctx, iv, elt_llt, ev, sizeof(ev));
                fprintf(ctx->out, "%s [%s %s", flt, evtype, ev);
                for (int j = 1; j < fi->arr_sz; j++)
                    fprintf(ctx->out, ", %s 0", elt_llt);
                fprintf(ctx->out, "]");
            }
        } else {
            const char *vtype = emit_gvar_const(ctx, iv, flt, vbuf, sizeof(vbuf));
            fprintf(ctx->out, "%s %s", vtype, vbuf);
        }
    }
    fprintf(ctx->out, " }");
    free(fv);
}

/* Emit a struct constant initializer as a global variable declaration */
static void emit_gvar_struct_init(IRCtx *ctx, Node *n, const char *llt,
                                   const char *linkage) {
    const char *tag = NULL;
    if (strncmp(llt,"%struct.",8)==0) tag = llt+8;
    else if (strncmp(llt,"%union.",7)==0) {
        fprintf(ctx->out, "@%s = %sglobal %s zeroinitializer\n",
                n->u.gvar.name, linkage, llt);
        return;
    }
    StructLayout *sl = tag ? lookup_struct(ctx, tag) : NULL;
    if (!sl || n->u.gvar.nInits == 0) {
        fprintf(ctx->out, "@%s = %sglobal %s zeroinitializer\n",
                n->u.gvar.name, linkage, llt);
        return;
    }

    fprintf(ctx->out, "@%s = %sglobal ", n->u.gvar.name, linkage);
    emit_struct_const_inline(ctx, llt, sl,
                              n->u.gvar.init_list,
                              n->u.gvar.init_fields,
                              n->u.gvar.nInits);
    fprintf(ctx->out, "\n");
}

/* ================================================================
 * emit_global_var – emit a single global variable
 * ================================================================ */

static void emit_global_var(IRCtx *ctx, Node *n) {
    if (!n || n->kind != NK_GLOBAL_VAR) return;

    char tb[64];
    const char *llt = lltype_of(n->u.gvar.base_type, n->u.gvar.ptr_lvl,
                                  tb, sizeof(tb));
    const char *linkage = n->u.gvar.is_extern ? "external " :
                          n->u.gvar.is_static  ? "internal " : "";

    if (n->u.gvar.is_extern) {
        fprintf(ctx->out, "@%s = external global %s\n", n->u.gvar.name, llt);
        return;
    }

    /* Determine initial value */
    const char *init = "0";
    char init_buf[64];
    if (n->u.gvar.init) {
        Node *iv = n->u.gvar.init;
        if (iv->kind == NK_INT_LIT) {
            if (strcmp(llt,"double")==0 || strcmp(llt,"float")==0) {
                /* Integer literal initializing a float global: format as LLVM float */
                fmt_double_llvm(init_buf, sizeof(init_buf), (double)iv->u.ival.val);
            } else {
                snprintf(init_buf, sizeof(init_buf), "%" PRId64, iv->u.ival.val);
            }
            init = init_buf;
        } else if (iv->kind == NK_FLOAT_LIT) {
            fmt_double_llvm(init_buf, sizeof(init_buf), iv->u.flt.val);
            init = init_buf;
        } else if (iv->kind == NK_STR_LIT) {
            if (n->u.gvar.arr_sz > 0 || strcmp(n->u.gvar.base_type,"char")==0) {
                /* char s[] = "abc" or char s[N] = "abc" → emit as [N x i8] array */
                size_t slen = iv->u.str.len + 1; /* include null terminator */
                size_t emit_sz = (n->u.gvar.arr_sz > 0) ? (size_t)n->u.gvar.arr_sz : slen;
                fprintf(ctx->out, "@%s = %sglobal [%zu x i8] c\"", n->u.gvar.name, linkage, emit_sz);
                for (size_t si = 0; si < emit_sz; si++) {
                    unsigned char c = (si < slen) ? (unsigned char)iv->u.str.bytes[si] : 0;
                    if (c == '"' || c == '\\') fprintf(ctx->out, "\\%02X", c);
                    else if (c < 32 || c > 126) fprintf(ctx->out, "\\%02X", c);
                    else fputc(c, ctx->out);
                }
                fprintf(ctx->out, "\"\n");
                return;
            }
            /* Global initialized with string pointer: ptr to string literal */
            const char *sname = register_strlit(ctx, iv->u.str.bytes, iv->u.str.len);
            fprintf(ctx->out, "@%s = %sglobal ptr getelementptr ([%zu x i8], ptr %s, i64 0, i64 0)\n",
                    n->u.gvar.name, linkage, iv->u.str.len+1, sname);
            return;
        } else if (iv->kind == NK_UNARY && strcmp(iv->u.unary.op,"&")==0
                   && iv->u.unary.expr->kind == NK_VAR) {
            /* &global_var: pointer initializer */
            fprintf(ctx->out, "@%s = %sglobal ptr @%s\n",
                    n->u.gvar.name, linkage, iv->u.unary.expr->u.var.name);
            return;
        } else if (iv->kind == NK_CAST) {
            /* Strip cast: recurse on inner expression */
            Node *inner = iv;
            while (inner->kind == NK_CAST) inner = inner->u.cast.expr;
            if (inner->kind == NK_INT_LIT) {
                snprintf(init_buf, sizeof(init_buf), "%" PRId64, inner->u.ival.val);
                init = init_buf;
            } else if (inner->kind == NK_FLOAT_LIT) {
                fmt_double_llvm(init_buf, sizeof(init_buf), inner->u.flt.val);
                init = init_buf;
            }
            /* else: leave init = "0" */
        } else if (iv->kind == NK_UNARY && strcmp(iv->u.unary.op,"-")==0
                   && iv->u.unary.expr->kind == NK_INT_LIT) {
            /* Unary minus integer constant */
            snprintf(init_buf, sizeof(init_buf), "%" PRId64, -iv->u.unary.expr->u.ival.val);
            init = init_buf;
        } else {
            init = "0";
        }
    }

    /* Array global: emit [N x elt_llt] with initializer or zeroinitializer.
     * arr_sz == -1 means the size was inferred from the initializer; compute it now. */
    int effective_arr_sz = (n->u.gvar.arr_sz == -1)
                           ? compute_global_arr_sz(n)
                           : n->u.gvar.arr_sz;
    if (effective_arr_sz > 0) {
        char etb[64];
        int elt_ptr_lvl = n->u.gvar.ptr_lvl > 0 ? n->u.gvar.ptr_lvl - 1 : 0;
        const char *elt_llt = lltype_of(n->u.gvar.base_type, elt_ptr_lvl, etb, sizeof(etb));
        int arr_sz = effective_arr_sz;
        if (n->u.gvar.nInits == 0 && !n->u.gvar.init) {
            fprintf(ctx->out, "@%s = %sglobal [%d x %s] zeroinitializer\n",
                    n->u.gvar.name, linkage, arr_sz, elt_llt);
        } else {
            /* Build a position-ordered array, respecting designated index "[N]" fields */
            Node **ordered = (Node **)xcalloc((size_t)arr_sz, sizeof(Node *));
            int next_idx = 0;  /* position for next non-designated initializer */
            for (int j = 0; j < n->u.gvar.nInits; j++) {
                const char *fname = (n->u.gvar.init_fields && n->u.gvar.init_fields[j])
                                    ? n->u.gvar.init_fields[j] : NULL;
                int target = next_idx;
                if (fname && fname[0] == '[') {
                    /* Parse "[N]" field name */
                    target = (int)strtol(fname + 1, NULL, 10);
                }
                if (target < arr_sz) {
                    ordered[target] = n->u.gvar.init_list[j];
                }
                next_idx = target + 1;
            }
            fprintf(ctx->out, "@%s = %sglobal [%d x %s] [",
                    n->u.gvar.name, linkage, arr_sz, elt_llt);
            char vbuf[256];
            bool elt_is_struct = (strncmp(elt_llt,"%struct.",8)==0 ||
                                   strncmp(elt_llt,"%union.",7)==0);
            for (int i = 0; i < arr_sz; i++) {
                if (i > 0) fprintf(ctx->out, ", ");
                Node *elem = ordered[i];
                if (elt_is_struct) {
                    if (!elem) {
                        fprintf(ctx->out, "%s zeroinitializer", elt_llt);
                    } else if (elem->kind == NK_GLOBAL_VAR && elem->u.gvar.nInits > 0) {
                        /* Nested struct init: emit_struct_const_inline already emits "%struct.S { ... }" */
                        StructLayout *nsl = strncmp(elt_llt,"%struct.",8)==0
                            ? lookup_struct(ctx, elt_llt+8) : NULL;
                        if (nsl) {
                            emit_struct_const_inline(ctx, elt_llt, nsl,
                                                     elem->u.gvar.init_list,
                                                     elem->u.gvar.init_fields,
                                                     elem->u.gvar.nInits);
                        } else {
                            fprintf(ctx->out, "%s zeroinitializer", elt_llt);
                        }
                    } else {
                        fprintf(ctx->out, "%s zeroinitializer", elt_llt);
                    }
                } else {
                    const char *vtype = emit_gvar_const(ctx, elem, elt_llt, vbuf, sizeof(vbuf));
                    fprintf(ctx->out, "%s %s", vtype, vbuf);
                }
            }
            fprintf(ctx->out, "]\n");
            free(ordered);
        }
        return;
    }

    /* Struct/union with brace initializer */
    if (n->u.gvar.nInits > 0 &&
        (strncmp(llt,"%struct.",8)==0 || strncmp(llt,"%union.",7)==0)) {
        emit_gvar_struct_init(ctx, n, llt, linkage);
        return;
    }

    if (strcmp(llt,"ptr")==0)
        fprintf(ctx->out, "@%s = %sglobal ptr null\n", n->u.gvar.name, linkage);
    else if (strncmp(llt,"%struct.",8)==0 || strncmp(llt,"%union.",7)==0)
        /* Struct/union globals require zeroinitializer, not "0" */
        fprintf(ctx->out, "@%s = %sglobal %s zeroinitializer\n",
                n->u.gvar.name, linkage, llt);
    else
        fprintf(ctx->out, "@%s = %sglobal %s %s\n",
                n->u.gvar.name, linkage, llt, init);
}

/* ================================================================
 * emit_struct_type – emit a struct type definition
 * ================================================================ */

static void emit_struct_type(IRCtx *ctx, Node *n) {
    if (!n || n->kind != NK_STRUCT_DEF) return;
    const char *tag = n->u.sdef.tag;
    if (!tag) return;

    char tb[64];
    if (n->u.sdef.is_union) {
        /* Union: find max field size, emit as [N x i8] */
        int max_sz = 1;
        for (int i = 0; i < n->u.sdef.nfields; i++) {
            StructField *sf = &n->u.sdef.fields[i];
            int fsz = type_size(sf->base_type, sf->ptr_lvl);
            if (sf->arr_sz > 0) fsz *= sf->arr_sz;
            if (fsz > max_sz) max_sz = fsz;
        }
        fprintf(ctx->out, "%%union.%s = type { [%d x i8] }\n", tag, max_sz);
    } else {
        fprintf(ctx->out, "%%struct.%s = type { ", tag);
        int cnt = 0;
        for (int i = 0; i < n->u.sdef.nfields; i++) {
            StructField *sf = &n->u.sdef.fields[i];
            if (cnt++ > 0) fprintf(ctx->out, ", ");
            const char *flt = lltype_of(sf->base_type, sf->ptr_lvl, tb, sizeof(tb));
            if (sf->arr_sz > 0)
                fprintf(ctx->out, "[%d x %s]", sf->arr_sz, flt);
            else
                fprintf(ctx->out, "%s", flt);
        }
        fprintf(ctx->out, " }\n");
    }
}

/* ================================================================
 * ir_emit_program – main entry point
 * ================================================================ */

void ir_emit_program(Node *prog, const char *module_id, FILE *out) {
    if (!prog || prog->kind != NK_PROGRAM) return;

    IRCtx *ctx = (IRCtx *)xcalloc(1, sizeof(IRCtx));
    ctx->out = out;
    sb_init(&ctx->str_buf);

    /* ---- First pass: collect struct defs, enum defs, function sigs, global vars ---- */
    for (int i = 0; i < prog->u.prog.nitems; i++) {
        Node *item = prog->u.prog.items[i];
        if (item->kind == NK_STRUCT_DEF)  collect_struct_def(ctx, item);
        if (item->kind == NK_ENUM_DEF)    collect_enum_def(ctx, item);
        if (item->kind == NK_FUNCTION)    collect_func_sig(ctx, item);
        if (item->kind == NK_GLOBAL_VAR)  collect_global_var(ctx, item);
    }

    /* Register common runtime functions if not already declared.
     * For variadic functions, provide the fixed parameter types so the call
     * instruction's function type matches the declaration exactly. */
    register_funcsig(ctx, "printf",  "i32", true,  "ptr");
    register_funcsig(ctx, "fprintf", "i32", true,  "ptr, ptr");
    register_funcsig(ctx, "sprintf", "i32", true,  "ptr, ptr");
    register_funcsig(ctx, "snprintf","i32", true,  "ptr, i64, ptr");
    register_funcsig(ctx, "scanf",   "i32", true,  "ptr");
    register_funcsig(ctx, "sscanf",  "i32", true,  "ptr, ptr");
    register_funcsig(ctx, "fscanf",  "i32", true,  "ptr, ptr");
    register_funcsig(ctx, "malloc",  "ptr", false, NULL);
    register_funcsig(ctx, "calloc",  "ptr", false, NULL);
    register_funcsig(ctx, "realloc", "ptr", false, NULL);
    register_funcsig(ctx, "free",    "void", false, NULL);
    register_funcsig(ctx, "strlen",  "i64", false, NULL);
    register_funcsig(ctx, "strcpy",  "ptr", false, NULL);
    register_funcsig(ctx, "strncpy", "ptr", false, NULL);
    register_funcsig(ctx, "strcmp",  "i32", false, NULL);
    register_funcsig(ctx, "strncmp", "i32", false, NULL);
    register_funcsig(ctx, "strcat",  "ptr", false, NULL);
    register_funcsig(ctx, "strchr",  "ptr", false, NULL);
    register_funcsig(ctx, "memcpy",  "ptr", false, NULL);
    register_funcsig(ctx, "memset",  "ptr", false, NULL);
    register_funcsig(ctx, "memcmp",  "i32", false, NULL);
    register_funcsig(ctx, "exit",    "void", false, NULL);
    register_funcsig(ctx, "abort",   "void", false, NULL);
    register_funcsig(ctx, "puts",    "i32", false, NULL);
    register_funcsig(ctx, "putchar", "i32", false, NULL);
    register_funcsig(ctx, "getchar", "i32", false, NULL);
    register_funcsig(ctx, "fopen",   "ptr", false, NULL);
    register_funcsig(ctx, "fclose",  "i32", false, NULL);
    register_funcsig(ctx, "fgets",   "ptr", false, NULL);
    register_funcsig(ctx, "fread",   "i64", false, NULL);
    register_funcsig(ctx, "fwrite",  "i64", false, NULL);
    register_funcsig(ctx, "atoi",    "i32", false, NULL);
    register_funcsig(ctx, "atol",    "i64", false, NULL);
    register_funcsig(ctx, "atof",    "double", false, NULL);
    register_funcsig(ctx, "abs",     "i32", false, NULL);
    register_funcsig(ctx, "labs",    "i64", false, NULL);

    /* ---- Pre-scan: register all string literals ---- */
    /* We need to initialise a small tmp pool for the scan phase */
    scan_strlits(ctx, prog);

    /* ---- Emit module header ---- */
    fprintf(out, "; ModuleID = '%s'\n\n", module_id ? module_id : "module");

    /* ---- Emit struct type definitions ---- */
    for (int i = 0; i < prog->u.prog.nitems; i++) {
        Node *item = prog->u.prog.items[i];
        if (item->kind == NK_STRUCT_DEF)
            emit_struct_type(ctx, item);
    }

    /* ---- Emit string literal globals ---- */
    if (ctx->str_buf.buf && ctx->str_buf.len > 0) {
        fputc('\n', out);
        fwrite(ctx->str_buf.buf, 1, ctx->str_buf.len, out);
    }

    /* ---- Build dedup maps ---- */
    int nitems = prog->u.prog.nitems;

    /* Global var dedup: for each name keep the "best" definition.
       Priority: 2=initialized, 1=tentative(no init), 0=extern */
    typedef struct { char name[128]; int best_idx; int best_prio; } GVEntry;
    GVEntry *gv_dedup = (GVEntry *)xcalloc((size_t)nitems, sizeof(GVEntry));
    int n_gv_dedup = 0;
    for (int i = 0; i < nitems; i++) {
        Node *item = prog->u.prog.items[i];
        if (item->kind != NK_GLOBAL_VAR) continue;
        const char *nm = item->u.gvar.name;
        int prio = item->u.gvar.is_extern ? 0
                 : (item->u.gvar.init || item->u.gvar.nInits > 0) ? 2 : 1;
        int found = -1;
        for (int j = 0; j < n_gv_dedup; j++)
            if (strcmp(gv_dedup[j].name, nm) == 0) { found = j; break; }
        if (found < 0) {
            strncpy(gv_dedup[n_gv_dedup].name, nm, 127);
            gv_dedup[n_gv_dedup].best_idx = i;
            gv_dedup[n_gv_dedup].best_prio = prio;
            n_gv_dedup++;
        } else if (prio > gv_dedup[found].best_prio) {
            gv_dedup[found].best_idx = i;
            gv_dedup[found].best_prio = prio;
        }
    }
    /* Build keep[] array */
    bool *gv_keep = (bool *)xcalloc((size_t)nitems, sizeof(bool));
    for (int j = 0; j < n_gv_dedup; j++)
        gv_keep[gv_dedup[j].best_idx] = true;
    free(gv_dedup);

    /* Function dedup: collect names of defined (has body) functions */
    #define MAX_FN_DEDUP 512
    char defined_fns[MAX_FN_DEDUP][128];
    int n_defined = 0;
    for (int i = 0; i < nitems; i++) {
        Node *item = prog->u.prog.items[i];
        if (item->kind == NK_FUNCTION && item->u.fn.body && n_defined < MAX_FN_DEDUP)
            strncpy(defined_fns[n_defined++], item->u.fn.name, 127);
    }
    char declared_fns[MAX_FN_DEDUP][128];
    int n_declared = 0;

    /* ---- Emit global variable declarations ---- */
    fputc('\n', out);
    for (int i = 0; i < nitems; i++) {
        if (!gv_keep[i]) continue;
        Node *item = prog->u.prog.items[i];
        if (item->kind == NK_GLOBAL_VAR)
            emit_global_var(ctx, item);
    }
    free(gv_keep);

    /* ---- Emit function declarations (extern / no body) ---- */
    fputc('\n', out);
    for (int i = 0; i < nitems; i++) {
        Node *item = prog->u.prog.items[i];
        if (item->kind != NK_FUNCTION || item->u.fn.body) continue;
        const char *fname = item->u.fn.name;
        /* Skip if a define exists for this function */
        bool has_def = false;
        for (int j = 0; j < n_defined; j++)
            if (strcmp(defined_fns[j], fname) == 0) { has_def = true; break; }
        if (has_def) continue;
        /* Skip if already declared */
        bool already = false;
        for (int j = 0; j < n_declared; j++)
            if (strcmp(declared_fns[j], fname) == 0) { already = true; break; }
        if (already) continue;
        if (n_declared < MAX_FN_DEDUP)
            strncpy(declared_fns[n_declared++], fname, 127);
        emit_function(ctx, item);
    }

    /* ---- Emit function definitions ---- */
    /* Also deduplicate: emit each defined function only once */
    char emitted_defs[MAX_FN_DEDUP][128];
    int n_emitted_defs = 0;
    for (int i = 0; i < nitems; i++) {
        Node *item = prog->u.prog.items[i];
        if (item->kind != NK_FUNCTION || !item->u.fn.body) continue;
        const char *fname = item->u.fn.name;
        bool already = false;
        for (int j = 0; j < n_emitted_defs; j++)
            if (strcmp(emitted_defs[j], fname) == 0) { already = true; break; }
        if (already) continue;
        if (n_emitted_defs < MAX_FN_DEDUP)
            strncpy(emitted_defs[n_emitted_defs++], fname, 127);
        emit_function(ctx, item);
    }

    /* Cleanup */
    if (ctx->str_buf.buf) free(ctx->str_buf.buf);
    for (int i = 0; i < ctx->n_strlits; i++)
        free(ctx->strlits[i].bytes);
    free(ctx);
}
