#include "parser.hpp"
#include "parser_internal.hpp"

#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace tinyc2ll::frontend_cxx {

// Compute vector_lanes from vector_bytes and base type.
// Called after the final base type is resolved.
static void finalize_vector_type(TypeSpec& ts) {
    if (!ts.is_vector || ts.vector_bytes <= 0) return;
    long long elem_sz = 4;
    switch (ts.base) {
        case TB_CHAR: case TB_SCHAR: case TB_UCHAR: elem_sz = 1; break;
        case TB_SHORT: case TB_USHORT: elem_sz = 2; break;
        case TB_FLOAT: case TB_INT: case TB_UINT: elem_sz = 4; break;
        case TB_DOUBLE: case TB_LONG: case TB_ULONG:
        case TB_LONGLONG: case TB_ULONGLONG: elem_sz = 8; break;
        default: elem_sz = 4; break;
    }
    ts.vector_lanes = ts.vector_bytes / elem_sz;
}
bool Parser::is_typedef_name(const std::string& s) const {
    return typedefs_.count(s) > 0;
}

bool Parser::is_type_start() const {
    TokenKind k = cur().kind;
    if (is_type_kw(k)) return true;
    if (is_qualifier(k)) return true;
    if (is_storage_class(k)) return true;
    if (k == TokenKind::KwAttribute) return true;  // __attribute__((x)) type cast
    if (k == TokenKind::KwAlignas || k == TokenKind::KwStaticAssert) return false;
    if (k == TokenKind::Identifier && is_typedef_name(cur().lexeme)) return true;
    return false;
}

// ── skip helpers ─────────────────────────────────────────────────────────────

void Parser::skip_paren_group() {
    expect(TokenKind::LParen);
    int depth = 1;
    while (!at_end() && depth > 0) {
        if (check(TokenKind::LParen)) ++depth;
        else if (check(TokenKind::RParen)) { if (--depth == 0) { consume(); return; } }
        consume();
    }
}

void Parser::skip_brace_group() {
    expect(TokenKind::LBrace);
    int depth = 1;
    while (!at_end() && depth > 0) {
        if (check(TokenKind::LBrace)) ++depth;
        else if (check(TokenKind::RBrace)) { if (--depth == 0) { consume(); return; } }
        consume();
    }
}

void Parser::skip_attributes() {
    while (check(TokenKind::KwAttribute)) {
        consume();  // __attribute__
        if (check(TokenKind::LParen)) skip_paren_group();
        if (check(TokenKind::LParen)) skip_paren_group();  // double-paren
    }
    while (check(TokenKind::KwExtension)) {
        consume();
    }
    while (check(TokenKind::KwNoreturn)) {
        consume();
    }
}

void Parser::parse_attributes(TypeSpec* ts) {
    auto apply_aligned_attr = [&](long long align) {
        if (!ts || align <= 0) return;
        if (align > ts->align_bytes) ts->align_bytes = static_cast<int>(align);
    };

    auto apply_vector_size_attr = [&](long long bytes) {
        if (!ts || bytes <= 0) return;
        ts->is_vector = true;
        ts->vector_bytes = bytes;
        // Leave vector_lanes=0; finalize_vector() computes it once the
        // final base type is known.
        ts->vector_lanes = 0;
    };

    while (check(TokenKind::KwAttribute)) {
        consume();  // __attribute__
        if (!check(TokenKind::LParen)) continue;
        consume();  // (
        if (!check(TokenKind::LParen)) {
            skip_paren_group();
            continue;
        }
        consume();  // (

        while (!at_end() && !check(TokenKind::RParen)) {
            if (!check(TokenKind::Identifier)) {
                consume();
                continue;
            }

            const std::string attr_name = cur().lexeme;
            consume();

            if (attr_name == "aligned" || attr_name == "__aligned__") {
                long long align = 16;  // GCC bare aligned => target max useful alignment.
                if (check(TokenKind::LParen)) {
                    consume();
                    if (!check(TokenKind::RParen)) {
                        Node* align_expr = parse_assign_expr();
                        long long align_val = 0;
                        if (align_expr &&
                            eval_const_int(align_expr, &align_val, &struct_tag_def_map_) &&
                            align_val > 0) {
                            align = align_val;
                        }
                    }
                    expect(TokenKind::RParen);
                }
                apply_aligned_attr(align);
            } else if (attr_name == "packed" || attr_name == "__packed__") {
                if (ts) ts->is_packed = true;
                if (check(TokenKind::LParen)) skip_paren_group();
            } else if (attr_name == "vector_size" || attr_name == "__vector_size__") {
                if (check(TokenKind::LParen)) {
                    consume();
                    Node* sz_expr = parse_assign_expr();
                    long long sz_val = 0;
                    eval_const_int(sz_expr, &sz_val, &struct_tag_def_map_);
                    expect(TokenKind::RParen);
                    apply_vector_size_attr(sz_val);
                }
            } else {
                if (check(TokenKind::LParen)) skip_paren_group();
            }

            if (check(TokenKind::Comma)) consume();
        }

        expect(TokenKind::RParen);
        expect(TokenKind::RParen);
    }

    while (check(TokenKind::KwExtension)) consume();
    while (check(TokenKind::KwNoreturn)) consume();

    // If vector_size was set and the base type is already known, compute lanes.
    if (ts) finalize_vector_type(*ts);
}

void Parser::skip_asm() {
    if (check(TokenKind::KwAsm)) {
        consume();
        // Skip optional qualifiers: volatile, __volatile__, goto, inline
        while (check(TokenKind::KwVolatile) || check(TokenKind::KwInline) ||
               check(TokenKind::KwGoto))
            consume();
        if (check(TokenKind::LParen)) skip_paren_group();
    }
}

// ── type parsing ──────────────────────────────────────────────────────────────

TypeSpec Parser::parse_base_type() {
    last_enum_def_ = nullptr;
    TypeSpec ts{};
    ts.array_size = -1;
    ts.array_rank = 0;
    ts.inner_rank = -1;
    for (int i = 0; i < 8; ++i) ts.array_dims[i] = -1;
    ts.align_bytes = 0;
    ts.vector_lanes = 0;
    ts.vector_bytes = 0;
    ts.base = TB_INT;  // default

    bool has_signed   = false;
    bool has_unsigned = false;
    bool has_short    = false;
    int  long_count   = 0;
    bool has_int_kw   = false;
    bool has_char     = false;
    bool has_void     = false;
    bool has_float    = false;
    bool has_double   = false;
    bool has_bool     = false;
    bool has_complex  = false;
    bool has_struct   = false;
    bool has_union    = false;
    bool has_enum     = false;
    bool has_typedef  = false;
    bool done         = false;
    bool base_set     = false;  // true when ts.base was set directly (KwBuiltin, KwInt128, etc.)
    while (!done && !at_end()) {
        TokenKind k = cur().kind;
        switch (k) {
            // Qualifiers / storage class — skip but record const
            case TokenKind::KwConst:    ts.is_const    = true; consume(); break;
            case TokenKind::KwVolatile: ts.is_volatile = true; consume(); break;
            case TokenKind::KwRestrict:
            case TokenKind::KwAtomic:
            case TokenKind::KwRegister:
            case TokenKind::KwInline:
            case TokenKind::KwExtension:
            case TokenKind::KwNoreturn:
            case TokenKind::KwThreadLocal:
                consume(); break;
            case TokenKind::KwAuto:   consume(); break;  // ignore auto storage

            // Signed / unsigned modifiers
            case TokenKind::KwSigned:   has_signed   = true; consume(); break;
            case TokenKind::KwUnsigned: has_unsigned = true; consume(); break;

            // Base type keywords
            case TokenKind::KwVoid:   has_void   = true; consume(); break;
            case TokenKind::KwChar:   has_char   = true; consume(); break;
            case TokenKind::KwShort:  has_short  = true; consume(); break;
            case TokenKind::KwInt:    has_int_kw = true; consume(); break;
            case TokenKind::KwLong:   long_count++;      consume(); break;
            case TokenKind::KwFloat:  has_float  = true; consume(); break;
            case TokenKind::KwDouble: has_double = true; consume(); break;
            case TokenKind::KwBool:   has_bool   = true; consume(); break;
            case TokenKind::KwComplex: has_complex = true; consume(); break;

            case TokenKind::KwInt128:  ts.base = has_unsigned ? TB_UINT128 : TB_INT128;
                                       base_set = true; consume(); done = true; break;
            case TokenKind::KwUInt128: ts.base = TB_UINT128; base_set = true; consume(); done = true; break;

            case TokenKind::KwBuiltin:
                ts.base = TB_VA_LIST;
                ts.tag  = arena_.strdup("__va_list");
                base_set = true;
                consume(); done = true; break;

            case TokenKind::KwAutoType:
                // __auto_type: treat as int for now
                ts.base = TB_INT;
                base_set = true;
                consume(); done = true; break;

            case TokenKind::KwStruct:
                has_struct = true;
                consume();
                done = true;
                break;

            case TokenKind::KwUnion: has_union = true; consume(); done = true; break;
            case TokenKind::KwEnum:  has_enum  = true; consume(); done = true; break;

            case TokenKind::KwTypeofUnqual:
            case TokenKind::KwTypeof: {
                // typeof(type) or typeof(expr)
                bool strip_qual = (cur().kind == TokenKind::KwTypeofUnqual);
                consume();  // consume typeof / typeof_unqual
                expect(TokenKind::LParen);  // consume (
                if (is_type_start()) {
                    // typeof(type): parse type directly, merging outer qualifiers
                    bool save_c = ts.is_const, save_v = ts.is_volatile;
                    ts = parse_type_name();
                    if (strip_qual) {
                        ts.is_const = false;
                        ts.is_volatile = false;
                    }
                    ts.is_const   |= save_c;
                    ts.is_volatile |= save_v;
                    expect(TokenKind::RParen);
                } else if (check(TokenKind::Identifier)) {
                    // typeof(identifier): look up variable or enum constant type
                    std::string id = cur().lexeme;
                    consume();
                    auto vit = var_types_.find(id);
                    if (vit != var_types_.end()) {
                        ts = vit->second;
                    } else {
                        ts.base = TB_INT;  // enum constants and unknowns → int
                    }
                    if (strip_qual) {
                        ts.is_const = false;
                        ts.is_volatile = false;
                    }
                    // Skip remaining tokens (handles expressions like `i + 1`) to closing )
                    int depth = 0;
                    while (!at_end() && !(check(TokenKind::RParen) && depth == 0)) {
                        if (check(TokenKind::LParen)) ++depth;
                        else if (check(TokenKind::RParen)) --depth;
                        consume();
                    }
                    expect(TokenKind::RParen);
                } else {
                    // Complex expression: skip to balanced ) returning int
                    int depth = 1;
                    while (!at_end() && depth > 0) {
                        if (check(TokenKind::LParen)) ++depth;
                        else if (check(TokenKind::RParen)) { if (--depth == 0) { consume(); break; } }
                        consume();
                    }
                    ts.base = TB_INT;
                    if (strip_qual) {
                        ts.is_const = false;
                        ts.is_volatile = false;
                    }
                }
                base_set = true;
                done = true; break;
            }

            case TokenKind::KwStaticAssert: {
                // _Static_assert(expr, msg); - skip
                consume();
                skip_paren_group();
                match(TokenKind::Semi);
                done = true; break;
            }

            case TokenKind::Identifier:
                if (is_typedef_name(cur().lexeme)) {
                    // If we've already seen concrete type specifiers/modifiers,
                    // this identifier is the declarator name (e.g. `int s;` even
                    // when `s` is also a typedef name in outer scope).
                    bool already_have_base =
                        has_signed || has_unsigned || has_short || long_count > 0 ||
                        has_int_kw || has_char || has_void || has_float || has_double || has_bool ||
                        has_struct || has_union || has_enum || base_set;
                    if (!already_have_base) {
                        has_typedef = true;
                        ts.tag      = arena_.strdup(cur().lexeme);
                        consume();
                    }
                    done = true;
                } else {
                    done = true;  // end of type; identifier is the declarator name
                }
                break;

            // __attribute__ may appear within type specs (e.g., packed struct)
            case TokenKind::KwAttribute:
                parse_attributes(&ts);
                break;

            case TokenKind::KwAlignas: {
                consume();
                skip_paren_group();
                break;
            }

            default:
                done = true; break;
        }
    }

    // Handle struct/union/enum after switch (needs extra parsing)
    if (has_struct) {
        Node* sd = parse_struct_or_union(false);
        ts.base = TB_STRUCT;
        ts.tag  = sd ? sd->name : nullptr;
        return ts;
    }
    if (has_union) {
        Node* sd = parse_struct_or_union(true);
        ts.base = TB_UNION;
        ts.tag  = sd ? sd->name : nullptr;
        return ts;
    }
    if (has_enum) {
        Node* ed = parse_enum();
        last_enum_def_ = ed;
        ts.base = TB_ENUM;
        ts.tag  = ed ? ed->name : nullptr;
        return ts;
    }

    // If base was set directly (KwBuiltin, KwInt128, etc.), skip combined-specifier resolution.
    if (base_set) {
        finalize_vector_type(ts);
        return ts;
    }

    // Resolve combined specifiers
    if (has_typedef) {
        // Try to resolve the typedef to its underlying TypeSpec
        const char* tname = ts.tag;
        if (tname) {
            auto it = typedef_types_.find(tname);
            if (it != typedef_types_.end()) {
                // Resolve: use the stored TypeSpec, preserving qualifiers from this context
                bool save_const = ts.is_const, save_vol = ts.is_volatile;
                ts = it->second;
                ts.is_const   |= save_const;
                ts.is_volatile |= save_vol;
                return ts;
            }
        }
        ts.base = TB_TYPEDEF;
        // tag already set above
    } else if (has_void) {
        ts.base = TB_VOID;
    } else if (has_bool) {
        ts.base = TB_BOOL;
    } else if (has_float) {
        ts.base = has_complex ? TB_COMPLEX_FLOAT : TB_FLOAT;
    } else if (has_double) {
        if (has_complex) ts.base = long_count > 0 ? TB_COMPLEX_LONGDOUBLE : TB_COMPLEX_DOUBLE;
        else ts.base = long_count > 0 ? TB_LONGDOUBLE : TB_DOUBLE;
    } else if (has_char) {
        if (has_complex)
            ts.base = has_unsigned ? TB_COMPLEX_UCHAR : TB_COMPLEX_SCHAR;
        else
            ts.base = has_unsigned ? TB_UCHAR : (has_signed ? TB_SCHAR : TB_CHAR);
    } else if (has_short) {
        ts.base = has_complex ? (has_unsigned ? TB_COMPLEX_USHORT : TB_COMPLEX_SHORT)
                              : (has_unsigned ? TB_USHORT : TB_SHORT);
    } else if (long_count >= 2) {
        ts.base = has_complex ? (has_unsigned ? TB_COMPLEX_ULONGLONG : TB_COMPLEX_LONGLONG)
                              : (has_unsigned ? TB_ULONGLONG : TB_LONGLONG);
    } else if (long_count == 1) {
        ts.base = has_complex ? (has_unsigned ? TB_COMPLEX_ULONG : TB_COMPLEX_LONG)
                              : (has_unsigned ? TB_ULONG : TB_LONG);
    } else if (has_complex && !has_int_kw && !has_signed && !has_unsigned) {
        ts.base = TB_COMPLEX_DOUBLE;  // __complex__ with no type = double _Complex
    } else {
        // plain int (possibly unsigned)
        ts.base = has_complex ? (has_unsigned ? TB_COMPLEX_UINT : TB_COMPLEX_INT)
                              : (has_unsigned ? TB_UINT : TB_INT);
    }

    finalize_vector_type(ts);
    return ts;
}

// Parse declarator: pointer stars, name, array/function suffixes.
// Modifies ts in place.  Sets *out_name to the declared name (or nullptr).
// Handles abstract declarators (no name) when out_name is allowed to be null.
void Parser::parse_declarator(TypeSpec& ts, const char** out_name,
                              Node*** out_fn_ptr_params,
                              int* out_n_fn_ptr_params,
                              bool* out_fn_ptr_variadic) {
    if (out_name) *out_name = nullptr;
    if (out_fn_ptr_params) *out_fn_ptr_params = nullptr;
    if (out_n_fn_ptr_params) *out_n_fn_ptr_params = 0;
    if (out_fn_ptr_variadic) *out_fn_ptr_variadic = false;

    auto clear_array = [&](TypeSpec& t) {
        t.array_size = -1;
        t.array_rank = 0;
        for (int i = 0; i < 8; ++i) t.array_dims[i] = -1;
        t.is_ptr_to_array = false;
        t.array_size_expr = nullptr;
    };
    auto base_array_rank = [&]() -> int {
        if (ts.array_rank > 0) return ts.array_rank;
        if (ts.array_size != -1) return 1;
        return 0;
    };
    auto base_array_dim = [&](int i) -> long long {
        if (ts.array_rank > 0) {
            if (i >= 0 && i < ts.array_rank) return ts.array_dims[i];
            return -1;
        }
        if (i == 0 && ts.array_size != -1) return ts.array_size;
        return -1;
    };
    auto apply_decl_dims = [&](const std::vector<long long>& decl_dims) {
        if (decl_dims.empty()) return;
        // Preserve array_size_expr set by parse_one_array_dim (for first dim).
        Node* saved_size_expr = ts.array_size_expr;
        // If the base TypeSpec is already a pointer-to-array (e.g. A3_28* paa[2]),
        // record the old (typedef's) array rank as inner_rank so that llvm_ty
        // can produce [N x ptr] rather than [N x [inner x ...]].
        if (ts.ptr_level > 0 && ts.is_ptr_to_array) {
            ts.inner_rank = base_array_rank();
            ts.is_ptr_to_array = false;
        }
        long long merged[8];
        for (int i = 0; i < 8; ++i) merged[i] = -1;
        int out = 0;
        for (size_t i = 0; i < decl_dims.size() && out < 8; ++i) {
            merged[out++] = decl_dims[i];
        }
        int old_rank = base_array_rank();
        for (int i = 0; i < old_rank && out < 8; ++i) {
            merged[out++] = base_array_dim(i);
        }
        clear_array(ts);
        ts.array_rank = out;
        for (int i = 0; i < out; ++i) ts.array_dims[i] = merged[i];
        ts.array_size = out > 0 ? ts.array_dims[0] : -1;
        // Restore array_size_expr for first dim when it couldn't be evaluated at parse time
        if (saved_size_expr && !decl_dims.empty() && decl_dims[0] <= 0)
            ts.array_size_expr = saved_size_expr;
    };
    auto parse_one_array_dim = [&]() -> long long {
        expect(TokenKind::LBracket);
        long long dim = -2;  // unsized []
        if (!check(TokenKind::RBracket)) {
            // C99 array parameter qualifiers: [static N], [const N], [volatile N],
            // [restrict N], [const *] (VLA pointer). Skip qualifier keywords.
            while (is_qualifier(cur().kind) || cur().kind == TokenKind::KwStatic)
                consume();
            // [const *] or [static *]: unsized VLA pointer
            if (check(TokenKind::Star)) {
                consume();
                expect(TokenKind::RBracket);
                return -2;
            }
            if (check(TokenKind::RBracket)) { expect(TokenKind::RBracket); return dim; }
            Node* sz = parse_assign_expr();
            ts.array_size_expr = sz;
            long long cv = 0;
            if (sz && eval_const_int(sz, &cv, &struct_tag_def_map_) && cv > 0) {
                dim = cv;
            } else if (sz && sz->kind == NK_INT_LIT) {
                dim = sz->ival;
            } else {
                dim = 0;  // dynamic / unknown at parse time
            }
        }
        expect(TokenKind::RBracket);
        return dim;
    };
    std::vector<long long> decl_dims;
    // Skip qualifiers/attributes that can appear before pointer stars
    // This handles trailing qualifiers on the base type: e.g. `struct S const *p`
    // where `const` appears after the struct tag but before `*`.
    parse_attributes(&ts);
    while (is_qualifier(cur().kind)) consume();
    parse_attributes(&ts);

    // Count pointer stars (and qualifiers between them)
    while (check(TokenKind::Star)) {
        consume();
        // If the base type already has array dimensions, this star creates a
        // pointer-to-array (e.g. HARD_REG_SET *p where HARD_REG_SET = ulong[2]).
        if (ts.array_rank > 0 || ts.array_size != -1)
            ts.is_ptr_to_array = true;
        ts.ptr_level++;
        // Skip qualifiers that follow *
        while (is_qualifier(cur().kind)) consume();
        parse_attributes(&ts);
    }

    // Skip any extra qualifiers / nullability (macOS: _Nullable, _Nonnull, etc.)
    while (is_qualifier(cur().kind)) consume();
    if (check(TokenKind::Identifier)) {
        const std::string& lex = cur().lexeme;
        if (lex == "_Nullable" || lex == "_Nonnull" || lex == "_Null_unspecified" ||
            lex == "__nullable" || lex == "__nonnull") {
            consume();
        }
    }
    parse_attributes(&ts);

    bool used_paren_ptr_declarator = false;

    // Check for parenthesised declarator: (*name) or (ATTR *name) — function pointer
    // Peek past optional __attribute__((...)) to find * after (
    auto paren_star_peek = [&]() -> bool {
        if (!check(TokenKind::LParen)) return false;
        int pk = pos_ + 1;
        while (pk < (int)tokens_.size() && tokens_[pk].kind == TokenKind::KwAttribute) {
            pk++;  // skip __attribute__
            if (pk < (int)tokens_.size() && tokens_[pk].kind == TokenKind::LParen) {
                int d = 1; pk++;
                while (pk < (int)tokens_.size() && d > 0) {
                    if (tokens_[pk].kind == TokenKind::LParen) d++;
                    else if (tokens_[pk].kind == TokenKind::RParen) d--;
                    pk++;
                }
            }
        }
        return pk < (int)tokens_.size() &&
               (tokens_[pk].kind == TokenKind::Star || tokens_[pk].kind == TokenKind::Caret);
    };
    if (paren_star_peek()) {
        used_paren_ptr_declarator = true;
        std::vector<Node*> fn_ptr_params;
        bool fn_ptr_variadic = false;
        // function pointer: (*name)(...) or block pointer: (^name)(...) — record name, skip params
        consume();  // consume (
        skip_attributes();  // skip any __attribute__((...)) before * or ^
        consume();  // consume * or ^
        ts.ptr_level++;
        // Skip extra stars for **fpp
        while (check(TokenKind::Star)) { consume(); ts.ptr_level++; }
        // Skip nullability / qualifier annotations inside the parens: (* _Nullable name)
        while (check(TokenKind::Identifier)) {
            const std::string& lex = cur().lexeme;
            if (lex == "_Nullable" || lex == "_Nonnull" || lex == "_Null_unspecified" ||
                lex == "__nullable" || lex == "__nonnull" || lex == "__restrict" ||
                lex == "restrict" || lex == "const" || lex == "volatile") {
                consume();
            } else {
                break;
            }
        }
        // Also skip keyword qualifiers (const, volatile, restrict)
        while (is_qualifier(cur().kind)) consume();
        // Skip array dimensions inside parens: (*[4])(int) — array of function pointers
        // e.g. typedef int (*fptr4[4])(int);  or  int f(int (*[4])(int), ...)
        while (check(TokenKind::LBracket)) {
            consume();  // [
            while (!at_end() && !check(TokenKind::RBracket)) consume();
            if (check(TokenKind::RBracket)) consume();  // ]
        }
        bool got_name = false;
        if (out_name && check(TokenKind::Identifier)) {
            *out_name = arena_.strdup(cur().lexeme);
            consume();
            got_name = true;
        }
        // Skip array dimensions after optional name: (*fptr4[4])(int) or (*[4])(int)
        // e.g. typedef int (*fptr4[4])(int);  or  int f(int (*[4])(int), ...)
        while (check(TokenKind::LBracket)) {
            consume();  // [
            while (!at_end() && !check(TokenKind::RBracket)) consume();
            if (check(TokenKind::RBracket)) consume();  // ]
        }
        // Handle: int (* f1(int a, int b))(int c, int b) — function-returning-fptr
        // Only applies when we read a name AND the next token is '(' (function params).
        if (got_name && check(TokenKind::LParen)) {
            skip_paren_group();  // skip own params: (int a, int b)
        } else if (!got_name && check(TokenKind::LParen)) {
            // Nested declarator: (* (*p)(...)) — parse the inner declarator
            // to extract the name, then skip any trailing param lists
            TypeSpec inner_ts = ts;
            inner_ts.ptr_level = 0;
            parse_declarator(inner_ts, out_name);
            // skip_paren_group was handled inside inner parse_declarator
            // Merge: the inner name is what we want
        }
        expect(TokenKind::RParen);
        // Parse the pointed-to function's parameter list.
        if (check(TokenKind::LParen)) {
            consume();  // (
            if (!check(TokenKind::RParen)) {
                while (!at_end()) {
                    if (check(TokenKind::Ellipsis)) {
                        fn_ptr_variadic = true;
                        consume();
                        break;
                    }
                    if (check(TokenKind::RParen)) break;
                    if (!is_type_start()) {
                        consume();
                        if (match(TokenKind::Comma)) continue;
                        break;
                    }
                    Node* p = parse_param();
                    if (p) fn_ptr_params.push_back(p);
                    if (!match(TokenKind::Comma)) break;
                }
            }
            expect(TokenKind::RParen);
            ts.is_fn_ptr = true;  // confirmed function pointer: (*name)(params)
            if (out_n_fn_ptr_params) *out_n_fn_ptr_params = static_cast<int>(fn_ptr_params.size());
            if (out_fn_ptr_variadic) *out_fn_ptr_variadic = fn_ptr_variadic;
            if (out_fn_ptr_params && !fn_ptr_params.empty()) {
                *out_fn_ptr_params = arena_.alloc_array<Node*>(static_cast<int>(fn_ptr_params.size()));
                for (int i = 0; i < static_cast<int>(fn_ptr_params.size()); ++i) {
                    (*out_fn_ptr_params)[i] = fn_ptr_params[i];
                }
            }
        }
        // Array suffix after function pointer: (*p)[N] / (*p)[N][M]
        while (check(TokenKind::LBracket)) {
            decl_dims.push_back(parse_one_array_dim());
        }
        apply_decl_dims(decl_dims);
        if (!decl_dims.empty()) ts.is_ptr_to_array = used_paren_ptr_declarator;
        return;
    }

    // Grouped declarator: (name) or (name[N]) or (name[N][M]) — e.g. int *(p[25])
    // Covers the case where paren_star_peek was false because the paren contains
    // an identifier (not a pointer star), but the declarator is still grouped.
    if (check(TokenKind::LParen)) {
        int pk = pos_ + 1;
        // Skip attributes inside paren if any
        while (pk < (int)tokens_.size() && tokens_[pk].kind == TokenKind::KwAttribute) {
            pk++;
            if (pk < (int)tokens_.size() && tokens_[pk].kind == TokenKind::LParen) {
                int d = 1; pk++;
                while (pk < (int)tokens_.size() && d > 0) {
                    if (tokens_[pk].kind == TokenKind::LParen) d++;
                    else if (tokens_[pk].kind == TokenKind::RParen) d--;
                    pk++;
                }
            }
        }
        bool is_grouped = pk < (int)tokens_.size() &&
                          tokens_[pk].kind == TokenKind::Identifier;
        if (is_grouped) {
            consume();  // consume '('
            // Parse the inner declarator (name + optional array dims)
            if (out_name && check(TokenKind::Identifier)) {
                *out_name = arena_.strdup(cur().lexeme);
                consume();
            }
            while (check(TokenKind::LBracket))
                decl_dims.push_back(parse_one_array_dim());
            expect(TokenKind::RParen);
            // Any trailing array dims outside the paren (e.g. int (a[2])[3])
            while (check(TokenKind::LBracket))
                decl_dims.push_back(parse_one_array_dim());
            apply_decl_dims(decl_dims);
            if (!decl_dims.empty()) ts.is_ptr_to_array = used_paren_ptr_declarator;
            return;
        }
    }

    parse_attributes(&ts);

    // Normal declarator: optional identifier name
    if (out_name && check(TokenKind::Identifier)) {
        *out_name = arena_.strdup(cur().lexeme);
        consume();
    }

    parse_attributes(&ts);

    // Array suffix: [size] or [] (possibly multi-dimensional)
    while (check(TokenKind::LBracket)) {
        decl_dims.push_back(parse_one_array_dim());
    }
    apply_decl_dims(decl_dims);
    if (!decl_dims.empty()) ts.is_ptr_to_array = used_paren_ptr_declarator;

    // Function suffix: (params) — turns the declarator into a function
    // We only record variadic here; full param parsing is done by caller.
    // (The function definition parser calls parse_params separately.)
}

TypeSpec Parser::parse_type_name() {
    TypeSpec ts = parse_base_type();
    const char* ignored = nullptr;
    parse_declarator(ts, &ignored);
    return ts;
}

// ── struct / union parsing ───────────────────────────────────────────────────

Node* Parser::parse_struct_or_union(bool is_union) {
    int ln = cur().line;
    // Parse attributes before tag name — capture packed attribute.
    TypeSpec attr_ts{};
    parse_attributes(&attr_ts);

    const char* tag = nullptr;
    if (check(TokenKind::Identifier)) {
        tag = arena_.strdup(cur().lexeme);
        consume();
    }
    parse_attributes(&attr_ts);

    if (!check(TokenKind::LBrace)) {
        // Forward reference only; create a reference node
        if (!tag) {
            // anonymous without body — make synthetic tag
            char buf[32];
            snprintf(buf, sizeof(buf), "_anon_%d", anon_counter_++);
            tag = arena_.strdup(buf);
        }
        Node* ref = make_node(NK_STRUCT_DEF, ln);
        ref->name     = tag;
        ref->is_union = is_union;
        ref->n_fields = -1;  // -1 = forward reference (no body)
        return ref;
    }

    // Has body: { field; ... }
    if (!tag) {
        char buf[32];
        snprintf(buf, sizeof(buf), "_anon_%d", anon_counter_++);
        tag = arena_.strdup(buf);
    } else if (defined_struct_tags_.count(tag)) {
        if (parsing_top_level_context_) {
            throw std::runtime_error(std::string("redefinition of ") +
                                     (is_union ? "union " : "struct ") + tag);
        }
        // Block-scoped redefinition of an already-defined tag (C tag shadowing).
        // Generate a unique shadow tag so both definitions coexist in struct_defs_.
        // Variables declared inline with this definition will get the shadow tag.
        char buf[64];
        snprintf(buf, sizeof(buf), "%s.__shadow_%d", tag, anon_counter_++);
        tag = arena_.strdup(buf);
    }
    defined_struct_tags_.insert(tag);

    Node* sd = make_node(NK_STRUCT_DEF, ln);
    sd->name       = tag;
    sd->is_union   = is_union;
    // __attribute__((packed)) => pack_align=1; otherwise use #pragma pack state
    sd->pack_align = attr_ts.is_packed ? 1 : pack_alignment_;

    consume();  // consume {

    std::vector<Node*> fields;
    std::unordered_set<std::string> field_names_seen;
    auto check_dup_field = [&](const char* fname) {
        if (!fname) return;
        std::string n(fname);
        if (field_names_seen.count(n))
            throw std::runtime_error(std::string("duplicate field name: ") + n);
        field_names_seen.insert(n);
    };
    while (!at_end() && !check(TokenKind::RBrace)) {
        skip_attributes();
        if (check(TokenKind::RBrace)) break;

        // Handle nested anonymous struct/union
        if (check(TokenKind::KwStruct) || check(TokenKind::KwUnion)) {
            bool inner_union = (cur().kind == TokenKind::KwUnion);
            consume();
            Node* inner = parse_struct_or_union(inner_union);
            if (inner) struct_defs_.push_back(inner);
            // Parse optional declarator (name, *name, name[], etc.)
            // This handles: struct S s; struct S *p; struct S arr[N]; (anonymous: struct S;)
            TypeSpec anon_fts{};
            anon_fts.array_size = -1;
            anon_fts.array_rank = 0;
            anon_fts.base = inner_union ? TB_UNION : TB_STRUCT;
            anon_fts.tag  = inner ? inner->name : nullptr;
            parse_attributes(&anon_fts);
            bool has_declarator = !check(TokenKind::Semi) && !check(TokenKind::RBrace);
            if (has_declarator) {
                const char* fname = nullptr;
                parse_declarator(anon_fts, &fname);
                    if (fname) {
                        Node* f = make_node(NK_DECL, cur().line);
                        f->type = anon_fts;
                        f->name = fname;
                        check_dup_field(fname);
                        fields.push_back(f);
                } else if (inner && inner->name) {
                    // Declarator consumed pointer stars but no name — treat as anonymous
                    Node* f = make_node(NK_DECL, cur().line);
                    f->type = anon_fts;
                    f->name = inner->name;  // use struct tag as synthetic field name
                    f->is_anon_field = true;
                    fields.push_back(f);
                }
                while (match(TokenKind::Comma)) {
                    TypeSpec fts2{};
                    fts2.array_size = -1;
                    fts2.array_rank = 0;
                    fts2.base = inner_union ? TB_UNION : TB_STRUCT;
                    fts2.tag  = inner ? inner->name : nullptr;
                    const char* fname2 = nullptr;
                    parse_declarator(fts2, &fname2);
                    if (fname2) {
                        Node* f2 = make_node(NK_DECL, cur().line);
                        f2->type = fts2;
                        f2->name = fname2;
                        check_dup_field(fname2);
                        fields.push_back(f2);
                    }
                }
            } else if (inner && inner->name) {
                // Truly anonymous: no declarator at all, just struct/union { ... };
                // Add synthetic field using the tag as name for recursive lookup.
                Node* f = make_node(NK_DECL, cur().line);
                f->type = anon_fts;
                f->name = inner->name;
                f->is_anon_field = true;
                fields.push_back(f);
            }
            match(TokenKind::Semi);
            continue;
        }

        if (check(TokenKind::KwEnum)) {
            consume();
            Node* ed = parse_enum();
            if (ed && parsing_top_level_context_) struct_defs_.push_back(ed);
            // After the enum body, there may be one or more field declarators:
            // e.g.  enum { X } x;   or   enum E { A, B } kind;
            // If the next token starts a declarator (not ; or }), parse it.
            if (!check(TokenKind::Semi) && !check(TokenKind::RBrace)) {
                TypeSpec fts{};
                fts.array_size = -1;
                fts.array_rank = 0;
                // Base type is the enum (use int as fallback since enums are ints)
                fts.base = TB_INT;
                if (ed && ed->name) {
                    // Prefer enum type tag if available
                    fts.base = TB_ENUM;
                    fts.tag  = ed->name;
                }
                bool first_f = true;
                while (true) {
                    TypeSpec cur_fts = fts;
                    const char* fname = nullptr;
                    parse_declarator(cur_fts, &fname);
                    skip_attributes();
                    long long bf_width = -1;
                    if (check(TokenKind::Colon)) {
                        consume();
                        Node* bfw = parse_assign_expr();
                        if (bfw) eval_const_int(bfw, &bf_width, &struct_tag_def_map_);
                    }
                    if (fname) {
                        Node* f = make_node(NK_DECL, cur().line);
                        f->type = cur_fts;
                        f->name = fname;
                        f->ival = bf_width;  // -1 = not a bitfield; N = N-bit bitfield
                        check_dup_field(fname);
                        fields.push_back(f);
                    }
                    (void)first_f; first_f = false;
                    if (!match(TokenKind::Comma)) break;
                }
            }
            match(TokenKind::Semi);
            continue;
        }

        // Regular field declaration
        if (!is_type_start()) {
            // unknown token in struct body — skip
            consume();
            continue;
        }

        TypeSpec fts = parse_base_type();
        parse_attributes(&fts);

        // Handle anonymous bitfield: just ': expr;'
        if (check(TokenKind::Colon)) {
            consume();
            parse_assign_expr();  // skip bitfield width
            match(TokenKind::Semi);
            continue;
        }

        // If semicolon follows immediately: anonymous struct field (no name)
        if (check(TokenKind::Semi)) {
            match(TokenKind::Semi);
            continue;
        }

        bool first = true;
        while (true) {
            TypeSpec cur_fts = fts;
            // Preserve typedef ptr_level and array dims.
            // apply_decl_dims prepends declarator dims to base typedef dims correctly.
            if (!first) {
                // For multi-declarator fields, re-use base type (ptr_level from typedef preserved)
            }
            first = false;
            const char* fname = nullptr;
            parse_declarator(cur_fts, &fname);
            skip_attributes();

            // Bitfield: : expr
            long long bf_width = -1;
            if (check(TokenKind::Colon)) {
                consume();
                Node* bfw = parse_assign_expr();
                if (bfw) eval_const_int(bfw, &bf_width, &struct_tag_def_map_);
            }

            if (fname) {
                Node* f = make_node(NK_DECL, cur().line);
                f->type = cur_fts;
                f->name = fname;
                f->ival = bf_width;  // -1 = not a bitfield; N = N-bit bitfield
                check_dup_field(fname);
                fields.push_back(f);
            }

            if (!match(TokenKind::Comma)) break;
        }
        match(TokenKind::Semi);
    }
    expect(TokenKind::RBrace);

    // Check for trailing __attribute__((packed)) after closing brace.
    // We must only peek, not consume — other attributes like aligned()
    // belong to the enclosing declaration (typedef, variable, etc.).
    if (check(TokenKind::KwAttribute) && sd->pack_align == 0) {
        int save = pos_;
        TypeSpec trailing_attr{};
        parse_attributes(&trailing_attr);
        if (trailing_attr.is_packed) {
            sd->pack_align = 1;
        }
        // Restore position so the caller can re-parse attributes for the declaration.
        pos_ = save;
    }

    // Store fields in arena
    sd->n_fields = (int)fields.size();
    if (sd->n_fields > 0) {
        sd->fields = arena_.alloc_array<Node*>(sd->n_fields);
        for (int i = 0; i < sd->n_fields; ++i) sd->fields[i] = fields[i];
    }

    // Record concrete struct/union definition for parse-time offsetof evaluation.
    if (sd->name) struct_tag_def_map_[sd->name] = sd;
    struct_defs_.push_back(sd);
    return sd;
}

// ── enum parsing ─────────────────────────────────────────────────────────────

Node* Parser::parse_enum() {
    int ln = cur().line;
    skip_attributes();

    const char* tag = nullptr;
    if (check(TokenKind::Identifier)) {
        tag = arena_.strdup(cur().lexeme);
        consume();
    }
    skip_attributes();

    if (!check(TokenKind::LBrace)) {
        if (!tag) {
            char buf[32];
            snprintf(buf, sizeof(buf), "_anon_enum_%d", anon_counter_++);
            tag = arena_.strdup(buf);
        }
        Node* ref = make_node(NK_ENUM_DEF, ln);
        ref->name = tag;
        ref->n_enum_variants = -1;
        return ref;
    }

    if (!tag) {
        char buf[32];
        snprintf(buf, sizeof(buf), "_anon_enum_%d", anon_counter_++);
        tag = arena_.strdup(buf);
    }

    Node* ed = make_node(NK_ENUM_DEF, ln);
    ed->name = tag;

    consume();  // consume {

    std::vector<const char*> names;
    std::vector<long long>   vals;
    std::unordered_set<std::string> seen_names;
    long long cur_val = 0;

    while (!at_end() && !check(TokenKind::RBrace)) {
        skip_attributes();
        if (!check(TokenKind::Identifier)) { consume(); continue; }
        const char* vname = arena_.strdup(cur().lexeme);
        std::string vname_s(vname ? vname : "");
        if (seen_names.count(vname_s))
            throw std::runtime_error("duplicate enumerator: " + vname_s);
        seen_names.insert(vname_s);
        consume();
        // Skip __attribute__((...)) between enum constant name and '='
        // e.g. _CLOCK_REALTIME __attribute__((availability(...))) = 0,
        skip_attributes();
        long long vval = cur_val;
        if (match(TokenKind::Assign)) {
            Node* ve = parse_assign_expr();
            if (!eval_enum_expr(ve, enum_consts_, &vval))
                throw std::runtime_error("enum initializer is not an integer constant expression");
        }
        // Skip __attribute__((...)) annotations after enum value (e.g. availability macros)
        skip_attributes();
        cur_val = vval + 1;
        names.push_back(vname);
        vals.push_back(vval);
        // Track enum constant for use in subsequent enum initializers
        enum_consts_[std::string(vname)] = vval;
        if (!match(TokenKind::Comma)) break;
        // Trailing comma before } is allowed
        if (check(TokenKind::RBrace)) break;
    }
    expect(TokenKind::RBrace);

    ed->n_enum_variants = (int)names.size();
    if (ed->n_enum_variants > 0) {
        ed->enum_names = arena_.alloc_array<const char*>(ed->n_enum_variants);
        ed->enum_vals  = arena_.alloc_array<long long>(ed->n_enum_variants);
        for (int i = 0; i < ed->n_enum_variants; ++i) {
            ed->enum_names[i] = names[i];
            ed->enum_vals[i]  = vals[i];
        }
    }
    if (parsing_top_level_context_) struct_defs_.push_back(ed);
    return ed;
}

// ── parameter parsing ─────────────────────────────────────────────────────────

Node* Parser::parse_param() {
    int ln = cur().line;
    if (check(TokenKind::Ellipsis)) {
        // variadic marker — handled by caller
        consume();
        return nullptr;
    }
    TypeSpec pts = parse_base_type();
    const char* pname = nullptr;
    Node** fn_ptr_params = nullptr;
    int n_fn_ptr_params = 0;
    bool fn_ptr_variadic = false;
    parse_declarator(pts, &pname, &fn_ptr_params, &n_fn_ptr_params, &fn_ptr_variadic);
    // C rule: a parameter of function type decays to a pointer to that function.
    // Abstract function-type params look like: int(int x) or int()
    // Consume the function-type suffix and apply the pointer decay.
    if (check(TokenKind::LParen)) {
        skip_paren_group();
        // The parameter has function type — adjust it to pointer-to-function.
        pts.is_fn_ptr = true;
        pts.ptr_level += 1;
    }
    skip_attributes();

    Node* p = make_node(NK_DECL, ln);
    p->type = pts;
    p->name = pname ? pname : nullptr;
    p->fn_ptr_params = fn_ptr_params;
    p->n_fn_ptr_params = n_fn_ptr_params;
    p->fn_ptr_variadic = fn_ptr_variadic;
    return p;
}

// ── expression parsing ────────────────────────────────────────────────────────

// Operator precedence table (higher = tighter binding).

}  // namespace tinyc2ll::frontend_cxx
