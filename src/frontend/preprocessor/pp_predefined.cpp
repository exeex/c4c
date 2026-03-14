#include "pp_predefined.hpp"

namespace tinyc2ll::frontend_cxx {

namespace {

void def(MacroTable& t, const char* name, const char* body) {
  t[name] = MacroDef{name, false, false, {}, body};
}

}  // namespace

void init_predefined_macros(MacroTable& table) {
  def(table, "__STDC__", "1");
  def(table, "__STDC_VERSION__", "201710L");
  def(table, "__LP64__", "1");
  def(table, "_LP64", "1");

  // __COUNTER__ is handled specially in expand_text (auto-incrementing).
  // This entry ensures defined(__COUNTER__) is true and the expansion path fires.
  def(table, "__COUNTER__", "0");

  // Type size macros (LP64 / ILP32 targets: int=4, long=8, pointer=8)
  def(table, "__SIZEOF_INT__", "4");
  def(table, "__SIZEOF_SHORT__", "2");
  def(table, "__SIZEOF_LONG__", "8");
  def(table, "__SIZEOF_LONG_LONG__", "8");
  def(table, "__SIZEOF_POINTER__", "8");
  def(table, "__SIZEOF_FLOAT__", "4");
  def(table, "__SIZEOF_DOUBLE__", "8");
  def(table, "__SIZEOF_SIZE_T__", "8");

  // GCC built-in type macros (LP64 / arm64)
  def(table, "__SIZE_TYPE__", "long unsigned int");
  def(table, "__PTRDIFF_TYPE__", "long int");
  def(table, "__WCHAR_TYPE__", "int");
  def(table, "__WINT_TYPE__", "int");
  def(table, "__INTMAX_TYPE__", "long int");
  def(table, "__UINTMAX_TYPE__", "long unsigned int");
  def(table, "__INTPTR_TYPE__", "long int");
  def(table, "__UINTPTR_TYPE__", "long unsigned int");

  // Fixed-width integer type macros (LP64 / aarch64)
  def(table, "__INT8_TYPE__", "signed char");
  def(table, "__INT16_TYPE__", "short");
  def(table, "__INT32_TYPE__", "int");
  def(table, "__INT64_TYPE__", "long int");
  def(table, "__UINT8_TYPE__", "unsigned char");
  def(table, "__UINT16_TYPE__", "unsigned short");
  def(table, "__UINT32_TYPE__", "unsigned int");
  def(table, "__UINT64_TYPE__", "long unsigned int");
  def(table, "__INT_LEAST8_TYPE__", "signed char");
  def(table, "__INT_LEAST16_TYPE__", "short");
  def(table, "__INT_LEAST32_TYPE__", "int");
  def(table, "__INT_LEAST64_TYPE__", "long int");
  def(table, "__UINT_LEAST8_TYPE__", "unsigned char");
  def(table, "__UINT_LEAST16_TYPE__", "unsigned short");
  def(table, "__UINT_LEAST32_TYPE__", "unsigned int");
  def(table, "__UINT_LEAST64_TYPE__", "long unsigned int");
  def(table, "__INT_FAST8_TYPE__", "signed char");
  def(table, "__INT_FAST16_TYPE__", "short");
  def(table, "__INT_FAST32_TYPE__", "int");
  def(table, "__INT_FAST64_TYPE__", "long int");
  def(table, "__UINT_FAST8_TYPE__", "unsigned char");
  def(table, "__UINT_FAST16_TYPE__", "unsigned short");
  def(table, "__UINT_FAST32_TYPE__", "unsigned int");
  def(table, "__UINT_FAST64_TYPE__", "long unsigned int");
  def(table, "__CHAR16_TYPE__", "unsigned short");
  def(table, "__CHAR32_TYPE__", "unsigned int");

  // Integer limit macros (LP64: int=32-bit, long/long long=64-bit)
  def(table, "__CHAR_BIT__", "8");
  def(table, "__SCHAR_MAX__", "127");
  def(table, "__SHRT_MAX__", "32767");
  def(table, "__INT_MAX__", "2147483647");
  def(table, "__LONG_MAX__", "9223372036854775807L");
  def(table, "__LONG_LONG_MAX__", "9223372036854775807LL");
  def(table, "__INT8_MAX__", "127");
  def(table, "__INT16_MAX__", "32767");
  def(table, "__INT32_MAX__", "2147483647");
  def(table, "__INT64_MAX__", "9223372036854775807LL");
  def(table, "__UINT8_MAX__", "255");
  def(table, "__UINT16_MAX__", "65535");
  def(table, "__UINT32_MAX__", "4294967295U");
  def(table, "__UINT64_MAX__", "18446744073709551615ULL");
  def(table, "__INTMAX_MAX__", "9223372036854775807LL");
  def(table, "__UINTMAX_MAX__", "18446744073709551615ULL");
  def(table, "__SIZE_MAX__", "18446744073709551615ULL");
  def(table, "__INTPTR_MAX__", "9223372036854775807LL");
  def(table, "__UINTPTR_MAX__", "18446744073709551615ULL");
  def(table, "__PTRDIFF_MAX__", "9223372036854775807LL");
  def(table, "__WCHAR_MAX__", "2147483647");

  // Floating-point limit macros (IEEE 754: float=32, double=64, long double=64 on arm64)
  def(table, "__FLT_MAX__", "3.40282347e+38F");
  def(table, "__FLT_MIN__", "1.17549435e-38F");
  def(table, "__FLT_EPSILON__", "1.19209290e-07F");
  def(table, "__FLT_DIG__", "6");
  def(table, "__FLT_MANT_DIG__", "24");
  def(table, "__FLT_MAX_EXP__", "128");
  def(table, "__FLT_MIN_EXP__", "(-125)");
  def(table, "__DBL_MAX__", "1.7976931348623157e+308");
  def(table, "__DBL_MIN__", "2.2250738585072014e-308");
  def(table, "__DBL_EPSILON__", "2.2204460492503131e-16");
  def(table, "__DBL_DIG__", "15");
  def(table, "__DBL_MANT_DIG__", "53");
  def(table, "__DBL_MAX_EXP__", "1024");
  def(table, "__DBL_MIN_EXP__", "(-1021)");
  def(table, "__LDBL_MAX__", "1.7976931348623157e+308L");
  def(table, "__LDBL_MIN__", "2.2250738585072014e-308L");
  def(table, "__LDBL_EPSILON__", "2.2204460492503131e-16L");
  def(table, "__LDBL_DIG__", "15");
  def(table, "__LDBL_MANT_DIG__", "53");
  def(table, "__LDBL_MAX_EXP__", "1024");
  def(table, "__LDBL_MIN_EXP__", "(-1021)");

  // Byte-order macros (aarch64 / little-endian)
  def(table, "__ORDER_LITTLE_ENDIAN__", "1234");
  def(table, "__ORDER_BIG_ENDIAN__", "4321");
  def(table, "__ORDER_PDP_ENDIAN__", "3412");
  def(table, "__BYTE_ORDER__", "1234");

  // GCC compatibility macros
  def(table, "__GNUC__", "4");
  def(table, "__GNUC_MINOR__", "2");
  def(table, "__GNUC_PATCHLEVEL__", "1");
  def(table, "__VERSION__", "\"4.2.1 Compatible c4c\"");
  def(table, "__STDC_HOSTED__", "1");

  // Target architecture macros (host-detected at compile time)
#if defined(__aarch64__) || defined(_M_ARM64)
  def(table, "__aarch64__", "1");
  def(table, "__AARCH64EL__", "1");
  def(table, "__AARCH64_CMODEL_SMALL__", "1");
  def(table, "__ARM_64BIT_STATE", "1");
  def(table, "__ARM_ARCH", "8");
  def(table, "__ARM_ARCH_ISA_A64", "1");
  def(table, "__ARM_ARCH_PROFILE", "'A'");
  def(table, "__ARM_ACLE", "200");
  def(table, "__ARM_ALIGN_MAX_STACK_PWR", "4");
  def(table, "__ARM_FEATURE_CLZ", "1");
  def(table, "__ARM_FEATURE_DIRECTED_ROUNDING", "1");
  def(table, "__ARM_FEATURE_DIV", "1");
  def(table, "__ARM_FEATURE_FMA", "1");
  def(table, "__ARM_FEATURE_IDIV", "1");
  def(table, "__ARM_FEATURE_LDREX", "0xF");
  def(table, "__ARM_FEATURE_NUMERIC_MAXMIN", "1");
  def(table, "__ARM_FEATURE_UNALIGNED", "1");
  def(table, "__ARM_FP", "0xE");
  def(table, "__ARM_FP16_ARGS", "1");
  def(table, "__ARM_FP16_FORMAT_IEEE", "1");
  def(table, "__ARM_NEON", "1");
  def(table, "__ARM_NEON_FP", "0xE");
  def(table, "__ARM_PCS_AAPCS64", "1");
  def(table, "__ARM_SIZEOF_MINIMAL_ENUM", "4");
  def(table, "__ARM_SIZEOF_WCHAR_T", "4");
#elif defined(__x86_64__) || defined(_M_X64)
  def(table, "__x86_64__", "1");
  def(table, "__x86_64", "1");
  def(table, "__amd64__", "1");
  def(table, "__amd64", "1");
#elif defined(__i386__) || defined(_M_IX86)
  def(table, "__i386__", "1");
  def(table, "__i386", "1");
#elif defined(__riscv)
  def(table, "__riscv", "1");
  def(table, "__riscv_xlen", "64");
#endif

  // OS macros
#if defined(__linux__)
  def(table, "__linux__", "1");
  def(table, "__linux", "1");
  def(table, "linux", "1");
  def(table, "__gnu_linux__", "1");
#endif
#if defined(__unix__) || defined(__unix)
  def(table, "__unix__", "1");
  def(table, "__unix", "1");
  def(table, "unix", "1");
#endif
#if defined(__APPLE__)
  def(table, "__APPLE__", "1");
  def(table, "__MACH__", "1");
#endif
#if defined(_WIN32)
  def(table, "_WIN32", "1");
#endif
#if defined(_WIN64)
  def(table, "_WIN64", "1");
#endif

  // ELF target (common on Linux)
#if defined(__ELF__)
  def(table, "__ELF__", "1");
#endif

  // Atomic memory order constants (used by <stdatomic.h> and __atomic builtins)
  def(table, "__ATOMIC_RELAXED", "0");
  def(table, "__ATOMIC_CONSUME", "1");
  def(table, "__ATOMIC_ACQUIRE", "2");
  def(table, "__ATOMIC_RELEASE", "3");
  def(table, "__ATOMIC_ACQ_REL", "4");
  def(table, "__ATOMIC_SEQ_CST", "5");

  // GCC sync builtins availability (arch-dependent)
#if defined(__aarch64__) || defined(_M_ARM64) || defined(__x86_64__) || defined(_M_X64)
  def(table, "__GCC_HAVE_SYNC_COMPARE_AND_SWAP_1", "1");
  def(table, "__GCC_HAVE_SYNC_COMPARE_AND_SWAP_2", "1");
  def(table, "__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4", "1");
  def(table, "__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8", "1");
  def(table, "__GCC_HAVE_SYNC_COMPARE_AND_SWAP_16", "1");
#elif defined(__i386__) || defined(_M_IX86)
  def(table, "__GCC_HAVE_SYNC_COMPARE_AND_SWAP_1", "1");
  def(table, "__GCC_HAVE_SYNC_COMPARE_AND_SWAP_2", "1");
  def(table, "__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4", "1");
  def(table, "__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8", "1");
#endif

  // Common GCC/Clang compatibility macros
  def(table, "__BIGGEST_ALIGNMENT__", "16");
  def(table, "__FINITE_MATH_ONLY__", "0");
  def(table, "__NO_INLINE__", "1");
  def(table, "__GNUC_STDC_INLINE__", "1");
  def(table, "__PRAGMA_REDEFINE_EXTNAME", "1");
  def(table, "__STDC_UTF_16__", "1");
  def(table, "__STDC_UTF_32__", "1");
  def(table, "__USER_LABEL_PREFIX__", "");
  def(table, "__GCC_ASM_FLAG_OUTPUTS__", "1");
}

}  // namespace tinyc2ll::frontend_cxx
