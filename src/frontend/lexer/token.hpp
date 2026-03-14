#pragma once

#include <string>

namespace tinyc2ll::frontend_cxx {

// Token kinds mirroring ref/claudes-c-compiler/src/frontend/lexer/token.rs
// Pure-C backport note: map to a plain enum + tagged union when porting.
enum class TokenKind {
  // Literals (raw lexeme stored in Token::lexeme)
  IntLit,    // integer literal (decimal, hex, octal, binary + suffixes)
  FloatLit,  // floating-point literal (including f/l suffixes)
  StrLit,    // "..." string literal
  CharLit,   // '...' character literal

  // Identifier (not a keyword)
  Identifier,

  // ---- C keywords ----
  KwAuto,
  KwBreak,
  KwCase,
  KwChar,
  KwConst,
  KwContinue,
  KwDefault,
  KwDo,
  KwDouble,
  KwElse,
  KwEnum,
  KwExtern,
  KwFloat,
  KwFor,
  KwGoto,
  KwIf,
  KwInline,
  KwInt,
  KwLong,
  KwRegister,
  KwRestrict,
  KwReturn,
  KwShort,
  KwSigned,
  KwSizeof,
  KwStatic,
  KwStruct,
  KwSwitch,
  KwTypedef,
  KwUnion,
  KwUnsigned,
  KwVoid,
  KwVolatile,
  KwWhile,

  // C11 keywords
  KwAlignas,
  KwAlignof,
  KwAtomic,
  KwBool,
  KwComplex,
  KwGeneric,
  KwImaginary,
  KwNoreturn,
  KwStaticAssert,
  KwThreadLocal,

  // GCC extension keywords
  KwTypeof,
  KwTypeofUnqual,
  KwAsm,
  KwAttribute,   // __attribute__
  KwExtension,   // __extension__
  KwBuiltin,     // __builtin_va_list (used as type name)
  KwBuiltinVaArg,  // __builtin_va_arg
  KwInt128,      // __int128 / __int128_t
  KwUInt128,     // __uint128_t
  KwAutoType,    // __auto_type
  KwGnuAlignof,  // __alignof__
  KwGccReal,     // __real__ / __real
  KwGccImag,     // __imag__ / __imag

  // ---- Punctuation ----
  LParen,    // (
  RParen,    // )
  LBrace,    // {
  RBrace,    // }
  LBracket,  // [
  RBracket,  // ]
  Semi,      // ;
  Comma,     // ,
  Dot,       // .
  Arrow,     // ->
  Ellipsis,  // ...

  // ---- Single-char operators ----
  Plus,      // +
  Minus,     // -
  Star,      // *
  Slash,     // /
  Percent,   // %
  Amp,       // &
  Pipe,      // |
  Caret,     // ^
  Tilde,     // ~
  Bang,      // !
  Assign,    // =
  Less,      // <
  Greater,   // >
  Question,  // ?
  Colon,     // :
  Hash,      // #

  // ---- Compound operators ----
  PlusPlus,              // ++
  MinusMinus,            // --
  PlusAssign,            // +=
  MinusAssign,           // -=
  StarAssign,            // *=
  SlashAssign,           // /=
  PercentAssign,         // %=
  AmpAssign,             // &=
  PipeAssign,            // |=
  CaretAssign,           // ^=
  LessLess,              // <<
  GreaterGreater,        // >>
  LessLessAssign,        // <<=
  GreaterGreaterAssign,  // >>=
  EqualEqual,            // ==
  BangEqual,             // !=
  LessEqual,             // <=
  GreaterEqual,          // >=
  AmpAmp,                // &&
  PipePipe,              // ||
  HashHash,              // ##

  // Special
  EndOfFile,
  Invalid,
};

struct Token {
  TokenKind kind;
  std::string lexeme;  // raw source text
  int line;
  int column;
};

// Returns a short debug name for the kind (used in --lex-only output).
const char *token_kind_name(TokenKind kind);

// Returns the keyword TokenKind for a given identifier string, or
// TokenKind::Identifier if not a keyword. gnu_extensions enables bare
// "asm"/"typeof".
TokenKind keyword_from_string(const std::string &s, bool gnu_extensions = true);

}  // namespace tinyc2ll::frontend_cxx
