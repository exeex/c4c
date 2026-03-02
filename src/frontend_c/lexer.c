/* lexer.c – tokeniser for the C frontend
 *
 * Mirrors the behaviour of src/frontend/lexer.py:
 *   - Skips whitespace, // and block comments.
 *   - Handles string/char literals with all C escape sequences.
 *   - Handles wide/unicode prefixes: L"…", u"…", U"…", u8"…", L'…'.
 *   - Skips GCC extension keywords (__attribute__, __asm__, …).
 *   - Remaps __signed__, __unsigned__, __builtin_va_list, etc.
 *   - Returns numeric literals as decimal text (hex/bin/float converted).
 *   - Returns char literals as their numeric value.
 *   - Adjacent string literals are concatenated.
 *   - Lines starting with '#' (preprocessor directives) are skipped.
 */

#include "common.h"
#include "lexer.h"

/* ================================================================
 * Lexer state
 * ================================================================ */
typedef struct {
    const char *src;
    size_t      pos;
    size_t      len;
    int         line;
    int         col;
} Lx;

static inline int lx_cur(const Lx *L) {
    return L->pos < L->len ? (unsigned char)L->src[L->pos] : -1;
}
static inline int lx_peek(const Lx *L, int n) {
    size_t j = L->pos + n;
    return j < L->len ? (unsigned char)L->src[j] : -1;
}
static inline int lx_adv(Lx *L) {
    if (L->pos >= L->len) return -1;
    int c = (unsigned char)L->src[L->pos++];
    if (c == '\n') { L->line++; L->col = 1; }
    else           { L->col++; }
    return c;
}

/* ================================================================
 * TokenVec helpers
 * ================================================================ */
static void tv_init(TokenVec *tv) { tv->tokens = NULL; tv->count = tv->cap = 0; }

static void tv_push(TokenVec *tv, Token tok) {
    if (tv->count >= tv->cap) {
        tv->cap = tv->cap ? tv->cap * 2 : 256;
        tv->tokens = (Token *)xrealloc(tv->tokens, tv->cap * sizeof(Token));
    }
    tv->tokens[tv->count++] = tok;
}

static Token make_tok(TokenKind kind, char *text, size_t text_len, int line, int col) {
    Token t;
    t.kind = kind;
    t.text = text;
    t.text_len = text_len;
    t.line = line;
    t.col  = col;
    return t;
}
/* Convenience: token from a NUL-terminated string (no embedded NULs) */
static Token make_tok_s(TokenKind kind, char *text, int line, int col) {
    return make_tok(kind, text, strlen(text), line, col);
}

void tokenvec_free(TokenVec *tv) {
    for (size_t i = 0; i < tv->count; i++) free(tv->tokens[i].text);
    free(tv->tokens);
    tv->tokens = NULL; tv->count = tv->cap = 0;
}

/* ================================================================
 * Keyword table
 * ================================================================ */
typedef struct { const char *word; TokenKind kind; } KwEntry;

static const KwEntry kw_table[] = {
    {"int",      TK_INT},      {"char",     TK_CHAR},
    {"void",     TK_VOID},     {"extern",   TK_EXTERN},
    {"return",   TK_RETURN},   {"if",       TK_IF},
    {"else",     TK_ELSE},     {"while",    TK_WHILE},
    {"for",      TK_FOR},      {"do",       TK_DO},
    {"switch",   TK_SWITCH},   {"case",     TK_CASE},
    {"default",  TK_DEFAULT},  {"break",    TK_BREAK},
    {"continue", TK_CONTINUE}, {"goto",     TK_GOTO},
    {"struct",   TK_STRUCT},   {"typedef",  TK_TYPEDEF},
    {"static",   TK_STATIC},   {"sizeof",   TK_SIZEOF},
    {"long",     TK_LONG},     {"short",    TK_SHORT},
    {"unsigned", TK_UNSIGNED}, {"signed",   TK_SIGNED},
    {"double",   TK_DOUBLE},   {"float",    TK_FLOAT},
    {"const",    TK_CONST},    {"volatile", TK_VOLATILE},
    {"restrict", TK_RESTRICT}, {"inline",   TK_INLINE},
    {"enum",     TK_ENUM},     {"union",    TK_UNION},
    {"register", TK_REGISTER}, {"_Bool",    TK_INT},
    {NULL, 0}
};

static TokenKind lookup_keyword(const char *word) {
    for (int i = 0; kw_table[i].word; i++)
        if (strcmp(word, kw_table[i].word) == 0) return kw_table[i].kind;
    return TK_ID;
}

/* ================================================================
 * GCC extension handling tables
 * ================================================================ */
static const char *skip_names[] = {
    "__attribute__", "__asm__", "__asm", "__declspec",
    "__cdecl", "__stdcall", "__fastcall", "__thiscall",
    "__volatile__", "__const__", "__restrict__",
    "__extension__", "__inline__", "__inline",
    "__forceinline", "__typeof__",
    NULL
};

typedef struct { const char *from; const char *to; TokenKind to_kind; } RemapEntry;

static const RemapEntry remap_table[] = {
    {"__restrict",       NULL,        TK_EOF},       /* drop */
    {"__volatile",       NULL,        TK_EOF},       /* drop */
    {"__const",          NULL,        TK_EOF},       /* drop */
    {"__signed__",       "signed",    TK_SIGNED},
    {"__unsigned__",     "unsigned",  TK_UNSIGNED},
    {"__builtin_va_list","__va_list", TK_ID},
    {NULL, NULL, 0}
};

static bool is_skip_name(const char *w) {
    for (int i = 0; skip_names[i]; i++)
        if (strcmp(w, skip_names[i]) == 0) return true;
    return false;
}
static const RemapEntry *find_remap(const char *w) {
    for (int i = 0; remap_table[i].from; i++)
        if (strcmp(w, remap_table[i].from) == 0) return &remap_table[i];
    return NULL;
}

/* ================================================================
 * Balanced paren group skip
 * ================================================================ */
static void skip_paren_group(Lx *L) {
    if (lx_cur(L) != '(') return;
    int depth = 0;
    while (lx_cur(L) != -1) {
        int c = lx_adv(L);
        if      (c == '(') depth++;
        else if (c == ')') { if (--depth == 0) return; }
    }
}

/* ================================================================
 * Character classification
 * ================================================================ */
static inline bool is_id_start(int c) { return isalpha(c) || c == '_' || c == '$'; }
static inline bool is_id_cont(int c)  { return isalnum(c) || c == '_' || c == '$'; }
static inline bool is_hex_digit(int c) {
    return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}
static inline int hex_val(int c) {
    if (isdigit(c)) return c - '0';
    return tolower(c) - 'a' + 10;
}

/* ================================================================
 * Escape sequence decoder
 *
 * Called with the character AFTER the backslash already consumed.
 * Appends decoded byte(s) to *buf if buf != NULL.
 * Returns the decoded integer value (codepoint for \u/\U).
 * ================================================================ */
static uint32_t scan_escape(Lx *L, StrBuf *buf) {
    int esc = lx_adv(L);
    if (esc == -1) return 0;
    switch (esc) {
        case 'n':  if (buf) sb_push(buf, '\n'); return '\n';
        case 't':  if (buf) sb_push(buf, '\t'); return '\t';
        case 'r':  if (buf) sb_push(buf, '\r'); return '\r';
        case '\\': if (buf) sb_push(buf, '\\'); return '\\';
        case '"':  if (buf) sb_push(buf, '"');  return '"';
        case '\'': if (buf) sb_push(buf, '\''); return '\'';
        case 'a':  if (buf) sb_push(buf, '\a'); return '\a';
        case 'b':  if (buf) sb_push(buf, '\b'); return '\b';
        case 'f':  if (buf) sb_push(buf, '\f'); return '\f';
        case 'v':  if (buf) sb_push(buf, '\v'); return '\v';
        case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7': {
            /* octal escape */
            int val = esc - '0';
            for (int i = 0; i < 2; i++) {
                int c = lx_cur(L);
                if (c >= '0' && c <= '7') { val = val * 8 + (c - '0'); lx_adv(L); }
                else break;
            }
            if (buf) sb_push(buf, val & 0xFF);
            return (uint32_t)(val & 0xFF);
        }
        case 'x': {
            /* hex escape */
            int val = 0;
            while (is_hex_digit(lx_cur(L)))
                val = val * 16 + hex_val(lx_adv(L));
            val &= 0xFF;
            if (buf) sb_push(buf, val);
            return (uint32_t)val;
        }
        case 'u': case 'U': {
            /* Unicode: \uXXXX or \UXXXXXXXX — encode as UTF-8 bytes */
            int n = (esc == 'u') ? 4 : 8;
            uint32_t cp = 0;
            for (int i = 0; i < n; i++) {
                int c = lx_cur(L);
                if (!is_hex_digit(c)) break;
                cp = cp * 16 + hex_val(lx_adv(L));
            }
            if (buf) {
                /* Emit UTF-8 encoding */
                if (cp < 0x80) {
                    sb_push(buf, (int)cp);
                } else if (cp < 0x800) {
                    sb_push(buf, 0xC0 | (cp >> 6));
                    sb_push(buf, 0x80 | (cp & 0x3F));
                } else if (cp < 0x10000) {
                    sb_push(buf, 0xE0 | (cp >> 12));
                    sb_push(buf, 0x80 | ((cp >> 6) & 0x3F));
                    sb_push(buf, 0x80 | (cp & 0x3F));
                } else {
                    sb_push(buf, 0xF0 | (cp >> 18));
                    sb_push(buf, 0x80 | ((cp >> 12) & 0x3F));
                    sb_push(buf, 0x80 | ((cp >> 6) & 0x3F));
                    sb_push(buf, 0x80 | (cp & 0x3F));
                }
            }
            return cp;
        }
        default:
            if (buf) sb_push(buf, esc);
            return (uint32_t)(unsigned char)esc;
    }
}

/* Scan string body between quotes: calls lx_adv until closing '"' found.
   Appends decoded bytes to *buf.  Consumes the closing '"'. */
static void scan_string_body(Lx *L, StrBuf *buf) {
    while (lx_cur(L) != -1 && lx_cur(L) != '"') {
        if (lx_cur(L) == '\\') {
            lx_adv(L);              /* consume backslash */
            scan_escape(L, buf);
        } else {
            sb_push(buf, lx_adv(L));
        }
    }
    if (lx_cur(L) == '"') lx_adv(L);   /* consume closing '"' */
}

/* Skip whitespace; save/restore position helpers (for adjacent-string look-ahead) */
static inline void skip_ws(Lx *L) {
    while (lx_cur(L) != -1 && isspace(lx_cur(L))) lx_adv(L);
}

/* ================================================================
 * Number scanning helpers
 * ================================================================ */

/* Scan integer suffix chars (uUlL) into *sfx.  sfx may be NULL to discard. */
static void scan_int_suffix(Lx *L, StrBuf *sfx) {
    while (lx_cur(L) != -1 && strchr("uUlL", lx_cur(L))) {
        int c = lx_adv(L);
        if (sfx) sb_push(sfx, c);
    }
}

/* ================================================================
 * Main tokeniser
 * ================================================================ */
TokenVec lex_source(const char *src, size_t len) {
    Lx L;
    L.src = src; L.pos = 0; L.len = len; L.line = 1; L.col = 1;

    TokenVec tv;
    tv_init(&tv);

    while (lx_cur(&L) != -1) {
        int c = lx_cur(&L);
        int sl = L.line, sc = L.col;    /* saved start position */

        /* ---- whitespace ---- */
        if (isspace(c)) { lx_adv(&L); continue; }

        /* ---- preprocessor / GCC line-marker: skip rest of line ---- */
        if (c == '#') {
            while (lx_cur(&L) != -1 && lx_cur(&L) != '\n') lx_adv(&L);
            continue;
        }

        /* ---- line comment ---- */
        if (c == '/' && lx_peek(&L, 1) == '/') {
            while (lx_cur(&L) != -1 && lx_cur(&L) != '\n') lx_adv(&L);
            continue;
        }

        /* ---- block comment ---- */
        if (c == '/' && lx_peek(&L, 1) == '*') {
            lx_adv(&L); lx_adv(&L);    /* consume '/' and '*' */
            while (lx_cur(&L) != -1) {
                if (lx_cur(&L) == '*' && lx_peek(&L, 1) == '/') {
                    lx_adv(&L); lx_adv(&L); break;
                }
                lx_adv(&L);
            }
            continue;
        }

        /* ---- string literal (possibly with adjacent-string concat) ---- */
        if (c == '"') {
            lx_adv(&L);     /* consume opening '"' */
            StrBuf buf; sb_init(&buf);
            scan_string_body(&L, &buf);
            /* Adjacent string literal concatenation */
            for (;;) {
                size_t saved_pos  = L.pos;
                int    saved_line = L.line, saved_col = L.col;
                skip_ws(&L);
                if (lx_cur(&L) == '"') {
                    lx_adv(&L);
                    scan_string_body(&L, &buf);
                } else {
                    /* Restore look-ahead */
                    L.pos = saved_pos; L.line = saved_line; L.col = saved_col;
                    break;
                }
            }
            size_t tlen;
            char *txt = sb_finish(&buf, &tlen);
            tv_push(&tv, make_tok(TK_STR, txt, tlen, sl, sc));
            continue;
        }

        /* ---- character literal ---- */
        if (c == '\'') {
            lx_adv(&L);     /* consume opening '\'' */
            uint32_t char_val = 0;
            if (lx_cur(&L) == '\\') {
                lx_adv(&L);
                char_val = scan_escape(&L, NULL);
            } else if (lx_cur(&L) != '\'') {
                char_val = (uint32_t)lx_adv(&L);
            }
            if (lx_cur(&L) == '\'') lx_adv(&L);    /* closing '\'' */
            char text[32];
            int tl = snprintf(text, sizeof(text), "%u", char_val);
            tv_push(&tv, make_tok(TK_NUM, xstrdup(text), (size_t)tl, sl, sc));
            continue;
        }

        /* ---- identifiers / keywords / GCC extensions ---- */
        if (is_id_start(c)) {
            StrBuf wb; sb_init(&wb);
            while (is_id_cont(lx_cur(&L))) sb_push(&wb, lx_adv(&L));
            char *word = sb_str(&wb);

            /* Wide/unicode string prefix: L"…", u"…", U"…", u8"…" */
            bool is_prefix = (strcmp(word,"L")==0 || strcmp(word,"u")==0 ||
                              strcmp(word,"U")==0 || strcmp(word,"u8")==0);

            if (is_prefix && lx_cur(&L) == '"') {
                free(word);
                lx_adv(&L);     /* consume '"' */
                StrBuf buf; sb_init(&buf);
                scan_string_body(&L, &buf);
                /* Adjacent concat */
                for (;;) {
                    size_t sp = L.pos;
                    int sline = L.line, scol2 = L.col;
                    skip_ws(&L);
                    if (lx_cur(&L) == '"') {
                        lx_adv(&L);
                        scan_string_body(&L, &buf);
                    } else {
                        L.pos = sp; L.line = sline; L.col = scol2;
                        break;
                    }
                }
                size_t tlen;
                char *txt = sb_finish(&buf, &tlen);
                tv_push(&tv, make_tok(TK_STR, txt, tlen, sl, sc));
                continue;
            }

            /* Wide char literal: L'…' */
            if (is_prefix && lx_cur(&L) == '\'') {
                free(word);
                lx_adv(&L);     /* consume '\'' */
                uint32_t char_val = 0;
                if (lx_cur(&L) == '\\') {
                    lx_adv(&L);
                    char_val = scan_escape(&L, NULL);
                } else if (lx_cur(&L) != '\'') {
                    char_val = (uint32_t)lx_adv(&L);
                }
                if (lx_cur(&L) == '\'') lx_adv(&L);
                char text[32];
                int tl = snprintf(text, sizeof(text), "%u", char_val);
                tv_push(&tv, make_tok(TK_NUM, xstrdup(text), (size_t)tl, sl, sc));
                continue;
            }

            /* GCC skip names: skip identifier + optional following (...) */
            if (is_skip_name(word)) {
                free(word);
                skip_ws(&L);
                if (lx_cur(&L) == '(') skip_paren_group(&L);
                continue;
            }

            /* GCC remap */
            const RemapEntry *remap = find_remap(word);
            if (remap) {
                free(word);
                if (remap->to == NULL) continue;    /* drop */
                tv_push(&tv, make_tok_s(remap->to_kind, xstrdup(remap->to), sl, sc));
                continue;
            }

            /* Normal keyword or identifier */
            TokenKind kind = lookup_keyword(word);
            size_t wl = strlen(word);
            tv_push(&tv, make_tok(kind, word, wl, sl, sc));
            continue;
        }

        /* ---- numeric literals ---- */
        if (isdigit(c)) {
            /* Binary: 0b… */
            if (c == '0' && (lx_peek(&L,1) == 'b' || lx_peek(&L,1) == 'B')) {
                lx_adv(&L); lx_adv(&L);    /* 0 b */
                unsigned long long val = 0;
                while (lx_cur(&L) == '0' || lx_cur(&L) == '1')
                    val = val * 2 + (uint32_t)(lx_adv(&L) - '0');
                scan_int_suffix(&L, NULL);
                char text[64];
                int tl = snprintf(text, sizeof(text), "%llu", val);
                tv_push(&tv, make_tok(TK_NUM, xstrdup(text), (size_t)tl, sl, sc));
                continue;
            }

            /* Hex: 0x… */
            if (c == '0' && (lx_peek(&L,1) == 'x' || lx_peek(&L,1) == 'X')) {
                lx_adv(&L); lx_adv(&L);    /* 0 x */
                StrBuf hex; sb_init(&hex);
                while (is_hex_digit(lx_cur(&L))) sb_push(&hex, lx_adv(&L));

                /* Hex float: 0x<digits>.<digits>p±<exp> */
                if (lx_cur(&L) == '.' && is_hex_digit(lx_peek(&L,1))) {
                    sb_push(&hex, lx_adv(&L));  /* '.' */
                    while (is_hex_digit(lx_cur(&L))) sb_push(&hex, lx_adv(&L));
                    if (lx_cur(&L) == 'p' || lx_cur(&L) == 'P') {
                        sb_push(&hex, lx_adv(&L));
                        if (lx_cur(&L) == '+' || lx_cur(&L) == '-')
                            sb_push(&hex, lx_adv(&L));
                        while (isdigit(lx_cur(&L))) sb_push(&hex, lx_adv(&L));
                    }
                    while (lx_cur(&L) != -1 && strchr("fFlL", lx_cur(&L))) lx_adv(&L);
                    char *hstr = sb_str(&hex);
                    /* Build "0x<hstr>" and parse with strtod */
                    char hexnum[128];
                    snprintf(hexnum, sizeof(hexnum), "0x%s", hstr);
                    free(hstr);
                    double val = strtod(hexnum, NULL);
                    char text[64];
                    int tl = snprintf(text, sizeof(text), "%.17g", val);
                    tv_push(&tv, make_tok(TK_NUM, xstrdup(text), (size_t)tl, sl, sc));
                    continue;
                }

                /* Hex integer */
                char *digits = sb_str(&hex);
                unsigned long long val = digits[0] ? strtoull(digits, NULL, 16) : 0;
                free(digits);

                StrBuf sfx; sb_init(&sfx);
                scan_int_suffix(&L, &sfx);
                char *sfx_str = sb_str(&sfx);

                char text[64];
                int tl = snprintf(text, sizeof(text), "%llu%s", val, sfx_str);
                free(sfx_str);
                tv_push(&tv, make_tok(TK_NUM, xstrdup(text), (size_t)tl, sl, sc));
                continue;
            }

            /* Decimal integer or float */
            StrBuf num; sb_init(&num);
            while (isdigit(lx_cur(&L))) sb_push(&num, lx_adv(&L));
            bool is_float = false;

            if (lx_cur(&L) == '.' && isdigit(lx_peek(&L,1))) {
                is_float = true;
                sb_push(&num, lx_adv(&L));     /* '.' */
                while (isdigit(lx_cur(&L))) sb_push(&num, lx_adv(&L));
            }
            if (lx_cur(&L) == 'e' || lx_cur(&L) == 'E') {
                is_float = true;
                sb_push(&num, lx_adv(&L));
                if (lx_cur(&L) == '+' || lx_cur(&L) == '-')
                    sb_push(&num, lx_adv(&L));
                while (isdigit(lx_cur(&L))) sb_push(&num, lx_adv(&L));
            }
            if (is_float) {
                while (lx_cur(&L) != -1 && strchr("fFlL", lx_cur(&L))) lx_adv(&L);
                size_t tlen;
                char *txt = sb_finish(&num, &tlen);
                tv_push(&tv, make_tok(TK_NUM, txt, tlen, sl, sc));
            } else {
                StrBuf sfx; sb_init(&sfx);
                scan_int_suffix(&L, &sfx);
                char *sfx_str = sb_str(&sfx);
                char *num_str = sb_str(&num);
                size_t tlen;
                char *txt;
                if (sfx_str[0]) {
                    size_t total = strlen(num_str) + strlen(sfx_str) + 1;
                    txt = (char *)xmalloc(total);
                    tlen = (size_t)snprintf(txt, total, "%s%s", num_str, sfx_str);
                    free(num_str);
                } else {
                    txt = num_str;
                    tlen = strlen(txt);
                }
                free(sfx_str);
                tv_push(&tv, make_tok(TK_NUM, txt, tlen, sl, sc));
            }
            continue;
        }

        /* ---- .NN float literals (.5, .25, .1e3, …) ---- */
        if (c == '.' && isdigit(lx_peek(&L,1))) {
            StrBuf buf; sb_init(&buf);
            sb_push(&buf, '0');
            sb_push(&buf, lx_adv(&L));  /* '.' */
            while (isdigit(lx_cur(&L))) sb_push(&buf, lx_adv(&L));
            if (lx_cur(&L) == 'e' || lx_cur(&L) == 'E') {
                sb_push(&buf, lx_adv(&L));
                if (lx_cur(&L) == '+' || lx_cur(&L) == '-')
                    sb_push(&buf, lx_adv(&L));
                while (isdigit(lx_cur(&L))) sb_push(&buf, lx_adv(&L));
            }
            while (lx_cur(&L) != -1 && strchr("uUlLfF", lx_cur(&L))) lx_adv(&L);
            size_t tlen;
            char *txt = sb_finish(&buf, &tlen);
            tv_push(&tv, make_tok(TK_NUM, txt, tlen, sl, sc));
            continue;
        }

        /* ---- three-character operators ---- */
        {
            int c2 = lx_peek(&L,1), c3 = lx_peek(&L,2);
            if (c=='<' && c2=='<' && c3=='=') { lx_adv(&L);lx_adv(&L);lx_adv(&L); tv_push(&tv, make_tok_s(TK_LSHIFTEQ, xstrdup("<<="), sl,sc)); continue; }
            if (c=='>' && c2=='>' && c3=='=') { lx_adv(&L);lx_adv(&L);lx_adv(&L); tv_push(&tv, make_tok_s(TK_RSHIFTEQ, xstrdup(">>="), sl,sc)); continue; }
            if (c=='.' && c2=='.' && c3=='.') { lx_adv(&L);lx_adv(&L);lx_adv(&L); tv_push(&tv, make_tok_s(TK_ELLIPSIS, xstrdup("..."), sl,sc)); continue; }
        }

        /* ---- two-character operators ---- */
#define TW(A,B,KIND,TXT) \
        if (c==(A) && lx_peek(&L,1)==(B)) { \
            lx_adv(&L); lx_adv(&L); \
            tv_push(&tv, make_tok_s(KIND, xstrdup(TXT), sl, sc)); continue; }

        TW('=','=', TK_EQ,       "==")
        TW('!','=', TK_NE,       "!=")
        TW('<','=', TK_LE,       "<=")
        TW('>','=', TK_GE,       ">=")
        TW('+','=', TK_PLUSEQ,   "+=")
        TW('-','=', TK_MINUSEQ,  "-=")
        TW('*','=', TK_STAREQ,   "*=")
        TW('/','=', TK_SLASHEQ,  "/=")
        TW('%','=', TK_PERCENTEQ,"%=")
        TW('+','+', TK_PLUSPLUS, "++")
        TW('-','-', TK_MINUSMINUS,"--")
        TW('&','&', TK_AMPAMP,   "&&")
        TW('|','|', TK_PIPEPIPE, "||")
        TW('-','>', TK_ARROW,    "->")
        TW('<','<', TK_LSHIFT,   "<<")
        TW('>','>', TK_RSHIFT,   ">>")
        TW('&','=', TK_AMPEQ,    "&=")
        TW('|','=', TK_PIPEEQ,   "|=")
        TW('^','=', TK_CARETEQ,  "^=")
#undef TW

        /* ---- dot (not float, not ...) ---- */
        if (c == '.') {
            lx_adv(&L);
            tv_push(&tv, make_tok_s(TK_DOT, xstrdup("."), sl, sc));
            continue;
        }

        /* ---- single-character operators / punctuation ---- */
        {
            TokenKind kind = TK__COUNT;
            switch (c) {
                case '{': kind = TK_LBRACE;   break;
                case '}': kind = TK_RBRACE;   break;
                case '(': kind = TK_LPAREN;   break;
                case ')': kind = TK_RPAREN;   break;
                case '[': kind = TK_LBRACK;   break;
                case ']': kind = TK_RBRACK;   break;
                case ';': kind = TK_SEMI;     break;
                case ',': kind = TK_COMMA;    break;
                case ':': kind = TK_COLON;    break;
                case '?': kind = TK_QUESTION; break;
                case '=': kind = TK_ASSIGN;   break;
                case '+': kind = TK_PLUS;     break;
                case '-': kind = TK_MINUS;    break;
                case '*': kind = TK_STAR;     break;
                case '/': kind = TK_SLASH;    break;
                case '%': kind = TK_PERCENT;  break;
                case '&': kind = TK_AMP;      break;
                case '|': kind = TK_PIPE;     break;
                case '^': kind = TK_CARET;    break;
                case '!': kind = TK_BANG;     break;
                case '<': kind = TK_LT;       break;
                case '>': kind = TK_GT;       break;
                case '~': kind = TK_TILDE;    break;
            }
            if (kind != TK__COUNT) {
                char s[2] = { (char)c, '\0' };
                lx_adv(&L);
                tv_push(&tv, make_tok_s(kind, xstrdup(s), sl, sc));
                continue;
            }
        }

        /* ---- unknown character – skip with warning ---- */
        fprintf(stderr, "warning: unexpected character '%c' (0x%02X) at %d:%d\n",
                isprint(c) ? c : '?', c, sl, sc);
        lx_adv(&L);
    }

    /* EOF token */
    tv_push(&tv, make_tok_s(TK_EOF, xstrdup(""), L.line, L.col));
    return tv;
}
