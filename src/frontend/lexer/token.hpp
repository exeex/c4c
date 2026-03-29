#pragma once

#include <string>

#include "source_profile.hpp"

namespace c4c {

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

  // ---- C++ subset keywords (only reserved in CppSubset/C4 profiles) ----
  KwTemplate,    // template
  KwConstexpr,   // constexpr
  KwConsteval,   // consteval
  KwNamespace,   // namespace
  KwUsing,       // using
  KwTypename,    // typename
  KwNoexcept,    // noexcept
  KwDecltype,    // decltype
  KwStaticCast,      // static_cast
  KwReinterpretCast, // reinterpret_cast
  KwConstCast,       // const_cast
  KwClass,           // class (synonym for struct in C++ template contexts)
  KwFinal,           // final
  KwPrivate,         // private
  KwProtected,       // protected
  KwPublic,          // public
  KwFriend,          // friend
  KwVirtual,         // virtual
  KwNullptr,         // nullptr
  KwTrue,            // true
  KwFalse,           // false
  KwOperator,        // operator (C++ operator overloading)
  KwNew,             // new
  KwDelete,          // delete (= delete for deleted functions)
  KwExplicit,        // explicit (C++ explicit conversion/constructor specifier)

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
  ColonColon,            // :: (C++ scope resolution)

  // Pragma tokens (emitted by preprocessor, consumed by parser)
  PragmaPack,    // #pragma pack(...) — lexeme holds the arguments e.g. "1", "push,2", "pop", ""
  PragmaWeak,    // #pragma weak symbol — lexeme holds the symbol name
  PragmaGccVisibility, // #pragma GCC visibility push/pop — lexeme: "push,hidden", "push,default", "pop"

  // Special
  EndOfFile,
  Invalid,
};

struct Token {
  TokenKind kind;
  std::string lexeme;  // raw source text
  std::string file;    // logical source file after #line / include markers
  int line;
  int column;
};

// Returns a short debug name for the kind (used in --lex-only output).
const char *token_kind_name(TokenKind kind);

// Returns the keyword TokenKind for a given identifier string, or
// TokenKind::Identifier if not a keyword. gnu_extensions enables bare
// "asm"/"typeof".
TokenKind keyword_from_string(const std::string &s, bool gnu_extensions = true,
                              LexProfile profile = LexProfile::C);

}  // namespace c4c
