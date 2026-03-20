#pragma once

#include <cctype>
#include <cstdio>
#include <cstring>
#include <optional>
#include <string>

#include "llvm_codegen.hpp"

namespace c4c::codegen::llvm_backend::detail {

using namespace c4c;
using namespace c4c::hir;

inline bool target_triple_contains(const std::string& triple, const char* needle) {
  return !triple.empty() && triple.find(needle) != std::string::npos;
}

inline std::string& active_target_triple_storage() {
  static thread_local std::string triple;
  return triple;
}

inline void set_active_target_triple(const std::string& target_triple) {
  active_target_triple_storage() = target_triple;
}

inline const std::string& active_target_triple() {
  return active_target_triple_storage();
}

inline bool llvm_target_is_aarch64(const std::string& triple) {
  return target_triple_contains(triple, "aarch64") || target_triple_contains(triple, "arm64");
}

inline bool llvm_target_is_apple(const std::string& triple) {
  return target_triple_contains(triple, "apple") || target_triple_contains(triple, "darwin");
}

inline bool llvm_va_list_is_pointer_object(const std::string& target_triple) {
  return llvm_target_is_apple(target_triple) && llvm_target_is_aarch64(target_triple);
}

inline bool llvm_va_list_is_pointer_object() {
  return llvm_va_list_is_pointer_object(active_target_triple());
}

inline std::string llvm_va_list_storage_ty(const std::string& target_triple) {
  return llvm_va_list_is_pointer_object(target_triple) ? "ptr" : "%struct.__va_list_tag_";
}

inline std::string llvm_va_list_storage_ty() {
  return llvm_va_list_storage_ty(active_target_triple());
}

inline int llvm_va_list_storage_size(const std::string& target_triple) {
  return llvm_va_list_is_pointer_object(target_triple) ? 8 : 32;
}

inline int llvm_va_list_storage_size() {
  return llvm_va_list_storage_size(active_target_triple());
}

inline std::string llvm_default_datalayout(const std::string& target_triple) {
  if (target_triple.empty()) return "";
  if (llvm_target_is_aarch64(target_triple) && llvm_target_is_apple(target_triple))
    return "e-m:o-i64:64-i128:128-n32:64-S128-Fn32";
  if (llvm_target_is_aarch64(target_triple))
    return "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128-Fn32";
  if (target_triple_contains(target_triple, "x86_64") && llvm_target_is_apple(target_triple))
    return "e-m:o-i64:64-f80:128-n8:16:32:64-S128";
  if (target_triple_contains(target_triple, "x86_64"))
    return "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
  return "";
}

inline bool is_float_base(TypeBase b) {
  return b == TB_FLOAT || b == TB_DOUBLE || b == TB_LONGDOUBLE;
}

inline bool is_complex_base(TypeBase b) {
  switch (b) {
    case TB_COMPLEX_FLOAT:
    case TB_COMPLEX_DOUBLE:
    case TB_COMPLEX_LONGDOUBLE:
    case TB_COMPLEX_CHAR:
    case TB_COMPLEX_SCHAR:
    case TB_COMPLEX_UCHAR:
    case TB_COMPLEX_SHORT:
    case TB_COMPLEX_USHORT:
    case TB_COMPLEX_INT:
    case TB_COMPLEX_UINT:
    case TB_COMPLEX_LONG:
    case TB_COMPLEX_ULONG:
    case TB_COMPLEX_LONGLONG:
    case TB_COMPLEX_ULONGLONG:
      return true;
    default:
      return false;
  }
}

inline TypeSpec complex_component_ts(TypeBase b) {
  TypeSpec ts{};
  switch (b) {
    case TB_COMPLEX_FLOAT: ts.base = TB_FLOAT; break;
    case TB_COMPLEX_LONGDOUBLE: ts.base = TB_LONGDOUBLE; break;
    case TB_COMPLEX_DOUBLE: ts.base = TB_DOUBLE; break;
    case TB_COMPLEX_CHAR: ts.base = TB_CHAR; break;
    case TB_COMPLEX_SCHAR: ts.base = TB_SCHAR; break;
    case TB_COMPLEX_UCHAR: ts.base = TB_UCHAR; break;
    case TB_COMPLEX_SHORT: ts.base = TB_SHORT; break;
    case TB_COMPLEX_USHORT: ts.base = TB_USHORT; break;
    case TB_COMPLEX_INT: ts.base = TB_INT; break;
    case TB_COMPLEX_UINT: ts.base = TB_UINT; break;
    case TB_COMPLEX_LONG: ts.base = TB_LONG; break;
    case TB_COMPLEX_ULONG: ts.base = TB_ULONG; break;
    case TB_COMPLEX_LONGLONG: ts.base = TB_LONGLONG; break;
    case TB_COMPLEX_ULONGLONG: ts.base = TB_ULONGLONG; break;
    default: ts.base = TB_INT; break;
  }
  return ts;
}

inline std::string llvm_complex_ty(TypeBase b) {
  const TypeSpec elem_ts = complex_component_ts(b);
  std::string elem_ty;
  switch (elem_ts.base) {
    case TB_BOOL: elem_ty = "i1"; break;
    case TB_CHAR:
    case TB_SCHAR:
    case TB_UCHAR: elem_ty = "i8"; break;
    case TB_SHORT:
    case TB_USHORT: elem_ty = "i16"; break;
    case TB_INT:
    case TB_UINT: elem_ty = "i32"; break;
    case TB_LONG:
    case TB_ULONG:
    case TB_LONGLONG:
    case TB_ULONGLONG: elem_ty = "i64"; break;
    case TB_INT128:
    case TB_UINT128: elem_ty = "i128"; break;
    case TB_FLOAT: elem_ty = "float"; break;
    case TB_DOUBLE: elem_ty = "double"; break;
    case TB_LONGDOUBLE: elem_ty = "fp128"; break;
    default: elem_ty = "i32"; break;
  }
  return "{ " + elem_ty + ", " + elem_ty + " }";
}

inline std::optional<TypeSpec> type_spec_from_builtin_result_kind(
    BuiltinResultKind result_kind) {
  TypeSpec ts{};
  switch (result_kind) {
    case BuiltinResultKind::Pointer:
      ts.base = TB_VOID;
      ts.ptr_level = 1;
      return ts;
    case BuiltinResultKind::Int:
      ts.base = TB_INT;
      return ts;
    case BuiltinResultKind::UShort:
      ts.base = TB_USHORT;
      return ts;
    case BuiltinResultKind::UInt:
      ts.base = TB_UINT;
      return ts;
    case BuiltinResultKind::ULongLong:
      ts.base = TB_ULONGLONG;
      return ts;
    case BuiltinResultKind::Float:
      ts.base = TB_FLOAT;
      return ts;
    case BuiltinResultKind::Double:
      ts.base = TB_DOUBLE;
      return ts;
    case BuiltinResultKind::LongDouble:
      ts.base = TB_LONGDOUBLE;
      return ts;
    case BuiltinResultKind::ComplexFloat:
      ts.base = TB_COMPLEX_FLOAT;
      return ts;
    case BuiltinResultKind::ComplexDouble:
      ts.base = TB_COMPLEX_DOUBLE;
      return ts;
    case BuiltinResultKind::ComplexLongDouble:
      ts.base = TB_COMPLEX_LONGDOUBLE;
      return ts;
    case BuiltinResultKind::Void:
    case BuiltinResultKind::Unknown:
      return std::nullopt;
  }
  return std::nullopt;
}

inline std::optional<TypeBase> fp_base_from_builtin_result_kind(
    BuiltinResultKind result_kind) {
  switch (result_kind) {
    case BuiltinResultKind::Float:
      return TB_FLOAT;
    case BuiltinResultKind::Double:
      return TB_DOUBLE;
    case BuiltinResultKind::LongDouble:
      return TB_LONGDOUBLE;
    default:
      return std::nullopt;
  }
}

inline bool is_signed_int(TypeBase b) {
  switch (b) {
    case TB_CHAR: case TB_SCHAR: case TB_SHORT: case TB_INT:
    case TB_LONG: case TB_LONGLONG: case TB_INT128:
    case TB_ENUM:
      return true;
    default:
      return false;
  }
}

inline bool is_any_int(TypeBase b) {
  switch (b) {
    case TB_BOOL:
    case TB_CHAR: case TB_SCHAR: case TB_UCHAR:
    case TB_SHORT: case TB_USHORT:
    case TB_INT: case TB_UINT:
    case TB_LONG: case TB_ULONG:
    case TB_LONGLONG: case TB_ULONGLONG:
    case TB_INT128: case TB_UINT128:
    case TB_ENUM:
      return true;
    default:
      return false;
  }
}

inline int int_bits(TypeBase b) {
  switch (b) {
    case TB_BOOL: return 1;
    case TB_CHAR: case TB_SCHAR: case TB_UCHAR: return 8;
    case TB_SHORT: case TB_USHORT: return 16;
    case TB_INT: case TB_UINT: return 32;
    case TB_LONG: case TB_ULONG: return 64;
    case TB_LONGLONG: case TB_ULONGLONG: return 64;
    case TB_INT128: case TB_UINT128: return 128;
    default: return 32;
  }
}

inline TypeBase integer_promote(TypeBase b) {
  switch (b) {
    case TB_BOOL:
    case TB_CHAR: case TB_SCHAR: case TB_UCHAR:
    case TB_SHORT: case TB_USHORT:
      return TB_INT;
    default:
      return b;
  }
}

inline TypeBase usual_arith_conv(TypeBase a, TypeBase b) {
  a = integer_promote(a);
  b = integer_promote(b);
  if (a == b) return a;
  const int ba = int_bits(a), bb = int_bits(b);
  const bool as = is_signed_int(a), bs = is_signed_int(b);
  if (as == bs) return (ba >= bb) ? a : b;
  TypeBase u = as ? b : a;
  TypeBase s = as ? a : b;
  int bu = as ? bb : ba;
  int bbs = as ? ba : bb;
  if (bu >= bbs) return u;
  if (bbs > bu) return s;
  switch (s) {
    case TB_INT: return TB_UINT;
    case TB_LONG: return TB_ULONG;
    case TB_LONGLONG: return TB_ULONGLONG;
    case TB_INT128: return TB_UINT128;
    default: return TB_UINT;
  }
}

inline std::string llvm_base(TypeBase b) {
  switch (b) {
    case TB_VOID: return "void";
    case TB_BOOL: return "i1";
    case TB_CHAR: case TB_SCHAR: case TB_UCHAR: return "i8";
    case TB_SHORT: case TB_USHORT: return "i16";
    case TB_INT: case TB_UINT: return "i32";
    case TB_LONG: case TB_ULONG: return "i64";
    case TB_LONGLONG: case TB_ULONGLONG: return "i64";
    case TB_INT128: case TB_UINT128: return "i128";
    case TB_FLOAT: return "float";
    case TB_DOUBLE: return "double";
    case TB_LONGDOUBLE: return "fp128";
    case TB_VA_LIST: return "ptr";
    default: return "i32";
  }
}

inline std::string llvm_vector_ty(const TypeSpec& ts) {
  TypeSpec elem_ts = ts;
  elem_ts.is_vector = false;
  elem_ts.vector_lanes = 0;
  elem_ts.vector_bytes = 0;
  return "<" + std::to_string(ts.vector_lanes) + " x " + llvm_base(elem_ts.base) +
         ">";
}

inline bool is_vector_value(const TypeSpec& ts) {
  return ts.is_vector && ts.vector_lanes > 0 && ts.ptr_level == 0 &&
         ts.array_rank == 0;
}

inline std::string sanitize_llvm_ident(const std::string& raw);
inline std::string llvm_struct_type_str(const std::string& tag);

inline std::string llvm_ty(const TypeSpec& ts) {
  if (ts.ptr_level > 0 || ts.is_fn_ptr) return "ptr";
  if (ts.array_rank > 0) return "ptr";
  if (is_vector_value(ts)) return llvm_vector_ty(ts);
  if (is_complex_base(ts.base)) return llvm_complex_ty(ts.base);
  if ((ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.tag && ts.tag[0]) {
    return llvm_struct_type_str(ts.tag);
  }
  return llvm_base(ts.base);
}

// llvm_ty for types that may be references — returns ptr for T& types.
inline std::string llvm_ret_ty(const TypeSpec& ts) {
  if (ts.is_lvalue_ref || ts.is_rvalue_ref) return "ptr";
  return llvm_ty(ts);
}

inline bool has_concrete_type(const TypeSpec& ts) {
  return ts.base != TB_VOID || ts.ptr_level > 0 || ts.array_rank > 0 ||
         is_vector_value(ts);
}

// Returns true when raw contains characters that are not valid in a bare
// LLVM identifier ([a-zA-Z0-9._$]) and thus require quoting.
inline bool needs_llvm_quoting(const std::string& raw) {
  for (unsigned char c : raw) {
    if (!(std::isalnum(c) || c == '_' || c == '.' || c == '$')) return true;
  }
  return false;
}

// Produce a safe bare LLVM identifier by replacing non-[a-zA-Z0-9._$] chars
// with underscores.  Used for local variables and other contexts where the
// identifier is composed with a prefix (e.g. %lv.NAME).
inline std::string sanitize_llvm_ident(const std::string& raw) {
  std::string out;
  out.reserve(raw.size());
  for (unsigned char c : raw) {
    if (std::isalnum(c) || c == '_' || c == '.' || c == '$') {
      out.push_back(static_cast<char>(c));
    } else {
      out.push_back('_');
    }
  }
  if (out.empty() || std::isdigit(static_cast<unsigned char>(out[0]))) {
    out.insert(out.begin(), '_');
  }
  return out;
}

// Produce a valid LLVM identifier that preserves the source spelling by using
// LLVM double-quote syntax when necessary.  Intended for global symbols and
// type names where the full identifier (after sigil) is the result.
inline std::string quote_llvm_ident(const std::string& raw) {
  if (!needs_llvm_quoting(raw)) {
    if (raw.empty() || std::isdigit(static_cast<unsigned char>(raw[0]))) {
      return "_" + raw;
    }
    return raw;
  }
  std::string out = "\"";
  for (char c : raw) {
    if (c == '"')       out += "\\22";
    else if (c == '\\') out += "\\5C";
    else                out.push_back(c);
  }
  out += "\"";
  return out;
}

// Produce an LLVM struct/union type name with correct quoting.
// Returns e.g. "%struct.Foo" or "%\"struct.ns::Foo\"".
inline std::string llvm_struct_type_str(const std::string& tag) {
  if (!needs_llvm_quoting(tag)) return "%struct." + tag;
  std::string out = "%\"struct.";
  for (char c : tag) {
    if (c == '"')       out += "\\22";
    else if (c == '\\') out += "\\5C";
    else                out.push_back(c);
  }
  out += "\"";
  return out;
}

// Number of array dimensions that belong to the declarator (outer), NOT the
// pointed-to typedef array (inner).  For non-ptr-to-array types this is just
// array_rank.  For ptr-to-array types, inner_rank trailing dims are the
// pointed-to type's array, so outer = array_rank - inner_rank.
// Number of array dimensions that belong to the declarator (outer), NOT the
// pointed-to typedef array (inner).
// - is_ptr_to_array && inner_rank < 0: all dims are inner → outer = 0
// - inner_rank >= 0: inner_rank trailing dims are inner → outer = array_rank - inner_rank
// - otherwise: all dims are outer
inline int outer_array_rank(const TypeSpec& ts) {
  if (ts.is_ptr_to_array && ts.inner_rank < 0) return 0;
  if (ts.inner_rank >= 0 && ts.ptr_level > 0) {
    int outer = ts.array_rank - ts.inner_rank;
    return (outer > 0) ? outer : 0;
  }
  return ts.array_rank;
}

inline std::string llvm_alloca_ty(const TypeSpec& ts) {
  // Pure pointer-to-array with no outer array dims → just ptr
  if (ts.ptr_level > 0 && outer_array_rank(ts) == 0) return "ptr";
  if (ts.array_rank > 0) {
    if (ts.array_size >= 0) {
      TypeSpec elem = ts;
      elem.array_rank--;
      for (int i = 0; i < elem.array_rank; ++i) elem.array_dims[i] = elem.array_dims[i + 1];
      if (elem.array_rank > 0) elem.array_dims[elem.array_rank] = -1;
      elem.array_size = (elem.array_rank > 0) ? elem.array_dims[0] : -1;
      std::string elem_ty = llvm_alloca_ty(elem);
      if (elem_ty == "void") elem_ty = "i8";
      return "[" + std::to_string(ts.array_size) + " x " + elem_ty + "]";
    }
    return "ptr";
  }
  if (is_vector_value(ts)) return llvm_vector_ty(ts);
  if (ts.ptr_level > 0 || ts.is_fn_ptr) return "ptr";
  if (is_complex_base(ts.base)) return llvm_complex_ty(ts.base);
  if (ts.base == TB_VA_LIST) return llvm_va_list_storage_ty();
  if (ts.base == TB_STRUCT || ts.base == TB_UNION) {
    if (ts.tag && ts.tag[0]) return llvm_struct_type_str(ts.tag);
  }
  if (ts.base == TB_VOID) return "i8";
  return llvm_base(ts.base);
}

inline int sizeof_base(TypeBase b) {
  if (is_complex_base(b)) return 2 * sizeof_base(complex_component_ts(b).base);
  switch (b) {
    case TB_BOOL: case TB_CHAR: case TB_SCHAR: case TB_UCHAR: return 1;
    case TB_SHORT: case TB_USHORT: return 2;
    case TB_INT: case TB_UINT: case TB_FLOAT: return 4;
    case TB_LONG: case TB_ULONG: return 8;
    case TB_LONGLONG: case TB_ULONGLONG: return 8;
    case TB_DOUBLE: return 8;
    case TB_LONGDOUBLE: return 16;
    case TB_VA_LIST: return llvm_va_list_storage_size();
    default: return 4;
  }
}

inline int sizeof_ts(const Module& mod, const TypeSpec& ts) {
  if (ts.array_rank > 0) {
    if (ts.array_size == 0) return 0;  // zero-length array
    if (ts.array_size > 0) {
      TypeSpec elem = ts;
      elem.array_rank--;
      if (elem.array_rank > 0) {
        for (int i = 0; i < elem.array_rank; ++i) elem.array_dims[i] = elem.array_dims[i + 1];
      }
      elem.array_size = (elem.array_rank > 0) ? elem.array_dims[0] : -1;
      return static_cast<int>(ts.array_size * sizeof_ts(mod, elem));
    }
    return 8;
  }
  if (ts.is_vector && ts.vector_bytes > 0) return static_cast<int>(ts.vector_bytes);
  if (ts.ptr_level > 0 && ts.is_ptr_to_array) return 8;
  if (ts.ptr_level > 0 || ts.is_fn_ptr) return 8;
  if ((ts.base == TB_STRUCT || ts.base == TB_UNION) &&
      ts.ptr_level == 0 && ts.tag && ts.tag[0]) {
    const auto it = mod.struct_defs.find(ts.tag);
    if (it == mod.struct_defs.end()) return 4;
    return it->second.size_bytes;
  }
  return sizeof_base(ts.base);
}

inline std::string llvm_field_ty(const HirStructField& f) {
  if (f.array_first_dim >= 0) {
    std::string elem_ty = llvm_alloca_ty(f.elem_type);
    if (elem_ty == "void") elem_ty = "i8";
    return "[" + std::to_string(f.array_first_dim) + " x " + elem_ty + "]";
  }
  if (f.elem_type.ptr_level > 0 || f.elem_type.is_fn_ptr) return "ptr";
  if (is_complex_base(f.elem_type.base)) return llvm_complex_ty(f.elem_type.base);
  if (f.elem_type.base == TB_VA_LIST) return llvm_va_list_storage_ty();
  if (f.elem_type.base == TB_STRUCT || f.elem_type.base == TB_UNION) {
    if (f.elem_type.tag && f.elem_type.tag[0]) {
      return llvm_struct_type_str(f.elem_type.tag);
    }
  }
  return llvm_ty(f.elem_type);
}

inline std::string fp_to_hex(double v) {
  uint64_t bits;
  static_assert(sizeof(double) == sizeof(uint64_t));
  std::memcpy(&bits, &v, 8);
  char buf[32];
  std::snprintf(buf, sizeof(buf), "0x%016llX", (unsigned long long)bits);
  return buf;
}

inline std::string fp_to_float_literal(float v) {
  return fp_to_hex(static_cast<double>(v));
}

inline std::string fp_to_fp128_literal(double v) {
  uint64_t bits;
  static_assert(sizeof(double) == sizeof(uint64_t));
  std::memcpy(&bits, &v, 8);

  const uint64_t sign = (bits >> 63) & 1;
  const int64_t exp11 = static_cast<int64_t>((bits >> 52) & 0x7FF);
  const uint64_t mantissa52 = bits & 0x000F'FFFF'FFFF'FFFFULL;

  unsigned __int128 val128 = 0;
  if (exp11 == 0 && mantissa52 == 0) {
    val128 = static_cast<unsigned __int128>(sign) << 127;
  } else if (exp11 == 0x7FF) {
    const uint16_t exp15 = 0x7FFF;
    unsigned __int128 mantissa = 0;
    if (mantissa52 != 0) mantissa = static_cast<unsigned __int128>(1) << 111;
    val128 = (static_cast<unsigned __int128>(sign) << 127) |
             (static_cast<unsigned __int128>(exp15) << 112) | mantissa;
  } else {
    const uint16_t exp15 = static_cast<uint16_t>(exp11 - 1023 + 16383);
    const unsigned __int128 mantissa112 =
        static_cast<unsigned __int128>(mantissa52) << 60;
    val128 = (static_cast<unsigned __int128>(sign) << 127) |
             (static_cast<unsigned __int128>(exp15) << 112) | mantissa112;
  }

  const auto hi = static_cast<unsigned long long>(val128 >> 64);
  const auto lo = static_cast<unsigned long long>(val128);
  char buf[40];
  std::snprintf(buf, sizeof(buf), "0xL%016llX%016llX", lo, hi);
  return buf;
}

inline std::string fp_literal(TypeBase b, double v) {
  switch (b) {
    case TB_FLOAT: return fp_to_float_literal(static_cast<float>(v));
    case TB_LONGDOUBLE: return fp_to_fp128_literal(v);
    case TB_DOUBLE:
    default:
      return fp_to_hex(v);
  }
}

inline int hex_digit(unsigned char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
  if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
  return -1;
}

inline void append_utf8(std::string& out, uint32_t cp) {
  if (cp <= 0x7F) {
    out.push_back(static_cast<char>(cp));
  } else if (cp <= 0x7FF) {
    out.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  } else if (cp <= 0xFFFF) {
    out.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  } else {
    out.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  }
}

inline std::string decode_c_escaped_bytes(const std::string& raw) {
  std::string out;
  out.reserve(raw.size());
  for (size_t i = 0; i < raw.size(); ++i) {
    unsigned char c = static_cast<unsigned char>(raw[i]);
    if (c != '\\' || i + 1 >= raw.size()) {
      out.push_back(static_cast<char>(c));
      continue;
    }
    unsigned char e = static_cast<unsigned char>(raw[++i]);
    switch (e) {
      case 'n': out.push_back('\n'); break;
      case 'r': out.push_back('\r'); break;
      case 't': out.push_back('\t'); break;
      case '\\': out.push_back('\\'); break;
      case '\'': out.push_back('\''); break;
      case '"': out.push_back('"'); break;
      case 'a': out.push_back('\a'); break;
      case 'b': out.push_back('\b'); break;
      case 'f': out.push_back('\f'); break;
      case 'v': out.push_back('\v'); break;
      case 'x': {
        int v = 0;
        bool any = false;
        while (i + 1 < raw.size()) {
          int h = hex_digit(static_cast<unsigned char>(raw[i + 1]));
          if (h < 0) break;
          any = true;
          v = (v << 4) | h;
          ++i;
        }
        out.push_back(static_cast<char>(any ? (v & 0xFF) : 'x'));
        break;
      }
      case 'u':
      case 'U': {
        const int nhex = (e == 'u') ? 4 : 8;
        uint32_t cp = 0;
        bool ok = true;
        for (int k = 0; k < nhex; ++k) {
          if (i + 1 >= raw.size()) {
            ok = false;
            break;
          }
          int h = hex_digit(static_cast<unsigned char>(raw[i + 1]));
          if (h < 0) {
            ok = false;
            break;
          }
          cp = (cp << 4) | static_cast<uint32_t>(h);
          ++i;
        }
        if (ok) append_utf8(out, cp);
        break;
      }
      default:
        if (e >= '0' && e <= '7') {
          int v = e - '0';
          int count = 1;
          while (count < 3 && i + 1 < raw.size()) {
            unsigned char o = static_cast<unsigned char>(raw[i + 1]);
            if (o < '0' || o > '7') break;
            v = (v << 3) | (o - '0');
            ++i;
            ++count;
          }
          out.push_back(static_cast<char>(v & 0xFF));
        } else {
          out.push_back(static_cast<char>(e));
        }
        break;
    }
  }
  return out;
}

}  // namespace tinyc2ll::codegen::llvm_backend::detail
