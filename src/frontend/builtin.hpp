#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace tinyc2ll::frontend_cxx {

enum class BuiltinId : uint16_t {
  Unknown = 0,
#define TINYC2LL_BUILTIN_LIST(X)                                                \
  X(VaArg, "__builtin_va_arg")                                                  \
  X(Offsetof, "__builtin_offsetof")                                             \
  X(TypesCompatibleP, "__builtin_types_compatible_p")                           \
  X(Abort, "__builtin_abort")                                                   \
  X(Exit, "__builtin_exit")                                                     \
  X(Printf, "__builtin_printf")                                                 \
  X(PrintfUnlocked, "__builtin_printf_unlocked")                                \
  X(Fprintf, "__builtin_fprintf")                                               \
  X(FprintfUnlocked, "__builtin_fprintf_unlocked")                              \
  X(Sprintf, "__builtin_sprintf")                                               \
  X(Snprintf, "__builtin_snprintf")                                             \
  X(Puts, "__builtin_puts")                                                     \
  X(Putchar, "__builtin_putchar")                                               \
  X(Malloc, "__builtin_malloc")                                                 \
  X(Calloc, "__builtin_calloc")                                                 \
  X(Realloc, "__builtin_realloc")                                               \
  X(Free, "__builtin_free")                                                     \
  X(Memcpy, "__builtin_memcpy")                                                 \
  X(Memmove, "__builtin_memmove")                                               \
  X(Memset, "__builtin_memset")                                                 \
  X(Memcmp, "__builtin_memcmp")                                                 \
  X(Memchr, "__builtin_memchr")                                                 \
  X(Strcpy, "__builtin_strcpy")                                                 \
  X(Strncpy, "__builtin_strncpy")                                               \
  X(Strcat, "__builtin_strcat")                                                 \
  X(Strncat, "__builtin_strncat")                                               \
  X(Strcmp, "__builtin_strcmp")                                                 \
  X(Strncmp, "__builtin_strncmp")                                               \
  X(Strlen, "__builtin_strlen")                                                 \
  X(Strchr, "__builtin_strchr")                                                 \
  X(Strstr, "__builtin_strstr")                                                 \
  X(Expect, "__builtin_expect")                                                 \
  X(Unreachable, "__builtin_unreachable")                                       \
  X(ConstantP, "__builtin_constant_p")                                          \
  X(HugeVal, "__builtin_huge_val")                                              \
  X(HugeValF, "__builtin_huge_valf")                                            \
  X(HugeValL, "__builtin_huge_vall")                                            \
  X(Inf, "__builtin_inf")                                                       \
  X(InfF, "__builtin_inff")                                                     \
  X(InfL, "__builtin_infl")                                                     \
  X(Nan, "__builtin_nan")                                                       \
  X(NanF, "__builtin_nanf")                                                     \
  X(NanL, "__builtin_nanl")                                                     \
  X(Nans, "__builtin_nans")                                                     \
  X(NansF, "__builtin_nansf")                                                   \
  X(NansL, "__builtin_nansl")                                                   \
  X(VaStart, "__builtin_va_start")                                              \
  X(VaEnd, "__builtin_va_end")                                                  \
  X(VaCopy, "__builtin_va_copy")                                                \
  X(Alloca, "__builtin_alloca")                                                 \
  X(Bswap16, "__builtin_bswap16")                                               \
  X(Bswap32, "__builtin_bswap32")                                               \
  X(Bswap64, "__builtin_bswap64")                                               \
  X(Fabs, "__builtin_fabs")                                                     \
  X(FabsF, "__builtin_fabsf")                                                   \
  X(FabsL, "__builtin_fabsl")                                                   \
  X(Copysign, "__builtin_copysign")                                             \
  X(CopysignF, "__builtin_copysignf")                                           \
  X(CopysignL, "__builtin_copysignl")                                           \
  X(IsGreater, "__builtin_isgreater")                                           \
  X(IsGreaterEqual, "__builtin_isgreaterequal")                                 \
  X(IsLess, "__builtin_isless")                                                 \
  X(IsLessEqual, "__builtin_islessequal")                                       \
  X(IsLessGreater, "__builtin_islessgreater")                                   \
  X(IsUnordered, "__builtin_isunordered")                                       \
  X(IsNan, "__builtin_isnan")                                                   \
  X(IsNanF, "__builtin_isnanf")                                                 \
  X(IsNanL, "__builtin_isnanl")                                                 \
  X(IsInf, "__builtin_isinf")                                                   \
  X(IsInfF, "__builtin_isinff")                                                 \
  X(IsInfL, "__builtin_isinfl")                                                 \
  X(IsFinite, "__builtin_isfinite")                                             \
  X(IsFiniteF, "__builtin_isfinitef")                                           \
  X(IsFiniteL, "__builtin_isfinitel")                                           \
  X(Finite, "__builtin_finite")                                                 \
  X(FiniteF, "__builtin_finitef")                                               \
  X(FiniteL, "__builtin_finitel")                                               \
  X(IsInfSign, "__builtin_isinf_sign")                                          \
  X(SignBit, "__builtin_signbit")                                               \
  X(SignBitF, "__builtin_signbitf")                                             \
  X(SignBitL, "__builtin_signbitl")                                             \
  X(Ffs, "__builtin_ffs")                                                       \
  X(FfsL, "__builtin_ffsl")                                                     \
  X(FfsLL, "__builtin_ffsll")                                                   \
  X(Clz, "__builtin_clz")                                                       \
  X(ClzL, "__builtin_clzl")                                                     \
  X(ClzLL, "__builtin_clzll")                                                   \
  X(Ctz, "__builtin_ctz")                                                       \
  X(CtzL, "__builtin_ctzl")                                                     \
  X(CtzLL, "__builtin_ctzll")                                                   \
  X(Popcount, "__builtin_popcount")                                             \
  X(PopcountL, "__builtin_popcountl")                                           \
  X(PopcountLL, "__builtin_popcountll")                                         \
  X(Parity, "__builtin_parity")                                                 \
  X(ParityL, "__builtin_parityl")                                               \
  X(ParityLL, "__builtin_parityll")                                             \
  X(AddOverflow, "__builtin_add_overflow")                                      \
  X(SubOverflow, "__builtin_sub_overflow")                                      \
  X(MulOverflow, "__builtin_mul_overflow")                                      \
  X(Conj, "__builtin_conj")                                                     \
  X(ConjF, "__builtin_conjf")                                                   \
  X(ConjL, "__builtin_conjl")                                                   \
  X(ClassifyType, "__builtin_classify_type")
#define TINYC2LL_BUILTIN_ID_ENUM(id, spelling) id,
  TINYC2LL_BUILTIN_LIST(TINYC2LL_BUILTIN_ID_ENUM)
#undef TINYC2LL_BUILTIN_ID_ENUM
};

enum class BuiltinCategory : uint8_t {
  Unknown = 0,
  ParserSpecial,
  AliasCall,
  ConstantExpr,
  Identity,
  Intrinsic,
  Unsupported,
};

enum class BuiltinNodeKind : uint8_t {
  None = 0,
  DedicatedNode,
  BuiltinCall,
  CanonicalCall,
};

enum class BuiltinLoweringKind : uint8_t {
  None = 0,
  AliasCall,
  ConstantFold,
  Identity,
  Control,
  MemoryIntrinsic,
  VaIntrinsic,
  BitIntrinsic,
  MathIntrinsic,
  FpCompare,
  OverflowIntrinsic,
  ComplexIntrinsic,
  TypeQuery,
};

struct BuiltinInfo {
  BuiltinId id = BuiltinId::Unknown;
  std::string_view spelling{};
  BuiltinCategory category = BuiltinCategory::Unknown;
  BuiltinNodeKind node_kind = BuiltinNodeKind::None;
  BuiltinLoweringKind lowering = BuiltinLoweringKind::None;
  std::string_view canonical_name{};
};

inline constexpr BuiltinInfo kBuiltinTable[] = {
    {BuiltinId::VaArg, "__builtin_va_arg", BuiltinCategory::ParserSpecial,
     BuiltinNodeKind::DedicatedNode, BuiltinLoweringKind::VaIntrinsic, {}},
    {BuiltinId::Offsetof, "__builtin_offsetof", BuiltinCategory::ParserSpecial,
     BuiltinNodeKind::DedicatedNode, BuiltinLoweringKind::ConstantFold, {}},
    {BuiltinId::TypesCompatibleP, "__builtin_types_compatible_p",
     BuiltinCategory::ParserSpecial, BuiltinNodeKind::DedicatedNode,
     BuiltinLoweringKind::TypeQuery, {}},

    {BuiltinId::Abort, "__builtin_abort", BuiltinCategory::AliasCall,
     BuiltinNodeKind::CanonicalCall, BuiltinLoweringKind::AliasCall, "abort"},
    {BuiltinId::Exit, "__builtin_exit", BuiltinCategory::AliasCall,
     BuiltinNodeKind::CanonicalCall, BuiltinLoweringKind::AliasCall, "exit"},
    {BuiltinId::Printf, "__builtin_printf", BuiltinCategory::AliasCall,
     BuiltinNodeKind::CanonicalCall, BuiltinLoweringKind::AliasCall, "printf"},
    {BuiltinId::PrintfUnlocked, "__builtin_printf_unlocked",
     BuiltinCategory::AliasCall, BuiltinNodeKind::CanonicalCall,
     BuiltinLoweringKind::AliasCall, "printf_unlocked"},
    {BuiltinId::Fprintf, "__builtin_fprintf", BuiltinCategory::AliasCall,
     BuiltinNodeKind::CanonicalCall, BuiltinLoweringKind::AliasCall, "fprintf"},
    {BuiltinId::FprintfUnlocked, "__builtin_fprintf_unlocked",
     BuiltinCategory::AliasCall, BuiltinNodeKind::CanonicalCall,
     BuiltinLoweringKind::AliasCall, "fprintf_unlocked"},
    {BuiltinId::Sprintf, "__builtin_sprintf", BuiltinCategory::AliasCall,
     BuiltinNodeKind::CanonicalCall, BuiltinLoweringKind::AliasCall, "sprintf"},
    {BuiltinId::Snprintf, "__builtin_snprintf", BuiltinCategory::AliasCall,
     BuiltinNodeKind::CanonicalCall, BuiltinLoweringKind::AliasCall, "snprintf"},
    {BuiltinId::Puts, "__builtin_puts", BuiltinCategory::AliasCall,
     BuiltinNodeKind::CanonicalCall, BuiltinLoweringKind::AliasCall, "puts"},
    {BuiltinId::Putchar, "__builtin_putchar", BuiltinCategory::AliasCall,
     BuiltinNodeKind::CanonicalCall, BuiltinLoweringKind::AliasCall, "putchar"},
    {BuiltinId::Malloc, "__builtin_malloc", BuiltinCategory::AliasCall,
     BuiltinNodeKind::CanonicalCall, BuiltinLoweringKind::AliasCall, "malloc"},
    {BuiltinId::Calloc, "__builtin_calloc", BuiltinCategory::AliasCall,
     BuiltinNodeKind::CanonicalCall, BuiltinLoweringKind::AliasCall, "calloc"},
    {BuiltinId::Realloc, "__builtin_realloc", BuiltinCategory::AliasCall,
     BuiltinNodeKind::CanonicalCall, BuiltinLoweringKind::AliasCall, "realloc"},
    {BuiltinId::Free, "__builtin_free", BuiltinCategory::AliasCall,
     BuiltinNodeKind::CanonicalCall, BuiltinLoweringKind::AliasCall, "free"},
    {BuiltinId::Memcpy, "__builtin_memcpy", BuiltinCategory::AliasCall,
     BuiltinNodeKind::CanonicalCall, BuiltinLoweringKind::AliasCall, "memcpy"},
    {BuiltinId::Memmove, "__builtin_memmove", BuiltinCategory::AliasCall,
     BuiltinNodeKind::CanonicalCall, BuiltinLoweringKind::AliasCall, "memmove"},
    {BuiltinId::Memset, "__builtin_memset", BuiltinCategory::AliasCall,
     BuiltinNodeKind::CanonicalCall, BuiltinLoweringKind::AliasCall, "memset"},
    {BuiltinId::Memcmp, "__builtin_memcmp", BuiltinCategory::AliasCall,
     BuiltinNodeKind::CanonicalCall, BuiltinLoweringKind::AliasCall, "memcmp"},
    {BuiltinId::Memchr, "__builtin_memchr", BuiltinCategory::AliasCall,
     BuiltinNodeKind::CanonicalCall, BuiltinLoweringKind::AliasCall, "memchr"},
    {BuiltinId::Strcpy, "__builtin_strcpy", BuiltinCategory::AliasCall,
     BuiltinNodeKind::CanonicalCall, BuiltinLoweringKind::AliasCall, "strcpy"},
    {BuiltinId::Strncpy, "__builtin_strncpy", BuiltinCategory::AliasCall,
     BuiltinNodeKind::CanonicalCall, BuiltinLoweringKind::AliasCall, "strncpy"},
    {BuiltinId::Strcat, "__builtin_strcat", BuiltinCategory::AliasCall,
     BuiltinNodeKind::CanonicalCall, BuiltinLoweringKind::AliasCall, "strcat"},
    {BuiltinId::Strncat, "__builtin_strncat", BuiltinCategory::AliasCall,
     BuiltinNodeKind::CanonicalCall, BuiltinLoweringKind::AliasCall, "strncat"},
    {BuiltinId::Strcmp, "__builtin_strcmp", BuiltinCategory::AliasCall,
     BuiltinNodeKind::CanonicalCall, BuiltinLoweringKind::AliasCall, "strcmp"},
    {BuiltinId::Strncmp, "__builtin_strncmp", BuiltinCategory::AliasCall,
     BuiltinNodeKind::CanonicalCall, BuiltinLoweringKind::AliasCall, "strncmp"},
    {BuiltinId::Strlen, "__builtin_strlen", BuiltinCategory::AliasCall,
     BuiltinNodeKind::CanonicalCall, BuiltinLoweringKind::AliasCall, "strlen"},
    {BuiltinId::Strchr, "__builtin_strchr", BuiltinCategory::AliasCall,
     BuiltinNodeKind::CanonicalCall, BuiltinLoweringKind::AliasCall, "strchr"},
    {BuiltinId::Strstr, "__builtin_strstr", BuiltinCategory::AliasCall,
     BuiltinNodeKind::CanonicalCall, BuiltinLoweringKind::AliasCall, "strstr"},

    {BuiltinId::Expect, "__builtin_expect", BuiltinCategory::Identity,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::Identity, {}},
    {BuiltinId::Unreachable, "__builtin_unreachable",
     BuiltinCategory::Intrinsic, BuiltinNodeKind::BuiltinCall,
     BuiltinLoweringKind::Control, {}},
    {BuiltinId::ConstantP, "__builtin_constant_p", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::TypeQuery, {}},

    {BuiltinId::HugeVal, "__builtin_huge_val", BuiltinCategory::ConstantExpr,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::ConstantFold, {}},
    {BuiltinId::HugeValF, "__builtin_huge_valf", BuiltinCategory::ConstantExpr,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::ConstantFold, {}},
    {BuiltinId::HugeValL, "__builtin_huge_vall", BuiltinCategory::ConstantExpr,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::ConstantFold, {}},
    {BuiltinId::Inf, "__builtin_inf", BuiltinCategory::ConstantExpr,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::ConstantFold, {}},
    {BuiltinId::InfF, "__builtin_inff", BuiltinCategory::ConstantExpr,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::ConstantFold, {}},
    {BuiltinId::InfL, "__builtin_infl", BuiltinCategory::ConstantExpr,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::ConstantFold, {}},
    {BuiltinId::Nan, "__builtin_nan", BuiltinCategory::ConstantExpr,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::ConstantFold, {}},
    {BuiltinId::NanF, "__builtin_nanf", BuiltinCategory::ConstantExpr,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::ConstantFold, {}},
    {BuiltinId::NanL, "__builtin_nanl", BuiltinCategory::ConstantExpr,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::ConstantFold, {}},
    {BuiltinId::Nans, "__builtin_nans", BuiltinCategory::ConstantExpr,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::ConstantFold, {}},
    {BuiltinId::NansF, "__builtin_nansf", BuiltinCategory::ConstantExpr,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::ConstantFold, {}},
    {BuiltinId::NansL, "__builtin_nansl", BuiltinCategory::ConstantExpr,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::ConstantFold, {}},

    {BuiltinId::VaStart, "__builtin_va_start", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::VaIntrinsic, {}},
    {BuiltinId::VaEnd, "__builtin_va_end", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::VaIntrinsic, {}},
    {BuiltinId::VaCopy, "__builtin_va_copy", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::VaIntrinsic, {}},
    {BuiltinId::Alloca, "__builtin_alloca", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::MemoryIntrinsic, {}},

    {BuiltinId::Bswap16, "__builtin_bswap16", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::BitIntrinsic, {}},
    {BuiltinId::Bswap32, "__builtin_bswap32", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::BitIntrinsic, {}},
    {BuiltinId::Bswap64, "__builtin_bswap64", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::BitIntrinsic, {}},
    {BuiltinId::Fabs, "__builtin_fabs", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::MathIntrinsic, {}},
    {BuiltinId::FabsF, "__builtin_fabsf", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::MathIntrinsic, {}},
    {BuiltinId::FabsL, "__builtin_fabsl", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::MathIntrinsic, {}},
    {BuiltinId::Copysign, "__builtin_copysign", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::MathIntrinsic, {}},
    {BuiltinId::CopysignF, "__builtin_copysignf", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::MathIntrinsic, {}},
    {BuiltinId::CopysignL, "__builtin_copysignl", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::MathIntrinsic, {}},

    {BuiltinId::IsGreater, "__builtin_isgreater", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::FpCompare, {}},
    {BuiltinId::IsGreaterEqual, "__builtin_isgreaterequal",
     BuiltinCategory::Intrinsic, BuiltinNodeKind::BuiltinCall,
     BuiltinLoweringKind::FpCompare, {}},
    {BuiltinId::IsLess, "__builtin_isless", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::FpCompare, {}},
    {BuiltinId::IsLessEqual, "__builtin_islessequal",
     BuiltinCategory::Intrinsic, BuiltinNodeKind::BuiltinCall,
     BuiltinLoweringKind::FpCompare, {}},
    {BuiltinId::IsLessGreater, "__builtin_islessgreater",
     BuiltinCategory::Intrinsic, BuiltinNodeKind::BuiltinCall,
     BuiltinLoweringKind::FpCompare, {}},
    {BuiltinId::IsUnordered, "__builtin_isunordered",
     BuiltinCategory::Intrinsic, BuiltinNodeKind::BuiltinCall,
     BuiltinLoweringKind::FpCompare, {}},
    {BuiltinId::IsNan, "__builtin_isnan", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::FpCompare, {}},
    {BuiltinId::IsNanF, "__builtin_isnanf", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::FpCompare, {}},
    {BuiltinId::IsNanL, "__builtin_isnanl", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::FpCompare, {}},
    {BuiltinId::IsInf, "__builtin_isinf", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::FpCompare, {}},
    {BuiltinId::IsInfF, "__builtin_isinff", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::FpCompare, {}},
    {BuiltinId::IsInfL, "__builtin_isinfl", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::FpCompare, {}},
    {BuiltinId::IsFinite, "__builtin_isfinite", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::FpCompare, {}},
    {BuiltinId::IsFiniteF, "__builtin_isfinitef", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::FpCompare, {}},
    {BuiltinId::IsFiniteL, "__builtin_isfinitel", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::FpCompare, {}},
    {BuiltinId::Finite, "__builtin_finite", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::FpCompare, {}},
    {BuiltinId::FiniteF, "__builtin_finitef", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::FpCompare, {}},
    {BuiltinId::FiniteL, "__builtin_finitel", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::FpCompare, {}},
    {BuiltinId::IsInfSign, "__builtin_isinf_sign",
     BuiltinCategory::Intrinsic, BuiltinNodeKind::BuiltinCall,
     BuiltinLoweringKind::FpCompare, {}},
    {BuiltinId::SignBit, "__builtin_signbit", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::FpCompare, {}},
    {BuiltinId::SignBitF, "__builtin_signbitf", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::FpCompare, {}},
    {BuiltinId::SignBitL, "__builtin_signbitl", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::FpCompare, {}},

    {BuiltinId::Ffs, "__builtin_ffs", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::BitIntrinsic, {}},
    {BuiltinId::FfsL, "__builtin_ffsl", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::BitIntrinsic, {}},
    {BuiltinId::FfsLL, "__builtin_ffsll", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::BitIntrinsic, {}},
    {BuiltinId::Clz, "__builtin_clz", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::BitIntrinsic, {}},
    {BuiltinId::ClzL, "__builtin_clzl", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::BitIntrinsic, {}},
    {BuiltinId::ClzLL, "__builtin_clzll", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::BitIntrinsic, {}},
    {BuiltinId::Ctz, "__builtin_ctz", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::BitIntrinsic, {}},
    {BuiltinId::CtzL, "__builtin_ctzl", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::BitIntrinsic, {}},
    {BuiltinId::CtzLL, "__builtin_ctzll", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::BitIntrinsic, {}},
    {BuiltinId::Popcount, "__builtin_popcount", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::BitIntrinsic, {}},
    {BuiltinId::PopcountL, "__builtin_popcountl", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::BitIntrinsic, {}},
    {BuiltinId::PopcountLL, "__builtin_popcountll",
     BuiltinCategory::Intrinsic, BuiltinNodeKind::BuiltinCall,
     BuiltinLoweringKind::BitIntrinsic, {}},
    {BuiltinId::Parity, "__builtin_parity", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::BitIntrinsic, {}},
    {BuiltinId::ParityL, "__builtin_parityl", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::BitIntrinsic, {}},
    {BuiltinId::ParityLL, "__builtin_parityll", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::BitIntrinsic, {}},

    {BuiltinId::AddOverflow, "__builtin_add_overflow",
     BuiltinCategory::Intrinsic, BuiltinNodeKind::BuiltinCall,
     BuiltinLoweringKind::OverflowIntrinsic, {}},
    {BuiltinId::SubOverflow, "__builtin_sub_overflow",
     BuiltinCategory::Intrinsic, BuiltinNodeKind::BuiltinCall,
     BuiltinLoweringKind::OverflowIntrinsic, {}},
    {BuiltinId::MulOverflow, "__builtin_mul_overflow",
     BuiltinCategory::Intrinsic, BuiltinNodeKind::BuiltinCall,
     BuiltinLoweringKind::OverflowIntrinsic, {}},

    {BuiltinId::Conj, "__builtin_conj", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::ComplexIntrinsic, {}},
    {BuiltinId::ConjF, "__builtin_conjf", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::ComplexIntrinsic, {}},
    {BuiltinId::ConjL, "__builtin_conjl", BuiltinCategory::Intrinsic,
     BuiltinNodeKind::BuiltinCall, BuiltinLoweringKind::ComplexIntrinsic, {}},
    {BuiltinId::ClassifyType, "__builtin_classify_type",
     BuiltinCategory::Intrinsic, BuiltinNodeKind::BuiltinCall,
     BuiltinLoweringKind::TypeQuery, {}},
};

inline constexpr size_t builtin_count() {
  return sizeof(kBuiltinTable) / sizeof(kBuiltinTable[0]);
}

inline constexpr const BuiltinInfo* builtin_by_id(BuiltinId id) {
  for (size_t i = 0; i < builtin_count(); ++i) {
    if (kBuiltinTable[i].id == id) return &kBuiltinTable[i];
  }
  return nullptr;
}

inline constexpr const BuiltinInfo* builtin_by_name(std::string_view spelling) {
  for (size_t i = 0; i < builtin_count(); ++i) {
    if (kBuiltinTable[i].spelling == spelling) return &kBuiltinTable[i];
  }
  return nullptr;
}

inline constexpr BuiltinId builtin_id_from_name(std::string_view spelling) {
  const BuiltinInfo* info = builtin_by_name(spelling);
  return info ? info->id : BuiltinId::Unknown;
}

inline constexpr bool has_builtin_prefix(std::string_view spelling) {
  return spelling.size() >= 10 && spelling.substr(0, 10) == "__builtin_";
}

inline constexpr std::string_view builtin_name_from_id(BuiltinId id) {
  const BuiltinInfo* info = builtin_by_id(id);
  return info ? info->spelling : std::string_view{};
}

inline constexpr std::string_view builtin_canonical_name(BuiltinId id) {
  const BuiltinInfo* info = builtin_by_id(id);
  return info ? info->canonical_name : std::string_view{};
}

inline constexpr bool builtin_is_known(std::string_view spelling) {
  return has_builtin_prefix(spelling) &&
         builtin_id_from_name(spelling) != BuiltinId::Unknown;
}

inline constexpr bool builtin_is_alias_call(BuiltinId id) {
  const BuiltinInfo* info = builtin_by_id(id);
  return info && info->category == BuiltinCategory::AliasCall;
}

inline constexpr bool builtin_is_parser_special(BuiltinId id) {
  const BuiltinInfo* info = builtin_by_id(id);
  return info && info->category == BuiltinCategory::ParserSpecial;
}

#undef TINYC2LL_BUILTIN_LIST

}  // namespace tinyc2ll::frontend_cxx
