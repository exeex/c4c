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
}

}  // namespace tinyc2ll::frontend_cxx
