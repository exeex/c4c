#include "hir_to_llvm.hpp"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstring>
#include <functional>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace tinyc2ll::codegen::llvm_backend {

// ─────────────────────────────────────────────────────────────────────────────
// HirEmitter: native HIR → LLVM IR text emitter
// ─────────────────────────────────────────────────────────────────────────────

namespace {

using namespace tinyc2ll::frontend_cxx;
using namespace tinyc2ll::frontend_cxx::sema_ir;
using namespace tinyc2ll::frontend_cxx::sema_ir::phase2::hir;

// ── Type helpers ──────────────────────────────────────────────────────────────

static bool is_float_base(TypeBase b) {
  return b == TB_FLOAT || b == TB_DOUBLE || b == TB_LONGDOUBLE;
}

static bool is_complex_base(TypeBase b) {
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

static TypeSpec complex_component_ts(TypeBase b) {
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

static std::string llvm_complex_ty(TypeBase b) {
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

static bool is_signed_int(TypeBase b) {
  switch (b) {
    case TB_CHAR: case TB_SCHAR: case TB_SHORT: case TB_INT:
    case TB_LONG: case TB_LONGLONG: case TB_INT128:
    case TB_ENUM:  // C enum underlying type is int (signed)
      return true;
    default:
      return false;
  }
}

static bool is_any_int(TypeBase b) {
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

static int int_bits(TypeBase b) {
  switch (b) {
    case TB_BOOL:                              return 1;
    case TB_CHAR: case TB_SCHAR: case TB_UCHAR: return 8;
    case TB_SHORT: case TB_USHORT:             return 16;
    case TB_INT: case TB_UINT:                 return 32;
    case TB_LONG: case TB_ULONG:               return 64;
    case TB_LONGLONG: case TB_ULONGLONG:       return 64;
    case TB_INT128: case TB_UINT128:           return 128;
    default:                                   return 32;
  }
}

// C integer promotion: types narrower than int promote to int.
static TypeBase integer_promote(TypeBase b) {
  switch (b) {
    case TB_BOOL:
    case TB_CHAR: case TB_SCHAR: case TB_UCHAR:
    case TB_SHORT: case TB_USHORT:
      return TB_INT;
    default:
      return b;
  }
}

// C "usual arithmetic conversions" for two integer types.
// Returns the common type to which both operands should be converted.
static TypeBase usual_arith_conv(TypeBase a, TypeBase b) {
  a = integer_promote(a);
  b = integer_promote(b);
  if (a == b) return a;
  const int ba = int_bits(a), bb = int_bits(b);
  const bool as = is_signed_int(a), bs = is_signed_int(b);
  // If same signedness, use the wider type
  if (as == bs) return (ba >= bb) ? a : b;
  // Different signedness: identify signed and unsigned
  TypeBase u = as ? b : a;
  TypeBase s = as ? a : b;
  int bu = as ? bb : ba;
  int bbs = as ? ba : bb;
  // If unsigned rank >= signed rank, use unsigned
  if (bu >= bbs) return u;
  // If signed can represent all values of unsigned, use signed
  if (bbs > bu) return s;
  // Otherwise, use the unsigned counterpart of the signed type
  switch (s) {
    case TB_INT:      return TB_UINT;
    case TB_LONG:     return TB_ULONG;
    case TB_LONGLONG: return TB_ULONGLONG;
    case TB_INT128:   return TB_UINT128;
    default:          return TB_UINT;
  }
}

static std::string llvm_base(TypeBase b) {
  switch (b) {
    case TB_VOID:                               return "void";
    case TB_BOOL:                               return "i1";
    case TB_CHAR: case TB_SCHAR: case TB_UCHAR: return "i8";
    case TB_SHORT: case TB_USHORT:              return "i16";
    case TB_INT: case TB_UINT:                  return "i32";
    case TB_LONG: case TB_ULONG:                return "i64";
    case TB_LONGLONG: case TB_ULONGLONG:        return "i64";
    case TB_INT128: case TB_UINT128:            return "i128";
    case TB_FLOAT:                              return "float";
    case TB_DOUBLE:                             return "double";
    case TB_LONGDOUBLE:                         return "fp128";
    case TB_VA_LIST:                            return "ptr";
    default:                                    return "i32";
  }
}

static std::string llvm_ty(const TypeSpec& ts) {
  if (ts.ptr_level > 0 || ts.is_fn_ptr) return "ptr";
  if (ts.array_rank > 0) return "ptr";  // decayed
  if (is_complex_base(ts.base)) return llvm_complex_ty(ts.base);
  if ((ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.tag && ts.tag[0]) {
    return "%struct." + std::string(ts.tag);
  }
  return llvm_base(ts.base);
}

static bool has_concrete_type(const TypeSpec& ts) {
  return ts.base != TB_VOID || ts.ptr_level > 0 || ts.array_rank > 0;
}

static std::string sanitize_llvm_ident(const std::string& raw) {
  std::string out;
  out.reserve(raw.size());
  for (unsigned char c : raw) {
    if (std::isalnum(c) || c == '_' || c == '.' || c == '$') out.push_back(static_cast<char>(c));
    else out.push_back('_');
  }
  if (out.empty() || std::isdigit(static_cast<unsigned char>(out[0]))) out.insert(out.begin(), '_');
  return out;
}

// Non-decayed LLVM type for alloca/struct field (struct/union → %struct.TAG).
// Handles multi-dimensional arrays recursively.
static std::string llvm_alloca_ty(const TypeSpec& ts) {
  // Pointer-to-array declarators (e.g. `int (*p)[4]`) carry both ptr_level and
  // array metadata in TypeSpec; storage for the object itself is still a pointer.
  if (ts.ptr_level > 0 && ts.is_ptr_to_array) return "ptr";
  if (ts.array_rank > 0) {
    if (ts.array_size >= 0) {
      // Build element type: drop one dimension and recurse.
      TypeSpec elem = ts;
      elem.array_rank--;
      // Shift remaining dims left
      for (int i = 0; i < elem.array_rank; ++i) elem.array_dims[i] = elem.array_dims[i + 1];
      if (elem.array_rank > 0) elem.array_dims[elem.array_rank] = -1;
      elem.array_size = (elem.array_rank > 0) ? elem.array_dims[0] : -1;
      std::string elem_ty = llvm_alloca_ty(elem);
      if (elem_ty == "void") elem_ty = "i8";
      return "[" + std::to_string(ts.array_size) + " x " + elem_ty + "]";
    }
    return "ptr";  // unknown size
  }
  if (ts.ptr_level > 0 || ts.is_fn_ptr) return "ptr";
  if (is_complex_base(ts.base)) return llvm_complex_ty(ts.base);
  if (ts.base == TB_VA_LIST) return "%struct.__va_list_tag_";
  if (ts.base == TB_STRUCT || ts.base == TB_UNION) {
    if (ts.tag && ts.tag[0]) return "%struct." + std::string(ts.tag);
  }
  if (ts.base == TB_VOID) return "i8";
  return llvm_base(ts.base);
}

// Forward declaration for recursive aggregate size computation.
static int compute_struct_size(const Module& mod, const std::string& tag);

static int sizeof_base(TypeBase b) {
  if (is_complex_base(b)) return 2 * sizeof_base(complex_component_ts(b).base);
  switch (b) {
    case TB_BOOL: case TB_CHAR: case TB_SCHAR: case TB_UCHAR: return 1;
    case TB_SHORT: case TB_USHORT:             return 2;
    case TB_INT: case TB_UINT: case TB_FLOAT:  return 4;
    case TB_LONG: case TB_ULONG:               return 8;
    case TB_LONGLONG: case TB_ULONGLONG:       return 8;
    case TB_DOUBLE:                            return 8;
    case TB_LONGDOUBLE:                        return 16;
    case TB_VA_LIST:                           return 32;
    default:                                   return 4;
  }
}

static int sizeof_ts(const Module& mod, const TypeSpec& ts) {
  if (ts.array_rank > 0) {
    if (ts.array_size > 0) {
      // Recursively compute element size
      TypeSpec elem = ts;
      elem.array_rank--;
      if (elem.array_rank > 0) {
        for (int i = 0; i < elem.array_rank; ++i) elem.array_dims[i] = elem.array_dims[i + 1];
      }
      elem.array_size = (elem.array_rank > 0) ? elem.array_dims[0] : -1;
      return (int)(ts.array_size * sizeof_ts(mod, elem));
    }
    return 8;  // unknown size — treat as pointer
  }
  if (ts.ptr_level > 0 && ts.is_ptr_to_array) return 8;
  if (ts.ptr_level > 0 || ts.is_fn_ptr) return 8;
  if ((ts.base == TB_STRUCT || ts.base == TB_UNION) &&
      ts.ptr_level == 0 && ts.tag && ts.tag[0]) {
    return compute_struct_size(mod, std::string(ts.tag));
  }
  return sizeof_base(ts.base);
}

// ── Struct/union type helpers ──────────────────────────────────────────────────

// LLVM type string for a struct field (non-decayed: may be array or nested struct)
static std::string llvm_field_ty(const HirStructField& f) {
  if (f.array_first_dim >= 0) {
    // Array field: [N x elem]. Keep pointer element types intact
    // (e.g. struct S *a[13] -> [13 x ptr]).
    std::string elem_ty = llvm_alloca_ty(f.elem_type);
    if (elem_ty == "void") elem_ty = "i8";
    return "[" + std::to_string(f.array_first_dim) + " x " + elem_ty + "]";
  }
  if (f.elem_type.ptr_level > 0 || f.elem_type.is_fn_ptr) return "ptr";
  if (is_complex_base(f.elem_type.base)) return llvm_complex_ty(f.elem_type.base);
  if (f.elem_type.base == TB_VA_LIST) return "%struct.__va_list_tag_";
  if (f.elem_type.base == TB_STRUCT || f.elem_type.base == TB_UNION) {
    if (f.elem_type.tag && f.elem_type.tag[0])
      return "%struct." + std::string(f.elem_type.tag);
  }
  return llvm_ty(f.elem_type);
}

// Forward declaration for recursive size computation
static int compute_struct_size(const Module& mod, const std::string& tag);

static int compute_field_size(const Module& mod, const HirStructField& f) {
  int sz = 0;
  if (f.elem_type.ptr_level > 0 || f.elem_type.is_fn_ptr) {
    sz = 8;
  } else if (f.elem_type.base == TB_STRUCT || f.elem_type.base == TB_UNION) {
    if (f.elem_type.tag && f.elem_type.tag[0])
      sz = compute_struct_size(mod, f.elem_type.tag);
    else
      sz = 4;
  } else {
    sz = sizeof_base(f.elem_type.base);
  }
  if (f.array_first_dim >= 0) return sz * (int)f.array_first_dim;
  return sz;
}

static int compute_struct_size(const Module& mod, const std::string& tag) {
  const auto it = mod.struct_defs.find(tag);
  if (it == mod.struct_defs.end()) return 4;
  const HirStructDef& sd = it->second;
  if (sd.is_union) {
    int max_sz = 0;
    for (const auto& f : sd.fields) {
      const int sz = compute_field_size(mod, f);
      if (sz > max_sz) max_sz = sz;
    }
    return max_sz;
  }
  int total = 0;
  for (const auto& f : sd.fields) total += compute_field_size(mod, f);
  return total;
}

static std::string fp_to_hex(double v) {
  uint64_t bits;
  static_assert(sizeof(double) == sizeof(uint64_t));
  std::memcpy(&bits, &v, 8);
  char buf[32];
  std::snprintf(buf, sizeof(buf), "0x%016llX", (unsigned long long)bits);
  return buf;
}

static std::string fp_to_float_literal(float v) {
  // LLVM textual IR expects float constants in a hex form.
  return fp_to_hex(static_cast<double>(v));
}

static std::string fp_to_fp128_literal(double v) {
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
             (static_cast<unsigned __int128>(exp15) << 112) |
             mantissa;
  } else {
    const uint16_t exp15 = static_cast<uint16_t>(exp11 - 1023 + 16383);
    const unsigned __int128 mantissa112 =
        static_cast<unsigned __int128>(mantissa52) << 60;
    val128 = (static_cast<unsigned __int128>(sign) << 127) |
             (static_cast<unsigned __int128>(exp15) << 112) |
             mantissa112;
  }

  const auto hi = static_cast<unsigned long long>(val128 >> 64);
  const auto lo = static_cast<unsigned long long>(val128);
  char buf[40];
  std::snprintf(buf, sizeof(buf), "0xL%016llX%016llX", lo, hi);
  return buf;
}

static std::string fp_literal(TypeBase b, double v) {
  switch (b) {
    case TB_FLOAT: return fp_to_float_literal(static_cast<float>(v));
    case TB_LONGDOUBLE: return fp_to_fp128_literal(v);
    case TB_DOUBLE:
    default:
      return fp_to_hex(v);
  }
}

static int hex_digit(unsigned char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
  if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
  return -1;
}

static void append_utf8(std::string& out, uint32_t cp) {
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

static std::string decode_c_escaped_bytes(const std::string& raw) {
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

// ── Block metadata for break/continue targets ─────────────────────────────────

struct BlockMeta {
  std::optional<std::string> break_label;
  std::optional<std::string> continue_label;
};

// Bitfield access metadata returned by emit_member_gep/emit_member_lval.
struct BitfieldAccess {
  int bit_width = -1;       // -1 = not a bitfield
  int bit_offset = 0;
  int storage_unit_bits = 0;
  bool is_signed = false;
  bool is_bitfield() const { return bit_width >= 0; }
};

// ── HirEmitter class ──────────────────────────────────────────────────────────

class HirEmitter {
 public:
  explicit HirEmitter(const Module& m) : mod_(m) {}

  std::string emit() {
    emit_preamble();
    // Deduplicate globals: C tentative definitions can produce multiple entries
    // for the same name. Prefer the entry with an explicit initializer; among
    // equals, prefer later entries (last wins for extern semantics).
    std::unordered_map<std::string, size_t> best_gv; // name -> index in mod_.globals
    for (size_t i = 0; i < mod_.globals.size(); ++i) {
      const GlobalVar& gv = mod_.globals[i];
      auto it = best_gv.find(gv.name);
      if (it == best_gv.end()) {
        best_gv[gv.name] = i;
      } else {
        // Prefer explicitly initialized over tentative (monostate)
        const bool cur_has_init = !std::holds_alternative<std::monostate>(mod_.globals[it->second].init);
        const bool new_has_init = !std::holds_alternative<std::monostate>(gv.init);
        if (new_has_init || !cur_has_init) it->second = i;
      }
    }
    // Emit in original order, skipping non-best duplicates
    for (size_t i = 0; i < mod_.globals.size(); ++i) {
      const GlobalVar& gv = mod_.globals[i];
      if (best_gv.count(gv.name) && best_gv[gv.name] == i) emit_global(gv);
    }
    // Deduplicate functions: prefer definitions (non-empty blocks) over declarations.
    std::unordered_map<std::string, size_t> best_fn; // name -> index
    for (size_t i = 0; i < mod_.functions.size(); ++i) {
      const Function& fn = mod_.functions[i];
      auto it = best_fn.find(fn.name);
      if (it == best_fn.end()) {
        best_fn[fn.name] = i;
      } else {
        const bool cur_is_def = !mod_.functions[it->second].blocks.empty();
        const bool new_is_def = !fn.blocks.empty();
        if (new_is_def && !cur_is_def) it->second = i;
      }
    }
    // Emit in original order, skipping non-best duplicates
    for (size_t i = 0; i < mod_.functions.size(); ++i) {
      const Function& fn = mod_.functions[i];
      if (best_fn.count(fn.name) && best_fn[fn.name] == i) emit_function(fn);
    }

    std::ostringstream out;
    out << preamble_.str();
    if (!preamble_.str().empty()) out << "\n";
    if (need_llvm_va_start_) out << "declare void @llvm.va_start.p0(ptr)\n";
    if (need_llvm_va_end_) out << "declare void @llvm.va_end.p0(ptr)\n";
    if (need_llvm_va_copy_) out << "declare void @llvm.va_copy.p0.p0(ptr, ptr)\n";
    if (need_llvm_memcpy_) out << "declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1)\n";
    if (need_llvm_stacksave_) out << "declare ptr @llvm.stacksave()\n";
    if (need_llvm_stackrestore_) out << "declare void @llvm.stackrestore(ptr)\n";
    if (need_llvm_va_start_ || need_llvm_va_end_ || need_llvm_va_copy_ ||
        need_llvm_memcpy_ || need_llvm_stacksave_ || need_llvm_stackrestore_) out << "\n";
    for (const auto& [name, ret_ty] : extern_call_decls_) {
      if (mod_.fn_index.count(name)) continue;
      out << "declare " << ret_ty << " @" << name << "(...)\n";
    }
    if (!extern_call_decls_.empty()) out << "\n";
    for (const auto& b : fn_bodies_) out << b;
    return out.str();
  }

 private:
  const Module& mod_;
  std::ostringstream preamble_;
  std::vector<std::string> fn_bodies_;
  std::unordered_map<std::string, std::string> extern_call_decls_;  // name -> ret llvm type
  std::unordered_map<std::string, std::string> str_pool_;
  int str_idx_ = 0;
  bool need_llvm_va_start_ = false;
  bool need_llvm_va_end_ = false;
  bool need_llvm_va_copy_ = false;
  bool need_llvm_memcpy_ = false;
  bool need_llvm_stacksave_ = false;
  bool need_llvm_stackrestore_ = false;

  // ── Per-function state ────────────────────────────────────────────────────

  struct FnCtx {
    const Function* fn = nullptr;
    int tmp_idx = 0;
    bool last_term = false;
    // local_id.value → alloca slot (e.g. "%lv.x")
    std::unordered_map<uint32_t, std::string> local_slots;
    // local_id.value → C TypeSpec
    std::unordered_map<uint32_t, TypeSpec> local_types;
    // local_id.value → true if this local is a VLA allocated dynamically at runtime
    std::unordered_map<uint32_t, bool> local_is_vla;
    // param_index → SSA name (e.g. "%p.x")
    std::unordered_map<uint32_t, std::string> param_slots;
    std::vector<std::string> alloca_lines;
    std::vector<std::string> body_lines;
    // legacy per-block metadata (kept for compatibility; mostly unused now)
    std::unordered_map<uint32_t, BlockMeta> block_meta;
    // body_block -> continue branch target label
    std::unordered_map<uint32_t, std::string> continue_redirect;
    // user label name → LLVM label
    std::unordered_map<std::string, std::string> user_labels;
    // local_id.value → return TypeSpec when calling through that fn-ptr local
    // Populated when a fn-ptr local is initialized from a known function.
    std::unordered_map<uint32_t, TypeSpec> local_fn_ret_types;
    // local_id.value / param_index / global_id.value → fn-ptr signature metadata.
    std::unordered_map<uint32_t, FnPtrSig> local_fn_ptr_sigs;
    std::unordered_map<uint32_t, FnPtrSig> param_fn_ptr_sigs;
    std::unordered_map<uint32_t, FnPtrSig> global_fn_ptr_sigs;
    // Per-function stacksave pointer for VLA scope rewinds.
    std::optional<std::string> vla_stack_save_ptr;
    // Block currently being emitted (for backward-goto detection).
    uint32_t current_block_id = 0;
  };

  // ── Instruction helpers ───────────────────────────────────────────────────

  void emit_instr(FnCtx& ctx, const std::string& line) {
    ctx.body_lines.push_back("  " + line);
    ctx.last_term = false;
  }

  void emit_lbl(FnCtx& ctx, const std::string& lbl) {
    ctx.body_lines.push_back(lbl + ":");
    ctx.last_term = false;
  }

  void emit_term(FnCtx& ctx, const std::string& line) {
    if (!ctx.last_term) {
      ctx.body_lines.push_back("  " + line);
      ctx.last_term = true;
    }
  }

  std::string fresh_tmp(FnCtx& ctx) {
    return "%t" + std::to_string(ctx.tmp_idx++);
  }

  void record_extern_call_decl(const std::string& name, const std::string& ret_ty) {
    if (name.empty() || mod_.fn_index.count(name)) return;
    auto it = extern_call_decls_.find(name);
    if (it == extern_call_decls_.end()) {
      extern_call_decls_.emplace(name, ret_ty);
      return;
    }
    if (it->second == "void" && ret_ty != "void") it->second = ret_ty;
  }

  std::string fresh_lbl(FnCtx& ctx, const std::string& pfx) {
    return pfx + std::to_string(ctx.tmp_idx++);
  }

  static std::string block_lbl(BlockId id) {
    return "block_" + std::to_string(id.value);
  }

  // ── String constant pool ──────────────────────────────────────────────────

  std::string intern_str(const std::string& raw_bytes) {
    auto it = str_pool_.find(raw_bytes);
    if (it != str_pool_.end()) return it->second;
    const std::string name = "@.str" + std::to_string(str_idx_++);
    str_pool_[raw_bytes] = name;
    const size_t len = raw_bytes.size() + 1;
    std::string esc;
    for (unsigned char c : raw_bytes) {
      if (c == '"')      { esc += "\\22"; }
      else if (c == '\\') { esc += "\\5C"; }
      else if (c == '\n') { esc += "\\0A"; }
      else if (c == '\r') { esc += "\\0D"; }
      else if (c == '\t') { esc += "\\09"; }
      else if (c < 32 || c >= 127) {
        char buf[8]; std::snprintf(buf, sizeof(buf), "\\%02X", c); esc += buf;
      } else {
        esc += static_cast<char>(c);
      }
    }
    preamble_ << name << " = private unnamed_addr constant ["
              << len << " x i8] c\"" << esc << "\\00\"\n";
    return name;
  }

  // ── Preamble ──────────────────────────────────────────────────────────────

  void emit_preamble() {
    preamble_ << "%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }\n";
    // Emit struct/union type definitions in declaration order
    for (const auto& tag : mod_.struct_def_order) {
      const auto it = mod_.struct_defs.find(tag);
      if (it == mod_.struct_defs.end()) continue;
      const HirStructDef& sd = it->second;
      if (sd.fields.empty()) {
        // Empty struct: emit as opaque type {}.
        preamble_ << "%struct." << tag << " = type {}\n";
        continue;
      }
      if (sd.is_union) {
        const int sz = compute_struct_size(mod_, tag);
        preamble_ << "%struct." << tag << " = type { ["
                  << sz << " x i8] }\n";
      } else {
        // Deduplicate fields by llvm_idx (bitfields share one storage unit)
        preamble_ << "%struct." << tag << " = type { ";
        bool first = true;
        int last_idx = -1;
        for (const auto& f : sd.fields) {
          if (f.llvm_idx == last_idx) continue;  // same storage unit
          last_idx = f.llvm_idx;
          if (!first) preamble_ << ", ";
          first = false;
          preamble_ << llvm_field_ty(f);
        }
        preamble_ << " }\n";
      }
    }
  }

  // ── Globals ───────────────────────────────────────────────────────────────

  static bool is_char_like(TypeBase b) {
    return b == TB_CHAR || b == TB_SCHAR || b == TB_UCHAR;
  }

  static TypeSpec drop_one_array_dim(TypeSpec ts) {
    if (ts.array_rank <= 0) return ts;
    for (int i = 0; i < ts.array_rank - 1; ++i) ts.array_dims[i] = ts.array_dims[i + 1];
    ts.array_dims[ts.array_rank - 1] = -1;
    ts.array_rank--;
    ts.array_size = (ts.array_rank > 0) ? ts.array_dims[0] : -1;
    return ts;
  }

  static std::string bytes_from_string_literal(const StringLiteral& sl) {
    std::string bytes = sl.raw;
    if (bytes.size() >= 2 && bytes.front() == '"' && bytes.back() == '"') {
      bytes = bytes.substr(1, bytes.size() - 2);
    } else if (bytes.size() >= 3 && bytes[0] == 'L' && bytes[1] == '"' &&
               bytes.back() == '"') {
      bytes = bytes.substr(2, bytes.size() - 3);
    }
    return decode_c_escaped_bytes(bytes);
  }

  // Decode a wide string literal (L"...") into wchar_t (i32) values with null terminator
  static std::vector<long long> decode_wide_string_values(const std::string& raw) {
    std::vector<long long> out;
    // Strip L"..." wrapper
    const char* p = raw.c_str();
    while (*p && *p != '"') ++p;
    if (*p == '"') ++p;
    while (*p && *p != '"') {
      if (*p == '\\' && *(p + 1)) {
        ++p;
        switch (*p) {
          case 'n':  out.push_back('\n'); ++p; break;
          case 't':  out.push_back('\t'); ++p; break;
          case 'r':  out.push_back('\r'); ++p; break;
          case '0':  out.push_back(0); ++p; break;
          case '\\': out.push_back('\\'); ++p; break;
          case '\'': out.push_back('\''); ++p; break;
          case '"':  out.push_back('"'); ++p; break;
          case 'a':  out.push_back('\a'); ++p; break;
          case 'x': {
            ++p;
            long long v = 0;
            while (isxdigit((unsigned char)*p)) {
              v = v * 16 + (isdigit((unsigned char)*p) ? *p - '0' : tolower((unsigned char)*p) - 'a' + 10);
              ++p;
            }
            out.push_back(v);
            break;
          }
          default:
            if (*p >= '0' && *p <= '7') {
              long long v = 0;
              for (int k = 0; k < 3 && *p >= '0' && *p <= '7'; ++k, ++p)
                v = v * 8 + (*p - '0');
              out.push_back(v);
            } else {
              out.push_back(static_cast<unsigned char>(*p)); ++p;
            }
            break;
        }
        continue;
      }
      // Decode UTF-8 to Unicode codepoint
      const unsigned char c0 = static_cast<unsigned char>(*p);
      if (c0 < 0x80) {
        out.push_back(c0); ++p;
      } else if ((c0 & 0xE0) == 0xC0 && p[1]) {
        out.push_back(((c0 & 0x1F) << 6) | (static_cast<unsigned char>(p[1]) & 0x3F));
        p += 2;
      } else if ((c0 & 0xF0) == 0xE0 && p[1] && p[2]) {
        out.push_back(((c0 & 0x0F) << 12) | ((static_cast<unsigned char>(p[1]) & 0x3F) << 6) |
                       (static_cast<unsigned char>(p[2]) & 0x3F));
        p += 3;
      } else if ((c0 & 0xF8) == 0xF0 && p[1] && p[2] && p[3]) {
        out.push_back(((c0 & 0x07) << 18) | ((static_cast<unsigned char>(p[1]) & 0x3F) << 12) |
                       ((static_cast<unsigned char>(p[2]) & 0x3F) << 6) |
                       (static_cast<unsigned char>(p[3]) & 0x3F));
        p += 4;
      } else {
        out.push_back(c0); ++p;
      }
    }
    out.push_back(0); // null terminator
    return out;
  }

  static std::string escape_llvm_c_bytes(const std::string& raw_bytes) {
    std::string esc;
    for (unsigned char c : raw_bytes) {
      if (c == '"')      { esc += "\\22"; }
      else if (c == '\\') { esc += "\\5C"; }
      else if (c == '\n') { esc += "\\0A"; }
      else if (c == '\r') { esc += "\\0D"; }
      else if (c == '\t') { esc += "\\09"; }
      else if (c < 32 || c >= 127) {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "\\%02X", c);
        esc += buf;
      } else {
        esc += static_cast<char>(c);
      }
    }
    return esc;
  }

  TypeSpec field_decl_type(const HirStructField& f) const {
    TypeSpec ts = f.elem_type;
    if (f.array_first_dim >= 0) {
      for (int i = 0; i < 8; ++i) ts.array_dims[i] = -1;
      ts.array_rank = 1;
      ts.array_size = f.array_first_dim;
      ts.array_dims[0] = f.array_first_dim;
    }
    return ts;
  }

  // Recursively constant-evaluate an expression to an integer (returns nullopt if not possible).
  std::optional<long long> try_const_eval_int(ExprId id) {
    const Expr& e = get_expr(id);
    return std::visit([&](const auto& p) -> std::optional<long long> {
      using T = std::decay_t<decltype(p)>;
      if constexpr (std::is_same_v<T, IntLiteral>) {
        return p.value;
      } else if constexpr (std::is_same_v<T, CharLiteral>) {
        return static_cast<long long>(p.value);
      } else if constexpr (std::is_same_v<T, CastExpr>) {
        return try_const_eval_int(p.expr);
      } else if constexpr (std::is_same_v<T, UnaryExpr>) {
        auto v = try_const_eval_int(p.operand);
        if (!v) return std::nullopt;
        switch (p.op) {
          case UnaryOp::Plus:   return *v;
          case UnaryOp::Minus:  return -*v;
          case UnaryOp::BitNot: return ~*v;
          case UnaryOp::Not:    return static_cast<long long>(!*v);
          default: return std::nullopt;
        }
      } else if constexpr (std::is_same_v<T, BinaryExpr>) {
        auto lv = try_const_eval_int(p.lhs);
        auto rv = try_const_eval_int(p.rhs);
        if (!lv || !rv) return std::nullopt;
        switch (p.op) {
          case BinaryOp::Add:    return *lv + *rv;
          case BinaryOp::Sub:    return *lv - *rv;
          case BinaryOp::Mul:    return *lv * *rv;
          case BinaryOp::Div:    return (*rv != 0) ? *lv / *rv : 0LL;
          case BinaryOp::Mod:    return (*rv != 0) ? *lv % *rv : 0LL;
          case BinaryOp::Shl:    return *lv << *rv;
          case BinaryOp::Shr:    return *lv >> *rv;
          case BinaryOp::BitAnd: return *lv & *rv;
          case BinaryOp::BitOr:  return *lv | *rv;
          case BinaryOp::BitXor: return *lv ^ *rv;
          case BinaryOp::Lt:     return static_cast<long long>(*lv < *rv);
          case BinaryOp::Le:     return static_cast<long long>(*lv <= *rv);
          case BinaryOp::Gt:     return static_cast<long long>(*lv > *rv);
          case BinaryOp::Ge:     return static_cast<long long>(*lv >= *rv);
          case BinaryOp::Eq:     return static_cast<long long>(*lv == *rv);
          case BinaryOp::Ne:     return static_cast<long long>(*lv != *rv);
          case BinaryOp::LAnd:   return static_cast<long long>(*lv && *rv);
          case BinaryOp::LOr:    return static_cast<long long>(*lv || *rv);
          default: return std::nullopt;
        }
      } else {
        return std::nullopt;
      }
    }, e.payload);
  }

  std::optional<double> try_const_eval_float(ExprId id) {
    const Expr& e = get_expr(id);
    return std::visit([&](const auto& p) -> std::optional<double> {
      using T = std::decay_t<decltype(p)>;
      if constexpr (std::is_same_v<T, FloatLiteral>) {
        return p.value;
      } else if constexpr (std::is_same_v<T, IntLiteral>) {
        return static_cast<double>(p.value);
      } else if constexpr (std::is_same_v<T, CharLiteral>) {
        return static_cast<double>(p.value);
      } else if constexpr (std::is_same_v<T, CastExpr>) {
        return try_const_eval_float(p.expr);
      } else if constexpr (std::is_same_v<T, UnaryExpr>) {
        auto v = try_const_eval_float(p.operand);
        if (!v) return std::nullopt;
        switch (p.op) {
          case UnaryOp::Plus:  return *v;
          case UnaryOp::Minus: return -*v;
          default: return std::nullopt;
        }
      } else if constexpr (std::is_same_v<T, BinaryExpr>) {
        auto lv = try_const_eval_float(p.lhs);
        auto rv = try_const_eval_float(p.rhs);
        if (!lv || !rv) return std::nullopt;
        switch (p.op) {
          case BinaryOp::Add: return *lv + *rv;
          case BinaryOp::Sub: return *lv - *rv;
          case BinaryOp::Mul: return *lv * *rv;
          case BinaryOp::Div: return *lv / *rv;  // IEEE: div-by-zero → ±Inf
          default: return std::nullopt;
        }
      } else if constexpr (std::is_same_v<T, CallExpr>) {
        // Constant builtin calls: __builtin_inf/nan/huge_val
        const Expr& callee_e = get_expr(p.callee);
        if (const auto* dr = std::get_if<DeclRef>(&callee_e.payload)) {
          const std::string& fn = dr->name;
          if (fn == "__builtin_huge_val"  || fn == "__builtin_huge_vall" ||
              fn == "__builtin_inf"       || fn == "__builtin_infl")
            return std::numeric_limits<double>::infinity();
          if (fn == "__builtin_huge_valf" || fn == "__builtin_inff")
            return static_cast<double>(std::numeric_limits<float>::infinity());
          if (fn == "__builtin_nan" || fn == "__builtin_nans" ||
              fn == "__builtin_nanl" || fn == "__builtin_nansl")
            return std::numeric_limits<double>::quiet_NaN();
          if (fn == "__builtin_nanf" || fn == "__builtin_nansf")
            return static_cast<double>(std::numeric_limits<float>::quiet_NaN());
        }
        return std::nullopt;
      } else {
        return std::nullopt;
      }
    }, e.payload);
  }

  std::optional<std::pair<long long, long long>> try_const_eval_complex_int(ExprId id) {
    const Expr& e = get_expr(id);
    return std::visit([&](const auto& p) -> std::optional<std::pair<long long, long long>> {
      using T = std::decay_t<decltype(p)>;
      if constexpr (std::is_same_v<T, IntLiteral>) {
        if (e.type.spec.base == TB_COMPLEX_CHAR || e.type.spec.base == TB_COMPLEX_SCHAR ||
            e.type.spec.base == TB_COMPLEX_UCHAR || e.type.spec.base == TB_COMPLEX_SHORT ||
            e.type.spec.base == TB_COMPLEX_USHORT || e.type.spec.base == TB_COMPLEX_INT ||
            e.type.spec.base == TB_COMPLEX_UINT || e.type.spec.base == TB_COMPLEX_LONG ||
            e.type.spec.base == TB_COMPLEX_ULONG || e.type.spec.base == TB_COMPLEX_LONGLONG ||
            e.type.spec.base == TB_COMPLEX_ULONGLONG) {
          return std::pair<long long, long long>{0, p.value};
        }
        return std::pair<long long, long long>{p.value, 0};
      } else if constexpr (std::is_same_v<T, CharLiteral>) {
        return std::pair<long long, long long>{p.value, 0};
      } else if constexpr (std::is_same_v<T, UnaryExpr>) {
        auto v = try_const_eval_complex_int(p.operand);
        if (!v) return std::nullopt;
        switch (p.op) {
          case UnaryOp::Plus: return v;
          case UnaryOp::Minus: return std::pair<long long, long long>{-v->first, -v->second};
          default: return std::nullopt;
        }
      } else if constexpr (std::is_same_v<T, BinaryExpr>) {
        auto lv = try_const_eval_complex_int(p.lhs);
        auto rv = try_const_eval_complex_int(p.rhs);
        if (!lv || !rv) return std::nullopt;
        switch (p.op) {
          case BinaryOp::Add:
            return std::pair<long long, long long>{lv->first + rv->first, lv->second + rv->second};
          case BinaryOp::Sub:
            return std::pair<long long, long long>{lv->first - rv->first, lv->second - rv->second};
          default:
            return std::nullopt;
        }
      } else if constexpr (std::is_same_v<T, CastExpr>) {
        return try_const_eval_complex_int(p.expr);
      } else {
        return std::nullopt;
      }
    }, e.payload);
  }

  std::optional<std::pair<double, double>> try_const_eval_complex_fp(ExprId id) {
    const Expr& e = get_expr(id);
    return std::visit([&](const auto& p) -> std::optional<std::pair<double, double>> {
      using T = std::decay_t<decltype(p)>;
      if constexpr (std::is_same_v<T, FloatLiteral>) {
        if (is_complex_base(e.type.spec.base)) return std::pair<double, double>{0.0, p.value};
        return std::pair<double, double>{p.value, 0.0};
      } else if constexpr (std::is_same_v<T, IntLiteral>) {
        if (is_complex_base(e.type.spec.base)) return std::pair<double, double>{0.0, static_cast<double>(p.value)};
        return std::pair<double, double>{static_cast<double>(p.value), 0.0};
      } else if constexpr (std::is_same_v<T, CharLiteral>) {
        return std::pair<double, double>{static_cast<double>(p.value), 0.0};
      } else if constexpr (std::is_same_v<T, UnaryExpr>) {
        auto v = try_const_eval_complex_fp(p.operand);
        if (!v) return std::nullopt;
        switch (p.op) {
          case UnaryOp::Plus: return v;
          case UnaryOp::Minus: return std::pair<double, double>{-v->first, -v->second};
          default: return std::nullopt;
        }
      } else if constexpr (std::is_same_v<T, BinaryExpr>) {
        auto lv = try_const_eval_complex_fp(p.lhs);
        auto rv = try_const_eval_complex_fp(p.rhs);
        if (!lv || !rv) return std::nullopt;
        switch (p.op) {
          case BinaryOp::Add:
            return std::pair<double, double>{lv->first + rv->first, lv->second + rv->second};
          case BinaryOp::Sub:
            return std::pair<double, double>{lv->first - rv->first, lv->second - rv->second};
          case BinaryOp::Mul:
            return std::pair<double, double>{
                lv->first * rv->first - lv->second * rv->second,
                lv->first * rv->second + lv->second * rv->first};
          case BinaryOp::Div: {
            const double denom = rv->first * rv->first + rv->second * rv->second;
            if (denom == 0.0) return std::nullopt;
            return std::pair<double, double>{
                (lv->first * rv->first + lv->second * rv->second) / denom,
                (lv->second * rv->first - lv->first * rv->second) / denom};
          }
          default:
            return std::nullopt;
        }
      } else if constexpr (std::is_same_v<T, CastExpr>) {
        return try_const_eval_complex_fp(p.expr);
      } else {
        return std::nullopt;
      }
    }, e.payload);
  }

  std::string emit_const_int_like(long long value, const TypeSpec& expected_ts) {
    if (llvm_ty(expected_ts) == "ptr") {
      if (value == 0) return "null";
      return "inttoptr (i64 " + std::to_string(value) + " to ptr)";
    }
    if (is_float_base(expected_ts.base) && expected_ts.ptr_level == 0 && expected_ts.array_rank == 0)
      return fp_literal(expected_ts.base, static_cast<double>(value));
    return std::to_string(value);
  }

  std::string emit_const_scalar_expr(ExprId id, const TypeSpec& expected_ts) {
    const Expr& e = get_expr(id);
    return std::visit([&](const auto& p) -> std::string {
      using T = std::decay_t<decltype(p)>;
      if (is_complex_base(expected_ts.base) && expected_ts.ptr_level == 0 && expected_ts.array_rank == 0) {
        const TypeSpec elem_ts = complex_component_ts(expected_ts.base);
        if (is_float_base(elem_ts.base)) {
          if (auto cv = try_const_eval_complex_fp(id)) {
            const auto emit_fp = [&](double value) -> std::string {
              return fp_literal(elem_ts.base, value);
            };
            return "{ " + llvm_ty(elem_ts) + " " + emit_fp(cv->first) +
                   ", " + llvm_ty(elem_ts) + " " + emit_fp(cv->second) + " }";
          }
        }
        if (auto cv = try_const_eval_complex_int(id)) {
          return "{ " + llvm_ty(elem_ts) + " " +
                 emit_const_int_like(cv->first, elem_ts) +
                 ", " + llvm_ty(elem_ts) + " " +
                 emit_const_int_like(cv->second, elem_ts) +
                 " }";
        }
        return "zeroinitializer";
      }
      if constexpr (std::is_same_v<T, IntLiteral>) {
        if (llvm_ty(expected_ts) == "ptr") {
          if (p.value == 0) return "null";
          return "inttoptr (i64 " + std::to_string(p.value) + " to ptr)";
        }
        if (is_float_base(expected_ts.base) && expected_ts.ptr_level == 0 && expected_ts.array_rank == 0)
          return fp_literal(expected_ts.base, static_cast<double>(p.value));
        return std::to_string(p.value);
      } else if constexpr (std::is_same_v<T, FloatLiteral>) {
        if (is_float_base(expected_ts.base) && expected_ts.ptr_level == 0 && expected_ts.array_rank == 0)
          return fp_literal(expected_ts.base, p.value);
        return fp_to_hex(p.value);
      } else if constexpr (std::is_same_v<T, CharLiteral>) {
        return std::to_string(p.value);
      } else if constexpr (std::is_same_v<T, StringLiteral>) {
        const std::string bytes = bytes_from_string_literal(p);
        const std::string gname = intern_str(bytes);
        const size_t len = bytes.size() + 1;
        return "getelementptr inbounds ([" + std::to_string(len) + " x i8], ptr " +
               gname + ", i64 0, i64 0)";
      } else if constexpr (std::is_same_v<T, DeclRef>) {
        if (mod_.fn_index.count(p.name) || llvm_ty(expected_ts) == "ptr" ||
            expected_ts.ptr_level > 0 || expected_ts.is_fn_ptr) {
          return "@" + p.name;
        }
        return "0";
      } else if constexpr (std::is_same_v<T, UnaryExpr>) {
        if (p.op == UnaryOp::AddrOf) {
          const Expr& op_e = get_expr(p.operand);
          if (const auto* r = std::get_if<DeclRef>(&op_e.payload))
            return "@" + r->name;
          if (const auto* s = std::get_if<StringLiteral>(&op_e.payload)) {
            const std::string bytes = bytes_from_string_literal(*s);
            const std::string gname = intern_str(bytes);
            const size_t len = bytes.size() + 1;
            return "getelementptr inbounds ([" + std::to_string(len) + " x i8], ptr " +
                   gname + ", i64 0, i64 0)";
          }
          // &arr[idx] — global array subscript: emit GEP constant
          if (const auto* idx_e = std::get_if<IndexExpr>(&op_e.payload)) {
            const Expr& base_e = get_expr(idx_e->base);
            if (const auto* dr = std::get_if<DeclRef>(&base_e.payload)) {
              auto git = mod_.global_index.find(dr->name);
              if (git != mod_.global_index.end()) {
                const GlobalVar* gv = mod_.find_global(git->second);
                if (gv && gv->type.spec.array_rank > 0) {
                  const std::string aty = llvm_alloca_ty(gv->type.spec);
                  const long long idx_int =
                      try_const_eval_int(idx_e->index).value_or(0);
                  return "getelementptr inbounds (" + aty + ", ptr @" + dr->name +
                         ", i64 0, i64 " + std::to_string(idx_int) + ")";
                }
              }
            }
          }
          // &struct_var.field — global struct member: emit GEP constant
          if (const auto* mem_e = std::get_if<MemberExpr>(&op_e.payload)) {
            if (!mem_e->is_arrow) {
              const Expr& base_e = get_expr(mem_e->base);
              if (const auto* dr = std::get_if<DeclRef>(&base_e.payload)) {
                auto git = mod_.global_index.find(dr->name);
                if (git != mod_.global_index.end()) {
                  const GlobalVar* gv = mod_.find_global(git->second);
                  if (gv) {
                    const std::string tag = std::string(gv->type.spec.tag);
                    if (!tag.empty()) {
                      const auto sit = mod_.struct_defs.find(tag);
                    const HirStructDef* sd = (sit != mod_.struct_defs.end()) ? &sit->second : nullptr;
                      if (sd) {
                        int field_idx = 0;
                        for (const auto& f : sd->fields) {
                          if (f.name == mem_e->field) break;
                          ++field_idx;
                        }
                        const std::string sty = "%struct." + tag;
                        return "getelementptr inbounds (" + sty + ", ptr @" + dr->name +
                               ", i32 0, i32 " + std::to_string(field_idx) + ")";
                      }
                    }
                  }
                }
              }
            }
          }
        }
        if (p.op == UnaryOp::Plus) {
          return emit_const_scalar_expr(p.operand, expected_ts);
        }
        if (p.op == UnaryOp::Minus) {
          const Expr& op_e = get_expr(p.operand);
          if (const auto* i = std::get_if<IntLiteral>(&op_e.payload)) {
            // When target is float, emit as float constant, not integer literal.
            if (is_float_base(expected_ts.base) && expected_ts.ptr_level == 0 && expected_ts.array_rank == 0)
              return fp_literal(expected_ts.base, static_cast<double>(-i->value));
            return std::to_string(-i->value);
          }
          if (const auto* f = std::get_if<FloatLiteral>(&op_e.payload)) {
            if (is_float_base(expected_ts.base) && expected_ts.ptr_level == 0 && expected_ts.array_rank == 0)
              return fp_literal(expected_ts.base, -f->value);
            return fp_to_hex(-f->value);
          }
        }
        // For float targets, try full float constant eval (handles -0.0, etc.)
        if (is_float_base(expected_ts.base) && expected_ts.ptr_level == 0 && expected_ts.array_rank == 0) {
          auto fval = try_const_eval_float(id);
          if (fval) return fp_literal(expected_ts.base, *fval);
        }
        return (llvm_ty(expected_ts) == "ptr") ? "null" : "0";
      } else if constexpr (std::is_same_v<T, CastExpr>) {
        // Apply the cast's target type to properly truncate/extend the value.
        const TypeSpec& cast_ts = p.to_type.spec;
        if (is_any_int(cast_ts.base) && cast_ts.ptr_level == 0 &&
            is_any_int(expected_ts.base) && expected_ts.ptr_level == 0) {
          auto val = try_const_eval_int(p.expr);
          if (val) {
            // Truncate to the cast target width, then zero/sign-extend.
            long long v = *val;
            const int bits = int_bits(cast_ts.base);
            if (bits < 64) {
              unsigned long long mask = (1ULL << bits) - 1;
              unsigned long long uv = static_cast<unsigned long long>(v) & mask;
              if (is_signed_int(cast_ts.base) && (uv >> (bits - 1))) {
                // Sign-extend
                v = static_cast<long long>(uv | ~mask);
              } else {
                v = static_cast<long long>(uv);
              }
            }
            if (is_float_base(expected_ts.base))
              return fp_literal(expected_ts.base, static_cast<double>(v));
            return std::to_string(v);
          }
        }
        return emit_const_scalar_expr(p.expr, expected_ts);
      } else if constexpr (std::is_same_v<T, BinaryExpr>) {
        // Try float eval first when target is a float type
        if (is_float_base(expected_ts.base) && expected_ts.ptr_level == 0 && expected_ts.array_rank == 0) {
          auto fval = try_const_eval_float(id);
          if (fval) return fp_literal(expected_ts.base, *fval);
        }
        auto val = try_const_eval_int(id);
        if (val) {
          if (llvm_ty(expected_ts) == "ptr") {
            if (*val == 0) return "null";
            return "inttoptr (i64 " + std::to_string(*val) + " to ptr)";
          }
          return std::to_string(*val);
        }
        return (llvm_ty(expected_ts) == "ptr") ? "null" : "0";
      } else if constexpr (std::is_same_v<T, LabelAddrExpr>) {
        return "blockaddress(@" + p.fn_name + ", %ulbl_" + p.label_name + ")";
      } else if constexpr (std::is_same_v<T, CallExpr>) {
        // Constant builtin calls that produce float/double constants in global initializers
        const Expr& callee_e = get_expr(p.callee);
        if (const auto* dr = std::get_if<DeclRef>(&callee_e.payload)) {
          const std::string& fn = dr->name;
          const bool is_float_tgt = is_float_base(expected_ts.base) &&
                                    expected_ts.ptr_level == 0 && expected_ts.array_rank == 0;
          // __builtin_huge_val / __builtin_inf / __builtin_nan → infinity/NaN constants
          if (fn == "__builtin_huge_val" || fn == "__builtin_huge_vall" ||
              fn == "__builtin_inf"      || fn == "__builtin_infl") {
            return fp_literal(expected_ts.base, std::numeric_limits<double>::infinity());
          }
          if (fn == "__builtin_huge_valf" || fn == "__builtin_inff") {
            if (expected_ts.base == TB_FLOAT && expected_ts.ptr_level == 0)
              return fp_to_float_literal(std::numeric_limits<float>::infinity());
            return fp_to_hex(static_cast<double>(std::numeric_limits<float>::infinity()));
          }
          if (fn == "__builtin_nan" || fn == "__builtin_nans" ||
              fn == "__builtin_nanl" || fn == "__builtin_nansl") {
            return fp_literal(expected_ts.base, std::numeric_limits<double>::quiet_NaN());
          }
          if (fn == "__builtin_nanf" || fn == "__builtin_nansf") {
            if (expected_ts.base == TB_FLOAT && expected_ts.ptr_level == 0)
              return fp_to_float_literal(std::numeric_limits<float>::quiet_NaN());
            return fp_to_hex(static_cast<double>(std::numeric_limits<float>::quiet_NaN()));
          }
          (void)is_float_tgt;
        }
        return (llvm_ty(expected_ts) == "ptr") ? "null" : "0";
      } else {
        return (llvm_ty(expected_ts) == "ptr") ? "null" : "0";
      }
    }, e.payload);
  }

  // Brace-elision-aware constant init: consume scalars from a flat InitList
  // at the given cursor, grouping them into nested aggregates as needed.
  std::string emit_const_from_flat(const TypeSpec& ts, const InitList& list, size_t& cursor) {
    if (cursor >= list.items.size())
      return emit_const_init(ts, GlobalInit(std::monostate{}));

    const auto& item = list.items[cursor];
    bool has_sublist = std::holds_alternative<std::shared_ptr<InitList>>(item.value);

    // If item is already a sub-list, use it directly for this element.
    if (has_sublist) {
      auto sub = std::get<std::shared_ptr<InitList>>(item.value);
      ++cursor;
      return emit_const_init(ts, GlobalInit(*sub));
    }

    bool is_agg = (ts.array_rank > 0) ||
        ((ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.ptr_level == 0);

    // Scalar target: consume one item.
    if (!is_agg) {
      const auto* scalar = std::get_if<InitScalar>(&item.value);
      ++cursor;
      if (scalar) return emit_const_scalar_expr(scalar->expr, ts);
      return emit_const_init(ts, GlobalInit(std::monostate{}));
    }

    // String literal directly initializes a char array (not brace elision).
    if (ts.array_rank > 0) {
      TypeSpec ets = drop_one_array_dim(ts);
      if (is_char_like(ets.base) && ets.ptr_level == 0 && ets.array_rank == 0) {
        const auto* scalar = std::get_if<InitScalar>(&item.value);
        if (scalar) {
          const Expr& e = get_expr(scalar->expr);
          if (std::holds_alternative<StringLiteral>(e.payload)) {
            ++cursor;
            return emit_const_init(ts, GlobalInit(*scalar));
          }
        }
      }
    }

    // Array target, scalar item -> brace elision: consume element-by-element.
    if (ts.array_rank > 0) {
      long long n = ts.array_size;
      if (n <= 0) return "zeroinitializer";
      TypeSpec elem_ts = drop_one_array_dim(ts);
      std::string out = "[";
      const std::string ety = llvm_alloca_ty(elem_ts);
      for (long long i = 0; i < n; ++i) {
        if (i) out += ", ";
        out += ety + " ";
        if (cursor < list.items.size())
          out += emit_const_from_flat(elem_ts, list, cursor);
        else
          out += emit_const_init(elem_ts, GlobalInit(std::monostate{}));
      }
      out += "]";
      return out;
    }

    // Struct target, scalar item -> brace elision: consume field-by-field.
    if (!ts.tag || !ts.tag[0]) { ++cursor; return "zeroinitializer"; }
    const auto it = mod_.struct_defs.find(ts.tag);
    if (it == mod_.struct_defs.end()) { ++cursor; return "zeroinitializer"; }
    const HirStructDef& sd = it->second;
    if (sd.is_union) {
      // Union brace elision: init first field only.
      if (sd.fields.empty()) { ++cursor; return "zeroinitializer"; }
      const int sz = compute_struct_size(mod_, std::string(ts.tag));
      const auto* scalar = std::get_if<InitScalar>(&item.value);
      ++cursor;
      if (scalar) {
        const Expr& se = get_expr(scalar->expr);
        long long val = 0;
        if (const auto* il = std::get_if<IntLiteral>(&se.payload)) val = il->value;
        else if (const auto* cl = std::get_if<CharLiteral>(&se.payload)) val = cl->value;
        if (val != 0) {
          std::string bytes(static_cast<size_t>(sz), '\0');
          for (int i = 0; i < sz; ++i)
            bytes[static_cast<size_t>(i)] = static_cast<char>((val >> (8 * i)) & 0xFF);
          return "{ [" + std::to_string(sz) + " x i8] c\"" + escape_llvm_c_bytes(bytes) + "\" }";
        }
      }
      return "{ [" + std::to_string(sz) + " x i8] zeroinitializer }";
    }

    std::vector<std::string> field_vals;
    field_vals.reserve(sd.fields.size());
    for (const auto& f : sd.fields)
      field_vals.push_back(emit_const_init(field_decl_type(f), GlobalInit(std::monostate{})));
    for (size_t fi = 0; fi < sd.fields.size() && cursor < list.items.size(); ++fi)
      field_vals[fi] = emit_const_from_flat(field_decl_type(sd.fields[fi]), list, cursor);

    // Check if struct has bitfields
    bool has_bf = false;
    for (const auto& f : sd.fields) { if (f.bit_width >= 0) { has_bf = true; break; } }

    if (!has_bf) {
      std::string out = "{ ";
      for (size_t i = 0; i < sd.fields.size(); ++i) {
        if (i) out += ", ";
        out += llvm_field_ty(sd.fields[i]) + " " + field_vals[i];
      }
      out += " }";
      return out;
    }

    // Pack bitfields by llvm_idx
    std::string out = "{ ";
    bool first = true;
    int last_idx = -1;
    for (size_t i = 0; i < sd.fields.size(); ) {
      const auto& f0 = sd.fields[i];
      if (f0.llvm_idx == last_idx) { ++i; continue; }
      last_idx = f0.llvm_idx;
      if (!first) out += ", ";
      first = false;
      out += llvm_field_ty(f0) + " ";
      if (f0.bit_width < 0) {
        out += field_vals[i];
        ++i;
      } else {
        unsigned long long packed = 0;
        while (i < sd.fields.size() && sd.fields[i].llvm_idx == last_idx) {
          const auto& bf = sd.fields[i];
          if (bf.bit_width > 0) {
            long long val = 0;
            try { val = std::stoll(field_vals[i]); } catch (...) {}
            unsigned long long mask = (bf.bit_width >= 64) ? ~0ULL : ((1ULL << bf.bit_width) - 1);
            packed |= (static_cast<unsigned long long>(val) & mask) << bf.bit_offset;
          }
          ++i;
        }
        out += std::to_string(static_cast<long long>(packed));
      }
    }
    out += " }";
    return out;
  }

  // Check whether an InitList needs brace-elision: it has more items than
  // the array size and elements are structs/arrays without sub-lists.
  bool needs_brace_elision(const TypeSpec& ts, const InitList& list) const {
    if (ts.array_rank <= 0 || ts.array_size <= 0) return false;
    TypeSpec elem_ts = drop_one_array_dim(ts);
    bool elem_is_agg = (elem_ts.array_rank > 0) ||
        ((elem_ts.base == TB_STRUCT || elem_ts.base == TB_UNION) && elem_ts.ptr_level == 0);
    if (!elem_is_agg) return false;
    // If list has more items than array size, likely brace-elided.
    if ((long long)list.items.size() > ts.array_size) return true;
    // Also check: any item that targets an aggregate but is a scalar.
    for (const auto& item : list.items) {
      if (std::holds_alternative<InitScalar>(item.value)) return true;
    }
    return false;
  }

  std::string emit_const_array(const TypeSpec& ts, const GlobalInit& init) {
    const long long n = ts.array_size;
    if (n <= 0) return "zeroinitializer";
    TypeSpec elem_ts = drop_one_array_dim(ts);

    if (const auto* s = std::get_if<InitScalar>(&init)) {
      const Expr& e = get_expr(s->expr);
      if (const auto* sl = std::get_if<StringLiteral>(&e.payload);
          sl && ts.array_rank == 1 && is_char_like(elem_ts.base) && elem_ts.ptr_level == 0) {
        std::string bytes = bytes_from_string_literal(*sl);
        if ((long long)bytes.size() > n) bytes.resize(static_cast<size_t>(n));
        if ((long long)bytes.size() < n) bytes.resize(static_cast<size_t>(n), '\0');
        return "c\"" + escape_llvm_c_bytes(bytes) + "\"";
      }
      // Wide string literal → array of i32 (wchar_t) values
      if (const auto* sl = std::get_if<StringLiteral>(&e.payload);
          sl && sl->is_wide && ts.array_rank == 1 && elem_ts.ptr_level == 0) {
        std::vector<long long> vals = decode_wide_string_values(sl->raw);
        if ((long long)vals.size() > n) vals.resize(static_cast<size_t>(n));
        while ((long long)vals.size() < n) vals.push_back(0);
        std::string out = "[";
        const std::string ety = llvm_alloca_ty(elem_ts);
        for (size_t i = 0; i < vals.size(); ++i) {
          if (i) out += ", ";
          out += ety + " " + std::to_string(vals[i]);
        }
        out += "]";
        return out;
      }
      std::vector<std::string> elems(static_cast<size_t>(n), "zeroinitializer");
      if (n > 0) elems[0] = emit_const_scalar_expr(s->expr, elem_ts);
      std::string out = "[";
      const std::string ety = llvm_alloca_ty(elem_ts);
      for (size_t i = 0; i < elems.size(); ++i) {
        if (i) out += ", ";
        out += ety + " " + elems[i];
      }
      out += "]";
      return out;
    }

    if (const auto* list = std::get_if<InitList>(&init)) {
      // Brace-elision path: flat scalars need to be grouped into aggregates.
      if (needs_brace_elision(ts, *list)) {
        std::vector<std::string> elems(static_cast<size_t>(n), "zeroinitializer");
        size_t cursor = 0;
        for (long long i = 0; i < n && cursor < list->items.size(); ++i)
          elems[static_cast<size_t>(i)] = emit_const_from_flat(elem_ts, *list, cursor);
        std::string out = "[";
        const std::string ety = llvm_alloca_ty(elem_ts);
        for (size_t i = 0; i < elems.size(); ++i) {
          if (i) out += ", ";
          out += ety + " " + elems[i];
        }
        out += "]";
        return out;
      }
    }

    std::vector<std::string> elems(static_cast<size_t>(n), "zeroinitializer");
    if (const auto* list = std::get_if<InitList>(&init)) {
      size_t next_idx = 0;
      for (const auto& item : list->items) {
        size_t idx = next_idx;
        if (item.index_designator && *item.index_designator >= 0)
          idx = static_cast<size_t>(*item.index_designator);
        if (idx >= static_cast<size_t>(n)) continue;
        GlobalInit child_init = std::visit([&](const auto& v) -> GlobalInit {
          using V = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<V, InitScalar>) return GlobalInit(v);
          else return GlobalInit(*v);
        }, item.value);
        elems[idx] = emit_const_init(elem_ts, child_init);
        next_idx = idx + 1;  // always advance; designators set the base, next follows
      }
    }

    std::string out = "[";
    const std::string ety = llvm_alloca_ty(elem_ts);  // non-decayed for nested arrays
    for (size_t i = 0; i < elems.size(); ++i) {
      if (i) out += ", ";
      out += ety + " " + elems[i];
    }
    out += "]";
    return out;
  }

  std::optional<std::string> try_emit_ptr_from_char_init(const GlobalInit& init) {
    auto make_ptr_to_bytes = [&](const std::string& bytes) -> std::string {
      const std::string gname = intern_str(bytes);
      const size_t len = bytes.size() + 1;
      return "getelementptr inbounds ([" + std::to_string(len) + " x i8], ptr " +
             gname + ", i64 0, i64 0)";
    };

    if (const auto* scalar = std::get_if<InitScalar>(&init)) {
      const Expr& e = get_expr(scalar->expr);
      if (const auto* sl = std::get_if<StringLiteral>(&e.payload)) {
        return make_ptr_to_bytes(bytes_from_string_literal(*sl));
      }
      return std::nullopt;
    }

    const auto* list = std::get_if<InitList>(&init);
    if (!list) return std::nullopt;

    std::string bytes;
    size_t next_idx = 0;
    for (const auto& item : list->items) {
      size_t idx = next_idx;
      if (item.index_designator && *item.index_designator >= 0) {
        idx = static_cast<size_t>(*item.index_designator);
      }
      if (idx > bytes.size()) bytes.resize(idx, '\0');

      GlobalInit child_init = std::visit([&](const auto& v) -> GlobalInit {
        using V = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<V, InitScalar>) return GlobalInit(v);
        else return GlobalInit(*v);
      }, item.value);
      const auto* child_scalar = std::get_if<InitScalar>(&child_init);
      if (!child_scalar) return std::nullopt;

      const Expr& ce = get_expr(child_scalar->expr);
      int ch = 0;
      if (const auto* c = std::get_if<CharLiteral>(&ce.payload)) {
        ch = c->value;
      } else if (const auto* i = std::get_if<IntLiteral>(&ce.payload)) {
        ch = static_cast<int>(i->value);
      } else {
        return std::nullopt;
      }

      if (idx == bytes.size()) bytes.push_back(static_cast<char>(ch & 0xFF));
      else bytes[idx] = static_cast<char>(ch & 0xFF);
      next_idx = idx + 1;
    }

    return make_ptr_to_bytes(bytes);
  }

  TypeSpec resolve_flexible_array_field_ts(const HirStructField& f,
                                           const GlobalInit& init) {
    TypeSpec ts = field_decl_type(f);
    if (f.array_first_dim != 0) return ts;
    long long deduced = deduce_array_size_from_init(init);
    if (deduced <= 0) return ts;
    ts.array_rank = 1;
    ts.array_size = deduced;
    ts.array_dims[0] = deduced;
    return ts;
  }

  std::vector<std::string> emit_const_struct_fields(const TypeSpec&,
                                                    const HirStructDef& sd,
                                                    const GlobalInit& init,
                                                    std::vector<TypeSpec>* out_field_types = nullptr) {
    std::vector<TypeSpec> field_types;
    field_types.reserve(sd.fields.size());
    for (const auto& f : sd.fields) field_types.push_back(field_decl_type(f));

    std::vector<std::string> field_vals;
    field_vals.reserve(sd.fields.size());
    for (size_t i = 0; i < sd.fields.size(); ++i) {
      field_vals.push_back(emit_const_init(field_types[i], GlobalInit(std::monostate{})));
    }

    auto update_field_type = [&](size_t idx, const GlobalInit& child_init) -> const TypeSpec& {
      if (idx + 1 == sd.fields.size() && sd.fields[idx].array_first_dim == 0) {
        field_types[idx] = resolve_flexible_array_field_ts(sd.fields[idx], child_init);
      }
      return field_types[idx];
    };

    // Helper: check if an InitScalar is a string literal (initializes char arrays directly).
    auto is_string_scalar = [&](const InitScalar& s) -> bool {
      const Expr& e = get_expr(s.expr);
      return std::holds_alternative<StringLiteral>(e.payload);
    };
    // Helper: check if a scalar init for an array field is a string literal
    // (not brace elision — string directly fills char array).
    auto is_direct_array_scalar = [&](const TypeSpec& fts, const InitListItem& item) -> bool {
      if (fts.array_rank <= 0) return false;
      TypeSpec ets = drop_one_array_dim(fts);
      if (!is_char_like(ets.base) || ets.ptr_level != 0) return false;
      if (const auto* s = std::get_if<InitScalar>(&item.value))
        return is_string_scalar(*s);
      return false;
    };

    if (const auto* list = std::get_if<InitList>(&init)) {
      // Detect brace elision: more items than fields, or scalar items for agg fields
      // (excluding string literals for char array fields).
      bool use_cursor = false;
      if (list->items.size() > sd.fields.size()) {
        // More items than fields could still be non-elided if designators are used.
        // Check if any non-designated scalar targets an aggregate field.
        bool has_designators = false;
        for (const auto& item : list->items) {
          if (item.field_designator || item.index_designator) { has_designators = true; break; }
        }
        if (!has_designators) use_cursor = true;
      }
      if (!use_cursor) {
        size_t fi = 0;
        for (size_t li = 0; li < list->items.size() && fi < sd.fields.size(); ++li, ++fi) {
          const auto& item = list->items[li];
          if (item.field_designator || item.index_designator) continue;
          TypeSpec fts = field_types[fi];
          bool f_is_agg = (fts.array_rank > 0) ||
              ((fts.base == TB_STRUCT || fts.base == TB_UNION) && fts.ptr_level == 0);
          if (f_is_agg && std::holds_alternative<InitScalar>(item.value) &&
              !is_direct_array_scalar(fts, item)) {
            use_cursor = true;
            break;
          }
        }
      }

      if (use_cursor) {
        size_t cursor = 0;
        for (size_t fi = 0; fi < sd.fields.size() && cursor < list->items.size(); ++fi) {
          const auto& item = list->items[cursor];
          // Handle designators.
          if (item.field_designator) {
            const auto fit = std::find_if(
                sd.fields.begin(), sd.fields.end(),
                [&](const HirStructField& f) { return f.name == *item.field_designator; });
            if (fit != sd.fields.end()) {
              fi = static_cast<size_t>(std::distance(sd.fields.begin(), fit));
            }
          }
          if (fi >= sd.fields.size()) break;
          bool has_sublist = std::holds_alternative<std::shared_ptr<InitList>>(item.value);
          if (has_sublist) {
            auto sub = std::get<std::shared_ptr<InitList>>(item.value);
            ++cursor;
            GlobalInit child_init(*sub);
            const TypeSpec& field_ts = update_field_type(fi, child_init);
            if (llvm_field_ty(sd.fields[fi]) == "ptr") {
              if (auto ptr_init = try_emit_ptr_from_char_init(child_init)) {
                field_vals[fi] = *ptr_init;
                continue;
              }
            }
            field_vals[fi] = emit_const_init(field_ts, child_init);
          } else {
            GlobalInit child_init = std::visit([&](const auto& v) -> GlobalInit {
              using V = std::decay_t<decltype(v)>;
              if constexpr (std::is_same_v<V, InitScalar>) return GlobalInit(v);
              else return GlobalInit(*v);
            }, item.value);
            const TypeSpec& field_ts = update_field_type(fi, child_init);
            bool f_is_agg = (field_ts.array_rank > 0) ||
                ((field_ts.base == TB_STRUCT || field_ts.base == TB_UNION) && field_ts.ptr_level == 0);
            if (f_is_agg && !is_direct_array_scalar(field_ts, item)) {
              // Brace elision: consume from flat list.
              field_vals[fi] = emit_const_from_flat(field_ts, *list, cursor);
            } else {
              const auto* scalar = std::get_if<InitScalar>(&item.value);
              ++cursor;
              if (scalar) {
                if (llvm_field_ty(sd.fields[fi]) == "ptr") {
                  if (auto ptr_init = try_emit_ptr_from_char_init(child_init)) {
                    field_vals[fi] = *ptr_init;
                    continue;
                  }
                }
                field_vals[fi] = emit_const_init(field_ts, child_init);
              }
            }
          }
        }
      } else {
        size_t next_idx = 0;
        for (const auto& item : list->items) {
          size_t idx = next_idx;
          if (item.field_designator) {
            const auto fit = std::find_if(
                sd.fields.begin(), sd.fields.end(),
                [&](const HirStructField& f) { return f.name == *item.field_designator; });
            if (fit == sd.fields.end()) continue;
            idx = static_cast<size_t>(std::distance(sd.fields.begin(), fit));
          } else if (item.index_designator && *item.index_designator >= 0) {
            idx = static_cast<size_t>(*item.index_designator);
          }
          if (idx >= sd.fields.size()) continue;

          GlobalInit child_init = std::visit([&](const auto& v) -> GlobalInit {
            using V = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<V, InitScalar>) return GlobalInit(v);
            else return GlobalInit(*v);
          }, item.value);
          const TypeSpec& field_ts = update_field_type(idx, child_init);
          if (llvm_field_ty(sd.fields[idx]) == "ptr") {
            if (auto ptr_init = try_emit_ptr_from_char_init(child_init)) {
              field_vals[idx] = *ptr_init;
              next_idx = idx + 1;
              continue;
            }
          }
          field_vals[idx] = emit_const_init(field_ts, child_init);
          next_idx = idx + 1;
        }
      }
    } else if (const auto* scalar = std::get_if<InitScalar>(&init)) {
      // Scalar fallback for aggregates: treat as first field init.
      if (!sd.fields.empty()) {
        GlobalInit child_init(*scalar);
        const TypeSpec& field_ts = update_field_type(0, child_init);
        field_vals[0] = emit_const_init(field_ts, child_init);
      }
    }

    if (out_field_types) *out_field_types = std::move(field_types);
    return field_vals;
  }

  std::string emit_const_struct(const TypeSpec& ts, const GlobalInit& init) {
    if (!ts.tag || !ts.tag[0]) return "zeroinitializer";
    const auto it = mod_.struct_defs.find(ts.tag);
    if (it == mod_.struct_defs.end()) return "zeroinitializer";
    const HirStructDef& sd = it->second;
    if (sd.is_union) {
      if (sd.fields.empty()) return "zeroinitializer";  // empty union: type {} init
      const int sz = compute_struct_size(mod_, std::string(ts.tag));
      auto zero_union = [&]() -> std::string {
        if (sz == 0) return "zeroinitializer";
        return "{ [" + std::to_string(sz) + " x i8] zeroinitializer }";
      };

      auto child_init_of = [&](const InitListItem& item) -> GlobalInit {
        return std::visit([&](const auto& v) -> GlobalInit {
          using V = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<V, InitScalar>) return GlobalInit(v);
          else return GlobalInit(*v);
        }, item.value);
      };

      std::function<bool(const TypeSpec&, const GlobalInit&, std::vector<unsigned char>&)> encode_bytes;
      encode_bytes = [&](const TypeSpec& cur_ts, const GlobalInit& cur_init,
                         std::vector<unsigned char>& out) -> bool {
        const int cur_sz = std::max(1, sizeof_ts(mod_, cur_ts));
        out.assign(static_cast<size_t>(cur_sz), 0);

        if (cur_ts.ptr_level > 0 || cur_ts.is_fn_ptr) return false;

        if (cur_ts.array_rank > 0) {
          const long long n = cur_ts.array_size;
          if (n <= 0) return true;
          TypeSpec elem_ts = drop_one_array_dim(cur_ts);
          const int elem_sz = std::max(1, sizeof_ts(mod_, elem_ts));
          if (const auto* scalar = std::get_if<InitScalar>(&cur_init)) {
            const Expr& e = get_expr(scalar->expr);
            if (const auto* sl = std::get_if<StringLiteral>(&e.payload)) {
              if (elem_ts.ptr_level == 0 && is_char_like(elem_ts.base)) {
                const std::string bytes = bytes_from_string_literal(*sl);
                const size_t max_n = static_cast<size_t>(n);
                for (size_t i = 0; i < max_n && i < bytes.size(); ++i)
                  out[i * static_cast<size_t>(elem_sz)] = static_cast<unsigned char>(bytes[i]);
                if (bytes.size() < max_n) out[bytes.size() * static_cast<size_t>(elem_sz)] = 0;
                return true;
              }
            }
            std::vector<unsigned char> elem;
            if (encode_bytes(elem_ts, cur_init, elem) && !elem.empty()) {
              const size_t copy_n = std::min<size_t>(static_cast<size_t>(elem_sz), elem.size());
              std::memcpy(out.data(), elem.data(), copy_n);
              return true;
            }
            return false;
          }
          if (const auto* list = std::get_if<InitList>(&cur_init)) {
            long long next_idx = 0;
            for (const auto& item : list->items) {
              long long idx = next_idx;
              if (item.index_designator && *item.index_designator >= 0) idx = *item.index_designator;
              if (idx >= 0 && idx < n) {
                std::vector<unsigned char> elem;
                GlobalInit child = child_init_of(item);
                if (encode_bytes(elem_ts, child, elem)) {
                  const size_t off = static_cast<size_t>(idx) * static_cast<size_t>(elem_sz);
                  const size_t copy_n = std::min<size_t>(static_cast<size_t>(elem_sz), elem.size());
                  std::memcpy(out.data() + off, elem.data(), copy_n);
                }
              }
              next_idx = idx + 1;
            }
            return true;
          }
          return true;
        }

        if ((cur_ts.base == TB_STRUCT || cur_ts.base == TB_UNION) && cur_ts.tag && cur_ts.tag[0]) {
          const auto sit = mod_.struct_defs.find(cur_ts.tag);
          if (sit == mod_.struct_defs.end()) return false;
          const HirStructDef& cur_sd = sit->second;
          if (cur_sd.is_union) {
            size_t fi = 0;
            GlobalInit child = GlobalInit(std::monostate{});
            bool has_child = false;
            if (const auto* list = std::get_if<InitList>(&cur_init)) {
              if (!list->items.empty()) {
                const auto& item0 = list->items.front();
                bool matched_union_field = false;
                if (item0.field_designator) {
                  const auto fit = std::find_if(
                      cur_sd.fields.begin(), cur_sd.fields.end(),
                      [&](const HirStructField& f) { return f.name == *item0.field_designator; });
                  if (fit != cur_sd.fields.end()) {
                    fi = static_cast<size_t>(std::distance(cur_sd.fields.begin(), fit));
                    child = child_init_of(item0);
                    has_child = true;
                    matched_union_field = true;
                  }
                } else if (item0.index_designator && *item0.index_designator >= 0) {
                  fi = static_cast<size_t>(*item0.index_designator);
                  child = child_init_of(item0);
                  has_child = true;
                  matched_union_field = true;
                }
                if (!matched_union_field) {
                  fi = 0;
                  if (list->items.size() == 1 &&
                      std::holds_alternative<std::shared_ptr<InitList>>(item0.value)) {
                    child = GlobalInit(*std::get<std::shared_ptr<InitList>>(item0.value));
                  } else {
                    child = GlobalInit(*list);
                  }
                  has_child = true;
                }
              }
            } else if (std::holds_alternative<InitScalar>(cur_init)) {
              child = cur_init;
              has_child = true;
            }
            if (!has_child || fi >= cur_sd.fields.size()) return true;
            const HirStructField& fld = cur_sd.fields[fi];
            TypeSpec fts = field_decl_type(fld);
            std::vector<unsigned char> fb;
            if (!encode_bytes(fts, child, fb)) return false;
            const size_t copy_n = std::min(out.size(), fb.size());
            std::memcpy(out.data(), fb.data(), copy_n);
            return true;
          }

          if (const auto* list = std::get_if<InitList>(&cur_init)) {
            size_t next_field = 0;
            size_t offset = 0;
            std::vector<size_t> field_offsets(cur_sd.fields.size(), 0);
            for (size_t i = 0; i < cur_sd.fields.size(); ++i) {
              field_offsets[i] = offset;
              offset += static_cast<size_t>(std::max(1, compute_field_size(mod_, cur_sd.fields[i])));
            }
            for (const auto& item : list->items) {
              size_t fi = next_field;
              if (item.field_designator) {
                const auto fit = std::find_if(
                    cur_sd.fields.begin(), cur_sd.fields.end(),
                    [&](const HirStructField& f) { return f.name == *item.field_designator; });
                if (fit == cur_sd.fields.end()) continue;
                fi = static_cast<size_t>(std::distance(cur_sd.fields.begin(), fit));
              } else if (item.index_designator && *item.index_designator >= 0) {
                fi = static_cast<size_t>(*item.index_designator);
              }
              if (fi >= cur_sd.fields.size()) continue;
              std::vector<unsigned char> fb;
              GlobalInit child = child_init_of(item);
              if (encode_bytes(field_decl_type(cur_sd.fields[fi]), child, fb)) {
                const size_t off = field_offsets[fi];
                const size_t copy_n = std::min(out.size() - off, fb.size());
                std::memcpy(out.data() + off, fb.data(), copy_n);
              }
              next_field = fi + 1;
            }
            return true;
          }

          if (const auto* scalar = std::get_if<InitScalar>(&cur_init)) {
            if (!cur_sd.fields.empty()) {
              std::vector<unsigned char> fb;
              if (encode_bytes(field_decl_type(cur_sd.fields[0]), GlobalInit(*scalar), fb)) {
                const size_t copy_n = std::min(out.size(), fb.size());
                std::memcpy(out.data(), fb.data(), copy_n);
              }
            }
            return true;
          }
          return true;
        }

        if (const auto* scalar = std::get_if<InitScalar>(&cur_init)) {
          const Expr& e = get_expr(scalar->expr);
          unsigned long long v = 0;
          if (const auto* il = std::get_if<IntLiteral>(&e.payload)) v = static_cast<unsigned long long>(il->value);
          else if (const auto* cl = std::get_if<CharLiteral>(&e.payload)) v = static_cast<unsigned long long>(cl->value);
          else return false;
          for (int i = 0; i < cur_sz; ++i)
            out[static_cast<size_t>(i)] = static_cast<unsigned char>((v >> (8 * i)) & 0xFF);
          return true;
        }
        if (const auto* list = std::get_if<InitList>(&cur_init)) {
          if (!list->items.empty()) {
            std::vector<unsigned char> inner;
            if (encode_bytes(cur_ts, child_init_of(list->items.front()), inner)) {
              const size_t copy_n = std::min(out.size(), inner.size());
              std::memcpy(out.data(), inner.data(), copy_n);
              return true;
            }
            return false;
          }
        }
        return true;
      };

      auto emit_union_from_field =
          [&](size_t field_idx, const GlobalInit& child_init) -> std::optional<std::string> {
        if (field_idx >= sd.fields.size()) return std::nullopt;
        std::vector<unsigned char> bytes(static_cast<size_t>(sz), 0);
        std::vector<unsigned char> field_bytes;
        if (!encode_bytes(field_decl_type(sd.fields[field_idx]), child_init, field_bytes))
          return std::nullopt;
        if (field_bytes.empty()) return std::nullopt;
        const size_t copy_n = std::min(bytes.size(), field_bytes.size());
        std::memcpy(bytes.data(), field_bytes.data(), copy_n);
        const bool all_zero = std::all_of(bytes.begin(), bytes.end(), [](unsigned char b) { return b == 0; });
        if (all_zero) return zero_union();
        std::string arr = "[";
        for (size_t i = 0; i < bytes.size(); ++i) {
          if (i) arr += ", ";
          arr += "i8 " + std::to_string(static_cast<unsigned int>(bytes[i]));
        }
        arr += "]";
        return "{ [" + std::to_string(sz) + " x i8] " + arr + " }";
      };

      if (const auto* list = std::get_if<InitList>(&init)) {
        if (!list->items.empty() && !sd.fields.empty()) {
          const InitListItem& item0 = list->items.front();
          size_t idx = 0;
          bool matched_union_field = false;
          GlobalInit child_init = GlobalInit(std::monostate{});
          if (item0.field_designator) {
            const auto fit = std::find_if(
                sd.fields.begin(), sd.fields.end(),
                [&](const HirStructField& f) { return f.name == *item0.field_designator; });
            if (fit != sd.fields.end()) {
              idx = static_cast<size_t>(std::distance(sd.fields.begin(), fit));
              child_init = child_init_of(item0);
              matched_union_field = true;
            }
          } else if (item0.index_designator && *item0.index_designator >= 0) {
            idx = static_cast<size_t>(*item0.index_designator);
            child_init = child_init_of(item0);
            matched_union_field = true;
          }
          if (!matched_union_field) {
            idx = 0;
            if (list->items.size() == 1 &&
                std::holds_alternative<std::shared_ptr<InitList>>(item0.value)) {
              child_init = GlobalInit(*std::get<std::shared_ptr<InitList>>(item0.value));
            } else {
              child_init = GlobalInit(*list);
            }
          }
          if (auto out = emit_union_from_field(idx, child_init)) return *out;
        }
        return zero_union();
      }

      if (const auto* scalar = std::get_if<InitScalar>(&init)) {
        const Expr& se = get_expr(scalar->expr);
        long long val = 0;
        if (const auto* il = std::get_if<IntLiteral>(&se.payload)) val = il->value;
        else if (const auto* cl = std::get_if<CharLiteral>(&se.payload)) val = cl->value;
        if (val != 0) {
          std::string bytes(static_cast<size_t>(sz), '\0');
          for (int i = 0; i < sz; ++i)
            bytes[static_cast<size_t>(i)] = static_cast<char>((val >> (8 * i)) & 0xFF);
          return "{ [" + std::to_string(sz) + " x i8] c\"" + escape_llvm_c_bytes(bytes) + "\" }";
        }
      }
      return zero_union();
    }

    const auto field_vals = emit_const_struct_fields(ts, sd, init);

    // Check if struct has any bitfields
    bool has_bitfields = false;
    for (const auto& f : sd.fields) {
      if (f.bit_width >= 0) { has_bitfields = true; break; }
    }

    if (!has_bitfields) {
      // No bitfields: emit one value per field as before
      std::string out = "{ ";
      for (size_t i = 0; i < sd.fields.size(); ++i) {
        if (i) out += ", ";
        out += llvm_field_ty(sd.fields[i]) + " " + field_vals[i];
      }
      out += " }";
      return out;
    }

    // Has bitfields: group by llvm_idx and pack bitfield values into storage units
    // Build list of unique LLVM fields
    struct LlvmField {
      int llvm_idx;
      std::string type_str;
      std::vector<size_t> c_field_indices;  // C field indices sharing this llvm_idx
    };
    std::vector<LlvmField> llvm_fields;
    int last_idx = -1;
    for (size_t i = 0; i < sd.fields.size(); ++i) {
      if (sd.fields[i].llvm_idx != last_idx) {
        last_idx = sd.fields[i].llvm_idx;
        llvm_fields.push_back({last_idx, llvm_field_ty(sd.fields[i]), {}});
      }
      llvm_fields.back().c_field_indices.push_back(i);
    }

    std::string out = "{ ";
    for (size_t li = 0; li < llvm_fields.size(); ++li) {
      if (li) out += ", ";
      const auto& lf = llvm_fields[li];
      out += lf.type_str + " ";

      if (lf.c_field_indices.size() == 1 && sd.fields[lf.c_field_indices[0]].bit_width < 0) {
        // Single non-bitfield: use value directly
        out += field_vals[lf.c_field_indices[0]];
      } else {
        // Pack bitfield values into storage unit integer
        unsigned long long packed = 0;
        for (size_t ci : lf.c_field_indices) {
          const auto& f = sd.fields[ci];
          if (f.bit_width <= 0) continue;  // skip zero-width or non-bitfield
          // Parse the integer value from field_vals string
          long long val = 0;
          try { val = std::stoll(field_vals[ci]); } catch (...) {}
          // Mask to bit_width bits and shift into position
          unsigned long long mask = (f.bit_width >= 64) ? ~0ULL : ((1ULL << f.bit_width) - 1);
          packed |= (static_cast<unsigned long long>(val) & mask) << f.bit_offset;
        }
        out += std::to_string(static_cast<long long>(packed));
      }
    }
    out += " }";
    return out;
  }

  std::string emit_const_init(const TypeSpec& ts, const GlobalInit& init) {
    if (ts.array_rank > 0) return emit_const_array(ts, init);
    if (ts.base == TB_VA_LIST && ts.ptr_level == 0) return "zeroinitializer";
    if ((ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.ptr_level == 0)
      return emit_const_struct(ts, init);
    if (const auto* s = std::get_if<InitScalar>(&init))
      return emit_const_scalar_expr(s->expr, ts);
    if (const auto* list = std::get_if<InitList>(&init)) {
      if (!list->items.empty()) {
        const auto& first = list->items.front();
        GlobalInit child_init = std::visit([&](const auto& v) -> GlobalInit {
          using V = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<V, InitScalar>) return GlobalInit(v);
          else return GlobalInit(*v);
        }, first.value);
        return emit_const_init(ts, child_init);
      }
    }
    return (llvm_ty(ts) == "ptr") ? "null" : "zeroinitializer";
  }

  // Compute the number of flat scalar initializers an aggregate type consumes
  // when brace elision is in effect.
  long long flat_scalar_count(const TypeSpec& ts) const {
    if (ts.array_rank > 0) {
      long long n = ts.array_size;
      if (n <= 0) return 1;
      return n * flat_scalar_count(drop_one_array_dim(ts));
    }
    if ((ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.ptr_level == 0 && ts.tag) {
      const auto it = mod_.struct_defs.find(ts.tag);
      if (it == mod_.struct_defs.end()) return 1;
      const auto& sd = it->second;
      if (sd.is_union) return sd.fields.empty() ? 1 : flat_scalar_count(field_decl_type(sd.fields[0]));
      long long count = 0;
      for (const auto& f : sd.fields) count += flat_scalar_count(field_decl_type(f));
      return count > 0 ? count : 1;
    }
    return 1;
  }

  // Deduce the first dimension of an unsized array from its initializer.
  // Returns -1 if it cannot be deduced.
  long long deduce_array_size_from_init(const GlobalInit& init) {
    if (const auto* list = std::get_if<InitList>(&init)) {
      long long max_idx = -1;
      long long next = 0;
      for (const auto& item : list->items) {
        long long idx = next;
        if (item.index_designator && *item.index_designator >= 0)
          idx = *item.index_designator;
        if (idx > max_idx) max_idx = idx;
        next = idx + 1;
      }
      return max_idx + 1;  // 0 items → -1+1=0, handled as <=0 (zeroinitializer)
    }
    if (const auto* scalar = std::get_if<InitScalar>(&init)) {
      const Expr& e = get_expr(scalar->expr);
      if (const auto* sl = std::get_if<StringLiteral>(&e.payload)) {
        const std::string bytes = bytes_from_string_literal(*sl);
        return (long long)bytes.size() + 1;  // include NUL
      }
    }
    return -1;
  }

  // Return a TypeSpec with the first array dimension filled in (if unsized).
  TypeSpec resolve_array_ts(const TypeSpec& ts, const GlobalInit& init) {
    if (ts.array_rank <= 0 || ts.array_size >= 0) return ts;
    long long deduced = deduce_array_size_from_init(init);
    if (deduced <= 0) return ts;
    // Account for brace elision: if element is aggregate and the flat item
    // count exceeds the raw item count, divide by per-element flat count.
    TypeSpec elem_ts = drop_one_array_dim(ts);
    bool elem_is_agg = (elem_ts.array_rank > 0) ||
        ((elem_ts.base == TB_STRUCT || elem_ts.base == TB_UNION) && elem_ts.ptr_level == 0);
    if (elem_is_agg) {
      if (const auto* list = std::get_if<InitList>(&init)) {
        // Only apply flat-count division when ALL items are scalars
        // (fully brace-elided). Mixed lists are handled by cursor-based
        // consumption in emit_const_array.
        bool all_scalar = true;
        for (const auto& item : list->items) {
          if (!std::holds_alternative<InitScalar>(item.value)) { all_scalar = false; break; }
        }
        if (all_scalar) {
          long long fc = flat_scalar_count(elem_ts);
          if (fc > 1) deduced = (deduced + fc - 1) / fc;
        }
      }
    }
    TypeSpec out = ts;
    out.array_size = deduced;
    out.array_dims[0] = deduced;
    return out;
  }

  void emit_global(const GlobalVar& gv) {
    // Resolve unsized array dimensions from the initializer (e.g. int a[] = {1,2,3}).
    const TypeSpec ts = resolve_array_ts(gv.type.spec, gv.init);
    if (!gv.linkage.is_extern &&
        ts.ptr_level == 0 &&
        ts.array_rank == 0 &&
        ts.tag && ts.tag[0] &&
        ts.base == TB_STRUCT) {
      const auto it = mod_.struct_defs.find(ts.tag);
      if (it != mod_.struct_defs.end()) {
        const HirStructDef& sd = it->second;
        if (!sd.is_union && !sd.fields.empty() && sd.fields.back().array_first_dim == 0) {
          std::vector<TypeSpec> field_types;
          const auto field_vals = emit_const_struct_fields(ts, sd, gv.init, &field_types);
          const TypeSpec& last_ts = field_types.back();
          if (last_ts.array_rank > 0 && last_ts.array_size > 0) {
            const std::string lk = gv.linkage.is_static ? "internal " : "";
            const std::string qual = gv.is_const ? "constant " : "global ";
            std::string literal_ty = "{ ";
            std::string literal_init = "{ ";
            for (size_t i = 0; i < sd.fields.size(); ++i) {
              if (i) {
                literal_ty += ", ";
                literal_init += ", ";
              }
              literal_ty += llvm_alloca_ty(field_types[i]);
              literal_init += llvm_alloca_ty(field_types[i]) + " " + field_vals[i];
            }
            literal_ty += " }";
            literal_init += " }";
            preamble_ << "@" << gv.name << " = " << lk << qual
                      << literal_ty << " " << literal_init << "\n";
            return;
          }
        }
      }
    }
    const std::string ty = llvm_alloca_ty(ts);
    if (gv.linkage.is_extern) {
      preamble_ << "@" << gv.name << " = external global " << ty << "\n";
      return;
    }
    const std::string lk = gv.linkage.is_static ? "internal " : "";
    const std::string qual = gv.is_const ? "constant " : "global ";
    const std::string init = emit_const_init(ts, gv.init);
    preamble_ << "@" << gv.name << " = " << lk << qual << ty << " " << init << "\n";
  }

  // ── Expr lookup ───────────────────────────────────────────────────────────

  const Expr& get_expr(ExprId id) const {
    for (const auto& e : mod_.expr_pool)
      if (e.id.value == id.value) return e;
    throw std::runtime_error("HirEmitter: expr not found id=" + std::to_string(id.value));
  }

  // ── Coerce ────────────────────────────────────────────────────────────────

  std::string coerce(FnCtx& ctx, const std::string& val,
                     const TypeSpec& from_ts, const TypeSpec& to_ts) {
    const std::string ft = llvm_ty(from_ts);
    const std::string tt = llvm_ty(to_ts);
    if (tt == "ptr" && val == "0") return "null";
    if (ft == tt) return val;
    if (ft == "ptr" && tt == "ptr") return val;

    if (is_complex_base(from_ts.base) && is_complex_base(to_ts.base)) {
      const TypeSpec from_elem_ts = complex_component_ts(from_ts.base);
      const TypeSpec to_elem_ts = complex_component_ts(to_ts.base);
      const std::string real0 = fresh_tmp(ctx);
      emit_instr(ctx, real0 + " = extractvalue " + ft + " " + val + ", 0");
      const std::string imag0 = fresh_tmp(ctx);
      emit_instr(ctx, imag0 + " = extractvalue " + ft + " " + val + ", 1");
      const std::string real1 = coerce(ctx, real0, from_elem_ts, to_elem_ts);
      const std::string imag1 = coerce(ctx, imag0, from_elem_ts, to_elem_ts);
      const std::string with_real = fresh_tmp(ctx);
      emit_instr(ctx, with_real + " = insertvalue " + tt + " undef, " + llvm_ty(to_elem_ts) +
                                 " " + real1 + ", 0");
      const std::string out = fresh_tmp(ctx);
      emit_instr(ctx, out + " = insertvalue " + tt + " " + with_real + ", " +
                            llvm_ty(to_elem_ts) + " " + imag1 + ", 1");
      return out;
    }

    if (!is_complex_base(from_ts.base) && is_complex_base(to_ts.base) &&
        from_ts.ptr_level == 0 && from_ts.array_rank == 0) {
      const TypeSpec to_elem_ts = complex_component_ts(to_ts.base);
      const std::string real = coerce(ctx, val, from_ts, to_elem_ts);
      const std::string elem_ty = llvm_ty(to_elem_ts);
      const std::string zero =
          is_float_base(to_elem_ts.base) ? fp_literal(to_elem_ts.base, 0.0) : "0";
      const std::string with_real = fresh_tmp(ctx);
      emit_instr(ctx, with_real + " = insertvalue " + tt + " undef, " + elem_ty +
                                 " " + real + ", 0");
      const std::string out = fresh_tmp(ctx);
      emit_instr(ctx, out + " = insertvalue " + tt + " " + with_real + ", " +
                            elem_ty + " " + zero + ", 1");
      return out;
    }

    // i1 → wider int
    if (ft == "i1" && tt != "ptr" && tt != "void") {
      const std::string tmp = fresh_tmp(ctx);
      emit_instr(ctx, tmp + " = zext i1 " + val + " to " + tt);
      return tmp;
    }

    // int → int
    if (from_ts.ptr_level == 0 && from_ts.array_rank == 0 &&
        to_ts.ptr_level == 0 && to_ts.array_rank == 0 &&
        is_any_int(from_ts.base) && is_any_int(to_ts.base)) {
      const int fb = int_bits(from_ts.base), tb = int_bits(to_ts.base);
      if (fb == tb) return val;
      const std::string tmp = fresh_tmp(ctx);
      if (tb > fb) {
        const std::string ext = is_signed_int(from_ts.base) ? "sext" : "zext";
        emit_instr(ctx, tmp + " = " + ext + " " + ft + " " + val + " to " + tt);
      } else {
        emit_instr(ctx, tmp + " = trunc " + ft + " " + val + " to " + tt);
      }
      return tmp;
    }

    // float → float
    if (is_float_base(from_ts.base) && is_float_base(to_ts.base)) {
      const int fb = (from_ts.base == TB_FLOAT) ? 32 : (from_ts.base == TB_LONGDOUBLE ? 128 : 64);
      const int tb = (to_ts.base == TB_FLOAT) ? 32 : (to_ts.base == TB_LONGDOUBLE ? 128 : 64);
      if (fb == tb) return val;
      const std::string tmp = fresh_tmp(ctx);
      const std::string op = (tb > fb) ? "fpext" : "fptrunc";
      emit_instr(ctx, tmp + " = " + op + " " + ft + " " + val + " to " + tt);
      return tmp;
    }

    // int → float (only for non-pointer scalar float types)
    if (is_any_int(from_ts.base) && from_ts.ptr_level == 0 &&
        is_float_base(to_ts.base) && to_ts.ptr_level == 0 && to_ts.array_rank == 0) {
      const std::string tmp = fresh_tmp(ctx);
      const std::string op = is_signed_int(from_ts.base) ? "sitofp" : "uitofp";
      emit_instr(ctx, tmp + " = " + op + " " + ft + " " + val + " to " + tt);
      return tmp;
    }

    // float → int (only for non-pointer scalar types)
    if (is_float_base(from_ts.base) && from_ts.ptr_level == 0 && from_ts.array_rank == 0 &&
        is_any_int(to_ts.base) && to_ts.ptr_level == 0) {
      const std::string tmp = fresh_tmp(ctx);
      const std::string op = is_signed_int(to_ts.base) ? "fptosi" : "fptoui";
      emit_instr(ctx, tmp + " = " + op + " " + ft + " " + val + " to " + tt);
      return tmp;
    }

    // ptr ↔ int
    if (ft == "ptr" && is_any_int(to_ts.base)) {
      const std::string tmp = fresh_tmp(ctx);
      emit_instr(ctx, tmp + " = ptrtoint ptr " + val + " to " + tt);
      return tmp;
    }
    if (is_any_int(from_ts.base) && tt == "ptr") {
      const std::string tmp = fresh_tmp(ctx);
      emit_instr(ctx, tmp + " = inttoptr " + ft + " " + val + " to ptr");
      return tmp;
    }

    // Same size numeric: bitcast
    return val;
  }

  // ── to_bool: convert any value to i1 ─────────────────────────────────────

  std::string to_bool(FnCtx& ctx, const std::string& val, const TypeSpec& ts) {
    const std::string ty = llvm_ty(ts);
    if (ty == "i1") return val;
    const std::string tmp = fresh_tmp(ctx);
    // Check ptr FIRST: a TypeSpec with ptr_level>0 always uses icmp even if the
    // base is a float type (e.g. function pointer typed as float-return + ptr_level=1).
    if (ty == "ptr") {
      emit_instr(ctx, tmp + " = icmp ne ptr " + val + ", null");
    } else if (is_float_base(ts.base) && ts.ptr_level == 0 && ts.array_rank == 0) {
      emit_instr(ctx, tmp + " = fcmp une " + ty + " " + val + ", " + fp_literal(ts.base, 0.0));
    } else {
      emit_instr(ctx, tmp + " = icmp ne " + ty + " " + val + ", 0");
    }
    return tmp;
  }

  // ── Struct field lookup ───────────────────────────────────────────────────

  struct FieldStep {
    std::string tag;
    int llvm_idx = 0;
    bool is_union = false;
    // Bitfield metadata (bit_width >= 0 means this is a bitfield access)
    int bit_width = -1;
    int bit_offset = 0;
    int storage_unit_bits = 0;
    bool bf_is_signed = false;
  };

  // Recursively find field_name in struct/union identified by tag.
  // On success, appends steps to chain (outermost first).
  // Returns true and sets out_field_ts.
  bool find_field_chain(const std::string& tag, const std::string& field_name,
                        std::vector<FieldStep>& chain, TypeSpec& out_field_ts) {
    const auto it = mod_.struct_defs.find(tag);
    if (it == mod_.struct_defs.end()) return false;
    const HirStructDef& sd = it->second;

    // Direct lookup
    for (const auto& f : sd.fields) {
      if (!f.is_anon_member && f.name == field_name) {
        FieldStep step{tag, f.llvm_idx, sd.is_union};
        if (f.bit_width >= 0) {
          step.bit_width = f.bit_width;
          step.bit_offset = f.bit_offset;
          step.storage_unit_bits = f.storage_unit_bits;
          step.bf_is_signed = f.is_bf_signed;
        }
        chain.push_back(std::move(step));
        out_field_ts = field_decl_type(f);  // includes array dims if array field
        return true;
      }
    }
    // Recursive: search anonymous embedded members
    for (const auto& f : sd.fields) {
      if (!f.is_anon_member) continue;
      if (!f.elem_type.tag || !f.elem_type.tag[0]) continue;
      std::vector<FieldStep> sub_chain;
      TypeSpec sub_ts{};
      if (find_field_chain(f.elem_type.tag, field_name, sub_chain, sub_ts)) {
        // prepend: current struct's step to reach the anon member
        chain.push_back({tag, f.llvm_idx, sd.is_union});
        for (const auto& s : sub_chain) chain.push_back(s);
        out_field_ts = sub_ts;
        return true;
      }
    }
    return false;
  }

  // Emit GEP chain for struct member access.
  // base_ptr: LLVM value (ptr to struct), tag: struct type tag, field_name: C field name.
  // Returns LLVM value (ptr to field), sets out_field_ts.
  std::string emit_member_gep(FnCtx& ctx, const std::string& base_ptr,
                               const std::string& tag, const std::string& field_name,
                               TypeSpec& out_field_ts,
                               BitfieldAccess* out_bf = nullptr) {
    std::vector<FieldStep> chain;
    if (!find_field_chain(tag, field_name, chain, out_field_ts)) {
      throw std::runtime_error(
          "HirEmitter: field '" + field_name + "' not found in struct/union '" + tag + "'");
    }
    std::string cur_ptr = base_ptr;
    for (const auto& step : chain) {
      if (step.is_union) {
        // Union: GEP to field 0 (byte array); with opaque ptrs same addr
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = getelementptr %struct." + step.tag +
                           ", ptr " + cur_ptr + ", i32 0, i32 0");
        cur_ptr = tmp;
      } else {
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = getelementptr %struct." + step.tag +
                           ", ptr " + cur_ptr + ", i32 0, i32 " +
                           std::to_string(step.llvm_idx));
        cur_ptr = tmp;
      }
    }
    // Propagate bitfield info from the last (innermost) step
    if (out_bf && !chain.empty()) {
      const auto& last = chain.back();
      out_bf->bit_width = last.bit_width;
      out_bf->bit_offset = last.bit_offset;
      out_bf->storage_unit_bits = last.storage_unit_bits;
      out_bf->is_signed = last.bf_is_signed;
    }
    return cur_ptr;
  }

  // ── Bitfield load/store helpers ─────────────────────────────────────────────

  // Determine the promoted LLVM type for a bitfield.
  // C integer promotion: if bit_width fits in int (<=31 unsigned, <=32 signed),
  // promote to i32. Otherwise keep the storage unit type (e.g. i64).
  static int bitfield_promoted_bits(const BitfieldAccess& bf) {
    if (bf.bit_width <= 31) return 32;  // fits in int
    if (bf.bit_width == 32) return 32;  // fits in int (signed) or uint (unsigned)
    return bf.storage_unit_bits;        // >32 bits: use storage type (i64)
  }

  // Load a bitfield value from a storage unit pointer.
  // Returns a value of the promoted type (i32 for <=32-bit fields, i64 for wider).
  std::string emit_bitfield_load(FnCtx& ctx, const std::string& unit_ptr,
                                  const BitfieldAccess& bf) {
    const std::string unit_ty = "i" + std::to_string(bf.storage_unit_bits);
    // Load the full storage unit
    const std::string unit = fresh_tmp(ctx);
    emit_instr(ctx, unit + " = load " + unit_ty + ", ptr " + unit_ptr);
    // Shift right to align bitfield to bit 0
    std::string shifted = unit;
    if (bf.bit_offset > 0) {
      shifted = fresh_tmp(ctx);
      emit_instr(ctx, shifted + " = lshr " + unit_ty + " " + unit +
                          ", " + std::to_string(bf.bit_offset));
    }
    // Mask to bit_width bits
    const unsigned long long mask = (bf.bit_width >= 64)
        ? ~0ULL : ((1ULL << bf.bit_width) - 1);
    const std::string masked = fresh_tmp(ctx);
    emit_instr(ctx, masked + " = and " + unit_ty + " " + shifted +
                        ", " + std::to_string(mask));
    std::string result = masked;
    // Sign-extend if needed
    if (bf.is_signed && bf.bit_width < bf.storage_unit_bits) {
      const int shift_amt = bf.storage_unit_bits - bf.bit_width;
      const std::string shl = fresh_tmp(ctx);
      emit_instr(ctx, shl + " = shl " + unit_ty + " " + masked +
                          ", " + std::to_string(shift_amt));
      result = fresh_tmp(ctx);
      emit_instr(ctx, result + " = ashr " + unit_ty + " " + shl +
                          ", " + std::to_string(shift_amt));
    }
    // Truncate or extend to promoted type
    const int promoted_bits = bitfield_promoted_bits(bf);
    const std::string promoted_ty = "i" + std::to_string(promoted_bits);
    if (bf.storage_unit_bits != promoted_bits) {
      const std::string promoted = fresh_tmp(ctx);
      if (bf.storage_unit_bits > promoted_bits) {
        emit_instr(ctx, promoted + " = trunc " + unit_ty + " " + result + " to " + promoted_ty);
      } else {
        emit_instr(ctx, promoted + " = " +
                        (bf.is_signed ? "sext " : "zext ") +
                        unit_ty + " " + result + " to " + promoted_ty);
      }
      return promoted;
    }
    return result;
  }

  // Store a value into a bitfield within a storage unit.
  void emit_bitfield_store(FnCtx& ctx, const std::string& unit_ptr,
                            const BitfieldAccess& bf,
                            const std::string& new_val, const TypeSpec& val_ts) {
    const std::string unit_ty = "i" + std::to_string(bf.storage_unit_bits);
    // Coerce new_val to storage unit type
    TypeSpec unit_ts{};
    switch (bf.storage_unit_bits) {
      case 8:  unit_ts.base = TB_UCHAR; break;
      case 16: unit_ts.base = TB_USHORT; break;
      case 32: unit_ts.base = TB_UINT; break;
      case 64: unit_ts.base = TB_ULONGLONG; break;
      default: unit_ts.base = TB_UINT; break;
    }
    std::string val_coerced = coerce(ctx, new_val, val_ts, unit_ts);
    // Load current storage unit
    const std::string old_unit = fresh_tmp(ctx);
    emit_instr(ctx, old_unit + " = load " + unit_ty + ", ptr " + unit_ptr);
    // Create mask to clear the bitfield bits
    const unsigned long long field_mask_val = (bf.bit_width >= 64)
        ? ~0ULL : ((1ULL << bf.bit_width) - 1);
    const unsigned long long clear_mask = ~(field_mask_val << bf.bit_offset);
    const std::string cleared = fresh_tmp(ctx);
    emit_instr(ctx, cleared + " = and " + unit_ty + " " + old_unit +
                        ", " + std::to_string(static_cast<long long>(clear_mask)));
    // Mask and shift new value into position
    const std::string new_masked = fresh_tmp(ctx);
    emit_instr(ctx, new_masked + " = and " + unit_ty + " " + val_coerced +
                        ", " + std::to_string(field_mask_val));
    std::string new_shifted = new_masked;
    if (bf.bit_offset > 0) {
      new_shifted = fresh_tmp(ctx);
      emit_instr(ctx, new_shifted + " = shl " + unit_ty + " " + new_masked +
                          ", " + std::to_string(bf.bit_offset));
    }
    // Combine
    const std::string combined = fresh_tmp(ctx);
    emit_instr(ctx, combined + " = or " + unit_ty + " " + cleared + ", " + new_shifted);
    emit_instr(ctx, "store " + unit_ty + " " + combined + ", ptr " + unit_ptr);
  }

  // ── Lvalue emission ───────────────────────────────────────────────────────

  std::string emit_lval(FnCtx& ctx, ExprId id, TypeSpec& pointee_ts) {
    const Expr& e = get_expr(id);
    return emit_lval_dispatch(ctx, e, pointee_ts);
  }

  std::string emit_va_list_obj_ptr(FnCtx& ctx, ExprId id, TypeSpec& ts) {
    const Expr& e = get_expr(id);
    ts = resolve_expr_type(ctx, id);
    if (const auto* r = std::get_if<DeclRef>(&e.payload)) {
      if (r->param_index && ctx.fn && *r->param_index < ctx.fn->params.size()) {
        const TypeSpec& pts = ctx.fn->params[*r->param_index].type.spec;
        if (pts.base == TB_VA_LIST && pts.ptr_level == 0 && pts.array_rank == 0) {
          const auto spill_it = ctx.param_slots.find(*r->param_index + 0x80000000u);
          if (spill_it != ctx.param_slots.end()) {
            const std::string tmp = fresh_tmp(ctx);
            emit_instr(ctx, tmp + " = load ptr, ptr " + spill_it->second);
            return tmp;
          }
          const auto it = ctx.param_slots.find(*r->param_index);
          if (it != ctx.param_slots.end()) return it->second;
        }
      }
    }
    return emit_lval(ctx, id, ts);
  }

  std::string emit_lval_dispatch(FnCtx& ctx, const Expr& e, TypeSpec& pts) {
    if (const auto* r = std::get_if<DeclRef>(&e.payload)) {
      if (r->local) {
        pts = ctx.local_types.at(r->local->value);
        return ctx.local_slots.at(r->local->value);
      }
      // Parameter used as lvalue (e.g. p++): spill to a stack slot on first use
      if (r->param_index && ctx.fn && *r->param_index < ctx.fn->params.size()) {
        const auto& param = ctx.fn->params[*r->param_index];
        pts = param.type.spec;
        const std::string pname = "%p." + sanitize_llvm_ident(param.name);
        // Check if we already have a spill slot
        auto it = ctx.param_slots.find(*r->param_index + 0x80000000u);
        if (it != ctx.param_slots.end()) {
          return it->second;
        }
        // Create a new alloca for this parameter
        const std::string slot = "%lv.param." + sanitize_llvm_ident(param.name);
        ctx.alloca_lines.push_back("  " + slot + " = alloca " + llvm_alloca_ty(pts));
        ctx.alloca_lines.push_back("  store " + llvm_ty(pts) + " " + pname + ", ptr " + slot);
        ctx.param_slots[*r->param_index + 0x80000000u] = slot;
        return slot;
      }
      if (r->global) {
        size_t gv_idx = r->global->value;
        const auto& gv0 = mod_.globals[gv_idx];
        // For incomplete array declarations (e.g. `extern T arr[]`), find the
        // definition with a deducible size so GEP uses the correct element type.
        if (gv0.type.spec.array_rank > 0 && gv0.type.spec.array_size < 0) {
          for (size_t gi = 0; gi < mod_.globals.size(); ++gi) {
            const auto& g = mod_.globals[gi];
            if (g.name == gv0.name && g.type.spec.array_rank > 0) {
              const TypeSpec rs = resolve_array_ts(g.type.spec, g.init);
              if (rs.array_size > 0) { gv_idx = gi; break; }
            }
          }
        }
        const auto& gv = mod_.globals[gv_idx];
        pts = resolve_array_ts(gv.type.spec, gv.init);
        return "@" + gv.name;
      }
      // Unresolved: assume external global
      pts = e.type.spec;
      return "@" + r->name;
    }
    if (const auto* u = std::get_if<UnaryExpr>(&e.payload)) {
      if (u->op == UnaryOp::Deref) {
        TypeSpec ptr_ts{};
        const std::string ptr = emit_rval_id(ctx, u->operand, ptr_ts);
        pts = ptr_ts;
        if (pts.ptr_level > 0) pts.ptr_level--;
        return ptr;
      }
      if (u->op == UnaryOp::RealPart || u->op == UnaryOp::ImagPart) {
        TypeSpec complex_ts{};
        const std::string complex_ptr = emit_lval(ctx, u->operand, complex_ts);
        if (!is_complex_base(complex_ts.base)) {
          throw std::runtime_error("HirEmitter: real/imag lvalue on non-complex expr");
        }
        pts = complex_component_ts(complex_ts.base);
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = getelementptr " + llvm_alloca_ty(complex_ts) +
                            ", ptr " + complex_ptr + ", i32 0, i32 " +
                            std::to_string(u->op == UnaryOp::ImagPart ? 1 : 0));
        return tmp;
      }
    }
    if (const auto* idx = std::get_if<IndexExpr>(&e.payload)) {
      TypeSpec base_ts{};
      const std::string base = emit_rval_id(ctx, idx->base, base_ts);
      TypeSpec ix_ts{};
      const std::string ix = emit_rval_id(ctx, idx->index, ix_ts);
      // Widen index to i64
      TypeSpec i64_ts{}; i64_ts.base = TB_LONGLONG;
      const std::string ix64 = coerce(ctx, ix, ix_ts, i64_ts);
      // Element type: strip one array or pointer level with proper dim shifting.
      pts = base_ts;
      if (pts.ptr_level > 0 && pts.is_ptr_to_array) {
        // Pointer-to-array: subscript consumes the pointer layer first.
        pts.ptr_level--;
      } else if (pts.array_rank > 0) {
        pts = drop_one_array_dim(pts);
      } else if (pts.ptr_level > 0) {
        pts.ptr_level--;
      }
      const std::string tmp = fresh_tmp(ctx);
      // Use alloca type (non-decayed) for array elements so GEP advances by the
      // correct stride ([N x T] instead of ptr).
      const std::string elem_ty = (pts.array_rank > 0)
                                      ? llvm_alloca_ty(pts)
                                      : llvm_ty(pts);
      const std::string safe_elem_ty = (elem_ty == "void") ? "i8" : elem_ty;
      emit_instr(ctx, tmp + " = getelementptr " + safe_elem_ty +
                          ", ptr " + base + ", i64 " + ix64);
      return tmp;
    }
    if (const auto* m = std::get_if<MemberExpr>(&e.payload)) {
      return emit_member_lval(ctx, *m, pts);
    }
    throw std::runtime_error("HirEmitter: cannot take lval of expr");
  }

  std::string emit_member_lval(FnCtx& ctx, const MemberExpr& m, TypeSpec& out_pts,
                                BitfieldAccess* out_bf = nullptr) {
    // Get base pointer
    const Expr& base_e = get_expr(m.base);
    TypeSpec base_ts = base_e.type.spec;
    if (base_ts.base == TB_VOID && base_ts.ptr_level == 0)
      base_ts = resolve_expr_type(ctx, m.base);

    std::string base_ptr;
    if (m.is_arrow) {
      // expr->field: load the pointer from the base expression
      TypeSpec ptr_ts{};
      base_ptr = emit_rval_id(ctx, m.base, ptr_ts);
      base_ts = ptr_ts;
      // Strip one pointer level for the struct type
      if (base_ts.ptr_level > 0) base_ts.ptr_level--;
    } else {
      // expr.field: prefer direct lvalue; for struct rvalues materialize a temp.
      TypeSpec dummy{};
      try {
        base_ptr = emit_lval_dispatch(ctx, base_e, dummy);
      } catch (const std::runtime_error&) {
        if (base_ts.base != TB_STRUCT && base_ts.base != TB_UNION) {
          throw;
        }
        TypeSpec rval_ts{};
        const std::string rval = emit_rval_id(ctx, m.base, rval_ts);
        if (rval_ts.base != TB_VOID || rval_ts.ptr_level > 0 || rval_ts.array_rank > 0) {
          base_ts = rval_ts;
        }
        const std::string slot = fresh_tmp(ctx) + ".agg";
        ctx.alloca_lines.push_back("  " + slot + " = alloca " + llvm_alloca_ty(base_ts));
        emit_instr(ctx, "store " + llvm_ty(base_ts) + " " + rval + ", ptr " + slot);
        base_ptr = slot;
      }
    }

    const char* tag = base_ts.tag;
    if (!tag || !tag[0]) {
      throw std::runtime_error(
          "HirEmitter: MemberExpr base has no struct tag (field='" + m.field + "')");
    }
    return emit_member_gep(ctx, base_ptr, tag, m.field, out_pts, out_bf);
  }

  // ── Rvalue emission ───────────────────────────────────────────────────────

  // Recursively resolve the C type of an expression from HIR structure.
  // The AST doesn't annotate types on NK_BINOP/NK_VAR nodes, so we infer.
  TypeSpec resolve_expr_type(FnCtx& ctx, ExprId id) {
    const Expr& e = get_expr(id);
    // Use stored type if not void
    const TypeSpec& ts = e.type.spec;
    if (ts.base != TB_VOID || ts.ptr_level > 0 || ts.array_rank > 0) return ts;

    return std::visit([&](const auto& p) -> TypeSpec {
      return resolve_payload_type(ctx, p);
    }, e.payload);
  }

  TypeSpec resolve_expr_type(FnCtx& ctx, const Expr& e) {
    if (has_concrete_type(e.type.spec)) return e.type.spec;
    return std::visit([&](const auto& p) -> TypeSpec {
      return resolve_payload_type(ctx, p);
    }, e.payload);
  }

  const FnPtrSig* resolve_callee_fn_ptr_sig(FnCtx& ctx, const Expr& callee_e) {
    if (const auto* u = std::get_if<UnaryExpr>(&callee_e.payload)) {
      if (u->op == UnaryOp::Deref) {
        const Expr& inner_e = get_expr(u->operand);
        return resolve_callee_fn_ptr_sig(ctx, inner_e);
      }
    }
    if (const auto* dr = std::get_if<DeclRef>(&callee_e.payload)) {
      if (dr->local) {
        const auto it = ctx.local_fn_ptr_sigs.find(dr->local->value);
        if (it != ctx.local_fn_ptr_sigs.end()) return &it->second;
      }
      if (dr->param_index) {
        const auto it = ctx.param_fn_ptr_sigs.find(*dr->param_index);
        if (it != ctx.param_fn_ptr_sigs.end()) return &it->second;
      }
      if (dr->global) {
        const auto it = ctx.global_fn_ptr_sigs.find(dr->global->value);
        if (it != ctx.global_fn_ptr_sigs.end()) return &it->second;
      }
    }
    return nullptr;
  }

  TypeSpec resolve_payload_type(FnCtx& ctx, const DeclRef& r) {
    if (r.local) {
      const auto it = ctx.local_types.find(r.local->value);
      if (it != ctx.local_types.end()) return it->second;
    }
    if (r.param_index && ctx.fn && *r.param_index < ctx.fn->params.size())
      return ctx.fn->params[*r.param_index].type.spec;
    if (r.global) {
      if (const GlobalVar* gv0 = mod_.find_global(*r.global)) {
        if (gv0->type.spec.array_rank > 0) {
          const GlobalVar* best = gv0;
          if (gv0->type.spec.array_size < 0) {
            for (const auto& g : mod_.globals) {
              if (g.name != gv0->name || g.type.spec.array_rank <= 0) continue;
              const TypeSpec rs = resolve_array_ts(g.type.spec, g.init);
              if (rs.array_size > 0) { best = &g; break; }
            }
          }
          return resolve_array_ts(best->type.spec, best->init);
        }
        return gv0->type.spec;
      }
    }
    // Function reference (resolved via fn_index, not global_index): treat as ptr.
    // Do NOT set is_fn_ptr here; that would cause call-return-type resolution to
    // decrement ptr_level and return void. The call-return fallback (implicit int)
    // handles the case correctly.
    if (!r.name.empty() && mod_.fn_index.count(r.name)) {
      const auto fit = mod_.fn_index.find(r.name);
      if (fit != mod_.fn_index.end() && fit->second.value < mod_.functions.size()) {
        TypeSpec ts = mod_.functions[fit->second.value].return_type.spec;
        ts.ptr_level++;
        ts.is_fn_ptr = true;
        return ts;
      }
    }
    return {};
  }

  TypeSpec resolve_payload_type(FnCtx& ctx, const BinaryExpr& b) {
    switch (b.op) {
      case BinaryOp::Lt:
      case BinaryOp::Le:
      case BinaryOp::Gt:
      case BinaryOp::Ge:
      case BinaryOp::Eq:
      case BinaryOp::Ne:
      case BinaryOp::LAnd:
      case BinaryOp::LOr: {
        TypeSpec ts{};
        ts.base = TB_INT;
        return ts;
      }
      case BinaryOp::Comma: {
        // Comma operator: result type/value is from RHS
        const Expr& rhs_e = get_expr(b.rhs);
        return std::visit([&](const auto& p) -> TypeSpec {
          return resolve_payload_type(ctx, p);
        }, rhs_e.payload);
      }
      default:
        break;
    }
    const TypeSpec lts = resolve_expr_type(ctx, b.lhs);
    const TypeSpec rts = resolve_expr_type(ctx, b.rhs);

    if (b.op == BinaryOp::Sub && llvm_ty(lts) == "ptr" && llvm_ty(rts) == "ptr") {
      TypeSpec ts{};
      ts.base = TB_LONGLONG;
      return ts;
    }
    if ((b.op == BinaryOp::Add || b.op == BinaryOp::Sub) &&
        llvm_ty(lts) == "ptr" &&
        rts.ptr_level == 0 && rts.array_rank == 0 && is_any_int(rts.base)) {
      return lts;
    }
    if (b.op == BinaryOp::Add &&
        llvm_ty(rts) == "ptr" &&
        lts.ptr_level == 0 && lts.array_rank == 0 && is_any_int(lts.base)) {
      return rts;
    }
    if (is_complex_base(lts.base) || is_complex_base(rts.base)) {
      if (!is_complex_base(lts.base)) return rts;
      if (!is_complex_base(rts.base)) return lts;
      return sizeof_ts(mod_, lts) >= sizeof_ts(mod_, rts) ? lts : rts;
    }

    // For arithmetic/bitwise the result type is the UAC common type.
    if (lts.base != TB_VOID && rts.base != TB_VOID &&
        lts.ptr_level == 0 && rts.ptr_level == 0 &&
        lts.array_rank == 0 && rts.array_rank == 0 &&
        is_any_int(lts.base) && is_any_int(rts.base)) {
      const bool is_shift = (b.op == BinaryOp::Shl || b.op == BinaryOp::Shr);
      TypeSpec ts{};
      ts.base = is_shift ? integer_promote(lts.base) : usual_arith_conv(lts.base, rts.base);
      return ts;
    }
    if (lts.base != TB_VOID) return lts;
    return resolve_expr_type(ctx, b.rhs);
  }

  TypeSpec resolve_payload_type(FnCtx& ctx, const UnaryExpr& u) {
    TypeSpec ts = resolve_expr_type(ctx, u.operand);
    switch (u.op) {
      case UnaryOp::AddrOf:
        if (ts.array_rank > 0) {
          ts.array_rank = 0;
          ts.array_size = -1;
        }
        ts.ptr_level += 1;
        return ts;
      case UnaryOp::Deref:
        if (ts.ptr_level > 0) ts.ptr_level -= 1;
        else if (ts.array_rank > 0) ts.array_rank -= 1;
        return ts;
      case UnaryOp::RealPart:
      case UnaryOp::ImagPart:
        if (is_complex_base(ts.base)) return complex_component_ts(ts.base);
        return ts;
      case UnaryOp::Not: {
        TypeSpec out{};
        out.base = TB_INT;
        return out;
      }
      default:
        return ts;
    }
  }

  TypeSpec resolve_payload_type(FnCtx& ctx, const AssignExpr& a) {
    return resolve_expr_type(ctx, a.lhs);
  }

  TypeSpec resolve_payload_type(FnCtx& ctx, const CastExpr& c) {
    return c.to_type.spec;
  }

  TypeSpec resolve_payload_type(FnCtx& ctx, const CallExpr& c) {
    const Expr& callee_e = get_expr(c.callee);
    if (const auto* dr_builtin = std::get_if<DeclRef>(&callee_e.payload)) {
      if (dr_builtin->name.rfind("__builtin_", 0) == 0) {
        if (dr_builtin->name == "__builtin_expect" && !c.args.empty())
          return resolve_expr_type(ctx, c.args[0]);
        if (dr_builtin->name == "__builtin_unreachable" ||
            dr_builtin->name == "__builtin_va_start" ||
            dr_builtin->name == "__builtin_va_end" ||
            dr_builtin->name == "__builtin_va_copy")
          return {};
        if ((dr_builtin->name == "__builtin_bswap16" ||
             dr_builtin->name == "__builtin_bswap32" ||
             dr_builtin->name == "__builtin_bswap64") && !c.args.empty())
          return resolve_expr_type(ctx, c.args[0]);
        if (dr_builtin->name == "__builtin_huge_vall" ||
            dr_builtin->name == "__builtin_infl"      ||
            dr_builtin->name == "__builtin_nanl"      ||
            dr_builtin->name == "__builtin_nansl"     ||
            dr_builtin->name == "__builtin_fabsl"     ||
            dr_builtin->name == "__builtin_copysignl") {
          TypeSpec ldbl{}; ldbl.base = TB_LONGDOUBLE; return ldbl;
        }
        if (dr_builtin->name == "__builtin_huge_val"  ||
            dr_builtin->name == "__builtin_inf"       ||
            dr_builtin->name == "__builtin_nan"       ||
            dr_builtin->name == "__builtin_nans"      ||
            dr_builtin->name == "__builtin_fabs"      ||
            dr_builtin->name == "__builtin_copysign") {
          TypeSpec dbl{}; dbl.base = TB_DOUBLE; return dbl;
        }
        if (dr_builtin->name == "__builtin_huge_valf" || dr_builtin->name == "__builtin_inff"  ||
            dr_builtin->name == "__builtin_nanf"      || dr_builtin->name == "__builtin_nansf" ||
            dr_builtin->name == "__builtin_fabsf"     || dr_builtin->name == "__builtin_copysignf") {
          TypeSpec flt{}; flt.base = TB_FLOAT; return flt;
        }
        if (dr_builtin->name == "__builtin_conjf") {
          TypeSpec cflt{}; cflt.base = TB_COMPLEX_FLOAT; return cflt;
        }
        if (dr_builtin->name == "__builtin_conj") {
          TypeSpec cdbl{}; cdbl.base = TB_COMPLEX_DOUBLE; return cdbl;
        }
        if (dr_builtin->name == "__builtin_conjl") {
          TypeSpec cldbl{}; cldbl.base = TB_COMPLEX_LONGDOUBLE; return cldbl;
        }
        // Pointer-returning builtins (void* or first-arg type)
        if (dr_builtin->name == "__builtin_memcpy"   ||
            dr_builtin->name == "__builtin_memmove"  ||
            dr_builtin->name == "__builtin_memset"   ||
            dr_builtin->name == "__builtin_memchr"   ||
            dr_builtin->name == "__builtin_strcpy"   ||
            dr_builtin->name == "__builtin_strncpy"  ||
            dr_builtin->name == "__builtin_strcat"   ||
            dr_builtin->name == "__builtin_strncat"  ||
            dr_builtin->name == "__builtin_strchr"   ||
            dr_builtin->name == "__builtin_strstr"   ||
            dr_builtin->name == "__builtin_malloc"   ||
            dr_builtin->name == "__builtin_calloc"   ||
            dr_builtin->name == "__builtin_realloc"  ||
            dr_builtin->name == "__builtin_alloca") {
          TypeSpec void_ptr{};
          void_ptr.base = TB_VOID;
          void_ptr.ptr_level = 1;
          return void_ptr;
        }
        TypeSpec builtin_default{};
        builtin_default.base = TB_INT;
        return builtin_default;
      }
    }
    // For calls through (*local_var) where the local was initialized from a
    // known function, use that function's return type directly.  This handles
    // nested fn-ptr types like int(*(*)(...))(...) where TypeSpec alone is
    // insufficient to distinguish the return type from a simple fn-ptr.
    if (const auto* u = std::get_if<UnaryExpr>(&callee_e.payload)) {
      if (u->op == UnaryOp::Deref) {
        const Expr& inner_e = get_expr(u->operand);
        if (const auto* dr = std::get_if<DeclRef>(&inner_e.payload)) {
          if (dr->local) {
            const auto frt_it = ctx.local_fn_ret_types.find(dr->local->value);
            if (frt_it != ctx.local_fn_ret_types.end()) return frt_it->second;
          }
        }
      }
    }
    // Direct call through a local fn-ptr local without explicit deref: local(...)
    if (const auto* dr0 = std::get_if<DeclRef>(&callee_e.payload)) {
      if (dr0->local) {
        const auto frt_it = ctx.local_fn_ret_types.find(dr0->local->value);
        if (frt_it != ctx.local_fn_ret_types.end()) return frt_it->second;
      }
    }
    if (const FnPtrSig* sig = resolve_callee_fn_ptr_sig(ctx, callee_e)) {
      return sig->return_type.spec;
    }
    if (const auto* r = std::get_if<DeclRef>(&callee_e.payload)) {
      const auto fit = mod_.fn_index.find(r->name);
      if (fit != mod_.fn_index.end() && fit->second.value < mod_.functions.size()) {
        return mod_.functions[fit->second.value].return_type.spec;
      }
      TypeSpec ref_ts = resolve_payload_type(ctx, *r);
      if (ref_ts.is_fn_ptr && ref_ts.ptr_level > 0) {
        ref_ts.ptr_level--;
        ref_ts.is_fn_ptr = false;
        return ref_ts;
      }
      if (ref_ts.base == TB_FUNC_PTR && ref_ts.ptr_level > 0) {
        ref_ts.ptr_level--;
        return ref_ts;
      }
    }
    TypeSpec callee_ts = resolve_expr_type(ctx, c.callee);
    if (callee_ts.is_fn_ptr && callee_ts.ptr_level > 0) {
      callee_ts.ptr_level--;
      callee_ts.is_fn_ptr = false;
      return callee_ts;
    }
    if (callee_ts.is_fn_ptr && callee_ts.ptr_level == 0) {
      // *fp where fp was ptr_level=1: after deref, ptr_level=0.
      // The function pointer's return type is encoded in the remaining spec.
      callee_ts.is_fn_ptr = false;
      return callee_ts;
    }
    if (callee_ts.base == TB_FUNC_PTR && callee_ts.ptr_level > 0) {
      callee_ts.ptr_level--;
      return callee_ts;
    }
    // Unknown external call: C implicit declaration defaults to int return
    TypeSpec implicit_int{}; implicit_int.base = TB_INT;
    return implicit_int;
  }

  TypeSpec resolve_payload_type(FnCtx&, const IntLiteral&) {
    TypeSpec ts{}; ts.base = TB_INT; return ts;
  }

  TypeSpec resolve_payload_type(FnCtx&, const FloatLiteral&) {
    TypeSpec ts{}; ts.base = TB_DOUBLE; return ts;
  }

  TypeSpec resolve_payload_type(FnCtx&, const CharLiteral&) {
    TypeSpec ts{}; ts.base = TB_CHAR; return ts;
  }

  TypeSpec resolve_payload_type(FnCtx&, const StringLiteral&) {
    TypeSpec ts{}; ts.base = TB_CHAR; ts.ptr_level = 1; return ts;
  }

  TypeSpec resolve_payload_type(FnCtx& ctx, const MemberExpr& m) {
    // Look up the field type in struct_defs
    const Expr& base_e = get_expr(m.base);
    TypeSpec base_ts = base_e.type.spec;
    if (base_ts.base == TB_VOID && base_ts.ptr_level == 0)
      base_ts = resolve_expr_type(ctx, m.base);
    if (m.is_arrow && base_ts.ptr_level > 0) base_ts.ptr_level--;
    if (!base_ts.tag || !base_ts.tag[0]) return {};
    std::vector<FieldStep> chain;
    TypeSpec field_ts{};
    find_field_chain(std::string(base_ts.tag), m.field, chain, field_ts);
    // Bitfield values are promoted per C integer promotion rules
    if (!chain.empty() && chain.back().bit_width >= 0) {
      TypeSpec bf_ts{};
      if (chain.back().bit_width > 32) {
        // >32-bit bitfield: promote to the declared type (long long)
        bf_ts.base = chain.back().bf_is_signed ? TB_LONGLONG : TB_ULONGLONG;
      } else {
        bf_ts.base = chain.back().bf_is_signed ? TB_INT : TB_UINT;
      }
      return bf_ts;
    }
    return field_ts;
  }

  TypeSpec resolve_payload_type(FnCtx& ctx, const IndexExpr& idx) {
    TypeSpec base_ts = resolve_expr_type(ctx, idx.base);
    if (base_ts.array_rank > 0) { base_ts.array_rank--; return base_ts; }
    if (base_ts.ptr_level > 0) { base_ts.ptr_level--; return base_ts; }
    return base_ts;
  }

  TypeSpec resolve_payload_type(FnCtx& ctx, const TernaryExpr& t) {
    TypeSpec ts = resolve_expr_type(ctx, t.then_expr);
    if (ts.base != TB_VOID || ts.ptr_level > 0 || ts.array_rank > 0) return ts;
    return resolve_expr_type(ctx, t.else_expr);
  }

  template <typename T>
  TypeSpec resolve_payload_type(FnCtx&, const T&) { return {}; }

  std::string emit_rval_id(FnCtx& ctx, ExprId id, TypeSpec& out_ts) {
    const Expr& e = get_expr(id);
    out_ts = e.type.spec;
    if (const auto* b = std::get_if<BinaryExpr>(&e.payload)) {
      // Prefer semantic reconstruction for binary expressions; parser-provided
      // AST types are often too coarse for pointer arithmetic.
      out_ts = resolve_payload_type(ctx, *b);
    } else if (const auto* c = std::get_if<CallExpr>(&e.payload)) {
      // Builtin calls such as __builtin_copysignl/__builtin_fabsl can refine
      // the return type beyond what survived from the parser AST.
      out_ts = resolve_payload_type(ctx, *c);
    }
    // AST does not annotate types on all nodes; resolve recursively.
    if (out_ts.base == TB_VOID && out_ts.ptr_level == 0 && out_ts.array_rank == 0) {
      out_ts = resolve_expr_type(ctx, id);
    }
    return emit_rval_expr(ctx, e);
  }

  std::string emit_rval_expr(FnCtx& ctx, const Expr& e) {
    return std::visit([&](const auto& p) -> std::string {
      return emit_rval_payload(ctx, p, e);
    }, e.payload);
  }

  // ── Default payload (unimplemented) ─────────────────────────────────────

  template <typename T>
  std::string emit_rval_payload(FnCtx&, const T&, const Expr&) {
    throw std::runtime_error(
        std::string("HirEmitter: unimplemented expr: ") + typeid(T).name());
  }

  // ── Literal payloads ─────────────────────────────────────────────────────

  std::string emit_rval_payload(FnCtx&, const IntLiteral& x, const Expr& e) {
    if (is_complex_base(e.type.spec.base)) {
      const TypeSpec elem_ts = complex_component_ts(e.type.spec.base);
      return "{ " + llvm_ty(elem_ts) + " " + emit_const_int_like(0, elem_ts) + ", " +
             llvm_ty(elem_ts) + " " + emit_const_int_like(x.value, elem_ts) + " }";
    }
    return std::to_string(x.value);
  }

  std::string emit_rval_payload(FnCtx&, const FloatLiteral& x, const Expr& e) {
    if (is_complex_base(e.type.spec.base)) {
      const TypeSpec elem_ts = complex_component_ts(e.type.spec.base);
      const std::string imag_v = fp_literal(elem_ts.base, x.value);
      return "{ " + llvm_ty(elem_ts) + " " + emit_const_int_like(0, elem_ts) + ", " +
             llvm_ty(elem_ts) + " " + imag_v + " }";
    }
    return is_float_base(e.type.spec.base) ? fp_literal(e.type.spec.base, x.value)
                                           : fp_to_hex(x.value);
  }

  std::string emit_rval_payload(FnCtx&, const CharLiteral& x, const Expr&) {
    return std::to_string(x.value);
  }

  std::string emit_complex_binary_arith(FnCtx& ctx,
                                        BinaryOp op,
                                        const std::string& lv,
                                        const TypeSpec& lts,
                                        const std::string& rv,
                                        const TypeSpec& rts,
                                        const TypeSpec& res_spec) {
    TypeSpec complex_ts = lts;
    if (!is_complex_base(complex_ts.base) ||
        (is_complex_base(rts.base) && sizeof_ts(mod_, rts) > sizeof_ts(mod_, complex_ts))) {
      complex_ts = rts;
    }
    const TypeSpec elem_ts = complex_component_ts(complex_ts.base);
    auto lift_scalar_to_complex = [&](const std::string& scalar,
                                      const TypeSpec& scalar_ts) -> std::string {
      const std::string real_v = coerce(ctx, scalar, scalar_ts, elem_ts);
      const std::string imag_v = emit_const_int_like(0, elem_ts);
      const std::string with_real = fresh_tmp(ctx);
      emit_instr(ctx, with_real + " = insertvalue " + llvm_ty(complex_ts) + " undef, " +
                             llvm_ty(elem_ts) + " " + real_v + ", 0");
      const std::string out = fresh_tmp(ctx);
      emit_instr(ctx, out + " = insertvalue " + llvm_ty(complex_ts) + " " + with_real + ", " +
                          llvm_ty(elem_ts) + " " + imag_v + ", 1");
      return out;
    };

    std::string complex_lv = lv;
    std::string complex_rv = rv;
    TypeSpec complex_lts = lts;
    TypeSpec complex_rts = rts;
    if (!is_complex_base(complex_lts.base)) {
      complex_lv = lift_scalar_to_complex(complex_lv, complex_lts);
      complex_lts = complex_ts;
    } else if (llvm_ty(complex_lts) != llvm_ty(complex_ts)) {
      complex_lv = coerce(ctx, complex_lv, complex_lts, complex_ts);
      complex_lts = complex_ts;
    }
    if (!is_complex_base(complex_rts.base)) {
      complex_rv = lift_scalar_to_complex(complex_rv, complex_rts);
      complex_rts = complex_ts;
    } else if (llvm_ty(complex_rts) != llvm_ty(complex_ts)) {
      complex_rv = coerce(ctx, complex_rv, complex_rts, complex_ts);
      complex_rts = complex_ts;
    }

    const std::string lreal = fresh_tmp(ctx);
    emit_instr(ctx, lreal + " = extractvalue " + llvm_ty(complex_ts) + " " + complex_lv + ", 0");
    const std::string limag = fresh_tmp(ctx);
    emit_instr(ctx, limag + " = extractvalue " + llvm_ty(complex_ts) + " " + complex_lv + ", 1");
    const std::string rreal = fresh_tmp(ctx);
    emit_instr(ctx, rreal + " = extractvalue " + llvm_ty(complex_ts) + " " + complex_rv + ", 0");
    const std::string rimag = fresh_tmp(ctx);
    emit_instr(ctx, rimag + " = extractvalue " + llvm_ty(complex_ts) + " " + complex_rv + ", 1");

    const std::string elem_ty = llvm_ty(elem_ts);
    const char* add_instr = is_float_base(elem_ts.base) ? "fadd" : "add";
    const char* sub_instr = is_float_base(elem_ts.base) ? "fsub" : "sub";
    const char* mul_instr = is_float_base(elem_ts.base) ? "fmul" : "mul";
    const char* div_instr = is_float_base(elem_ts.base)
                                ? "fdiv"
                                : (is_signed_int(elem_ts.base) ? "sdiv" : "udiv");

    std::string out_real;
    std::string out_imag;
    if (op == BinaryOp::Add || op == BinaryOp::Sub) {
      out_real = fresh_tmp(ctx);
      emit_instr(ctx, out_real + " = " + std::string(op == BinaryOp::Add ? add_instr : sub_instr) +
                          " " + elem_ty + " " + lreal + ", " + rreal);
      out_imag = fresh_tmp(ctx);
      emit_instr(ctx, out_imag + " = " + std::string(op == BinaryOp::Add ? add_instr : sub_instr) +
                          " " + elem_ty + " " + limag + ", " + rimag);
    } else if (op == BinaryOp::Mul) {
      const std::string ac = fresh_tmp(ctx);
      const std::string bd = fresh_tmp(ctx);
      const std::string ad = fresh_tmp(ctx);
      const std::string bc = fresh_tmp(ctx);
      emit_instr(ctx, ac + " = " + std::string(mul_instr) + " " + elem_ty + " " + lreal + ", " + rreal);
      emit_instr(ctx, bd + " = " + std::string(mul_instr) + " " + elem_ty + " " + limag + ", " + rimag);
      emit_instr(ctx, ad + " = " + std::string(mul_instr) + " " + elem_ty + " " + lreal + ", " + rimag);
      emit_instr(ctx, bc + " = " + std::string(mul_instr) + " " + elem_ty + " " + limag + ", " + rreal);
      out_real = fresh_tmp(ctx);
      emit_instr(ctx, out_real + " = " + std::string(sub_instr) + " " + elem_ty + " " + ac + ", " + bd);
      out_imag = fresh_tmp(ctx);
      emit_instr(ctx, out_imag + " = " + std::string(add_instr) + " " + elem_ty + " " + ad + ", " + bc);
    } else {
      const std::string ac = fresh_tmp(ctx);
      const std::string bd = fresh_tmp(ctx);
      const std::string bc = fresh_tmp(ctx);
      const std::string ad = fresh_tmp(ctx);
      const std::string cc = fresh_tmp(ctx);
      const std::string dd = fresh_tmp(ctx);
      const std::string denom = fresh_tmp(ctx);
      emit_instr(ctx, ac + " = " + std::string(mul_instr) + " " + elem_ty + " " + lreal + ", " + rreal);
      emit_instr(ctx, bd + " = " + std::string(mul_instr) + " " + elem_ty + " " + limag + ", " + rimag);
      emit_instr(ctx, bc + " = " + std::string(mul_instr) + " " + elem_ty + " " + limag + ", " + rreal);
      emit_instr(ctx, ad + " = " + std::string(mul_instr) + " " + elem_ty + " " + lreal + ", " + rimag);
      emit_instr(ctx, cc + " = " + std::string(mul_instr) + " " + elem_ty + " " + rreal + ", " + rreal);
      emit_instr(ctx, dd + " = " + std::string(mul_instr) + " " + elem_ty + " " + rimag + ", " + rimag);
      emit_instr(ctx, denom + " = " + std::string(add_instr) + " " + elem_ty + " " + cc + ", " + dd);
      const std::string real_num = fresh_tmp(ctx);
      const std::string imag_num = fresh_tmp(ctx);
      emit_instr(ctx, real_num + " = " + std::string(add_instr) + " " + elem_ty + " " + ac + ", " + bd);
      emit_instr(ctx, imag_num + " = " + std::string(sub_instr) + " " + elem_ty + " " + bc + ", " + ad);
      out_real = fresh_tmp(ctx);
      out_imag = fresh_tmp(ctx);
      emit_instr(ctx, out_real + " = " + std::string(div_instr) + " " + elem_ty + " " + real_num + ", " + denom);
      emit_instr(ctx, out_imag + " = " + std::string(div_instr) + " " + elem_ty + " " + imag_num + ", " + denom);
    }

    const std::string with_real = fresh_tmp(ctx);
    emit_instr(ctx, with_real + " = insertvalue " + llvm_ty(complex_ts) + " undef, " +
                               elem_ty + " " + out_real + ", 0");
    const std::string out = fresh_tmp(ctx);
    emit_instr(ctx, out + " = insertvalue " + llvm_ty(complex_ts) + " " + with_real + ", " +
                          elem_ty + " " + out_imag + ", 1");
    return llvm_ty(complex_ts) == llvm_ty(res_spec) ? out : coerce(ctx, out, complex_ts, res_spec);
  }

  std::string emit_rval_payload(FnCtx& ctx, const StringLiteral& sl, const Expr& /*e*/) {
    if (sl.is_wide) {
      // Wide string: emit as global array of i32 (wchar_t) values
      const auto vals = decode_wide_string_values(sl.raw);
      const std::string gname = "@.str" + std::to_string(str_idx_++);
      std::string init = "[";
      for (size_t i = 0; i < vals.size(); ++i) {
        if (i) init += ", ";
        init += "i32 " + std::to_string(vals[i]);
      }
      init += "]";
      preamble_ << gname << " = private unnamed_addr constant ["
                << vals.size() << " x i32] " << init << "\n";
      const std::string tmp = fresh_tmp(ctx);
      emit_instr(ctx, tmp + " = getelementptr [" + std::to_string(vals.size()) +
                          " x i32], ptr " + gname + ", i64 0, i64 0");
      return tmp;
    }
    // Strip enclosing quotes from raw, use as bytes
    std::string bytes = sl.raw;
    if (bytes.size() >= 2 && bytes.front() == '"' && bytes.back() == '"') {
      bytes = bytes.substr(1, bytes.size() - 2);
    }
    bytes = decode_c_escaped_bytes(bytes);
    const std::string gname = intern_str(bytes);
    const size_t len = bytes.size() + 1;
    const std::string tmp = fresh_tmp(ctx);
    emit_instr(ctx, tmp + " = getelementptr [" + std::to_string(len) +
                        " x i8], ptr " + gname + ", i64 0, i64 0");
    return tmp;
  }

  // ── DeclRef ───────────────────────────────────────────────────────────────

  std::string emit_rval_payload(FnCtx& ctx, const DeclRef& r, const Expr& e) {
    // Param: SSA value (check before function refs — params shadow function names)
    if (r.param_index && ctx.fn && *r.param_index < ctx.fn->params.size()) {
      // If we already have a spill slot for this param (due to lval use like p++),
      // load from it to get the current (possibly modified) value.
      const auto spill_it = ctx.param_slots.find(*r.param_index + 0x80000000u);
      if (spill_it != ctx.param_slots.end()) {
        const TypeSpec& pts = ctx.fn->params[*r.param_index].type.spec;
        const std::string ty = llvm_ty(pts);
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = load " + ty + ", ptr " + spill_it->second);
        return tmp;
      }
      // Otherwise use the SSA param value directly.
      const auto it = ctx.param_slots.find(*r.param_index);
      if (it != ctx.param_slots.end()) return it->second;
    }

    // Local: load
    if (r.local) {
      const auto it = ctx.local_slots.find(r.local->value);
      if (it == ctx.local_slots.end())
        throw std::runtime_error("HirEmitter: local slot not found: " + r.name);
      const TypeSpec ts = ctx.local_types.at(r.local->value);
      const auto vit = ctx.local_is_vla.find(r.local->value);
      if (vit != ctx.local_is_vla.end() && vit->second) {
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = load ptr, ptr " + it->second);
        return tmp;
      }
      if (ts.base == TB_VA_LIST && ts.ptr_level == 0 && ts.array_rank == 0) {
        // Treat va_list as array-like in expressions: it decays to a pointer to
        // the va_list object, so callers like vprintf(fmt, ap) receive ptr.
        return it->second;
      }
      if (ts.array_rank > 0 && !ts.is_ptr_to_array) {
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = getelementptr " + llvm_alloca_ty(ts) +
                            ", ptr " + it->second + ", i64 0, i64 0");
        return tmp;
      }
      const std::string ty = llvm_ty(ts);
      if (ty == "void") return "0";
      const std::string tmp = fresh_tmp(ctx);
      emit_instr(ctx, tmp + " = load " + ty + ", ptr " + it->second);
      return tmp;
    }

    // Global: load
    if (r.global) {
      size_t gv_idx = r.global->value;
      const auto& gv0 = mod_.globals[gv_idx];
      // If this global has an incomplete array type (unknown size, array_size<0),
      // prefer the definition entry (has initializer) so GEP uses correct element type.
      // Both the forward declaration and the definition may have array_size=-1; the
      // definition has a non-empty init from which resolve_array_ts deduces the size.
      if (gv0.type.spec.array_rank > 0 && gv0.type.spec.array_size < 0) {
        for (size_t gi = 0; gi < mod_.globals.size(); ++gi) {
          const auto& g = mod_.globals[gi];
          if (g.name == gv0.name && g.type.spec.array_rank > 0) {
            const TypeSpec rs = resolve_array_ts(g.type.spec, g.init);
            if (rs.array_size > 0) { gv_idx = gi; break; }
          }
        }
      }
      const auto& gv = mod_.globals[gv_idx];
      if (gv.type.spec.array_rank > 0 && !gv.type.spec.is_ptr_to_array) {
        // Use resolved type (deduced from initializer) for GEP — avoids "ptr" fallback.
        const TypeSpec resolved_ts = resolve_array_ts(gv.type.spec, gv.init);
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = getelementptr " + llvm_alloca_ty(resolved_ts) +
                            ", ptr @" + gv.name + ", i64 0, i64 0");
        return tmp;
      }
      const std::string ty = llvm_ty(gv.type.spec);
      if (ty == "void") return "0";
      const std::string tmp = fresh_tmp(ctx);
      emit_instr(ctx, tmp + " = load " + ty + ", ptr @" + gv.name);
      return tmp;
    }

    // Function reference: return as ptr value (after checking locals/params)
    if (mod_.fn_index.count(r.name)) return "@" + r.name;

    if ((r.name == "__func__" || r.name == "__FUNCTION__" || r.name == "__PRETTY_FUNCTION__") &&
        ctx.fn) {
      const std::string gname = intern_str(ctx.fn->name);
      const size_t len = ctx.fn->name.size() + 1;
      const std::string tmp = fresh_tmp(ctx);
      emit_instr(ctx, tmp + " = getelementptr [" + std::to_string(len) +
                          " x i8], ptr " + gname + ", i64 0, i64 0");
      return tmp;
    }

    // Unresolved: load from external global
    const TypeSpec ets = resolve_expr_type(ctx, e);
    if (!has_concrete_type(ets)) return "0";
    const std::string ty = llvm_ty(ets);
    if (ty == "void") return "0";
    const std::string tmp = fresh_tmp(ctx);
    emit_instr(ctx, tmp + " = load " + ty + ", ptr @" + r.name);
    return tmp;
  }

  // ── Unary ─────────────────────────────────────────────────────────────────

  std::string emit_rval_payload(FnCtx& ctx, const UnaryExpr& u, const Expr& e) {
    TypeSpec et = resolve_expr_type(ctx, e);
    TypeSpec op_ts{};
    const std::string val = emit_rval_id(ctx, u.operand, op_ts);
    if (!has_concrete_type(et)) et = op_ts;
    const std::string ty = llvm_ty(et);
    const std::string op_ty = llvm_ty(op_ts);

    switch (u.op) {
      case UnaryOp::Plus: return val;

      case UnaryOp::Minus: {
        const std::string tmp = fresh_tmp(ctx);
        if (is_float_base(op_ts.base)) {
          emit_instr(ctx, tmp + " = fneg " + ty + " " + val);
        } else {
          emit_instr(ctx, tmp + " = sub " + ty + " 0, " + val);
        }
        return tmp;
      }

      case UnaryOp::Not: {
        const std::string cmp = to_bool(ctx, val, op_ts);
        // Invert i1
        const std::string inv = fresh_tmp(ctx);
        emit_instr(ctx, inv + " = xor i1 " + cmp + ", true");
        if (ty == "i1") return inv;
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = zext i1 " + inv + " to " + ty);
        return tmp;
      }

      case UnaryOp::BitNot: {
        if (is_complex_base(op_ts.base)) {
          const TypeSpec elem_ts = complex_component_ts(op_ts.base);
          const std::string real_v = fresh_tmp(ctx);
          emit_instr(ctx, real_v + " = extractvalue " + op_ty + " " + val + ", 0");
          const std::string imag_v0 = fresh_tmp(ctx);
          emit_instr(ctx, imag_v0 + " = extractvalue " + op_ty + " " + val + ", 1");
          const std::string imag_v = fresh_tmp(ctx);
          if (is_float_base(elem_ts.base)) {
            emit_instr(ctx, imag_v + " = fneg " + llvm_ty(elem_ts) + " " + imag_v0);
          } else {
            emit_instr(ctx, imag_v + " = sub " + llvm_ty(elem_ts) + " 0, " + imag_v0);
          }
          const std::string with_real = fresh_tmp(ctx);
          emit_instr(ctx, with_real + " = insertvalue " + op_ty + " undef, " +
                                 llvm_ty(elem_ts) + " " + real_v + ", 0");
          const std::string out = fresh_tmp(ctx);
          emit_instr(ctx, out + " = insertvalue " + op_ty + " " + with_real + ", " +
                                llvm_ty(elem_ts) + " " + imag_v + ", 1");
          return out;
        }
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = xor " + ty + " " + val + ", -1");
        return tmp;
      }

      case UnaryOp::AddrOf: {
        TypeSpec pts{};
        return emit_lval(ctx, u.operand, pts);
      }

      case UnaryOp::Deref: {
        TypeSpec load_ts = op_ts;
        if (load_ts.ptr_level > 0) {
          load_ts.ptr_level -= 1;
        } else if (load_ts.array_rank > 0) {
          // Decayed array: the value is already a pointer to the first element.
          // Dereference it by loading the element type.
          load_ts.array_rank--;
          load_ts.array_size = -1;
        }
        // In C, dereferencing a function pointer (*fp) yields the function,
        // which immediately decays back to a function pointer — identity op.
        if (load_ts.is_fn_ptr && load_ts.ptr_level == 0) return val;
        const std::string load_ty = llvm_ty(load_ts);
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = load " + load_ty + ", ptr " + val);
        return tmp;
      }

      case UnaryOp::PreInc:
      case UnaryOp::PreDec: {
        // Check for bitfield operand
        const Expr& operand_e = get_expr(u.operand);
        if (const auto* m = std::get_if<MemberExpr>(&operand_e.payload)) {
          TypeSpec lhs_ts{};
          BitfieldAccess bf;
          const std::string bf_ptr = emit_member_lval(ctx, *m, lhs_ts, &bf);
          if (bf.is_bitfield()) {
            const int pbits = bitfield_promoted_bits(bf);
            const std::string pty = "i" + std::to_string(pbits);
            const std::string loaded = emit_bitfield_load(ctx, bf_ptr, bf);
            const std::string delta = (u.op == UnaryOp::PreInc) ? "1" : "-1";
            const std::string result = fresh_tmp(ctx);
            emit_instr(ctx, result + " = add " + pty + " " + loaded + ", " + delta);
            TypeSpec promoted_ts{};
            promoted_ts.base = (pbits > 32) ? (bf.is_signed ? TB_LONGLONG : TB_ULONGLONG)
                                             : (bf.is_signed ? TB_INT : TB_UINT);
            emit_bitfield_store(ctx, bf_ptr, bf, result, promoted_ts);
            // Re-read to get value masked to bitfield width (C semantics)
            return emit_bitfield_load(ctx, bf_ptr, bf);
          }
        }
        TypeSpec pts{};
        const std::string ptr = emit_lval(ctx, u.operand, pts);
        const std::string pty = llvm_ty(pts);
        const std::string loaded = fresh_tmp(ctx);
        emit_instr(ctx, loaded + " = load " + pty + ", ptr " + ptr);
        const std::string result = fresh_tmp(ctx);
        if (pty == "ptr") {
          const std::string delta = (u.op == UnaryOp::PreInc) ? "1" : "-1";
          TypeSpec elem_ts = pts;
          if (elem_ts.ptr_level > 0) elem_ts.ptr_level -= 1;
          else elem_ts.base = TB_CHAR;
          const std::string gep_ety1 =
              (elem_ts.base == TB_VOID && elem_ts.ptr_level == 0 && elem_ts.array_rank == 0)
                  ? "i8" : llvm_ty(elem_ts);
          emit_instr(ctx, result + " = getelementptr " + gep_ety1 +
                              ", ptr " + loaded + ", i64 " + delta);
        } else if (is_float_base(pts.base)) {
          const std::string delta = (u.op == UnaryOp::PreInc) ? "1.0" : "-1.0";
          emit_instr(ctx, result + " = fadd " + pty + " " + loaded + ", " + delta);
        } else {
          const std::string delta = (u.op == UnaryOp::PreInc) ? "1" : "-1";
          emit_instr(ctx, result + " = add " + pty + " " + loaded + ", " + delta);
        }
        emit_instr(ctx, "store " + pty + " " + result + ", ptr " + ptr);
        return result;
      }

      case UnaryOp::PostInc:
      case UnaryOp::PostDec: {
        // Check for bitfield operand
        const Expr& post_operand_e = get_expr(u.operand);
        if (const auto* m = std::get_if<MemberExpr>(&post_operand_e.payload)) {
          TypeSpec lhs_ts{};
          BitfieldAccess bf;
          const std::string bf_ptr = emit_member_lval(ctx, *m, lhs_ts, &bf);
          if (bf.is_bitfield()) {
            const int pbits = bitfield_promoted_bits(bf);
            const std::string pty = "i" + std::to_string(pbits);
            const std::string old_val = emit_bitfield_load(ctx, bf_ptr, bf);
            const std::string delta = (u.op == UnaryOp::PostInc) ? "1" : "-1";
            const std::string new_val = fresh_tmp(ctx);
            emit_instr(ctx, new_val + " = add " + pty + " " + old_val + ", " + delta);
            TypeSpec promoted_ts{};
            promoted_ts.base = (pbits > 32) ? (bf.is_signed ? TB_LONGLONG : TB_ULONGLONG)
                                             : (bf.is_signed ? TB_INT : TB_UINT);
            emit_bitfield_store(ctx, bf_ptr, bf, new_val, promoted_ts);
            return old_val;  // post: return old value
          }
        }
        TypeSpec pts{};
        const std::string ptr = emit_lval(ctx, u.operand, pts);
        const std::string pty = llvm_ty(pts);
        const std::string loaded = fresh_tmp(ctx);
        emit_instr(ctx, loaded + " = load " + pty + ", ptr " + ptr);
        const std::string result = fresh_tmp(ctx);
        if (pty == "ptr") {
          const std::string delta = (u.op == UnaryOp::PostInc) ? "1" : "-1";
          TypeSpec elem_ts = pts;
          if (elem_ts.ptr_level > 0) elem_ts.ptr_level -= 1;
          else elem_ts.base = TB_CHAR;
          const std::string gep_ety2 =
              (elem_ts.base == TB_VOID && elem_ts.ptr_level == 0 && elem_ts.array_rank == 0)
                  ? "i8" : llvm_ty(elem_ts);
          emit_instr(ctx, result + " = getelementptr " + gep_ety2 +
                              ", ptr " + loaded + ", i64 " + delta);
        } else if (is_float_base(pts.base)) {
          const std::string delta = (u.op == UnaryOp::PostInc) ? "1.0" : "-1.0";
          emit_instr(ctx, result + " = fadd " + pty + " " + loaded + ", " + delta);
        } else {
          const std::string delta = (u.op == UnaryOp::PostInc) ? "1" : "-1";
          emit_instr(ctx, result + " = add " + pty + " " + loaded + ", " + delta);
        }
        emit_instr(ctx, "store " + pty + " " + result + ", ptr " + ptr);
        return loaded;  // post: return old value
      }

      case UnaryOp::RealPart:
      case UnaryOp::ImagPart: {
        try {
          TypeSpec complex_ts{};
          const std::string complex_ptr = emit_lval(ctx, u.operand, complex_ts);
          if (!is_complex_base(complex_ts.base)) throw std::runtime_error("non-complex");
          const TypeSpec part_ts = complex_component_ts(complex_ts.base);
          const std::string part_ptr = fresh_tmp(ctx);
          emit_instr(ctx, part_ptr + " = getelementptr " + llvm_alloca_ty(complex_ts) +
                                 ", ptr " + complex_ptr + ", i32 0, i32 " +
                                 std::to_string(u.op == UnaryOp::ImagPart ? 1 : 0));
          const std::string tmp = fresh_tmp(ctx);
          emit_instr(ctx, tmp + " = load " + llvm_ty(part_ts) + ", ptr " + part_ptr);
          return tmp;
        } catch (const std::runtime_error&) {
        }
        if (!is_complex_base(op_ts.base)) return "0";
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = extractvalue " + llvm_ty(op_ts) + " " + val + ", " +
                            std::to_string(u.op == UnaryOp::ImagPart ? 1 : 0));
        return tmp;
      }
    }
    return "0";
  }

  // ── Binary ────────────────────────────────────────────────────────────────

  std::string emit_rval_payload(FnCtx& ctx, const BinaryExpr& b, const Expr& e) {
    // Comma: eval both, return rhs
    if (b.op == BinaryOp::Comma) {
      TypeSpec lts{};
      emit_rval_id(ctx, b.lhs, lts);
      TypeSpec rts{};
      return emit_rval_id(ctx, b.rhs, rts);
    }
    // Short-circuit logical
    if (b.op == BinaryOp::LAnd || b.op == BinaryOp::LOr) {
      return emit_logical(ctx, b, e);
    }

    TypeSpec lts{};
    std::string lv = emit_rval_id(ctx, b.lhs, lts);
    TypeSpec rts{};
    std::string rv = emit_rval_id(ctx, b.rhs, rts);
    TypeSpec res_spec = (e.type.spec.base != TB_VOID || e.type.spec.ptr_level > 0)
                            ? e.type.spec
                            : lts;

    if ((b.op == BinaryOp::Eq || b.op == BinaryOp::Ne) &&
        (is_complex_base(lts.base) || is_complex_base(rts.base))) {
      TypeSpec cmp_lts = lts;
      TypeSpec cmp_rts = rts;
      std::string cmp_lv = lv;
      std::string cmp_rv = rv;
      const TypeSpec elem_ts = is_complex_base(cmp_lts.base)
                                   ? complex_component_ts(cmp_lts.base)
                                   : complex_component_ts(cmp_rts.base);
      const TypeSpec complex_ts = is_complex_base(cmp_lts.base) ? cmp_lts : cmp_rts;
      auto lift_scalar_to_complex = [&](const std::string& scalar,
                                        const TypeSpec& scalar_ts,
                                        bool scalar_is_lhs) -> std::string {
        std::string real_v = coerce(ctx, scalar, scalar_ts, elem_ts);
        const std::string imag_v = emit_const_int_like(0, elem_ts);
        const std::string with_real = fresh_tmp(ctx);
        emit_instr(ctx, with_real + " = insertvalue " + llvm_ty(complex_ts) + " undef, " +
                                   llvm_ty(elem_ts) + " " + real_v + ", 0");
        const std::string out = fresh_tmp(ctx);
        emit_instr(ctx, out + " = insertvalue " + llvm_ty(complex_ts) + " " + with_real + ", " +
                            llvm_ty(elem_ts) + " " + imag_v + ", 1");
        (void)scalar_is_lhs;
        return out;
      };
      if (!is_complex_base(cmp_lts.base)) {
        cmp_lv = lift_scalar_to_complex(cmp_lv, cmp_lts, true);
        cmp_lts = complex_ts;
      } else if (llvm_ty(cmp_lts) != llvm_ty(complex_ts)) {
        cmp_lv = coerce(ctx, cmp_lv, cmp_lts, complex_ts);
        cmp_lts = complex_ts;
      }
      if (!is_complex_base(cmp_rts.base)) {
        cmp_rv = lift_scalar_to_complex(cmp_rv, cmp_rts, false);
        cmp_rts = complex_ts;
      } else if (llvm_ty(cmp_rts) != llvm_ty(complex_ts)) {
        cmp_rv = coerce(ctx, cmp_rv, cmp_rts, complex_ts);
        cmp_rts = complex_ts;
      }
      const std::string lreal = fresh_tmp(ctx);
      emit_instr(ctx, lreal + " = extractvalue " + llvm_ty(cmp_lts) + " " + cmp_lv + ", 0");
      const std::string limag = fresh_tmp(ctx);
      emit_instr(ctx, limag + " = extractvalue " + llvm_ty(cmp_lts) + " " + cmp_lv + ", 1");
      const std::string rreal0 = fresh_tmp(ctx);
      emit_instr(ctx, rreal0 + " = extractvalue " + llvm_ty(cmp_rts) + " " + cmp_rv + ", 0");
      const std::string rimag0 = fresh_tmp(ctx);
      emit_instr(ctx, rimag0 + " = extractvalue " + llvm_ty(cmp_rts) + " " + cmp_rv + ", 1");
      const std::string rreal = coerce(ctx, rreal0, elem_ts, elem_ts);
      const std::string rimag = coerce(ctx, rimag0, elem_ts, elem_ts);
      const std::string creal = fresh_tmp(ctx);
      const std::string cimag = fresh_tmp(ctx);
      if (is_float_base(elem_ts.base)) {
        emit_instr(ctx, creal + " = fcmp oeq " + llvm_ty(elem_ts) + " " + lreal + ", " + rreal);
        emit_instr(ctx, cimag + " = fcmp oeq " + llvm_ty(elem_ts) + " " + limag + ", " + rimag);
      } else {
        emit_instr(ctx, creal + " = icmp eq " + llvm_ty(elem_ts) + " " + lreal + ", " + rreal);
        emit_instr(ctx, cimag + " = icmp eq " + llvm_ty(elem_ts) + " " + limag + ", " + rimag);
      }
      const std::string both = fresh_tmp(ctx);
      emit_instr(ctx, both + " = and i1 " + creal + ", " + cimag);
      const std::string out = fresh_tmp(ctx);
      if (b.op == BinaryOp::Eq) {
        emit_instr(ctx, out + " = zext i1 " + both + " to i32");
      } else {
        const std::string inv = fresh_tmp(ctx);
        emit_instr(ctx, inv + " = xor i1 " + both + ", true");
        emit_instr(ctx, out + " = zext i1 " + inv + " to i32");
      }
      return out;
    }

    if ((b.op == BinaryOp::Add || b.op == BinaryOp::Sub ||
         b.op == BinaryOp::Mul || b.op == BinaryOp::Div) &&
        (is_complex_base(lts.base) || is_complex_base(rts.base))) {
      return emit_complex_binary_arith(ctx, b.op, lv, lts, rv, rts, res_spec);
    }

    // Apply C usual arithmetic conversions (UAC) for integer operands.
    // This promotes both operands to at least int, then to the common type.
    const bool l_is_int = is_any_int(lts.base) && lts.ptr_level == 0 && lts.array_rank == 0;
    const bool r_is_int = is_any_int(rts.base) && rts.ptr_level == 0 && rts.array_rank == 0;
    const bool l_is_flt = is_float_base(lts.base) && lts.ptr_level == 0 && lts.array_rank == 0;
    const bool r_is_flt = is_float_base(rts.base) && rts.ptr_level == 0 && rts.array_rank == 0;
    if (l_is_int && r_is_int) {
      // For shift operations, the result type is the promoted LHS only;
      // the RHS (shift amount) doesn't participate in usual arithmetic conversions.
      const bool is_shift = (b.op == BinaryOp::Shl || b.op == BinaryOp::Shr);
      if (is_shift) {
        TypeBase lp = integer_promote(lts.base);
        TypeSpec lp_ts{}; lp_ts.base = lp;
        if (lts.base != lp) lv = coerce(ctx, lv, lts, lp_ts);
        lts = lp_ts;
        // Coerce RHS to match LHS width for LLVM (shift needs same-width operands)
        if (llvm_ty(rts) != llvm_ty(lts)) rv = coerce(ctx, rv, rts, lts);
        rts = lts;
      } else {
        TypeBase common = usual_arith_conv(lts.base, rts.base);
        TypeSpec common_ts{};
        common_ts.base = common;
        if (lts.base != common) lv = coerce(ctx, lv, lts, common_ts);
        lts = common_ts;
        if (rts.base != common) rv = coerce(ctx, rv, rts, common_ts);
        rts = common_ts;
      }
    } else if (l_is_int && r_is_flt) {
      lv = coerce(ctx, lv, lts, rts);
      lts = rts;
    } else if (l_is_flt && r_is_int) {
      rv = coerce(ctx, rv, rts, lts);
      rts = lts;
    }

    // After UAC, the result type should be at least as wide as the promoted
    // operand type.  The stored res_spec may reflect the pre-promotion type
    // from sema, so override it with the promoted lts for int arithmetic.
    if (l_is_int && r_is_int && is_any_int(res_spec.base) &&
        res_spec.ptr_level == 0 && res_spec.array_rank == 0) {
      res_spec = lts;
    }

    // If result type is unannotated (void), use the operand type
    const std::string res_ty = llvm_ty(res_spec);
    const std::string op_ty  = llvm_ty(lts);
    // lf/ls must be false when the LLVM type is ptr (even if base is double/float).
    const bool lf  = is_float_base(lts.base) && lts.ptr_level == 0 && lts.array_rank == 0;
    const bool ls  = is_signed_int(lts.base) && lts.ptr_level == 0 && lts.array_rank == 0;
    const bool arith_ls = ls;

    // Pointer arithmetic: ptr +/- int and int + ptr.
    if (b.op == BinaryOp::Add || b.op == BinaryOp::Sub) {
      auto emit_ptr_gep = [&](const std::string& base_ptr, const TypeSpec& base_ts,
                              const std::string& idx_val, const TypeSpec& idx_ts,
                              bool negate_idx) -> std::string {
        TypeSpec i64_ts{};
        i64_ts.base = TB_LONGLONG;
        std::string idx = coerce(ctx, idx_val, idx_ts, i64_ts);
        if (negate_idx) {
          const std::string neg = fresh_tmp(ctx);
          emit_instr(ctx, neg + " = sub i64 0, " + idx);
          idx = neg;
        }
        TypeSpec elem_ts = base_ts;
        if (elem_ts.ptr_level > 0) {
          // T* +/- n: stride by sizeof(T).  For pointer-to-array keep the
          // array object type after consuming one pointer layer.
          elem_ts.ptr_level -= 1;
        } else if (elem_ts.array_rank > 0 && !elem_ts.is_ptr_to_array) {
          // Array expression in arithmetic decays to pointer-to-first-element.
          elem_ts = drop_one_array_dim(elem_ts);
        } else {
          elem_ts = {};
          elem_ts.base = TB_CHAR;
        }
        // GCC extension: void* arithmetic uses byte stride (same as char*).
        const std::string gep_elem_ty =
            (elem_ts.base == TB_VOID && elem_ts.ptr_level == 0 &&
             elem_ts.array_rank == 0)
                ? "i8"
                : llvm_ty(elem_ts);
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = getelementptr " + gep_elem_ty +
                            ", ptr " + base_ptr + ", i64 " + idx);
        return tmp;
      };

      if (op_ty == "ptr" && rts.ptr_level == 0 && rts.array_rank == 0 && is_any_int(rts.base)) {
        return emit_ptr_gep(lv, lts, rv, rts, b.op == BinaryOp::Sub);
      }
      if (llvm_ty(rts) == "ptr" && lts.ptr_level == 0 && lts.array_rank == 0 &&
          is_any_int(lts.base) && b.op == BinaryOp::Add) {
        return emit_ptr_gep(rv, rts, lv, lts, false);
      }
      if (b.op == BinaryOp::Sub && op_ty == "ptr" && llvm_ty(rts) == "ptr") {
        const std::string lhs_i = fresh_tmp(ctx);
        const std::string rhs_i = fresh_tmp(ctx);
        emit_instr(ctx, lhs_i + " = ptrtoint ptr " + lv + " to i64");
        emit_instr(ctx, rhs_i + " = ptrtoint ptr " + rv + " to i64");
        const std::string byte_diff = fresh_tmp(ctx);
        emit_instr(ctx, byte_diff + " = sub i64 " + lhs_i + ", " + rhs_i);
        TypeSpec elem_ts = lts;
        if (elem_ts.ptr_level > 0) elem_ts.ptr_level -= 1;
        const int elem_sz = std::max(1, sizeof_ts(mod_, elem_ts));
        std::string diff = byte_diff;
        if (elem_sz != 1) {
          const std::string scaled = fresh_tmp(ctx);
          emit_instr(ctx, scaled + " = sdiv i64 " + byte_diff + ", " + std::to_string(elem_sz));
          diff = scaled;
        }
        TypeSpec i64_ts{};
        i64_ts.base = TB_LONGLONG;
        if (res_spec.ptr_level > 0 || res_spec.array_rank > 0 || res_spec.base == TB_VOID) {
          return diff;
        }
        return coerce(ctx, diff, i64_ts, res_spec);
      }
    }

    rv = coerce(ctx, rv, rts, lts);

    // Arithmetic
    static const struct { BinaryOp op; const char* i_s; const char* i_u; const char* f; } arith[] = {
      { BinaryOp::Add, "add",  "add",  "fadd" },
      { BinaryOp::Sub, "sub",  "sub",  "fsub" },
      { BinaryOp::Mul, "mul",  "mul",  "fmul" },
      { BinaryOp::Div, "sdiv", "udiv", "fdiv" },
      { BinaryOp::Mod, "srem", "urem", nullptr },
      { BinaryOp::Shl, "shl",  "shl",  nullptr },
      { BinaryOp::Shr, "ashr", "lshr", nullptr },
      { BinaryOp::BitAnd, "and", "and", nullptr },
      { BinaryOp::BitOr,  "or",  "or",  nullptr },
      { BinaryOp::BitXor, "xor", "xor", nullptr },
    };
    for (const auto& row : arith) {
      if (row.op == b.op) {
        const char* instr = nullptr;
        if (lf) {
          instr = row.f;
        } else if (row.op == BinaryOp::Shr) {
          instr = ls ? row.i_s : row.i_u;
        } else {
          instr = arith_ls ? row.i_s : row.i_u;
        }
        if (!instr) break;
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = " + instr + " " + op_ty + " " + lv + ", " + rv);
        return coerce(ctx, tmp, lts, res_spec);
      }
    }

    // Comparison
    static const struct { BinaryOp op; const char* is; const char* iu; const char* f; } cmp[] = {
      { BinaryOp::Lt, "slt", "ult", "olt" },
      { BinaryOp::Le, "sle", "ule", "ole" },
      { BinaryOp::Gt, "sgt", "ugt", "ogt" },
      { BinaryOp::Ge, "sge", "uge", "oge" },
      { BinaryOp::Eq, "eq",  "eq",  "oeq" },
      { BinaryOp::Ne, "ne",  "ne",  "une" },
    };
    for (const auto& row : cmp) {
      if (row.op == b.op) {
        // C comparisons always yield int. Always zext i1 → i32 so the returned
        // value matches the TB_INT TypeSpec reported by resolve_payload_type.
        const std::string cmp_tmp = fresh_tmp(ctx);
        if (lf) {
          emit_instr(ctx, cmp_tmp + " = fcmp " + row.f + " " + op_ty + " " + lv + ", " + rv);
        } else {
          // After UAC, both operands are at the common type.
          // Use the common type's signedness for the comparison predicate.
          const char* pred = ls ? row.is : row.iu;
          emit_instr(ctx, cmp_tmp + " = icmp " + pred + " " + op_ty + " " + lv + ", " + rv);
        }
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = zext i1 " + cmp_tmp + " to i32");
        return tmp;
      }
    }

    throw std::runtime_error("HirEmitter: unhandled binary op");
  }

  std::string emit_logical(FnCtx& ctx, const BinaryExpr& b, const Expr& e) {
    TypeSpec res_spec = resolve_expr_type(ctx, e);
    if (!has_concrete_type(res_spec)) res_spec.base = TB_INT;
    const std::string res_ty = llvm_ty(res_spec);
    TypeSpec lts{};
    const std::string lv = emit_rval_id(ctx, b.lhs, lts);
    const std::string lc = to_bool(ctx, lv, lts);

    const std::string rhs_lbl  = fresh_lbl(ctx, "logic.rhs.");
    const std::string skip_lbl = fresh_lbl(ctx, "logic.skip.");
    const std::string rhs_end_lbl = fresh_lbl(ctx, "logic.rhs.end.");
    const std::string end_lbl  = fresh_lbl(ctx, "logic.end.");

    if (b.op == BinaryOp::LAnd) {
      emit_term(ctx, "br i1 " + lc + ", label %" + rhs_lbl + ", label %" + skip_lbl);
    } else {
      emit_term(ctx, "br i1 " + lc + ", label %" + skip_lbl + ", label %" + rhs_lbl);
    }

    emit_lbl(ctx, rhs_lbl);
    TypeSpec rts{};
    const std::string rv = emit_rval_id(ctx, b.rhs, rts);
    const std::string rc = to_bool(ctx, rv, rts);
    std::string rhs_val;
    if (res_ty == "i1") {
      rhs_val = rc;
    } else if (is_float_base(res_spec.base)) {
      const std::string as_i32 = fresh_tmp(ctx);
      emit_instr(ctx, as_i32 + " = zext i1 " + rc + " to i32");
      rhs_val = fresh_tmp(ctx);
      emit_instr(ctx, rhs_val + " = sitofp i32 " + as_i32 + " to " + res_ty);
    } else {
      rhs_val = fresh_tmp(ctx);
      emit_instr(ctx, rhs_val + " = zext i1 " + rc + " to " + res_ty);
    }
    emit_term(ctx, "br label %" + rhs_end_lbl);

    emit_lbl(ctx, rhs_end_lbl);
    emit_term(ctx, "br label %" + end_lbl);

    emit_lbl(ctx, skip_lbl);
    std::string skip_val;
    if (res_ty == "i1") {
      skip_val = (b.op == BinaryOp::LAnd) ? "false" : "true";
    } else if (is_float_base(res_spec.base)) {
      skip_val = (b.op == BinaryOp::LAnd) ? "0.0" : "1.0";
    } else {
      skip_val = (b.op == BinaryOp::LAnd) ? "0" : "1";
    }
    emit_term(ctx, "br label %" + end_lbl);

    emit_lbl(ctx, end_lbl);
    const std::string tmp = fresh_tmp(ctx);
    emit_instr(ctx, tmp + " = phi " + res_ty +
                        " [ " + rhs_val + ", %" + rhs_end_lbl + " ]," +
                        " [ " + skip_val + ", %" + skip_lbl + " ]");
    return tmp;
  }

  // ── Assign ────────────────────────────────────────────────────────────────

  std::string emit_rval_payload(FnCtx& ctx, const AssignExpr& a, const Expr& /*e*/) {
    TypeSpec rhs_ts{};
    std::string rhs = emit_rval_id(ctx, a.rhs, rhs_ts);

    TypeSpec lhs_ts{};
    BitfieldAccess bf;
    // Check if LHS is a member expression (potential bitfield)
    const Expr& lhs_e = get_expr(a.lhs);
    std::string lhs_ptr;
    if (const auto* m = std::get_if<MemberExpr>(&lhs_e.payload)) {
      lhs_ptr = emit_member_lval(ctx, *m, lhs_ts, &bf);
    } else {
      lhs_ptr = emit_lval(ctx, a.lhs, lhs_ts);
    }
    const std::string lty = llvm_ty(lhs_ts);

    // Bitfield simple assignment
    if (bf.is_bitfield() && a.op == AssignOp::Set) {
      emit_bitfield_store(ctx, lhs_ptr, bf, rhs, rhs_ts);
      return rhs;
    }
    // Bitfield compound assignment
    if (bf.is_bitfield() && a.op != AssignOp::Set) {
      const int pbits = bitfield_promoted_bits(bf);
      const std::string promoted_ty = "i" + std::to_string(pbits);
      TypeSpec promoted_ts{};
      promoted_ts.base = (pbits > 32) ? (bf.is_signed ? TB_LONGLONG : TB_ULONGLONG)
                                       : (bf.is_signed ? TB_INT : TB_UINT);
      const std::string loaded = emit_bitfield_load(ctx, lhs_ptr, bf);
      std::string lhs_op = loaded;
      std::string rhs_op = coerce(ctx, rhs, rhs_ts, promoted_ts);
      const bool ls = bf.is_signed;
      const char* instr = nullptr;
      static const struct { AssignOp op; const char* is; const char* iu; } tbl[] = {
        {AssignOp::Add, "add", "add"}, {AssignOp::Sub, "sub", "sub"},
        {AssignOp::Mul, "mul", "mul"}, {AssignOp::Div, "sdiv", "udiv"},
        {AssignOp::Mod, "srem", "urem"}, {AssignOp::Shl, "shl", "shl"},
        {AssignOp::Shr, "ashr", "lshr"}, {AssignOp::BitAnd, "and", "and"},
        {AssignOp::BitOr, "or", "or"}, {AssignOp::BitXor, "xor", "xor"},
      };
      for (const auto& r : tbl) {
        if (r.op == a.op) { instr = ls ? r.is : r.iu; break; }
      }
      if (!instr) throw std::runtime_error("HirEmitter: bitfield compound assign: unknown op");
      const std::string result = fresh_tmp(ctx);
      emit_instr(ctx, result + " = " + instr + " " + promoted_ty + " " + lhs_op + ", " + rhs_op);
      emit_bitfield_store(ctx, lhs_ptr, bf, result, promoted_ts);
      return result;
    }

    if (a.op != AssignOp::Set) {
      const std::string loaded = fresh_tmp(ctx);
      emit_instr(ctx, loaded + " = load " + lty + ", ptr " + lhs_ptr);

      if ((a.op == AssignOp::Add || a.op == AssignOp::Sub) && lty == "ptr") {
        TypeSpec i64_ts{};
        i64_ts.base = TB_LONGLONG;
        std::string delta = coerce(ctx, rhs, rhs_ts, i64_ts);
        if (a.op == AssignOp::Sub) {
          const std::string neg = fresh_tmp(ctx);
          emit_instr(ctx, neg + " = sub i64 0, " + delta);
          delta = neg;
        }
        TypeSpec elem_ts = lhs_ts;
        if (elem_ts.ptr_level > 0) {
          elem_ts.ptr_level -= 1;
        } else {
          elem_ts.base = TB_CHAR;
        }
        const std::string gep_ety3 =
            (elem_ts.base == TB_VOID && elem_ts.ptr_level == 0 && elem_ts.array_rank == 0)
                ? "i8" : llvm_ty(elem_ts);
        const std::string result = fresh_tmp(ctx);
        emit_instr(ctx, result + " = getelementptr " + gep_ety3 +
                            ", ptr " + loaded + ", i64 " + delta);
        emit_instr(ctx, "store ptr " + result + ", ptr " + lhs_ptr);
        return result;
      }

      static const struct { AssignOp op; BinaryOp bop; } compound_map[] = {
        { AssignOp::Add, BinaryOp::Add }, { AssignOp::Sub, BinaryOp::Sub },
        { AssignOp::Mul, BinaryOp::Mul }, { AssignOp::Div, BinaryOp::Div },
        { AssignOp::Mod, BinaryOp::Mod }, { AssignOp::Shl, BinaryOp::Shl },
        { AssignOp::Shr, BinaryOp::Shr }, { AssignOp::BitAnd, BinaryOp::BitAnd },
        { AssignOp::BitOr, BinaryOp::BitOr }, { AssignOp::BitXor, BinaryOp::BitXor },
      };
      const char* instr = nullptr;
      TypeSpec op_ts = lhs_ts;
      for (const auto& row : compound_map) {
        if (row.op != a.op) continue;
        if ((row.bop == BinaryOp::Add || row.bop == BinaryOp::Sub ||
             row.bop == BinaryOp::Mul || row.bop == BinaryOp::Div) &&
            (is_complex_base(lhs_ts.base) || is_complex_base(rhs_ts.base))) {
          const std::string result =
              emit_complex_binary_arith(ctx, row.bop, loaded, lhs_ts, rhs, rhs_ts, lhs_ts);
          emit_instr(ctx, "store " + lty + " " + result + ", ptr " + lhs_ptr);
          return result;
        }
        // Compute the operation type via C usual arithmetic conversions.
        if (lhs_ts.ptr_level == 0 && lhs_ts.array_rank == 0 &&
            rhs_ts.ptr_level == 0 && rhs_ts.array_rank == 0) {
          if (is_float_base(lhs_ts.base) || is_float_base(rhs_ts.base)) {
            op_ts.base = (lhs_ts.base == TB_DOUBLE || rhs_ts.base == TB_DOUBLE ||
                          lhs_ts.base == TB_LONGDOUBLE || rhs_ts.base == TB_LONGDOUBLE)
                             ? TB_DOUBLE
                             : TB_FLOAT;
          } else if (is_any_int(lhs_ts.base) && is_any_int(rhs_ts.base)) {
            const bool is_shift = (row.bop == BinaryOp::Shl || row.bop == BinaryOp::Shr);
            op_ts.base = is_shift ? integer_promote(lhs_ts.base)
                                  : usual_arith_conv(lhs_ts.base, rhs_ts.base);
          }
        }
        rhs = coerce(ctx, rhs, rhs_ts, op_ts);
        const bool lf = is_float_base(op_ts.base);
        const bool ls = is_signed_int(op_ts.base);
        // Re-use binary arith table by emitting inline
        static const struct { BinaryOp op; const char* is; const char* iu; const char* f; } tbl[] = {
          { BinaryOp::Add, "add",  "add",  "fadd" }, { BinaryOp::Sub, "sub", "sub", "fsub" },
          { BinaryOp::Mul, "mul",  "mul",  "fmul" }, { BinaryOp::Div, "sdiv","udiv","fdiv"  },
          { BinaryOp::Mod, "srem", "urem", nullptr }, { BinaryOp::Shl, "shl","shl", nullptr  },
          { BinaryOp::Shr, "ashr","lshr", nullptr  }, { BinaryOp::BitAnd,"and","and",nullptr  },
          { BinaryOp::BitOr, "or","or",   nullptr  }, { BinaryOp::BitXor,"xor","xor",nullptr },
        };
        for (const auto& r : tbl) {
          if (r.op == row.bop) {
            if (lf) instr = r.f;
            else if (r.op == BinaryOp::Shr) instr = is_signed_int(lhs_ts.base) ? r.is : r.iu;
            else instr = ls ? r.is : r.iu;
            break;
          }
        }
        break;
      }
      if (!instr) throw std::runtime_error("HirEmitter: compound assign: unknown op");

      const std::string op_ty = llvm_ty(op_ts);
      std::string lhs_op = loaded;
      if (op_ty != lty) lhs_op = coerce(ctx, loaded, lhs_ts, op_ts);
      const std::string result = fresh_tmp(ctx);
      emit_instr(ctx, result + " = " + instr + " " + op_ty + " " + lhs_op + ", " + rhs);
      std::string store_v = result;
      if (op_ty != lty) store_v = coerce(ctx, result, op_ts, lhs_ts);
      emit_instr(ctx, "store " + lty + " " + store_v + ", ptr " + lhs_ptr);
      return store_v;
    }

    rhs = coerce(ctx, rhs, rhs_ts, lhs_ts);
    const bool is_agg = (lhs_ts.base == TB_STRUCT || lhs_ts.base == TB_UNION) &&
                        lhs_ts.ptr_level == 0 && lhs_ts.array_rank == 0;
    if (is_agg && (rhs == "0" || rhs.empty())) rhs = "zeroinitializer";
    emit_instr(ctx, "store " + lty + " " + rhs + ", ptr " + lhs_ptr);
    return rhs;
  }

  // ── Cast ─────────────────────────────────────────────────────────────────

  std::string emit_rval_payload(FnCtx& ctx, const CastExpr& c, const Expr& /*e*/) {
    TypeSpec from_ts{};
    const std::string val = emit_rval_id(ctx, c.expr, from_ts);
    return coerce(ctx, val, from_ts, c.to_type.spec);
  }

  // ── Call ─────────────────────────────────────────────────────────────────

  std::string emit_rval_payload(FnCtx& ctx, const CallExpr& call, const Expr& e) {
    TypeSpec callee_ts{};
    std::string callee_val;
    const Expr& callee_e = get_expr(call.callee);
    bool unresolved_external_callee = false;
    std::string unresolved_external_name;
    if (const auto* r = std::get_if<DeclRef>(&callee_e.payload);
        r && !r->local && !r->param_index && !r->global) {
      // Treat unresolved decl refs in call position as external functions.
      callee_val = "@" + r->name;
      callee_ts = resolve_payload_type(ctx, *r);
      unresolved_external_callee = true;
      unresolved_external_name = r->name;
    } else {
      callee_val = emit_rval_id(ctx, call.callee, callee_ts);
    }
    TypeSpec ret_spec = resolve_payload_type(ctx, call);
    if (ret_spec.base == TB_VOID && ret_spec.ptr_level == 0 && ret_spec.array_rank == 0) {
      ret_spec = e.type.spec;
    }
    const std::string ret_ty = llvm_ty(ret_spec);
    if (unresolved_external_callee) {
      record_extern_call_decl(unresolved_external_name, ret_ty);
    }

    // Look up function signature for argument type coercion
    std::string fn_name;
    if (const auto* r = std::get_if<DeclRef>(&callee_e.payload)) {
      fn_name = r->name;
    }

    // Handle GCC/Clang builtins
    if (!fn_name.empty() && fn_name.substr(0, 10) == "__builtin_") {
      if (fn_name == "__builtin_memcpy" && call.args.size() >= 3) {
        TypeSpec dst_ts{};
        TypeSpec src_ts{};
        TypeSpec size_ts{};
        const std::string dst = emit_rval_id(ctx, call.args[0], dst_ts);
        const std::string src = emit_rval_id(ctx, call.args[1], src_ts);
        std::string size = emit_rval_id(ctx, call.args[2], size_ts);
        TypeSpec i64_ts{};
        i64_ts.base = TB_ULONGLONG;
        size = coerce(ctx, size, size_ts, i64_ts);
        need_llvm_memcpy_ = true;
        emit_instr(ctx, "call void @llvm.memcpy.p0.p0.i64(ptr " + dst + ", ptr " + src +
                            ", i64 " + size + ", i1 false)");
        return dst;
      }
      if (fn_name == "__builtin_va_start" && call.args.size() >= 1) {
        TypeSpec ap_ts{};
        const std::string ap_ptr = emit_va_list_obj_ptr(ctx, call.args[0], ap_ts);
        need_llvm_va_start_ = true;
        emit_instr(ctx, "call void @llvm.va_start.p0(ptr " + ap_ptr + ")");
        return "";
      }
      if (fn_name == "__builtin_va_end" && call.args.size() >= 1) {
        TypeSpec ap_ts{};
        const std::string ap_ptr = emit_va_list_obj_ptr(ctx, call.args[0], ap_ts);
        need_llvm_va_end_ = true;
        emit_instr(ctx, "call void @llvm.va_end.p0(ptr " + ap_ptr + ")");
        return "";
      }
      if (fn_name == "__builtin_va_copy" && call.args.size() >= 2) {
        TypeSpec dst_ts{};
        TypeSpec src_ts{};
        const std::string dst_ptr = emit_va_list_obj_ptr(ctx, call.args[0], dst_ts);
        const std::string src_ptr = emit_va_list_obj_ptr(ctx, call.args[1], src_ts);
        need_llvm_va_copy_ = true;
        emit_instr(ctx, "call void @llvm.va_copy.p0.p0(ptr " + dst_ptr + ", ptr " + src_ptr + ")");
        return "";
      }
      // bswap builtins → llvm.bswap intrinsic
      if (fn_name == "__builtin_bswap16" || fn_name == "__builtin_bswap32" ||
          fn_name == "__builtin_bswap64") {
        if (call.args.size() == 1) {
          TypeSpec arg_ts{};
          const std::string arg = emit_rval_id(ctx, call.args[0], arg_ts);
          const std::string aty = llvm_ty(arg_ts);
          const std::string intrinsic = "@llvm.bswap." + aty;
          const std::string tmp = fresh_tmp(ctx);
          emit_instr(ctx, tmp + " = call " + aty + " " + intrinsic + "(" + aty + " " + arg + ")");
          return tmp;
        }
      }
      // __builtin_expect(x, y) → just return x
      if (fn_name == "__builtin_expect" && call.args.size() >= 1) {
        TypeSpec arg_ts{};
        return emit_rval_id(ctx, call.args[0], arg_ts);
      }
      // __builtin_unreachable() → emit unreachable
      if (fn_name == "__builtin_unreachable" && call.args.empty()) {
        emit_term(ctx, "unreachable");
        return "";
      }
      // __builtin_huge_val / __builtin_inf → double +Inf
      if (fn_name == "__builtin_huge_val" || fn_name == "__builtin_huge_vall" ||
          fn_name == "__builtin_inf"      || fn_name == "__builtin_infl") {
        const TypeBase base = (fn_name == "__builtin_huge_vall" || fn_name == "__builtin_infl")
                                  ? TB_LONGDOUBLE
                                  : TB_DOUBLE;
        return fp_literal(base, std::numeric_limits<double>::infinity());
      }
      // __builtin_huge_valf / __builtin_inff → float +Inf (as double hex for LLVM)
      if (fn_name == "__builtin_huge_valf" || fn_name == "__builtin_inff") {
        return fp_to_float_literal(std::numeric_limits<float>::infinity());
      }
      // __builtin_nan / __builtin_nans → double NaN
      if (fn_name == "__builtin_nan" || fn_name == "__builtin_nans" ||
          fn_name == "__builtin_nanl" || fn_name == "__builtin_nansl") {
        const TypeBase base = (fn_name == "__builtin_nanl" || fn_name == "__builtin_nansl")
                                  ? TB_LONGDOUBLE
                                  : TB_DOUBLE;
        return fp_literal(base, std::numeric_limits<double>::quiet_NaN());
      }
      // __builtin_nanf / __builtin_nansf → float NaN
      if (fn_name == "__builtin_nanf" || fn_name == "__builtin_nansf") {
        return fp_to_float_literal(std::numeric_limits<float>::quiet_NaN());
      }
      // IEEE ordered/unordered comparison builtins
      // These are semantically: __builtin_isXXX(a, b) → fcmp XXX a, b (int result)
      static const struct { const char* name; const char* pred; } fp_cmp_builtins[] = {
        { "__builtin_isless",         "olt" },
        { "__builtin_islessequal",    "ole" },
        { "__builtin_isgreater",      "ogt" },
        { "__builtin_isgreaterequal", "oge" },
        { "__builtin_islessgreater",  "one" },
        { nullptr, nullptr }
      };
      for (const auto* bc = fp_cmp_builtins; bc->name; ++bc) {
        if (fn_name == bc->name && call.args.size() == 2) {
          TypeSpec at{}, bt{};
          std::string a = emit_rval_id(ctx, call.args[0], at);
          std::string b = emit_rval_id(ctx, call.args[1], bt);
          // Promote float to double if needed
          auto ensure_double = [&](std::string& v, TypeSpec& ts) {
            if (ts.base == TB_FLOAT && ts.ptr_level == 0 && ts.array_rank == 0) {
              const std::string p = fresh_tmp(ctx);
              emit_instr(ctx, p + " = fpext float " + v + " to double");
              v = p; ts.base = TB_DOUBLE;
            }
          };
          ensure_double(a, at); ensure_double(b, bt);
          const std::string fty = llvm_ty(at);
          const std::string cmp = fresh_tmp(ctx);
          emit_instr(ctx, cmp + " = fcmp " + std::string(bc->pred) + " " + fty + " " + a + ", " + b);
          const std::string tmp = fresh_tmp(ctx);
          emit_instr(ctx, tmp + " = zext i1 " + cmp + " to i32");
          return tmp;
        }
      }
      // __builtin_isunordered(a, b) → fcmp uno a, b (1 if either is NaN)
      if (fn_name == "__builtin_isunordered" && call.args.size() == 2) {
        TypeSpec at{}, bt{};
        std::string a = emit_rval_id(ctx, call.args[0], at);
        std::string b = emit_rval_id(ctx, call.args[1], bt);
        // Promote to double for comparison
        auto promote_to_double = [&](std::string& v, TypeSpec& ts) {
          if (ts.base == TB_FLOAT && ts.ptr_level == 0 && ts.array_rank == 0) {
            const std::string p = fresh_tmp(ctx);
            emit_instr(ctx, p + " = fpext float " + v + " to double");
            v = p; ts.base = TB_DOUBLE;
          }
        };
        promote_to_double(a, at);
        promote_to_double(b, bt);
        const std::string fty = llvm_ty(at);
        const std::string cmp = fresh_tmp(ctx);
        emit_instr(ctx, cmp + " = fcmp uno " + fty + " " + a + ", " + b);
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = zext i1 " + cmp + " to i32");
        return tmp;
      }
      // __builtin_isnan(a) → fcmp uno a, a
      if ((fn_name == "__builtin_isnan" || fn_name == "__builtin_isnanf" ||
           fn_name == "__builtin_isnanl") && call.args.size() == 1) {
        TypeSpec at{};
        std::string a = emit_rval_id(ctx, call.args[0], at);
        if (at.base == TB_FLOAT && at.ptr_level == 0 && at.array_rank == 0) {
          const std::string p = fresh_tmp(ctx);
          emit_instr(ctx, p + " = fpext float " + a + " to double");
          a = p; at.base = TB_DOUBLE;
        }
        const std::string fty = llvm_ty(at);
        const std::string cmp = fresh_tmp(ctx);
        emit_instr(ctx, cmp + " = fcmp uno " + fty + " " + a + ", " + a);
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = zext i1 " + cmp + " to i32");
        return tmp;
      }
      // __builtin_isinf(a) → abs(a) == infinity
      if ((fn_name == "__builtin_isinf" || fn_name == "__builtin_isinff" ||
           fn_name == "__builtin_isinfl") && call.args.size() == 1) {
        TypeSpec at{};
        std::string a = emit_rval_id(ctx, call.args[0], at);
        if (at.base == TB_FLOAT && at.ptr_level == 0 && at.array_rank == 0) {
          const std::string p = fresh_tmp(ctx);
          emit_instr(ctx, p + " = fpext float " + a + " to double");
          a = p; at.base = TB_DOUBLE;
        }
        const std::string fty = llvm_ty(at);
        const std::string inf_val = fp_literal(at.base, std::numeric_limits<double>::infinity());
        // Check if a == +inf or a == -inf by comparing abs(a) to +inf
        const std::string abs_tmp = fresh_tmp(ctx);
        emit_instr(ctx, abs_tmp + " = call " + fty + " @llvm.fabs." + fty + "(" + fty + " " + a + ")");
        const std::string cmp = fresh_tmp(ctx);
        emit_instr(ctx, cmp + " = fcmp oeq " + fty + " " + abs_tmp + ", " + inf_val);
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = zext i1 " + cmp + " to i32");
        return tmp;
      }
      // __builtin_isfinite(a) → abs(a) < infinity
      if ((fn_name == "__builtin_isfinite" || fn_name == "__builtin_isfinitef" ||
           fn_name == "__builtin_isfinitel" || fn_name == "__builtin_finite" ||
           fn_name == "__builtin_finitef" || fn_name == "__builtin_finitel") && call.args.size() == 1) {
        TypeSpec at{};
        std::string a = emit_rval_id(ctx, call.args[0], at);
        if (at.base == TB_FLOAT && at.ptr_level == 0 && at.array_rank == 0) {
          const std::string p = fresh_tmp(ctx);
          emit_instr(ctx, p + " = fpext float " + a + " to double");
          a = p; at.base = TB_DOUBLE;
        }
        const std::string fty = llvm_ty(at);
        const std::string inf_val = fp_literal(at.base, std::numeric_limits<double>::infinity());
        const std::string abs_tmp = fresh_tmp(ctx);
        emit_instr(ctx, abs_tmp + " = call " + fty + " @llvm.fabs." + fty + "(" + fty + " " + a + ")");
        const std::string cmp = fresh_tmp(ctx);
        emit_instr(ctx, cmp + " = fcmp olt " + fty + " " + abs_tmp + ", " + inf_val);
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = zext i1 " + cmp + " to i32");
        return tmp;
      }
      // __builtin_copysign / __builtin_copysignf / __builtin_copysignl
      // → llvm.copysign intrinsic
      if ((fn_name == "__builtin_copysign" || fn_name == "__builtin_copysignl" ||
           fn_name == "__builtin_copysignf") && call.args.size() == 2) {
        TypeSpec at{}, bt{};
        std::string a = emit_rval_id(ctx, call.args[0], at);
        std::string b = emit_rval_id(ctx, call.args[1], bt);
        const bool is_float = (fn_name == "__builtin_copysignf");
        const bool is_long_double = (fn_name == "__builtin_copysignl");
        if (is_float) {
          // Ensure both are float
          auto to_float = [&](std::string& v, TypeSpec& ts) {
            if (ts.base == TB_DOUBLE && ts.ptr_level == 0 && ts.array_rank == 0) {
              const std::string p = fresh_tmp(ctx); emit_instr(ctx, p + " = fptrunc double " + v + " to float");
              v = p; ts.base = TB_FLOAT;
            }
          };
          to_float(a, at); to_float(b, bt);
          const std::string tmp = fresh_tmp(ctx);
          emit_instr(ctx, tmp + " = call float @llvm.copysign.f32(float " + a + ", float " + b + ")");
          return tmp;
        } else if (is_long_double) {
          auto to_fp128 = [&](std::string& v, TypeSpec& ts) {
            if (ts.base == TB_FLOAT && ts.ptr_level == 0 && ts.array_rank == 0) {
              const std::string p = fresh_tmp(ctx);
              emit_instr(ctx, p + " = fpext float " + v + " to fp128");
              v = p; ts.base = TB_LONGDOUBLE;
            } else if (ts.base == TB_DOUBLE && ts.ptr_level == 0 && ts.array_rank == 0) {
              const std::string p = fresh_tmp(ctx);
              emit_instr(ctx, p + " = fpext double " + v + " to fp128");
              v = p; ts.base = TB_LONGDOUBLE;
            }
          };
          to_fp128(a, at); to_fp128(b, bt);
          const std::string tmp = fresh_tmp(ctx);
          emit_instr(ctx, tmp + " = call fp128 @llvm.copysign.f128(fp128 " + a + ", fp128 " + b + ")");
          return tmp;
        } else {
          // Ensure both are double
          auto to_double = [&](std::string& v, TypeSpec& ts) {
            if (ts.base == TB_FLOAT && ts.ptr_level == 0 && ts.array_rank == 0) {
              const std::string p = fresh_tmp(ctx); emit_instr(ctx, p + " = fpext float " + v + " to double");
              v = p; ts.base = TB_DOUBLE;
            }
          };
          to_double(a, at); to_double(b, bt);
          const std::string tmp = fresh_tmp(ctx);
          emit_instr(ctx, tmp + " = call double @llvm.copysign.f64(double " + a + ", double " + b + ")");
          return tmp;
        }
      }
      // __builtin_fabs / __builtin_fabsf / __builtin_fabsl → llvm.fabs intrinsic
      if ((fn_name == "__builtin_fabs" || fn_name == "__builtin_fabsl" ||
           fn_name == "__builtin_fabsf") && call.args.size() == 1) {
        TypeSpec arg_ts{};
        std::string arg = emit_rval_id(ctx, call.args[0], arg_ts);
        // Promote float arg to the expected width for fabs/fabsl.
        if (fn_name == "__builtin_fabs") {
          if (arg_ts.base == TB_FLOAT && arg_ts.ptr_level == 0 && arg_ts.array_rank == 0) {
            const std::string promoted = fresh_tmp(ctx);
            emit_instr(ctx, promoted + " = fpext float " + arg + " to double");
            arg = promoted;
            arg_ts.base = TB_DOUBLE;
          }
        } else if (fn_name == "__builtin_fabsl") {
          if (arg_ts.base == TB_FLOAT && arg_ts.ptr_level == 0 && arg_ts.array_rank == 0) {
            const std::string promoted = fresh_tmp(ctx);
            emit_instr(ctx, promoted + " = fpext float " + arg + " to fp128");
            arg = promoted;
            arg_ts.base = TB_LONGDOUBLE;
          } else if (arg_ts.base == TB_DOUBLE && arg_ts.ptr_level == 0 && arg_ts.array_rank == 0) {
            const std::string promoted = fresh_tmp(ctx);
            emit_instr(ctx, promoted + " = fpext double " + arg + " to fp128");
            arg = promoted;
            arg_ts.base = TB_LONGDOUBLE;
          }
        } else {
          // fabsf: ensure float
          if (arg_ts.base == TB_DOUBLE && arg_ts.ptr_level == 0 && arg_ts.array_rank == 0) {
            const std::string trunc = fresh_tmp(ctx);
            emit_instr(ctx, trunc + " = fptrunc double " + arg + " to float");
            arg = trunc;
            arg_ts.base = TB_FLOAT;
          }
        }
        const std::string fty = llvm_ty(arg_ts);
        const std::string intrinsic = "@llvm.fabs." + fty;
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = call " + fty + " " + intrinsic + "(" + fty + " " + arg + ")");
        return tmp;
      }
      // __builtin_ffs{,l,ll}(x) → (x==0) ? 0 : (cttz(x)+1)
      if ((fn_name == "__builtin_ffs"  || fn_name == "__builtin_ffsl" ||
           fn_name == "__builtin_ffsll") && call.args.size() == 1) {
        TypeSpec arg_ts{};
        std::string arg = emit_rval_id(ctx, call.args[0], arg_ts);
        // Coerce to i32 or i64 depending on type
        const bool is_ll = (fn_name == "__builtin_ffsll");
        const std::string ity = is_ll ? "i64" : "i32";
        TypeSpec target_ts{}; target_ts.base = is_ll ? TB_LONGLONG : TB_INT;
        arg = coerce(ctx, arg, arg_ts, target_ts);
        const std::string cttz = fresh_tmp(ctx);
        emit_instr(ctx, cttz + " = call " + ity + " @llvm.cttz." + ity +
                             "(" + ity + " " + arg + ", i1 false)");
        const std::string plus1 = fresh_tmp(ctx);
        emit_instr(ctx, plus1 + " = add " + ity + " " + cttz + ", 1");
        const std::string is_zero = fresh_tmp(ctx);
        emit_instr(ctx, is_zero + " = icmp eq " + ity + " " + arg + ", 0");
        const std::string sel = fresh_tmp(ctx);
        emit_instr(ctx, sel + " = select i1 " + is_zero + ", " + ity + " 0, " + ity + " " + plus1);
        // Always return i32
        if (is_ll) {
          const std::string trunc = fresh_tmp(ctx);
          emit_instr(ctx, trunc + " = trunc i64 " + sel + " to i32");
          return trunc;
        }
        return sel;
      }
      // __builtin_ctz{,l,ll}(x) → cttz(x, undef_if_zero=true)
      if ((fn_name == "__builtin_ctz"  || fn_name == "__builtin_ctzl" ||
           fn_name == "__builtin_ctzll") && call.args.size() == 1) {
        TypeSpec arg_ts{};
        std::string arg = emit_rval_id(ctx, call.args[0], arg_ts);
        const bool is_ll = (fn_name == "__builtin_ctzll");
        const std::string ity = is_ll ? "i64" : "i32";
        TypeSpec target_ts{}; target_ts.base = is_ll ? TB_LONGLONG : TB_INT;
        arg = coerce(ctx, arg, arg_ts, target_ts);
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = call " + ity + " @llvm.cttz." + ity +
                             "(" + ity + " " + arg + ", i1 true)");
        if (is_ll) {
          const std::string trunc = fresh_tmp(ctx);
          emit_instr(ctx, trunc + " = trunc i64 " + tmp + " to i32");
          return trunc;
        }
        return tmp;
      }
      // __builtin_clz{,l,ll}(x) → ctlz(x, undef_if_zero=true)
      if ((fn_name == "__builtin_clz"  || fn_name == "__builtin_clzl" ||
           fn_name == "__builtin_clzll") && call.args.size() == 1) {
        TypeSpec arg_ts{};
        std::string arg = emit_rval_id(ctx, call.args[0], arg_ts);
        const bool is_ll = (fn_name == "__builtin_clzll");
        const std::string ity = is_ll ? "i64" : "i32";
        TypeSpec target_ts{}; target_ts.base = is_ll ? TB_LONGLONG : TB_INT;
        arg = coerce(ctx, arg, arg_ts, target_ts);
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = call " + ity + " @llvm.ctlz." + ity +
                             "(" + ity + " " + arg + ", i1 true)");
        if (is_ll) {
          const std::string trunc = fresh_tmp(ctx);
          emit_instr(ctx, trunc + " = trunc i64 " + tmp + " to i32");
          return trunc;
        }
        return tmp;
      }
      // __builtin_popcount{,l,ll}(x) → ctpop(x)
      if ((fn_name == "__builtin_popcount"  || fn_name == "__builtin_popcountl" ||
           fn_name == "__builtin_popcountll") && call.args.size() == 1) {
        TypeSpec arg_ts{};
        std::string arg = emit_rval_id(ctx, call.args[0], arg_ts);
        const bool is_ll = (fn_name == "__builtin_popcountll");
        const std::string ity = is_ll ? "i64" : "i32";
        TypeSpec target_ts{}; target_ts.base = is_ll ? TB_LONGLONG : TB_INT;
        arg = coerce(ctx, arg, arg_ts, target_ts);
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = call " + ity + " @llvm.ctpop." + ity +
                             "(" + ity + " " + arg + ")");
        if (is_ll) {
          const std::string trunc = fresh_tmp(ctx);
          emit_instr(ctx, trunc + " = trunc i64 " + tmp + " to i32");
          return trunc;
        }
        return tmp;
      }
      // __builtin_parity{,l,ll}(x) → ctpop(x) & 1
      if ((fn_name == "__builtin_parity"  || fn_name == "__builtin_parityl" ||
           fn_name == "__builtin_parityll") && call.args.size() == 1) {
        TypeSpec arg_ts{};
        std::string arg = emit_rval_id(ctx, call.args[0], arg_ts);
        const bool is_ll = (fn_name == "__builtin_parityll");
        const std::string ity = is_ll ? "i64" : "i32";
        TypeSpec target_ts{}; target_ts.base = is_ll ? TB_LONGLONG : TB_INT;
        arg = coerce(ctx, arg, arg_ts, target_ts);
        const std::string pop = fresh_tmp(ctx);
        emit_instr(ctx, pop + " = call " + ity + " @llvm.ctpop." + ity +
                            "(" + ity + " " + arg + ")");
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = and " + ity + " " + pop + ", 1");
        if (is_ll) {
          const std::string trunc = fresh_tmp(ctx);
          emit_instr(ctx, trunc + " = trunc i64 " + tmp + " to i32");
          return trunc;
        }
        return tmp;
      }
      if ((fn_name == "__builtin_conj" || fn_name == "__builtin_conjf" ||
           fn_name == "__builtin_conjl") && call.args.size() == 1) {
        TypeSpec arg_ts{};
        std::string arg = emit_rval_id(ctx, call.args[0], arg_ts);
        if (!is_complex_base(arg_ts.base)) return arg;
        const TypeSpec elem_ts = complex_component_ts(arg_ts.base);
        const std::string complex_ty = llvm_ty(arg_ts);
        const std::string elem_ty = llvm_ty(elem_ts);
        const std::string real_v = fresh_tmp(ctx);
        emit_instr(ctx, real_v + " = extractvalue " + complex_ty + " " + arg + ", 0");
        const std::string imag_v0 = fresh_tmp(ctx);
        emit_instr(ctx, imag_v0 + " = extractvalue " + complex_ty + " " + arg + ", 1");
        const std::string imag_v = fresh_tmp(ctx);
        if (is_float_base(elem_ts.base)) {
          emit_instr(ctx, imag_v + " = fneg " + elem_ty + " " + imag_v0);
        } else {
          emit_instr(ctx, imag_v + " = sub " + elem_ty + " 0, " + imag_v0);
        }
        const std::string with_real = fresh_tmp(ctx);
        emit_instr(ctx, with_real + " = insertvalue " + complex_ty + " undef, " + elem_ty +
                                 " " + real_v + ", 0");
        const std::string out = fresh_tmp(ctx);
        emit_instr(ctx, out + " = insertvalue " + complex_ty + " " + with_real + ", " +
                            elem_ty + " " + imag_v + ", 1");
        return out;
      }
      // Unknown builtin: emit as 0/null
      if (ret_ty == "void") return "";
      if (!ret_ty.empty() && (ret_ty[0] == '{' || ret_ty[0] == '%' || ret_ty[0] == '['))
        return "zeroinitializer";
      return (ret_ty == "ptr") ? "null" : "0";
    }

    const Function* target_fn = nullptr;
    if (!fn_name.empty()) {
      const auto fit = mod_.fn_index.find(fn_name);
      if (fit != mod_.fn_index.end() && fit->second.value < mod_.functions.size()) {
        target_fn = &mod_.functions[fit->second.value];
      }
    }
    const FnPtrSig* callee_fn_ptr_sig = target_fn ? nullptr : resolve_callee_fn_ptr_sig(ctx, callee_e);

    auto callee_needs_va_list_by_value_copy = [&](size_t arg_index) -> bool {
      if (target_fn) {
        if (arg_index < target_fn->params.size()) {
          const TypeSpec& param_ts = target_fn->params[arg_index].type.spec;
          return param_ts.base == TB_VA_LIST &&
                 param_ts.ptr_level == 0 &&
                 param_ts.array_rank == 0;
        }
        return false;
      }
      if (callee_fn_ptr_sig) {
        if (arg_index < callee_fn_ptr_sig->params.size()) {
          const TypeSpec& param_ts = callee_fn_ptr_sig->params[arg_index].spec;
          return param_ts.base == TB_VA_LIST &&
                 param_ts.ptr_level == 0 &&
                 param_ts.array_rank == 0;
        }
        return false;
      }
      return fn_name == "vprintf"   || fn_name == "vfprintf" ||
             fn_name == "vsprintf"  || fn_name == "vsnprintf" ||
             fn_name == "vscanf"    || fn_name == "vfscanf" ||
             fn_name == "vsscanf"   || fn_name == "vasprintf" ||
             fn_name == "vdprintf";
    };

    std::string args_str;
    auto apply_default_arg_promotion = [&](std::string& arg, TypeSpec& out_ts, const TypeSpec& in_ts) {
      TypeSpec promoted = in_ts;
      if (promoted.array_rank > 0 && !promoted.is_ptr_to_array) {
        promoted.array_rank = 0;
        promoted.array_size = -1;
        promoted.ptr_level += 1;
      }
      if (promoted.ptr_level == 0 && promoted.array_rank == 0) {
        if (promoted.base == TB_FLOAT) {
          promoted.base = TB_DOUBLE;
        } else if (promoted.base == TB_BOOL || promoted.base == TB_CHAR ||
                   promoted.base == TB_SCHAR || promoted.base == TB_UCHAR ||
                   promoted.base == TB_SHORT || promoted.base == TB_USHORT) {
          promoted.base = TB_INT;
        }
      }
      arg = coerce(ctx, arg, in_ts, promoted);
      out_ts = promoted;
    };
    for (size_t i = 0; i < call.args.size(); ++i) {
      TypeSpec arg_ts{};
      std::string arg = emit_rval_id(ctx, call.args[i], arg_ts);
      TypeSpec out_arg_ts = arg_ts;
      const bool is_va_list_value =
          arg_ts.base == TB_VA_LIST &&
          arg_ts.ptr_level == 0 &&
          arg_ts.array_rank == 0;
      const bool is_variadic_aggregate =
          target_fn && target_fn->attrs.variadic && i >= target_fn->params.size() &&
          (arg_ts.base == TB_STRUCT || arg_ts.base == TB_UNION) &&
          arg_ts.ptr_level == 0 && arg_ts.array_rank == 0 &&
          arg_ts.tag && arg_ts.tag[0];
      if (target_fn) {
        const bool has_void_param_list =
            target_fn->params.size() == 1 &&
            target_fn->params[0].type.spec.base == TB_VOID &&
            target_fn->params[0].type.spec.ptr_level == 0 &&
            target_fn->params[0].type.spec.array_rank == 0;
        if (!has_void_param_list && i < target_fn->params.size()) {
          out_arg_ts = target_fn->params[i].type.spec;
          arg = coerce(ctx, arg, arg_ts, out_arg_ts);
        } else if (target_fn->attrs.variadic && !is_variadic_aggregate) {
          apply_default_arg_promotion(arg, out_arg_ts, arg_ts);
        }
      } else if (callee_fn_ptr_sig) {
        const bool has_void_param_list =
            callee_fn_ptr_sig->params.size() == 1 &&
            callee_fn_ptr_sig->params[0].spec.base == TB_VOID &&
            callee_fn_ptr_sig->params[0].spec.ptr_level == 0 &&
            callee_fn_ptr_sig->params[0].spec.array_rank == 0;
        if (!has_void_param_list && i < callee_fn_ptr_sig->params.size()) {
          out_arg_ts = callee_fn_ptr_sig->params[i].spec;
          arg = coerce(ctx, arg, arg_ts, out_arg_ts);
        } else if (callee_fn_ptr_sig->variadic && !is_variadic_aggregate) {
          apply_default_arg_promotion(arg, out_arg_ts, arg_ts);
        }
      }
      if (is_va_list_value && callee_needs_va_list_by_value_copy(i)) {
        TypeSpec ap_ts{};
        const std::string src_ptr = emit_va_list_obj_ptr(ctx, call.args[i], ap_ts);
        const std::string tmp_addr = fresh_tmp(ctx);
        emit_instr(ctx, tmp_addr + " = alloca %struct.__va_list_tag_");
        need_llvm_memcpy_ = true;
        emit_instr(ctx, "call void @llvm.memcpy.p0.p0.i64(ptr " + tmp_addr + ", ptr " +
                            src_ptr + ", i64 32, i1 false)");
        arg = tmp_addr;
        out_arg_ts = arg_ts;
      }
      if (is_variadic_aggregate) {
        const int payload_sz = compute_struct_size(mod_, arg_ts.tag);
        if (payload_sz == 0) continue;

        std::string obj_ptr;
        if (get_expr(call.args[i]).type.category == ValueCategory::LValue) {
          TypeSpec obj_ts{};
          obj_ptr = emit_lval(ctx, call.args[i], obj_ts);
        } else {
          const std::string tmp_addr = fresh_tmp(ctx);
          emit_instr(ctx, tmp_addr + " = alloca " + llvm_ty(arg_ts));
          emit_instr(ctx, "store " + llvm_ty(arg_ts) + " " + arg + ", ptr " + tmp_addr);
          obj_ptr = tmp_addr;
        }

        need_llvm_memcpy_ = true;
        if (i) args_str += ", ";
        if (payload_sz > 16) {
          args_str += "ptr " + obj_ptr;
          continue;
        }
        if (payload_sz <= 8) {
          const std::string tmp_addr = fresh_tmp(ctx);
          emit_instr(ctx, tmp_addr + " = alloca i64");
          emit_instr(ctx, "call void @llvm.memcpy.p0.p0.i64(ptr " + tmp_addr + ", ptr " + obj_ptr +
                              ", i64 " + std::to_string(payload_sz) + ", i1 false)");
          const std::string packed = fresh_tmp(ctx);
          emit_instr(ctx, packed + " = load i64, ptr " + tmp_addr);
          args_str += "i64 " + packed;
          continue;
        }

        const std::string tmp_addr = fresh_tmp(ctx);
        emit_instr(ctx, tmp_addr + " = alloca [2 x i64]");
        emit_instr(ctx, "call void @llvm.memcpy.p0.p0.i64(ptr " + tmp_addr + ", ptr " + obj_ptr +
                            ", i64 " + std::to_string(payload_sz) + ", i1 false)");
        const std::string packed = fresh_tmp(ctx);
        emit_instr(ctx, packed + " = load [2 x i64], ptr " + tmp_addr);
        args_str += "[2 x i64] " + packed;
        continue;
      }
      if (i) args_str += ", ";
      args_str += llvm_ty(out_arg_ts) + " " + arg;
    }

    if (ret_ty == "void") {
      emit_instr(ctx, "call void " + callee_val + "(" + args_str + ")");
      return "";
    }
    const std::string tmp = fresh_tmp(ctx);
    emit_instr(ctx, tmp + " = call " + ret_ty + " " + callee_val + "(" + args_str + ")");
    return tmp;
  }

  // ── VaArg ────────────────────────────────────────────────────────────────

  std::string emit_aarch64_vaarg_gp_src_ptr(FnCtx& ctx, const std::string& ap_ptr, int slot_bytes) {
    const std::string offs_ptr = fresh_tmp(ctx);
    emit_instr(ctx, offs_ptr + " = getelementptr %struct.__va_list_tag_, ptr " + ap_ptr +
                            ", i32 0, i32 3");
    const std::string offs = fresh_tmp(ctx);
    emit_instr(ctx, offs + " = load i32, ptr " + offs_ptr);

    const std::string stack_lbl = fresh_lbl(ctx, "vaarg.stack.");
    const std::string reg_try_lbl = fresh_lbl(ctx, "vaarg.regtry.");
    const std::string reg_lbl = fresh_lbl(ctx, "vaarg.reg.");
    const std::string join_lbl = fresh_lbl(ctx, "vaarg.join.");

    const std::string is_stack0 = fresh_tmp(ctx);
    emit_instr(ctx, is_stack0 + " = icmp sge i32 " + offs + ", 0");
    emit_term(ctx, "br i1 " + is_stack0 + ", label %" + stack_lbl + ", label %" + reg_try_lbl);

    emit_lbl(ctx, reg_try_lbl);
    const std::string next_offs = fresh_tmp(ctx);
    emit_instr(ctx, next_offs + " = add i32 " + offs + ", " + std::to_string(slot_bytes));
    emit_instr(ctx, "store i32 " + next_offs + ", ptr " + offs_ptr);
    const std::string use_reg = fresh_tmp(ctx);
    emit_instr(ctx, use_reg + " = icmp sle i32 " + next_offs + ", 0");
    emit_term(ctx, "br i1 " + use_reg + ", label %" + reg_lbl + ", label %" + stack_lbl);

    emit_lbl(ctx, reg_lbl);
    const std::string gr_top_ptr = fresh_tmp(ctx);
    emit_instr(ctx, gr_top_ptr + " = getelementptr %struct.__va_list_tag_, ptr " + ap_ptr +
                             ", i32 0, i32 1");
    const std::string gr_top = fresh_tmp(ctx);
    emit_instr(ctx, gr_top + " = load ptr, ptr " + gr_top_ptr);
    const std::string reg_addr = fresh_tmp(ctx);
    emit_instr(ctx, reg_addr + " = getelementptr i8, ptr " + gr_top + ", i32 " + offs);
    emit_term(ctx, "br label %" + join_lbl);

    emit_lbl(ctx, stack_lbl);
    const std::string stack_ptr_ptr = fresh_tmp(ctx);
    emit_instr(ctx, stack_ptr_ptr + " = getelementptr %struct.__va_list_tag_, ptr " + ap_ptr +
                                ", i32 0, i32 0");
    const std::string stack_ptr = fresh_tmp(ctx);
    emit_instr(ctx, stack_ptr + " = load ptr, ptr " + stack_ptr_ptr);
    const std::string stack_next = fresh_tmp(ctx);
    emit_instr(ctx, stack_next + " = getelementptr i8, ptr " + stack_ptr + ", i64 " +
                            std::to_string(slot_bytes));
    emit_instr(ctx, "store ptr " + stack_next + ", ptr " + stack_ptr_ptr);
    emit_term(ctx, "br label %" + join_lbl);

    emit_lbl(ctx, join_lbl);
    const std::string src_ptr = fresh_tmp(ctx);
    emit_instr(ctx, src_ptr + " = phi ptr [ " + reg_addr + ", %" + reg_lbl + " ], [ " +
                             stack_ptr + ", %" + stack_lbl + " ]");
    return src_ptr;
  }

  std::string emit_aarch64_vaarg_fp_src_ptr(
      FnCtx& ctx, const std::string& ap_ptr, int reg_slot_bytes, int stack_slot_bytes,
      int stack_align_bytes) {
    const std::string offs_ptr = fresh_tmp(ctx);
    emit_instr(ctx, offs_ptr + " = getelementptr %struct.__va_list_tag_, ptr " + ap_ptr +
                            ", i32 0, i32 4");
    const std::string offs = fresh_tmp(ctx);
    emit_instr(ctx, offs + " = load i32, ptr " + offs_ptr);

    const std::string stack_lbl = fresh_lbl(ctx, "vaarg.fp.stack.");
    const std::string reg_try_lbl = fresh_lbl(ctx, "vaarg.fp.regtry.");
    const std::string reg_lbl = fresh_lbl(ctx, "vaarg.fp.reg.");
    const std::string join_lbl = fresh_lbl(ctx, "vaarg.fp.join.");

    const std::string is_stack0 = fresh_tmp(ctx);
    emit_instr(ctx, is_stack0 + " = icmp sge i32 " + offs + ", 0");
    emit_term(ctx, "br i1 " + is_stack0 + ", label %" + stack_lbl + ", label %" + reg_try_lbl);

    emit_lbl(ctx, reg_try_lbl);
    const std::string next_offs = fresh_tmp(ctx);
    emit_instr(ctx, next_offs + " = add i32 " + offs + ", " + std::to_string(reg_slot_bytes));
    emit_instr(ctx, "store i32 " + next_offs + ", ptr " + offs_ptr);
    const std::string use_reg = fresh_tmp(ctx);
    emit_instr(ctx, use_reg + " = icmp sle i32 " + next_offs + ", 0");
    emit_term(ctx, "br i1 " + use_reg + ", label %" + reg_lbl + ", label %" + stack_lbl);

    emit_lbl(ctx, reg_lbl);
    const std::string vr_top_ptr = fresh_tmp(ctx);
    emit_instr(ctx, vr_top_ptr + " = getelementptr %struct.__va_list_tag_, ptr " + ap_ptr +
                             ", i32 0, i32 2");
    const std::string vr_top = fresh_tmp(ctx);
    emit_instr(ctx, vr_top + " = load ptr, ptr " + vr_top_ptr);
    const std::string reg_addr = fresh_tmp(ctx);
    emit_instr(ctx, reg_addr + " = getelementptr i8, ptr " + vr_top + ", i32 " + offs);
    emit_term(ctx, "br label %" + join_lbl);

    emit_lbl(ctx, stack_lbl);
    const std::string stack_ptr_ptr = fresh_tmp(ctx);
    emit_instr(ctx, stack_ptr_ptr + " = getelementptr %struct.__va_list_tag_, ptr " + ap_ptr +
                                ", i32 0, i32 0");
    const std::string stack_ptr = fresh_tmp(ctx);
    emit_instr(ctx, stack_ptr + " = load ptr, ptr " + stack_ptr_ptr);
    std::string aligned_stack_ptr = stack_ptr;
    if (stack_align_bytes > 1) {
      const std::string stack_i = fresh_tmp(ctx);
      emit_instr(ctx, stack_i + " = ptrtoint ptr " + stack_ptr + " to i64");
      const std::string plus_mask = fresh_tmp(ctx);
      emit_instr(ctx, plus_mask + " = add i64 " + stack_i + ", " +
                              std::to_string(stack_align_bytes - 1));
      const std::string masked = fresh_tmp(ctx);
      emit_instr(ctx, masked + " = and i64 " + plus_mask + ", " +
                              std::to_string(-stack_align_bytes));
      aligned_stack_ptr = fresh_tmp(ctx);
      emit_instr(ctx, aligned_stack_ptr + " = inttoptr i64 " + masked + " to ptr");
    }
    const std::string stack_next = fresh_tmp(ctx);
    emit_instr(ctx, stack_next + " = getelementptr i8, ptr " + aligned_stack_ptr + ", i64 " +
                            std::to_string(stack_slot_bytes));
    emit_instr(ctx, "store ptr " + stack_next + ", ptr " + stack_ptr_ptr);
    emit_term(ctx, "br label %" + join_lbl);

    emit_lbl(ctx, join_lbl);
    const std::string src_ptr = fresh_tmp(ctx);
    emit_instr(ctx, src_ptr + " = phi ptr [ " + reg_addr + ", %" + reg_lbl + " ], [ " +
                             aligned_stack_ptr + ", %" + stack_lbl + " ]");
    return src_ptr;
  }

  std::string emit_rval_payload(FnCtx& ctx, const VaArgExpr& v, const Expr& e) {
    TypeSpec ap_ts{};
    const std::string ap_ptr = emit_va_list_obj_ptr(ctx, v.ap, ap_ts);
    TypeSpec res_ts = e.type.spec;
    if (!has_concrete_type(res_ts)) res_ts = resolve_expr_type(ctx, e);
    const std::string res_ty = llvm_ty(res_ts);
    if (res_ty == "void") return "";
    if ((res_ts.base == TB_STRUCT || res_ts.base == TB_UNION) &&
        res_ts.ptr_level == 0 && res_ts.array_rank == 0 &&
        res_ts.tag && res_ts.tag[0]) {
      const auto it = mod_.struct_defs.find(res_ts.tag);
      if (it != mod_.struct_defs.end()) {
        const HirStructDef& sd = it->second;
        int payload_sz = 0;
        if (sd.is_union) {
          for (const auto& f : sd.fields) {
            payload_sz = std::max(payload_sz, compute_field_size(mod_, f));
          }
        } else {
          for (const auto& f : sd.fields) payload_sz += compute_field_size(mod_, f);
        }
        // GCC zero-size aggregate extension: va_arg yields a zero-initialized value
        // and does not consume any argument slot.
        if (payload_sz == 0) return "zeroinitializer";
      }
    }
    if ((res_ts.base == TB_STRUCT || res_ts.base == TB_UNION) &&
        res_ts.ptr_level == 0 && res_ts.array_rank == 0 &&
        res_ts.tag && res_ts.tag[0]) {
      const auto it = mod_.struct_defs.find(res_ts.tag);
      if (it != mod_.struct_defs.end()) {
        const int payload_sz = compute_struct_size(mod_, res_ts.tag);
        if (payload_sz == 0) return "zeroinitializer";
        if (payload_sz > 0) {
          if (payload_sz > 16) {
            const std::string slot_ptr = emit_aarch64_vaarg_gp_src_ptr(ctx, ap_ptr, 8);
            const std::string src_ptr = fresh_tmp(ctx);
            emit_instr(ctx, src_ptr + " = load ptr, ptr " + slot_ptr);
            const std::string tmp_addr = fresh_tmp(ctx);
            emit_instr(ctx, tmp_addr + " = alloca " + res_ty);
            need_llvm_memcpy_ = true;
            emit_instr(ctx, "call void @llvm.memcpy.p0.p0.i64(ptr " + tmp_addr + ", ptr " +
                                src_ptr + ", i64 " + std::to_string(payload_sz) + ", i1 false)");
            const std::string out = fresh_tmp(ctx);
            emit_instr(ctx, out + " = load " + res_ty + ", ptr " + tmp_addr);
            return out;
          }

          const int slot_bytes = payload_sz > 8 ? 16 : 8;
          const std::string src_ptr = emit_aarch64_vaarg_gp_src_ptr(ctx, ap_ptr, slot_bytes);
          const std::string tmp_addr = fresh_tmp(ctx);
          emit_instr(ctx, tmp_addr + " = alloca " + res_ty);
          need_llvm_memcpy_ = true;
          emit_instr(ctx, "call void @llvm.memcpy.p0.p0.i64(ptr " + tmp_addr + ", ptr " +
                              src_ptr + ", i64 " + std::to_string(payload_sz) + ", i1 false)");
          const std::string out = fresh_tmp(ctx);
          emit_instr(ctx, out + " = load " + res_ty + ", ptr " + tmp_addr);
          return out;
        }
      }
    }

    const bool is_gp_scalar =
        (res_ty == "ptr") ||
        (res_ts.ptr_level == 0 && res_ts.array_rank == 0 && is_any_int(res_ts.base));
    const bool is_fp_scalar =
        res_ts.ptr_level == 0 && res_ts.array_rank == 0 &&
        (res_ts.base == TB_FLOAT || res_ts.base == TB_DOUBLE);
    const bool is_fp128_scalar =
        res_ts.ptr_level == 0 && res_ts.array_rank == 0 && res_ts.base == TB_LONGDOUBLE;

    // AArch64 va_list matches clang's __va_list_tag layout:
    // { stack, gr_top, vr_top, gr_offs, vr_offs }.
    // For integer/pointer args we first try GR save area, then fall back to stack.
    if (is_gp_scalar) {
      const std::string src_ptr = emit_aarch64_vaarg_gp_src_ptr(ctx, ap_ptr, 8);
      const std::string out = fresh_tmp(ctx);
      emit_instr(ctx, out + " = load " + res_ty + ", ptr " + src_ptr);
      return out;
    }
    if (is_fp_scalar) {
      const std::string src_ptr = emit_aarch64_vaarg_fp_src_ptr(ctx, ap_ptr, 16, 8, 8);
      const std::string out = fresh_tmp(ctx);
      emit_instr(ctx, out + " = load " + res_ty + ", ptr " + src_ptr);
      return out;
    }
    if (is_fp128_scalar) {
      const std::string src_ptr = emit_aarch64_vaarg_fp_src_ptr(ctx, ap_ptr, 16, 16, 16);
      const std::string out = fresh_tmp(ctx);
      emit_instr(ctx, out + " = load " + res_ty + ", ptr " + src_ptr);
      return out;
    }

    const std::string out = fresh_tmp(ctx);
    emit_instr(ctx, out + " = va_arg ptr " + ap_ptr + ", " + res_ty);
    return out;
  }

  // ── Ternary ───────────────────────────────────────────────────────────────

  std::string emit_rval_payload(FnCtx& ctx, const TernaryExpr& t, const Expr& e) {
    TypeSpec cond_ts{};
    const std::string cond_v = emit_rval_id(ctx, t.cond, cond_ts);
    const std::string cond_i1 = to_bool(ctx, cond_v, cond_ts);

    const std::string then_lbl = fresh_lbl(ctx, "tern.then.");
    const std::string else_lbl = fresh_lbl(ctx, "tern.else.");
    const std::string end_lbl  = fresh_lbl(ctx, "tern.end.");
    TypeSpec res_spec = resolve_expr_type(ctx, e);
    if (!has_concrete_type(res_spec)) res_spec.base = TB_INT;
    const std::string res_ty = llvm_ty(res_spec);

    emit_term(ctx, "br i1 " + cond_i1 + ", label %" + then_lbl + ", label %" + else_lbl);

    emit_lbl(ctx, then_lbl);
    TypeSpec then_ts{};
    std::string then_v = emit_rval_id(ctx, t.then_expr, then_ts);
    then_v = coerce(ctx, then_v, then_ts, res_spec);
    emit_term(ctx, "br label %" + end_lbl);

    emit_lbl(ctx, else_lbl);
    TypeSpec else_ts{};
    std::string else_v = emit_rval_id(ctx, t.else_expr, else_ts);
    else_v = coerce(ctx, else_v, else_ts, res_spec);
    emit_term(ctx, "br label %" + end_lbl);

    emit_lbl(ctx, end_lbl);
    if (res_ty == "void") return "";
    // If either arm produced a void/empty value (e.g. void function call in one arm),
    // substitute a zero value to avoid an invalid empty phi operand.
    auto void_to_zero = [&](const std::string& v) -> std::string {
      if (!v.empty()) return v;
      if (res_ty == "ptr") return "null";
      if (res_ty == "float" || res_ty == "double") return "0.0";
      return "0";
    };
    const std::string tmp = fresh_tmp(ctx);
    emit_instr(ctx, tmp + " = phi " + res_ty +
                        " [ " + void_to_zero(then_v) + ", %" + then_lbl + " ]," +
                        " [ " + void_to_zero(else_v) + ", %" + else_lbl + " ]");
    return tmp;
  }

  // ── Sizeof ────────────────────────────────────────────────────────────────

  std::string emit_rval_payload(FnCtx& ctx, const SizeofExpr& s, const Expr&) {
    const Expr& op = get_expr(s.expr);
    TypeSpec op_ts = std::visit([&](const auto& p) -> TypeSpec {
      return resolve_payload_type(ctx, p);
    }, op.payload);
    if (!has_concrete_type(op_ts)) op_ts = resolve_expr_type(ctx, s.expr);
    if (!has_concrete_type(op_ts)) op_ts = op.type.spec;
    // DeclRef: get type from global/local declaration (NK_VAR nodes have no type set).
    if (const auto* r = std::get_if<DeclRef>(&op.payload)) {
      auto resolve_named_global_object_type = [&](const std::string& name) -> std::optional<TypeSpec> {
        const GlobalVar* best = nullptr;
        for (const auto& g : mod_.globals) {
          if (g.name != name) continue;
          if (!best) {
            best = &g;
            continue;
          }
          const TypeSpec best_ts = resolve_array_ts(best->type.spec, best->init);
          const TypeSpec cand_ts = resolve_array_ts(g.type.spec, g.init);
          if (cand_ts.array_rank > 0 && best_ts.array_rank <= 0) {
            best = &g;
          } else if (cand_ts.array_rank > 0 && best_ts.array_rank > 0 &&
                     cand_ts.array_size > best_ts.array_size) {
            best = &g;
          } else if (cand_ts.array_rank <= 0 && best_ts.array_rank <= 0 &&
                     g.init.index() != 0 && best->init.index() == 0) {
            best = &g;
          }
        }
        if (!best) return std::nullopt;
        return resolve_array_ts(best->type.spec, best->init);
      };
      if (r->global) {
        if (const GlobalVar* gv0 = mod_.find_global(*r->global)) {
          const GlobalVar* best = gv0;
          if (gv0->type.spec.array_rank > 0 && gv0->type.spec.array_size < 0) {
            for (const auto& g : mod_.globals) {
              if (g.name != gv0->name || g.type.spec.array_rank <= 0) continue;
              const TypeSpec rs = resolve_array_ts(g.type.spec, g.init);
              if (rs.array_size > 0) { best = &g; break; }
            }
          }
          op_ts = resolve_array_ts(best->type.spec, best->init);
        }
      } else if (r->local) {
        const auto it = ctx.local_types.find(r->local->value);
        if (it != ctx.local_types.end()) op_ts = it->second;
      } else if (r->param_index && ctx.fn &&
                 *r->param_index < ctx.fn->params.size()) {
        op_ts = ctx.fn->params[*r->param_index].type.spec;
        // Array params decay to pointer in C — sizeof(param) = sizeof(void*)
        if (op_ts.array_rank > 0) {
          op_ts.array_rank = 0; op_ts.array_size = -1; op_ts.ptr_level = 1;
        }
      } else if (!r->name.empty()) {
        if (auto named_ts = resolve_named_global_object_type(r->name)) {
          op_ts = *named_ts;
        }
      }
      if (op_ts.array_rank == 0 && !r->name.empty()) {
        if (auto named_ts = resolve_named_global_object_type(r->name)) {
          op_ts = *named_ts;
        }
      }
    }
    if (!has_concrete_type(op_ts)) return "8";
    return std::to_string(sizeof_ts(mod_, op_ts));
  }

  std::string emit_rval_payload(FnCtx& ctx, const SizeofTypeExpr& s, const Expr&) {
    return std::to_string(sizeof_ts(mod_, s.type.spec));
  }

  // ── LabelAddrExpr (GCC &&label extension) ────────────────────────────────

  std::string emit_rval_payload(FnCtx& ctx, const LabelAddrExpr& la, const Expr&) {
    return "blockaddress(@" + ctx.fn->name + ", %ulbl_" + la.label_name + ")";
  }

  // ── IndexExpr (rval: load through computed ptr) ──────────────────────────

  std::string emit_rval_payload(FnCtx& ctx, const IndexExpr&, const Expr& e) {
    TypeSpec pts{};
    const std::string ptr = emit_lval_dispatch(ctx, e, pts);
    // Array element decay: if the element type is itself an array, return its
    // address directly (C array-to-pointer decay) rather than loading it.
    if (pts.array_rank > 0 && pts.ptr_level == 0) return ptr;
    TypeSpec load_ts = resolve_expr_type(ctx, e);
    if (!has_concrete_type(load_ts)) load_ts = pts;
    const std::string ty = llvm_ty(load_ts);
    const std::string tmp = fresh_tmp(ctx);
    emit_instr(ctx, tmp + " = load " + ty + ", ptr " + ptr);
    return tmp;
  }

  // ── MemberExpr ────────────────────────────────────────────────────────────

  std::string emit_rval_payload(FnCtx& ctx, const MemberExpr& m, const Expr& e) {
    TypeSpec field_ts{};
    BitfieldAccess bf;
    const std::string gep = emit_member_lval(ctx, m, field_ts, &bf);
    // Bitfield: use shift/mask load
    if (bf.is_bitfield()) {
      return emit_bitfield_load(ctx, gep, bf);
    }
    // Use stored type if available, else use resolved field type
    const TypeSpec& load_ts = (e.type.spec.base != TB_VOID || e.type.spec.ptr_level > 0)
                                  ? e.type.spec
                                  : field_ts;
    // Array fields: decay to pointer-to-first-element (no load needed)
    if (load_ts.array_rank > 0 || field_ts.array_rank > 0) {
      const TypeSpec& arr_ts = (field_ts.array_rank > 0) ? field_ts : load_ts;
      const std::string arr_alloca_ty = llvm_alloca_ty(arr_ts);
      if (arr_alloca_ty == "ptr") {
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = load ptr, ptr " + gep);
        return tmp;
      }
      const std::string tmp = fresh_tmp(ctx);
      emit_instr(ctx, tmp + " = getelementptr " + arr_alloca_ty +
                          ", ptr " + gep + ", i64 0, i64 0");
      return tmp;
    }
    const std::string ty = llvm_ty(load_ts);
    if (ty == "void") return "";
    const std::string tmp = fresh_tmp(ctx);
    emit_instr(ctx, tmp + " = load " + ty + ", ptr " + gep);
    return tmp;
  }

  // ── Statement emission ────────────────────────────────────────────────────

  void emit_stmt(FnCtx& ctx, const Stmt& stmt) {
    std::visit([&](const auto& s) { emit_stmt_impl(ctx, s); }, stmt.payload);
  }

  void emit_stmt_impl(FnCtx& ctx, const LocalDecl& d) {
    if (d.fn_ptr_sig) {
      ctx.local_fn_ptr_sigs[d.id.value] = *d.fn_ptr_sig;
      ctx.local_fn_ret_types[d.id.value] = d.fn_ptr_sig->return_type.spec;
    }
    if (d.vla_size) {
      const std::string slot = ctx.local_slots.at(d.id.value);
      TypeSpec sz_ts{};
      std::string count = emit_rval_id(ctx, *d.vla_size, sz_ts);
      TypeSpec i64_ts{};
      i64_ts.base = TB_LONGLONG;
      count = coerce(ctx, count, sz_ts, i64_ts);

      long long static_mult = 1;
      for (int i = 1; i < d.type.spec.array_rank; ++i) {
        const long long dim = d.type.spec.array_dims[i];
        if (dim > 0) static_mult *= dim;
      }
      if (static_mult > 1) {
        const std::string scaled = fresh_tmp(ctx);
        emit_instr(ctx, scaled + " = mul i64 " + count + ", " + std::to_string(static_mult));
        count = scaled;
      }

      TypeSpec elem_ts = d.type.spec;
      elem_ts.array_rank = 0;
      elem_ts.array_size = -1;
      for (int i = 0; i < 8; ++i) elem_ts.array_dims[i] = -1;
      std::string elem_ty = llvm_ty(elem_ts);
      if (elem_ty == "void") elem_ty = "i8";
      const std::string dyn_ptr = fresh_tmp(ctx);
      emit_instr(ctx, dyn_ptr + " = alloca " + elem_ty + ", i64 " + count);
      emit_instr(ctx, "store ptr " + dyn_ptr + ", ptr " + slot);
    }

    if (!d.init) return;
    const std::string slot = ctx.local_slots.at(d.id.value);
    // If this is a fn-ptr local initialized from a known function, track the
    // function's return type so nested fn-ptr calls resolve correctly.
    if (d.type.spec.is_fn_ptr) {
      const Expr& init_e = get_expr(*d.init);
      if (const auto* dr = std::get_if<DeclRef>(&init_e.payload)) {
        const auto fit = mod_.fn_index.find(dr->name);
        if (fit != mod_.fn_index.end() && fit->second.value < mod_.functions.size()) {
          ctx.local_fn_ret_types[d.id.value] =
              mod_.functions[fit->second.value].return_type.spec;
        }
      }
    }
    TypeSpec rhs_ts{};
    std::string rhs = emit_rval_id(ctx, *d.init, rhs_ts);
    const std::string ty =
        (d.type.spec.array_rank > 0) ? llvm_alloca_ty(d.type.spec) : llvm_ty(d.type.spec);
    const bool is_agg_or_array =
        d.type.spec.array_rank > 0 ||
        (d.type.spec.ptr_level == 0 &&
         (d.type.spec.base == TB_STRUCT || d.type.spec.base == TB_UNION) &&
         d.type.spec.array_rank == 0);
    if (is_agg_or_array && (rhs == "0" || rhs.empty())) {
      emit_instr(ctx, "store " + ty + " zeroinitializer, ptr " + slot);
      return;
    }
    rhs = coerce(ctx, rhs, rhs_ts, d.type.spec);
    emit_instr(ctx, "store " + ty + " " + rhs + ", ptr " + slot);
  }

  void emit_stmt_impl(FnCtx& ctx, const ExprStmt& s) {
    if (!s.expr) return;
    TypeSpec ts{};
    emit_rval_id(ctx, *s.expr, ts);
  }

  void emit_stmt_impl(FnCtx& ctx, const ReturnStmt& s) {
    if (!s.expr) { emit_term(ctx, "ret void"); return; }
    TypeSpec ts{};
    std::string val = emit_rval_id(ctx, *s.expr, ts);
    val = coerce(ctx, val, ts, ctx.fn->return_type.spec);
    emit_term(ctx, "ret " + llvm_ty(ctx.fn->return_type.spec) + " " + val);
  }

  void emit_stmt_impl(FnCtx& ctx, const IfStmt& s) {
    TypeSpec cond_ts{};
    const std::string cond_v  = emit_rval_id(ctx, s.cond, cond_ts);
    const std::string cond_i1 = to_bool(ctx, cond_v, cond_ts);
    const std::string then_lbl = block_lbl(s.then_block);
    const std::string after_lbl = block_lbl(s.after_block);
    if (s.else_block) {
      emit_term(ctx, "br i1 " + cond_i1 + ", label %" + then_lbl +
                         ", label %" + block_lbl(*s.else_block));
    } else {
      emit_term(ctx, "br i1 " + cond_i1 + ", label %" + then_lbl +
                         ", label %" + after_lbl);
    }
    // Store after_block in meta for then/else blocks (so they fall through)
    ctx.block_meta[s.then_block.value].break_label = std::nullopt;  // not a loop
    if (s.else_block)
      ctx.block_meta[s.else_block->value].break_label = std::nullopt;
    // after_block label is needed as fallthrough for then/else
    // We inject it into the meta as "after" by convention (handled in emit_function)
    (void)after_lbl;
  }

  void emit_stmt_impl(FnCtx& ctx, const WhileStmt& s) {
    const std::string cond_lbl = s.continue_target
        ? block_lbl(*s.continue_target)
        : fresh_lbl(ctx, "while.cond.");
    const std::string body_lbl = block_lbl(s.body_block);
    const std::string end_lbl  = s.break_target
        ? block_lbl(*s.break_target)
        : fresh_lbl(ctx, "while.end.");

    ctx.continue_redirect[s.body_block.value] = cond_lbl;

    TypeSpec cond_ts{};
    const std::string cond_v = emit_rval_id(ctx, s.cond, cond_ts);
    const std::string cond_i1 = to_bool(ctx, cond_v, cond_ts);
    emit_term(ctx, "br i1 " + cond_i1 + ", label %" + body_lbl + ", label %" + end_lbl);
  }

  void emit_stmt_impl(FnCtx& ctx, const ForStmt& s) {
    if (s.init) {
      TypeSpec ts{};
      emit_rval_id(ctx, *s.init, ts);
    }
    const std::string body_lbl = block_lbl(s.body_block);
    const std::string end_lbl  = s.break_target
        ? block_lbl(*s.break_target)
        : fresh_lbl(ctx, "for.end.");
    const std::string cond_lbl = "for.cond." + std::to_string(s.body_block.value);
    const std::string latch_lbl = "for.latch." + std::to_string(s.body_block.value);
    ctx.continue_redirect[s.body_block.value] = latch_lbl;

    emit_term(ctx, "br label %" + cond_lbl);
    emit_lbl(ctx, cond_lbl);
    if (s.cond) {
      TypeSpec cts{};
      std::string cv = emit_rval_id(ctx, *s.cond, cts);
      cv = to_bool(ctx, cv, cts);
      emit_term(ctx, "br i1 " + cv + ", label %" + body_lbl + ", label %" + end_lbl);
    } else {
      emit_term(ctx, "br label %" + body_lbl);
    }
    emit_lbl(ctx, latch_lbl);
    if (s.update) {
      TypeSpec uts{};
      (void)emit_rval_id(ctx, *s.update, uts);
    }
    emit_term(ctx, "br label %" + cond_lbl);
  }

  void emit_stmt_impl(FnCtx& ctx, const DoWhileStmt& s) {
    const std::string body_lbl = block_lbl(s.body_block);
    const std::string end_lbl  = s.break_target
        ? block_lbl(*s.break_target)
        : fresh_lbl(ctx, "dowhile.end.");
    const std::string cond_lbl = "dowhile.cond." + std::to_string(s.body_block.value);
    ctx.continue_redirect[s.body_block.value] = cond_lbl;
    emit_term(ctx, "br label %" + cond_lbl);
    emit_lbl(ctx, cond_lbl);
    TypeSpec cond_ts{};
    const std::string cond_v = emit_rval_id(ctx, s.cond, cond_ts);
    const std::string cond_i1 = to_bool(ctx, cond_v, cond_ts);
    emit_term(ctx, "br i1 " + cond_i1 + ", label %" + body_lbl + ", label %" + end_lbl);
  }

  void emit_stmt_impl(FnCtx& ctx, const SwitchStmt& s) {
    TypeSpec ts{};
    std::string val = emit_rval_id(ctx, s.cond, ts);
    // C requires integer promotion on the controlling expression.
    if (ts.ptr_level == 0 && ts.array_rank == 0 && is_any_int(ts.base)) {
      TypeBase promoted = integer_promote(ts.base);
      if (promoted != ts.base) {
        TypeSpec promoted_ts{}; promoted_ts.base = promoted;
        val = coerce(ctx, val, ts, promoted_ts);
        ts = promoted_ts;
      }
    }
    const std::string ty  = llvm_ty(ts);

    // Default label: use explicit default block if present, else the break (after-switch) block
    const std::string default_lbl = s.default_block
        ? block_lbl(*s.default_block)
        : (s.break_block ? block_lbl(*s.break_block) : fresh_lbl(ctx, "sw.end."));
    const std::string break_lbl = s.break_block
        ? block_lbl(*s.break_block)
        : default_lbl;

    ctx.block_meta[s.body_block.value].break_label = break_lbl;

    // Collect case values by scanning body block statements
    // All cases jump to body_block (they share the body, case markers are sequential)
    const Block* body_blk = nullptr;
    for (const auto& blk : ctx.fn->blocks) {
      if (blk.id.value == s.body_block.value) { body_blk = &blk; break; }
    }

    const std::string body_lbl = block_lbl(s.body_block);
    std::string sw = "switch " + ty + " " + val + ", label %" + default_lbl + " [\n";
    if (!s.case_blocks.empty()) {
      // Use pre-collected case→block mappings (supports Duff's device / interleaved cases).
      for (const auto& [case_val, case_bid] : s.case_blocks) {
        sw += "    " + ty + " " + std::to_string(case_val) +
              ", label %" + block_lbl(case_bid) + "\n";
      }
    } else if (body_blk) {
      // Fallback: scan body block for inline CaseStmt markers (all map to body start).
      for (const auto& stmt : body_blk->stmts) {
        if (const auto* cs = std::get_if<CaseStmt>(&stmt.payload)) {
          sw += "    " + ty + " " + std::to_string(cs->value) +
                ", label %" + body_lbl + "\n";
        } else if (std::get_if<CaseRangeStmt>(&stmt.payload)) {
          // GCC range extension: skip for now
        }
      }
    }
    sw += "  ]";
    emit_term(ctx, sw);
  }

  void emit_stmt_impl(FnCtx& ctx, const GotoStmt& s) {
    if (ctx.vla_stack_save_ptr && s.target.resolved_block.valid() &&
        s.target.resolved_block.value <= ctx.current_block_id) {
      need_llvm_stackrestore_ = true;
      emit_instr(ctx, "call void @llvm.stackrestore(ptr " + *ctx.vla_stack_save_ptr + ")");
    }
    if (s.target.resolved_block.valid()) {
      emit_term(ctx, "br label %" + block_lbl(s.target.resolved_block));
    } else {
      emit_term(ctx, "br label %ulbl_" + s.target.user_name);
    }
  }

  void emit_stmt_impl(FnCtx& ctx, const IndirBrStmt& s) {
    // Collect all user labels in this function as possible targets.
    std::vector<std::string> targets;
    for (const auto& bb : ctx.fn->blocks) {
      for (const auto& stmt : bb.stmts) {
        if (const auto* ls = std::get_if<LabelStmt>(&stmt.payload)) {
          targets.push_back("%ulbl_" + ls->name);
        }
      }
    }
    TypeSpec dummy_ts{};
    const std::string val = emit_rval_id(ctx, s.target, dummy_ts);
    std::string tgt_list;
    for (const auto& t : targets) {
      if (!tgt_list.empty()) tgt_list += ", ";
      tgt_list += "label " + t;
    }
    emit_term(ctx, "indirectbr ptr " + val + ", [" + tgt_list + "]");
  }

  void emit_stmt_impl(FnCtx& ctx, const LabelStmt& s) {
    if (ctx.last_term) {
      // We need a new basic block for the label
      emit_lbl(ctx, "ulbl_" + s.name);
    } else {
      emit_term(ctx, "br label %ulbl_" + s.name);
      emit_lbl(ctx, "ulbl_" + s.name);
    }
  }

  void emit_stmt_impl(FnCtx& ctx, const BreakStmt& s) {
    if (s.target) emit_term(ctx, "br label %" + block_lbl(*s.target));
  }

  void emit_stmt_impl(FnCtx& ctx, const ContinueStmt& s) {
    if (!s.target) return;
    const auto it = ctx.continue_redirect.find(s.target->value);
    if (it != ctx.continue_redirect.end()) {
      emit_term(ctx, "br label %" + it->second);
      return;
    }
    emit_term(ctx, "br label %" + block_lbl(*s.target));
  }

  void emit_stmt_impl(FnCtx&, const CaseStmt&)      {}
  void emit_stmt_impl(FnCtx&, const CaseRangeStmt&) {}
  void emit_stmt_impl(FnCtx&, const DefaultStmt&)   {}

  // ── Function emission ─────────────────────────────────────────────────────

  // Returns set of param indices that are used as lvalues (modified or address-taken).
  std::unordered_set<uint32_t> find_modified_params(const Function& fn) {
    std::unordered_set<uint32_t> result;
    // Recursively check all expressions in the function body.
    std::function<void(ExprId)> scan = [&](ExprId id) {
      const Expr& e = get_expr(id);
      std::visit([&](const auto& p) {
        using T = std::decay_t<decltype(p)>;
        if constexpr (std::is_same_v<T, UnaryExpr>) {
          if (p.op == UnaryOp::PreInc || p.op == UnaryOp::PostInc ||
              p.op == UnaryOp::PreDec || p.op == UnaryOp::PostDec ||
              p.op == UnaryOp::AddrOf) {
            // If operand is a param, mark it
            const Expr& op_e = get_expr(p.operand);
            if (const auto* r = std::get_if<DeclRef>(&op_e.payload)) {
              if (r->param_index) result.insert(*r->param_index);
            }
          }
          scan(p.operand);
        } else if constexpr (std::is_same_v<T, AssignExpr>) {
          // lhs assignment
          const Expr& lhs_e = get_expr(p.lhs);
          if (const auto* r = std::get_if<DeclRef>(&lhs_e.payload)) {
            if (r->param_index) result.insert(*r->param_index);
          }
          scan(p.lhs); scan(p.rhs);
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
          scan(p.lhs); scan(p.rhs);
        } else if constexpr (std::is_same_v<T, CallExpr>) {
          scan(p.callee);
          for (const auto& a : p.args) scan(a);
        } else if constexpr (std::is_same_v<T, TernaryExpr>) {
          scan(p.cond); scan(p.then_expr); scan(p.else_expr);
        } else if constexpr (std::is_same_v<T, IndexExpr>) {
          scan(p.base); scan(p.index);
        } else if constexpr (std::is_same_v<T, MemberExpr>) {
          scan(p.base);
        } else if constexpr (std::is_same_v<T, CastExpr>) {
          scan(p.expr);
        } else if constexpr (std::is_same_v<T, SizeofExpr>) {
          scan(p.expr);
        }
      }, e.payload);
    };
    for (const auto& blk : fn.blocks) {
      for (const auto& stmt : blk.stmts) {
        std::visit([&](const auto& s) {
          using S = std::decay_t<decltype(s)>;
          if constexpr (std::is_same_v<S, LocalDecl>) {
            if (s.init) scan(*s.init);
          } else if constexpr (std::is_same_v<S, ExprStmt>) {
            if (s.expr) scan(*s.expr);
          } else if constexpr (std::is_same_v<S, ReturnStmt>) {
            if (s.expr) scan(*s.expr);
          } else if constexpr (std::is_same_v<S, IfStmt>) {
            scan(s.cond);
          } else if constexpr (std::is_same_v<S, WhileStmt>) {
            scan(s.cond);
          } else if constexpr (std::is_same_v<S, ForStmt>) {
            if (s.init) scan(*s.init);
            if (s.cond) scan(*s.cond);
            if (s.update) scan(*s.update);
          } else if constexpr (std::is_same_v<S, SwitchStmt>) {
            scan(s.cond);
          }
        }, stmt.payload);
      }
    }
    return result;
  }

  static bool fn_has_vla_locals(const Function& fn) {
    for (const auto& blk : fn.blocks) {
      for (const auto& stmt : blk.stmts) {
        if (const auto* d = std::get_if<LocalDecl>(&stmt.payload)) {
          if (d->vla_size.has_value()) return true;
        }
      }
    }
    return false;
  }

  void hoist_allocas(FnCtx& ctx, const Function& fn) {
    // Identify parameters that are modified (need spill slots)
    const std::unordered_set<uint32_t> modified_params = find_modified_params(fn);
    for (size_t i = 0; i < fn.params.size(); ++i) {
      if (!modified_params.count(static_cast<uint32_t>(i))) continue;
      const auto& param = fn.params[i];
      const std::string slot = "%lv.param." + sanitize_llvm_ident(param.name);
      const std::string pname = "%p." + sanitize_llvm_ident(param.name);
      // Store the spill slot under key (param_index + sentinel)
      ctx.param_slots[static_cast<uint32_t>(i) + 0x80000000u] = slot;
      ctx.alloca_lines.push_back("  " + slot + " = alloca " + llvm_alloca_ty(param.type.spec));
      ctx.alloca_lines.push_back("  store " + llvm_ty(param.type.spec) + " " + pname + ", ptr " + slot);
    }

    std::unordered_map<std::string, int> name_count;
    for (const auto& blk : fn.blocks) {
      for (const auto& stmt : blk.stmts) {
        const auto* d = std::get_if<LocalDecl>(&stmt.payload);
        if (!d) continue;
        if (ctx.local_slots.count(d->id.value)) continue;
        // Dedup slot names for shadowing locals
        const int cnt = name_count[d->name]++;
        const std::string base = sanitize_llvm_ident(d->name);
        const std::string slot = cnt == 0
            ? "%lv." + base
            : "%lv." + base + "." + std::to_string(cnt);
        ctx.local_slots[d->id.value] = slot;
        ctx.local_types[d->id.value] = d->type.spec;
        ctx.local_is_vla[d->id.value] = d->vla_size.has_value();
        if (d->vla_size) {
          // VLA locals are allocated dynamically at declaration time.
          // Keep a pointer slot in entry and store the runtime alloca address into it.
          ctx.alloca_lines.push_back("  " + slot + " = alloca ptr");
        } else {
          const std::string alloca_ty = llvm_alloca_ty(d->type.spec);
          ctx.alloca_lines.push_back("  " + slot + " = alloca " + alloca_ty);
          if (d->init &&
              (d->type.spec.array_rank > 0 ||
               (d->type.spec.ptr_level == 0 &&
                (d->type.spec.base == TB_STRUCT ||
                 d->type.spec.base == TB_UNION)))) {
            ctx.alloca_lines.push_back("  store " + alloca_ty + " zeroinitializer, ptr " + slot);
          }
        }
      }
    }
  }

  void emit_function(const Function& fn) {
    FnCtx ctx;
    ctx.fn = &fn;
    for (size_t i = 0; i < fn.params.size(); ++i) {
      if (fn.params[i].fn_ptr_sig) {
        ctx.param_fn_ptr_sigs[static_cast<uint32_t>(i)] = *fn.params[i].fn_ptr_sig;
      }
    }
    for (const auto& g : mod_.globals) {
      if (g.fn_ptr_sig) ctx.global_fn_ptr_sigs[g.id.value] = *g.fn_ptr_sig;
    }

    std::ostringstream out;
    const std::string ret_ty = llvm_ty(fn.return_type.spec);

    const bool void_param_list =
        fn.params.size() == 1 &&
        fn.params[0].type.spec.base == TB_VOID &&
        fn.params[0].type.spec.ptr_level == 0 &&
        fn.params[0].type.spec.array_rank == 0;

    if (fn.linkage.is_extern && fn.blocks.empty()) {
      out << "declare " << ret_ty << " @" << fn.name << "(";
      for (size_t i = 0; i < fn.params.size(); ++i) {
        if (void_param_list) break;
        if (i) out << ", ";
        out << llvm_ty(fn.params[i].type.spec);
      }
      if (fn.attrs.variadic) {
        if (!fn.params.empty() && !void_param_list) out << ", ";
        out << "...";
      }
      out << ")\n\n";
      fn_bodies_.push_back(out.str());
      return;
    }

    // Signature
    out << "define " << ret_ty << " @" << fn.name << "(";
    for (size_t i = 0; i < fn.params.size(); ++i) {
      if (void_param_list) break;
      if (i) out << ", ";
      const std::string pty = llvm_ty(fn.params[i].type.spec);
      const std::string pname = "%p." + sanitize_llvm_ident(fn.params[i].name);
      out << pty << " " << pname;
      ctx.param_slots[static_cast<uint32_t>(i)] = pname;
    }
    if (fn.attrs.variadic) {
      if (!fn.params.empty()) out << ", ";
      out << "...";
    }
    out << ")\n";
    out << "{\nentry:\n";

    // Alloca hoisting
    hoist_allocas(ctx, fn);
    if (fn_has_vla_locals(fn)) {
      need_llvm_stacksave_ = true;
      const std::string saved_sp = fresh_tmp(ctx);
      emit_instr(ctx, saved_sp + " = call ptr @llvm.stacksave()");
      ctx.vla_stack_save_ptr = saved_sp;
    }

    // Also alloca params if they're assigned to (for address-takeable params)
    // We don't do this here; params are SSA values directly.

    // Emit blocks in order (entry first, then rest)
    const Block* entry_blk = nullptr;
    std::vector<const Block*> ordered;
    for (const auto& blk : fn.blocks) {
      if (blk.id.value == fn.entry.value) { entry_blk = &blk; }
    }
    if (entry_blk) ordered.push_back(entry_blk);
    for (const auto& blk : fn.blocks) {
      if (blk.id.value != fn.entry.value) ordered.push_back(&blk);
    }

    // Pre-scan to build block_meta from WhileStmt/ForStmt/DoWhileStmt
    // (This happens during emit_stmt calls, which populate ctx.block_meta)
    // But we emit stmts sequentially, so meta is built as we go.

    for (size_t bi = 0; bi < ordered.size(); ++bi) {
      const Block* blk = ordered[bi];
      ctx.current_block_id = blk->id.value;

      // Emit block label (except for entry which uses "entry:")
      if (bi > 0) {
        emit_lbl(ctx, block_lbl(blk->id));
      }

      // Emit statements
      for (const auto& stmt : blk->stmts) {
        emit_stmt(ctx, stmt);
      }

      // HIR lowering must encode CFG edges explicitly.
      // Do not auto-fallthrough into the next emitted block, otherwise
      // unrelated blocks can be accidentally connected and create loops.
      if (!ctx.last_term) {
        if (fn.return_type.spec.base == TB_VOID &&
            fn.return_type.spec.ptr_level == 0 &&
            fn.return_type.spec.array_rank == 0) {
          emit_term(ctx, "ret void");
        } else if (ret_ty == "ptr") {
          emit_term(ctx, "ret ptr null");
        } else if (is_float_base(fn.return_type.spec.base) &&
                   fn.return_type.spec.ptr_level == 0 && fn.return_type.spec.array_rank == 0) {
          // Float/double: zero must be expressed as a float constant, not integer 0
          const std::string zero_val = fp_literal(fn.return_type.spec.base, 0.0);
          emit_term(ctx, "ret " + ret_ty + " " + zero_val);
        } else if (is_complex_base(fn.return_type.spec.base) ||
                   ((fn.return_type.spec.base == TB_STRUCT || fn.return_type.spec.base == TB_UNION) &&
                    fn.return_type.spec.ptr_level == 0 && fn.return_type.spec.array_rank == 0)) {
          emit_term(ctx, "ret " + ret_ty + " zeroinitializer");
        } else {
          emit_term(ctx, "ret " + ret_ty + " 0");
        }
      }
    }

    // Assemble
    for (const auto& l : ctx.alloca_lines) out << l << "\n";
    for (const auto& l : ctx.body_lines)   out << l << "\n";
    out << "}\n\n";
    fn_bodies_.push_back(out.str());
  }
};

}  // namespace

std::string emit_module_native(const Module& mod) {
  HirEmitter emitter(mod);
  return emitter.emit();
}

}  // namespace tinyc2ll::codegen::llvm_backend
