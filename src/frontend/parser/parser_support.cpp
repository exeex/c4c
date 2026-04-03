#include "parser.hpp"

#include <cstring>
#include <cstdlib>

namespace c4c {

// ── ParserSnapshot save / restore ────────────────────────────────────────────

Parser::ParserSnapshot Parser::save_state() const {
    ParserSnapshot snap;
    snap.pos                  = pos_;
    snap.typedefs             = typedefs_;
    snap.user_typedefs        = user_typedefs_;
    snap.typedef_types        = typedef_types_;
    snap.var_types            = var_types_;
    snap.last_resolved_typedef = last_resolved_typedef_;
    snap.template_arg_expr_depth = template_arg_expr_depth_;
    return snap;
}

void Parser::restore_state(const ParserSnapshot& snap) {
    pos_                   = snap.pos;
    typedefs_              = snap.typedefs;
    user_typedefs_         = snap.user_typedefs;
    typedef_types_         = snap.typedef_types;
    var_types_             = snap.var_types;
    last_resolved_typedef_ = snap.last_resolved_typedef;
    template_arg_expr_depth_ = snap.template_arg_expr_depth;
}


bool eval_enum_expr(Node* n, const std::unordered_map<std::string, long long>& consts,
                           long long* out) {
    if (!n || !out) return false;
    if (n->kind == NK_INT_LIT || n->kind == NK_CHAR_LIT) { *out = n->ival; return true; }
    if (n->kind == NK_VAR && n->name) {
        auto it = consts.find(n->name);
        if (it != consts.end()) { *out = it->second; return true; }
        return false;
    }
    if (n->kind == NK_CAST && n->left) return eval_enum_expr(n->left, consts, out);
    if (n->kind == NK_SIZEOF_TYPE) {
        if (n->type.base == TB_TYPEDEF && n->type.tag && consts.count(n->type.tag) == 0) {
            return false;  // dependent or unresolved type
        }
        long long sz = sizeof_base(n->type.base);
        if (n->type.ptr_level > 0) sz = 8;
        *out = sz;
        return true;
    }
    if (n->kind == NK_UNARY && n->op && n->left) {
        long long v = 0;
        if (!eval_enum_expr(n->left, consts, &v)) return false;
        if (strcmp(n->op, "-") == 0) { *out = -v; return true; }
        if (strcmp(n->op, "+") == 0) { *out = v;  return true; }
        return false;
    }
    if (n->kind == NK_BINOP && n->op && n->left && n->right) {
        long long l = 0, r = 0;
        if (!eval_enum_expr(n->left, consts, &l)) return false;
        if (!eval_enum_expr(n->right, consts, &r)) return false;
        if (strcmp(n->op, "+") == 0) { *out = l + r; return true; }
        if (strcmp(n->op, "-") == 0) { *out = l - r; return true; }
        if (strcmp(n->op, "*") == 0) { *out = l * r; return true; }
        if (strcmp(n->op, "&") == 0) { *out = l & r; return true; }
        if (strcmp(n->op, "|") == 0) { *out = l | r; return true; }
        if (strcmp(n->op, "^") == 0) { *out = l ^ r; return true; }
        if (strcmp(n->op, "<<") == 0) { *out = l << r; return true; }
        if (strcmp(n->op, ">>") == 0) { *out = l >> r; return true; }
        if (strcmp(n->op, "/") == 0 && r != 0) { *out = l / r; return true; }
        if (strcmp(n->op, "%") == 0 && r != 0) { *out = l % r; return true; }
    }
    if (n->kind == NK_TERNARY && n->cond && n->then_ && n->else_) {
        long long c = 0;
        if (!eval_enum_expr(n->cond, consts, &c)) return false;
        return eval_enum_expr(c ? n->then_ : n->else_, consts, out);
    }
    return false;
}

bool is_dependent_enum_expr(Node* n,
                            const std::unordered_map<std::string, long long>& consts) {
    if (!n) return false;
    if (n->kind == NK_INT_LIT || n->kind == NK_CHAR_LIT) return false;
    if (n->kind == NK_VAR && n->name) {
        if (consts.count(n->name) > 0) return false;
        return true;
    }
    if (n->kind == NK_SIZEOF_TYPE) {
        return n->type.base == TB_TYPEDEF && n->type.tag && consts.count(n->type.tag) == 0;
    }
    if (n->kind == NK_SIZEOF_EXPR) {
        return true;
    }
    if (n->kind == NK_SIZEOF_PACK) {
        return true;
    }
    if (n->kind == NK_CAST && n->left) return is_dependent_enum_expr(n->left, consts);
    if (n->kind == NK_UNARY && n->left) return is_dependent_enum_expr(n->left, consts);
    if (n->kind == NK_BINOP && n->left && n->right) {
        return is_dependent_enum_expr(n->left, consts) ||
               is_dependent_enum_expr(n->right, consts);
    }
    if (n->kind == NK_TERNARY && n->cond && n->then_ && n->else_) {
        return is_dependent_enum_expr(n->cond, consts) ||
               is_dependent_enum_expr(n->then_, consts) ||
               is_dependent_enum_expr(n->else_, consts);
    }
    return false;
}

// ── constant expression evaluator ─────────────────────────────────────────────
// Evaluate a simple AST subtree as an integer constant (for array sizes).
// Returns true and sets *out on success; false if the expression is dynamic.
long long sizeof_base(TypeBase b) {
    switch (b) {
        case TB_VOID: return 1;
        case TB_BOOL: case TB_CHAR: case TB_SCHAR: case TB_UCHAR: return 1;
        case TB_SHORT: case TB_USHORT: return 2;
        case TB_INT: case TB_UINT: case TB_FLOAT: return 4;
        case TB_LONG: case TB_ULONG: case TB_LONGLONG: case TB_ULONGLONG:
        case TB_DOUBLE: return 8;
        case TB_LONGDOUBLE: return 16;
        case TB_INT128: case TB_UINT128: return 16;
        case TB_COMPLEX_CHAR:
        case TB_COMPLEX_SCHAR:
        case TB_COMPLEX_UCHAR:
            return 2;
        case TB_COMPLEX_SHORT:
        case TB_COMPLEX_USHORT:
            return 4;
        case TB_COMPLEX_INT:
        case TB_COMPLEX_UINT:
        case TB_COMPLEX_FLOAT:
            return 8;
        case TB_COMPLEX_LONG:
        case TB_COMPLEX_ULONG:
        case TB_COMPLEX_LONGLONG:
        case TB_COMPLEX_ULONGLONG:
        case TB_COMPLEX_DOUBLE:
            return 16;
        case TB_COMPLEX_LONGDOUBLE:
            return 32;
        default: return 4;
    }
}

long long align_base(TypeBase b, int ptr_level) {
    if (ptr_level > 0) return 8;
    switch (b) {
        case TB_BOOL: case TB_CHAR: case TB_SCHAR: case TB_UCHAR: return 1;
        case TB_SHORT: case TB_USHORT: return 2;
        case TB_INT: case TB_UINT: case TB_FLOAT: case TB_ENUM: return 4;
        case TB_LONG: case TB_ULONG:
        case TB_LONGLONG: case TB_ULONGLONG:
        case TB_DOUBLE:
            return 8;
        case TB_LONGDOUBLE:
        case TB_INT128: case TB_UINT128:
            return 16;
        case TB_COMPLEX_CHAR:
        case TB_COMPLEX_SCHAR:
        case TB_COMPLEX_UCHAR:
            return 1;
        case TB_COMPLEX_SHORT:
        case TB_COMPLEX_USHORT:
            return 2;
        case TB_COMPLEX_INT:
        case TB_COMPLEX_UINT:
        case TB_COMPLEX_FLOAT:
            return 4;
        case TB_COMPLEX_LONG:
        case TB_COMPLEX_ULONG:
        case TB_COMPLEX_LONGLONG:
        case TB_COMPLEX_ULONGLONG:
        case TB_COMPLEX_DOUBLE:
            return 8;
        case TB_COMPLEX_LONGDOUBLE:
            return 16;
        default: return 8;
    }
}

bool eval_const_int(Node* n, long long* out,
    const std::unordered_map<std::string, Node*>* struct_map,
    const std::unordered_map<std::string, long long>* named_consts);

// Forward declaration
static long long struct_align(const char* tag,
    const std::unordered_map<std::string, Node*>* struct_map);
static long long struct_sizeof(const char* tag,
    const std::unordered_map<std::string, Node*>* struct_map);

// Compute the ABI alignment of a field type.
static long long field_align(const TypeSpec& ts,
    const std::unordered_map<std::string, Node*>* struct_map) {
    long long natural = 0;
    if (ts.ptr_level > 0) return 8;
    if (ts.is_vector && ts.vector_bytes > 0) {
        natural = ts.vector_bytes > 16 ? 16 : ts.vector_bytes;
    } else if ((ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.tag) {
        natural = struct_align(ts.tag, struct_map);
    } else {
        natural = align_base(ts.base, ts.ptr_level);
    }
    if (ts.align_bytes > 0 && ts.align_bytes > natural) return ts.align_bytes;
    return natural;
}

// Compute the alignment of a struct/union (max alignment of its fields).
static long long struct_align(const char* tag,
    const std::unordered_map<std::string, Node*>* struct_map) {
    if (!struct_map || !tag) return 8;
    auto it = struct_map->find(tag);
    if (it == struct_map->end()) return 8;
    Node* sd = it->second;
    if (!sd || sd->kind != NK_STRUCT_DEF) return 8;
    long long max_align = 1;
    for (int i = 0; i < sd->n_fields; i++) {
        Node* f = sd->fields[i];
        if (!f) continue;
        long long a = field_align(f->type, struct_map);
        if (a > max_align) max_align = a;
    }
    if (sd->struct_align > max_align) max_align = sd->struct_align;
    return max_align;
}

// Compute sizeof a struct (with alignment padding).
static long long struct_sizeof(const char* tag,
    const std::unordered_map<std::string, Node*>* struct_map) {
    if (!struct_map || !tag) return 8;
    auto it = struct_map->find(tag);
    if (it == struct_map->end()) return 8;
    Node* sd = it->second;
    if (!sd || sd->kind != NK_STRUCT_DEF) return 8;
    long long offset = 0, max_align = 1;
    for (int i = 0; i < sd->n_fields; i++) {
        Node* f = sd->fields[i];
        if (!f) continue;
        long long a = field_align(f->type, struct_map);
        if (a > max_align) max_align = a;
        offset = (offset + a - 1) / a * a;
        long long sz;
        if (f->type.ptr_level > 0) sz = 8;
        else if (f->type.is_vector && f->type.vector_bytes > 0)
            sz = f->type.vector_bytes;
        else if (f->type.base == TB_STRUCT || f->type.base == TB_UNION)
            sz = struct_sizeof(f->type.tag, struct_map);
        else sz = sizeof_base(f->type.base);
        if (f->type.array_rank > 0)
            for (int d = 0; d < f->type.array_rank; d++)
                if (f->type.array_dims[d] > 0) sz *= f->type.array_dims[d];
        offset += sz;
    }
    return (offset + max_align - 1) / max_align * max_align;
}

// Helper: compute offsetof(tag, field_name) using struct_tag_def_map
static bool compute_offsetof(const char* tag, const char* field_name,
    const std::unordered_map<std::string, Node*>* struct_map, long long* out) {
    if (!struct_map || !tag || !field_name) return false;
    auto it = struct_map->find(tag);
    if (it == struct_map->end()) return false;
    Node* sd = it->second;
    if (!sd || sd->kind != NK_STRUCT_DEF) return false;
    std::string path(field_name);
    std::string head = path;
    std::string tail;
    size_t dot = path.find('.');
    if (dot != std::string::npos) {
        head = path.substr(0, dot);
        tail = path.substr(dot + 1);
    }

    long long offset = 0;
    for (int i = 0; i < sd->n_fields; i++) {
        Node* f = sd->fields[i];
        if (!f) continue;
        long long align = field_align(f->type, struct_map);
        // Align offset
        offset = (offset + align - 1) / align * align;
        if (f->name && head == f->name) {
            if (tail.empty()) {
                *out = offset;
                return true;
            }
            if ((f->type.base == TB_STRUCT || f->type.base == TB_UNION) && f->type.tag) {
                long long inner = 0;
                if (!compute_offsetof(f->type.tag, tail.c_str(), struct_map, &inner)) return false;
                *out = offset + inner;
                return true;
            }
            return false;
        }
        // Advance by field size
        long long sz;
        if (f->type.ptr_level > 0) sz = 8;
        else if (f->type.base == TB_STRUCT || f->type.base == TB_UNION)
            sz = struct_sizeof(f->type.tag, struct_map);
        else sz = sizeof_base(f->type.base);
        if (f->type.array_rank > 0) {
            for (int d = 0; d < f->type.array_rank; d++)
                if (f->type.array_dims[d] > 0) sz *= f->type.array_dims[d];
        }
        offset += sz;
    }
    return false;
}

bool eval_const_int(Node* n, long long* out,
    const std::unordered_map<std::string, Node*>* struct_map,
    const std::unordered_map<std::string, long long>* named_consts) {
    if (!n) return false;
    if (n->kind == NK_INT_LIT || n->kind == NK_CHAR_LIT) {
        *out = n->ival;
        return true;
    }
    if (n->kind == NK_VAR && n->name) {
        if (named_consts) {
            auto it = named_consts->find(n->name);
            if (it != named_consts->end()) {
                *out = it->second;
                return true;
            }
        }
        return false;
    }
    if (n->kind == NK_CAST && n->left) {
        return eval_const_int(n->left, out, struct_map, named_consts);
    }
    if (n->kind == NK_OFFSETOF) {
        if (n->type.base == TB_STRUCT || n->type.base == TB_UNION) {
            return compute_offsetof(n->type.tag, n->name, struct_map, out);
        }
        return false;
    }
    if (n->kind == NK_ALIGNOF_TYPE) {
        *out = field_align(n->type, struct_map);
        return true;
    }
    if (n->kind == NK_SIZEOF_TYPE) {
        // sizeof(type): use LP64 sizes
        long long sz = sizeof_base(n->type.base);
        if (n->type.ptr_level > 0) sz = 8;
        if ((n->type.base == TB_STRUCT || n->type.base == TB_UNION) && n->type.tag) {
            sz = struct_sizeof(n->type.tag, struct_map);
        }
        if (n->type.array_rank > 0) {
            for (int i = 0; i < n->type.array_rank; ++i)
                if (n->type.array_dims[i] > 0) sz *= n->type.array_dims[i];
        }
        *out = sz;
        return true;
    }
    if (n->kind == NK_SIZEOF_EXPR) {
        // sizeof(expr): approximate from the expression type if it's a literal
        // For non-literals, return a conservative estimate
        return false;  // complex: skip for now
    }
    if (n->kind == NK_SIZEOF_PACK) {
        return false;
    }
    if (n->kind == NK_UNARY && n->op && n->left) {
        long long v;
        if (!eval_const_int(n->left, &v, struct_map, named_consts)) return false;
        if (strcmp(n->op, "-") == 0) { *out = -v; return true; }
        if (strcmp(n->op, "+") == 0) { *out = v; return true; }
        if (strcmp(n->op, "~") == 0) { *out = ~v; return true; }
        if (strcmp(n->op, "!") == 0) { *out = !v; return true; }
    }
    if (n->kind == NK_BINOP && n->op) {
        long long l, r;
        if (!eval_const_int(n->left, &l, struct_map, named_consts)) return false;
        if (!eval_const_int(n->right, &r, struct_map, named_consts)) return false;
        if (strcmp(n->op, "+")  == 0) { *out = l + r; return true; }
        if (strcmp(n->op, "-")  == 0) { *out = l - r; return true; }
        if (strcmp(n->op, "*")  == 0) { *out = l * r; return true; }
        if (strcmp(n->op, "/")  == 0 && r != 0) { *out = l / r; return true; }
        if (strcmp(n->op, "%")  == 0 && r != 0) { *out = l % r; return true; }
        if (strcmp(n->op, "<<") == 0) { *out = l << r; return true; }
        if (strcmp(n->op, ">>") == 0) { *out = l >> r; return true; }
        if (strcmp(n->op, "&")  == 0) { *out = l & r;  return true; }
        if (strcmp(n->op, "|")  == 0) { *out = l | r;  return true; }
        if (strcmp(n->op, "^")  == 0) { *out = l ^ r;  return true; }
        if (strcmp(n->op, "<")  == 0) { *out = l < r;  return true; }
        if (strcmp(n->op, "<=") == 0) { *out = l <= r; return true; }
        if (strcmp(n->op, ">")  == 0) { *out = l > r;  return true; }
        if (strcmp(n->op, ">=") == 0) { *out = l >= r; return true; }
        if (strcmp(n->op, "==") == 0) { *out = l == r; return true; }
        if (strcmp(n->op, "!=") == 0) { *out = l != r; return true; }
        if (strcmp(n->op, "&&") == 0) { *out = (l && r); return true; }
        if (strcmp(n->op, "||") == 0) { *out = (l || r); return true; }
    }
    if (n->kind == NK_TERNARY && n->cond && n->then_ && n->else_) {
        long long c;
        if (!eval_const_int(n->cond, &c, struct_map, named_consts)) return false;
        return eval_const_int(c ? n->then_ : n->else_, out, struct_map, named_consts);
    }
    return false;
}

// ── helpers ───────────────────────────────────────────────────────────────────

bool is_qualifier(TokenKind k) {
    return k == TokenKind::KwConst    || k == TokenKind::KwVolatile ||
           k == TokenKind::KwRestrict || k == TokenKind::KwAtomic   ||
           k == TokenKind::KwInline;
}

bool is_storage_class(TokenKind k) {
    return k == TokenKind::KwStatic   || k == TokenKind::KwExtern ||
           k == TokenKind::KwRegister || k == TokenKind::KwAuto   ||
           k == TokenKind::KwTypedef  || k == TokenKind::KwInline ||
           k == TokenKind::KwExtension || k == TokenKind::KwNoreturn ||
           k == TokenKind::KwThreadLocal || k == TokenKind::KwMutable;
}

bool is_type_kw(TokenKind k) {
    switch (k) {
        case TokenKind::KwVoid:
        case TokenKind::KwChar:
        case TokenKind::KwShort:
        case TokenKind::KwInt:
        case TokenKind::KwLong:
        case TokenKind::KwFloat:
        case TokenKind::KwDouble:
        case TokenKind::KwSigned:
        case TokenKind::KwUnsigned:
        case TokenKind::KwBool:
        case TokenKind::KwStruct:
        case TokenKind::KwClass:
        case TokenKind::KwUnion:
        case TokenKind::KwEnum:
        case TokenKind::KwDecltype:
        case TokenKind::KwInt128:
        case TokenKind::KwUInt128:
        case TokenKind::KwBuiltin:   // __builtin_va_list
        case TokenKind::KwAutoType:  // __auto_type
        case TokenKind::KwTypeof:
        case TokenKind::KwTypeofUnqual:
        case TokenKind::KwComplex:
            return true;
        default:
            return false;
    }
}

// parse an integer literal from a lexeme string
// Returns true if a numeric literal lexeme has an imaginary suffix (i/j/I/J).
bool lexeme_is_imaginary(const char* s) {
    size_t len = strlen(s);
    for (size_t k = 0; k < len; k++) {
        char c = s[k];
        if (c == 'i' || c == 'I' || c == 'j' || c == 'J') return true;
    }
    return false;
}

long long parse_int_lexeme(const char* s) {
    // strip u/U, l/L and imaginary i/j/I/J suffixes (any order)
    char buf[64];
    int  len = (int)strlen(s);
    int  end = len;
    while (end > 0) {
        char cc = s[end - 1];
        if (cc == 'u' || cc == 'U' || cc == 'l' || cc == 'L' ||
            cc == 'i' || cc == 'I' || cc == 'j' || cc == 'J') {
            --end;
        } else {
            break;
        }
    }
    if (end >= (int)sizeof(buf)) end = (int)sizeof(buf) - 1;
    strncpy(buf, s, (size_t)end);
    buf[end] = '\0';

    if (buf[0] == '0' && (buf[1] == 'x' || buf[1] == 'X')) {
        return (long long)strtoull(buf, nullptr, 16);
    } else if (buf[0] == '0' && (buf[1] == 'b' || buf[1] == 'B')) {
        return (long long)strtoull(buf + 2, nullptr, 2);
    } else if (buf[0] == '0' && buf[1] != '\0') {
        return (long long)strtoull(buf, nullptr, 8);
    } else {
        return (long long)strtoull(buf, nullptr, 10);
    }
}

double parse_float_lexeme(const char* s) {
    // strip f/F, l/L, f16/F16, f32/F32, f64/F64, f128/F128, bf16/BF16,
    // and imaginary i/j/I/J suffixes
    char buf[64];
    size_t len = strlen(s);
    if (len >= sizeof(buf)) len = sizeof(buf) - 1;
    strncpy(buf, s, len);
    buf[len] = '\0';
    bool is_float_suffix = false;  // 'f'/'F' suffix → single-precision
    size_t end = len;
    while (end > 0) {
        if (end >= 4 &&
            ((buf[end - 4] == 'f' && buf[end - 3] == '1' &&
              buf[end - 2] == '2' && buf[end - 1] == '8') ||
             (buf[end - 4] == 'F' && buf[end - 3] == '1' &&
              buf[end - 2] == '2' && buf[end - 1] == '8') ||
             (buf[end - 4] == 'b' && buf[end - 3] == 'f' &&
              buf[end - 2] == '1' && buf[end - 1] == '6') ||
             (buf[end - 4] == 'B' && buf[end - 3] == 'F' &&
              buf[end - 2] == '1' && buf[end - 1] == '6'))) {
            is_float_suffix = true;
            end -= 4;
            continue;
        }
        if (end >= 3 &&
            ((buf[end - 3] == 'f' && buf[end - 2] == '1' &&
              buf[end - 1] == '6') ||
             (buf[end - 3] == 'F' && buf[end - 2] == '1' &&
              buf[end - 1] == '6') ||
             (buf[end - 3] == 'f' && buf[end - 2] == '3' &&
              buf[end - 1] == '2') ||
             (buf[end - 3] == 'F' && buf[end - 2] == '3' &&
              buf[end - 1] == '2') ||
             (buf[end - 3] == 'f' && buf[end - 2] == '6' &&
              buf[end - 1] == '4') ||
             (buf[end - 3] == 'F' && buf[end - 2] == '6' &&
              buf[end - 1] == '4'))) {
            is_float_suffix = true;
            end -= 3;
            continue;
        }
        char c = buf[end - 1];
        if (c == 'f' || c == 'F') { is_float_suffix = true; --end; }
        else if (c == 'l' || c == 'L' ||
                 c == 'i' || c == 'I' || c == 'j' || c == 'J') { --end; }
        else { break; }
    }
    buf[end] = '\0';
    // For float (F suffix) literals, round to single-precision first so that fval
    // stores the exact float-rounded value when promoted to double.
    if (is_float_suffix) return (double)(float)strtod(buf, nullptr);
    return strtod(buf, nullptr);
}

TypeSpec resolve_typedef_chain(TypeSpec ts,
                                      const std::unordered_map<std::string, TypeSpec>& tmap) {
    for (int depth = 0; depth < 16; ++depth) {
        if (ts.base != TB_TYPEDEF || ts.ptr_level > 0 || ts.array_rank > 0) break;
        if (!ts.tag) break;
        auto it = tmap.find(ts.tag);
        if (it == tmap.end()) break;
        bool c = ts.is_const, v = ts.is_volatile;
        ts = it->second;
        ts.is_const   |= c;
        ts.is_volatile |= v;
    }
    return ts;
}

// Returns true if type a and type b are compatible per GCC __builtin_types_compatible_p rules.
bool types_compatible_p(TypeSpec a, TypeSpec b,
                                const std::unordered_map<std::string, TypeSpec>& tmap) {
    a = resolve_typedef_chain(a, tmap);
    b = resolve_typedef_chain(b, tmap);
    // Strip top-level cv-qualifiers (they don't affect compatibility at the value level).
    // But for pointer types, qualifiers on the pointed-to type DO matter (char* vs const char*).
    if (a.ptr_level == 0 && a.array_rank == 0) { a.is_const = false; a.is_volatile = false; }
    if (b.ptr_level == 0 && b.array_rank == 0) { b.is_const = false; b.is_volatile = false; }
    if (a.is_vector != b.is_vector) return false;
    if (a.is_vector &&
        (a.vector_lanes != b.vector_lanes || a.vector_bytes != b.vector_bytes))
        return false;
    if (a.base != b.base) return false;
    if (a.ptr_level != b.ptr_level) return false;
    if (a.is_const != b.is_const || a.is_volatile != b.is_volatile) return false;
    if (a.array_rank != b.array_rank) return false;
    for (int i = 0; i < a.array_rank; ++i) {
        long long da = a.array_dims[i], db = b.array_dims[i];
        // -2 means unsized ([]) — compatible with any same-rank dimension
        if (da != db && da != -2 && db != -2) return false;
    }
    // Struct/union/enum: same tag required
    if (a.base == TB_STRUCT || a.base == TB_UNION || a.base == TB_ENUM) {
        if (!a.tag || !b.tag || strcmp(a.tag, b.tag) != 0) return false;
    }
    return true;
}

}  // namespace c4c
