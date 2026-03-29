#include "token.hpp"

namespace c4c {

namespace {

bool starts_with(const std::string& s, const char* prefix) {
  const size_t prefix_len = std::char_traits<char>::length(prefix);
  return s.size() >= prefix_len && s.compare(0, prefix_len, prefix) == 0;
}

bool string_in_list(const std::string& s, const char* const* items,
                    size_t count) {
  for (size_t i = 0; i < count; ++i) {
    if (s == items[i]) return true;
  }
  return false;
}

// Clang TokenKinds.def TYPE_TRAIT_{1,2,N}, ARRAY_TYPE_TRAIT, and
// EXPRESSION_TRAIT spellings. Transform type traits are also tracked so the
// lexer-side registry can answer one "known builtin trait" question.
const char* const kBuiltinTypeTraitNames[] = {
    "__builtin_types_compatible_p",
    "__is_interface_class",
    "__is_sealed",
    "__is_destructible",
    "__is_trivially_destructible",
    "__is_nothrow_destructible",
    "__is_nothrow_assignable",
    "__is_constructible",
    "__is_nothrow_constructible",
    "__is_assignable",
    "__has_nothrow_move_assign",
    "__has_trivial_move_assign",
    "__has_trivial_move_constructor",
    "__builtin_is_implicit_lifetime",
    "__builtin_is_virtual_base_of",
    "__has_nothrow_assign",
    "__has_nothrow_copy",
    "__has_nothrow_constructor",
    "__has_trivial_assign",
    "__has_trivial_copy",
    "__has_trivial_constructor",
    "__has_trivial_destructor",
    "__has_virtual_destructor",
    "__is_abstract",
    "__is_aggregate",
    "__is_base_of",
    "__is_class",
    "__is_convertible_to",
    "__is_empty",
    "__is_enum",
    "__is_final",
    "__is_literal",
    "__is_pod",
    "__is_polymorphic",
    "__is_standard_layout",
    "__is_trivial",
    "__is_trivially_assignable",
    "__is_trivially_constructible",
    "__is_trivially_copyable",
    "__is_union",
    "__has_unique_object_representations",
    "__is_layout_compatible",
    "__is_pointer_interconvertible_base_of",
    "__is_trivially_equality_comparable",
    "__is_bounded_array",
    "__is_unbounded_array",
    "__is_scoped_enum",
    "__can_pass_in_regs",
    "__reference_binds_to_temporary",
    "__reference_constructs_from_temporary",
    "__reference_converts_from_temporary",
    "__builtin_lt_synthesizes_from_spaceship",
    "__builtin_le_synthesizes_from_spaceship",
    "__builtin_gt_synthesizes_from_spaceship",
    "__builtin_ge_synthesizes_from_spaceship",
    "__builtin_is_cpp_trivially_relocatable",
    "__is_trivially_relocatable",
    "__is_bitwise_cloneable",
    "__builtin_structured_binding_size",
    "__is_lvalue_expr",
    "__is_rvalue_expr",
    "__is_arithmetic",
    "__is_floating_point",
    "__is_integral",
    "__is_complete_type",
    "__is_void",
    "__is_array",
    "__is_function",
    "__is_reference",
    "__is_lvalue_reference",
    "__is_rvalue_reference",
    "__is_fundamental",
    "__is_object",
    "__is_scalar",
    "__is_compound",
    "__is_pointer",
    "__is_member_object_pointer",
    "__is_member_function_pointer",
    "__is_member_pointer",
    "__is_const",
    "__is_volatile",
    "__is_signed",
    "__is_unsigned",
    "__is_same",
    "__is_convertible",
    "__is_nothrow_convertible",
    "__array_rank",
    "__array_extent",
    "__builtin_hlsl_is_scalarized_layout_compatible",
    "__builtin_hlsl_is_intangible",
    "__builtin_hlsl_is_typed_resource_element_compatible",
    "__add_lvalue_reference",
    "__add_pointer",
    "__add_rvalue_reference",
    "__decay",
    "__make_signed",
    "__make_unsigned",
    "__remove_all_extents",
    "__remove_const",
    "__remove_cv",
    "__remove_cvref",
    "__remove_extent",
    "__remove_pointer",
    "__remove_reference_t",
    "__remove_restrict",
    "__remove_volatile",
    "__underlying_type",
};

}  // namespace

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
    case TokenKind::KwTypeofUnqual: return "KW_typeof_unqual";
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
    case TokenKind::ColonColon:           return "COLON_COLON";

    // C++ subset keywords
    case TokenKind::KwTemplate:  return "KW_template";
    case TokenKind::KwConstexpr: return "KW_constexpr";
    case TokenKind::KwConsteval: return "KW_consteval";
    case TokenKind::KwNamespace: return "KW_namespace";
    case TokenKind::KwUsing: return "KW_using";
    case TokenKind::KwTypename: return "KW_typename";
    case TokenKind::KwNoexcept: return "KW_noexcept";
    case TokenKind::KwDecltype: return "KW_decltype";
    case TokenKind::KwStaticCast: return "KW_static_cast";
    case TokenKind::KwReinterpretCast: return "KW_reinterpret_cast";
    case TokenKind::KwConstCast: return "KW_const_cast";
    case TokenKind::KwClass: return "KW_class";
    case TokenKind::KwFinal: return "KW_final";
    case TokenKind::KwOverride: return "KW_override";
    case TokenKind::KwPrivate: return "KW_private";
    case TokenKind::KwProtected: return "KW_protected";
    case TokenKind::KwPublic: return "KW_public";
    case TokenKind::KwFriend: return "KW_friend";
    case TokenKind::KwVirtual: return "KW_virtual";
    case TokenKind::KwMutable: return "KW_mutable";
    case TokenKind::KwNullptr: return "KW_nullptr";
    case TokenKind::KwTrue: return "KW_true";
    case TokenKind::KwFalse: return "KW_false";
    case TokenKind::KwOperator: return "KW_operator";
    case TokenKind::KwNew: return "KW_new";
    case TokenKind::KwDelete: return "KW_delete";
    case TokenKind::KwExplicit: return "KW_explicit";
    case TokenKind::KwRequires: return "KW_requires";

    case TokenKind::PragmaPack: return "PRAGMA_PACK";

    case TokenKind::EndOfFile: return "EOF";
    case TokenKind::Invalid:   return "INVALID";
  }
  return "UNKNOWN";
}

// keyword_from_string: map identifier text to its keyword TokenKind.
// Returns TokenKind::Identifier when the string is not a keyword.
// Pure-C backport note: replace std::string param with const char* + strlen.
TokenKind keyword_from_string(const std::string &s, bool gnu_extensions,
                              LexProfile profile) {
  // Fast-reject: all C/GCC/cpp keywords start with one of these chars.
  if (s.empty()) return TokenKind::Identifier;
  char first = s[0];
  if (first != '_' && first != 'a' && first != 'b' && first != 'c' &&
      first != 'd' && first != 'e' && first != 'f' && first != 'g' &&
      first != 'i' && first != 'l' && first != 'm' && first != 'n' && first != 'o' && first != 'r' &&
      first != 'p' && first != 's' && first != 't' && first != 'u' && first != 'v' &&
      first != 'w') {
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
  if (s == "alignas")
    return TokenKind::KwAlignas;
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
  if (s == "__typeof_unqual__" || s == "typeof_unqual")
                                                return TokenKind::KwTypeofUnqual;
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

  // C++ subset keywords (only in CppSubset or C4 profiles)
  if (profile != LexProfile::C) {
    if (s == "bool")      return TokenKind::KwBool;
    if (s == "and")       return TokenKind::AmpAmp;
    if (s == "bitand")    return TokenKind::Amp;
    if (s == "bitor")     return TokenKind::Pipe;
    if (s == "bitxor")    return TokenKind::Caret;
    if (s == "compl")     return TokenKind::Tilde;
    if (s == "not")       return TokenKind::Bang;
    if (s == "not_eq")    return TokenKind::BangEqual;
    if (s == "or")        return TokenKind::PipePipe;
    if (s == "template")  return TokenKind::KwTemplate;
    if (s == "constexpr") return TokenKind::KwConstexpr;
    if (s == "consteval") return TokenKind::KwConsteval;
    if (s == "namespace") return TokenKind::KwNamespace;
    if (s == "using") return TokenKind::KwUsing;
    if (s == "typename") return TokenKind::KwTypename;
    if (s == "noexcept") return TokenKind::KwNoexcept;
    if (s == "decltype" || s == "__decltype") return TokenKind::KwDecltype;
    if (s == "static_cast")      return TokenKind::KwStaticCast;
    if (s == "reinterpret_cast") return TokenKind::KwReinterpretCast;
    if (s == "const_cast")       return TokenKind::KwConstCast;
    if (s == "class")            return TokenKind::KwClass;
    if (s == "final")            return TokenKind::KwFinal;
    if (s == "override")         return TokenKind::KwOverride;
    if (s == "private")          return TokenKind::KwPrivate;
    if (s == "protected")        return TokenKind::KwProtected;
    if (s == "public")           return TokenKind::KwPublic;
    if (s == "friend")           return TokenKind::KwFriend;
    if (s == "virtual")          return TokenKind::KwVirtual;
    if (s == "mutable")         return TokenKind::KwMutable;
    if (s == "nullptr")          return TokenKind::KwNullptr;
    if (s == "true")             return TokenKind::KwTrue;
    if (s == "false")            return TokenKind::KwFalse;
    if (s == "alignof")          return TokenKind::KwAlignof;
    if (s == "operator")         return TokenKind::KwOperator;
    if (s == "new")              return TokenKind::KwNew;
    if (s == "delete")           return TokenKind::KwDelete;
    if (s == "explicit")         return TokenKind::KwExplicit;
    if (s == "requires")         return TokenKind::KwRequires;
  }

  return TokenKind::Identifier;
}

bool is_builtin_type_trait_name(const std::string &s) {
  return string_in_list(s, kBuiltinTypeTraitNames,
                        sizeof(kBuiltinTypeTraitNames) /
                            sizeof(kBuiltinTypeTraitNames[0]));
}

bool is_parser_supported_builtin_type_trait_name(const std::string &s) {
  if (!is_builtin_type_trait_name(s)) return false;
  if (s == "__builtin_types_compatible_p") return true;
  if (s == "__is_lvalue_expr" || s == "__is_rvalue_expr") return false;
  if (starts_with(s, "__is_") || starts_with(s, "__has_")) return true;
  return false;
}

}  // namespace c4c
