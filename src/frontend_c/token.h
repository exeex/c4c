/* token.h – token kinds and token struct for the C frontend */
#pragma once
#include <stddef.h>

typedef enum {
    /* ---- keywords ---- */
    TK_INT = 0, TK_CHAR,  TK_VOID,     TK_EXTERN, TK_RETURN,
    TK_IF,      TK_ELSE,  TK_WHILE,    TK_FOR,    TK_DO,
    TK_SWITCH,  TK_CASE,  TK_DEFAULT,  TK_BREAK,  TK_CONTINUE, TK_GOTO,
    TK_STRUCT,  TK_TYPEDEF, TK_STATIC, TK_SIZEOF,
    TK_LONG,    TK_SHORT, TK_UNSIGNED, TK_SIGNED,
    TK_DOUBLE,  TK_FLOAT, TK_CONST,    TK_VOLATILE, TK_RESTRICT,
    TK_INLINE,  TK_ENUM,  TK_UNION,    TK_REGISTER,
    /* ---- atoms ---- */
    TK_ID,   /* identifier */
    TK_NUM,  /* numeric literal (decimal text, float text, or char value) */
    TK_STR,  /* string literal (decoded bytes) */
    /* ---- punctuation ---- */
    TK_LBRACE,  TK_RBRACE,   /* { } */
    TK_LPAREN,  TK_RPAREN,   /* ( ) */
    TK_LBRACK,  TK_RBRACK,   /* [ ] */
    TK_SEMI,    TK_COMMA,    TK_DOT,  TK_COLON,  TK_QUESTION,
    TK_ELLIPSIS,                       /* ... */
    /* ---- operators ---- */
    TK_ASSIGN,                         /* =  */
    TK_EQ,    TK_NE,                   /* == != */
    TK_LT,    TK_LE,  TK_GT,  TK_GE,  /* < <= > >= */
    TK_PLUS,  TK_PLUSEQ,  TK_PLUSPLUS,      /* + += ++ */
    TK_MINUS, TK_MINUSEQ, TK_MINUSMINUS, TK_ARROW, /* - -= -- -> */
    TK_STAR,  TK_STAREQ,                /* *  *= */
    TK_SLASH, TK_SLASHEQ,              /* /  /= */
    TK_PERCENT,  TK_PERCENTEQ,         /* %  %= */
    TK_AMP,   TK_AMPAMP,   TK_AMPEQ,  /* &  &&  &= */
    TK_PIPE,  TK_PIPEPIPE, TK_PIPEEQ, /* |  ||  |= */
    TK_CARET, TK_CARETEQ,             /* ^  ^= */
    TK_BANG,  TK_TILDE,               /* !  ~ */
    TK_LSHIFT, TK_RSHIFT,             /* << >> */
    TK_LSHIFTEQ, TK_RSHIFTEQ,        /* <<= >>= */
    /* ---- sentinel ---- */
    TK_EOF,
    TK__COUNT  /* must be last */
} TokenKind;

/* Return a short debug name for a token kind */
static inline const char *tok_kind_str(TokenKind k) {
    static const char *names[TK__COUNT + 1] = {
        "INT","CHAR","VOID","EXTERN","RETURN",
        "IF","ELSE","WHILE","FOR","DO",
        "SWITCH","CASE","DEFAULT","BREAK","CONTINUE","GOTO",
        "STRUCT","TYPEDEF","STATIC","SIZEOF",
        "LONG","SHORT","UNSIGNED","SIGNED",
        "DOUBLE","FLOAT","CONST","VOLATILE","RESTRICT",
        "INLINE","ENUM","UNION","REGISTER",
        "ID","NUM","STR",
        "LBRACE","RBRACE","LPAREN","RPAREN","LBRACK","RBRACK",
        "SEMI","COMMA","DOT","COLON","QUESTION","ELLIPSIS",
        "ASSIGN","EQ","NE","LT","LE","GT","GE",
        "PLUS","PLUSEQ","PLUSPLUS",
        "MINUS","MINUSEQ","MINUSMINUS","ARROW",
        "STAR","STAREQ","SLASH","SLASHEQ",
        "PERCENT","PERCENTEQ",
        "AMP","AMPAMP","AMPEQ",
        "PIPE","PIPEPIPE","PIPEEQ",
        "CARET","CARETEQ","BANG","TILDE",
        "LSHIFT","RSHIFT","LSHIFTEQ","RSHIFTEQ",
        "EOF","???"
    };
    if (k < 0 || k > TK__COUNT) return "???";
    return names[k];
}

typedef struct {
    TokenKind kind;
    char     *text;      /* malloc'd; may contain embedded NUL bytes */
    size_t    text_len;  /* actual byte count (since text may have NUL bytes) */
    int       line;
    int       col;
} Token;
