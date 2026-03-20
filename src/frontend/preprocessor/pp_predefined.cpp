#include "pp_predefined.hpp"

namespace c4c {

namespace {

void def(MacroTable& t, const char* name, const char* body) {
  t[name] = MacroDef{name, false, false, {}, body};
}

bool triple_contains(const std::string& triple, const char* needle) {
  return !triple.empty() && triple.find(needle) != std::string::npos;
}

bool target_is_apple(const std::string& triple) {
  return triple_contains(triple, "apple");
}

bool target_is_darwin(const std::string& triple) {
  return target_is_apple(triple) || triple_contains(triple, "darwin");
}

bool target_is_linux(const std::string& triple) {
  return triple_contains(triple, "linux");
}

bool target_is_aarch64(const std::string& triple) {
  return triple_contains(triple, "aarch64") || triple_contains(triple, "arm64");
}

bool target_is_x86_64(const std::string& triple) {
  return triple_contains(triple, "x86_64") || triple_contains(triple, "amd64");
}

bool target_is_i386(const std::string& triple) {
  return triple_contains(triple, "i386") || triple_contains(triple, "i686") ||
         triple_contains(triple, "x86");
}

std::string default_host_target_triple() {
#if defined(__aarch64__) || defined(_M_ARM64)
#if defined(__APPLE__)
  return "aarch64-apple-darwin";
#elif defined(__linux__)
  return "aarch64-unknown-linux-gnu";
#else
  return "aarch64-unknown-unknown";
#endif
#elif defined(__x86_64__) || defined(_M_X64)
#if defined(__APPLE__)
  return "x86_64-apple-darwin";
#elif defined(__linux__)
  return "x86_64-unknown-linux-gnu";
#else
  return "x86_64-unknown-unknown";
#endif
#elif defined(__i386__) || defined(_M_IX86)
#if defined(__linux__)
  return "i386-unknown-linux-gnu";
#else
  return "i386-unknown-unknown";
#endif
#else
  return "unknown-unknown-unknown";
#endif
}

void add_apple_target_predefines(MacroTable& table, const std::string& triple) {
  if (!target_is_apple(triple)) return;
  def(table, "__APPLE__", "1");
  def(table, "__MACH__", "1");
  def(table, "__APPLE_CC__", "6000");
  def(table, "TARGET_OS_MAC", "1");
  def(table, "TARGET_OS_OSX", "1");
  def(table, "TARGET_OS_IPHONE", "0");
  def(table, "TARGET_OS_IOS", "0");
  def(table, "TARGET_OS_MACCATALYST", "0");
  def(table, "TARGET_OS_SIMULATOR", "0");
  def(table, "TARGET_OS_DRIVERKIT", "0");
  def(table, "TARGET_OS_EMBEDDED", "0");
  def(table, "TARGET_OS_TV", "0");
  def(table, "TARGET_OS_WATCH", "0");
  def(table, "TARGET_OS_VISION", "0");
  def(table, "TARGET_OS_BRIDGE", "0");
  def(table, "TARGET_OS_UNIX", "0");
  def(table, "TARGET_OS_LINUX", "0");
  def(table, "TARGET_OS_WINDOWS", "0");
  def(table, "TARGET_OS_WIN32", "0");
  def(table, "TARGET_RT_LITTLE_ENDIAN", "1");
  def(table, "TARGET_RT_BIG_ENDIAN", "0");
  def(table, "TARGET_RT_64_BIT", "1");
  def(table, "TARGET_RT_MAC_MACHO", "1");
  def(table, "TARGET_RT_MAC_CFM", "0");
  if (target_is_aarch64(triple)) {
  def(table, "TARGET_CPU_ARM64", "1");
  def(table, "TARGET_CPU_ARM", "0");
  def(table, "TARGET_CPU_X86", "0");
  def(table, "TARGET_CPU_X86_64", "0");
  } else if (target_is_x86_64(triple)) {
  def(table, "TARGET_CPU_ARM64", "0");
  def(table, "TARGET_CPU_ARM", "0");
  def(table, "TARGET_CPU_X86", "0");
  def(table, "TARGET_CPU_X86_64", "1");
  }
  def(table, "__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__", "140000");
}

}  // namespace

void init_predefined_macros(MacroTable& table, const std::string& requested_target_triple) {
  const std::string target_triple =
      requested_target_triple.empty() ? default_host_target_triple() : requested_target_triple;
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

  // Target architecture macros.
  if (target_is_aarch64(target_triple)) {
  def(table, "__aarch64__", "1");
  def(table, "__arm64__", "1");
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
  } else if (target_is_x86_64(target_triple)) {
  def(table, "__x86_64__", "1");
  def(table, "__x86_64", "1");
  def(table, "__amd64__", "1");
  def(table, "__amd64", "1");
  def(table, "__code_model_small__", "1");
  def(table, "__MMX__", "1");
  def(table, "__SSE__", "1");
  def(table, "__SSE2__", "1");
  def(table, "__SSE_MATH__", "1");
  def(table, "__SSE2_MATH__", "1");
  def(table, "__NO_MATH_INLINES", "1");
  def(table, "__SEG_FS", "1");
  def(table, "__SEG_GS", "1");
  def(table, "__GCC_HAVE_DWARF2_CFI_ASM", "1");
  def(table, "__REGISTER_PREFIX__", "");
  } else if (target_is_i386(target_triple)) {
  def(table, "__i386__", "1");
  def(table, "__i386", "1");
  def(table, "__code_model_32__", "1");
  def(table, "__NO_MATH_INLINES", "1");
  def(table, "__REGISTER_PREFIX__", "");
  } else if (triple_contains(target_triple, "riscv")) {
  def(table, "__riscv", "1");
  def(table, "__riscv_xlen", "64");
  }

  // OS macros
  if (target_is_linux(target_triple)) {
  def(table, "__linux__", "1");
  def(table, "__linux", "1");
  def(table, "linux", "1");
  def(table, "__gnu_linux__", "1");
  }
  if (target_is_linux(target_triple) || target_is_darwin(target_triple)) {
  def(table, "__unix__", "1");
  def(table, "__unix", "1");
  def(table, "unix", "1");
  }
  add_apple_target_predefines(table, target_triple);
  if (triple_contains(target_triple, "windows")) {
  def(table, "_WIN32", "1");
    if (target_is_x86_64(target_triple)) def(table, "_WIN64", "1");
  }

  // ELF target (common on Linux)
  if (target_is_linux(target_triple)) def(table, "__ELF__", "1");

  // Atomic memory order constants (used by <stdatomic.h> and __atomic builtins)
  def(table, "__ATOMIC_RELAXED", "0");
  def(table, "__ATOMIC_CONSUME", "1");
  def(table, "__ATOMIC_ACQUIRE", "2");
  def(table, "__ATOMIC_RELEASE", "3");
  def(table, "__ATOMIC_ACQ_REL", "4");
  def(table, "__ATOMIC_SEQ_CST", "5");

  // GCC sync builtins availability (arch-dependent)
  if (target_is_aarch64(target_triple) || target_is_x86_64(target_triple)) {
  def(table, "__GCC_HAVE_SYNC_COMPARE_AND_SWAP_1", "1");
  def(table, "__GCC_HAVE_SYNC_COMPARE_AND_SWAP_2", "1");
  def(table, "__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4", "1");
  def(table, "__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8", "1");
  def(table, "__GCC_HAVE_SYNC_COMPARE_AND_SWAP_16", "1");
  } else if (target_is_i386(target_triple)) {
  def(table, "__GCC_HAVE_SYNC_COMPARE_AND_SWAP_1", "1");
  def(table, "__GCC_HAVE_SYNC_COMPARE_AND_SWAP_2", "1");
  def(table, "__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4", "1");
  def(table, "__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8", "1");
  }

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

}  // namespace c4c
