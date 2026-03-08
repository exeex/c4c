#include "token.hpp"

namespace tinyc2ll::frontend_cxx {

const char *token_kind_name(TokenKind kind) {
  switch (kind) {
    case TokenKind::IntLit:    return "INT_LIT";
    case TokenKind::FloatLit:  return "FLOAT_LIT";
    case TokenKind::StrLit:    return "STR_LIT";
    case TokenKind::CharLit:   return "CHAR_LIT";
    case TokenKind::Identifier: return "IDENT";

    // C keywords
    case TokenKind::KwAuto:         return "KW_auto";
    case TokenKind::KwBreak:        return "KW_break";
    case TokenKind::KwCase:         return "KW_case";
    case TokenKind::KwChar:         return "KW_char";
    case TokenKind::KwConst:        return "KW_const";
    case TokenKind::KwContinue:     return "KW_continue";
    case TokenKind::KwDefault:      return "KW_default";
    case TokenKind::KwDo:           return "KW_do";
    case TokenKind::KwDouble:       return "KW_double";
    case TokenKind::KwElse:         return "KW_else";
    case TokenKind::KwEnum:         return "KW_enum";
    case TokenKind::KwExtern:       return "KW_extern";
    case TokenKind::KwFloat:        return "KW_float";
    case TokenKind::KwFor:          return "KW_for";
    case TokenKind::KwGoto:         return "KW_goto";
    case TokenKind::KwIf:           return "KW_if";
    case TokenKind::KwInline:       return "KW_inline";
    case TokenKind::KwInt:          return "KW_int";
    case TokenKind::KwLong:         return "KW_long";
    case TokenKind::KwRegister:     return "KW_register";
    case TokenKind::KwRestrict:     return "KW_restrict";
    case TokenKind::KwReturn:       return "KW_return";
    case TokenKind::KwShort:        return "KW_short";
    case TokenKind::KwSigned:       return "KW_signed";
    case TokenKind::KwSizeof:       return "KW_sizeof";
    case TokenKind::KwStatic:       return "KW_static";
    case TokenKind::KwStruct:       return "KW_struct";
    case TokenKind::KwSwitch:       return "KW_switch";
    case TokenKind::KwTypedef:      return "KW_typedef";
    case TokenKind::KwUnion:        return "KW_union";
    case TokenKind::KwUnsigned:     return "KW_unsigned";
    case TokenKind::KwVoid:         return "KW_void";
    case TokenKind::KwVolatile:     return "KW_volatile";
    case TokenKind::KwWhile:        return "KW_while";

    // C11 keywords
    case TokenKind::KwAlignas:      return "KW__Alignas";
    case TokenKind::KwAlignof:      return "KW__Alignof";
    case TokenKind::KwAtomic:       return "KW__Atomic";
    case TokenKind::KwBool:         return "KW__Bool";
    case TokenKind::KwComplex:      return "KW__Complex";
    case TokenKind::KwGeneric:      return "KW__Generic";
    case TokenKind::KwImaginary:    return "KW__Imaginary";
    case TokenKind::KwNoreturn:     return "KW__Noreturn";
    case TokenKind::KwStaticAssert: return "KW__Static_assert";
    case TokenKind::KwThreadLocal:  return "KW__Thread_local";

    // GCC extensions
    case TokenKind::KwTypeof:       return "KW_typeof";
    case TokenKind::KwAsm:          return "KW_asm";
    case TokenKind::KwAttribute:    return "KW___attribute__";
    case TokenKind::KwExtension:    return "KW___extension__";
    case TokenKind::KwBuiltin:      return "KW___builtin_va_list";
    case TokenKind::KwBuiltinVaArg: return "KW___builtin_va_arg";
    case TokenKind::KwInt128:       return "KW___int128";
    case TokenKind::KwUInt128:      return "KW___uint128_t";
    case TokenKind::KwAutoType:     return "KW___auto_type";
    case TokenKind::KwGnuAlignof:   return "KW___alignof__";
    case TokenKind::KwGccReal:      return "KW___real__";
    case TokenKind::KwGccImag:      return "KW___imag__";

    // Punctuation
    case TokenKind::LParen:    return "LPAREN";
    case TokenKind::RParen:    return "RPAREN";
    case TokenKind::LBrace:    return "LBRACE";
    case TokenKind::RBrace:    return "RBRACE";
    case TokenKind::LBracket:  return "LBRACKET";
    case TokenKind::RBracket:  return "RBRACKET";
    case TokenKind::Semi:      return "SEMI";
    case TokenKind::Comma:     return "COMMA";
    case TokenKind::Dot:       return "DOT";
    case TokenKind::Arrow:     return "ARROW";
    case TokenKind::Ellipsis:  return "ELLIPSIS";

    // Single-char operators
    case TokenKind::Plus:      return "PLUS";
    case TokenKind::Minus:     return "MINUS";
    case TokenKind::Star:      return "STAR";
    case TokenKind::Slash:     return "SLASH";
    case TokenKind::Percent:   return "PERCENT";
    case TokenKind::Amp:       return "AMP";
    case TokenKind::Pipe:      return "PIPE";
    case TokenKind::Caret:     return "CARET";
    case TokenKind::Tilde:     return "TILDE";
    case TokenKind::Bang:      return "BANG";
    case TokenKind::Assign:    return "ASSIGN";
    case TokenKind::Less:      return "LESS";
    case TokenKind::Greater:   return "GREATER";
    case TokenKind::Question:  return "QUESTION";
    case TokenKind::Colon:     return "COLON";
    case TokenKind::Hash:      return "HASH";

    // Compound operators
    case TokenKind::PlusPlus:             return "PLUS_PLUS";
    case TokenKind::MinusMinus:           return "MINUS_MINUS";
    case TokenKind::PlusAssign:           return "PLUS_ASSIGN";
    case TokenKind::MinusAssign:          return "MINUS_ASSIGN";
    case TokenKind::StarAssign:           return "STAR_ASSIGN";
    case TokenKind::SlashAssign:          return "SLASH_ASSIGN";
    case TokenKind::PercentAssign:        return "PERCENT_ASSIGN";
    case TokenKind::AmpAssign:            return "AMP_ASSIGN";
    case TokenKind::PipeAssign:           return "PIPE_ASSIGN";
    case TokenKind::CaretAssign:          return "CARET_ASSIGN";
    case TokenKind::LessLess:             return "LSHIFT";
    case TokenKind::GreaterGreater:       return "RSHIFT";
    case TokenKind::LessLessAssign:       return "LSHIFT_ASSIGN";
    case TokenKind::GreaterGreaterAssign: return "RSHIFT_ASSIGN";
    case TokenKind::EqualEqual:           return "EQUAL_EQUAL";
    case TokenKind::BangEqual:            return "BANG_EQUAL";
    case TokenKind::LessEqual:            return "LESS_EQUAL";
    case TokenKind::GreaterEqual:         return "GREATER_EQUAL";
    case TokenKind::AmpAmp:               return "AMP_AMP";
    case TokenKind::PipePipe:             return "PIPE_PIPE";
    case TokenKind::HashHash:             return "HASH_HASH";

    case TokenKind::EndOfFile: return "EOF";
    case TokenKind::Invalid:   return "INVALID";
  }
  return "UNKNOWN";
}

// keyword_from_string: map identifier text to its keyword TokenKind.
// Returns TokenKind::Identifier when the string is not a keyword.
// Pure-C backport note: replace std::string param with const char* + strlen.
TokenKind keyword_from_string(const std::string &s, bool gnu_extensions) {
  // Fast-reject: all C/GCC keywords start with one of these chars.
  if (s.empty()) return TokenKind::Identifier;
  char first = s[0];
  if (first != '_' && first != 'a' && first != 'b' && first != 'c' &&
      first != 'd' && first != 'e' && first != 'f' && first != 'g' &&
      first != 'i' && first != 'l' && first != 'r' && first != 's' &&
      first != 't' && first != 'u' && first != 'v' && first != 'w') {
    return TokenKind::Identifier;
  }

  // Standard C keywords
  if (s == "auto")     return TokenKind::KwAuto;
  if (s == "break")    return TokenKind::KwBreak;
  if (s == "case")     return TokenKind::KwCase;
  if (s == "char")     return TokenKind::KwChar;
  if (s == "const")    return TokenKind::KwConst;
  if (s == "continue") return TokenKind::KwContinue;
  if (s == "default")  return TokenKind::KwDefault;
  if (s == "do")       return TokenKind::KwDo;
  if (s == "double")   return TokenKind::KwDouble;
  if (s == "else")     return TokenKind::KwElse;
  if (s == "enum")     return TokenKind::KwEnum;
  if (s == "extern")   return TokenKind::KwExtern;
  if (s == "float")    return TokenKind::KwFloat;
  if (s == "for")      return TokenKind::KwFor;
  if (s == "goto")     return TokenKind::KwGoto;
  if (s == "if")       return TokenKind::KwIf;
  if (s == "inline")   return TokenKind::KwInline;
  if (s == "int")      return TokenKind::KwInt;
  if (s == "long")     return TokenKind::KwLong;
  if (s == "register") return TokenKind::KwRegister;
  if (s == "restrict") return TokenKind::KwRestrict;
  if (s == "return")   return TokenKind::KwReturn;
  if (s == "short")    return TokenKind::KwShort;
  if (s == "signed")   return TokenKind::KwSigned;
  if (s == "sizeof")   return TokenKind::KwSizeof;
  if (s == "static")   return TokenKind::KwStatic;
  if (s == "struct")   return TokenKind::KwStruct;
  if (s == "switch")   return TokenKind::KwSwitch;
  if (s == "typedef")  return TokenKind::KwTypedef;
  if (s == "union")    return TokenKind::KwUnion;
  if (s == "unsigned") return TokenKind::KwUnsigned;
  if (s == "void")     return TokenKind::KwVoid;
  if (s == "while")    return TokenKind::KwWhile;

  // Volatile aliases
  if (s == "volatile" || s == "__volatile__" || s == "__volatile")
    return TokenKind::KwVolatile;
  // Const aliases
  if (s == "__const" || s == "__const__")
    return TokenKind::KwConst;
  // Inline aliases
  if (s == "__inline" || s == "__inline__")
    return TokenKind::KwInline;
  // Restrict aliases
  if (s == "__restrict" || s == "__restrict__")
    return TokenKind::KwRestrict;
  // Signed alias
  if (s == "__signed__")
    return TokenKind::KwSigned;

  // C11 keywords
  if (s == "_Alignas")                          return TokenKind::KwAlignas;
  if (s == "_Alignof")                          return TokenKind::KwAlignof;
  if (s == "_Atomic")                           return TokenKind::KwAtomic;
  if (s == "_Bool")                             return TokenKind::KwBool;
  if (s == "_Complex" || s == "__complex__" || s == "__complex")
                                                return TokenKind::KwComplex;
  if (s == "_Generic")                          return TokenKind::KwGeneric;
  if (s == "_Imaginary")                        return TokenKind::KwImaginary;
  if (s == "_Noreturn" || s == "__noreturn__")  return TokenKind::KwNoreturn;
  if (s == "_Static_assert" || s == "static_assert")
                                                return TokenKind::KwStaticAssert;
  if (s == "_Thread_local" || s == "__thread")  return TokenKind::KwThreadLocal;

  // GCC extensions
  if (s == "__typeof__" || s == "__typeof")     return TokenKind::KwTypeof;
  if (gnu_extensions && s == "typeof")          return TokenKind::KwTypeof;
  if (s == "__asm__" || s == "__asm")           return TokenKind::KwAsm;
  if (gnu_extensions && s == "asm")             return TokenKind::KwAsm;
  if (s == "__attribute__" || s == "__attribute")
                                                return TokenKind::KwAttribute;
  if (s == "__extension__")                     return TokenKind::KwExtension;
  if (s == "__builtin_va_list")                 return TokenKind::KwBuiltin;
  if (s == "__builtin_va_arg")                  return TokenKind::KwBuiltinVaArg;
  if (s == "__int128" || s == "__int128_t")     return TokenKind::KwInt128;
  if (s == "__uint128_t")                       return TokenKind::KwUInt128;
  if (s == "__auto_type")                       return TokenKind::KwAutoType;
  if (s == "__alignof__" || s == "__alignof")   return TokenKind::KwGnuAlignof;
  if (s == "__real__" || s == "__real")         return TokenKind::KwGccReal;
  if (s == "__imag__" || s == "__imag")         return TokenKind::KwGccImag;

  return TokenKind::Identifier;
}

}  // namespace tinyc2ll::frontend_cxx
