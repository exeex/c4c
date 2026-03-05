#include "parser.hpp"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <stdexcept>

namespace tinyc2ll::frontend_cxx {

// ── enum constant expression evaluator ────────────────────────────────────────
// Evaluate an enum initializer expression, including references to previously
// defined enum constants (across multiple enum declarations).
static long long eval_enum_expr(Node* n, const std::unordered_map<std::string, long long>& consts) {
    if (!n) return 0;
    if (n->kind == NK_INT_LIT) return n->ival;
    if (n->kind == NK_CHAR_LIT) return n->ival;
    if (n->kind == NK_VAR && n->name) {
        auto it = consts.find(n->name);
        if (it != consts.end()) return it->second;
    }
    if (n->kind == NK_UNARY && n->op && strcmp(n->op, "-") == 0 && n->left)
        return -eval_enum_expr(n->left, consts);
    if (n->kind == NK_UNARY && n->op && strcmp(n->op, "+") == 0 && n->left)
        return eval_enum_expr(n->left, consts);
    if (n->kind == NK_CAST && n->left)
        return eval_enum_expr(n->left, consts);
    if (n->kind == NK_BINOP && n->op && n->left && n->right) {
        long long l = eval_enum_expr(n->left, consts);
        long long r = eval_enum_expr(n->right, consts);
        if (strcmp(n->op, "+") == 0) return l + r;
        if (strcmp(n->op, "-") == 0) return l - r;
        if (strcmp(n->op, "*") == 0) return l * r;
        if (strcmp(n->op, "&") == 0) return l & r;
        if (strcmp(n->op, "|") == 0) return l | r;
        if (strcmp(n->op, "^") == 0) return l ^ r;
        if (strcmp(n->op, "<<") == 0) return l << r;
        if (strcmp(n->op, ">>") == 0) return l >> r;
        if (strcmp(n->op, "/") == 0 && r != 0) return l / r;
        if (strcmp(n->op, "%") == 0 && r != 0) return l % r;
    }
    return 0;
}

// ── constant expression evaluator ─────────────────────────────────────────────
// Evaluate a simple AST subtree as an integer constant (for array sizes).
// Returns true and sets *out on success; false if the expression is dynamic.
static bool eval_const_int(Node* n, long long* out) {
    if (!n) return false;
    if (n->kind == NK_INT_LIT || n->kind == NK_CHAR_LIT) {
        *out = n->ival;
        return true;
    }
    if (n->kind == NK_UNARY && n->op && strcmp(n->op, "-") == 0 && n->left) {
        long long v;
        if (!eval_const_int(n->left, &v)) return false;
        *out = -v;
        return true;
    }
    if (n->kind == NK_BINOP && n->op) {
        long long l, r;
        if (!eval_const_int(n->left, &l)) return false;
        if (!eval_const_int(n->right, &r)) return false;
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
    }
    return false;
}

// ── helpers ───────────────────────────────────────────────────────────────────

static bool is_qualifier(TokenKind k) {
    return k == TokenKind::KwConst    || k == TokenKind::KwVolatile ||
           k == TokenKind::KwRestrict || k == TokenKind::KwAtomic   ||
           k == TokenKind::KwInline;
}

static bool is_storage_class(TokenKind k) {
    return k == TokenKind::KwStatic   || k == TokenKind::KwExtern ||
           k == TokenKind::KwRegister || k == TokenKind::KwAuto   ||
           k == TokenKind::KwTypedef  || k == TokenKind::KwInline ||
           k == TokenKind::KwExtension || k == TokenKind::KwNoreturn ||
           k == TokenKind::KwThreadLocal;
}

static bool is_type_kw(TokenKind k) {
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
        case TokenKind::KwUnion:
        case TokenKind::KwEnum:
        case TokenKind::KwInt128:
        case TokenKind::KwUInt128:
        case TokenKind::KwBuiltin:   // __builtin_va_list
        case TokenKind::KwAutoType:  // __auto_type
        case TokenKind::KwTypeof:
        case TokenKind::KwComplex:
            return true;
        default:
            return false;
    }
}

// parse an integer literal from a lexeme string
// Returns true if a numeric literal lexeme has an imaginary suffix (i/j/I/J).
static bool lexeme_is_imaginary(const char* s) {
    size_t len = strlen(s);
    for (size_t k = 0; k < len; k++) {
        char c = s[k];
        if (c == 'i' || c == 'I' || c == 'j' || c == 'J') return true;
    }
    return false;
}

static long long parse_int_lexeme(const char* s) {
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

static double parse_float_lexeme(const char* s) {
    // strip f/F/l/L and imaginary i/j/I/J suffixes (any order)
    char buf[64];
    size_t len = strlen(s);
    if (len >= sizeof(buf)) len = sizeof(buf) - 1;
    strncpy(buf, s, len);
    buf[len] = '\0';
    size_t end = len;
    while (end > 0) {
        char c = buf[end - 1];
        if (c == 'f' || c == 'F' || c == 'l' || c == 'L' ||
            c == 'i' || c == 'I' || c == 'j' || c == 'J') { --end; }
        else { break; }
    }
    buf[end] = '\0';
    return strtod(buf, nullptr);
}

// ── Parser constructor ────────────────────────────────────────────────────────

Parser::Parser(std::vector<Token> tokens, Arena& arena)
    : tokens_(std::move(tokens)), pos_(0), arena_(arena), anon_counter_(0), had_error_(false) {
    // Pre-seed well-known typedef names from standard / system headers
    // so the parser can disambiguate type-name vs identifier.
    static const char* seed[] = {
        "size_t", "ssize_t", "ptrdiff_t", "intptr_t", "uintptr_t",
        "int8_t", "int16_t", "int32_t", "int64_t",
        "uint8_t", "uint16_t", "uint32_t", "uint64_t",
        "int_least8_t", "int_least16_t", "int_least32_t", "int_least64_t",
        "uint_least8_t","uint_least16_t","uint_least32_t","uint_least64_t",
        "int_fast8_t", "int_fast16_t", "int_fast32_t", "int_fast64_t",
        "uint_fast8_t", "uint_fast16_t", "uint_fast32_t", "uint_fast64_t",
        "intmax_t", "uintmax_t",
        "off_t", "pid_t", "uid_t", "gid_t",
        "time_t", "clock_t", "suseconds_t",
        "FILE", "DIR", "fpos_t",
        "va_list", "__va_list", "__va_list_tag",
        "wchar_t", "wint_t", "wctype_t",
        "bool", "true", "false",
        "NULL",
        "__builtin_va_list", "__gnuc_va_list",
        "jmp_buf", "sigjmp_buf",
        "pthread_t", "pthread_mutex_t", "pthread_cond_t",
        "socklen_t", "in_addr_t", "in_port_t",
        "mode_t", "ino_t", "dev_t", "nlink_t", "blksize_t", "blkcnt_t",
        "div_t", "ldiv_t", "lldiv_t",
        "__int128_t", "__uint128_t",
        "OSStatus", "OSErr",
        nullptr
    };
    for (int i = 0; seed[i]; ++i) typedefs_.insert(seed[i]);

    // Seed typedef_types_ for well-known types so they resolve to correct LLVM types
    // instead of the TB_TYPEDEF fallback (which emits i32).
    {
        TypeSpec va_ts{};
        va_ts.array_size = -1;
        va_ts.array_rank = 0;
        va_ts.is_ptr_to_array = false;
        va_ts.base = TB_VA_LIST;
        typedef_types_["va_list"]          = va_ts;
        typedef_types_["__va_list"]        = va_ts;
        typedef_types_["__builtin_va_list"]= va_ts;
        typedef_types_["__gnuc_va_list"]   = va_ts;

        // size_t / uintptr_t etc. → u64 on 64-bit platforms
        TypeSpec u64_ts{};
        u64_ts.array_size = -1;
        u64_ts.array_rank = 0;
        u64_ts.is_ptr_to_array = false;
        u64_ts.base = TB_ULONGLONG;  // 64-bit unsigned
        typedef_types_["size_t"]    = u64_ts;
        typedef_types_["uintptr_t"] = u64_ts;
        typedef_types_["uintmax_t"] = u64_ts;
        typedef_types_["uint64_t"]  = u64_ts;
        typedef_types_["uint_least64_t"] = u64_ts;
        typedef_types_["uint_fast64_t"]  = u64_ts;

        TypeSpec i64_ts{};
        i64_ts.array_size = -1;
        i64_ts.array_rank = 0;
        i64_ts.is_ptr_to_array = false;
        i64_ts.base = TB_LONGLONG;
        typedef_types_["ssize_t"]   = i64_ts;
        typedef_types_["ptrdiff_t"] = i64_ts;
        typedef_types_["intptr_t"]  = i64_ts;
        typedef_types_["intmax_t"]  = i64_ts;
        typedef_types_["int64_t"]   = i64_ts;
        typedef_types_["int_least64_t"] = i64_ts;
        typedef_types_["int_fast64_t"]  = i64_ts;
        typedef_types_["off_t"]     = i64_ts;

        TypeSpec u32_ts{};
        u32_ts.array_size = -1;
        u32_ts.array_rank = 0;
        u32_ts.is_ptr_to_array = false;
        u32_ts.base = TB_UINT;
        typedef_types_["uint32_t"]  = u32_ts;
        typedef_types_["uint_least32_t"] = u32_ts;
        typedef_types_["uint_fast32_t"]  = u32_ts;

        TypeSpec i32_ts{};
        i32_ts.array_size = -1;
        i32_ts.array_rank = 0;
        i32_ts.is_ptr_to_array = false;
        i32_ts.base = TB_INT;
        typedef_types_["int32_t"]   = i32_ts;
        typedef_types_["int_least32_t"] = i32_ts;
        typedef_types_["int_fast32_t"]  = i32_ts;

        TypeSpec u16_ts{};
        u16_ts.array_size = -1;
        u16_ts.array_rank = 0;
        u16_ts.is_ptr_to_array = false;
        u16_ts.base = TB_USHORT;
        typedef_types_["uint16_t"]  = u16_ts;
        typedef_types_["uint_least16_t"] = u16_ts;
        typedef_types_["uint_fast16_t"]  = u16_ts;

        TypeSpec i16_ts{};
        i16_ts.array_size = -1;
        i16_ts.array_rank = 0;
        i16_ts.is_ptr_to_array = false;
        i16_ts.base = TB_SHORT;
        typedef_types_["int16_t"]   = i16_ts;
        typedef_types_["int_least16_t"] = i16_ts;
        typedef_types_["int_fast16_t"]  = i16_ts;

        TypeSpec u8_ts{};
        u8_ts.array_size = -1;
        u8_ts.array_rank = 0;
        u8_ts.is_ptr_to_array = false;
        u8_ts.base = TB_UCHAR;
        typedef_types_["uint8_t"]   = u8_ts;
        typedef_types_["uint_least8_t"] = u8_ts;
        typedef_types_["uint_fast8_t"]  = u8_ts;

        TypeSpec i8_ts{};
        i8_ts.array_size = -1;
        i8_ts.array_rank = 0;
        i8_ts.is_ptr_to_array = false;
        i8_ts.base = TB_SCHAR;
        typedef_types_["int8_t"]    = i8_ts;
        typedef_types_["int_least8_t"] = i8_ts;
        typedef_types_["int_fast8_t"]  = i8_ts;

        // wchar_t on macOS/ARM64 is i32
        TypeSpec wchar_ts{};
        wchar_ts.array_size = -1;
        wchar_ts.array_rank = 0;
        wchar_ts.is_ptr_to_array = false;
        wchar_ts.base = TB_INT;
        typedef_types_["wchar_t"]  = wchar_ts;
        typedef_types_["wint_t"]   = wchar_ts;
    }
}

// ── token cursor helpers ──────────────────────────────────────────────────────

const Token& Parser::cur() const {
    return tokens_[pos_];
}

const Token& Parser::peek(int offset) const {
    int idx = pos_ + offset;
    if (idx < 0 || idx >= (int)tokens_.size()) {
        static Token eof{TokenKind::EndOfFile, "", 0, 0};
        return eof;
    }
    return tokens_[idx];
}

const Token& Parser::consume() {
    const Token& t = tokens_[pos_];
    if (pos_ + 1 < (int)tokens_.size()) ++pos_;
    return t;
}

bool Parser::at_end() const {
    return tokens_[pos_].kind == TokenKind::EndOfFile;
}

bool Parser::check(TokenKind k) const {
    return tokens_[pos_].kind == k;
}

bool Parser::check2(TokenKind k) const {
    int idx = pos_ + 1;
    if (idx >= (int)tokens_.size()) return false;
    return tokens_[idx].kind == k;
}

bool Parser::match(TokenKind k) {
    if (check(k)) { consume(); return true; }
    return false;
}

void Parser::expect(TokenKind k) {
    if (!check(k)) {
        std::ostringstream msg;
        msg << "expected " << token_kind_name(k) << " but got '"
            << cur().lexeme << "' at line " << cur().line;
        throw std::runtime_error(msg.str());
    }
    consume();
}

void Parser::skip_until(TokenKind k) {
    while (!at_end() && !check(k)) consume();
    if (!at_end()) consume();  // consume the terminator
}

// ── typedef / type-start helpers ─────────────────────────────────────────────

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
    TypeSpec ts{};
    ts.array_size = -1;
    ts.array_rank = 0;
    for (int i = 0; i < 8; ++i) ts.array_dims[i] = -1;
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

            case TokenKind::KwTypeof: {
                // typeof(expr) or typeof(type): skip to get contained type
                consume();
                expect(TokenKind::LParen);
                // skip contents - we'll just treat as int for now
                skip_until(TokenKind::RParen);
                ts.base = TB_INT;
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
                skip_attributes();
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
        ts.base = TB_ENUM;
        ts.tag  = ed ? ed->name : nullptr;
        return ts;
    }

    // If base was set directly (KwBuiltin, KwInt128, etc.), skip combined-specifier resolution.
    if (base_set) return ts;

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
    } else if (has_complex) {
        ts.base = TB_COMPLEX_DOUBLE;  // __complex__ with no type = double _Complex
    } else {
        // plain int (possibly unsigned)
        ts.base = has_complex ? (has_unsigned ? TB_COMPLEX_UINT : TB_COMPLEX_INT)
                              : (has_unsigned ? TB_UINT : TB_INT);
    }

    return ts;
}

// Parse declarator: pointer stars, name, array/function suffixes.
// Modifies ts in place.  Sets *out_name to the declared name (or nullptr).
// Handles abstract declarators (no name) when out_name is allowed to be null.
void Parser::parse_declarator(TypeSpec& ts, const char** out_name) {
    if (out_name) *out_name = nullptr;

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
            if (sz && eval_const_int(sz, &cv) && cv > 0) {
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
    skip_attributes();

    // Count pointer stars (and qualifiers between them)
    while (check(TokenKind::Star)) {
        consume();
        ts.ptr_level++;
        // Skip qualifiers that follow *
        while (is_qualifier(cur().kind)) consume();
        skip_attributes();
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
    skip_attributes();

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
        // Parse (and discard) the return-fptr's param list
        if (check(TokenKind::LParen)) {
            skip_paren_group();
            ts.is_fn_ptr = true;  // confirmed function pointer: (*name)(params)
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

    // Normal declarator: optional identifier name
    if (out_name && check(TokenKind::Identifier)) {
        *out_name = arena_.strdup(cur().lexeme);
        consume();
    }

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
    skip_attributes();

    const char* tag = nullptr;
    if (check(TokenKind::Identifier)) {
        tag = arena_.strdup(cur().lexeme);
        consume();
    }
    skip_attributes();

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
        // Block-scoped redefinition of an already-defined tag (C tag shadowing).
        // Generate a unique shadow tag so both definitions coexist in struct_defs_.
        // Variables declared inline with this definition will get the shadow tag.
        char buf[64];
        snprintf(buf, sizeof(buf), "%s.__shadow_%d", tag, anon_counter_++);
        tag = arena_.strdup(buf);
    }
    defined_struct_tags_.insert(tag);

    Node* sd = make_node(NK_STRUCT_DEF, ln);
    sd->name     = tag;
    sd->is_union = is_union;

    consume();  // consume {

    std::vector<Node*> fields;
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
            bool has_declarator = !check(TokenKind::Semi) && !check(TokenKind::RBrace);
            if (has_declarator) {
                const char* fname = nullptr;
                parse_declarator(anon_fts, &fname);
                if (fname) {
                    Node* f = make_node(NK_DECL, cur().line);
                    f->type = anon_fts;
                    f->name = fname;
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
            if (ed) struct_defs_.push_back(ed);
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
                    if (check(TokenKind::Colon)) {
                        consume();
                        parse_assign_expr();  // skip bitfield width
                    }
                    if (fname) {
                        Node* f = make_node(NK_DECL, cur().line);
                        f->type = cur_fts;
                        f->name = fname;
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
        skip_attributes();

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
            // Preserve ptr_level from typedef resolution (e.g. typedef void *tree; tree field;)
            // parse_declarator will add any additional pointer levels from the declarator syntax.
            cur_fts.array_size = -1;
            cur_fts.array_rank = 0;
            for (int i = 0; i < 8; ++i) cur_fts.array_dims[i] = -1;
            cur_fts.is_ptr_to_array = false;
            if (!first) {
                // For multi-declarator fields, re-use base type (ptr_level from typedef preserved)
            }
            first = false;
            const char* fname = nullptr;
            parse_declarator(cur_fts, &fname);
            skip_attributes();

            // Bitfield: : expr  (skip)
            if (check(TokenKind::Colon)) {
                consume();
                parse_assign_expr();  // skip width
            }

            if (fname) {
                Node* f = make_node(NK_DECL, cur().line);
                f->type = cur_fts;
                f->name = fname;
                fields.push_back(f);
            }

            if (!match(TokenKind::Comma)) break;
        }
        match(TokenKind::Semi);
    }
    expect(TokenKind::RBrace);
    skip_attributes();

    // Store fields in arena
    sd->n_fields = (int)fields.size();
    if (sd->n_fields > 0) {
        sd->fields = arena_.alloc_array<Node*>(sd->n_fields);
        for (int i = 0; i < sd->n_fields; ++i) sd->fields[i] = fields[i];
    }

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
    long long cur_val = 0;

    while (!at_end() && !check(TokenKind::RBrace)) {
        skip_attributes();
        if (!check(TokenKind::Identifier)) { consume(); continue; }
        const char* vname = arena_.strdup(cur().lexeme);
        consume();
        // Skip __attribute__((...)) between enum constant name and '='
        // e.g. _CLOCK_REALTIME __attribute__((availability(...))) = 0,
        skip_attributes();
        long long vval = cur_val;
        if (match(TokenKind::Assign)) {
            Node* ve = parse_assign_expr();
            vval = eval_enum_expr(ve, enum_consts_);
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
    struct_defs_.push_back(ed);
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
    parse_declarator(pts, &pname);
    // C rule: a parameter of function type decays to a pointer to that function.
    // Abstract function-type params look like: int(int x) or int()
    // Consume the function-type suffix so the caller's param list stays in sync.
    if (check(TokenKind::LParen)) {
        skip_paren_group();
    }
    skip_attributes();

    Node* p = make_node(NK_DECL, ln);
    p->type = pts;
    p->name = pname ? pname : nullptr;
    return p;
}

// ── expression parsing ────────────────────────────────────────────────────────

// Operator precedence table (higher = tighter binding).
int Parser::bin_prec(TokenKind k) {
    switch (k) {
        case TokenKind::PipePipe:       return 4;
        case TokenKind::AmpAmp:         return 5;
        case TokenKind::Pipe:           return 6;
        case TokenKind::Caret:          return 7;
        case TokenKind::Amp:            return 8;
        case TokenKind::EqualEqual:
        case TokenKind::BangEqual:      return 9;
        case TokenKind::Less:
        case TokenKind::LessEqual:
        case TokenKind::Greater:
        case TokenKind::GreaterEqual:   return 10;
        case TokenKind::LessLess:
        case TokenKind::GreaterGreater: return 11;
        case TokenKind::Plus:
        case TokenKind::Minus:          return 12;
        case TokenKind::Star:
        case TokenKind::Slash:
        case TokenKind::Percent:        return 13;
        default:                        return -1;
    }
}

Node* Parser::parse_expr() {
    Node* lhs = parse_assign_expr();
    while (check(TokenKind::Comma)) {
        int ln = cur().line;
        consume();
        Node* rhs = parse_assign_expr();
        Node* n = make_node(NK_COMMA_EXPR, ln);
        n->left  = lhs;
        n->right = rhs;
        lhs = n;
    }
    return lhs;
}

Node* Parser::parse_assign_expr() {
    Node* lhs = parse_ternary();
    int ln = cur().line;
    TokenKind k = cur().kind;
    const char* op = nullptr;
    switch (k) {
        case TokenKind::Assign:               op = "=";   break;
        case TokenKind::PlusAssign:           op = "+=";  break;
        case TokenKind::MinusAssign:          op = "-=";  break;
        case TokenKind::StarAssign:           op = "*=";  break;
        case TokenKind::SlashAssign:          op = "/=";  break;
        case TokenKind::PercentAssign:        op = "%=";  break;
        case TokenKind::AmpAssign:            op = "&=";  break;
        case TokenKind::PipeAssign:           op = "|=";  break;
        case TokenKind::CaretAssign:          op = "^=";  break;
        case TokenKind::LessLessAssign:       op = "<<="; break;
        case TokenKind::GreaterGreaterAssign: op = ">>="; break;
        default: break;
    }
    if (op) {
        consume();
        Node* rhs = parse_assign_expr();  // right-associative
        if (op[0] == '=' && op[1] == '\0') {
            return make_assign("=", lhs, rhs, ln);
        } else {
            return make_assign(op, lhs, rhs, ln);
        }
    }
    return lhs;
}

Node* Parser::parse_ternary() {
    Node* cond = parse_binary(4);
    if (!check(TokenKind::Question)) return cond;
    int ln = cur().line;
    consume();  // ?

    Node* then_node;
    if (check(TokenKind::Colon)) {
        // GNU extension: cond ?: else  (omitted-middle)
        then_node = cond;
    } else {
        then_node = parse_expr();
    }
    expect(TokenKind::Colon);
    Node* else_node = parse_assign_expr();

    Node* n = make_node(NK_TERNARY, ln);
    n->cond  = cond;
    n->then_ = then_node;
    n->else_ = else_node;
    return n;
}

Node* Parser::parse_binary(int min_prec) {
    Node* lhs = parse_unary();
    while (true) {
        int prec = bin_prec(cur().kind);
        if (prec < min_prec) break;
        TokenKind k = cur().kind;
        int ln = cur().line;
        const char* op;
        switch (k) {
            case TokenKind::Plus:             op = "+";  break;
            case TokenKind::Minus:            op = "-";  break;
            case TokenKind::Star:             op = "*";  break;
            case TokenKind::Slash:            op = "/";  break;
            case TokenKind::Percent:          op = "%";  break;
            case TokenKind::Amp:              op = "&";  break;
            case TokenKind::Pipe:             op = "|";  break;
            case TokenKind::Caret:            op = "^";  break;
            case TokenKind::LessLess:         op = "<<"; break;
            case TokenKind::GreaterGreater:   op = ">>"; break;
            case TokenKind::EqualEqual:       op = "=="; break;
            case TokenKind::BangEqual:        op = "!="; break;
            case TokenKind::Less:             op = "<";  break;
            case TokenKind::LessEqual:        op = "<="; break;
            case TokenKind::Greater:          op = ">";  break;
            case TokenKind::GreaterEqual:     op = ">="; break;
            case TokenKind::AmpAmp:           op = "&&"; break;
            case TokenKind::PipePipe:         op = "||"; break;
            default:                          op = "?";  break;
        }
        consume();
        Node* rhs = parse_binary(prec + 1);  // left-associative
        lhs = make_binop(op, lhs, rhs, ln);
    }
    return lhs;
}

Node* Parser::parse_unary() {
    int ln = cur().line;
    switch (cur().kind) {
        case TokenKind::Bang: {
            consume();
            Node* operand = parse_unary();
            return make_unary("!", operand, ln);
        }
        case TokenKind::Tilde: {
            consume();
            Node* operand = parse_unary();
            return make_unary("~", operand, ln);
        }
        case TokenKind::Minus: {
            consume();
            Node* operand = parse_unary();
            return make_unary("-", operand, ln);
        }
        case TokenKind::Plus: {
            consume();
            Node* operand = parse_unary();
            return make_unary("+", operand, ln);
        }
        case TokenKind::PlusPlus: {
            consume();
            Node* operand = parse_unary();
            return make_unary("++pre", operand, ln);
        }
        case TokenKind::MinusMinus: {
            consume();
            Node* operand = parse_unary();
            return make_unary("--pre", operand, ln);
        }
        case TokenKind::AmpAmp: {
            // GCC label-address: &&label_name  (computed goto extension)
            consume();
            if (check(TokenKind::Identifier)) {
                // Emit as a null void* placeholder; computed goto not fully supported
                const char* lbl_name = arena_.strdup(cur().lexeme);
                consume();
                Node* n = make_node(NK_INT_LIT, ln);
                n->ival = 0;
                n->name = lbl_name;  // store label name for future use
                return n;
            }
            // Fall back: treat as bitwise AND of two address-of
            Node* operand = parse_unary();
            Node* addr = make_node(NK_ADDR, ln);
            addr->left = operand;
            Node* addr2 = make_node(NK_ADDR, ln);
            addr2->left = addr;
            return addr2;
        }
        case TokenKind::Amp: {
            consume();
            Node* operand = parse_unary();
            Node* n = make_node(NK_ADDR, ln);
            n->left = operand;
            return n;
        }
        case TokenKind::Star: {
            consume();
            Node* operand = parse_unary();
            Node* n = make_node(NK_DEREF, ln);
            n->left = operand;
            return n;
        }
        case TokenKind::KwSizeof: {
            consume();
            if (check(TokenKind::LParen)) {
                // Could be sizeof(type) or sizeof(expr)
                // Disambiguate: if first token inside is a type keyword, it's a type
                int save_pos = pos_;
                consume();  // consume (
                if (is_type_start()) {
                    TypeSpec ts = parse_type_name();
                    expect(TokenKind::RParen);
                    Node* n = make_node(NK_SIZEOF_TYPE, ln);
                    n->type = ts;
                    return n;
                } else {
                    // Restore and parse as sizeof(expr)
                    pos_ = save_pos;
                    consume();  // consume (
                    Node* inner = parse_assign_expr();
                    expect(TokenKind::RParen);
                    Node* n = make_node(NK_SIZEOF_EXPR, ln);
                    n->left = inner;
                    return n;
                }
            } else {
                Node* inner = parse_unary();
                Node* n = make_node(NK_SIZEOF_EXPR, ln);
                n->left = inner;
                return n;
            }
        }
        case TokenKind::KwAlignof:
        case TokenKind::KwGnuAlignof: {
            consume();
            expect(TokenKind::LParen);
            if (is_type_start()) {
                TypeSpec ts = parse_type_name();
                expect(TokenKind::RParen);
                Node* n = make_node(NK_ALIGNOF_TYPE, ln);
                n->type = ts;
                return n;
            } else {
                Node* inner = parse_assign_expr();
                expect(TokenKind::RParen);
                Node* n = make_node(NK_SIZEOF_EXPR, ln);
                n->left = inner;
                return n;
            }
        }
        case TokenKind::KwBuiltinVaArg: {
            // __builtin_va_arg(ap, type)
            consume();
            expect(TokenKind::LParen);
            Node* ap = parse_assign_expr();
            expect(TokenKind::Comma);
            TypeSpec ts = parse_type_name();
            expect(TokenKind::RParen);
            Node* n = make_node(NK_VA_ARG, ln);
            n->left = ap;
            n->type = ts;
            return n;
        }
        case TokenKind::KwGccReal: {
            // GNU extension: __real__ expr
            consume();
            Node* operand = parse_unary();
            Node* n = make_node(NK_REAL_PART, ln);
            n->left = operand;
            return n;
        }
        case TokenKind::KwGccImag: {
            // GNU extension: __imag__ expr
            consume();
            Node* operand = parse_unary();
            Node* n = make_node(NK_IMAG_PART, ln);
            n->left = operand;
            return n;
        }
        default:
            return parse_postfix(parse_primary());
    }
}

Node* Parser::parse_postfix(Node* base) {
    while (true) {
        int ln = cur().line;
        switch (cur().kind) {
            case TokenKind::PlusPlus: {
                consume();
                Node* n = make_node(NK_POSTFIX, ln);
                n->op   = "++";
                n->left = base;
                base = n;
                break;
            }
            case TokenKind::MinusMinus: {
                consume();
                Node* n = make_node(NK_POSTFIX, ln);
                n->op   = "--";
                n->left = base;
                base = n;
                break;
            }
            case TokenKind::Dot: {
                consume();
                if (!check(TokenKind::Identifier)) break;
                const char* fname = arena_.strdup(cur().lexeme);
                consume();
                Node* n = make_node(NK_MEMBER, ln);
                n->left     = base;
                n->name     = fname;
                n->is_arrow = false;
                base = n;
                break;
            }
            case TokenKind::Arrow: {
                consume();
                if (!check(TokenKind::Identifier)) break;
                const char* fname = arena_.strdup(cur().lexeme);
                consume();
                Node* n = make_node(NK_MEMBER, ln);
                n->left     = base;
                n->name     = fname;
                n->is_arrow = true;
                base = n;
                break;
            }
            case TokenKind::LBracket: {
                consume();
                Node* idx = parse_expr();
                expect(TokenKind::RBracket);
                Node* n = make_node(NK_INDEX, ln);
                n->left  = base;
                n->right = idx;
                base = n;
                break;
            }
            case TokenKind::LParen: {
                // Function call
                consume();
                std::vector<Node*> args;
                while (!at_end() && !check(TokenKind::RParen)) {
                    args.push_back(parse_assign_expr());
                    if (!match(TokenKind::Comma)) break;
                }
                expect(TokenKind::RParen);
                Node* n = make_node(NK_CALL, ln);
                n->left       = base;
                n->n_children = (int)args.size();
                if (n->n_children > 0) {
                    n->children = arena_.alloc_array<Node*>(n->n_children);
                    for (int i = 0; i < n->n_children; ++i) n->children[i] = args[i];
                }
                base = n;
                break;
            }
            default:
                return base;
        }
    }
}

Node* Parser::parse_primary() {
    int ln = cur().line;

    // Integer literal
    if (check(TokenKind::IntLit)) {
        long long v = parse_int_lexeme(cur().lexeme.c_str());
        bool imag   = lexeme_is_imaginary(cur().lexeme.c_str());
        const char* lex = arena_.strdup(cur().lexeme);
        consume();
        Node* n = make_int_lit(v, ln);
        n->sval = lex;
        n->is_imaginary = imag;
        return n;
    }

    // Float literal
    if (check(TokenKind::FloatLit)) {
        bool imag   = lexeme_is_imaginary(cur().lexeme.c_str());
        double v    = parse_float_lexeme(cur().lexeme.c_str());
        const char* raw = arena_.strdup(cur().lexeme);
        consume();
        Node* n = make_float_lit(v, raw, ln);
        n->is_imaginary = imag;
        return n;
    }

    // String literal (possibly multiple adjacent)
    if (check(TokenKind::StrLit)) {
        std::string combined = cur().lexeme;
        consume();
        while (check(TokenKind::StrLit)) {
            // Adjacent string concatenation: trim closing quote of first,
            // trim opening quote of second, join
            if (!combined.empty() && combined.back() == '"') combined.pop_back();
            const std::string& next = cur().lexeme;
            if (!next.empty() && next.front() == '"') {
                combined += next.substr(1);
            } else {
                combined += next;
            }
            consume();
        }
        return make_str_lit(arena_.strdup(combined), ln);
    }

    // Char literal
    if (check(TokenKind::CharLit)) {
        const std::string& lex = cur().lexeme;
        long long cv = 0;
        // lex is like 'a' or '\n' or L'\0' etc.
        // Skip wide/unicode prefix if present (L, u, U)
        int char_start = 0;
        if (!lex.empty() && (lex[0] == 'L' || lex[0] == 'u' || lex[0] == 'U'))
            char_start = 1;
        // Now lex[char_start] should be the opening quote '
        if ((int)lex.size() >= char_start + 3) {
            if (lex[char_start + 1] == '\\') {
                switch (lex[char_start + 2]) {
                    case 'n':  cv = '\n'; break;
                    case 't':  cv = '\t'; break;
                    case 'r':  cv = '\r'; break;
                    case '0':  cv = 0;   break;
                    case '\\': cv = '\\'; break;
                    case '\'': cv = '\''; break;
                    case '"':  cv = '"';  break;
                    case 'a':  cv = '\a'; break;
                    case 'b':  cv = '\b'; break;
                    case 'f':  cv = '\f'; break;
                    case 'v':  cv = '\v'; break;
                    case 'x': {
                        // \xNN
                        cv = strtol(lex.c_str() + char_start + 3, nullptr, 16);
                        break;
                    }
                    case 'u': {
                        // \uXXXX — 4 hex digits
                        const char* p = lex.c_str() + char_start + 3;
                        cv = 0;
                        for (int k = 0; k < 4 && *p && isxdigit((unsigned char)*p); k++, p++)
                            cv = cv * 16 + (isdigit((unsigned char)*p) ? *p - '0' :
                                            tolower((unsigned char)*p) - 'a' + 10);
                        break;
                    }
                    case 'U': {
                        // \UXXXXXXXX — 8 hex digits
                        const char* p = lex.c_str() + char_start + 3;
                        cv = 0;
                        for (int k = 0; k < 8 && *p && isxdigit((unsigned char)*p); k++, p++)
                            cv = cv * 16 + (isdigit((unsigned char)*p) ? *p - '0' :
                                            tolower((unsigned char)*p) - 'a' + 10);
                        break;
                    }
                    default:
                        if (lex[char_start + 2] >= '0' && lex[char_start + 2] <= '7') {
                            cv = strtol(lex.c_str() + char_start + 2, nullptr, 8);
                        } else {
                            cv = (unsigned char)lex[char_start + 2];
                        }
                        break;
                }
            } else {
                cv = (unsigned char)lex[char_start + 1];
            }
        }
        const char* raw_lex = arena_.strdup(lex);
        consume();
        Node* n = make_node(NK_CHAR_LIT, ln);
        n->ival = cv;
        n->sval = raw_lex;  // store raw lexeme to detect L prefix for wide char
        return n;
    }

    // Parenthesised expression or cast or compound literal
    if (check(TokenKind::LParen)) {
        consume();  // consume (

        // Check for cast: (type-name)expr
        if (is_type_start()) {
            int save_pos = pos_;
            TypeSpec cast_ts = parse_type_name();
            if (check(TokenKind::RParen)) {
                consume();  // consume )
                // Check for compound literal: (type){ ... }
                if (check(TokenKind::LBrace)) {
                    Node* init_list = parse_init_list();
                    Node* n = make_node(NK_COMPOUND_LIT, ln);
                    n->type     = cast_ts;
                    n->left     = init_list;
                    return n;
                }
                // Regular cast
                Node* operand = parse_unary();
                Node* n = make_node(NK_CAST, ln);
                n->type = cast_ts;
                n->left = operand;
                return n;
            } else {
                // Not a type name after all — restore and parse as expression
                pos_ = save_pos;
            }
        }

        // GCC statement expression: ({ ... })
        if (check(TokenKind::LBrace)) {
            Node* block = parse_block();
            expect(TokenKind::RParen);
            Node* n = make_node(NK_STMT_EXPR, ln);
            n->body = block;
            return n;
        }

        Node* inner = parse_expr();
        expect(TokenKind::RParen);
        return parse_postfix(inner);
    }

    // Identifier / label address
    if (check(TokenKind::Identifier)) {
        const char* nm = arena_.strdup(cur().lexeme);
        consume();
        return parse_postfix(make_var(nm, ln));
    }

    // _Generic selection
    if (check(TokenKind::KwGeneric)) {
        consume();
        expect(TokenKind::LParen);
        Node* ctrl = parse_assign_expr();
        std::vector<Node*> assocs;
        while (!check(TokenKind::RParen) && !at_end()) {
            if (!match(TokenKind::Comma)) break;
            if (check(TokenKind::RParen)) break;
            Node* assoc = make_node(NK_CAST, cur().line);
            if (check(TokenKind::KwDefault)) {
                consume();
                assoc->ival = 1;  // marks as default association
            } else {
                assoc->type = parse_type_name();
            }
            expect(TokenKind::Colon);
            assoc->left = parse_assign_expr();
            assocs.push_back(assoc);
        }
        expect(TokenKind::RParen);
        Node* n = make_node(NK_GENERIC, ln);
        n->left = ctrl;
        n->children = (Node**)arena_.alloc(assocs.size() * sizeof(Node*));
        n->n_children = (int)assocs.size();
        for (int i = 0; i < (int)assocs.size(); i++) n->children[i] = assocs[i];
        return n;
    }

    // Initializer list in some contexts — just parse it
    if (check(TokenKind::LBrace)) {
        return parse_init_list();
    }

    // Fallback: return a zero literal for unrecognized primaries
    if (!at_end()) {
        // skip the token to avoid infinite loop
        consume();
    }
    return make_int_lit(0, ln);
}

// ── initializer parsing ───────────────────────────────────────────────────────

Node* Parser::parse_initializer() {
    if (check(TokenKind::LBrace)) {
        return parse_init_list();
    }
    return parse_assign_expr();
}

Node* Parser::parse_init_list() {
    int ln = cur().line;
    expect(TokenKind::LBrace);
    std::vector<Node*> items;
    while (!at_end() && !check(TokenKind::RBrace)) {
        Node* item = make_node(NK_INIT_ITEM, cur().line);

        // Designator?
        // Old GCC-style: `fieldname: value` (same as `.fieldname = value`)
        if (check(TokenKind::Identifier) &&
            peek(1).kind == TokenKind::Colon) {
            item->desig_field    = arena_.strdup(cur().lexeme);
            item->is_designated  = true;
            item->is_index_desig = false;
            consume();  // identifier
            consume();  // ':'
            item->left = parse_initializer();
            items.push_back(item);
            if (!match(TokenKind::Comma)) break;
            if (check(TokenKind::RBrace)) break;
            continue;
        }
        if (check(TokenKind::Dot)) {
            consume();
            if (check(TokenKind::Identifier)) {
                item->desig_field    = arena_.strdup(cur().lexeme);
                item->is_designated  = true;
                item->is_index_desig = false;
                consume();
            }
            // Multi-level designator: .a.b = val → synthesize nested { .b = val }
            if (check(TokenKind::Dot) || check(TokenKind::LBracket)) {
                // Recursively build nested init list for the remaining chain
                std::function<Node*()> build_nested = [&]() -> Node* {
                    int ln2 = cur().line;
                    Node* inner = make_node(NK_INIT_ITEM, ln2);
                    if (check(TokenKind::Dot)) {
                        consume();
                        if (check(TokenKind::Identifier)) {
                            inner->desig_field    = arena_.strdup(cur().lexeme);
                            inner->is_designated  = true;
                            inner->is_index_desig = false;
                            consume();
                        }
                    } else if (check(TokenKind::LBracket)) {
                        consume();
                        Node* ie = parse_assign_expr();
                        long long iv = (ie && ie->kind == NK_INT_LIT) ? ie->ival : 0;
                        expect(TokenKind::RBracket);
                        inner->is_designated  = true;
                        inner->is_index_desig = true;
                        inner->desig_val      = iv;
                    }
                    if (check(TokenKind::Dot) || check(TokenKind::LBracket)) {
                        inner->left = build_nested();
                    } else {
                        match(TokenKind::Assign);
                        inner->left = parse_initializer();
                    }
                    Node* lst2 = make_node(NK_INIT_LIST, ln2);
                    Node** ch = (Node**)arena_.alloc(sizeof(Node*));
                    ch[0] = inner;
                    lst2->children = ch;
                    lst2->n_children = 1;
                    return lst2;
                };
                item->left = build_nested();
                items.push_back(item);
                if (!match(TokenKind::Comma)) break;
                if (check(TokenKind::RBrace)) break;
                continue;
            }
            match(TokenKind::Assign);
        } else if (check(TokenKind::LBracket)) {
            consume();
            Node* idx_expr = parse_assign_expr();
            long long idx_lo = 0;
            if (idx_expr && idx_expr->kind == NK_INT_LIT) idx_lo = idx_expr->ival;
            // GCC range: [lo ... hi]
            long long idx_hi = idx_lo;
            if (check(TokenKind::Ellipsis)) {
                consume();
                Node* hi_expr = parse_assign_expr();
                if (hi_expr && hi_expr->kind == NK_INT_LIT) idx_hi = hi_expr->ival;
            }
            expect(TokenKind::RBracket);
            match(TokenKind::Assign);
            item->is_designated  = true;
            item->is_index_desig = true;
            item->desig_val      = idx_lo;
            // For ranges, store hi in right->ival
            if (idx_hi != idx_lo) {
                Node* hi_node = make_int_lit(idx_hi, cur().line);
                item->right = hi_node;
            }
        }

        item->left = parse_initializer();
        items.push_back(item);

        if (!match(TokenKind::Comma)) break;
        if (check(TokenKind::RBrace)) break;
    }
    expect(TokenKind::RBrace);

    Node* list = make_node(NK_INIT_LIST, ln);
    list->n_children = (int)items.size();
    if (list->n_children > 0) {
        list->children = arena_.alloc_array<Node*>(list->n_children);
        for (int i = 0; i < list->n_children; ++i) list->children[i] = items[i];
    }
    return list;
}

// ── statement parsing ─────────────────────────────────────────────────────────

Node* Parser::parse_block() {
    int ln = cur().line;
    expect(TokenKind::LBrace);
    std::vector<Node*> stmts;
    while (!at_end() && !check(TokenKind::RBrace)) {
        Node* s = parse_stmt();
        if (s) stmts.push_back(s);
    }
    expect(TokenKind::RBrace);
    return make_block(stmts.empty() ? nullptr : stmts.data(), (int)stmts.size(), ln);
}

Node* Parser::parse_stmt() {
    int ln = cur().line;
    skip_attributes();

    switch (cur().kind) {
        case TokenKind::LBrace:
            return parse_block();

        case TokenKind::KwReturn: {
            consume();
            Node* val = nullptr;
            if (!check(TokenKind::Semi)) {
                val = parse_expr();
            }
            match(TokenKind::Semi);
            Node* n = make_node(NK_RETURN, ln);
            n->left = val;
            return n;
        }

        case TokenKind::KwIf: {
            consume();
            expect(TokenKind::LParen);
            Node* cnd = parse_expr();
            expect(TokenKind::RParen);
            Node* then_stmt = parse_stmt();
            Node* else_stmt = nullptr;
            if (match(TokenKind::KwElse)) {
                else_stmt = parse_stmt();
            }
            Node* n = make_node(NK_IF, ln);
            n->cond  = cnd;
            n->then_ = then_stmt;
            n->else_ = else_stmt;
            return n;
        }

        case TokenKind::KwWhile: {
            consume();
            expect(TokenKind::LParen);
            Node* cnd = parse_expr();
            expect(TokenKind::RParen);
            Node* bd = parse_stmt();
            Node* n = make_node(NK_WHILE, ln);
            n->cond = cnd;
            n->body = bd;
            return n;
        }

        case TokenKind::KwDo: {
            consume();
            Node* bd = parse_stmt();
            expect(TokenKind::KwWhile);
            expect(TokenKind::LParen);
            Node* cnd = parse_expr();
            expect(TokenKind::RParen);
            match(TokenKind::Semi);
            Node* n = make_node(NK_DO_WHILE, ln);
            n->body = bd;
            n->cond = cnd;
            return n;
        }

        case TokenKind::KwFor: {
            consume();
            expect(TokenKind::LParen);
            Node* for_init = nullptr;
            if (!check(TokenKind::Semi)) {
                if (is_type_start()) {
                    for_init = parse_local_decl();
                    // for-decl init already consumed the semicolon via parse_local_decl
                    // Actually parse_local_decl doesn't consume ';' for for-init context
                    // We'll handle with a flag — just consume ; here
                    match(TokenKind::Semi);
                } else {
                    for_init = parse_expr();
                    match(TokenKind::Semi);
                }
            } else {
                consume();  // consume ;
            }
            Node* for_cond = nullptr;
            if (!check(TokenKind::Semi)) {
                for_cond = parse_expr();
            }
            match(TokenKind::Semi);
            Node* for_update = nullptr;
            if (!check(TokenKind::RParen)) {
                for_update = parse_expr();
            }
            expect(TokenKind::RParen);
            Node* bd = parse_stmt();
            Node* n = make_node(NK_FOR, ln);
            n->init   = for_init;
            n->cond   = for_cond;
            n->update = for_update;
            n->body   = bd;
            return n;
        }

        case TokenKind::KwSwitch: {
            consume();
            expect(TokenKind::LParen);
            Node* cnd = parse_expr();
            expect(TokenKind::RParen);
            Node* bd = parse_stmt();
            Node* n = make_node(NK_SWITCH, ln);
            n->cond = cnd;
            n->body = bd;
            return n;
        }

        case TokenKind::KwCase: {
            consume();
            Node* val_lo = parse_assign_expr();
            Node* val_hi = nullptr;
            if (check(TokenKind::Ellipsis)) {
                consume();
                val_hi = parse_assign_expr();
            }
            expect(TokenKind::Colon);
            // Accumulate case body as the next statement
            Node* body_stmt = nullptr;
            if (!check(TokenKind::RBrace) && !check(TokenKind::KwCase) &&
                !check(TokenKind::KwDefault)) {
                body_stmt = parse_stmt();
            }
            if (val_hi) {
                Node* n = make_node(NK_CASE_RANGE, ln);
                n->left  = val_lo;
                n->right = val_hi;
                n->body  = body_stmt;
                return n;
            }
            Node* n = make_node(NK_CASE, ln);
            n->left = val_lo;
            n->body = body_stmt;
            return n;
        }

        case TokenKind::KwDefault: {
            consume();
            expect(TokenKind::Colon);
            Node* body_stmt = nullptr;
            if (!check(TokenKind::RBrace) && !check(TokenKind::KwCase) &&
                !check(TokenKind::KwDefault)) {
                body_stmt = parse_stmt();
            }
            Node* n = make_node(NK_DEFAULT, ln);
            n->body = body_stmt;
            return n;
        }

        case TokenKind::KwBreak:
            consume(); match(TokenKind::Semi);
            return make_node(NK_BREAK, ln);

        case TokenKind::KwContinue:
            consume(); match(TokenKind::Semi);
            return make_node(NK_CONTINUE, ln);

        case TokenKind::KwGoto: {
            consume();
            const char* lbl = nullptr;
            if (check(TokenKind::Identifier)) {
                lbl = arena_.strdup(cur().lexeme);
                consume();
            } else if (check(TokenKind::Star)) {
                // computed goto: goto *expr;
                consume();
                Node* target = parse_unary();
                match(TokenKind::Semi);
                Node* n = make_node(NK_GOTO, ln);
                n->name = "__computed__";
                n->left = target;
                return n;
            }
            match(TokenKind::Semi);
            Node* n = make_node(NK_GOTO, ln);
            n->name = lbl;
            return n;
        }

        case TokenKind::KwAsm: {
            // asm volatile ("..." : outputs : inputs : clobbers) — skip entirely
            consume();
            // Skip optional qualifiers: volatile, goto, inline
            while (check(TokenKind::KwVolatile) || check(TokenKind::KwInline) ||
                   check(TokenKind::KwGoto))
                consume();
            if (check(TokenKind::LParen)) skip_paren_group();
            match(TokenKind::Semi);
            return make_node(NK_EMPTY, ln);
        }

        case TokenKind::Semi:
            consume();
            return make_node(NK_EMPTY, ln);

        default:
            break;
    }

    // Identifier followed by colon → label
    if (check(TokenKind::Identifier) && check2(TokenKind::Colon)) {
        const char* lbl = arena_.strdup(cur().lexeme);
        consume(); consume();  // consume name and :
        Node* body_stmt = nullptr;
        if (!check(TokenKind::RBrace)) {
            body_stmt = parse_stmt();
        }
        Node* n = make_node(NK_LABEL, ln);
        n->name = lbl;
        n->body = body_stmt;
        return n;
    }

    // Local declaration?
    if (is_type_start()) {
        return parse_local_decl();
    }

    // Expression statement
    Node* expr = parse_expr();
    match(TokenKind::Semi);
    Node* n = make_node(NK_EXPR_STMT, ln);
    n->left = expr;
    return n;
}

// ── local declaration parsing ─────────────────────────────────────────────────

Node* Parser::parse_local_decl() {
    int ln = cur().line;

    bool is_static  = false;
    bool is_extern  = false;
    bool is_typedef = false;

    // Consume storage class specifiers before the type
    while (true) {
        if (match(TokenKind::KwStatic))  { is_static = true; }
        else if (match(TokenKind::KwExtern))  { is_extern = true; }
        else if (match(TokenKind::KwTypedef)) { is_typedef = true; }
        else if (match(TokenKind::KwRegister)) {}
        else if (match(TokenKind::KwInline))   {}
        else if (match(TokenKind::KwExtension)) {}
        else break;
    }

    TypeSpec base_ts = parse_base_type();
    skip_attributes();

    if (is_typedef) {
        // Simple typedef: typedef base_type name[...];
        // Also handles typedef base_type (*fn_name)(...);
        const char* tdname = nullptr;
        TypeSpec ts_copy = base_ts;
        parse_declarator(ts_copy, &tdname);
        if (tdname) {
            typedefs_.insert(tdname);
            typedef_types_[tdname] = ts_copy;
        }
        // multiple declarators in typedef
        while (match(TokenKind::Comma)) {
            TypeSpec ts2 = base_ts;
            const char* tdn2 = nullptr;
            parse_declarator(ts2, &tdn2);
            if (tdn2) {
                typedefs_.insert(tdn2);
                typedef_types_[tdn2] = ts2;
            }
        }
        match(TokenKind::Semi);
        return make_node(NK_EMPTY, ln);
    }

    // Collect all declarators for this declaration
    std::vector<Node*> decls;
    do {
        TypeSpec ts = base_ts;
        // Preserve ptr_level from typedef resolution (e.g. typedef void (*fptr)(); fptr arr[3])
        // only reset array suffix info since declarators add their own
        ts.array_size = -1;
        ts.array_rank = 0;
        for (int i = 0; i < 8; ++i) ts.array_dims[i] = -1;
        ts.is_ptr_to_array = false;
        ts.array_size_expr = nullptr;
        const char* vname = nullptr;
        parse_declarator(ts, &vname);
        skip_attributes();
        skip_asm();
        skip_attributes();

        if (!vname) {
            // Abstract declarator in statement position — skip
            match(TokenKind::Semi);
            return make_node(NK_EMPTY, ln);
        }

        Node* init_node = nullptr;
        if (match(TokenKind::Assign)) {
            init_node = parse_initializer();
        }

        Node* d = make_node(NK_DECL, ln);
        d->type      = ts;
        d->name      = vname;
        d->init      = init_node;
        d->is_static = is_static;
        d->is_extern = is_extern;
        decls.push_back(d);
    } while (match(TokenKind::Comma));

    match(TokenKind::Semi);

    if (decls.size() == 1) return decls[0];

    // Multiple declarators: wrap in a block
    Node* blk = make_node(NK_BLOCK, ln);
    blk->n_children = (int)decls.size();
    blk->children   = arena_.alloc_array<Node*>(blk->n_children);
    for (int i = 0; i < blk->n_children; ++i) blk->children[i] = decls[i];
    return blk;
}

// ── top-level parsing ─────────────────────────────────────────────────────────

Node* Parser::parse_top_level() {
    int ln = cur().line;
    if (at_end()) return nullptr;

    skip_attributes();
    if (at_end()) return nullptr;

    // Handle top-level asm
    if (check(TokenKind::KwAsm)) {
        consume();
        if (check(TokenKind::LParen)) skip_paren_group();
        match(TokenKind::Semi);
        return make_node(NK_EMPTY, ln);
    }

    // _Static_assert
    if (check(TokenKind::KwStaticAssert)) {
        consume();
        skip_paren_group();
        match(TokenKind::Semi);
        return make_node(NK_EMPTY, ln);
    }

    bool is_static   = false;
    bool is_extern   = false;
    bool is_typedef  = false;
    bool is_inline   = false;

    // Consume storage class specifiers
    while (true) {
        if (match(TokenKind::KwStatic))    { is_static  = true; }
        else if (match(TokenKind::KwExtern))   { is_extern  = true; }
        else if (match(TokenKind::KwTypedef))  { is_typedef = true; }
        else if (match(TokenKind::KwInline))   { is_inline  = true; }
        else if (match(TokenKind::KwExtension)) {}
        else if (match(TokenKind::KwNoreturn))  {}
        else break;
    }
    skip_attributes();

    TypeSpec base_ts{};
    if (!is_type_start()) {
        // K&R/old-style implicit-int function at file scope:
        // e.g. `main() { ... }` or `static foo() { ... }`.
        // Treat as `int name(...)` instead of discarding the whole definition.
        bool maybe_implicit_int_fn =
            check(TokenKind::Identifier) && check2(TokenKind::LParen);
        if (!maybe_implicit_int_fn) {
            // Skip to semicolon
            skip_until(TokenKind::Semi);
            return make_node(NK_EMPTY, ln);
        }
        base_ts.array_size = -1;
        base_ts.array_rank = 0;
        for (int i = 0; i < 8; ++i) base_ts.array_dims[i] = -1;
        base_ts.base = TB_INT;
    } else {
        base_ts = parse_base_type();
        skip_attributes();
    }

    if (is_typedef) {
        // Local typedef
        const char* tdname = nullptr;
        TypeSpec ts_copy = base_ts;
        parse_declarator(ts_copy, &tdname);
        if (tdname) {
            typedefs_.insert(tdname);
            typedef_types_[tdname] = ts_copy;
        }
        while (match(TokenKind::Comma)) {
            TypeSpec ts2 = base_ts;
            const char* tdn2 = nullptr;
            parse_declarator(ts2, &tdn2);
            if (tdn2) {
                typedefs_.insert(tdn2);
                typedef_types_[tdn2] = ts2;
            }
        }
        match(TokenKind::Semi);
        return make_node(NK_EMPTY, ln);
    }

    // Peek to disambiguate: struct/union/enum declaration only (no declarator)
    if (check(TokenKind::Semi) &&
        (base_ts.base == TB_STRUCT || base_ts.base == TB_UNION ||
         base_ts.base == TB_ENUM)) {
        consume();  // consume ;
        return make_node(NK_EMPTY, ln);
    }

    // Parse declarator (name + pointer stars + maybe function params)
    TypeSpec ts = base_ts;
    // Preserve ptr_level from typedef resolution (e.g. typedef void (*fptr)(); fptr arr[3])
    ts.array_size = -1;
    ts.array_rank = 0;
    for (int i = 0; i < 8; ++i) ts.array_dims[i] = -1;
    ts.is_ptr_to_array = false;
    ts.array_size_expr = nullptr;
    const char* decl_name = nullptr;
    bool is_fptr_global = false;

    // Check for function-pointer global: type (*name)(...);
    // Also handles function-returning-fptr: int (* f1(params))(ret_params);
    bool fn_returning_fptr = false;
    std::vector<Node*> fptr_fn_params;
    bool fptr_fn_variadic = false;
    if (check(TokenKind::LParen) && check2(TokenKind::Star)) {
        consume();  // (
        consume();  // *
        ts.ptr_level++;
        while (check(TokenKind::Star)) { consume(); ts.ptr_level++; }
        if (check(TokenKind::Identifier)) {
            decl_name = arena_.strdup(cur().lexeme);
            consume();
        }
        // Handle array-of-function-pointer: void (*func2[6])(void)
        while (check(TokenKind::LBracket)) {
            consume();  // [
            long long dim = -2;
            if (!check(TokenKind::RBracket)) {
                Node* sz = parse_expr();
                if (sz) { long long v; if (eval_const_int(sz, &v)) dim = v; }
            }
            expect(TokenKind::RBracket);
            if (ts.array_rank < 8) ts.array_dims[ts.array_rank++] = dim;
            if (ts.array_rank == 1) ts.array_size = ts.array_dims[0];
        }
        // Detect function-returning-fptr: (* f1(params)) — has inner params before ')'
        if (check(TokenKind::LParen)) {
            fn_returning_fptr = true;
            // Parse the function's own parameter list properly
            consume();  // consume '('
            if (!check(TokenKind::RParen)) {
                while (!at_end()) {
                    if (check(TokenKind::Ellipsis)) {
                        fptr_fn_variadic = true; consume(); break;
                    }
                    if (check(TokenKind::RParen)) break;
                    if (!is_type_start()) { consume(); if (match(TokenKind::Comma)) continue; break; }
                    Node* p = parse_param();
                    if (p) fptr_fn_params.push_back(p);
                    if (!match(TokenKind::Comma)) break;
                }
            }
            expect(TokenKind::RParen);  // close function's own params
        }
        expect(TokenKind::RParen);  // close (*name...)
        if (check(TokenKind::LParen)) skip_paren_group();  // skip return-fptr params
        if (!fn_returning_fptr) is_fptr_global = true;
    } else {
        parse_declarator(ts, &decl_name);
    }

    skip_attributes();
    skip_asm();
    skip_attributes();

    if (!decl_name) {
        // No name: just a type declaration; skip to ;
        match(TokenKind::Semi);
        return make_node(NK_EMPTY, ln);
    }

    // Handle function-returning-fptr: int (* f1(a, b))(c, d) { body }
    // Params were already parsed into fptr_fn_params; now look for { body }.
    if (fn_returning_fptr && decl_name) {
        skip_attributes(); skip_asm(); skip_attributes();
        if (check(TokenKind::LBrace)) {
            Node* body = parse_block();
            Node* fn = make_node(NK_FUNCTION, ln);
            fn->type      = ts;
            fn->name      = decl_name;
            fn->variadic  = fptr_fn_variadic;
            fn->is_static = is_static;
            fn->is_extern = is_extern;
            fn->is_inline = is_inline;
            fn->body      = body;
            fn->n_params  = (int)fptr_fn_params.size();
            if (fn->n_params > 0) {
                fn->params = arena_.alloc_array<Node*>(fn->n_params);
                for (int i = 0; i < fn->n_params; ++i) fn->params[i] = fptr_fn_params[i];
            }
            return fn;
        }
        // Function declaration (no body): consume semicolon and return
        match(TokenKind::Semi);
        Node* fn = make_node(NK_FUNCTION, ln);
        fn->type      = ts;
        fn->name      = decl_name;
        fn->variadic  = fptr_fn_variadic;
        fn->is_static = is_static;
        fn->is_extern = is_extern;
        fn->is_inline = is_inline;
        fn->body      = nullptr;
        fn->n_params  = (int)fptr_fn_params.size();
        if (fn->n_params > 0) {
            fn->params = arena_.alloc_array<Node*>(fn->n_params);
            for (int i = 0; i < fn->n_params; ++i) fn->params[i] = fptr_fn_params[i];
        }
        return fn;
    }

    // Check for function definition: name followed by ( params ) {
    // We need to figure out if we're looking at a function or a variable.
    // The key distinction: if we have ( params ) { body }, it's a function.
    // We might have already consumed the function params in parse_declarator.
    // Re-check: if the next token is { or if the type's base indicates function.

    // Actually, after parse_declarator, if we encounter '(' next, it means
    // the declarator name was just the name and the function params are next.
    // OR: the declarator already handled the suffix.
    // Let's handle it here.

    if (!is_fptr_global && check(TokenKind::LParen)) {
        // Function declaration or definition
        consume();  // consume (
        std::vector<Node*> params;
        std::vector<const char*> knr_param_names;
        bool variadic = false;
        if (!check(TokenKind::RParen)) {
            while (!at_end()) {
                if (check(TokenKind::Ellipsis)) {
                    variadic = true;
                    consume();
                    break;
                }
                if (check(TokenKind::RParen)) break;
                // K&R-style: just identifiers
                if (!is_type_start() && check(TokenKind::Identifier)) {
                    knr_param_names.push_back(arena_.strdup(cur().lexeme));
                    consume();
                    if (match(TokenKind::Comma)) continue;
                    break;
                }
                if (!is_type_start()) break;
                Node* p = parse_param();
                if (p) params.push_back(p);
                if (check(TokenKind::Ellipsis)) {
                    variadic = true;
                    consume();
                    break;
                }
                if (!match(TokenKind::Comma)) break;
            }
        }
        expect(TokenKind::RParen);
        skip_attributes();
        skip_asm();
        skip_attributes();

        // K&R style parameter declarations before body:
        //   f(a, b) int a; short *b; { ... }
        // Bind declared types back to identifier list order.
        if (!knr_param_names.empty()) {
            std::unordered_map<std::string, Node*> knr_param_decl_map;
            while (is_type_start() && !check(TokenKind::LBrace)) {
                TypeSpec knr_base = parse_base_type();
                do {
                    TypeSpec knr_ts = knr_base;
                    knr_ts.array_size = -1;
                    knr_ts.array_rank = 0;
                    for (int i = 0; i < 8; ++i) knr_ts.array_dims[i] = -1;
                    knr_ts.is_ptr_to_array = false;
                    knr_ts.array_size_expr = nullptr;
                    const char* knr_name = nullptr;
                    parse_declarator(knr_ts, &knr_name);
                    if (!knr_name) continue;
                    bool listed = false;
                    for (const char* pnm : knr_param_names) {
                        if (pnm && strcmp(pnm, knr_name) == 0) { listed = true; break; }
                    }
                    if (!listed) continue;
                    Node* p = make_node(NK_DECL, ln);
                    p->name = knr_name;
                    p->type = knr_ts;
                    knr_param_decl_map[knr_name] = p;
                } while (match(TokenKind::Comma));
                match(TokenKind::Semi);
            }
            params.clear();
            for (const char* pnm : knr_param_names) {
                auto it = knr_param_decl_map.find(pnm ? pnm : "");
                if (it != knr_param_decl_map.end()) {
                    params.push_back(it->second);
                    continue;
                }
                // Old-style fallback: undeclared parameter defaults to int.
                Node* p = make_node(NK_DECL, ln);
                p->name = pnm;
                TypeSpec pts{};
                pts.array_size = -1;
                pts.array_rank = 0;
                for (int i = 0; i < 8; ++i) pts.array_dims[i] = -1;
                pts.base = TB_INT;
                p->type = pts;
                params.push_back(p);
            }
        } else {
            // Non-K&R: ignore declaration-only lines before body.
            while (is_type_start() && !check(TokenKind::LBrace)) {
                skip_until(TokenKind::Semi);
            }
        }

        if (check(TokenKind::LBrace)) {
            // Function definition
            Node* body = parse_block();
            Node* fn = make_node(NK_FUNCTION, ln);
            fn->type      = ts;
            fn->name      = decl_name;
            fn->variadic  = variadic;
            fn->is_static = is_static;
            fn->is_extern = is_extern;
            fn->is_inline = is_inline;
            fn->body      = body;
            fn->n_params  = (int)params.size();
            if (fn->n_params > 0) {
                fn->params = arena_.alloc_array<Node*>(fn->n_params);
                for (int i = 0; i < fn->n_params; ++i) fn->params[i] = params[i];
            }
            return fn;
        }

        // Function declaration only
        match(TokenKind::Semi);
        Node* fn = make_node(NK_FUNCTION, ln);
        fn->type      = ts;
        fn->name      = decl_name;
        fn->variadic  = variadic;
        fn->is_static = is_static;
        fn->is_extern = is_extern;
        fn->is_inline = is_inline;
        fn->body      = nullptr;  // declaration only
        fn->n_params  = (int)params.size();
        if (fn->n_params > 0) {
            fn->params = arena_.alloc_array<Node*>(fn->n_params);
            for (int i = 0; i < fn->n_params; ++i) fn->params[i] = params[i];
        }
        return fn;
    }

    // Global variable (possibly with initializer and multiple declarators)
    std::vector<Node*> gvars;

    auto make_gvar = [&](TypeSpec gts, const char* gname, Node* ginit) {
        Node* gv = make_node(NK_GLOBAL_VAR, ln);
        gv->type      = gts;
        gv->name      = gname;
        gv->init      = ginit;
        gv->is_static = is_static;
        gv->is_extern = is_extern;
        return gv;
    };

    Node* first_init = nullptr;
    if (match(TokenKind::Assign)) {
        first_init = parse_initializer();
    }
    gvars.push_back(make_gvar(ts, decl_name, first_init));

    while (match(TokenKind::Comma)) {
        TypeSpec ts2 = base_ts;
        ts2.ptr_level = 0;
        ts2.array_size = -1;
        ts2.array_rank = 0;
        for (int i = 0; i < 8; ++i) ts2.array_dims[i] = -1;
        ts2.is_ptr_to_array = false;
        ts2.array_size_expr = nullptr;
        const char* n2 = nullptr;
        parse_declarator(ts2, &n2);
        skip_attributes();
        skip_asm();
        Node* init2 = nullptr;
        if (match(TokenKind::Assign)) init2 = parse_initializer();
        if (n2) gvars.push_back(make_gvar(ts2, n2, init2));
    }
    match(TokenKind::Semi);

    if (gvars.size() == 1) return gvars[0];

    // Multiple global vars — wrap in a block
    Node* blk = make_node(NK_BLOCK, ln);
    blk->n_children = (int)gvars.size();
    blk->children   = arena_.alloc_array<Node*>(blk->n_children);
    for (int i = 0; i < blk->n_children; ++i) blk->children[i] = gvars[i];
    return blk;
}

// ── parse() entry point ───────────────────────────────────────────────────────

Node* Parser::parse() {
    std::vector<Node*> items;
    had_error_ = false;

    while (!at_end()) {
        Node* item = nullptr;
        try {
            item = parse_top_level();
        } catch (const std::exception& e) {
            // Parse error: print warning and try to recover to next semicolon / }
            fprintf(stderr, "parse error: %s (skipping to next ';' or '}')\n", e.what());
            had_error_ = true;
            while (!at_end() && !check(TokenKind::Semi) && !check(TokenKind::RBrace)) {
                consume();
            }
            if (!at_end()) consume();
            continue;
        }
        if (item) items.push_back(item);
    }

    // Prepend collected struct/enum definitions (so IR builder sees them first)
    std::vector<Node*> all;
    for (Node* sd : struct_defs_) all.push_back(sd);
    for (Node* it : items)        all.push_back(it);

    Node* prog = make_node(NK_PROGRAM, 0);
    prog->n_children = (int)all.size();
    if (prog->n_children > 0) {
        prog->children = arena_.alloc_array<Node*>(prog->n_children);
        for (int i = 0; i < prog->n_children; ++i) prog->children[i] = all[i];
    }
    return prog;
}

// ── node constructors ─────────────────────────────────────────────────────────

Node* Parser::make_node(NodeKind k, int line) {
    Node* n = arena_.alloc_array<Node>(1);
    n->kind = k;
    n->line = line;
    n->type.array_size = -1;
    n->type.array_rank = 0;
    for (int i = 0; i < 8; ++i) n->type.array_dims[i] = -1;
    n->type.is_ptr_to_array = false;
    return n;
}

Node* Parser::make_int_lit(long long v, int line) {
    Node* n = make_node(NK_INT_LIT, line);
    n->ival = v;
    return n;
}

Node* Parser::make_float_lit(double v, const char* raw, int line) {
    Node* n = make_node(NK_FLOAT_LIT, line);
    n->fval = v;
    n->sval = raw;
    return n;
}

Node* Parser::make_str_lit(const char* raw, int line) {
    Node* n = make_node(NK_STR_LIT, line);
    n->sval = raw;
    return n;
}

Node* Parser::make_var(const char* name, int line) {
    Node* n = make_node(NK_VAR, line);
    n->name = name;
    return n;
}

Node* Parser::make_binop(const char* op, Node* l, Node* r, int line) {
    Node* n = make_node(NK_BINOP, line);
    n->op    = arena_.strdup(op);
    n->left  = l;
    n->right = r;
    return n;
}

Node* Parser::make_unary(const char* op, Node* operand, int line) {
    Node* n = make_node(NK_UNARY, line);
    n->op   = arena_.strdup(op);
    n->left = operand;
    return n;
}

Node* Parser::make_assign(const char* op, Node* lhs, Node* rhs, int line) {
    Node* n = make_node(NK_ASSIGN, line);
    n->op    = arena_.strdup(op);
    n->left  = lhs;
    n->right = rhs;
    return n;
}

Node* Parser::make_block(Node** stmts, int n_stmts, int line) {
    Node* n = make_node(NK_BLOCK, line);
    n->n_children = n_stmts;
    if (n_stmts > 0 && stmts) {
        n->children = arena_.alloc_array<Node*>(n_stmts);
        for (int i = 0; i < n_stmts; ++i) n->children[i] = stmts[i];
    }
    return n;
}

// ── AST dump ─────────────────────────────────────────────────────────────────

const char* node_kind_name(NodeKind k) {
    switch (k) {
        case NK_INT_LIT:      return "IntLit";
        case NK_FLOAT_LIT:    return "FloatLit";
        case NK_STR_LIT:      return "StrLit";
        case NK_CHAR_LIT:     return "CharLit";
        case NK_VAR:          return "Var";
        case NK_BINOP:        return "BinOp";
        case NK_UNARY:        return "Unary";
        case NK_POSTFIX:      return "Postfix";
        case NK_ADDR:         return "Addr";
        case NK_DEREF:        return "Deref";
        case NK_ASSIGN:       return "Assign";
        case NK_COMPOUND_ASSIGN: return "CompoundAssign";
        case NK_CALL:         return "Call";
        case NK_INDEX:        return "Index";
        case NK_MEMBER:       return "Member";
        case NK_CAST:         return "Cast";
        case NK_TERNARY:      return "Ternary";
        case NK_SIZEOF_EXPR:  return "SizeofExpr";
        case NK_SIZEOF_TYPE:  return "SizeofType";
        case NK_ALIGNOF_TYPE: return "AlignofType";
        case NK_COMMA_EXPR:   return "CommaExpr";
        case NK_STMT_EXPR:    return "StmtExpr";
        case NK_COMPOUND_LIT: return "CompoundLit";
        case NK_VA_ARG:       return "VaArg";
        case NK_GENERIC:      return "Generic";
        case NK_INIT_LIST:    return "InitList";
        case NK_INIT_ITEM:    return "InitItem";
        case NK_BLOCK:        return "Block";
        case NK_EXPR_STMT:    return "ExprStmt";
        case NK_RETURN:       return "Return";
        case NK_IF:           return "If";
        case NK_WHILE:        return "While";
        case NK_FOR:          return "For";
        case NK_DO_WHILE:     return "DoWhile";
        case NK_SWITCH:       return "Switch";
        case NK_CASE:         return "Case";
        case NK_CASE_RANGE:   return "CaseRange";
        case NK_DEFAULT:      return "Default";
        case NK_BREAK:        return "Break";
        case NK_CONTINUE:     return "Continue";
        case NK_GOTO:         return "Goto";
        case NK_LABEL:        return "Label";
        case NK_EMPTY:        return "Empty";
        case NK_DECL:         return "Decl";
        case NK_FUNCTION:     return "Function";
        case NK_GLOBAL_VAR:   return "GlobalVar";
        case NK_STRUCT_DEF:   return "StructDef";
        case NK_ENUM_DEF:     return "EnumDef";
        case NK_PROGRAM:      return "Program";
        default:              return "Unknown";
    }
}

static void indent_print(int depth) {
    for (int i = 0; i < depth * 2; ++i) putchar(' ');
}

void ast_dump(const Node* n, int indent) {
    if (!n) { indent_print(indent); printf("<null>\n"); return; }
    indent_print(indent);
    printf("%s", node_kind_name(n->kind));
    switch (n->kind) {
        case NK_INT_LIT:   printf("(%lld)", n->ival); break;
        case NK_FLOAT_LIT: printf("(%g)", n->fval); break;
        case NK_CHAR_LIT:  printf("(%lld)", n->ival); break;
        case NK_STR_LIT:   printf("(...)"); break;
        case NK_VAR:       printf("(%s)", n->name ? n->name : "?"); break;
        case NK_BINOP:     printf("(%s)", n->op ? n->op : "?"); break;
        case NK_UNARY:     printf("(%s)", n->op ? n->op : "?"); break;
        case NK_POSTFIX:   printf("(%s)", n->op ? n->op : "?"); break;
        case NK_ASSIGN:    printf("(%s)", n->op ? n->op : "?"); break;
        case NK_MEMBER:    printf("(%s%s)", n->is_arrow ? "->" : ".", n->name ? n->name : "?"); break;
        case NK_FUNCTION:  printf("(%s)", n->name ? n->name : "?"); break;
        case NK_DECL:      printf("(%s)", n->name ? n->name : "?"); break;
        case NK_GLOBAL_VAR: printf("(%s)", n->name ? n->name : "?"); break;
        case NK_STRUCT_DEF: printf("(%s%s)", n->is_union ? "union " : "struct ", n->name ? n->name : "?"); break;
        case NK_ENUM_DEF:  printf("(enum %s)", n->name ? n->name : "?"); break;
        case NK_GOTO:      printf("(%s)", n->name ? n->name : "?"); break;
        case NK_LABEL:     printf("(%s)", n->name ? n->name : "?"); break;
        default: break;
    }
    printf("\n");

    // Recurse into children
    if (n->left)   ast_dump(n->left,   indent + 1);
    if (n->right)  ast_dump(n->right,  indent + 1);
    if (n->cond)   ast_dump(n->cond,   indent + 1);
    if (n->then_)  ast_dump(n->then_,  indent + 1);
    if (n->else_)  ast_dump(n->else_,  indent + 1);
    if (n->init)   ast_dump(n->init,   indent + 1);
    if (n->update) ast_dump(n->update, indent + 1);
    if (n->body)   ast_dump(n->body,   indent + 1);

    for (int i = 0; i < n->n_params; ++i) ast_dump(n->params[i], indent + 1);
    for (int i = 0; i < n->n_children; ++i) ast_dump(n->children[i], indent + 1);
    for (int i = 0; i < n->n_fields; ++i) ast_dump(n->fields[i], indent + 1);
}

}  // namespace tinyc2ll::frontend_cxx
