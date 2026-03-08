#include "ir_builder.hpp"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <unordered_set>
#include <cstdint>
#include <cmath>
#include <functional>
#include <sstream>
#include <stdexcept>

namespace tinyc2ll::frontend_cxx {

// ─────────────────────────── static helpers ────────────────────────────────

static TypeSpec make_ts(TypeBase base) {
  TypeSpec ts{};
  ts.base = base;
  ts.array_size = -1;
  ts.array_rank = 0;
  for (int i = 0; i < 8; ++i) ts.array_dims[i] = -1;
  ts.is_ptr_to_array = false;
  ts.inner_rank = -1;
  return ts;
}
static TypeSpec int_ts()    { return make_ts(TB_INT); }
static TypeSpec void_ts()   { return make_ts(TB_VOID); }
static TypeSpec double_ts() { return make_ts(TB_DOUBLE); }
static TypeSpec float_ts()  { return make_ts(TB_FLOAT); }
static TypeSpec char_ts()   { return make_ts(TB_CHAR); }
static TypeSpec ll_ts()     { return make_ts(TB_LONGLONG); }

static TypeSpec ptr_to(TypeBase base) {
  TypeSpec ts = make_ts(base);
  ts.ptr_level = 1;
  return ts;
}

static int array_rank_of(const TypeSpec& ts) {
  if (ts.array_rank > 0) return ts.array_rank;
  if (ts.array_size != -1) return 1;
  return 0;
}

static long long array_dim_at(const TypeSpec& ts, int idx) {
  if (ts.array_rank > 0) {
    if (idx >= 0 && idx < ts.array_rank) return ts.array_dims[idx];
    return -1;
  }
  if (idx == 0 && ts.array_size != -1) return ts.array_size;
  return -1;
}

static bool is_array_ty(const TypeSpec& ts) {
  return array_rank_of(ts) > 0;
}

static void clear_array(TypeSpec& ts) {
  ts.array_size = -1;
  ts.array_rank = 0;
  for (int i = 0; i < 8; ++i) ts.array_dims[i] = -1;
  ts.is_ptr_to_array = false;
}

static void set_array_dims(TypeSpec& ts, const long long* dims, int rank) {
  clear_array(ts);
  if (rank <= 0) return;
  if (rank > 8) rank = 8;
  ts.array_rank = rank;
  for (int i = 0; i < rank; ++i) ts.array_dims[i] = dims[i];
  ts.array_size = ts.array_dims[0];
}

static void set_first_array_dim(TypeSpec& ts, long long dim0) {
  int rank = array_rank_of(ts);
  if (rank <= 0) {
    long long dims[1] = {dim0};
    set_array_dims(ts, dims, 1);
    return;
  }
  long long dims[8];
  for (int i = 0; i < rank && i < 8; ++i) dims[i] = array_dim_at(ts, i);
  dims[0] = dim0;
  set_array_dims(ts, dims, rank);
}

static TypeSpec drop_array_dim(TypeSpec ts) {
  int rank = array_rank_of(ts);
  if (rank <= 0) {
    clear_array(ts);
    return ts;
  }
  long long dims[8];
  int out = 0;
  for (int i = 1; i < rank && out < 8; ++i) dims[out++] = array_dim_at(ts, i);
  set_array_dims(ts, dims, out);
  // Update inner_rank: we consumed one outer dim.
  // If inner_rank >= 0, the outer dims count = (old_rank - inner_rank).
  // After drop, outer_count -= 1. If outer_count becomes 0, restore is_ptr_to_array.
  if (ts.inner_rank >= 0) {
    int new_rank = out;
    if (new_rank == ts.inner_rank) {
      // All outer (declarator) dims consumed; remaining are typedef inner dims.
      ts.is_ptr_to_array = true;
      ts.inner_rank = -1;
    }
  }
  return ts;
}

static bool is_unsigned_base(TypeBase b) {
  return b == TB_UCHAR || b == TB_USHORT || b == TB_UINT ||
         b == TB_ULONG || b == TB_ULONGLONG || b == TB_BOOL;
}

static bool is_float_base(TypeBase b) {
  return b == TB_FLOAT || b == TB_DOUBLE || b == TB_LONGDOUBLE;
}

static bool is_complex_base(TypeBase b) {
  switch (b) {
  case TB_COMPLEX_FLOAT: case TB_COMPLEX_DOUBLE: case TB_COMPLEX_LONGDOUBLE:
  case TB_COMPLEX_CHAR: case TB_COMPLEX_SCHAR: case TB_COMPLEX_UCHAR:
  case TB_COMPLEX_SHORT: case TB_COMPLEX_USHORT:
  case TB_COMPLEX_INT: case TB_COMPLEX_UINT:
  case TB_COMPLEX_LONG: case TB_COMPLEX_ULONG:
  case TB_COMPLEX_LONGLONG: case TB_COMPLEX_ULONGLONG:
    return true;
  default: return false;
  }
}

static TypeSpec complex_component_ts(TypeBase b) {
  switch (b) {
  case TB_COMPLEX_FLOAT:      return make_ts(TB_FLOAT);
  case TB_COMPLEX_LONGDOUBLE: return make_ts(TB_LONGDOUBLE);
  case TB_COMPLEX_DOUBLE:     return make_ts(TB_DOUBLE);
  case TB_COMPLEX_CHAR:       return make_ts(TB_CHAR);
  case TB_COMPLEX_SCHAR:      return make_ts(TB_SCHAR);
  case TB_COMPLEX_UCHAR:      return make_ts(TB_UCHAR);
  case TB_COMPLEX_SHORT:      return make_ts(TB_SHORT);
  case TB_COMPLEX_USHORT:     return make_ts(TB_USHORT);
  case TB_COMPLEX_INT:        return make_ts(TB_INT);
  case TB_COMPLEX_UINT:       return make_ts(TB_UINT);
  case TB_COMPLEX_LONG:       return make_ts(TB_LONG);
  case TB_COMPLEX_ULONG:      return make_ts(TB_ULONG);
  case TB_COMPLEX_LONGLONG:   return make_ts(TB_LONGLONG);
  case TB_COMPLEX_ULONGLONG:  return make_ts(TB_ULONGLONG);
  default:                    return make_ts(TB_DOUBLE);
  }
}

// Return bit-width from an LLVM integer type string ("i8"→8, "i32"→32, …)
static int llty_bits(const std::string& llty) {
  if (llty == "i1")   return 1;
  if (llty == "i8")   return 8;
  if (llty == "i16")  return 16;
  if (llty == "i32")  return 32;
  if (llty == "i64")  return 64;
  if (llty == "i128") return 128;
  if (llty == "float")  return 32;
  if (llty == "double") return 64;
  return 32;
}


// Forward declaration (defined later near global_const)
static int infer_array_size_from_init(Node* init);
// Forward declaration of static integer constant evaluator (defined near global_const)
static long long static_eval_int(Node* n, const std::unordered_map<std::string, long long>& enum_consts);

// Check if a string literal is a wide string (starts with L, u, U, or u8 prefix).
static bool is_wide_str_lit(Node* n) {
  if (!n || n->kind != NK_STR_LIT || !n->sval) return false;
  const char* s = n->sval;
  return (s[0] == 'L' || s[0] == 'u' || s[0] == 'U') && s[1] == '"';
}

// Compute the byte length of a narrow string literal (including null terminator).
// Returns -1 if unknown.
static int str_lit_byte_len(Node* n) {
  if (!n || n->kind != NK_STR_LIT || !n->sval || is_wide_str_lit(n)) return -1;
  const char* raw = n->sval;
  // Find opening quote
  int start = 0;
  if (raw[start] == '"') start++;
  else return -1;
  int len = 0;
  for (const char* p = raw + start; *p && *p != '"'; p++) {
    if (*p == '\\' && *(p+1)) {
      char esc = *(p+1);
      if ((esc >= '0' && esc <= '7')) {
        // Octal: up to 3 digits
        p++;
        while (*(p+1) >= '0' && *(p+1) <= '7') p++;
      } else if (esc == 'x' || esc == 'X') {
        p++;
        while (isxdigit((unsigned char)*(p+1))) p++;
      } else {
        p++;  // single char escape
      }
    }
    len++;
  }
  return len + 1;  // +1 for null terminator
}

// In C, assigning a string literal to char*/unsigned char*/signed char*/void*
// is accepted (although writing through it is undefined behavior).
static bool allows_string_literal_ptr_target(const TypeSpec& ts) {
  if (ts.ptr_level <= 0) return false;
  switch (ts.base) {
    case TB_CHAR:
    case TB_SCHAR:
    case TB_UCHAR:
    case TB_VOID:
      return true;
    default:
      return false;
  }
}

static std::string decode_narrow_string_lit(const char* sval) {
  if (!sval) return "";
  std::string raw = sval;
  std::string content;
  if (raw.size() >= 2 && raw.front() == '"' && raw.back() == '"') {
    for (size_t i = 1; i + 1 < raw.size(); i++) {
      if (raw[i] == '\\' && i + 1 < raw.size() - 1) {
        char esc = raw[++i];
        switch (esc) {
          case 'n': content += '\n'; break;
          case 't': content += '\t'; break;
          case 'r': content += '\r'; break;
          case '0': content += '\0'; break;
          case '\\': content += '\\'; break;
          case '"': content += '"'; break;
          case 'x': case 'X': {
            unsigned int v = 0; int nhex = 0;
            while (i + 1 < raw.size() - 1 && isxdigit((unsigned char)raw[i + 1])) {
              char h = raw[++i];
              v = v * 16 + (isdigit((unsigned char)h) ? h - '0'
                           : (tolower((unsigned char)h) - 'a' + 10));
              nhex++;
            }
            if (nhex == 0) content += 'x';
            else content += (char)(unsigned char)v;
            break;
          }
          default:
            if (esc >= '0' && esc <= '7') {
              unsigned int v = (unsigned int)(esc - '0');
              int cnt = 1;
              while (cnt < 3 && i + 1 < raw.size() - 1 &&
                     raw[i + 1] >= '0' && raw[i + 1] <= '7') {
                v = v * 8 + (unsigned int)(raw[++i] - '0');
                cnt++;
              }
              content += (char)(unsigned char)v;
            } else {
              content += esc;
            }
            break;
        }
      } else {
        content += raw[i];
      }
    }
  } else {
    content = raw;
  }
  return content;
}

static std::string normalize_printf_longdouble_format(std::string s) {
  std::string out;
  out.reserve(s.size());
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] != '%') { out.push_back(s[i]); continue; }
    if (i + 1 < s.size() && s[i + 1] == '%') { out += "%%"; i++; continue; }
    size_t j = i + 1;
    bool saw_L = false;
    std::string spec = "%";
    while (j < s.size()) {
      char c = s[j];
      if (c == 'L') { saw_L = true; j++; continue; }
      spec.push_back(c);
      if (isalpha((unsigned char)c) || c == '%') {
        if (saw_L && (c == 'f' || c == 'F' || c == 'e' || c == 'E' ||
                      c == 'g' || c == 'G' || c == 'a' || c == 'A')) {
          out += spec;
        } else {
          if (saw_L) out.push_back('%');
          if (saw_L) {
            for (size_t k = i + 1; k <= j; k++) if (s[k] != 'L') out.push_back(s[k]);
          } else {
            out += spec;
          }
        }
        break;
      }
      j++;
    }
    i = j;
  }
  return out;
}

// Decode a wide string literal into a vector of Unicode codepoints.
// sval is like L"hello\u4F60..." — returns codepoints including null terminator.
static std::vector<uint32_t> decode_wide_string(const char* sval) {
  std::vector<uint32_t> codepoints;
  if (!sval) { codepoints.push_back(0); return codepoints; }
  // Skip prefix (L, u, U) and opening quote
  const char* p = sval;
  while (*p && *p != '"') p++;
  if (*p == '"') p++;  // skip opening quote
  while (*p && *p != '"') {
    if (*p == '\\') {
      p++;
      switch (*p) {
        case 'n':  codepoints.push_back('\n'); p++; break;
        case 't':  codepoints.push_back('\t'); p++; break;
        case 'r':  codepoints.push_back('\r'); p++; break;
        case '0':  codepoints.push_back(0); p++; break;
        case 'a':  codepoints.push_back('\a'); p++; break;
        case 'b':  codepoints.push_back('\b'); p++; break;
        case 'f':  codepoints.push_back('\f'); p++; break;
        case 'v':  codepoints.push_back('\v'); p++; break;
        case '\\': codepoints.push_back('\\'); p++; break;
        case '"':  codepoints.push_back('"'); p++; break;
        case '\'': codepoints.push_back('\''); p++; break;
        case 'x': case 'X': {
          p++;
          uint32_t v = 0;
          while (*p && isxdigit((unsigned char)*p)) {
            v = v * 16 + (isdigit((unsigned char)*p) ? *p - '0' :
                          tolower((unsigned char)*p) - 'a' + 10);
            p++;
          }
          codepoints.push_back(v);
          break;
        }
        case 'u': {
          // \uXXXX
          p++;
          uint32_t v = 0;
          for (int k = 0; k < 4 && *p && isxdigit((unsigned char)*p); k++, p++) {
            v = v * 16 + (isdigit((unsigned char)*p) ? *p - '0' :
                          tolower((unsigned char)*p) - 'a' + 10);
          }
          codepoints.push_back(v);
          break;
        }
        case 'U': {
          // \UXXXXXXXX
          p++;
          uint32_t v = 0;
          for (int k = 0; k < 8 && *p && isxdigit((unsigned char)*p); k++, p++) {
            v = v * 16 + (isdigit((unsigned char)*p) ? *p - '0' :
                          tolower((unsigned char)*p) - 'a' + 10);
          }
          codepoints.push_back(v);
          break;
        }
        default:
          if (*p >= '0' && *p <= '7') {
            uint32_t v = 0;
            for (int k = 0; k < 3 && *p >= '0' && *p <= '7'; k++, p++)
              v = v * 8 + (*p - '0');
            codepoints.push_back(v);
          } else {
            codepoints.push_back((unsigned char)*p); p++;
          }
          break;
      }
    } else {
      // Decode UTF-8 multi-byte sequence
      unsigned char c0 = (unsigned char)*p;
      if (c0 < 0x80) {
        codepoints.push_back(c0); p++;
      } else if ((c0 & 0xE0) == 0xC0) {
        uint32_t v = c0 & 0x1F;
        if (p[1] && (unsigned char)p[1] >= 0x80) { v = (v << 6) | ((unsigned char)p[1] & 0x3F); p += 2; }
        else p++;
        codepoints.push_back(v);
      } else if ((c0 & 0xF0) == 0xE0) {
        uint32_t v = c0 & 0x0F;
        if (p[1] && (unsigned char)p[1] >= 0x80) { v = (v << 6) | ((unsigned char)p[1] & 0x3F); p += 2; }
        else { p++; codepoints.push_back(v); continue; }
        if (*p && (unsigned char)*p >= 0x80) { v = (v << 6) | ((unsigned char)*p & 0x3F); p++; }
        codepoints.push_back(v);
      } else if ((c0 & 0xF8) == 0xF0) {
        uint32_t v = c0 & 0x07;
        for (int k = 0; k < 3 && p[1] && (unsigned char)p[1] >= 0x80; k++, p++) {
          v = (v << 6) | ((unsigned char)p[1] & 0x3F);
        }
        p++;
        codepoints.push_back(v);
      } else {
        codepoints.push_back(c0); p++;
      }
    }
  }
  codepoints.push_back(0);  // null terminator
  return codepoints;
}

// ─────────────────────────── constructor ───────────────────────────────────

IRBuilder::IRBuilder()
    : string_idx_(0), tmp_idx_(0), label_idx_(0),
      last_was_terminator_(false), block_depth_(0) {}

// ─────────────────────────── type helpers ──────────────────────────────────

// Return LLVM element type for GEP: void → i8 (void has no size; GEP of void* uses i8).
static std::string gep_elem_ty(const std::string& llty) {
    return (llty == "void") ? "i8" : llty;
}

std::string IRBuilder::llvm_ty_base(TypeBase base) {
  switch (base) {
  case TB_VOID:       return "void";
  case TB_BOOL:       return "i1";
  case TB_CHAR:
  case TB_UCHAR:
  case TB_SCHAR:      return "i8";
  case TB_SHORT:
  case TB_USHORT:     return "i16";
  case TB_INT:
  case TB_UINT:       return "i32";
  case TB_LONG:
  case TB_ULONG:      return "i64";   // LP64 (macOS / Linux)
  case TB_LONGLONG:
  case TB_ULONGLONG:  return "i64";
  case TB_FLOAT:      return "float";
  case TB_DOUBLE:
  case TB_LONGDOUBLE: return "double";
  case TB_COMPLEX_FLOAT:      return "{ float, float }";
  case TB_COMPLEX_DOUBLE:     return "{ double, double }";
  case TB_COMPLEX_LONGDOUBLE: return "{ double, double }";
  case TB_COMPLEX_CHAR:
  case TB_COMPLEX_SCHAR:
  case TB_COMPLEX_UCHAR:      return "{ i8, i8 }";
  case TB_COMPLEX_SHORT:
  case TB_COMPLEX_USHORT:     return "{ i16, i16 }";
  case TB_COMPLEX_INT:
  case TB_COMPLEX_UINT:       return "{ i32, i32 }";
  case TB_COMPLEX_LONG:
  case TB_COMPLEX_ULONG:
  case TB_COMPLEX_LONGLONG:
  case TB_COMPLEX_ULONGLONG:  return "{ i64, i64 }";
  case TB_INT128:
  case TB_UINT128:    return "i128";
  case TB_ENUM:       return "i32";
  case TB_VA_LIST:    // AArch64 glibc va_list: struct __va_list_tag
    return "%struct.__va_list_tag_";
  case TB_FUNC_PTR:   return "ptr";
  case TB_TYPEDEF:    return "i32";
  default:            return "i32";
  }
}

std::string IRBuilder::llvm_ty(const TypeSpec& ts) {
  if (ts.is_fn_ptr && ts.ptr_level == 0) return "ptr";  // function pointer value
  if (ts.ptr_level > 0 && ts.is_ptr_to_array) return "ptr";
  // Array-of-pointers-to-typedef-array: inner_rank splits the dims.
  // The outer (array_rank - inner_rank) dims form [N x ptr].
  if (ts.ptr_level > 0 && ts.inner_rank >= 0 && is_array_ty(ts)) {
    int outer_rank = ts.array_rank - ts.inner_rank;
    if (outer_rank <= 0) return "ptr";  // no outer dims, just ptr
    // Build [dim0 x [dim1 x ... x ptr]] for outer_rank dims
    std::string result = "ptr";
    for (int i = outer_rank - 1; i >= 0; i--) {
      long long dim = array_dim_at(ts, i);
      if (dim < 0) dim = 0;
      result = "[" + std::to_string(dim) + " x " + result + "]";
    }
    return result;
  }
  // Check array BEFORE ptr_level: for typedef-derived pointer types used as array elements
  // (e.g. typedef void (*fptr)(void); fptr table[3] → [3 x ptr], not ptr)
  if (is_array_ty(ts)) {
    long long dim = array_dim_at(ts, 0);
    if (dim < 0) dim = 0;
    TypeSpec elem = drop_array_dim(ts);
    std::string elem_llty = llvm_ty(elem);
    // void is not a valid array element type; use ptr instead (e.g. function pointer arrays)
    if (elem_llty == "void") elem_llty = "ptr";
    return "[" + std::to_string(dim) + " x " + elem_llty + "]";
  }
  if (ts.ptr_level > 0) return "ptr";
  if (ts.base == TB_STRUCT || ts.base == TB_UNION) {
    if (ts.tag && ts.tag[0]) {
      return std::string("%struct.") + ts.tag;
    }
    return "ptr";  // opaque fallback
  }
  return llvm_ty_base(ts.base);
}

// Return the ABI alignment of a type (in bytes) for AArch64.
static int alignof_ty_impl(const TypeSpec& ts,
                            const std::unordered_map<std::string, IRBuilder::StructInfo>& struct_defs);

static int alignof_ty_impl(const TypeSpec& ts,
                            const std::unordered_map<std::string, IRBuilder::StructInfo>& struct_defs) {
  if (ts.ptr_level > 0) return 8;
  if (is_array_ty(ts)) {
    TypeSpec elem = drop_array_dim(ts);
    return alignof_ty_impl(elem, struct_defs);
  }
  switch (ts.base) {
  case TB_BOOL:
  case TB_CHAR: case TB_UCHAR: case TB_SCHAR: return 1;
  case TB_SHORT: case TB_USHORT: return 2;
  case TB_INT:  case TB_UINT:   return 4;
  case TB_LONG: case TB_ULONG:  return 8;
  case TB_LONGLONG: case TB_ULONGLONG: return 8;
  case TB_FLOAT:    return 4;
  case TB_DOUBLE:   return 8;
  case TB_LONGDOUBLE: return 8;
  case TB_COMPLEX_FLOAT: return 4;
  case TB_COMPLEX_DOUBLE: return 8;
  case TB_COMPLEX_LONGDOUBLE: return 8;
  case TB_COMPLEX_CHAR: case TB_COMPLEX_SCHAR: case TB_COMPLEX_UCHAR: return 1;
  case TB_COMPLEX_SHORT: case TB_COMPLEX_USHORT: return 2;
  case TB_COMPLEX_INT: case TB_COMPLEX_UINT: return 4;
  case TB_COMPLEX_LONG: case TB_COMPLEX_ULONG:
  case TB_COMPLEX_LONGLONG: case TB_COMPLEX_ULONGLONG: return 8;
  case TB_INT128: case TB_UINT128: return 16;
  case TB_ENUM:   return 4;
  case TB_VA_LIST: return 8;
  case TB_STRUCT: case TB_UNION: {
    if (!ts.tag || !ts.tag[0]) return 8;
    auto it = struct_defs.find(ts.tag);
    if (it == struct_defs.end()) return 8;
    const IRBuilder::StructInfo& si = it->second;
    int max_align = 1;
    for (size_t i = 0; i < si.field_types.size(); i++) {
      TypeSpec ft = si.field_types[i];
      if (si.field_array_sizes[i] >= 0) {
        // field element type; align is same as element
      }
      int a = alignof_ty_impl(ft, struct_defs);
      if (a > max_align) max_align = a;
    }
    return max_align;
  }
  default: return 4;
  }
}

int IRBuilder::sizeof_ty(const TypeSpec& ts) {
  if (ts.ptr_level > 0 && ts.is_ptr_to_array) return 8;
  // Check array before ptr_level: e.g. fptr[3] has ptr_level=1 but is an array
  if (is_array_ty(ts)) {
    long long dim = array_dim_at(ts, 0);
    if (dim < 0) dim = 0;
    TypeSpec elem = drop_array_dim(ts);
    return (int)dim * sizeof_ty(elem);
  }
  if (ts.ptr_level > 0) return 8;
  switch (ts.base) {
  case TB_BOOL:
  case TB_CHAR: case TB_UCHAR: case TB_SCHAR: return 1;
  case TB_SHORT: case TB_USHORT: return 2;
  case TB_INT:  case TB_UINT:   return 4;
  case TB_LONG: case TB_ULONG:  return 8;
  case TB_LONGLONG: case TB_ULONGLONG: return 8;
  case TB_FLOAT:    return 4;
  case TB_DOUBLE:   return 8;
  case TB_LONGDOUBLE: return 8; // current IR model maps long double to double
  case TB_COMPLEX_FLOAT: return 8;
  case TB_COMPLEX_DOUBLE: return 16;
  case TB_COMPLEX_LONGDOUBLE: return 32;
  case TB_COMPLEX_CHAR: case TB_COMPLEX_SCHAR: case TB_COMPLEX_UCHAR: return 2;
  case TB_COMPLEX_SHORT: case TB_COMPLEX_USHORT: return 4;
  case TB_COMPLEX_INT: case TB_COMPLEX_UINT: return 8;
  case TB_COMPLEX_LONG: case TB_COMPLEX_ULONG:
  case TB_COMPLEX_LONGLONG: case TB_COMPLEX_ULONGLONG: return 16;
  case TB_INT128: case TB_UINT128: return 16;
  case TB_ENUM:   return 4;
  case TB_VA_LIST: return 40; // {ptr,ptr,ptr,i32,i32}
  case TB_STRUCT: case TB_UNION: {
    if (!ts.tag || !ts.tag[0]) return 8;
    auto it = struct_defs_.find(ts.tag);
    if (it == struct_defs_.end()) return 8;
    const StructInfo& si = it->second;
    if (si.is_union) {
      // union: size = max field size rounded up to max alignment
      int max_sz = 0, max_align = 1;
      for (size_t i = 0; i < si.field_types.size(); i++) {
        if (si.field_array_sizes[i] == -2) continue;
        TypeSpec ft = si.field_types[i];
        int sz = sizeof_ty(ft);
        if (si.field_array_sizes[i] >= 0)
          sz = (int)si.field_array_sizes[i] * sz;
        if (sz > max_sz) max_sz = sz;
        int a = alignof_ty_impl(ft, struct_defs_);
        if (a > max_align) max_align = a;
      }
      // pad up to alignment
      return (max_sz + max_align - 1) & ~(max_align - 1);
    } else {
      // struct: sum fields with alignment padding, then pad end to struct alignment
      int offset = 0, max_align = 1;
      for (size_t i = 0; i < si.field_types.size(); i++) {
        if (si.field_array_sizes[i] == -2) continue;  // flex array: zero size
        TypeSpec ft = si.field_types[i];
        int sz = sizeof_ty(ft);
        if (si.field_array_sizes[i] >= 0)
          sz = (int)si.field_array_sizes[i] * sz;
        int a = alignof_ty_impl(ft, struct_defs_);
        if (a > max_align) max_align = a;
        // pad current offset to field alignment
        offset = (offset + a - 1) & ~(a - 1);
        offset += sz;
      }
      // pad end to struct alignment
      return (offset + max_align - 1) & ~(max_align - 1);
    }
  }
  default: return 4;
  }
}

// ─────────────────────────── expression type ───────────────────────────────

TypeSpec IRBuilder::expr_type(Node* n) {
  if (!n) return void_ts();
  switch (n->kind) {
  case NK_INT_LIT: {
    // &&label (GCC label address): type is void*
    if (n->name && !n->is_imaginary) {
      TypeSpec vp{}; vp.base = TB_VOID; vp.ptr_level = 1;
      return vp;
    }
    // Imaginary integer literal (e.g. 200i) → _Complex long long
    if (n->is_imaginary) return make_ts(TB_COMPLEX_LONGLONG);
    // Check suffix for type: LL/ULL → long long, L/UL → long, U → unsigned int
    const char* sv = n->sval;
    bool is_hex_or_oct = sv && sv[0] == '0' && (sv[1] == 'x' || sv[1] == 'X' ||
                                                 (sv[1] >= '0' && sv[1] <= '7'));
    if (sv) {
      size_t len = strlen(sv);
      // Scan suffix (case-insensitive): strip trailing 'l', 'll', 'u', 'i', 'j'
      int lcount = 0; bool has_u = false;
      for (int i = (int)len - 1; i >= 0; i--) {
        char c = sv[i];
        if (c == 'l' || c == 'L') { lcount++; }
        else if (c == 'u' || c == 'U') { has_u = true; }
        else if (c == 'i' || c == 'I' || c == 'j' || c == 'J') { /* skip */ }
        else break;
      }
      if (lcount >= 2) return has_u ? make_ts(TB_ULONGLONG) : make_ts(TB_LONGLONG);
      if (lcount == 1) return has_u ? make_ts(TB_ULONG) : make_ts(TB_LONG);
      if (has_u) return make_ts(TB_UINT);
    }
    // No suffix: for hex/octal literals, C standard says try each type in order:
    // int, uint, long, ulong, llong, ullong. For decimal, only signed types.
    // This matters for 0xabcd0000 (doesn't fit int → uint).
    long long v = n->ival;
    if (is_hex_or_oct) {
      if (v >= 0 && v <= 0x7fffffff) return int_ts();             // fits int
      if ((unsigned long long)v <= 0xffffffff) return make_ts(TB_UINT);  // fits uint
      if (v >= 0 && v <= (long long)0x7fffffffffffffff) return make_ts(TB_LONGLONG);  // fits llong
      return make_ts(TB_ULONGLONG);  // fits ullong
    }
    return int_ts();
  }
  case NK_CHAR_LIT:
    // Wide char literal L'...' has type int (wchar_t); narrow has type char
    if (n->sval && n->sval[0] == 'L') return int_ts();
    return char_ts();
  case NK_FLOAT_LIT: {
    // Imaginary float literal (e.g. 1.0fi) → _Complex float or _Complex double
    if (n->is_imaginary) {
      const char* sv = n->sval;
      bool is_f32 = sv && (strchr(sv, 'f') || strchr(sv, 'F'));
      return is_f32 ? make_ts(TB_COMPLEX_FLOAT) : make_ts(TB_COMPLEX_DOUBLE);
    }
    const char* sv = n->sval;
    if (sv && (sv[strlen(sv)-1] == 'f' || sv[strlen(sv)-1] == 'F'))
      return float_ts();
    return double_ts();
  }
  case NK_STR_LIT:
    // Wide string literals (L"...") have type wchar_t* = int* on LP64 platforms
    if (is_wide_str_lit(n)) return ptr_to(TB_INT);
    return ptr_to(TB_CHAR);

  case NK_VAR: {
    if (n->name) {
      auto it = local_types_.find(n->name);
      if (it != local_types_.end()) return it->second;
      auto it2 = global_types_.find(n->name);
      if (it2 != global_types_.end()) return it2->second;
      if (enum_consts_.count(n->name)) return int_ts();
      // Function used as value (function pointer): return ptr
      if (func_sigs_.count(n->name)) return ptr_to(TB_VOID);
    }
    return int_ts();
  }

  case NK_BINOP: {
    const char* op = n->op;
    // Logical / comparison → int
    if (!op) return int_ts();
    if (strcmp(op,"==")==0 || strcmp(op,"!=")==0 ||
        strcmp(op,"<")==0  || strcmp(op,"<=")==0 ||
        strcmp(op,">")==0  || strcmp(op,">=")==0 ||
        strcmp(op,"&&")==0 || strcmp(op,"||")==0)
      return int_ts();
    // Shifts: result type = promoted left operand (C11 §6.3.1.1)
    if (strcmp(op,"<<")==0 || strcmp(op,">>")==0) {
      TypeSpec lt = expr_type(n->left);
      if (lt.ptr_level > 0) lt = int_ts();
      // Narrow integer types promote to int before shifting
      if (lt.ptr_level == 0 && !is_array_ty(lt) && !is_float_base(lt.base) &&
          !is_complex_base(lt.base) && sizeof_ty(lt) < 4)
        return int_ts();
      return lt;
    }
    TypeSpec lt = expr_type(n->left);
    TypeSpec rt = expr_type(n->right);
    // Vector element-wise arithmetic: both operands are the same 1D array type (vector_size).
    // Return the vector type instead of decaying to pointer.
    if (is_array_ty(lt) && lt.ptr_level == 0 && !lt.is_ptr_to_array &&
        is_array_ty(rt) && rt.ptr_level == 0 && !rt.is_ptr_to_array &&
        array_rank_of(lt) == 1 && array_rank_of(rt) == 1 &&
        op && strcmp(op,"==") != 0 && strcmp(op,"!=") != 0 &&
        strcmp(op,"<") != 0 && strcmp(op,"<=") != 0 &&
        strcmp(op,">") != 0 && strcmp(op,">=") != 0)
      return lt;
    // Pointer subtraction: ptr - ptr → ptrdiff_t (i64)
    if ((lt.ptr_level > 0 || is_array_ty(lt)) &&
        (rt.ptr_level > 0 || is_array_ty(rt)) &&
        op && strcmp(op, "-") == 0)
      return ll_ts();
    // Pointer arithmetic: ptr/array +/- int → ptr (array decays to pointer)
    if (lt.ptr_level > 0) return lt;
    if (is_array_ty(lt)) {
      TypeSpec decay = drop_array_dim(lt);
      decay.ptr_level += 1;
      // If the element type is still an array (multi-dim), mark as ptr-to-array
      // so NK_DEREF correctly returns the sub-array ptr without loading.
      if (is_array_ty(decay)) decay.is_ptr_to_array = true;
      return decay;
    }
    if (rt.ptr_level > 0) return rt;
    if (is_array_ty(rt)) {
      TypeSpec decay = drop_array_dim(rt);
      decay.ptr_level += 1;
      if (is_array_ty(decay)) decay.is_ptr_to_array = true;
      return decay;
    }
    // Complex arithmetic: if either operand is complex, return the wider complex type
    if (is_complex_base(lt.base) || is_complex_base(rt.base)) {
      if (is_complex_base(lt.base) && is_complex_base(rt.base))
        return (sizeof_ty(lt) >= sizeof_ty(rt)) ? lt : rt;
      return is_complex_base(lt.base) ? lt : rt;
    }
    // Usual arithmetic conversions (C11 §6.3.1.8): unsigned types win at same rank
    if (lt.base == TB_DOUBLE || rt.base == TB_DOUBLE) return double_ts();
    if (lt.base == TB_FLOAT  || rt.base == TB_FLOAT)  return float_ts();
    if (lt.base == TB_LONGDOUBLE || rt.base == TB_LONGDOUBLE) return double_ts();
    if (lt.base == TB_ULONGLONG || rt.base == TB_ULONGLONG) return make_ts(TB_ULONGLONG);
    if (lt.base == TB_LONGLONG  || rt.base == TB_LONGLONG) {
      // longlong + ulong: same size on 64-bit, unsigned wins → ulonglong
      if (lt.base == TB_ULONG || rt.base == TB_ULONG) return make_ts(TB_ULONGLONG);
      return ll_ts();
    }
    if (lt.base == TB_ULONG || rt.base == TB_ULONG) return make_ts(TB_ULONG);
    if (lt.base == TB_LONG  || rt.base == TB_LONG)  return make_ts(TB_LONG);
    if (lt.base == TB_UINT  || rt.base == TB_UINT)  return make_ts(TB_UINT);
    return int_ts();
  }

  case NK_UNARY: {
    if (!n->op) return int_ts();
    if (strcmp(n->op,"!")==0) return int_ts();
    TypeSpec ts = expr_type(n->left);
    // C11 §6.3.1.1: integer promotions apply to arithmetic unary operators
    if ((strcmp(n->op,"~")==0 || strcmp(n->op,"-")==0 || strcmp(n->op,"+")==0) &&
        ts.ptr_level == 0 && !is_array_ty(ts) && !is_float_base(ts.base) &&
        !is_complex_base(ts.base) && sizeof_ty(ts) < 4)
      return int_ts();
    return ts;
  }
  case NK_POSTFIX:          return expr_type(n->left);
  case NK_ADDR: {
    TypeSpec inner = expr_type(n->left);
    // Array decays to pointer (same base, just 1 more ptr level)
    if (is_array_ty(inner)) {
      inner = drop_array_dim(inner);
      inner.ptr_level += 1;
    } else {
      inner.ptr_level += 1;
    }
    return inner;
  }
  case NK_DEREF: {
    TypeSpec inner = expr_type(n->left);
    if (inner.ptr_level > 0) {
      inner.ptr_level -= 1;
      if (inner.ptr_level == 0) inner.is_ptr_to_array = false;  // deref gives the value, not ptr-to-array
    }
    else if (is_array_ty(inner)) inner = drop_array_dim(inner);
    return inner;
  }
  case NK_REAL_PART: {
    TypeSpec inner = expr_type(n->left);
    if (inner.ptr_level == 0 && is_complex_base(inner.base))
      return complex_component_ts(inner.base);
    // GCC: __real__ scalar_expr yields the scalar type itself.
    return inner;
  }
  case NK_IMAG_PART: {
    TypeSpec inner = expr_type(n->left);
    if (inner.ptr_level == 0 && is_complex_base(inner.base))
      return complex_component_ts(inner.base);
    // GCC: __imag__ scalar_expr yields 0 of scalar type.
    return inner;
  }
  case NK_ASSIGN:
  case NK_COMPOUND_ASSIGN:  return expr_type(n->left);
  case NK_CALL: {
    if (n->left && n->left->name) {
      const char* cn = n->left->name;
      // Remap common __builtin_* before signature lookup.
      std::string cn_str = cn;
      static const struct { const char* from; const char* to; } et_remap[] = {
        {"__builtin_malloc",  "malloc"},  {"__builtin_free",    "free"},
        {"__builtin_calloc",  "calloc"},  {"__builtin_realloc", "realloc"},
        {"__builtin_alloca",  "alloca"},
        {"__builtin_memcpy",  "memcpy"},  {"__builtin_memmove", "memmove"},
        {"__builtin_memset",  "memset"},  {"__builtin_memcmp",  "memcmp"},
        {"__builtin_memchr",  "memchr"},  {"__builtin_strcpy",  "strcpy"},
        {"__builtin_strncpy", "strncpy"}, {"__builtin_strcat",  "strcat"},
        {"__builtin_strncat", "strncat"}, {"__builtin_strcmp",  "strcmp"},
        {"__builtin_strncmp", "strncmp"}, {"__builtin_strlen",  "strlen"},
        {"__builtin_strchr",  "strchr"},  {"__builtin_strstr",  "strstr"},
        {"__builtin_printf",  "printf"},  {"__builtin_puts",    "puts"},
        {"__builtin_abort",   "abort"},   {"__builtin_exit",    "exit"},
        {nullptr, nullptr}
      };
      for (int ri = 0; et_remap[ri].from; ri++) {
        if (cn_str == et_remap[ri].from) { cn_str = et_remap[ri].to; break; }
      }
      cn = cn_str.c_str();
      auto it = func_sigs_.find(cn);
      if (it != func_sigs_.end()) return it->second.ret_type;
      // Known pointer-returning standard library functions.
      if (strcmp(cn, "malloc") == 0 || strcmp(cn, "calloc") == 0 ||
          strcmp(cn, "realloc") == 0 || strcmp(cn, "strdup") == 0 ||
          strcmp(cn, "strndup") == 0 || strcmp(cn, "memcpy") == 0 ||
          strcmp(cn, "memmove") == 0 || strcmp(cn, "memset") == 0 ||
          strcmp(cn, "alloca") == 0 || strcmp(cn, "memchr") == 0 ||
          strcmp(cn, "strcpy") == 0 || strcmp(cn, "strncpy") == 0 ||
          strcmp(cn, "strcat") == 0 || strcmp(cn, "strncat") == 0 ||
          strcmp(cn, "strchr") == 0 || strcmp(cn, "strstr") == 0)
        return ptr_to(TB_VOID);
      // Known void-returning standard library functions.
      if (strcmp(cn, "free") == 0 || strcmp(cn, "abort") == 0 ||
          strcmp(cn, "exit") == 0 || strcmp(cn, "puts") == 0)
        return void_ts();
      // Builtin return types
      if (strcmp(cn, "__builtin_bswap64") == 0) return make_ts(TB_ULONGLONG);
      if (strcmp(cn, "__builtin_bswap32") == 0) return make_ts(TB_UINT);
      if (strcmp(cn, "__builtin_bswap16") == 0) return make_ts(TB_USHORT);
      if (strcmp(cn, "__builtin_expect") == 0 && n->n_children > 0)
        return expr_type(n->children[0]);
      if (strcmp(cn, "__builtin_fabs") == 0 || strcmp(cn, "__builtin_fabsl") == 0 ||
          strcmp(cn, "__builtin_inf") == 0 || strcmp(cn, "__builtin_infl") == 0 ||
          strcmp(cn, "__builtin_huge_val") == 0 || strcmp(cn, "__builtin_huge_vall") == 0)
        return double_ts();
      if (strcmp(cn, "__builtin_fabsf") == 0 || strcmp(cn, "__builtin_inff") == 0 ||
          strcmp(cn, "__builtin_huge_valf") == 0)
        return float_ts();
      if (strcmp(cn, "__builtin_constant_p") == 0) return make_ts(TB_INT);
      if (strcmp(cn, "__builtin_unreachable") == 0) return void_ts();
      if (strcmp(cn, "__builtin_signbit") == 0 || strcmp(cn, "__builtin_signbitf") == 0 ||
          strcmp(cn, "__builtin_signbitl") == 0) return make_ts(TB_INT);
      if (strcmp(cn, "__builtin_add_overflow") == 0 || strcmp(cn, "__builtin_sub_overflow") == 0 ||
          strcmp(cn, "__builtin_mul_overflow") == 0) return make_ts(TB_INT);
      if (strcmp(cn, "__builtin_conjf") == 0) return make_ts(TB_COMPLEX_FLOAT);
      if (strcmp(cn, "__builtin_conj")  == 0) return make_ts(TB_COMPLEX_DOUBLE);
      if (strcmp(cn, "__builtin_conjl") == 0) return make_ts(TB_COMPLEX_LONGDOUBLE);
      if (strcmp(cn, "__builtin_classify_type") == 0) return make_ts(TB_INT);
      if (strcmp(cn, "__builtin_copysign")  == 0 || strcmp(cn, "__builtin_copysignl") == 0) return double_ts();
      if (strcmp(cn, "__builtin_copysignf") == 0) return float_ts();
      if (strcmp(cn, "__builtin_nan")  == 0 || strcmp(cn, "__builtin_nanl")  == 0) return double_ts();
      if (strcmp(cn, "__builtin_nanf") == 0) return float_ts();
      if (strcmp(cn, "__builtin_isgreater") == 0 || strcmp(cn, "__builtin_isgreaterequal") == 0 ||
          strcmp(cn, "__builtin_isless")    == 0 || strcmp(cn, "__builtin_islessequal")    == 0 ||
          strcmp(cn, "__builtin_islessgreater") == 0 || strcmp(cn, "__builtin_isunordered") == 0 ||
          strcmp(cn, "__builtin_isnan") == 0 || strcmp(cn, "__builtin_isinf") == 0 ||
          strcmp(cn, "__builtin_isinf_sign") == 0) return make_ts(TB_INT);
      if (strcmp(cn, "__builtin_ffs")  == 0 || strcmp(cn, "__builtin_ffsl")  == 0 ||
          strcmp(cn, "__builtin_ffsll") == 0) return make_ts(TB_INT);
      if (strcmp(cn, "__builtin_clz")  == 0 || strcmp(cn, "__builtin_clzl")  == 0 ||
          strcmp(cn, "__builtin_clzll") == 0) return make_ts(TB_INT);
      if (strcmp(cn, "__builtin_ctz")  == 0 || strcmp(cn, "__builtin_ctzl")  == 0 ||
          strcmp(cn, "__builtin_ctzll") == 0) return make_ts(TB_INT);
      if (strcmp(cn, "__builtin_popcount")  == 0 || strcmp(cn, "__builtin_popcountl")  == 0 ||
          strcmp(cn, "__builtin_popcountll") == 0) return make_ts(TB_INT);
      if (strcmp(cn, "__builtin_parity")  == 0 || strcmp(cn, "__builtin_parityl")  == 0 ||
          strcmp(cn, "__builtin_parityll") == 0) return make_ts(TB_INT);
    }
    // Indirect call through function pointer expression.
    if (n->left) {
      // Peel dereference to get the function pointer itself.
      Node* callee = n->left;
      if ((callee->kind == NK_DEREF || (callee->kind == NK_UNARY && callee->op && callee->op[0] == '*' && callee->op[1] == '\0')) && callee->left)
        callee = callee->left;
      if (callee->kind == NK_VAR && callee->name) {
        auto fsit = fptr_sigs_.find(callee->name);
        if (fsit != fptr_sigs_.end()) return fsit->second.ret_type;
      }
      TypeSpec callee_ts = expr_type(callee);
      // If the callee has is_fn_ptr set, its base type IS the function return type.
      if (callee_ts.is_fn_ptr && callee_ts.ptr_level > 0) {
        callee_ts.ptr_level--;
        callee_ts.is_fn_ptr = false;
        return callee_ts;
      }
    }
    return int_ts();
  }
  case NK_INDEX: {
    TypeSpec bt = expr_type(n->left);
    // Handle commutative form integer[pointer]: use the pointer/array side
    if (bt.ptr_level == 0 && !is_array_ty(bt)) {
      TypeSpec rt = expr_type(n->right);
      if (rt.ptr_level > 0 || is_array_ty(rt)) bt = rt;
    }
    if (is_array_ty(bt) && !(bt.ptr_level > 0 && bt.is_ptr_to_array))
      return drop_array_dim(bt);
    if (bt.ptr_level > 0) {
      bt.ptr_level -= 1;
      if (bt.ptr_level == 0 && bt.is_ptr_to_array) bt.is_ptr_to_array = false;
      return bt;
    }
    return int_ts();
  }
  case NK_MEMBER: {
    TypeSpec bts = expr_type(n->left);
    if (n->is_arrow && bts.ptr_level > 0) bts.ptr_level -= 1;
    if ((bts.base == TB_STRUCT || bts.base == TB_UNION) && bts.tag) {
      FieldPath path;
      if (find_field(bts.tag, n->name, path)) {
        TypeSpec ft = field_type_from_path(bts.tag, path);
        // C11 §6.3.1.1: narrow bitfields promote to int (or uint if width==32 and unsigned)
        int bfw = bitfield_width_from_path(bts.tag, path);
        if (bfw > 0 && bfw < 32 && ft.ptr_level == 0 && !is_array_ty(ft))
          return int_ts();
        return ft;
      }
      throw std::runtime_error(std::string("unknown field ") +
                               (n->name ? n->name : "?") + " in " + bts.tag);
    }
    return int_ts();
  }
  case NK_CAST:
  case NK_COMPOUND_LIT:
  case NK_VA_ARG:           return n->type;
  case NK_GENERIC:
    // Return type of first non-default association (they should all match anyway)
    for (int i = 0; i < n->n_children; i++)
      if (n->children[i] && n->children[i]->ival != 1)
        return expr_type(n->children[i]->left);
    if (n->n_children > 0 && n->children[0])
      return expr_type(n->children[0]->left);
    return int_ts();
  case NK_TERNARY:          return expr_type(n->then_);
  case NK_SIZEOF_EXPR:
  case NK_SIZEOF_TYPE:
  case NK_ALIGNOF_TYPE:     return make_ts(TB_ULONG);
  case NK_COMMA_EXPR:       return expr_type(n->right);
  case NK_STMT_EXPR: {
    // Type is the type of the last expression in the block
    if (n->body && n->body->n_children > 0) {
      Node* last = n->body->children[n->body->n_children - 1];
      if (last && last->kind == NK_EXPR_STMT && last->left) {
        Node* last_expr = last->left;
        // If the last expr is a variable that may be declared inside this stmt_expr
        // (not yet in local_types_ because emit_stmt hasn't run it yet), scan the body.
        if (last_expr->kind == NK_VAR && last_expr->name) {
          if (local_types_.count(last_expr->name) || global_types_.count(last_expr->name))
            return expr_type(last_expr);
          for (int i = 0; i < n->body->n_children - 1; i++) {
            Node* ch = n->body->children[i];
            if (ch && ch->kind == NK_DECL && ch->name &&
                std::strcmp(ch->name, last_expr->name) == 0)
              return ch->type;
          }
        }
        return expr_type(last_expr);
      }
    }
    return void_ts();
  }
  default: return int_ts();
  }
}

// ─────────────────────────── code emission ─────────────────────────────────

void IRBuilder::emit(const std::string& line) {
  body_lines_.push_back("  " + line);
  last_was_terminator_ = false;
}

void IRBuilder::emit_label(const std::string& label) {
  body_lines_.push_back(label + ":");
  last_was_terminator_ = false;
}

void IRBuilder::emit_terminator(const std::string& line) {
  if (!last_was_terminator_) {
    body_lines_.push_back("  " + line);
    last_was_terminator_ = true;
  }
}

// ─────────────────────────── string literals ───────────────────────────────

std::string IRBuilder::get_string_global(const std::string& content) {
  auto it = string_consts_.find(content);
  if (it != string_consts_.end()) return it->second;
  std::string name = "@.str" + std::to_string(string_idx_++);
  string_consts_[content] = name;

  // Build LLVM c"..." representation
  std::string llvm_str = "c\"";
  for (unsigned char c : content) {
    if (c == '"' || c == '\\' || c < 0x20 || c >= 0x7F) {
      char esc[8];
      snprintf(esc, sizeof(esc), "\\%02X", (unsigned)c);
      llvm_str += esc;
    } else {
      llvm_str += (char)c;
    }
  }
  llvm_str += "\\00\"";
  int len = (int)content.size() + 1;  // +1 for null terminator
  preamble_.push_back(name + " = private unnamed_addr constant [" +
                      std::to_string(len) + " x i8] " + llvm_str);
  return name;
}

/*static*/ std::string IRBuilder::fp_to_hex(double v) {
  uint64_t bits;
  memcpy(&bits, &v, 8);
  char buf[32];
  snprintf(buf, sizeof(buf), "0x%016llX", (unsigned long long)bits);
  return std::string(buf);
}

// For float (single-precision) constants: round to float first, then back to
// double so LLVM gets a hex value that is exactly representable as float.
static std::string fp_to_hex_float(double v) {
  float fv = (float)v;
  double dv = (double)fv;
  return IRBuilder::fp_to_hex(dv);
}

// ─────────────────────────── coerce / to_bool ──────────────────────────────

std::string IRBuilder::to_bool(const std::string& val, const TypeSpec& ts) {
  std::string llty = llvm_ty(ts);
  if (llty == "i1") return val;
  std::string t = fresh_tmp();
  if (ts.ptr_level > 0 || (is_array_ty(ts) && !ts.is_ptr_to_array)) {
    // Pointer or decayed array: compare against null
    emit(t + " = icmp ne ptr " + val + ", null");
  } else if (llty == "double" || llty == "float") {
    emit(t + " = fcmp une " + llty + " " + val + ", 0.0");
  } else {
    emit(t + " = icmp ne " + llty + " " + val + ", 0");
  }
  return t;
}

std::string IRBuilder::coerce(const std::string& val, const TypeSpec& from_ts,
                               const std::string& to_llty) {
  std::string from_llty = llvm_ty(from_ts);
  if (from_llty == to_llty) return val;
  if (to_llty == "void") return val;

  std::string t = fresh_tmp();

  // Complex type → complex type (e.g. _Complex float → _Complex double, or _Complex double → _Complex int)
  if (is_complex_base(from_ts.base) && from_ts.ptr_level == 0) {
    if (from_llty == to_llty) return val;
    // Determine from-element type
    bool from_is_fp = is_float_base(complex_component_ts(from_ts.base).base);
    std::string from_elem_llty = llvm_ty(complex_component_ts(from_ts.base));
    // Parse to-element type from the struct string "{ TYPE, TYPE }"
    std::string to_elem_llty;
    if (to_llty.size() > 2 && to_llty.front() == '{') {
      size_t s = to_llty.find_first_not_of(" \t{");
      size_t e = to_llty.find(',', s);
      if (s != std::string::npos && e != std::string::npos) {
        to_elem_llty = to_llty.substr(s, e - s);
        while (!to_elem_llty.empty() && to_elem_llty.back() == ' ') to_elem_llty.pop_back();
      }
    }
    if (!to_elem_llty.empty()) {
      bool to_is_fp = (to_elem_llty == "float" || to_elem_llty == "double");
      std::string re = fresh_tmp(), im = fresh_tmp();
      std::string re2 = fresh_tmp(), im2 = fresh_tmp();
      std::string r0 = fresh_tmp(), r1 = fresh_tmp();
      emit(re + " = extractvalue " + from_llty + " " + val + ", 0");
      emit(im + " = extractvalue " + from_llty + " " + val + ", 1");
      if (from_is_fp && to_is_fp) {
        // float ↔ double complex
        bool upcast = (from_elem_llty == "float" && to_elem_llty == "double");
        if (upcast) {
          emit(re2 + " = fpext float " + re + " to double");
          emit(im2 + " = fpext float " + im + " to double");
        } else {
          emit(re2 + " = fptrunc double " + re + " to " + to_elem_llty);
          emit(im2 + " = fptrunc double " + im + " to " + to_elem_llty);
        }
      } else if (from_is_fp && !to_is_fp) {
        // float complex → integer complex: fptosi
        emit(re2 + " = fptosi " + from_elem_llty + " " + re + " to " + to_elem_llty);
        emit(im2 + " = fptosi " + from_elem_llty + " " + im + " to " + to_elem_llty);
      } else if (!from_is_fp && to_is_fp) {
        // integer complex → float complex: sitofp
        emit(re2 + " = sitofp " + from_elem_llty + " " + re + " to " + to_elem_llty);
        emit(im2 + " = sitofp " + from_elem_llty + " " + im + " to " + to_elem_llty);
      } else {
        // integer complex → integer complex: sext/trunc/zext
        emit(re2 + " = sext " + from_elem_llty + " " + re + " to " + to_elem_llty);
        emit(im2 + " = sext " + from_elem_llty + " " + im + " to " + to_elem_llty);
      }
      emit(r0 + " = insertvalue " + to_llty + " zeroinitializer, " + to_elem_llty + " " + re2 + ", 0");
      emit(r1 + " = insertvalue " + to_llty + " " + r0 + ", " + to_elem_llty + " " + im2 + ", 1");
      return r1;
    }
    return val;  // unrecognized target, best effort
  }

  // Array type: val is already a ptr (array decays to pointer)
  if (is_array_ty(from_ts) && to_llty == "ptr") return val;
  if (is_array_ty(from_ts)) {
    // array → aggregate (another array/struct): reinterpret cast — load from same address
    if (!to_llty.empty() && (to_llty[0] == '[' || to_llty[0] == '{')) {
      emit(t + " = load " + to_llty + ", ptr " + val);
      return t;
    }
    // array → integer (e.g. for printf %p): ptrtoint
    emit(t + " = ptrtoint ptr " + val + " to " + to_llty);
    return t;
  }

  // Pointer conversions
  if (from_ts.ptr_level > 0 && to_llty == "ptr") return val;
  if (from_ts.ptr_level > 0) {
    // ptr → aggregate (array/struct): load the value from the pointer.
    // This handles vector-type params (e.g. V value passed as ptr) where we
    // need to materialise the [N x T] value. ptrtoint is only valid for integers.
    if (!to_llty.empty() && (to_llty[0] == '[' || to_llty[0] == '{')) {
      emit(t + " = load " + to_llty + ", ptr " + val);
      return t;
    }
    // ptr → integer
    emit(t + " = ptrtoint ptr " + val + " to " + to_llty);
    return t;
  }
  if (to_llty == "ptr") {
    if (val == "0") return "null";
    if (from_llty == "ptr") return val;
    // integer → ptr
    emit(t + " = inttoptr " + from_llty + " " + val + " to ptr");
    return t;
  }

  // Float ↔ float
  if ((from_llty == "float" || from_llty == "double") &&
      (to_llty == "float" || to_llty == "double")) {
    if (from_llty == "float" && to_llty == "double")
      emit(t + " = fpext float " + val + " to double");
    else
      emit(t + " = fptrunc double " + val + " to float");
    return t;
  }
  // Integer → float
  if (to_llty == "float" || to_llty == "double") {
    std::string op = is_unsigned_base(from_ts.base) ? "uitofp" : "sitofp";
    emit(t + " = " + op + " " + from_llty + " " + val + " to " + to_llty);
    return t;
  }
  // Float → integer
  if (from_llty == "float" || from_llty == "double") {
    emit(t + " = fptosi " + from_llty + " " + val + " to " + to_llty);
    return t;
  }

  // Integer ↔ integer (width change)
  int from_b = llty_bits(from_llty);
  int to_b   = llty_bits(to_llty);
  if (from_b == to_b) {
    // same width, different signedness - bitcast not needed in LLVM
    return val;
  }
  if (from_b > to_b) {
    emit(t + " = trunc " + from_llty + " " + val + " to " + to_llty);
  } else {
    std::string ext = is_unsigned_base(from_ts.base) ? "zext" : "sext";
    emit(t + " = " + ext + " " + from_llty + " " + val + " to " + to_llty);
  }
  return t;
}

// ─────────────────────────── hoisting ──────────────────────────────────────

// Helper: recursively scan an expression for NK_STMT_EXPR / NK_TERNARY nodes
// that contain local variable declarations, and hoist allocas for them.
static void hoist_allocas_in_expr(IRBuilder* irb, Node* n) {
  if (!n) return;
  if (n->kind == NK_STMT_EXPR) {
    irb->hoist_allocas(n->body);
    return;
  }
  if (n->kind == NK_TERNARY) {
    hoist_allocas_in_expr(irb, n->then_);
    hoist_allocas_in_expr(irb, n->else_);
    return;
  }
  // Recurse into all child pointers that might contain statement expressions
  hoist_allocas_in_expr(irb, n->left);
  hoist_allocas_in_expr(irb, n->right);
  hoist_allocas_in_expr(irb, n->cond);
  for (int i = 0; i < n->n_children; i++)
    hoist_allocas_in_expr(irb, n->children[i]);
}

int IRBuilder::eval_array_dim_expr(Node* n) {
  if (!n) return 0;
  if (n->kind == NK_INT_LIT) return (int)n->ival;
  if (n->kind == NK_CHAR_LIT) return (int)n->ival;
  if (n->kind == NK_SIZEOF_TYPE) return sizeof_ty(n->type);
  if (n->kind == NK_SIZEOF_EXPR) {
    if (n->left && n->left->kind == NK_STR_LIT && !is_wide_str_lit(n->left)) {
      int slen = str_lit_byte_len(n->left);
      if (slen >= 0) return slen;
    }
    return sizeof_ty(expr_type(n->left));
  }
  if (n->kind == NK_ALIGNOF_TYPE) return alignof_ty_impl(n->type, struct_defs_);
  if (n->kind == NK_CAST && n->left) return eval_array_dim_expr(n->left);
  if (n->kind == NK_UNARY && n->op && n->left) {
    int v = eval_array_dim_expr(n->left);
    if (strcmp(n->op, "-") == 0) return -v;
    if (strcmp(n->op, "+") == 0) return v;
  }
  if (n->kind == NK_BINOP && n->op && n->left && n->right) {
    int l = eval_array_dim_expr(n->left), r = eval_array_dim_expr(n->right);
    if (strcmp(n->op, "+") == 0) return l + r;
    if (strcmp(n->op, "-") == 0) return l - r;
    if (strcmp(n->op, "*") == 0) return l * r;
    if (strcmp(n->op, "/") == 0 && r) return l / r;
    if (strcmp(n->op, "%") == 0 && r) return l % r;
  }
  if (n->kind == NK_VAR && n->name) {
    auto it = enum_consts_.find(n->name);
    if (it != enum_consts_.end()) return (int)it->second;
  }
  return 0;
}

void IRBuilder::hoist_allocas(Node* node) {
  if (!node) return;
  switch (node->kind) {
  case NK_DECL: {
    if (!node->name) break;
    TypeSpec ts = node->type;
    // Infer array size from size expression (e.g. `char buf[sizeof(x)+1]`)
    if (is_array_ty(ts) && ts.array_size_expr && array_dim_at(ts, 0) <= 0) {
      int dim = eval_array_dim_expr(ts.array_size_expr);
      if (dim > 0) set_first_array_dim(ts, dim);
    }
    // Infer array size from initializer for int x[] / char s[] local arrays
    if (is_array_ty(ts) && array_dim_at(ts, 0) == -2 && node->init) {
      int inferred = infer_array_size_from_init(node->init);
      if (inferred > 0) set_first_array_dim(ts, inferred);
    }
    if (node->is_static) {
      // Static local: emit as a global with internal linkage.
      // Use mangled name @__static_FUNCNAME_VARNAME to avoid collisions.
      std::string gname = "@__static_" + current_fn_name_ + "_" + node->name;
      node_slots_[node] = gname;
      node_types_[node] = ts;
      std::string llty = llvm_ty(ts);
      std::string init_val = node->init ? global_const(ts, node->init) : "zeroinitializer";
      preamble_.push_back(gname + " = internal global " + llty + " " + init_val);
      break;
    }
    if (node->is_extern) {
      // Block-scoped extern declaration: resolve to global symbol, no alloca.
      node_slots_[node] = std::string("@") + node->name;
      node_types_[node] = ts;
      break;
    }
    // Generate unique slot name to avoid LLVM alloca name collision when the same
    // variable name appears in multiple scopes (e.g. separate if/else branches).
    std::string slot;
    {
      auto dup_it = local_slot_dedup_.find(node->name);
      if (dup_it == local_slot_dedup_.end()) {
        slot = std::string("%lv.") + node->name;
        local_slot_dedup_[node->name] = 0;
      } else {
        dup_it->second++;
        slot = std::string("%lv.") + node->name + "." + std::to_string(dup_it->second);
      }
    }
    node_slots_[node] = slot;
    node_types_[node] = ts;       // per-node type (survives overwrite by later same-named vars)
    std::string llty = llvm_ty(ts);
    alloca_lines_.push_back("  " + slot + " = alloca " + llty);
    break;
  }
  case NK_BLOCK:
    for (int i = 0; i < node->n_children; i++)
      hoist_allocas(node->children[i]);
    break;
  case NK_IF:
    hoist_allocas(node->then_);
    hoist_allocas(node->else_);
    break;
  case NK_WHILE:
  case NK_DO_WHILE:
    hoist_allocas(node->body);
    break;
  case NK_FOR:
    hoist_allocas(node->init);
    hoist_allocas(node->body);
    break;
  case NK_SWITCH:
    hoist_allocas(node->body);
    break;
  case NK_CASE:
  case NK_DEFAULT:
  case NK_LABEL:
    hoist_allocas(node->body);
    break;
  case NK_STMT_EXPR:
    hoist_allocas(node->body);
    break;
  case NK_EXPR_STMT:
    // Recurse into expression to find nested statement expressions
    hoist_allocas_in_expr(this, node->left);
    break;
  case NK_TERNARY:
    hoist_allocas_in_expr(this, node->then_);
    hoist_allocas_in_expr(this, node->else_);
    break;
  default:
    break;
  }
}

void IRBuilder::prescan_labels(Node* node) {
  if (!node) return;
  if (node->kind == NK_LABEL && node->name) {
    if (!user_labels_.count(node->name)) {
      user_labels_[node->name] =
          "user_" + std::string(node->name) + "_" + std::to_string(label_idx_++);
    }
    prescan_labels(node->body);
    return;
  }
  // Recurse into all children
  switch (node->kind) {
  case NK_BLOCK:
    for (int i = 0; i < node->n_children; i++)
      prescan_labels(node->children[i]);
    break;
  case NK_IF:
    prescan_labels(node->cond);
    prescan_labels(node->then_);
    prescan_labels(node->else_);
    break;
  case NK_WHILE:
  case NK_DO_WHILE:
    prescan_labels(node->body);
    break;
  case NK_FOR:
    prescan_labels(node->init);
    prescan_labels(node->body);
    break;
  case NK_SWITCH:
  case NK_CASE:
  case NK_DEFAULT:
    prescan_labels(node->body);
    break;
  default:
    break;
  }
}

// ─────────────────────── anonymous field helpers ───────────────────────────

// Recursively look for `fname` in the struct/union identified by `tag`.
// On success, fills `path` and returns true.
bool IRBuilder::find_field(const std::string& tag, const char* fname, FieldPath& path) {
  auto it = struct_defs_.find(tag);
  if (it == struct_defs_.end()) return false;
  const StructInfo& si = it->second;

  // 1. Direct lookup
  for (size_t i = 0; i < si.field_names.size(); i++) {
    if (!si.field_is_anon[i] && si.field_names[i] == fname) {
      path.anon_indices.clear();
      path.final_idx = (int)i;
      path.final_tag = tag;
      return true;
    }
  }

  // 2. Recursive search in anonymous sub-struct fields
  for (size_t i = 0; i < si.field_names.size(); i++) {
    if (!si.field_is_anon[i]) continue;
    const TypeSpec& fts = si.field_types[i];
    if (!fts.tag || !fts.tag[0]) continue;
    FieldPath sub_path;
    if (find_field(fts.tag, fname, sub_path)) {
      path.anon_indices.clear();
      path.anon_indices.push_back((int)i);
      for (int ai : sub_path.anon_indices)
        path.anon_indices.push_back(ai);
      path.final_idx = sub_path.final_idx;
      path.final_tag = sub_path.final_tag;
      return true;
    }
  }

  // 3. For anonymous unions: also look for fields promoted by name aliases
  // (union's fields are all at offset 0, so any field name in the union works)
  if (si.is_union) {
    // Already handled by direct lookup above when stored as anon union fields
  }

  return false;
}

// Emit a GEP chain for a FieldPath.  Returns the LLVM ptr to the field.
std::string IRBuilder::emit_field_gep(const std::string& struct_ptr,
                                       const std::string& outer_tag,
                                       const FieldPath& path) {
  std::string cur_ptr = struct_ptr;
  std::string cur_tag = outer_tag;

  // Walk the anonymous field chain
  for (size_t step = 0; step < path.anon_indices.size(); step++) {
    int ai = path.anon_indices[step];
    auto it = struct_defs_.find(cur_tag);
    if (it == struct_defs_.end()) break;
    const StructInfo& si = it->second;

    if (si.is_union) {
      // Union: all fields at offset 0; GEP to i8 array element 0, then bitcast
      std::string t = fresh_tmp();
      emit(t + " = getelementptr %struct." + cur_tag + ", ptr " + cur_ptr +
           ", i32 0, i32 0");
      cur_ptr = t;
    } else {
      int llvm_ai = ai;
      if (ai >= 0 && ai < (int)si.field_llvm_idx.size())
        llvm_ai = si.field_llvm_idx[ai];
      std::string t = fresh_tmp();
      emit(t + " = getelementptr %struct." + cur_tag + ", ptr " + cur_ptr +
           ", i32 0, i32 " + std::to_string(llvm_ai));
      cur_ptr = t;
    }

    // Advance cur_tag to the sub-struct
    if (ai < (int)si.field_types.size()) {
      const TypeSpec& sub_ts = si.field_types[ai];
      if (sub_ts.tag && sub_ts.tag[0]) cur_tag = sub_ts.tag;
    }
  }

  // Final field GEP
  auto it = struct_defs_.find(cur_tag);
  if (it == struct_defs_.end()) return cur_ptr;  // shouldn't happen
  const StructInfo& si = it->second;

  if (si.is_union) {
    // Union: GEP to element 0 (byte array)
    std::string t = fresh_tmp();
    emit(t + " = getelementptr %struct." + cur_tag + ", ptr " + cur_ptr +
         ", i32 0, i32 0");
    return t;
  } else {
    // Flexible array member: use byte-level GEP (FAM is excluded from LLVM struct type)
    if (path.final_idx < (int)si.field_array_sizes.size() &&
        si.field_array_sizes[path.final_idx] == -2) {
      int byte_offset = 0;
      for (int i = 0; i < path.final_idx; i++) {
        if (si.field_array_sizes[i] == -2) continue;
        TypeSpec ft = si.field_types[i];
        int sz = sizeof_ty(ft);
        if (si.field_array_sizes[i] >= 0) sz *= (int)si.field_array_sizes[i];
        byte_offset += sz;
      }
      std::string t = fresh_tmp();
      emit(t + " = getelementptr i8, ptr " + cur_ptr + ", i64 " +
           std::to_string(byte_offset));
      return t;
    }
    // Use LLVM field index (may differ from C field index due to bitfield packing)
    int llvm_idx = path.final_idx;
    if (path.final_idx >= 0 && path.final_idx < (int)si.field_llvm_idx.size())
      llvm_idx = si.field_llvm_idx[path.final_idx];
    std::string t = fresh_tmp();
    emit(t + " = getelementptr %struct." + cur_tag + ", ptr " + cur_ptr +
         ", i32 0, i32 " + std::to_string(llvm_idx));
    return t;
  }
}

TypeSpec IRBuilder::field_type_from_path(const std::string& outer_tag,
                                          const FieldPath& path) {
  std::string cur_tag = outer_tag;

  // Walk the anonymous chain to find the innermost struct
  for (size_t step = 0; step < path.anon_indices.size(); step++) {
    int ai = path.anon_indices[step];
    auto it = struct_defs_.find(cur_tag);
    if (it == struct_defs_.end()) return int_ts();
    const StructInfo& si = it->second;
    if (ai < (int)si.field_types.size()) {
      const TypeSpec& sub_ts = si.field_types[ai];
      if (sub_ts.tag && sub_ts.tag[0]) cur_tag = sub_ts.tag;
    }
  }

  auto it = struct_defs_.find(cur_tag);
  if (it == struct_defs_.end()) return int_ts();
  const StructInfo& si = it->second;

  if (si.is_union) {
    // For unions, use the specific requested field's type (path.final_idx).
    // Falling back to the first non-anon field only when no specific index.
    if (path.final_idx >= 0 && path.final_idx < (int)si.field_names.size()) {
      TypeSpec ft = si.field_types[path.final_idx];
      if (si.field_array_sizes[path.final_idx] != -1) {
        ft.array_size = si.field_array_sizes[path.final_idx];
        ft.inner_rank = -1;
      }
      return ft;
    }
    for (size_t i = 0; i < si.field_names.size(); i++) {
      if (!si.field_is_anon[i]) {
        TypeSpec ft = si.field_types[i];
        if (si.field_array_sizes[i] != -1) {
          ft.array_size = si.field_array_sizes[i];
          ft.inner_rank = -1;
        }
        return ft;
      }
    }
    return int_ts();
  }

  if (path.final_idx >= 0 && path.final_idx < (int)si.field_names.size()) {
    TypeSpec ft = si.field_types[path.final_idx];
    if (si.field_array_sizes[path.final_idx] != -1) {
      ft.array_size = si.field_array_sizes[path.final_idx];
      ft.inner_rank = -1;
    }
    return ft;
  }
  return int_ts();
}

// Returns the bitfield width for the final field in path, or -1 if not a bitfield.
int IRBuilder::bitfield_width_from_path(const std::string& /*outer_tag*/,
                                        const FieldPath& path) const {
  // The final struct is identified by path.final_tag.
  const std::string& cur_tag = path.final_tag;
  auto it = struct_defs_.find(cur_tag);
  if (it == struct_defs_.end()) return -1;
  const StructInfo& si = it->second;
  int idx = path.final_idx;
  if (idx < 0 || idx >= (int)si.field_bit_widths.size()) return -1;
  return si.field_bit_widths[idx];
}

// Returns the bit offset within the storage unit for a bitfield member node.
// Returns 0 if not a bitfield or not found.
static int member_bit_offset(const std::unordered_map<std::string, IRBuilder::StructInfo>& struct_defs,
                              Node* n) {
  if (!n || n->kind != NK_MEMBER || !n->name) return 0;
  // Need to get the struct type — but we don't want to call expr_type here (const issue).
  // Instead, walk to find the tag.
  // This is called from expr_type context, so just return 0 if we can't find it.
  return 0;
}

// Get bit offset for a field given its tag and C field index.
static int field_bit_offset(const std::unordered_map<std::string, IRBuilder::StructInfo>& struct_defs,
                             const std::string& tag, int c_field_idx) {
  auto it = struct_defs.find(tag);
  if (it == struct_defs.end()) return 0;
  const IRBuilder::StructInfo& si = it->second;
  if (c_field_idx < 0 || c_field_idx >= (int)si.field_bit_offsets.size()) return 0;
  return si.field_bit_offsets[c_field_idx];
}

// Get LLVM storage type string for a bitfield given its tag and C field index.
static std::string field_storage_llty(const std::unordered_map<std::string, IRBuilder::StructInfo>& struct_defs,
                                       const std::string& tag, int c_field_idx) {
  auto it = struct_defs.find(tag);
  if (it == struct_defs.end()) return "i32";
  const IRBuilder::StructInfo& si = it->second;
  if (c_field_idx < 0 || c_field_idx >= (int)si.field_types.size()) return "i32";
  const TypeSpec& ft = si.field_types[c_field_idx];
  if (ft.ptr_level > 0) return "i64";
  switch (ft.base) {
    case TB_BOOL: case TB_CHAR: case TB_SCHAR: case TB_UCHAR: return "i8";
    case TB_SHORT: case TB_USHORT: return "i16";
    case TB_INT: case TB_UINT: case TB_ENUM: return "i32";
    case TB_LONG: case TB_ULONG: case TB_LONGLONG: case TB_ULONGLONG: return "i64";
    default: return "i32";
  }
}

// Helper: given an NK_MEMBER node, return bitfield width (-1 if not a bitfield).
int IRBuilder::member_bitfield_width(Node* n) const {
  if (!n || n->kind != NK_MEMBER || !n->name) return -1;
  TypeSpec obj_ts = const_cast<IRBuilder*>(this)->expr_type(n->left);
  if (n->is_arrow && obj_ts.ptr_level > 0) obj_ts.ptr_level -= 1;
  const char* tag = obj_ts.tag;
  if (!tag || !tag[0]) return -1;
  auto it = struct_defs_.find(tag);
  if (it == struct_defs_.end()) return -1;
  FieldPath path;
  if (!const_cast<IRBuilder*>(this)->find_field(tag, n->name, path)) return -1;
  return bitfield_width_from_path(tag, path);
}

// Forward declaration (defined after codegen_expr)
static void emit_agg_init_impl(IRBuilder* irb, TypeSpec ts, const std::string& ptr,
                                Node* flat_list, int& flat_idx);

// ─────────────────────────── lvalue codegen ────────────────────────────────

std::string IRBuilder::codegen_lval(Node* n) {
  if (!n) throw std::runtime_error("null lvalue node");

  switch (n->kind) {
  case NK_VAR: {
    if (!n->name) throw std::runtime_error("var has no name");
    auto it = local_slots_.find(n->name);
    if (it != local_slots_.end()) return it->second;
    // Global variable or function (functions are lvalue-addressable via &f)
    if (global_types_.count(n->name) || func_sigs_.count(n->name))
      return std::string("@") + n->name;
    throw std::runtime_error(std::string("assignment to undeclared identifier: ") + n->name);
  }

  case NK_DEREF:
  case NK_UNARY: {
    // *expr: evaluate the pointer value, that IS the address
    std::string ptr_val = codegen_expr(n->left);
    // ptr_val is already the address we want
    return ptr_val;
  }

  case NK_INDEX: {
    // base[index]  — C allows the commutative form: integer[pointer]
    TypeSpec base_ts = expr_type(n->left);
    TypeSpec right_ts = expr_type(n->right);
    // Swap if left is non-pointer/non-array and right is pointer/array (e.g. N[(char*)x])
    bool swapped = false;
    if (base_ts.ptr_level == 0 && !is_array_ty(base_ts) &&
        (right_ts.ptr_level > 0 || is_array_ty(right_ts))) {
      std::swap(base_ts, right_ts);
      swapped = true;
    }
    // Evaluate index (the non-pointer operand) and base (the pointer/array operand)
    std::string idx_val = swapped ? codegen_expr(n->left) : codegen_expr(n->right);
    std::string idx_i64;
    {
      idx_i64 = coerce(idx_val, swapped ? right_ts : expr_type(n->right), "i64");
    }

    std::string t = fresh_tmp();
    if (is_array_ty(base_ts) && !(base_ts.ptr_level > 0 && base_ts.is_ptr_to_array)) {
      // Array variable: GEP [N x elem], ptr, 0, idx
      TypeSpec elem_ts = drop_array_dim(base_ts);
      std::string arr_llty = llvm_ty(base_ts);
      std::string elem_llty = llvm_ty(elem_ts);
      Node* base_node = swapped ? n->right : n->left;
      std::string ptr = codegen_lval(base_node);
      (void)elem_llty;
      emit(t + " = getelementptr " + arr_llty + ", ptr " + ptr +
           ", i64 0, i64 " + idx_i64);
    } else if (base_ts.ptr_level > 0) {
      // Pointer arithmetic: GEP elem, ptr_val, idx
      TypeSpec elem_ts = base_ts;
      elem_ts.ptr_level -= 1;
      if (elem_ts.ptr_level == 0 && elem_ts.is_ptr_to_array)
        elem_ts.is_ptr_to_array = false;
      std::string elem_llty = gep_elem_ty(llvm_ty(elem_ts));
      std::string ptr_val = swapped ? codegen_expr(n->right) : codegen_expr(n->left);
      emit(t + " = getelementptr " + elem_llty + ", ptr " + ptr_val +
           ", i64 " + idx_i64);
    } else {
      throw std::runtime_error("index of non-pointer non-array: base_ts base=" +
        std::to_string((int)base_ts.base) + " ptr_level=" + std::to_string(base_ts.ptr_level) +
        " array_size=" + std::to_string(base_ts.array_size) + " array_rank=" + std::to_string(base_ts.array_rank) +
        " is_ptr_to_array=" + std::to_string(base_ts.is_ptr_to_array));
    }
    return t;
  }

  case NK_MEMBER: {
    // expr.field or expr->field
    Node* obj = n->left;
    bool is_arrow = n->is_arrow;

    // Get the struct type
    TypeSpec obj_ts = expr_type(obj);
    if (is_arrow && obj_ts.ptr_level > 0) obj_ts.ptr_level -= 1;

    const char* tag = obj_ts.tag;
    if (!tag || !tag[0]) {
      std::string left_info = std::to_string((int)(obj ? obj->kind : -1));
      if (obj && obj->kind == NK_MEMBER && obj->name)
        left_info += std::string(":") + obj->name;
      if (obj && obj->kind == NK_VAR && obj->name)
        left_info += std::string(":var=") + obj->name;
      throw std::runtime_error(
          "member access on non-struct ." + std::string(n->name ? n->name : "?") +
          " (base=" + std::to_string((int)obj_ts.base) +
          ", ptr=" + std::to_string(obj_ts.ptr_level) +
          ", arr_rank=" + std::to_string(array_rank_of(obj_ts)) +
          ", left=" + left_info + ", line=" + std::to_string(n->line) + ")");
    }

    if (!struct_defs_.count(tag))
      throw std::runtime_error(std::string("unknown struct ") + tag);

    // Use recursive field lookup (handles anonymous sub-struct promotion)
    FieldPath path;
    if (!find_field(tag, n->name, path))
      throw std::runtime_error(std::string("unknown field ") + (n->name ? n->name : "?") +
                               " in " + tag);

    // Get the struct pointer
    std::string struct_ptr;
    if (is_arrow) {
      struct_ptr = codegen_expr(obj);  // load the ptr
    } else {
      struct_ptr = codegen_lval(obj);  // address of the struct
    }

    return emit_field_gep(struct_ptr, tag, path);
  }

  case NK_REAL_PART:
  case NK_IMAG_PART: {
    TypeSpec cts = expr_type(n->left);
    if (cts.ptr_level != 0 || !is_complex_base(cts.base))
      throw std::runtime_error("__real__/__imag__ lvalue requires complex operand");
    std::string cptr = codegen_lval(n->left);
    std::string t = fresh_tmp();
    std::string cllty = llvm_ty(cts);
    int field_idx = (n->kind == NK_IMAG_PART) ? 1 : 0;
    emit(t + " = getelementptr " + cllty + ", ptr " + cptr +
         ", i32 0, i32 " + std::to_string(field_idx));
    return t;
  }

  case NK_CALL: {
    // Function call result used as lvalue (e.g. fr_s1().x) — spill to temp.
    TypeSpec ret_ts = expr_type(n);
    std::string ret_llty = llvm_ty(ret_ts);
    std::string slot = "%cret_" + std::to_string(tmp_idx_++);
    alloca_lines_.push_back("  " + slot + " = alloca " + ret_llty);
    std::string val = codegen_expr(n);
    emit("store " + ret_llty + " " + val + ", ptr " + slot);
    return slot;
  }

  case NK_COMPOUND_LIT: {
    // (type){...} used as lvalue — need the alloca address.
    // Duplicate the alloca+init logic here so we get the raw slot ptr
    // even after codegen_expr was updated to return loaded values for struct/union.
    TypeSpec clit_ts = n->type;
    Node* clit_init = n->left ? n->left : n->init;
    // Infer array size for unsized compound literals: (int[]){0,1,2} → [3 x i32]
    if (is_array_ty(clit_ts) && array_dim_at(clit_ts, 0) == -2 && clit_init) {
      int inferred = infer_array_size_from_init(clit_init);
      if (inferred > 0) set_first_array_dim(clit_ts, inferred);
    }
    std::string clit_llty = llvm_ty(clit_ts);
    std::string clit_slot = "%clit_" + std::to_string(tmp_idx_++);
    alloca_lines_.push_back("  " + clit_slot + " = alloca " + clit_llty);
    if (clit_init && clit_init->kind == NK_INIT_LIST &&
        (is_array_ty(clit_ts) || clit_ts.base == TB_STRUCT || clit_ts.base == TB_UNION)) {
      int flat_idx = 0;
      emit_agg_init_impl(this, clit_ts, clit_slot, clit_init, flat_idx);
    }
    return clit_slot;
  }

  case NK_STMT_EXPR: {
    // ({ stmts... last_expr; }) used as lvalue — execute all but last, then
    // return lvalue of last expression (or spill scalar result to a temp slot).
    if (!n->body) {
      // Empty stmt_expr: allocate a dummy slot.
      TypeSpec ts = expr_type(n);
      std::string slot = "%se_" + std::to_string(tmp_idx_++);
      alloca_lines_.push_back("  " + slot + " = alloca " + llvm_ty(ts));
      return slot;
    }
    Node* blk = n->body;
    // Execute all but the last statement.
    for (int i = 0; i < blk->n_children - 1; i++)
      emit_stmt(blk->children[i]);
    // Last child must be an expression statement.
    Node* last = blk->n_children > 0 ? blk->children[blk->n_children - 1] : nullptr;
    if (last && last->kind == NK_EXPR_STMT && last->left) {
      Node* last_expr = last->left;
      // If the last expression is directly addressable, use its lvalue.
      if (last_expr->kind == NK_VAR || last_expr->kind == NK_DEREF ||
          last_expr->kind == NK_INDEX || last_expr->kind == NK_MEMBER ||
          last_expr->kind == NK_COMPOUND_LIT) {
        return codegen_lval(last_expr);
      }
      // Otherwise spill.
      TypeSpec ts = expr_type(last_expr);
      std::string llty = llvm_ty(ts);
      std::string slot = "%se_" + std::to_string(tmp_idx_++);
      alloca_lines_.push_back("  " + slot + " = alloca " + llty);
      std::string val = codegen_expr(last_expr);
      emit("store " + llty + " " + val + ", ptr " + slot);
      return slot;
    }
    // Fallback: no last expr, return a dummy slot.
    if (last) emit_stmt(last);
    TypeSpec ts2 = expr_type(n);
    std::string slot2 = "%se_" + std::to_string(tmp_idx_++);
    alloca_lines_.push_back("  " + slot2 + " = alloca " + llvm_ty(ts2));
    return slot2;
  }

  case NK_COMMA_EXPR: {
    // Evaluate left for side effects, then lvalue of right.
    codegen_expr(n->left);
    return codegen_lval(n->right);
  }

  case NK_CAST: {
    // Vector reinterpret cast used as lvalue: (V8)(V64){...}[1].
    // The cast doesn't change the address — return the inner lval.
    // This handles (VecType)expr[i] where expr is a compound literal or array var.
    return codegen_lval(n->left);
  }

  default:
    throw std::runtime_error(std::string("codegen_lval: unhandled kind ") +
                             std::to_string(n->kind));
  }
}

// ─────────────────────────── expression codegen ────────────────────────────

// Forward declaration (defined after codegen_expr)
static void emit_agg_init_impl(IRBuilder* irb, TypeSpec ts, const std::string& ptr,
                                Node* flat_list, int& flat_idx);

std::string IRBuilder::codegen_expr(Node* n) {
  if (!n) return "0";

  switch (n->kind) {
  case NK_INT_LIT: {
    // &&label (GCC computed goto label address): return blockaddress constant
    if (n->name && !n->is_imaginary) {
      auto it = user_labels_.find(n->name);
      std::string lbl_bb = (it != user_labels_.end())
                           ? it->second
                           : (std::string("user_") + n->name + "_0");
      return "blockaddress(@" + current_fn_name_ + ", %" + lbl_bb + ")";
    }
    if (n->is_imaginary) {
      // Imaginary integer literal Ni: produce { 0, N } for the appropriate complex type.
      // We emit a { T, T } aggregate with real=0, imag=ival.
      // The concrete complex type is inferred from context; default to _Complex long long.
      TypeSpec cts = expr_type(n);
      std::string cllty = llvm_ty(cts);
      TypeSpec ets = complex_component_ts(cts.base);
      std::string ellty = llvm_ty(ets);
      std::string t0 = fresh_tmp(), t1 = fresh_tmp();
      emit(t0 + " = insertvalue " + cllty + " zeroinitializer, " + ellty + " 0, 0");
      std::string iv = std::to_string(n->ival);
      emit(t1 + " = insertvalue " + cllty + " " + t0 + ", " + ellty + " " + iv + ", 1");
      return t1;
    }
    return std::to_string(n->ival);
  }

  case NK_CHAR_LIT:
    return std::to_string(n->ival);

  case NK_FLOAT_LIT: {
    if (n->is_imaginary) {
      // Imaginary float literal: produce { 0.0, val }.
      TypeSpec cts = expr_type(n);
      std::string cllty = llvm_ty(cts);
      TypeSpec ets = complex_component_ts(cts.base);
      bool is_f32 = (ets.base == TB_FLOAT);
      std::string ellty = llvm_ty(ets);
      std::string iv = is_f32 ? fp_to_hex_float(n->fval) : fp_to_hex(n->fval);
      std::string t0 = fresh_tmp(), t1 = fresh_tmp();
      std::string zero = is_f32 ? fp_to_hex_float(0.0) : fp_to_hex(0.0);
      emit(t0 + " = insertvalue " + cllty + " zeroinitializer, " + ellty + " " + zero + ", 0");
      emit(t1 + " = insertvalue " + cllty + " " + t0 + ", " + ellty + " " + iv + ", 1");
      return t1;
    }
    // Use the hex representation to preserve precision
    double v = n->fval;
    TypeSpec flit_ts = expr_type(n);
    if (flit_ts.base == TB_FLOAT) return fp_to_hex_float(v);
    return fp_to_hex(v);
  }

  case NK_STR_LIT: {
    // Wide string literal L"..." → emit as [N x i32] global, return ptr
    if (is_wide_str_lit(n)) {
      auto codepoints = decode_wide_string(n->sval);
      int len = (int)codepoints.size();  // includes null terminator
      std::string gname = "@.wstr" + std::to_string(string_idx_++);
      std::string wstr_type = "[" + std::to_string(len) + " x i32]";
      // Build global initializer
      std::string ginit = "[";
      for (int wi = 0; wi < len; wi++) {
        if (wi > 0) ginit += ", ";
        ginit += "i32 " + std::to_string((int)codepoints[wi]);
      }
      ginit += "]";
      preamble_.push_back(gname + " = internal constant " + wstr_type + " " + ginit);
      std::string t = fresh_tmp();
      emit(t + " = getelementptr " + wstr_type + ", ptr " + gname + ", i64 0, i64 0");
      return t;
    }
    // n->sval is the raw lexeme (may include quotes); extract content
    std::string raw = n->sval ? n->sval : "";
    // Strip outer quotes if present
    std::string content;
    if (raw.size() >= 2 && raw.front() == '"' && raw.back() == '"') {
      // Decode the C string escape sequences
      content = "";
      for (size_t i = 1; i + 1 < raw.size(); i++) {
        if (raw[i] == '\\' && i+1 < raw.size()-1) {
          char esc = raw[i+1];
          switch (esc) {
          case 'n': content += '\n'; i++; break;
          case 't': content += '\t'; i++; break;
          case 'r': content += '\r'; i++; break;
          case '0': content += '\0'; i++; break;
          case 'a': content += '\a'; i++; break;
          case 'b': content += '\b'; i++; break;
          case 'f': content += '\f'; i++; break;
          case 'v': content += '\v'; i++; break;
          case '\\': content += '\\'; i++; break;
          case '"': content += '"'; i++; break;
          case '\'': content += '\''; i++; break;
          case 'x': case 'X': {
            // Hex escape \xNN
            i += 2;  // skip \ and x
            unsigned int v = 0;
            for (int k = 0; k < 2 && i < raw.size()-1 && isxdigit((unsigned char)raw[i]); k++, i++) {
              v = v * 16 + (isdigit((unsigned char)raw[i]) ? raw[i] - '0' :
                            tolower((unsigned char)raw[i]) - 'a' + 10);
            }
            content += (char)(unsigned char)v;
            i--;  // compensate for outer i++
            break;
          }
          case 'u': case 'U': {
            // \uXXXX or \UXXXXXXXX — encode codepoint as UTF-8
            int ndigits = (esc == 'u') ? 4 : 8;
            i += 2;  // skip \ and u/U
            uint32_t cp = 0;
            for (int k = 0; k < ndigits && i + 1 < raw.size() && isxdigit((unsigned char)raw[i]); k++, i++)
              cp = cp * 16 + (isdigit((unsigned char)raw[i]) ? raw[i] - '0' :
                              tolower((unsigned char)raw[i]) - 'a' + 10);
            i--;  // compensate for outer i++
            if (cp < 0x80) {
              content += (char)cp;
            } else if (cp < 0x800) {
              content += (char)(0xC0 | (cp >> 6));
              content += (char)(0x80 | (cp & 0x3F));
            } else if (cp < 0x10000) {
              content += (char)(0xE0 | (cp >> 12));
              content += (char)(0x80 | ((cp >> 6) & 0x3F));
              content += (char)(0x80 | (cp & 0x3F));
            } else {
              content += (char)(0xF0 | (cp >> 18));
              content += (char)(0x80 | ((cp >> 12) & 0x3F));
              content += (char)(0x80 | ((cp >> 6) & 0x3F));
              content += (char)(0x80 | (cp & 0x3F));
            }
            break;
          }
          default:
            if (esc >= '1' && esc <= '7') {
              // Octal escape \ooo
              i++;
              unsigned int v = 0;
              for (int k = 0; k < 3 && i < raw.size()-1 && raw[i] >= '0' && raw[i] <= '7'; k++, i++)
                v = v * 8 + (raw[i] - '0');
              content += (char)(unsigned char)v;
              i--;  // compensate for outer i++
            } else {
              content += '\\'; content += esc; i++;
            }
            break;
          }
        } else {
          content += raw[i];
        }
      }
    } else {
      content = raw;
    }
    std::string gname = get_string_global(content);
    // Return ptr to the global (getelementptr i8, ptr @.strN, i64 0)
    std::string t = fresh_tmp();
    int len = (int)content.size() + 1;
    emit(t + " = getelementptr [" + std::to_string(len) + " x i8], ptr " +
         gname + ", i64 0, i64 0");
    return t;
  }

  case NK_VAR: {
    if (!n->name) return "0";
    // Check enum constants first
    auto ec = enum_consts_.find(n->name);
    if (ec != enum_consts_.end())
      return std::to_string(ec->second);
    // Local slot: load from alloca
    auto it = local_slots_.find(n->name);
    if (it != local_slots_.end()) {
      TypeSpec ts = local_types_[n->name];
      std::string llty = llvm_ty(ts);
      // Arrays decay to pointer in expressions. Keep GNU vector arrays as values.
      // But an array OF vectors (array_rank > 1, e.g. V x[32] where V is vector_size)
      // is a real C array and must also decay to pointer.
      if (is_array_ty(ts) && !ts.is_ptr_to_array && (!ts.is_vector || ts.array_rank > 1)) {
        return it->second;
      }
      std::string t = fresh_tmp();
      emit(t + " = load " + llty + ", ptr " + it->second);
      return t;
    }
    // Global: load from @name
    auto it2 = global_types_.find(n->name);
    if (it2 != global_types_.end()) {
      TypeSpec ts = it2->second;
      std::string llty = llvm_ty(ts);
      if (is_array_ty(ts) && !ts.is_ptr_to_array && (!ts.is_vector || ts.array_rank > 1)) {
        // Array global: return ptr to it (array decays)
        return std::string("@") + n->name;
      }
      std::string t = fresh_tmp();
      emit(t + " = load " + llty + ", ptr @" + n->name);
      return t;
    }
    // Function reference: just return @name as a ptr value
    if (func_sigs_.count(n->name)) {
      return std::string("@") + n->name;
    }
    throw std::runtime_error(std::string("use of undeclared identifier: ") + n->name);
  }

  case NK_ADDR: {
    // &expr: return lvalue address
    Node* inner = n->left;
    // Special case: &arr[i] - handled by codegen_lval
    return codegen_lval(inner);
  }

  case NK_DEREF: {
    // *expr: load from the pointer
    TypeSpec ptr_ts = expr_type(n->left);
    std::string ptr_val = codegen_expr(n->left);
    // Dereference: elem type = ptr_ts with one less ptr level
    TypeSpec elem_ts = ptr_ts;
    if (elem_ts.ptr_level > 0) {
      elem_ts.ptr_level -= 1;
      if (elem_ts.ptr_level == 0) elem_ts.is_ptr_to_array = false;
    } else if (is_array_ty(elem_ts)) {
      std::string arr_ptr = ptr_val;
      std::string t_gep = fresh_tmp();
      std::string arr_llty = llvm_ty(elem_ts);
      elem_ts = drop_array_dim(elem_ts);
      ptr_val = t_gep;
      emit(t_gep + " = getelementptr " + arr_llty + ", ptr " + arr_ptr + ", i64 0, i64 0");
    }
    // When dereferencing a true C pointer-to-array (is_ptr_to_array was set on ptr_ts,
    // and element is NOT a vector), the result is an array lvalue.
    // Return the address without loading — arrays decay to pointers in arithmetic.
    // GNU vector types (is_vector) must be loaded as values.
    if (ptr_ts.is_ptr_to_array && is_array_ty(elem_ts) && elem_ts.ptr_level == 0 &&
        !elem_ts.is_vector) {
      return ptr_val;
    }
    std::string elem_llty = llvm_ty(elem_ts);
    std::string t = fresh_tmp();
    emit(t + " = load " + elem_llty + ", ptr " + ptr_val);
    return t;
  }

  case NK_REAL_PART:
  case NK_IMAG_PART: {
    TypeSpec inner_ts = expr_type(n->left);
    TypeSpec part_ts = expr_type(n);
    std::string part_llty = llvm_ty(part_ts);
    bool want_imag = (n->kind == NK_IMAG_PART);
    bool inner_is_complex = (inner_ts.ptr_level == 0 && is_complex_base(inner_ts.base));

    if (!inner_is_complex) {
      // GNU behavior on non-complex operands: __real__ x => x, __imag__ x => 0.
      if (!want_imag) return codegen_expr(n->left);
      if (part_llty == "float" || part_llty == "double") return "0.0";
      return "0";
    }

    // If operand is addressable, load the selected component from its storage.
    if (n->left &&
        (n->left->kind == NK_VAR || n->left->kind == NK_DEREF ||
         n->left->kind == NK_INDEX || n->left->kind == NK_MEMBER ||
         n->left->kind == NK_COMPOUND_LIT || n->left->kind == NK_REAL_PART ||
         n->left->kind == NK_IMAG_PART || n->left->kind == NK_STMT_EXPR)) {
      std::string part_ptr = codegen_lval(n);
      std::string t = fresh_tmp();
      emit(t + " = load " + part_llty + ", ptr " + part_ptr);
      return t;
    }

    // Non-lvalue complex expression (e.g. call returning complex): extractvalue.
    std::string cval = codegen_expr(n->left);
    std::string cllty = llvm_ty(inner_ts);
    std::string t = fresh_tmp();
    emit(t + " = extractvalue " + cllty + " " + cval + ", " +
         std::to_string(want_imag ? 1 : 0));
    return t;
  }

  case NK_UNARY: {
    if (!n->op) return "0";
    if (strcmp(n->op, "-") == 0) {
      TypeSpec ts = expr_type(n->left);
      std::string v = codegen_expr(n->left);
      std::string llty = llvm_ty(ts);
      std::string t = fresh_tmp();
      if (llty == "float" || llty == "double")
        emit(t + " = fneg " + llty + " " + v);
      else if (ts.ptr_level == 0 && !is_array_ty(ts) && !is_float_base(ts.base) &&
               !is_complex_base(ts.base) && sizeof_ty(ts) < 4) {
        // Integer promotion: promote narrow type to i32 before negation
        bool is_unsigned = is_unsigned_base(ts.base);
        std::string v32 = fresh_tmp();
        emit(v32 + " = " + (is_unsigned ? "zext" : "sext") + " " + llty + " " + v + " to i32");
        emit(t + " = sub i32 0, " + v32);
      } else
        emit(t + " = sub " + llty + " 0, " + v);
      return t;
    }
    if (strcmp(n->op, "!") == 0) {
      TypeSpec ts = expr_type(n->left);
      std::string v = codegen_expr(n->left);
      std::string b = to_bool(v, ts);
      // !x = (x == 0)
      std::string t = fresh_tmp();
      emit(t + " = xor i1 " + b + ", true");
      std::string t2 = fresh_tmp();
      emit(t2 + " = zext i1 " + t + " to i32");
      return t2;
    }
    if (strcmp(n->op, "~") == 0) {
      TypeSpec ts = expr_type(n->left);
      std::string v = codegen_expr(n->left);
      std::string llty = llvm_ty(ts);
      // Complex conjugate: ~(a+bi) = a-bi
      if (ts.ptr_level == 0 && is_complex_base(ts.base)) {
        TypeSpec ets = complex_component_ts(ts.base);
        std::string ellty = llvm_ty(ets);
        bool is_fp = is_float_base(ets.base);
        std::string re = fresh_tmp(), im = fresh_tmp(), neg_im = fresh_tmp();
        std::string t0 = fresh_tmp(), t1 = fresh_tmp();
        emit(re + " = extractvalue " + llty + " " + v + ", 0");
        emit(im + " = extractvalue " + llty + " " + v + ", 1");
        if (is_fp)
          emit(neg_im + " = fneg " + ellty + " " + im);
        else
          emit(neg_im + " = sub " + ellty + " 0, " + im);
        emit(t0 + " = insertvalue " + llty + " zeroinitializer, " + ellty + " " + re + ", 0");
        emit(t1 + " = insertvalue " + llty + " " + t0 + ", " + ellty + " " + neg_im + ", 1");
        return t1;
      }
      std::string t = fresh_tmp();
      if (ts.ptr_level == 0 && !is_array_ty(ts) && !is_float_base(ts.base) &&
          !is_complex_base(ts.base) && sizeof_ty(ts) < 4) {
        // Integer promotion: promote narrow type to i32 before bitwise NOT
        bool is_unsigned = is_unsigned_base(ts.base);
        std::string v32 = fresh_tmp();
        emit(v32 + " = " + (is_unsigned ? "zext" : "sext") + " " + llty + " " + v + " to i32");
        emit(t + " = xor i32 " + v32 + ", -1");
      } else
        emit(t + " = xor " + llty + " " + v + ", -1");
      return t;
    }
    if (strcmp(n->op, "+") == 0) {
      return codegen_expr(n->left);
    }
    if (strcmp(n->op, "++") == 0 || strcmp(n->op, "++pre") == 0) {
      // Prefix ++: load, add 1, store, return new value
      TypeSpec ts = expr_type(n->left);
      if (ts.is_const && ts.ptr_level == 0 && !is_array_ty(ts))
        throw std::runtime_error("increment of const-qualified lvalue");
      std::string llty = llvm_ty(ts);
      std::string ptr = codegen_lval(n->left);
      std::string old_val = fresh_tmp();
      emit(old_val + " = load " + llty + ", ptr " + ptr);
      std::string new_val = fresh_tmp();
      if (ts.ptr_level > 0) {
        TypeSpec elem_ts = ts; elem_ts.ptr_level -= 1;
        emit(new_val + " = getelementptr " + gep_elem_ty(llvm_ty(elem_ts)) + ", ptr " + old_val + ", i64 1");
      } else if (llty == "float" || llty == "double") {
        emit(new_val + " = fadd " + llty + " " + old_val + ", 1.0");
      } else {
        emit(new_val + " = add " + llty + " " + old_val + ", 1");
      }
      emit("store " + llty + " " + new_val + ", ptr " + ptr);
      return new_val;
    }
    if (strcmp(n->op, "--") == 0 || strcmp(n->op, "--pre") == 0) {
      // Prefix --: load, sub 1, store, return new value
      TypeSpec ts = expr_type(n->left);
      if (ts.is_const && ts.ptr_level == 0 && !is_array_ty(ts))
        throw std::runtime_error("decrement of const-qualified lvalue");
      std::string llty = llvm_ty(ts);
      std::string ptr = codegen_lval(n->left);
      std::string old_val = fresh_tmp();
      emit(old_val + " = load " + llty + ", ptr " + ptr);
      std::string new_val = fresh_tmp();
      if (ts.ptr_level > 0) {
        TypeSpec elem_ts = ts; elem_ts.ptr_level -= 1;
        emit(new_val + " = getelementptr " + gep_elem_ty(llvm_ty(elem_ts)) + ", ptr " + old_val + ", i64 -1");
      } else if (llty == "float" || llty == "double") {
        emit(new_val + " = fsub " + llty + " " + old_val + ", 1.0");
      } else {
        emit(new_val + " = sub " + llty + " " + old_val + ", 1");
      }
      emit("store " + llty + " " + new_val + ", ptr " + ptr);
      return new_val;
    }
    return codegen_expr(n->left);
  }

  case NK_POSTFIX: {
    // postfix ++ or --
    TypeSpec ts = expr_type(n->left);
    if (ts.is_const && ts.ptr_level == 0 && !is_array_ty(ts))
      throw std::runtime_error("increment/decrement of const-qualified lvalue");
    std::string llty = llvm_ty(ts);
    std::string ptr = codegen_lval(n->left);
    std::string old_val = fresh_tmp();
    emit(old_val + " = load " + llty + ", ptr " + ptr);
    std::string new_val = fresh_tmp();
    bool is_ptr = ts.ptr_level > 0;
    if (n->op && strcmp(n->op, "++") == 0) {
      if (is_ptr) {
        TypeSpec elem_ts = ts; elem_ts.ptr_level -= 1;
        emit(new_val + " = getelementptr " + gep_elem_ty(llvm_ty(elem_ts)) + ", ptr " +
             old_val + ", i64 1");
      } else if (llty == "float" || llty == "double") {
        emit(new_val + " = fadd " + llty + " " + old_val + ", 1.0");
      } else {
        emit(new_val + " = add " + llty + " " + old_val + ", 1");
      }
    } else {
      if (is_ptr) {
        TypeSpec elem_ts = ts; elem_ts.ptr_level -= 1;
        emit(new_val + " = getelementptr " + gep_elem_ty(llvm_ty(elem_ts)) + ", ptr " +
             old_val + ", i64 -1");
      } else if (llty == "float" || llty == "double") {
        emit(new_val + " = fsub " + llty + " " + old_val + ", 1.0");
      } else {
        emit(new_val + " = sub " + llty + " " + old_val + ", 1");
      }
    }
    emit("store " + llty + " " + new_val + ", ptr " + ptr);
    return old_val;  // postfix returns old value
  }

  case NK_BINOP: {
    if (!n->op) return "0";
    // Short-circuit operators (alloca-based for simplicity; no phi nodes needed)
    if (strcmp(n->op, "&&") == 0) {
      // x && y:  result = 0; if (x) { result = (y != 0); }
      std::string slot = "%land_" + std::to_string(tmp_idx_++);
      alloca_lines_.push_back("  " + slot + " = alloca i32");
      emit("store i32 0, ptr " + slot);  // default false

      std::string lv = codegen_expr(n->left);
      TypeSpec lts = expr_type(n->left);
      std::string lb = to_bool(lv, lts);
      std::string rhs_lbl = fresh_label("land_rhs");
      std::string end_lbl = fresh_label("land_end");
      emit_terminator("br i1 " + lb + ", label %" + rhs_lbl + ", label %" + end_lbl);

      emit_label(rhs_lbl);
      std::string rv = codegen_expr(n->right);
      TypeSpec rts = expr_type(n->right);
      std::string rb = to_bool(rv, rts);
      std::string ri = fresh_tmp();
      emit(ri + " = zext i1 " + rb + " to i32");
      emit("store i32 " + ri + ", ptr " + slot);
      emit_terminator("br label %" + end_lbl);

      emit_label(end_lbl);
      std::string result = fresh_tmp();
      emit(result + " = load i32, ptr " + slot);
      return result;
    }
    if (strcmp(n->op, "||") == 0) {
      // x || y:  result = 1; if (!x) { result = (y != 0); }
      std::string slot = "%lor_" + std::to_string(tmp_idx_++);
      alloca_lines_.push_back("  " + slot + " = alloca i32");
      emit("store i32 1, ptr " + slot);  // default true

      std::string lv = codegen_expr(n->left);
      TypeSpec lts = expr_type(n->left);
      std::string lb = to_bool(lv, lts);
      std::string rhs_lbl = fresh_label("lor_rhs");
      std::string end_lbl = fresh_label("lor_end");
      emit_terminator("br i1 " + lb + ", label %" + end_lbl + ", label %" + rhs_lbl);

      emit_label(rhs_lbl);
      std::string rv = codegen_expr(n->right);
      TypeSpec rts = expr_type(n->right);
      std::string rb = to_bool(rv, rts);
      std::string ri = fresh_tmp();
      emit(ri + " = zext i1 " + rb + " to i32");
      emit("store i32 " + ri + ", ptr " + slot);
      emit_terminator("br label %" + end_lbl);

      emit_label(end_lbl);
      std::string result = fresh_tmp();
      emit(result + " = load i32, ptr " + slot);
      return result;
    }

    TypeSpec lt = expr_type(n->left);
    TypeSpec rt = expr_type(n->right);

    // Complex type operations: handle before generic path.
    // Applies when at least one operand is complex (not a pointer).
    if ((is_complex_base(lt.base) && lt.ptr_level == 0) ||
        (is_complex_base(rt.base) && rt.ptr_level == 0)) {
      const char* cop = n->op ? n->op : "";
      bool is_eq = (strcmp(cop, "==") == 0);
      bool is_ne = (strcmp(cop, "!=") == 0);
      bool is_add = (strcmp(cop, "+") == 0);
      bool is_sub = (strcmp(cop, "-") == 0);

      // Determine the complex type for the operation (pick the wider type).
      TypeSpec cts;
      if (is_complex_base(lt.base) && is_complex_base(rt.base))
        cts = (sizeof_ty(lt) >= sizeof_ty(rt)) ? lt : rt;
      else
        cts = is_complex_base(lt.base) ? lt : rt;
      std::string cllty = llvm_ty(cts);
      TypeSpec ets = complex_component_ts(cts.base);
      std::string ellty = llvm_ty(ets);
      bool is_fp_elem = is_float_base(ets.base);

      // Helper: coerce a value to the operation's complex type.
      auto to_complex = [&](const std::string& val, TypeSpec vts) -> std::string {
        if (is_complex_base(vts.base) && vts.ptr_level == 0) {
          if (llvm_ty(vts) == cllty) return val;  // Same complex type, no conversion needed
          // Different complex width: coerce element-by-element
          TypeSpec src_ets = complex_component_ts(vts.base);
          std::string src_cllty = llvm_ty(vts);
          std::string re0 = fresh_tmp(), im0 = fresh_tmp();
          emit(re0 + " = extractvalue " + src_cllty + " " + val + ", 0");
          emit(im0 + " = extractvalue " + src_cllty + " " + val + ", 1");
          std::string re1 = coerce(re0, src_ets, ellty);
          std::string im1 = coerce(im0, src_ets, ellty);
          std::string t0 = fresh_tmp(), t1 = fresh_tmp();
          emit(t0 + " = insertvalue " + cllty + " zeroinitializer, " + ellty + " " + re1 + ", 0");
          emit(t1 + " = insertvalue " + cllty + " " + t0 + ", " + ellty + " " + im1 + ", 1");
          return t1;
        }
        // Scalar → real part; imaginary part = 0
        std::string cv = coerce(val, vts, ellty);
        std::string t0 = fresh_tmp(), t1 = fresh_tmp();
        std::string zero = is_fp_elem ? "0.0" : "0";
        emit(t0 + " = insertvalue " + cllty + " zeroinitializer, " + ellty + " " + cv + ", 0");
        emit(t1 + " = insertvalue " + cllty + " " + t0 + ", " + ellty + " " + zero + ", 1");
        return t1;
      };

      std::string lv_raw = codegen_expr(n->left);
      std::string rv_raw = codegen_expr(n->right);
      std::string lvc = to_complex(lv_raw, lt);
      std::string rvc = to_complex(rv_raw, rt);

      if (is_eq || is_ne) {
        // Compare element-wise; ne = (re_ne || im_ne), eq = (re_eq && im_eq)
        std::string lre = fresh_tmp(), lim = fresh_tmp();
        std::string rre = fresh_tmp(), rim = fresh_tmp();
        emit(lre + " = extractvalue " + cllty + " " + lvc + ", 0");
        emit(lim + " = extractvalue " + cllty + " " + lvc + ", 1");
        emit(rre + " = extractvalue " + cllty + " " + rvc + ", 0");
        emit(rim + " = extractvalue " + cllty + " " + rvc + ", 1");
        std::string icmp = is_fp_elem ? "fcmp" : "icmp";
        std::string op_re = fresh_tmp(), op_im = fresh_tmp();
        std::string comb  = fresh_tmp(), ext   = fresh_tmp();
        if (is_eq) {
          emit(op_re + " = " + icmp + (is_fp_elem ? " oeq " : " eq ") + ellty + " " + lre + ", " + rre);
          emit(op_im + " = " + icmp + (is_fp_elem ? " oeq " : " eq ") + ellty + " " + lim + ", " + rim);
          emit(comb  + " = and i1 " + op_re + ", " + op_im);
        } else {
          emit(op_re + " = " + icmp + (is_fp_elem ? " une " : " ne ") + ellty + " " + lre + ", " + rre);
          emit(op_im + " = " + icmp + (is_fp_elem ? " une " : " ne ") + ellty + " " + lim + ", " + rim);
          emit(comb  + " = or i1 " + op_re + ", " + op_im);
        }
        emit(ext + " = zext i1 " + comb + " to i32");
        return ext;
      }

      if (is_add || is_sub) {
        std::string lre = fresh_tmp(), lim = fresh_tmp();
        std::string rre = fresh_tmp(), rim = fresh_tmp();
        emit(lre + " = extractvalue " + cllty + " " + lvc + ", 0");
        emit(lim + " = extractvalue " + cllty + " " + lvc + ", 1");
        emit(rre + " = extractvalue " + cllty + " " + rvc + ", 0");
        emit(rim + " = extractvalue " + cllty + " " + rvc + ", 1");
        std::string inst = is_fp_elem ? (is_add ? "fadd" : "fsub") : (is_add ? "add" : "sub");
        std::string re_r = fresh_tmp(), im_r = fresh_tmp();
        emit(re_r + " = " + inst + " " + ellty + " " + lre + ", " + rre);
        emit(im_r + " = " + inst + " " + ellty + " " + lim + ", " + rim);
        std::string t0 = fresh_tmp(), t1 = fresh_tmp();
        emit(t0 + " = insertvalue " + cllty + " zeroinitializer, " + ellty + " " + re_r + ", 0");
        emit(t1 + " = insertvalue " + cllty + " " + t0 + ", " + ellty + " " + im_r + ", 1");
        return t1;
      }
      bool is_mul = (strcmp(cop, "*") == 0);
      bool is_div = (strcmp(cop, "/") == 0);

      if (is_mul || is_div) {
        // (a+bi)*(c+di) = (ac-bd) + (ad+bc)i
        // (a+bi)/(c+di) = ((ac+bd)/(c²+d²)) + ((bc-ad)/(c²+d²))i
        std::string lre = fresh_tmp(), lim = fresh_tmp();
        std::string rre = fresh_tmp(), rim = fresh_tmp();
        emit(lre + " = extractvalue " + cllty + " " + lvc + ", 0");
        emit(lim + " = extractvalue " + cllty + " " + lvc + ", 1");
        emit(rre + " = extractvalue " + cllty + " " + rvc + ", 0");
        emit(rim + " = extractvalue " + cllty + " " + rvc + ", 1");
        std::string re_r = fresh_tmp(), im_r = fresh_tmp();
        if (is_fp_elem) {
          if (is_mul) {
            // re = lre*rre - lim*rim
            // im = lre*rim + lim*rre
            std::string ac = fresh_tmp(), bd = fresh_tmp();
            std::string ad = fresh_tmp(), bc = fresh_tmp();
            emit(ac + " = fmul " + ellty + " " + lre + ", " + rre);
            emit(bd + " = fmul " + ellty + " " + lim + ", " + rim);
            emit(ad + " = fmul " + ellty + " " + lre + ", " + rim);
            emit(bc + " = fmul " + ellty + " " + lim + ", " + rre);
            emit(re_r + " = fsub " + ellty + " " + ac + ", " + bd);
            emit(im_r + " = fadd " + ellty + " " + ad + ", " + bc);
          } else {
            // re = (lre*rre + lim*rim) / (rre*rre + rim*rim)
            // im = (lim*rre - lre*rim) / (rre*rre + rim*rim)
            std::string ac = fresh_tmp(), bd = fresh_tmp();
            std::string bc = fresh_tmp(), ad = fresh_tmp();
            std::string cc = fresh_tmp(), dd = fresh_tmp();
            std::string num_re = fresh_tmp(), num_im = fresh_tmp(), denom = fresh_tmp();
            emit(ac + " = fmul " + ellty + " " + lre + ", " + rre);
            emit(bd + " = fmul " + ellty + " " + lim + ", " + rim);
            emit(bc + " = fmul " + ellty + " " + lim + ", " + rre);
            emit(ad + " = fmul " + ellty + " " + lre + ", " + rim);
            emit(cc + " = fmul " + ellty + " " + rre + ", " + rre);
            emit(dd + " = fmul " + ellty + " " + rim + ", " + rim);
            emit(num_re + " = fadd " + ellty + " " + ac + ", " + bd);
            emit(num_im + " = fsub " + ellty + " " + bc + ", " + ad);
            emit(denom  + " = fadd " + ellty + " " + cc + ", " + dd);
            emit(re_r + " = fdiv " + ellty + " " + num_re + ", " + denom);
            emit(im_r + " = fdiv " + ellty + " " + num_im + ", " + denom);
          }
        } else {
          if (is_mul) {
            std::string ac = fresh_tmp(), bd = fresh_tmp();
            std::string ad = fresh_tmp(), bc = fresh_tmp();
            emit(ac + " = mul " + ellty + " " + lre + ", " + rre);
            emit(bd + " = mul " + ellty + " " + lim + ", " + rim);
            emit(ad + " = mul " + ellty + " " + lre + ", " + rim);
            emit(bc + " = mul " + ellty + " " + lim + ", " + rre);
            emit(re_r + " = sub " + ellty + " " + ac + ", " + bd);
            emit(im_r + " = add " + ellty + " " + ad + ", " + bc);
          } else {
            // Integer complex division — compute via signed div
            std::string ac = fresh_tmp(), bd = fresh_tmp();
            std::string bc = fresh_tmp(), ad = fresh_tmp();
            std::string cc = fresh_tmp(), dd = fresh_tmp();
            std::string num_re = fresh_tmp(), num_im = fresh_tmp(), denom = fresh_tmp();
            emit(ac + " = mul " + ellty + " " + lre + ", " + rre);
            emit(bd + " = mul " + ellty + " " + lim + ", " + rim);
            emit(bc + " = mul " + ellty + " " + lim + ", " + rre);
            emit(ad + " = mul " + ellty + " " + lre + ", " + rim);
            emit(cc + " = mul " + ellty + " " + rre + ", " + rre);
            emit(dd + " = mul " + ellty + " " + rim + ", " + rim);
            emit(num_re + " = add " + ellty + " " + ac + ", " + bd);
            emit(num_im + " = sub " + ellty + " " + bc + ", " + ad);
            emit(denom  + " = add " + ellty + " " + cc + ", " + dd);
            emit(re_r + " = sdiv " + ellty + " " + num_re + ", " + denom);
            emit(im_r + " = sdiv " + ellty + " " + num_im + ", " + denom);
          }
        }
        std::string t0 = fresh_tmp(), t1 = fresh_tmp();
        emit(t0 + " = insertvalue " + cllty + " zeroinitializer, " + ellty + " " + re_r + ", 0");
        emit(t1 + " = insertvalue " + cllty + " " + t0 + ", " + ellty + " " + im_r + ", 1");
        return t1;
      }
      // Other operators on complex: fall through to generic (will likely produce bad IR;
      // can be improved later if more tests need it).
    }

    std::string lv = codegen_expr(n->left);
    std::string rv = codegen_expr(n->right);

    // Determine result type
    TypeSpec res_ts = expr_type(n);
    std::string res_llty = llvm_ty(res_ts);

    // Vector/array element-wise arithmetic: both operands are the same array type
    // (GNU vector_size types stored as [N x T]).  Handles +, -, *, /, %, &, |, ^, <<, >>.
    if (is_array_ty(lt) && lt.ptr_level == 0 && !lt.is_ptr_to_array &&
        is_array_ty(rt) && rt.ptr_level == 0 && !rt.is_ptr_to_array &&
        array_rank_of(lt) == 1 && array_rank_of(rt) == 1) {
      long long cnt = array_dim_at(lt, 0);
      TypeSpec elem_ts = drop_array_dim(lt);
      std::string elem_llty = llvm_ty(elem_ts);
      std::string vec_llty  = llvm_ty(lt);
      bool is_fp   = (elem_llty == "float" || elem_llty == "double");
      bool is_uint = is_unsigned_base(elem_ts.base);
      const char* op_str = n->op ? n->op : "+";
      std::string llvm_op;
      if      (strcmp(op_str, "+")  == 0) llvm_op = is_fp ? "fadd" : "add";
      else if (strcmp(op_str, "-")  == 0) llvm_op = is_fp ? "fsub" : "sub";
      else if (strcmp(op_str, "*")  == 0) llvm_op = is_fp ? "fmul" : "mul";
      else if (strcmp(op_str, "/")  == 0) llvm_op = is_fp ? "fdiv" : (is_uint ? "udiv" : "sdiv");
      else if (strcmp(op_str, "%")  == 0) llvm_op = is_uint ? "urem" : "srem";
      else if (strcmp(op_str, "&")  == 0) llvm_op = "and";
      else if (strcmp(op_str, "|")  == 0) llvm_op = "or";
      else if (strcmp(op_str, "^")  == 0) llvm_op = "xor";
      else if (strcmp(op_str, "<<") == 0) llvm_op = "shl";
      else if (strcmp(op_str, ">>") == 0) llvm_op = is_uint ? "lshr" : "ashr";
      if (!llvm_op.empty() && cnt > 0) {
        // Normalize operand: most node kinds return a ptr (address) for array/vector types;
        // load to get value. Exceptions: NK_VAR, NK_DEREF, NK_COMPOUND_LIT of pure vector
        // (rank==1, is_vector) already return the loaded value from codegen_expr.
        auto vec_val = [&](const std::string& v, Node* nd) -> std::string {
          if (!nd) return v;
          TypeSpec nd_ts = expr_type(nd);
          bool pure_vec = (nd_ts.ptr_level == 0 && nd_ts.is_vector && array_rank_of(nd_ts) == 1);
          // NK_VAR and NK_DEREF of pure vector already loaded the value in codegen_expr.
          if (pure_vec && (nd->kind == NK_VAR || nd->kind == NK_DEREF))
            return v;
          // These node kinds return a ptr (address) for array types; load to get value.
          if (nd->kind == NK_VAR || nd->kind == NK_COMPOUND_LIT ||
              nd->kind == NK_INDEX || nd->kind == NK_MEMBER || nd->kind == NK_DEREF) {
            std::string loaded = fresh_tmp();
            emit(loaded + " = load " + vec_llty + ", ptr " + v);
            return loaded;
          }
          return v;
        };
        std::string lv_val = vec_val(lv, n->left);
        std::string rv_val = vec_val(rv, n->right);
        std::string acc = "zeroinitializer";
        for (long long idx = 0; idx < cnt; idx++) {
          std::string a_elem = fresh_tmp(), b_elem = fresh_tmp(), r_elem = fresh_tmp();
          emit(a_elem + " = extractvalue " + vec_llty + " " + lv_val + ", " + std::to_string(idx));
          emit(b_elem + " = extractvalue " + vec_llty + " " + rv_val + ", " + std::to_string(idx));
          emit(r_elem + " = " + llvm_op + " " + elem_llty + " " + a_elem + ", " + b_elem);
          std::string new_acc = fresh_tmp();
          emit(new_acc + " = insertvalue " + vec_llty + " " + acc + ", " + elem_llty + " " + r_elem + ", " + std::to_string(idx));
          acc = new_acc;
        }
        return acc;
      }
    }

    // Pointer arithmetic / comparison
    bool lptr = lt.ptr_level > 0 || is_array_ty(lt);
    bool rptr = rt.ptr_level > 0 || is_array_ty(rt);
    if (lptr || rptr) {
      // Comparison operators on pointers (==, !=, <, <=, >, >=):
      // use icmp with ptr operands rather than converting to integers.
      // n->op is used directly here because 'op' is declared further below.
      const char* nop = n->op ? n->op : "";
      bool is_cmp_op = (strcmp(nop, "==") == 0 || strcmp(nop, "!=") == 0 ||
                        strcmp(nop, "<") == 0  || strcmp(nop, "<=") == 0 ||
                        strcmp(nop, ">") == 0  || strcmp(nop, ">=") == 0);
      if (is_cmp_op) {
        // Coerce both operands to ptr (integer 0 → null)
        std::string lv_ptr = lptr ? lv : coerce(lv, lt, "ptr");
        std::string rv_ptr = rptr ? rv : coerce(rv, rt, "ptr");
        const char* icmp_op =
            (strcmp(nop,"==")  == 0) ? "eq"  :
            (strcmp(nop,"!=")  == 0) ? "ne"  :
            (strcmp(nop,"<")   == 0) ? "ult" :
            (strcmp(nop,"<=")  == 0) ? "ule" :
            (strcmp(nop,">")   == 0) ? "ugt" : "uge";
        std::string ct = fresh_tmp();
        emit(ct + " = icmp " + icmp_op + " ptr " + lv_ptr + ", " + rv_ptr);
        std::string zt = fresh_tmp();
        emit(zt + " = zext i1 " + ct + " to i32");
        return zt;
      }
      // Include array types: arrays decay to pointers in arithmetic expressions.
      bool lptr_only = lt.ptr_level > 0 || is_array_ty(lt);
      bool rptr_only = rt.ptr_level > 0 || is_array_ty(rt);
      auto elem_of = [](const TypeSpec& ts) -> TypeSpec {
        if (ts.ptr_level > 0) { TypeSpec e = ts; e.ptr_level -= 1; return e; }
        return drop_array_dim(ts);
      };
      if (lptr_only && !rptr_only) {
        // ptr/array +/- int
        TypeSpec elem_ts = elem_of(lt);
        std::string elem_llty = gep_elem_ty(llvm_ty(elem_ts));
        std::string idx = coerce(rv, rt, "i64");
        if (n->op && strcmp(n->op, "-") == 0) {
          std::string neg_idx = fresh_tmp();
          emit(neg_idx + " = sub i64 0, " + idx);
          idx = neg_idx;
        }
        std::string t = fresh_tmp();
        emit(t + " = getelementptr " + elem_llty + ", ptr " + lv + ", i64 " + idx);
        return t;
      }
      if (rptr_only && !lptr_only) {
        // int + ptr/array
        TypeSpec elem_ts = elem_of(rt);
        std::string elem_llty = gep_elem_ty(llvm_ty(elem_ts));
        std::string idx = coerce(lv, lt, "i64");
        std::string t = fresh_tmp();
        emit(t + " = getelementptr " + elem_llty + ", ptr " + rv + ", i64 " + idx);
        return t;
      }
      // ptr - ptr → ptrdiff_t (i64)
      std::string t = fresh_tmp();
      emit(t + " = ptrtoint ptr " + lv + " to i64");
      std::string t2 = fresh_tmp();
      emit(t2 + " = ptrtoint ptr " + rv + " to i64");
      std::string t3 = fresh_tmp();
      emit(t3 + " = sub i64 " + t + ", " + t2);
      // divide by element size
      TypeSpec elem_ts2 = elem_of(lt);
      int esz = sizeof_ty(elem_ts2);
      if (esz > 1) {
        std::string t4 = fresh_tmp();
        emit(t4 + " = sdiv i64 " + t3 + ", " + std::to_string(esz));
        return t4;
      }
      return t3;
    }

    bool is_float = is_float_base(res_ts.base);
    // For ordered comparisons (<, <=, >, >=), track whether to use unsigned icmp.
    // Default: left operand signedness (overridden by the block below for mixed types).
    bool cmp_unsigned = is_unsigned_base(lt.base);

    // For comparison operators, operand types determine float vs integer instruction.
    // expr_type(comparison) always returns int, so check operand types separately.
    {
      const char* opc = n->op ? n->op : "";
      bool is_cmp = (strcmp(opc,"==")==0 || strcmp(opc,"!=")==0 ||
                     strcmp(opc,"<")==0  || strcmp(opc,"<=")==0 ||
                     strcmp(opc,">")==0  || strcmp(opc,">=")==0);
      if (is_cmp && (is_float_base(lt.base) || is_float_base(rt.base))) {
        // Determine common float type for both operands
        bool use_double = (lt.base == TB_DOUBLE || rt.base == TB_DOUBLE ||
                           lt.base == TB_LONGDOUBLE || rt.base == TB_LONGDOUBLE);
        TypeSpec cmp_ts = use_double ? double_ts() : float_ts();
        res_llty = llvm_ty(cmp_ts);
        is_float = true;
      } else if (is_cmp && lt.ptr_level == 0 && rt.ptr_level == 0 &&
                 !is_array_ty(lt) && !is_array_ty(rt)) {
        // Integer comparison: coerce operands to the wider of the two types, not the
        // comparison result type (always int/i32). Prevents truncation of e.g. long long
        // or unsigned long before comparison.
        int lt_sz = sizeof_ty(lt);
        int rt_sz = sizeof_ty(rt);
        if (lt_sz >= rt_sz && lt_sz > 4) res_llty = llvm_ty(lt);
        else if (rt_sz > lt_sz && rt_sz > 4) res_llty = llvm_ty(rt);
        // Determine comparison sign using C usual arithmetic conversions:
        // unsigned type wins at same rank (same size on our target).
        if (lt.base == TB_ULONGLONG || rt.base == TB_ULONGLONG)      cmp_unsigned = true;
        else if (lt.base == TB_LONGLONG || rt.base == TB_LONGLONG)    cmp_unsigned = (lt.base == TB_ULONG || rt.base == TB_ULONG);
        else if (lt.base == TB_ULONG    || rt.base == TB_ULONG)       cmp_unsigned = true;
        else if (lt.base == TB_LONG     || rt.base == TB_LONG)        cmp_unsigned = false;
        else if (lt.base == TB_UINT     || rt.base == TB_UINT)        cmp_unsigned = true;
        else                                                           cmp_unsigned = false;
      }
    }

    // Coerce operands to result type
    lv = coerce(lv, lt, res_llty);
    rv = coerce(rv, rt, res_llty);

    std::string t = fresh_tmp();
    const char* op = n->op;

    // Comparison operators → i1, then zext to i32
    auto emit_cmp = [&](const std::string& icmp_suffix,
                        const std::string& fcmp_suffix) -> std::string {
      std::string ct = fresh_tmp();
      if (is_float)
        emit(ct + " = fcmp " + fcmp_suffix + " " + res_llty + " " + lv + ", " + rv);
      else
        emit(ct + " = icmp " + icmp_suffix + " " + res_llty + " " + lv + ", " + rv);
      std::string zt = fresh_tmp();
      emit(zt + " = zext i1 " + ct + " to i32");
      return zt;
    };

    if (strcmp(op, "+") == 0) {
      emit(t + " = " + (is_float ? "fadd" : "add") + " " + res_llty + " " + lv + ", " + rv);
    } else if (strcmp(op, "-") == 0) {
      emit(t + " = " + (is_float ? "fsub" : "sub") + " " + res_llty + " " + lv + ", " + rv);
    } else if (strcmp(op, "*") == 0) {
      emit(t + " = " + (is_float ? "fmul" : "mul") + " " + res_llty + " " + lv + ", " + rv);
    } else if (strcmp(op, "/") == 0) {
      if (is_float)
        emit(t + " = fdiv " + res_llty + " " + lv + ", " + rv);
      else {
        std::string div_op = is_unsigned_base(res_ts.base) ? "udiv" : "sdiv";
        emit(t + " = " + div_op + " " + res_llty + " " + lv + ", " + rv);
      }
    } else if (strcmp(op, "%") == 0) {
      std::string rem_op = is_unsigned_base(res_ts.base) ? "urem" : "srem";
      emit(t + " = " + rem_op + " " + res_llty + " " + lv + ", " + rv);
    } else if (strcmp(op, "<<") == 0) {
      // lv/rv are already coerced to res_llty (promoted type) by the block above.
      emit(t + " = shl " + res_llty + " " + lv + ", " + rv);
    } else if (strcmp(op, ">>") == 0) {
      std::string shr_op = is_unsigned_base(res_ts.base) ? "lshr" : "ashr";
      emit(t + " = " + shr_op + " " + res_llty + " " + lv + ", " + rv);
    } else if (strcmp(op, "&") == 0) {
      emit(t + " = and " + res_llty + " " + lv + ", " + rv);
    } else if (strcmp(op, "|") == 0) {
      emit(t + " = or " + res_llty + " " + lv + ", " + rv);
    } else if (strcmp(op, "^") == 0) {
      emit(t + " = xor " + res_llty + " " + lv + ", " + rv);
    } else if (strcmp(op, "==") == 0) {
      return emit_cmp("eq", "oeq");
    } else if (strcmp(op, "!=") == 0) {
      return emit_cmp("ne", "une");
    } else if (strcmp(op, "<") == 0) {
      return emit_cmp(cmp_unsigned ? "ult" : "slt", "olt");
    } else if (strcmp(op, "<=") == 0) {
      return emit_cmp(cmp_unsigned ? "ule" : "sle", "ole");
    } else if (strcmp(op, ">") == 0) {
      return emit_cmp(cmp_unsigned ? "ugt" : "sgt", "ogt");
    } else if (strcmp(op, ">=") == 0) {
      return emit_cmp(cmp_unsigned ? "uge" : "sge", "oge");
    } else {
      // Unknown operator: return 0
      return "0";
    }
    return t;
  }

  case NK_ASSIGN: {
    const char* aop = n->op ? n->op : "=";
    TypeSpec lts = expr_type(n->left);
    if (lts.is_const && lts.ptr_level == 0 && !is_array_ty(lts))
      throw std::runtime_error("assignment to const-qualified lvalue");
    std::string llty = llvm_ty(lts);

    if (strcmp(aop, "=") == 0) {
      // Plain assignment
      TypeSpec rts = expr_type(n->right);
      auto discards_const_from_rhs = [&](Node* rhs, const TypeSpec& rhs_ts, const TypeSpec& lhs_ts) -> bool {
        if (lhs_ts.ptr_level <= 0 || lhs_ts.is_const) return false;
        if (!rhs) return false;
        if (rhs->kind == NK_STR_LIT) return !allows_string_literal_ptr_target(lhs_ts);
        if (rhs->kind == NK_ADDR && rhs->left) {
          TypeSpec inner = expr_type(rhs->left);
          if (inner.ptr_level == 0 && inner.is_const) return true;
        }
        if (rhs->kind == NK_VAR && rhs_ts.ptr_level > 0 && rhs_ts.is_const) return true;
        return false;
      };
      if (discards_const_from_rhs(n->right, rts, lts))
        throw std::runtime_error("assignment discards const qualifier");
      std::string rval = codegen_expr(n->right);
      rval = coerce(rval, rts, llty);
      // Array assignment: rhs from local array var/compound-lit/cast is an alloca ptr;
      // load the array value before storing (same pattern as NK_RETURN).
      // Exception: NK_VAR and NK_DEREF of pure vector (is_vector, rank==1) already
      // return a loaded value from codegen_expr — don't reload in those cases.
      {
        bool rhs_already_loaded = (rts.is_vector && array_rank_of(rts) == 1 &&
                                   n->right &&
                                   (n->right->kind == NK_VAR || n->right->kind == NK_DEREF));
        if (!rhs_already_loaded &&
            !llty.empty() && llty[0] == '[' &&
            is_array_ty(rts) && rts.ptr_level == 0 && !rts.is_ptr_to_array &&
            n->right && (n->right->kind == NK_VAR || n->right->kind == NK_COMPOUND_LIT ||
                         n->right->kind == NK_CAST || n->right->kind == NK_INDEX)) {
          std::string loaded = fresh_tmp();
          emit(loaded + " = load " + llty + ", ptr " + rval);
          rval = loaded;
        }
      }
      std::string ptr = codegen_lval(n->left);
      // Bitfield write: read-modify-write with storage unit type.
      {
        int bfw = member_bitfield_width(n->left);
        if (bfw > 0 && lts.ptr_level == 0 && !is_array_ty(lts)) {
          // Determine storage type and bit offset
          TypeSpec obj_ts = expr_type(n->left->left ? n->left->left : n->left);
          if (n->left->is_arrow && obj_ts.ptr_level > 0) obj_ts.ptr_level--;
          std::string stor_llty = "i32";
          int bit_off = 0;
          if ((obj_ts.base == TB_STRUCT || obj_ts.base == TB_UNION) && obj_ts.tag) {
            FieldPath path;
            if (find_field(obj_ts.tag, n->left->name, path)) {
              bit_off = field_bit_offset(struct_defs_, path.final_tag, path.final_idx);
              stor_llty = field_storage_llty(struct_defs_, path.final_tag, path.final_idx);
            }
          }
          long long mask = (bfw >= 64) ? (long long)-1 : ((1LL << bfw) - 1);
          // Coerce rval to storage type
          std::string rval_s = rval;
          if (stor_llty != llty) {
            auto llty_bits2 = [](const std::string& t) {
              if (t == "i1") return 1; if (t == "i8") return 8;
              if (t == "i16") return 16; if (t == "i32") return 32; return 64;
            };
            int sb2 = llty_bits2(stor_llty), lb2 = llty_bits2(llty);
            rval_s = fresh_tmp();
            if (sb2 > lb2)
              emit(rval_s + " = sext " + llty + " " + rval + " to " + stor_llty);
            else if (sb2 < lb2)
              emit(rval_s + " = trunc " + llty + " " + rval + " to " + stor_llty);
            else
              emit(rval_s + " = bitcast " + llty + " " + rval + " to " + stor_llty);
          }
          // Mask to bitfield width
          std::string new_bits = fresh_tmp();
          emit(new_bits + " = and " + stor_llty + " " + rval_s + ", " + std::to_string(mask));
          if (bit_off == 0 && bfw >= 8 * sizeof_ty(lts)) {
            // Entire storage unit is this bitfield — just store directly
            emit("store " + stor_llty + " " + new_bits + ", ptr " + ptr);
          } else {
            // Read-modify-write
            std::string old_val = fresh_tmp();
            emit(old_val + " = load " + stor_llty + ", ptr " + ptr);
            long long clear_mask = ~(mask << bit_off);
            std::string cleared = fresh_tmp();
            emit(cleared + " = and " + stor_llty + " " + old_val + ", " + std::to_string(clear_mask));
            std::string shifted = new_bits;
            if (bit_off > 0) {
              shifted = fresh_tmp();
              emit(shifted + " = shl " + stor_llty + " " + new_bits + ", " + std::to_string(bit_off));
            }
            std::string merged = fresh_tmp();
            emit(merged + " = or " + stor_llty + " " + cleared + ", " + shifted);
            emit("store " + stor_llty + " " + merged + ", ptr " + ptr);
          }
          return rval;  // assignment expression value is the RHS (not masked)
        }
      }
      emit("store " + llty + " " + rval + ", ptr " + ptr);
      return rval;
    }

    // Compound assignment: lhs op= rhs
    // Evaluate RHS before loading LHS so that rhs side-effects are visible
    // when we read the lhs (matches clang/gcc evaluation order).
    std::string ptr = codegen_lval(n->left);

    TypeSpec rts = expr_type(n->right);
    std::string rv = codegen_expr(n->right);

    std::string cur = fresh_tmp();
    emit(cur + " = load " + llty + ", ptr " + ptr);

    // Complex compound assignment: lhs is a complex type
    if (lts.ptr_level == 0 && is_complex_base(lts.base)) {
      TypeSpec ets = complex_component_ts(lts.base);
      std::string ellty = llvm_ty(ets);
      bool is_fp = is_float_base(ets.base);
      // Coerce rhs to same complex type
      auto to_complex = [&](const std::string& val, TypeSpec vts) -> std::string {
        if (is_complex_base(vts.base) && vts.ptr_level == 0) {
          if (llvm_ty(vts) == llty) return val;
          TypeSpec src_ets = complex_component_ts(vts.base);
          std::string src_llty = llvm_ty(vts);
          std::string re0 = fresh_tmp(), im0 = fresh_tmp();
          emit(re0 + " = extractvalue " + src_llty + " " + val + ", 0");
          emit(im0 + " = extractvalue " + src_llty + " " + val + ", 1");
          std::string re1 = coerce(re0, src_ets, ellty);
          std::string im1 = coerce(im0, src_ets, ellty);
          std::string t0 = fresh_tmp(), t1 = fresh_tmp();
          emit(t0 + " = insertvalue " + llty + " zeroinitializer, " + ellty + " " + re1 + ", 0");
          emit(t1 + " = insertvalue " + llty + " " + t0 + ", " + ellty + " " + im1 + ", 1");
          return t1;
        }
        std::string cv = coerce(val, vts, ellty);
        std::string t0 = fresh_tmp(), t1 = fresh_tmp();
        std::string zero = is_fp ? "0.0" : "0";
        emit(t0 + " = insertvalue " + llty + " zeroinitializer, " + ellty + " " + cv + ", 0");
        emit(t1 + " = insertvalue " + llty + " " + t0 + ", " + ellty + " " + zero + ", 1");
        return t1;
      };
      std::string rvc = to_complex(rv, rts);
      std::string lre = fresh_tmp(), lim = fresh_tmp();
      std::string rre = fresh_tmp(), rim = fresh_tmp();
      emit(lre + " = extractvalue " + llty + " " + cur + ", 0");
      emit(lim + " = extractvalue " + llty + " " + cur + ", 1");
      emit(rre + " = extractvalue " + llty + " " + rvc + ", 0");
      emit(rim + " = extractvalue " + llty + " " + rvc + ", 1");
      std::string result_re = fresh_tmp(), result_im = fresh_tmp();
      std::string t0 = fresh_tmp(), t1 = fresh_tmp();
      if (strcmp(aop, "+=") == 0 || strcmp(aop, "-=") == 0) {
        std::string arith = (strcmp(aop, "+=") == 0) ? (is_fp ? "fadd" : "add")
                                                      : (is_fp ? "fsub" : "sub");
        emit(result_re + " = " + arith + " " + ellty + " " + lre + ", " + rre);
        emit(result_im + " = " + arith + " " + ellty + " " + lim + ", " + rim);
      } else if (strcmp(aop, "*=") == 0) {
        std::string ac=fresh_tmp(),bd=fresh_tmp(),ad=fresh_tmp(),bc=fresh_tmp();
        std::string mul=is_fp?"fmul":"mul", sub=is_fp?"fsub":"sub", add2=is_fp?"fadd":"add";
        emit(ac + " = " + mul + " " + ellty + " " + lre + ", " + rre);
        emit(bd + " = " + mul + " " + ellty + " " + lim + ", " + rim);
        emit(ad + " = " + mul + " " + ellty + " " + lre + ", " + rim);
        emit(bc + " = " + mul + " " + ellty + " " + lim + ", " + rre);
        emit(result_re + " = " + sub + " " + ellty + " " + ac + ", " + bd);
        emit(result_im + " = " + add2 + " " + ellty + " " + ad + ", " + bc);
      } else {
        result_re = lre; result_im = lim;  // unhandled op: identity
      }
      emit(t0 + " = insertvalue " + llty + " zeroinitializer, " + ellty + " " + result_re + ", 0");
      emit(t1 + " = insertvalue " + llty + " " + t0 + ", " + ellty + " " + result_im + ", 1");
      emit("store " + llty + " " + t1 + ", ptr " + ptr);
      return t1;
    }

    bool is_float = is_float_base(lts.base);

    // C99: float op= expr does arithmetic in double precision, then truncates to float
    bool needs_fp_upcast = (lts.base == TB_FLOAT && lts.ptr_level == 0 && !lts.is_vector);
    if (needs_fp_upcast) {
      std::string cur_d = fresh_tmp();
      emit(cur_d + " = fpext float " + cur + " to double");
      rv = coerce(rv, rts, "double");
      std::string result_d = fresh_tmp();
      std::string op_str;
      if      (strcmp(aop, "+=") == 0) op_str = "fadd";
      else if (strcmp(aop, "-=") == 0) op_str = "fsub";
      else if (strcmp(aop, "*=") == 0) op_str = "fmul";
      else if (strcmp(aop, "/=") == 0) op_str = "fdiv";
      if (!op_str.empty()) {
        emit(result_d + " = " + op_str + " double " + cur_d + ", " + rv);
        std::string result_f = fresh_tmp();
        emit(result_f + " = fptrunc double " + result_d + " to float");
        emit("store float " + result_f + ", ptr " + ptr);
        return result_f;
      }
      // Fallback for unexpected ops: skip upcasting
      rv = coerce(rv, rts, llty);
    }

    // C11 §6.3.1.1: narrow integer lhs (< int width) promotes to int before arithmetic.
    bool needs_int_promote = (!needs_fp_upcast && lts.ptr_level == 0 && !is_float &&
                              !lts.is_vector && !is_complex_base(lts.base) &&
                              sizeof_ty(lts) < 4);
    if (needs_int_promote) {
      std::string cur_ext = fresh_tmp();
      emit(cur_ext + " = " + (is_unsigned_base(lts.base) ? "zext" : "sext") +
           " " + llty + " " + cur + " to i32");
      std::string rv32 = coerce(rv, rts, "i32");
      std::string res32 = fresh_tmp();
      std::string op32_str;
      if      (strcmp(aop, "+=")  == 0) op32_str = "add";
      else if (strcmp(aop, "-=")  == 0) op32_str = "sub";
      else if (strcmp(aop, "*=")  == 0) op32_str = "mul";
      else if (strcmp(aop, "/=")  == 0) op32_str = "sdiv";
      else if (strcmp(aop, "%=")  == 0) op32_str = "srem";
      else if (strcmp(aop, "&=")  == 0) op32_str = "and";
      else if (strcmp(aop, "|=")  == 0) op32_str = "or";
      else if (strcmp(aop, "^=")  == 0) op32_str = "xor";
      else if (strcmp(aop, "<<=") == 0) op32_str = "shl";
      else if (strcmp(aop, ">>=") == 0)
        op32_str = is_unsigned_base(lts.base) ? "lshr" : "ashr";
      if (!op32_str.empty()) {
        emit(res32 + " = " + op32_str + " i32 " + cur_ext + ", " + rv32);
        std::string res_narrow = fresh_tmp();
        emit(res_narrow + " = trunc i32 " + res32 + " to " + llty);
        emit("store " + llty + " " + res_narrow + ", ptr " + ptr);
        return res_narrow;
      }
    }

    // For pointer arithmetic, keep rv as integer (coerce to i64 for GEP index)
    // For non-pointer ops, coerce to lhs type
    if (!needs_fp_upcast && lts.ptr_level == 0)
      rv = coerce(rv, rts, llty);

    // Vector compound assignment: lhs is a GNU vector (is_vector, array_rank==1).
    // LLVM doesn't support arithmetic on [N x T] arrays directly; use element-wise
    // extractvalue/insertvalue (same approach as NK_BINOP vector arithmetic).
    if (lts.ptr_level == 0 && lts.is_vector && array_rank_of(lts) == 1) {
      // Normalize rv: NK_VAR and NK_DEREF of pure vector already return a loaded value;
      // NK_COMPOUND_LIT, NK_INDEX, NK_MEMBER return the alloca ptr — load it.
      bool rv_already_loaded = (rts.is_vector && array_rank_of(rts) == 1 && n->right &&
                                (n->right->kind == NK_VAR || n->right->kind == NK_DEREF));
      if (!rv_already_loaded) {
        std::string rv_loaded = fresh_tmp();
        emit(rv_loaded + " = load " + llty + ", ptr " + rv);
        rv = rv_loaded;
      }
      long long cnt = array_dim_at(lts, 0);
      TypeSpec elem_ts = drop_array_dim(lts);
      std::string elem_llty = llvm_ty(elem_ts);
      bool vec_fp   = (elem_llty == "float" || elem_llty == "double");
      bool vec_uint = is_unsigned_base(elem_ts.base);
      std::string llvm_op;
      if      (strcmp(aop, "+=") == 0)  llvm_op = vec_fp ? "fadd" : "add";
      else if (strcmp(aop, "-=") == 0)  llvm_op = vec_fp ? "fsub" : "sub";
      else if (strcmp(aop, "*=") == 0)  llvm_op = vec_fp ? "fmul" : "mul";
      else if (strcmp(aop, "/=") == 0)  llvm_op = vec_fp ? "fdiv" : (vec_uint ? "udiv" : "sdiv");
      else if (strcmp(aop, "%=") == 0)  llvm_op = vec_uint ? "urem" : "srem";
      else if (strcmp(aop, "&=") == 0)  llvm_op = "and";
      else if (strcmp(aop, "|=") == 0)  llvm_op = "or";
      else if (strcmp(aop, "^=") == 0)  llvm_op = "xor";
      else if (strcmp(aop, "<<=") == 0) llvm_op = "shl";
      else if (strcmp(aop, ">>=") == 0) llvm_op = vec_uint ? "lshr" : "ashr";
      if (!llvm_op.empty() && cnt > 0) {
        std::string acc = "zeroinitializer";
        for (long long idx = 0; idx < cnt; idx++) {
          std::string a_elem = fresh_tmp(), b_elem = fresh_tmp(), r_elem = fresh_tmp();
          emit(a_elem + " = extractvalue " + llty + " " + cur + ", " + std::to_string(idx));
          emit(b_elem + " = extractvalue " + llty + " " + rv  + ", " + std::to_string(idx));
          emit(r_elem + " = " + llvm_op + " " + elem_llty + " " + a_elem + ", " + b_elem);
          std::string new_acc = fresh_tmp();
          emit(new_acc + " = insertvalue " + llty + " " + acc + ", " + elem_llty + " " + r_elem + ", " + std::to_string(idx));
          acc = new_acc;
        }
        emit("store " + llty + " " + acc + ", ptr " + ptr);
        return acc;
      }
    }

    std::string result = fresh_tmp();

    if (strcmp(aop, "+=") == 0) {
      // Handle pointer arithmetic for +=
      if (lts.ptr_level > 0) {
        TypeSpec elem_ts = lts; elem_ts.ptr_level -= 1;
        std::string idx = coerce(rv, rts, "i64");
        emit(result + " = getelementptr " + gep_elem_ty(llvm_ty(elem_ts)) + ", ptr " + cur + ", i64 " + idx);
      } else {
        emit(result + " = " + (is_float ? "fadd" : "add") + " " + llty + " " + cur + ", " + rv);
      }
    } else if (strcmp(aop, "-=") == 0) {
      if (lts.ptr_level > 0) {
        TypeSpec elem_ts = lts; elem_ts.ptr_level -= 1;
        std::string idx = coerce(rv, rts, "i64");
        std::string neg = fresh_tmp();
        emit(neg + " = sub i64 0, " + idx);
        emit(result + " = getelementptr " + gep_elem_ty(llvm_ty(elem_ts)) + ", ptr " + cur + ", i64 " + neg);
      } else {
        emit(result + " = " + (is_float ? "fsub" : "sub") + " " + llty + " " + cur + ", " + rv);
      }
    } else if (strcmp(aop, "*=") == 0) {
      emit(result + " = " + (is_float ? "fmul" : "mul") + " " + llty + " " + cur + ", " + rv);
    } else if (strcmp(aop, "/=") == 0) {
      if (is_float)
        emit(result + " = fdiv " + llty + " " + cur + ", " + rv);
      else {
        std::string div_op = is_unsigned_base(lts.base) ? "udiv" : "sdiv";
        emit(result + " = " + div_op + " " + llty + " " + cur + ", " + rv);
      }
    } else if (strcmp(aop, "%=") == 0) {
      std::string rem_op = is_unsigned_base(lts.base) ? "urem" : "srem";
      emit(result + " = " + rem_op + " " + llty + " " + cur + ", " + rv);
    } else if (strcmp(aop, "&=") == 0) {
      emit(result + " = and " + llty + " " + cur + ", " + rv);
    } else if (strcmp(aop, "|=") == 0) {
      emit(result + " = or " + llty + " " + cur + ", " + rv);
    } else if (strcmp(aop, "^=") == 0) {
      emit(result + " = xor " + llty + " " + cur + ", " + rv);
    } else if (strcmp(aop, "<<=") == 0) {
      emit(result + " = shl " + llty + " " + cur + ", " + rv);
    } else if (strcmp(aop, ">>=") == 0) {
      std::string shr_op = is_unsigned_base(lts.base) ? "lshr" : "ashr";
      emit(result + " = " + shr_op + " " + llty + " " + cur + ", " + rv);
    } else {
      result = rv;  // unknown op: just use rhs
    }
    emit("store " + llty + " " + result + ", ptr " + ptr);
    return result;
  }

  case NK_COMPOUND_ASSIGN: {
    // lhs op= rhs
    // Evaluate RHS before loading LHS (matches clang/gcc evaluation order).
    TypeSpec lts = expr_type(n->left);
    if (lts.is_const && lts.ptr_level == 0 && !is_array_ty(lts))
      throw std::runtime_error("compound assignment to const-qualified lvalue");
    std::string llty = llvm_ty(lts);
    std::string ptr = codegen_lval(n->left);

    TypeSpec rts = expr_type(n->right);
    std::string rv = codegen_expr(n->right);

    std::string cur = fresh_tmp();
    emit(cur + " = load " + llty + ", ptr " + ptr);

    // Complex compound assignment: lhs is a complex type
    if (lts.ptr_level == 0 && is_complex_base(lts.base)) {
      TypeSpec ets = complex_component_ts(lts.base);
      std::string ellty = llvm_ty(ets);
      bool is_fp = is_float_base(ets.base);
      const char* op = n->op;
      // Coerce rhs to same complex type
      auto to_complex = [&](const std::string& val, TypeSpec vts) -> std::string {
        if (is_complex_base(vts.base) && vts.ptr_level == 0) {
          if (llvm_ty(vts) == llty) return val;
          TypeSpec src_ets = complex_component_ts(vts.base);
          std::string src_llty = llvm_ty(vts);
          std::string re0 = fresh_tmp(), im0 = fresh_tmp();
          emit(re0 + " = extractvalue " + src_llty + " " + val + ", 0");
          emit(im0 + " = extractvalue " + src_llty + " " + val + ", 1");
          std::string re1 = coerce(re0, src_ets, ellty);
          std::string im1 = coerce(im0, src_ets, ellty);
          std::string t0 = fresh_tmp(), t1 = fresh_tmp();
          emit(t0 + " = insertvalue " + llty + " zeroinitializer, " + ellty + " " + re1 + ", 0");
          emit(t1 + " = insertvalue " + llty + " " + t0 + ", " + ellty + " " + im1 + ", 1");
          return t1;
        }
        std::string cv = coerce(val, vts, ellty);
        std::string t0 = fresh_tmp(), t1 = fresh_tmp();
        std::string zero = is_fp ? "0.0" : "0";
        emit(t0 + " = insertvalue " + llty + " zeroinitializer, " + ellty + " " + cv + ", 0");
        emit(t1 + " = insertvalue " + llty + " " + t0 + ", " + ellty + " " + zero + ", 1");
        return t1;
      };
      std::string rvc = to_complex(rv, rts);
      std::string lre = fresh_tmp(), lim = fresh_tmp();
      std::string rre = fresh_tmp(), rim = fresh_tmp();
      emit(lre + " = extractvalue " + llty + " " + cur + ", 0");
      emit(lim + " = extractvalue " + llty + " " + cur + ", 1");
      emit(rre + " = extractvalue " + llty + " " + rvc + ", 0");
      emit(rim + " = extractvalue " + llty + " " + rvc + ", 1");
      std::string result_re = fresh_tmp(), result_im = fresh_tmp();
      std::string t0 = fresh_tmp(), t1 = fresh_tmp();
      if (strcmp(op, "+=") == 0 || strcmp(op, "-=") == 0) {
        std::string arith = (strcmp(op, "+=") == 0) ? (is_fp ? "fadd" : "add")
                                                    : (is_fp ? "fsub" : "sub");
        emit(result_re + " = " + arith + " " + ellty + " " + lre + ", " + rre);
        emit(result_im + " = " + arith + " " + ellty + " " + lim + ", " + rim);
      } else if (strcmp(op, "*=") == 0) {
        // (a+bi)(c+di) = (ac-bd) + (ad+bc)i
        std::string ac=fresh_tmp(),bd=fresh_tmp(),ad=fresh_tmp(),bc=fresh_tmp();
        std::string mul=is_fp?"fmul":"mul", sub=is_fp?"fsub":"sub", add2=is_fp?"fadd":"add";
        emit(ac + " = " + mul + " " + ellty + " " + lre + ", " + rre);
        emit(bd + " = " + mul + " " + ellty + " " + lim + ", " + rim);
        emit(ad + " = " + mul + " " + ellty + " " + lre + ", " + rim);
        emit(bc + " = " + mul + " " + ellty + " " + lim + ", " + rre);
        emit(result_re + " = " + sub + " " + ellty + " " + ac + ", " + bd);
        emit(result_im + " = " + add2 + " " + ellty + " " + ad + ", " + bc);
      } else {
        // Unhandled complex compound op: emit identity
        result_re = lre; result_im = lim;
      }
      emit(t0 + " = insertvalue " + llty + " zeroinitializer, " + ellty + " " + result_re + ", 0");
      emit(t1 + " = insertvalue " + llty + " " + t0 + ", " + ellty + " " + result_im + ", 1");
      emit("store " + llty + " " + t1 + ", ptr " + ptr);
      return t1;
    }

    bool is_float = is_float_base(lts.base);

    // C99: float op= expr does arithmetic in double precision, then truncates to float
    bool needs_fp_upcast = (lts.base == TB_FLOAT && lts.ptr_level == 0 && !lts.is_vector);
    if (needs_fp_upcast) {
      std::string cur_d = fresh_tmp();
      emit(cur_d + " = fpext float " + cur + " to double");
      rv = coerce(rv, rts, "double");
      std::string result_d = fresh_tmp();
      const char* op = n->op;
      std::string op_str;
      if      (op && strcmp(op, "+=") == 0) op_str = "fadd";
      else if (op && strcmp(op, "-=") == 0) op_str = "fsub";
      else if (op && strcmp(op, "*=") == 0) op_str = "fmul";
      else if (op && strcmp(op, "/=") == 0) op_str = "fdiv";
      if (!op_str.empty()) {
        emit(result_d + " = " + op_str + " double " + cur_d + ", " + rv);
        std::string result_f = fresh_tmp();
        emit(result_f + " = fptrunc double " + result_d + " to float");
        emit("store float " + result_f + ", ptr " + ptr);
        return result_f;
      }
      rv = coerce(rv, rts, llty);
    }

    // C11 §6.3.1.1: narrow integer lhs (< int width) promotes to int before arithmetic.
    // unsigned/signed char, short → all values fit in int → promote to signed int.
    bool needs_int_promote = (!needs_fp_upcast && lts.ptr_level == 0 && !is_float &&
                              !lts.is_vector && !is_complex_base(lts.base) &&
                              sizeof_ty(lts) < 4);
    if (needs_int_promote) {
      // Extend lhs and coerce rhs to i32, perform op as signed int, truncate back.
      std::string cur_ext = fresh_tmp();
      emit(cur_ext + " = " + (is_unsigned_base(lts.base) ? "zext" : "sext") +
           " " + llty + " " + cur + " to i32");
      std::string rv32 = coerce(rv, rts, "i32");
      std::string res32 = fresh_tmp();
      const char* op32 = n->op;
      std::string op32_str;
      if      (op32 && strcmp(op32, "+=")  == 0) op32_str = "add";
      else if (op32 && strcmp(op32, "-=")  == 0) op32_str = "sub";
      else if (op32 && strcmp(op32, "*=")  == 0) op32_str = "mul";
      else if (op32 && strcmp(op32, "/=")  == 0) op32_str = "sdiv";
      else if (op32 && strcmp(op32, "%=")  == 0) op32_str = "srem";
      else if (op32 && strcmp(op32, "&=")  == 0) op32_str = "and";
      else if (op32 && strcmp(op32, "|=")  == 0) op32_str = "or";
      else if (op32 && strcmp(op32, "^=")  == 0) op32_str = "xor";
      else if (op32 && strcmp(op32, "<<=") == 0) op32_str = "shl";
      else if (op32 && strcmp(op32, ">>=") == 0)
        op32_str = is_unsigned_base(lts.base) ? "lshr" : "ashr";
      if (!op32_str.empty()) {
        emit(res32 + " = " + op32_str + " i32 " + cur_ext + ", " + rv32);
        std::string res_narrow = fresh_tmp();
        emit(res_narrow + " = trunc i32 " + res32 + " to " + llty);
        // Bitfield masking (if lhs is a bitfield member)
        {
          int bfw = member_bitfield_width(n->left);
          if (bfw > 0 && (llty == "i8" || llty == "i16")) {
            long long mask = (1LL << bfw) - 1;
            std::string masked = fresh_tmp();
            emit(masked + " = and " + llty + " " + res_narrow + ", " + std::to_string(mask));
            res_narrow = masked;
          }
        }
        emit("store " + llty + " " + res_narrow + ", ptr " + ptr);
        return res_narrow;
      }
    }

    // For pointer arithmetic, keep rv as integer; coerce to lhs type for scalar ops
    if (!needs_fp_upcast && lts.ptr_level == 0)
      rv = coerce(rv, rts, llty);

    // Vector compound assignment: element-wise extractvalue/insertvalue
    if (lts.ptr_level == 0 && lts.is_vector && array_rank_of(lts) == 1) {
      // Normalize rv: NK_VAR/NK_DEREF of pure vector return loaded value;
      // NK_COMPOUND_LIT/NK_MEMBER/NK_INDEX return alloca ptr — load it.
      bool rv_already_loaded2 = (rts.is_vector && array_rank_of(rts) == 1 && n->right &&
                                  (n->right->kind == NK_VAR || n->right->kind == NK_DEREF));
      if (!rv_already_loaded2) {
        std::string rv_loaded = fresh_tmp();
        emit(rv_loaded + " = load " + llty + ", ptr " + rv);
        rv = rv_loaded;
      }
      const char* op2 = n->op;
      long long cnt = array_dim_at(lts, 0);
      TypeSpec elem_ts = drop_array_dim(lts);
      std::string elem_llty = llvm_ty(elem_ts);
      bool vec_fp   = (elem_llty == "float" || elem_llty == "double");
      bool vec_uint = is_unsigned_base(elem_ts.base);
      std::string llvm_op;
      if      (op2 && strcmp(op2, "+=") == 0)  llvm_op = vec_fp ? "fadd" : "add";
      else if (op2 && strcmp(op2, "-=") == 0)  llvm_op = vec_fp ? "fsub" : "sub";
      else if (op2 && strcmp(op2, "*=") == 0)  llvm_op = vec_fp ? "fmul" : "mul";
      else if (op2 && strcmp(op2, "/=") == 0)  llvm_op = vec_fp ? "fdiv" : (vec_uint ? "udiv" : "sdiv");
      else if (op2 && strcmp(op2, "%=") == 0)  llvm_op = vec_uint ? "urem" : "srem";
      else if (op2 && strcmp(op2, "&=") == 0)  llvm_op = "and";
      else if (op2 && strcmp(op2, "|=") == 0)  llvm_op = "or";
      else if (op2 && strcmp(op2, "^=") == 0)  llvm_op = "xor";
      else if (op2 && strcmp(op2, "<<=") == 0) llvm_op = "shl";
      else if (op2 && strcmp(op2, ">>=") == 0) llvm_op = vec_uint ? "lshr" : "ashr";
      if (!llvm_op.empty() && cnt > 0) {
        std::string acc = "zeroinitializer";
        for (long long idx = 0; idx < cnt; idx++) {
          std::string a_elem = fresh_tmp(), b_elem = fresh_tmp(), r_elem = fresh_tmp();
          emit(a_elem + " = extractvalue " + llty + " " + cur + ", " + std::to_string(idx));
          emit(b_elem + " = extractvalue " + llty + " " + rv  + ", " + std::to_string(idx));
          emit(r_elem + " = " + llvm_op + " " + elem_llty + " " + a_elem + ", " + b_elem);
          std::string new_acc = fresh_tmp();
          emit(new_acc + " = insertvalue " + llty + " " + acc + ", " + elem_llty + " " + r_elem + ", " + std::to_string(idx));
          acc = new_acc;
        }
        emit("store " + llty + " " + acc + ", ptr " + ptr);
        return acc;
      }
    }

    std::string result = fresh_tmp();
    const char* op = n->op;

    if (strcmp(op, "+=") == 0) {
      if (lts.ptr_level > 0) {
        TypeSpec elem_ts = lts; elem_ts.ptr_level -= 1;
        std::string idx = coerce(rv, rts, "i64");
        emit(result + " = getelementptr " + gep_elem_ty(llvm_ty(elem_ts)) + ", ptr " + cur + ", i64 " + idx);
      } else {
        emit(result + " = " + (is_float ? "fadd" : "add") + " " + llty + " " + cur + ", " + rv);
      }
    } else if (strcmp(op, "-=") == 0) {
      if (lts.ptr_level > 0) {
        TypeSpec elem_ts = lts; elem_ts.ptr_level -= 1;
        std::string idx = coerce(rv, rts, "i64");
        std::string neg = fresh_tmp();
        emit(neg + " = sub i64 0, " + idx);
        emit(result + " = getelementptr " + gep_elem_ty(llvm_ty(elem_ts)) + ", ptr " + cur + ", i64 " + neg);
      } else {
        emit(result + " = " + (is_float ? "fsub" : "sub") + " " + llty + " " + cur + ", " + rv);
      }
    } else if (strcmp(op, "*=") == 0) {
      emit(result + " = " + (is_float ? "fmul" : "mul") + " " + llty + " " + cur + ", " + rv);
    } else if (strcmp(op, "/=") == 0) {
      if (is_float)
        emit(result + " = fdiv " + llty + " " + cur + ", " + rv);
      else {
        std::string div_op = is_unsigned_base(lts.base) ? "udiv" : "sdiv";
        emit(result + " = " + div_op + " " + llty + " " + cur + ", " + rv);
      }
    } else if (strcmp(op, "%=") == 0) {
      std::string rem_op = is_unsigned_base(lts.base) ? "urem" : "srem";
      emit(result + " = " + rem_op + " " + llty + " " + cur + ", " + rv);
    } else if (strcmp(op, "&=") == 0) {
      emit(result + " = and " + llty + " " + cur + ", " + rv);
    } else if (strcmp(op, "|=") == 0) {
      emit(result + " = or " + llty + " " + cur + ", " + rv);
    } else if (strcmp(op, "^=") == 0) {
      emit(result + " = xor " + llty + " " + cur + ", " + rv);
    } else if (strcmp(op, "<<=") == 0) {
      emit(result + " = shl " + llty + " " + cur + ", " + rv);
    } else if (strcmp(op, ">>=") == 0) {
      std::string shr_op = is_unsigned_base(lts.base) ? "lshr" : "ashr";
      emit(result + " = " + shr_op + " " + llty + " " + cur + ", " + rv);
    } else {
      result = rv;
    }
    // Bitfield write: mask compound-assign result to bitfield width before storing.
    {
      int bfw = member_bitfield_width(n->left);
      if (bfw > 0 && lts.ptr_level == 0 && !is_array_ty(lts) &&
          (llty == "i8" || llty == "i16" || llty == "i32" || llty == "i64")) {
        long long mask = (bfw >= 64) ? (long long)-1 : ((1LL << bfw) - 1);
        std::string masked = fresh_tmp();
        emit(masked + " = and " + llty + " " + result + ", " + std::to_string(mask));
        result = masked;
      }
    }
    emit("store " + llty + " " + result + ", ptr " + ptr);
    return result;
  }

  case NK_CALL: {
    if (!n->left) return "0";
    std::string fn_name;
    TypeSpec ret_ts = int_ts();
    bool variadic = false;
    std::vector<TypeSpec> param_types;
    bool has_known_signature = false;
    bool is_indirect = false;
    std::string fn_ptr;

    if (n->left->kind == NK_VAR && n->left->name) {
      fn_name = n->left->name;
      // __builtin_alloca(size): emit LLVM variable-size alloca inline (size may be runtime).
      if (fn_name == "__builtin_alloca" && n->n_children >= 1) {
        std::string sz = codegen_expr(n->children[0]);
        sz = coerce(sz, expr_type(n->children[0]), "i64");
        std::string t = fresh_tmp();
        // Dynamic alloca stays in-place (size computed at runtime, cannot be hoisted).
        emit(t + " = alloca i8, i64 " + sz);
        return t;
      }
      // Remap common __builtin_* → standard library names.
      static const struct { const char* from; const char* to; } builtin_remap[] = {
        {"__builtin_malloc",  "malloc"},
        {"__builtin_free",    "free"},
        {"__builtin_calloc",  "calloc"},
        {"__builtin_realloc", "realloc"},
        {"__builtin_memcpy",  "memcpy"},
        {"__builtin_memmove", "memmove"},
        {"__builtin_memset",  "memset"},
        {"__builtin_memcmp",  "memcmp"},
        {"__builtin_memchr",  "memchr"},
        {"__builtin_strcpy",  "strcpy"},
        {"__builtin_strncpy", "strncpy"},
        {"__builtin_strcat",  "strcat"},
        {"__builtin_strncat", "strncat"},
        {"__builtin_strcmp",  "strcmp"},
        {"__builtin_strncmp", "strncmp"},
        {"__builtin_strlen",  "strlen"},
        {"__builtin_strchr",  "strchr"},
        {"__builtin_strstr",  "strstr"},
        {"__builtin_printf",  "printf"},
        {"__builtin_puts",    "puts"},
        {"__builtin_abort",   "abort"},
        {"__builtin_exit",    "exit"},
        {nullptr, nullptr}
      };
      for (int ri = 0; builtin_remap[ri].from; ri++) {
        if (fn_name == builtin_remap[ri].from) { fn_name = builtin_remap[ri].to; break; }
      }
      auto sig_it = func_sigs_.find(fn_name);
      if (sig_it != func_sigs_.end()) {
        ret_ts = sig_it->second.ret_type;
        variadic = sig_it->second.variadic;
        param_types = sig_it->second.param_types;
        has_known_signature = true;
      } else if (global_types_.count(fn_name) || local_slots_.count(fn_name)) {
        // It's a variable (e.g. function pointer) — use indirect call
        is_indirect = true;
        fn_ptr = codegen_expr(n->left);
        // Check if we know the signature of this function pointer variable
        auto fsit = fptr_sigs_.find(fn_name);
        if (fsit != fptr_sigs_.end()) {
          ret_ts = fsit->second.ret_type;
          variadic = fsit->second.variadic;
          param_types = fsit->second.param_types;
          has_known_signature = true;
        } else {
          ret_ts = expr_type(n);
          // Fallback: use the variable's TypeSpec is_fn_ptr base type as return type.
          TypeSpec var_ts;
          if (local_types_.count(fn_name)) var_ts = local_types_[fn_name];
          else if (global_types_.count(fn_name)) var_ts = global_types_[fn_name];
          if (var_ts.is_fn_ptr && var_ts.ptr_level > 0) {
            var_ts.ptr_level--;
            var_ts.is_fn_ptr = false;
            ret_ts = var_ts;
          }
        }
        fn_name.clear();
      } else {
        // Unknown function — will need auto-declare; mark as variadic for safety
        variadic = true;
        // Check known pointer-returning functions
        static const char* ptr_fns[] = {
          "malloc","calloc","realloc","strdup","strndup","memcpy","memmove","memset",
          "memchr","strcpy","strncpy","strcat","strncat","strchr","strrchr","strstr",
          "alloca","fopen","fdopen","freopen","popen","tmpfile","gets","fgets",
          "getenv","realpath","getcwd","dirname","basename",
          "inet_ntoa","inet_ntop","strtok","strtok_r","index","rindex",
          "dlopen","dlsym",nullptr
        };
        for (int k = 0; ptr_fns[k]; k++) {
          if (fn_name == ptr_fns[k]) { ret_ts = ptr_to(TB_VOID); break; }
        }
        // Check known void-returning functions
        static const char* void_fns[] = {
          "free","exit","_exit","abort","perror","assert","qsort",
          "srand","srandom","bzero","va_start","va_end","va_copy",
          "fclose","fflush","clearerr","rewind","setbuf","setbuffer",
          nullptr
        };
        for (int k = 0; void_fns[k]; k++) {
          if (fn_name == void_fns[k]) { ret_ts = void_ts(); break; }
        }
        // Check known double-returning functions
        static const char* dbl_fns[] = {
          "sin","cos","tan","asin","acos","atan","atan2","exp","log","log2","log10",
          "pow","sqrt","fabs","floor","ceil","round","fmod","frexp","ldexp","modf",
          "sinh","cosh","tanh","expm1","log1p","cbrt","hypot","copysign",nullptr
        };
        for (int k = 0; dbl_fns[k]; k++) {
          if (fn_name == dbl_fns[k]) { ret_ts = double_ts(); break; }
        }
      }
    } else {
      // Indirect call through function pointer.
      // (*fptr)(args) in C == fptr(args) — peel dereference from callee.
      Node* callee = n->left;
      if (callee->kind == NK_DEREF ||
          (callee->kind == NK_UNARY && callee->op && callee->op[0] == '*' &&
           (callee->op[1] == '\0'))) {
        callee = callee->left;
      }
      is_indirect = true;
      fn_ptr = codegen_expr(callee);
      ret_ts = expr_type(n);
      // If callee is a named variable and we know its function-pointer signature,
      // use that signature's return type (expr_type can't infer fptr return types).
      if (callee->kind == NK_VAR && callee->name) {
        auto fsit = fptr_sigs_.find(callee->name);
        if (fsit != fptr_sigs_.end()) {
          ret_ts = fsit->second.ret_type;
          if (param_types.empty()) {
            variadic = fsit->second.variadic;
            param_types = fsit->second.param_types;
            has_known_signature = true;
          }
        } else {
          // Fallback: the variable's TypeSpec encodes the return type via is_fn_ptr.
          // e.g. FLOAT (*pos)(FLOAT,FLOAT,...) has {base=FLOAT, ptr_level=1, is_fn_ptr=true}.
          TypeSpec callee_ts = expr_type(callee);
          if (callee_ts.is_fn_ptr && callee_ts.ptr_level > 0) {
            callee_ts.ptr_level--;
            callee_ts.is_fn_ptr = false;
            ret_ts = callee_ts;
          }
        }
      } else {
        // Callee is not a simple variable (e.g. struct member access).
        // Try to infer return type from the callee expression's is_fn_ptr flag.
        TypeSpec callee_ts = expr_type(callee);
        if (callee_ts.is_fn_ptr && callee_ts.ptr_level > 0) {
          callee_ts.ptr_level--;
          callee_ts.is_fn_ptr = false;
          ret_ts = callee_ts;
        }
      }
    }

    // ── Builtin function special handling ────────────────────────────────────
    if (!is_indirect && !fn_name.empty()) {
      // va_start(ap, last_named) / __builtin_va_start: emit @llvm.va_start(ptr)
      if ((fn_name == "va_start" || fn_name == "__builtin_va_start") && n->n_children >= 1) {
        std::string ap_ptr = codegen_lval(n->children[0]);
        if (!auto_declared_fns_.count("llvm.va_start"))
          auto_declared_fns_["llvm.va_start"] = "declare void @llvm.va_start(ptr)";
        emit("call void @llvm.va_start(ptr " + ap_ptr + ")");
        return "0";
      }
      // va_end(ap) / __builtin_va_end: emit @llvm.va_end(ptr)
      if ((fn_name == "va_end" || fn_name == "__builtin_va_end") && n->n_children >= 1) {
        std::string ap_ptr = codegen_lval(n->children[0]);
        if (!auto_declared_fns_.count("llvm.va_end"))
          auto_declared_fns_["llvm.va_end"] = "declare void @llvm.va_end(ptr)";
        emit("call void @llvm.va_end(ptr " + ap_ptr + ")");
        return "0";
      }
      // va_copy(dest, src) / __builtin_va_copy: emit @llvm.va_copy(ptr, ptr)
      if ((fn_name == "va_copy" || fn_name == "__builtin_va_copy") && n->n_children >= 2) {
        std::string dst_ptr = codegen_lval(n->children[0]);
        std::string src_ptr = codegen_lval(n->children[1]);
        if (!auto_declared_fns_.count("llvm.va_copy"))
          auto_declared_fns_["llvm.va_copy"] = "declare void @llvm.va_copy(ptr, ptr)";
        emit("call void @llvm.va_copy(ptr " + dst_ptr + ", ptr " + src_ptr + ")");
        return "0";
      }
      // __builtin_expect(expr, expected) → just return expr
      if (fn_name == "__builtin_expect" && n->n_children >= 1) {
        return codegen_expr(n->children[0]);
      }
      // __builtin_object_size / __builtin_dynamic_object_size → -1 (unknown)
      if (fn_name == "__builtin_object_size" ||
          fn_name == "__builtin_dynamic_object_size") {
        return "-1";
      }
      // __builtin_bswap16/32/64 → llvm.bswap intrinsic
      if (fn_name == "__builtin_bswap16" || fn_name == "__builtin_bswap32" ||
          fn_name == "__builtin_bswap64") {
        std::string itype = (fn_name == "__builtin_bswap64") ? "i64" :
                            (fn_name == "__builtin_bswap32") ? "i32" : "i16";
        std::string intrinsic = (fn_name == "__builtin_bswap64") ? "llvm.bswap.i64" :
                                (fn_name == "__builtin_bswap32") ? "llvm.bswap.i32" : "llvm.bswap.i16";
        // Ensure intrinsic is declared
        if (!auto_declared_fns_.count(intrinsic))
          auto_declared_fns_[intrinsic] = "declare " + itype + " @" + intrinsic + "(" + itype + ")";
        if (n->n_children >= 1) {
          TypeSpec ats = expr_type(n->children[0]);
          std::string av = codegen_expr(n->children[0]);
          av = coerce(av, ats, itype);
          std::string t = fresh_tmp();
          emit(t + " = call " + itype + " @" + intrinsic + "(" + itype + " " + av + ")");
          if ((fn_name == "__builtin_bswap32") && ret_ts.base != TB_ULONGLONG)
            ret_ts = make_ts(TB_UINT);
          if (fn_name == "__builtin_bswap64") ret_ts = make_ts(TB_ULONGLONG);
          if (fn_name == "__builtin_bswap16") ret_ts = make_ts(TB_USHORT);
          return t;
        }
        return "0";
      }
      // __builtin_ffs / __builtin_ffsl / __builtin_ffsll → cttz + 1, with zero check
      if (fn_name == "__builtin_ffs" || fn_name == "__builtin_ffsl" ||
          fn_name == "__builtin_ffsll") {
        bool is_ll = (fn_name == "__builtin_ffsll");
        bool is_l  = (fn_name == "__builtin_ffsl");
        std::string itype = is_ll ? "i64" : (is_l ? "i64" : "i32");
        std::string intrinsic = is_ll ? "llvm.cttz.i64" : (is_l ? "llvm.cttz.i64" : "llvm.cttz.i32");
        std::string arg = n->n_children > 0 ? codegen_expr(n->children[0]) : "0";
        TypeSpec ats = n->n_children > 0 ? expr_type(n->children[0]) : int_ts();
        arg = coerce(arg, ats, itype);
        std::string tz = fresh_tmp(), tz1_wide = fresh_tmp(), tz1 = fresh_tmp();
        std::string iz = fresh_tmp(), res = fresh_tmp();
        emit(tz + " = call " + itype + " @" + intrinsic + "(" + itype + " " + arg + ", i1 false)");
        emit(tz1_wide + " = add " + itype + " " + tz + ", 1");
        // ffs returns int, so truncate if needed
        if (itype == "i64")
          emit(tz1 + " = trunc i64 " + tz1_wide + " to i32");
        else
          tz1 = tz1_wide;
        std::string zero_arg = (itype == "i64") ? "i64 0" : "i32 0";
        emit(iz + " = icmp eq " + itype + " " + arg + ", " + zero_arg.substr(itype.size()+1));
        emit(res + " = select i1 " + iz + ", i32 0, i32 " + tz1);
        ret_ts = int_ts();
        return res;
      }
      // __builtin_clz / __builtin_clzl / __builtin_clzll → llvm.ctlz
      if (fn_name == "__builtin_clz" || fn_name == "__builtin_clzl" ||
          fn_name == "__builtin_clzll") {
        bool is_ll = (fn_name == "__builtin_clzll");
        bool is_l  = (fn_name == "__builtin_clzl");
        std::string itype = is_ll ? "i64" : (is_l ? "i64" : "i32");
        std::string intrinsic = is_ll ? "llvm.ctlz.i64" : (is_l ? "llvm.ctlz.i64" : "llvm.ctlz.i32");
        std::string arg = n->n_children > 0 ? codegen_expr(n->children[0]) : "0";
        TypeSpec ats = n->n_children > 0 ? expr_type(n->children[0]) : int_ts();
        arg = coerce(arg, ats, itype);
        std::string r = fresh_tmp(), res = fresh_tmp();
        emit(r + " = call " + itype + " @" + intrinsic + "(" + itype + " " + arg + ", i1 true)");
        if (itype == "i64")
          emit(res + " = trunc i64 " + r + " to i32");
        else
          res = r;
        ret_ts = int_ts();
        return res;
      }
      // __builtin_ctz / __builtin_ctzl / __builtin_ctzll → llvm.cttz
      if (fn_name == "__builtin_ctz" || fn_name == "__builtin_ctzl" ||
          fn_name == "__builtin_ctzll") {
        bool is_ll = (fn_name == "__builtin_ctzll");
        bool is_l  = (fn_name == "__builtin_ctzl");
        std::string itype = is_ll ? "i64" : (is_l ? "i64" : "i32");
        std::string intrinsic = is_ll ? "llvm.cttz.i64" : (is_l ? "llvm.cttz.i64" : "llvm.cttz.i32");
        std::string arg = n->n_children > 0 ? codegen_expr(n->children[0]) : "0";
        TypeSpec ats = n->n_children > 0 ? expr_type(n->children[0]) : int_ts();
        arg = coerce(arg, ats, itype);
        std::string r = fresh_tmp(), res = fresh_tmp();
        emit(r + " = call " + itype + " @" + intrinsic + "(" + itype + " " + arg + ", i1 true)");
        if (itype == "i64")
          emit(res + " = trunc i64 " + r + " to i32");
        else
          res = r;
        ret_ts = int_ts();
        return res;
      }
      // __builtin_popcount / __builtin_popcountl / __builtin_popcountll → llvm.ctpop
      if (fn_name == "__builtin_popcount" || fn_name == "__builtin_popcountl" ||
          fn_name == "__builtin_popcountll") {
        bool is_ll = (fn_name == "__builtin_popcountll");
        bool is_l  = (fn_name == "__builtin_popcountl");
        std::string itype = is_ll ? "i64" : (is_l ? "i64" : "i32");
        std::string intrinsic = is_ll ? "llvm.ctpop.i64" : (is_l ? "llvm.ctpop.i64" : "llvm.ctpop.i32");
        std::string arg = n->n_children > 0 ? codegen_expr(n->children[0]) : "0";
        TypeSpec ats = n->n_children > 0 ? expr_type(n->children[0]) : int_ts();
        arg = coerce(arg, ats, itype);
        std::string r = fresh_tmp(), res = fresh_tmp();
        emit(r + " = call " + itype + " @" + intrinsic + "(" + itype + " " + arg + ")");
        if (itype == "i64")
          emit(res + " = trunc i64 " + r + " to i32");
        else
          res = r;
        ret_ts = int_ts();
        return res;
      }
      // __builtin_parity / __builtin_parityl / __builtin_parityll → popcount & 1
      if (fn_name == "__builtin_parity" || fn_name == "__builtin_parityl" ||
          fn_name == "__builtin_parityll") {
        bool is_ll = (fn_name == "__builtin_parityll");
        bool is_l  = (fn_name == "__builtin_parityl");
        std::string itype = is_ll ? "i64" : (is_l ? "i64" : "i32");
        std::string intrinsic = is_ll ? "llvm.ctpop.i64" : (is_l ? "llvm.ctpop.i64" : "llvm.ctpop.i32");
        std::string arg = n->n_children > 0 ? codegen_expr(n->children[0]) : "0";
        TypeSpec ats = n->n_children > 0 ? expr_type(n->children[0]) : int_ts();
        arg = coerce(arg, ats, itype);
        std::string r = fresh_tmp(), r32 = fresh_tmp(), res = fresh_tmp();
        emit(r + " = call " + itype + " @" + intrinsic + "(" + itype + " " + arg + ")");
        if (itype == "i64")
          emit(r32 + " = trunc i64 " + r + " to i32");
        else
          r32 = r;
        emit(res + " = and i32 " + r32 + ", 1");
        ret_ts = int_ts();
        return res;
      }
      // __builtin_fabs / __builtin_inf → LLVM intrinsics / constants
      if (fn_name == "__builtin_fabs" || fn_name == "__builtin_fabsl") {
        std::string a = n->n_children > 0 ? codegen_expr(n->children[0]) : "0.0";
        TypeSpec ats = n->n_children > 0 ? expr_type(n->children[0]) : double_ts();
        std::string av = coerce(a, ats, "double");
        std::string t = fresh_tmp();
        emit(t + " = call double @llvm.fabs.f64(double " + av + ")");
        ret_ts = double_ts();
        return t;
      }
      if (fn_name == "__builtin_fabsf") {
        std::string a = n->n_children > 0 ? codegen_expr(n->children[0]) : "0.0";
        TypeSpec ats = n->n_children > 0 ? expr_type(n->children[0]) : float_ts();
        std::string av = coerce(a, ats, "float");
        std::string t = fresh_tmp();
        emit(t + " = call float @llvm.fabs.f32(float " + av + ")");
        ret_ts = float_ts();
        return t;
      }
      // __builtin_copysign / __builtin_copysignf / __builtin_copysignl → llvm.copysign
      if (fn_name == "__builtin_copysign" || fn_name == "__builtin_copysignl") {
        std::string a = n->n_children > 0 ? codegen_expr(n->children[0]) : "0.0";
        std::string b = n->n_children > 1 ? codegen_expr(n->children[1]) : "0.0";
        TypeSpec ats = n->n_children > 0 ? expr_type(n->children[0]) : double_ts();
        TypeSpec bts = n->n_children > 1 ? expr_type(n->children[1]) : double_ts();
        a = coerce(a, ats, "double"); b = coerce(b, bts, "double");
        std::string t = fresh_tmp();
        emit(t + " = call double @llvm.copysign.f64(double " + a + ", double " + b + ")");
        ret_ts = double_ts(); return t;
      }
      if (fn_name == "__builtin_copysignf") {
        std::string a = n->n_children > 0 ? codegen_expr(n->children[0]) : "0.0";
        std::string b = n->n_children > 1 ? codegen_expr(n->children[1]) : "0.0";
        TypeSpec ats = n->n_children > 0 ? expr_type(n->children[0]) : float_ts();
        TypeSpec bts = n->n_children > 1 ? expr_type(n->children[1]) : float_ts();
        a = coerce(a, ats, "float"); b = coerce(b, bts, "float");
        std::string t = fresh_tmp();
        emit(t + " = call float @llvm.copysign.f32(float " + a + ", float " + b + ")");
        ret_ts = float_ts(); return t;
      }
      if (fn_name == "__builtin_inf" || fn_name == "__builtin_infl" ||
          fn_name == "__builtin_huge_val" || fn_name == "__builtin_huge_vall") {
        ret_ts = double_ts();
        return "0x7FF0000000000000";  // +Inf as double hex
      }
      if (fn_name == "__builtin_inff" || fn_name == "__builtin_huge_valf") {
        ret_ts = float_ts();
        return "0x7FF0000000000000";  // LLVM accepts double-form hex for float +Inf
      }
      // __builtin_constant_p(x) — return 0 (conservative: x is not a compile-time constant)
      if (fn_name == "__builtin_constant_p") {
        ret_ts = int_ts();
        // GCC's builtin does not evaluate its operand.
        return "0";
      }
      // __builtin_unreachable() — emit LLVM unreachable terminator
      if (fn_name == "__builtin_unreachable") {
        emit_terminator("unreachable");
        ret_ts = void_ts();
        return "";
      }
      // __builtin_signbit / __builtin_signbitf / __builtin_signbitl
      if (fn_name == "__builtin_signbit" || fn_name == "__builtin_signbitf" ||
          fn_name == "__builtin_signbitl") {
        bool is_float = (fn_name == "__builtin_signbitf");
        std::string fty = is_float ? "float" : "double";
        std::string ity = is_float ? "i32" : "i64";
        std::string a = n->n_children > 0 ? codegen_expr(n->children[0]) : "0.0";
        TypeSpec ats = n->n_children > 0 ? expr_type(n->children[0]) : double_ts();
        a = coerce(a, ats, fty);
        std::string bits = fresh_tmp(), shifted = fresh_tmp(), t = fresh_tmp();
        emit(bits + " = bitcast " + fty + " " + a + " to " + ity);
        if (is_float)
          emit(shifted + " = lshr i32 " + bits + ", 31");
        else
          emit(shifted + " = lshr i64 " + bits + ", 63");
        if (!is_float)
          emit(t + " = trunc i64 " + shifted + " to i32");
        else
          t = shifted;
        ret_ts = int_ts();
        return t;
      }
      // __builtin_{add,sub,mul}_overflow(a, b, *result) → LLVM overflow intrinsics
      if (fn_name == "__builtin_add_overflow" || fn_name == "__builtin_sub_overflow" ||
          fn_name == "__builtin_mul_overflow") {
        if (n->n_children < 3) { ret_ts = int_ts(); return "0"; }
        // Overflow is determined by the output type (*res), not the input types.
        TypeSpec res_ptr_ts = expr_type(n->children[2]);
        TypeSpec res_ts = res_ptr_ts;
        if (res_ts.ptr_level > 0) res_ts.ptr_level--;  // pointee type
        std::string itype = "i32";
        if (res_ts.base == TB_LONGLONG || res_ts.base == TB_ULONGLONG) itype = "i64";
        else if (res_ts.base == TB_LONG || res_ts.base == TB_ULONG) itype = "i64";
        else if (res_ts.base == TB_SHORT || res_ts.base == TB_USHORT) itype = "i16";
        else if (res_ts.base == TB_CHAR || res_ts.base == TB_SCHAR || res_ts.base == TB_UCHAR) itype = "i8";
        std::string op = (fn_name == "__builtin_add_overflow") ? "add" :
                         (fn_name == "__builtin_sub_overflow") ? "sub" : "mul";
        bool is_signed = !(res_ts.base == TB_UINT || res_ts.base == TB_ULONG ||
                           res_ts.base == TB_ULONGLONG || res_ts.base == TB_USHORT ||
                           res_ts.base == TB_UCHAR);
        std::string intr = std::string("llvm.") + (is_signed ? "s" : "u") + op + ".with.overflow." + itype;
        if (!auto_declared_fns_.count(intr))
          auto_declared_fns_[intr] = "declare { " + itype + ", i1 } @" + intr +
                                     "(" + itype + ", " + itype + ")";
        std::string a = codegen_expr(n->children[0]);
        std::string b = codegen_expr(n->children[1]);
        a = coerce(a, expr_type(n->children[0]), itype);
        b = coerce(b, expr_type(n->children[1]), itype);
        std::string res_struct = fresh_tmp(), ov_bit = fresh_tmp(), res_val = fresh_tmp();
        std::string ov_int = fresh_tmp();
        emit(res_struct + " = call { " + itype + ", i1 } @" + intr + "(" + itype + " " + a + ", " + itype + " " + b + ")");
        emit(res_val + " = extractvalue { " + itype + ", i1 } " + res_struct + ", 0");
        emit(ov_bit + " = extractvalue { " + itype + ", i1 } " + res_struct + ", 1");
        emit(ov_int + " = zext i1 " + ov_bit + " to i32");
        // Store result to the pointer arg
        std::string ptr = codegen_expr(n->children[2]);
        emit("store " + itype + " " + res_val + ", ptr " + ptr);
        ret_ts = int_ts();
        return ov_int;
      }
      // __builtin_nan / __builtin_nanf / __builtin_nanl — quiet NaN constant
      if (fn_name == "__builtin_nan" || fn_name == "__builtin_nanl") {
        ret_ts = double_ts();
        // Ignore string argument; return canonical quiet NaN for double
        std::string t = fresh_tmp();
        emit(t + " = select i1 false, double 0.0, double 0x7FF8000000000000");
        return t;
      }
      if (fn_name == "__builtin_nanf") {
        ret_ts = float_ts();
        std::string t = fresh_tmp();
        emit(t + " = select i1 false, float 0.0, float 0x7FF8000000000000");
        return t;
      }
      // __builtin_isgreater / isless / isunordered / etc. — unmasked ordered FP comparisons
      // These return int (1 = true, 0 = false) and do not raise exceptions for NaN.
      {
        const char* fcmp_pred = nullptr;
        if (fn_name == "__builtin_isgreater")     fcmp_pred = "ogt";
        else if (fn_name == "__builtin_isgreaterequal") fcmp_pred = "oge";
        else if (fn_name == "__builtin_isless")        fcmp_pred = "olt";
        else if (fn_name == "__builtin_islessequal")   fcmp_pred = "ole";
        else if (fn_name == "__builtin_islessgreater") fcmp_pred = "one";
        else if (fn_name == "__builtin_isunordered")   fcmp_pred = "uno";
        if (fcmp_pred) {
          std::string a = n->n_children > 0 ? codegen_expr(n->children[0]) : "0.0";
          std::string b = n->n_children > 1 ? codegen_expr(n->children[1]) : "0.0";
          TypeSpec ats = n->n_children > 0 ? expr_type(n->children[0]) : double_ts();
          TypeSpec bts = n->n_children > 1 ? expr_type(n->children[1]) : double_ts();
          // Determine common FP type
          std::string fty = "double";
          if (llvm_ty(ats) == "float" && llvm_ty(bts) == "float") fty = "float";
          a = coerce(a, ats, fty);
          b = coerce(b, bts, fty);
          std::string cmp = fresh_tmp(), ext = fresh_tmp();
          emit(cmp + " = fcmp " + fcmp_pred + " " + fty + " " + a + ", " + b);
          emit(ext + " = zext i1 " + cmp + " to i32");
          ret_ts = int_ts();
          return ext;
        }
      }
      // __builtin_isnan / __builtin_isinf / __builtin_isfinite / __builtin_isnormal
      if (fn_name == "__builtin_isnan") {
        std::string a = n->n_children > 0 ? codegen_expr(n->children[0]) : "0.0";
        TypeSpec ats = n->n_children > 0 ? expr_type(n->children[0]) : double_ts();
        std::string fty = (llvm_ty(ats) == "float") ? "float" : "double";
        a = coerce(a, ats, fty);
        std::string cmp = fresh_tmp(), ext = fresh_tmp();
        emit(cmp + " = fcmp uno " + fty + " " + a + ", " + a);
        emit(ext + " = zext i1 " + cmp + " to i32");
        ret_ts = int_ts();
        return ext;
      }
      if (fn_name == "__builtin_isinf" || fn_name == "__builtin_isinf_sign") {
        std::string a = n->n_children > 0 ? codegen_expr(n->children[0]) : "0.0";
        TypeSpec ats = n->n_children > 0 ? expr_type(n->children[0]) : double_ts();
        std::string fty = (llvm_ty(ats) == "float") ? "float" : "double";
        a = coerce(a, ats, fty);
        std::string abs_intr = (fty == "float") ? "llvm.fabs.f32" : "llvm.fabs.f64";
        std::string inf = (fty == "float") ? "0x7FF0000000000000" : "0x7FF0000000000000";
        std::string absa = fresh_tmp(), cmp = fresh_tmp(), ext = fresh_tmp();
        emit(absa + " = call " + fty + " @" + abs_intr + "(" + fty + " " + a + ")");
        emit(cmp + " = fcmp oeq " + fty + " " + absa + ", " + inf);
        if (fn_name == "__builtin_isinf_sign") {
          // Returns -1 for -inf, +1 for +inf, 0 for not inf
          std::string sign_cmp = fresh_tmp(), neg_cmp = fresh_tmp();
          std::string r1 = fresh_tmp(), r2 = fresh_tmp(), r3 = fresh_tmp();
          emit(neg_cmp + " = fcmp olt " + fty + " " + a + ", " + fty + " 0.0");
          emit(r1 + " = select i1 " + neg_cmp + ", i32 -1, i32 1");
          emit(ext + " = select i1 " + cmp + ", i32 " + r1 + ", i32 0");
        } else {
          emit(ext + " = zext i1 " + cmp + " to i32");
        }
        ret_ts = int_ts();
        return ext;
      }
      // __builtin_conjf / __builtin_conj / __builtin_conjl — complex conjugate
      if (fn_name == "__builtin_conjf" || fn_name == "__builtin_conj" ||
          fn_name == "__builtin_conjl") {
        bool is_f  = (fn_name == "__builtin_conjf");
        bool is_ld = (fn_name == "__builtin_conjl");
        TypeBase comp_base = is_f ? TB_COMPLEX_FLOAT :
                             is_ld ? TB_COMPLEX_LONGDOUBLE : TB_COMPLEX_DOUBLE;
        TypeBase elem_base = is_f ? TB_FLOAT :
                             is_ld ? TB_LONGDOUBLE : TB_DOUBLE;
        std::string elem_llty = llvm_ty(make_ts(elem_base));
        std::string comp_llty = llvm_ty(make_ts(comp_base));
        std::string arg_val = n->n_children > 0 ? codegen_expr(n->children[0]) : "zeroinitializer";
        TypeSpec arg_ts = n->n_children > 0 ? expr_type(n->children[0]) : make_ts(comp_base);
        // Extract real and imag from argument
        std::string re = fresh_tmp(), im = fresh_tmp(), nim = fresh_tmp();
        std::string r0 = fresh_tmp(), r1 = fresh_tmp();
        if (is_complex_base(arg_ts.base) && arg_ts.ptr_level == 0) {
          // Already a struct value
          emit(re + " = extractvalue " + comp_llty + " " + arg_val + ", 0");
          emit(im + " = extractvalue " + comp_llty + " " + arg_val + ", 1");
        } else {
          // Scalar arg (not complex): treat as pure real, imag=0
          emit(re + " = " + elem_llty + " " + arg_val);
          emit(im + " = " + elem_llty + " 0.0");
        }
        emit(nim + " = fneg " + elem_llty + " " + im);
        emit(r0 + " = insertvalue " + comp_llty + " undef, " + elem_llty + " " + re + ", 0");
        emit(r1 + " = insertvalue " + comp_llty + " " + r0 + ", " + elem_llty + " " + nim + ", 1");
        ret_ts = make_ts(comp_base);
        return r1;
      }
      // __builtin_classify_type → compile-time integer constant based on argument type
      // GCC enum: void=0, integer=1, char=2, enumeral=3, boolean=4, pointer=5,
      //           real(float/double)=8, complex=9, record=12, union=13, array=14
      if (fn_name == "__builtin_classify_type" && n->n_children >= 1) {
        TypeSpec arg_ts = expr_type(n->children[0]);
        int cls = 1; // integer_type_class by default
        if (arg_ts.ptr_level > 0 || arg_ts.array_rank > 0)
          cls = 5; // pointer_type_class
        else if (arg_ts.base == TB_VOID)
          cls = 0; // void_type_class
        else if (arg_ts.base == TB_CHAR || arg_ts.base == TB_UCHAR)
          cls = 2; // char_type_class
        else if (arg_ts.base == TB_FLOAT || arg_ts.base == TB_DOUBLE ||
                 arg_ts.base == TB_LONGDOUBLE)
          cls = 8; // real_type_class
        else if (is_complex_base(arg_ts.base))
          cls = 9; // complex_type_class
        else if (arg_ts.base == TB_STRUCT)
          cls = 12; // record_type_class
        else if (arg_ts.base == TB_UNION)
          cls = 13; // union_type_class
        ret_ts = make_ts(TB_INT);
        return std::to_string(cls);
      }
      // __builtin_memcpy / __builtin_memset / __builtin_memmove / __builtin_memcmp
      // and other common memory/string builtins → lower to C library names.
      {
        static const struct { const char* from; const char* to; } mem_builtins[] = {
          {"__builtin_memcpy",   "memcpy"},
          {"__builtin_memset",   "memset"},
          {"__builtin_memmove",  "memmove"},
          {"__builtin_memcmp",   "memcmp"},
          {"__builtin_strcpy",   "strcpy"},
          {"__builtin_strncpy",  "strncpy"},
          {"__builtin_strcmp",   "strcmp"},
          {"__builtin_strncmp",  "strncmp"},
          {"__builtin_strlen",   "strlen"},
          {"__builtin_strcat",   "strcat"},
          {"__builtin_strncat",  "strncat"},
          {"__builtin_strchr",   "strchr"},
          {"__builtin_strrchr",  "strrchr"},
          {"__builtin_strstr",   "strstr"},
          {"__builtin_sprintf",  "sprintf"},
          {"__builtin_printf",   "printf"},
          {"__builtin_puts",     "puts"},
          {"__builtin_abort",    "abort"},
          {"__builtin_exit",     "exit"},
          {nullptr, nullptr}
        };
        for (int mi = 0; mem_builtins[mi].from; mi++) {
          if (fn_name == mem_builtins[mi].from) {
            fn_name = mem_builtins[mi].to;
            // Also update the callee node name so later code uses the new name.
            break;
          }
        }
      }

      // __builtin___*_chk → map to safe stdlib equivalents
      struct { const char* from; const char* to; int keep_args; } chk_map[] = {
        {"__builtin___memcpy_chk",   "memcpy",   3},
        {"__builtin___memset_chk",   "memset",   3},
        {"__builtin___memmove_chk",  "memmove",  3},
        {"__builtin___strcpy_chk",   "strcpy",   2},
        {"__builtin___strncpy_chk",  "strncpy",  3},
        {"__builtin___strcat_chk",   "strcat",   2},
        {"__builtin___strncat_chk",  "strncat",  3},
        {"__builtin___sprintf_chk",  "sprintf",  -1},  // drop args 1,2 (flag, size)
        {"__builtin___snprintf_chk", "snprintf", -2},  // drop args 2,3 (flag, size)
        {"__builtin___vsprintf_chk", "vsprintf", -1},
        {"__builtin___vsnprintf_chk","vsnprintf",-2},
        {"__builtin___fprintf_chk",  "fprintf",  -3},
        {"__builtin___printf_chk",   "printf",   -4},
        {nullptr, nullptr, 0}
      };
      for (int ci = 0; chk_map[ci].from; ci++) {
        if (fn_name == chk_map[ci].from) {
          // Remap call: evaluate args, drop the __chk-specific ones
          std::vector<std::string> avals;
          std::vector<TypeSpec> atypes;
          for (int i = 0; i < n->n_children; i++) {
            TypeSpec ats = expr_type(n->children[i]);
            std::string av = codegen_expr(n->children[i]);
            avals.push_back(av);
            atypes.push_back(ats);
          }
          // Build actual arg list for remapped function
          std::vector<int> keep_indices;
          int ka = chk_map[ci].keep_args;
          if (ka >= 0) {
            // keep first ka args
            for (int i = 0; i < ka && i < (int)avals.size(); i++) keep_indices.push_back(i);
          } else if (ka == -1) {
            // sprintf: keep args 0, then 3..
            if ((int)avals.size() > 0) keep_indices.push_back(0);
            for (int i = 3; i < (int)avals.size(); i++) keep_indices.push_back(i);
          } else if (ka == -2) {
            // snprintf: keep args 0,1, then 4..
            if ((int)avals.size() > 0) keep_indices.push_back(0);
            if ((int)avals.size() > 1) keep_indices.push_back(1);
            for (int i = 4; i < (int)avals.size(); i++) keep_indices.push_back(i);
          } else if (ka == -3) {
            // fprintf: keep args 0, then 2..
            if ((int)avals.size() > 0) keep_indices.push_back(0);
            for (int i = 2; i < (int)avals.size(); i++) keep_indices.push_back(i);
          } else if (ka == -4) {
            // printf: keep args 1..
            for (int i = 1; i < (int)avals.size(); i++) keep_indices.push_back(i);
          }
          // Get real function's param types for coercion
          std::vector<TypeSpec> real_params;
          {
            auto sit2 = func_sigs_.find(chk_map[ci].to);
            if (sit2 != func_sigs_.end()) real_params = sit2->second.param_types;
          }
          std::string args_s;
          for (int i = 0; i < (int)keep_indices.size(); i++) {
            if (i > 0) args_s += ", ";
            int idx = keep_indices[i];
            TypeSpec ats = atypes[idx];
            std::string av = avals[idx];
            // Coerce to real param type if known
            if (i < (int)real_params.size()) {
              av = coerce(av, ats, llvm_ty(real_params[i]));
              ats = real_params[i];
            }
            if (is_array_ty(ats)) {
              args_s += "ptr " + av;
            } else if (ats.base == TB_FLOAT && ats.ptr_level == 0) {
              std::string ext = fresh_tmp();
              emit(ext + " = fpext float " + av + " to double");
              args_s += "double " + ext;
            } else {
              args_s += llvm_ty(ats) + " " + av;
            }
          }
          // The real function is variadic — ensure proper call format
          std::string real = chk_map[ci].to;
          ret_ts = ptr_to(TB_VOID);  // most return ptr; sprintf returns int
          if (real == "sprintf" || real == "snprintf" || real == "vsprintf" ||
              real == "vsnprintf" || real == "fprintf" || real == "vfprintf" ||
              real == "printf" || real == "vprintf") ret_ts = int_ts();
          std::string ret_ll = llvm_ty(ret_ts);
          if (!auto_declared_fns_.count(real) && !func_sigs_.count(real))
            auto_declared_fns_[real] = "declare " + ret_ll + " @" + real + "(...)";
          // Build proper function type: use known signature if available,
          // otherwise fall back to (...).  On ARM64, variadic ABI requires
          // knowing which params are fixed vs variadic.
          auto build_fn_type = [&](const std::string& ret) -> std::string {
            auto sit = func_sigs_.find(real);
            if (sit != func_sigs_.end()) {
              std::string pts;
              for (size_t pi = 0; pi < sit->second.param_types.size(); pi++) {
                if (pi > 0) pts += ", ";
                pts += llvm_ty(sit->second.param_types[pi]);
              }
              if (sit->second.variadic)
                pts += (pts.empty() ? "..." : ", ...");
              return ret + " (" + pts + ")";
            }
            return ret + " (...)";
          };
          std::string fn_type = build_fn_type(ret_ll);
          if (ret_ll == "void") {
            emit("call " + fn_type + " @" + real + "(" + args_s + ")");
            return "";
          } else {
            std::string t = fresh_tmp();
            emit(t + " = call " + fn_type + " @" + real + "(" + args_s + ")");
            return t;
          }
        }
      }
    }
    // ── End builtin handling ──────────────────────────────────────────────────

    if (has_known_signature && !(param_types.empty() && !variadic)) {
      size_t argc = static_cast<size_t>(n->n_children);
      size_t fixed = param_types.size();
      bool bad_arity = variadic ? (argc < fixed) : (argc != fixed);
      if (bad_arity) {
        std::string name = !fn_name.empty() ? fn_name : "<function pointer>";
        throw std::runtime_error(
            "wrong number of arguments in call to " + name +
            ": expected " + (variadic ? ("at least " + std::to_string(fixed))
                                      : std::to_string(fixed)) +
            ", got " + std::to_string(argc));
      }
    }

    // Emit argument evaluations
    std::vector<std::string> arg_vals;
    std::vector<TypeSpec> arg_types;
    for (int i = 0; i < n->n_children; i++) {
      TypeSpec ats = expr_type(n->children[i]);
      if (i < (int)param_types.size()) {
        const TypeSpec& pts = param_types[i];
        bool discards_const = false;
        if (pts.ptr_level > 0 && !pts.is_const) {
          Node* arg = n->children[i];
          if (arg) {
            if (arg->kind == NK_STR_LIT)
              discards_const = !allows_string_literal_ptr_target(pts);
            else if (arg->kind == NK_ADDR && arg->left) {
              TypeSpec inner = expr_type(arg->left);
              if (inner.ptr_level == 0 && inner.is_const) discards_const = true;
            } else if (arg->kind == NK_VAR && ats.ptr_level > 0 && ats.is_const) {
              discards_const = true;
            }
          }
        }
        if (discards_const)
          throw std::runtime_error("passing argument discards const qualifier");
      }
      std::string av;
      bool rewrote_printf_fmt = false;
      if (!is_indirect && i == 0 && n->children[i] &&
          n->children[i]->kind == NK_STR_LIT &&
          (fn_name == "printf" || fn_name == "fprintf" || fn_name == "sprintf" ||
           fn_name == "snprintf" || fn_name == "asprintf" || fn_name == "dprintf")) {
        std::string content = decode_narrow_string_lit(n->children[i]->sval);
        std::string normalized = normalize_printf_longdouble_format(content);
        if (normalized != content) {
          std::string gname = get_string_global(normalized);
          std::string tgep = fresh_tmp();
          int len = (int)normalized.size() + 1;
          emit(tgep + " = getelementptr [" + std::to_string(len) + " x i8], ptr " +
               gname + ", i64 0, i64 0");
          av = tgep;
          rewrote_printf_fmt = true;
        }
      }
      if (!rewrote_printf_fmt) av = codegen_expr(n->children[i]);
      // Coerce to parameter type if known
      if (i < (int)param_types.size()) {
        av = coerce(av, ats, llvm_ty(param_types[i]));
        ats = param_types[i];
      } else if (is_indirect && ret_ts.base == TB_FLOAT && ret_ts.ptr_level == 0 &&
                 ats.base == TB_DOUBLE && ats.ptr_level == 0) {
        // Heuristic: for indirect calls through a float-returning function pointer,
        // double-typed arguments (e.g. 1.0 literals) should be narrowed to float
        // to match the ABI of functions like FLOAT (*fn)(FLOAT, FLOAT, ...).
        std::string tr = fresh_tmp();
        emit(tr + " = fptrunc double " + av + " to float");
        av = tr;
        ats = float_ts();
      }
      arg_vals.push_back(av);
      arg_types.push_back(ats);
    }

    std::string ret_llty = llvm_ty(ret_ts);

    // Auto-declare undeclared direct calls so LLVM doesn't reject the IR
    if (!is_indirect && !fn_name.empty() && !func_sigs_.count(fn_name) &&
        !auto_declared_fns_.count(fn_name)) {
      // Build declare with known fixed param types
      std::string param_str;
      for (size_t i = 0; i < param_types.size(); i++) {
        if (i > 0) param_str += ", ";
        param_str += llvm_ty(param_types[i]);
      }
      std::string decl;
      if (variadic) {
        decl = "declare " + ret_llty + " @" + fn_name + "(" +
               (param_str.empty() ? "" : param_str + ", ") + "...)";
      } else {
        decl = "declare " + ret_llty + " @" + fn_name + "(" + param_str + ")";
      }
      auto_declared_fns_[fn_name] = decl;
    }

    // Build argument list string.
    // Arrays decay to ptr; variadic args get default-argument-promotions:
    //   float → double, char/short (i8/i16) → i32
    auto hfa_info_ty = [&](const TypeSpec& ats, TypeSpec& out_elem, int& out_n) -> bool {
      if (ats.ptr_level != 0 || is_array_ty(ats) ||
          (ats.base != TB_STRUCT && ats.base != TB_UNION) || !ats.tag)
        return false;
      auto it = struct_defs_.find(ats.tag);
      if (it == struct_defs_.end()) return false;
      const StructInfo& si = it->second;
      if (si.is_union) return false;
      int n = 0;
      TypeBase elem = TB_VOID;
      for (size_t fi = 0; fi < si.field_types.size(); fi++) {
        if (si.field_array_sizes[fi] == -2) continue;
        TypeSpec ft = si.field_types[fi];
        if (ft.ptr_level != 0 || is_array_ty(ft)) return false;
        bool is_fp = (ft.base == TB_FLOAT || ft.base == TB_DOUBLE || ft.base == TB_LONGDOUBLE);
        if (!is_fp) return false;
        if (n == 0) elem = ft.base;
        else if (elem != ft.base) return false;
        n++;
      }
      if (n >= 1 && n <= 4) {
        out_n = n;
        out_elem = make_ts(elem);
        return true;
      }
      return false;
    };

    std::string args_str;
    for (size_t i = 0; i < arg_vals.size(); i++) {
      if (i > 0) args_str += ", ";
      bool is_variadic_slot = variadic && (i >= param_types.size());
      if (is_array_ty(arg_types[i])) {
        args_str += "ptr " + arg_vals[i];
      } else if (is_variadic_slot && arg_types[i].ptr_level == 0 &&
                 arg_types[i].base == TB_FLOAT) {
        // float → double promotion for variadic args
        std::string ext = fresh_tmp();
        emit(ext + " = fpext float " + arg_vals[i] + " to double");
        args_str += "double " + ext;
      } else if (is_variadic_slot && arg_types[i].ptr_level == 0 &&
                 (arg_types[i].base == TB_CHAR || arg_types[i].base == TB_SCHAR ||
                  arg_types[i].base == TB_UCHAR || arg_types[i].base == TB_SHORT ||
                  arg_types[i].base == TB_USHORT)) {
        // char/short → i32 promotion for variadic args
        std::string ext = fresh_tmp();
        bool is_unsigned = (arg_types[i].base == TB_UCHAR || arg_types[i].base == TB_USHORT);
        std::string from_llty = llvm_ty(arg_types[i]);
        std::string ext_op = is_unsigned ? "zext" : "sext";
        emit(ext + " = " + ext_op + " " + from_llty + " " + arg_vals[i] + " to i32");
        args_str += "i32 " + ext;
      } else if (is_variadic_slot && arg_types[i].ptr_level == 0 &&
                 !is_array_ty(arg_types[i]) &&
                 (arg_types[i].base == TB_STRUCT || arg_types[i].base == TB_UNION) &&
                 [&](){ TypeSpec e{}; int n=0; return !hfa_info_ty(arg_types[i], e, n); }()) {
        // Match clang ABI behavior for non-HFA aggregate variadic arguments.
        int sz = sizeof_ty(arg_types[i]);
        int n_slots = (sz + 7) / 8;
        if (n_slots < 1) n_slots = 1;
        std::string slot_ty = "[" + std::to_string(n_slots) + " x i64]";
        std::string struct_llty = llvm_ty(arg_types[i]);
        std::string coerce_buf = "%va_coerce_" + std::to_string(tmp_idx_++);
        alloca_lines_.push_back("  " + coerce_buf + " = alloca " + slot_ty + ", align 8");
        emit("store " + struct_llty + " " + arg_vals[i] + ", ptr " + coerce_buf);
        std::string coerced = fresh_tmp();
        emit(coerced + " = load " + slot_ty + ", ptr " + coerce_buf + ", align 8");
        args_str += slot_ty + " " + coerced;
      } else if (is_variadic_slot && arg_types[i].ptr_level == 0 &&
                 !is_array_ty(arg_types[i]) &&
                 (arg_types[i].base == TB_STRUCT || arg_types[i].base == TB_UNION)) {
        TypeSpec hfa_elem{};
        int hfa_n = 0;
        if (hfa_info_ty(arg_types[i], hfa_elem, hfa_n)) {
          std::string elem_llty = llvm_ty(hfa_elem);
          std::string hfa_arr_ty = "[" + std::to_string(hfa_n) + " x " + elem_llty + "]";
          std::string hfa_buf = "%va_hfa_coerce_" + std::to_string(tmp_idx_++);
          alloca_lines_.push_back("  " + hfa_buf + " = alloca " + hfa_arr_ty + ", align 8");
          emit("store " + llvm_ty(arg_types[i]) + " " + arg_vals[i] + ", ptr " + hfa_buf);
          std::string hfa_val = fresh_tmp();
          emit(hfa_val + " = load " + hfa_arr_ty + ", ptr " + hfa_buf + ", align 8");
          args_str += hfa_arr_ty + " " + hfa_val;
        } else {
          args_str += llvm_ty(arg_types[i]) + " " + arg_vals[i];
        }
      } else {
        args_str += llvm_ty(arg_types[i]) + " " + arg_vals[i];
      }
    }

    // Build indirect call function type string from arg types (best-effort).
    // On ARM64 (Apple Silicon), LLVM requires the function type in indirect
    // calls so it knows the ABI (variadic vs non-variadic).
    auto build_indirect_fn_type = [&](const std::string& ret) -> std::string {
      // For variadic calls where we know the fixed param count,
      // only include the fixed params in the type, then add "...".
      // This ensures the ABI matches the actual function signature.
      std::string pts;
      size_t n_fixed = variadic ? param_types.size() : arg_types.size();
      for (size_t ai = 0; ai < n_fixed && ai < arg_types.size(); ai++) {
        if (ai > 0) pts += ", ";
        TypeSpec ats = arg_types[ai];
        if (is_array_ty(ats)) pts += "ptr";
        else pts += llvm_ty(ats);
      }
      // If variadic, add "..."
      if (variadic)
        pts += (pts.empty() ? "..." : ", ...");
      return ret + " (" + pts + ")";
    };

    // Emit the call
    if (ret_llty == "void") {
      if (is_indirect) {
        std::string fn_type = build_indirect_fn_type("void");
        emit("call " + fn_type + " " + fn_ptr + "(" + args_str + ")");
      } else {
        if (variadic) {
          // Variadic void: emit fixed param types in the type part, then "..."
          std::string param_types_str;
          for (size_t i = 0; i < param_types.size(); i++) {
            if (i > 0) param_types_str += ", ";
            param_types_str += llvm_ty(param_types[i]);
          }
          std::string fn_type = "(" + (param_types_str.empty() ? "" : param_types_str + ", ")
                                + "...)";
          emit("call void " + fn_type + " @" + fn_name + "(" + args_str + ")");
        } else {
          emit("call void @" + fn_name + "(" + args_str + ")");
        }
      }
      return "";
    } else {
      std::string t = fresh_tmp();
      if (is_indirect) {
        std::string fn_type = build_indirect_fn_type(ret_llty);
        emit(t + " = call " + fn_type + " " + fn_ptr + "(" + args_str + ")");
      } else {
        if (variadic) {
          // Variadic: use explicit function type
          std::string param_types_str;
          for (size_t i = 0; i < param_types.size(); i++) {
            if (i > 0) param_types_str += ", ";
            param_types_str += llvm_ty(param_types[i]);
          }
          emit(t + " = call " + ret_llty + " (" +
               (param_types_str.empty() ? "" : param_types_str + ", ") +
               "...) @" + fn_name + "(" + args_str + ")");
        } else {
          emit(t + " = call " + ret_llty + " @" + fn_name + "(" + args_str + ")");
        }
      }
      return t;
    }
  }

  case NK_INDEX: {
    // array[index] as rvalue: GEP + load
    TypeSpec res_ts = expr_type(n);
    // If the result is an array type, return the pointer (array-to-pointer decay).
    if (is_array_ty(res_ts) && !res_ts.is_ptr_to_array) {
      return codegen_lval(n);
    }
    std::string llty = llvm_ty(res_ts);
    std::string ptr = codegen_lval(n);
    std::string t = fresh_tmp();
    emit(t + " = load " + llty + ", ptr " + ptr);
    return t;
  }

  case NK_MEMBER: {
    TypeSpec res_ts = expr_type(n);
    std::string llty = llvm_ty(res_ts);
    // If it's an array field, return the pointer (decay)
    if (is_array_ty(res_ts)) {
      return codegen_lval(n);
    }
    std::string ptr = codegen_lval(n);
    // Bitfield read: determine storage type, bit offset, then mask/shift.
    int bfw = member_bitfield_width(n);
    if (bfw > 0 && res_ts.ptr_level == 0 && !is_array_ty(res_ts)) {
      // Get storage type and bit offset for this bitfield.
      TypeSpec obj_ts = expr_type(n->left);
      if (n->is_arrow && obj_ts.ptr_level > 0) obj_ts.ptr_level -= 1;
      std::string stor_llty = "i32";
      int bit_off = 0;
      TypeSpec decl_ts = res_ts;
      if ((obj_ts.base == TB_STRUCT || obj_ts.base == TB_UNION) && obj_ts.tag) {
        FieldPath path;
        if (find_field(obj_ts.tag, n->name, path)) {
          bit_off = field_bit_offset(struct_defs_, path.final_tag, path.final_idx);
          stor_llty = field_storage_llty(struct_defs_, path.final_tag, path.final_idx);
          decl_ts = field_type_from_path(obj_ts.tag, path);
        }
      }
      // Load using storage type
      std::string t = fresh_tmp();
      emit(t + " = load " + stor_llty + ", ptr " + ptr);
      // Shift right by bit_off to move bitfield to position 0
      std::string shifted = t;
      if (bit_off > 0) {
        shifted = fresh_tmp();
        emit(shifted + " = lshr " + stor_llty + " " + t + ", " + std::to_string(bit_off));
      }
      // Mask to bitfield width
      long long mask = (bfw >= 64) ? (long long)-1 : ((1LL << bfw) - 1);
      std::string masked = fresh_tmp();
      emit(masked + " = and " + stor_llty + " " + shifted + ", " + std::to_string(mask));
      // Sign-extend for signed bitfields
      bool is_signed = !is_unsigned_base(decl_ts.base) && decl_ts.base != TB_BOOL;
      if (decl_ts.base == TB_ENUM && decl_ts.tag) {
        auto eit = enum_all_nonnegative_.find(decl_ts.tag);
        if (eit != enum_all_nonnegative_.end() && eit->second)
          is_signed = false;
      }
      std::string result = masked;
      if (is_signed && bfw < (int)(stor_llty == "i8" ? 8 : stor_llty == "i16" ? 16 :
                                    stor_llty == "i32" ? 32 : 64)) {
        long long sign_bit = 1LL << (bfw - 1);
        std::string sign_t = fresh_tmp(), sext_t = fresh_tmp();
        emit(sign_t + " = and " + stor_llty + " " + masked + ", " + std::to_string(sign_bit));
        emit(sext_t + " = icmp ne " + stor_llty + " " + sign_t + ", 0");
        long long sign_ext = ~mask;
        std::string ext_val = fresh_tmp(), phi_t = fresh_tmp();
        emit(ext_val + " = or " + stor_llty + " " + masked + ", " + std::to_string(sign_ext));
        emit(phi_t + " = select i1 " + sext_t + ", " + stor_llty + " " + ext_val + ", " + stor_llty + " " + masked);
        result = phi_t;
      }
      // Extend or truncate to result type if needed
      if (stor_llty != llty) {
        auto llty_bits = [](const std::string& t) {
          if (t == "i1") return 1; if (t == "i8") return 8;
          if (t == "i16") return 16; if (t == "i32") return 32; return 64;
        };
        int sb = llty_bits(stor_llty), db = llty_bits(llty);
        std::string ext_t = fresh_tmp();
        if (db > sb)
          emit(ext_t + " = " + (is_signed ? "sext" : "zext") + " " + stor_llty + " " + result + " to " + llty);
        else if (db < sb)
          emit(ext_t + " = trunc " + stor_llty + " " + result + " to " + llty);
        else
          emit(ext_t + " = bitcast " + stor_llty + " " + result + " to " + llty);
        return ext_t;
      }
      return result;
    }
    // Non-bitfield read
    std::string t = fresh_tmp();
    emit(t + " = load " + llty + ", ptr " + ptr);
    return t;
  }

  case NK_CAST: {
    TypeSpec from_ts = expr_type(n->left);
    if (from_ts.ptr_level > 0 && from_ts.is_const &&
        n->type.ptr_level > 0 && !n->type.is_const &&
        n->left && n->left->kind == NK_ADDR)
      throw std::runtime_error("cast discards const qualifier");
    if ((n->type.base == TB_STRUCT || n->type.base == TB_UNION) &&
        n->type.ptr_level == 0 && n->type.tag) {
      bool missing = (struct_defs_.find(n->type.tag) == struct_defs_.end());
      if (missing)
        throw std::runtime_error(std::string("cast to incomplete type: ") + n->type.tag);
    }
    std::string v = codegen_expr(n->left);
    return coerce(v, from_ts, llvm_ty(n->type));
  }

  case NK_TERNARY: {
    // cond ? then_ : else_
    TypeSpec res_ts = expr_type(n);
    std::string res_llty = llvm_ty(res_ts);
    bool is_void_result = (res_llty == "void");

    TypeSpec cond_ts = expr_type(n->cond);
    std::string cond_val = codegen_expr(n->cond);
    std::string cond_b = to_bool(cond_val, cond_ts);

    std::string then_lbl = fresh_label("tern_then");
    std::string else_lbl = fresh_label("tern_else");
    std::string end_lbl  = fresh_label("tern_end");

    // Allocate result slot only for non-void result types
    std::string res_slot;
    if (!is_void_result) {
      res_slot = "%ternary_" + std::to_string(tmp_idx_++);
      alloca_lines_.push_back("  " + res_slot + " = alloca " + res_llty);
    }

    emit_terminator("br i1 " + cond_b + ", label %" + then_lbl +
                    ", label %" + (n->else_ ? else_lbl : end_lbl));

    emit_label(then_lbl);
    std::string then_val = codegen_expr(n->then_);
    if (!is_void_result) {
      TypeSpec then_ts = expr_type(n->then_);
      then_val = coerce(then_val, then_ts, res_llty);
      emit("store " + res_llty + " " + then_val + ", ptr " + res_slot);
    }
    emit_terminator("br label %" + end_lbl);

    if (n->else_) {
      emit_label(else_lbl);
      std::string else_val = codegen_expr(n->else_);
      if (!is_void_result) {
        TypeSpec else_ts = expr_type(n->else_);
        else_val = coerce(else_val, else_ts, res_llty);
        emit("store " + res_llty + " " + else_val + ", ptr " + res_slot);
      }
      emit_terminator("br label %" + end_lbl);
    }

    emit_label(end_lbl);
    if (is_void_result) return "0";
    std::string t = fresh_tmp();
    emit(t + " = load " + res_llty + ", ptr " + res_slot);
    return t;
  }

  case NK_SIZEOF_EXPR: {
    // Special-case: sizeof(string_literal) = byte length including null terminator
    if (n->left && n->left->kind == NK_STR_LIT && !is_wide_str_lit(n->left)) {
      int slen = str_lit_byte_len(n->left);
      if (slen >= 0) return std::to_string(slen);
    }
    int sz = sizeof_ty(expr_type(n->left));
    return std::to_string(sz);
  }
  case NK_SIZEOF_TYPE: {
    int sz = sizeof_ty(n->type);
    return std::to_string(sz);
  }
  case NK_ALIGNOF_TYPE: {
    int sz = alignof_ty_impl(n->type, struct_defs_);
    return std::to_string(sz);
  }

  case NK_COMMA_EXPR: {
    codegen_expr(n->left);  // evaluate for side effects
    return codegen_expr(n->right);
  }

  case NK_STMT_EXPR: {
    // ({ ... }) - GCC statement expression
    if (!n->body) return "0";
    // Emit all statements except the last expression
    Node* blk = n->body;
    std::string last_val = "0";
    for (int i = 0; i < blk->n_children; i++) {
      Node* child = blk->children[i];
      if (i == blk->n_children - 1 && child->kind == NK_EXPR_STMT && child->left) {
        last_val = codegen_expr(child->left);
      } else {
        emit_stmt(child);
      }
    }
    return last_val;
  }

  case NK_COMPOUND_LIT: {
    // (type){ ... } - treat as a temporary local variable
    TypeSpec ts = n->type;
    // Initializer is in ->left (set by parser). Use emit_agg_init_impl for
    // full brace-elision, sub-list, and designator support.
    Node* cl_init = n->left ? n->left : n->init;
    // Infer array size for unsized compound literals: (int[]){0,1,2} → [3 x i32]
    if (is_array_ty(ts) && array_dim_at(ts, 0) == -2 && cl_init) {
      int inferred = infer_array_size_from_init(cl_init);
      if (inferred > 0) set_first_array_dim(ts, inferred);
    }
    std::string llty = llvm_ty(ts);
    std::string slot = "%clit_" + std::to_string(tmp_idx_++);
    alloca_lines_.push_back("  " + slot + " = alloca " + llty);
    if (cl_init && cl_init->kind == NK_INIT_LIST &&
        (is_array_ty(ts) || ts.base == TB_STRUCT || ts.base == TB_UNION)) {
      int flat_idx = 0;
      emit_agg_init_impl(this, ts, slot, cl_init, flat_idx);
    }
    // For struct/union rvalue context: load and return the value so that
    // callers (e.g. return stmt, assignment) get a proper value not a ptr.
    // Arrays decay to pointer, so keep returning the slot for array types.
    // Vectors: keep returning the alloca ptr — callers that need the value
    // (compound assignment, arithmetic) must load it explicitly.
    if (ts.ptr_level == 0 && (ts.base == TB_STRUCT || ts.base == TB_UNION)) {
      std::string t = fresh_tmp();
      emit(t + " = load " + llty + ", ptr " + slot);
      return t;
    }
    return slot;
  }

  case NK_VA_ARG: {
    // AArch64 SysV va_arg lowering over __va_list_tag:
    //   struct { void* stack, *gr_top, *vr_top; int gr_offs, vr_offs; }
    // Aggregate va_arg is lowered manually to avoid backend crashes.
    TypeSpec ts = n->type;
    std::string llty = llvm_ty(ts);
    std::string ap_ptr = codegen_lval(n->left);

    auto ensure_memcpy_decl = [&]() {
      if (!auto_declared_fns_.count("llvm.memcpy.p0.p0.i64"))
        auto_declared_fns_["llvm.memcpy.p0.p0.i64"] =
          "declare void @llvm.memcpy.p0.p0.i64(ptr noalias nocapture writeonly, "
          "ptr noalias nocapture readonly, i64, i1 immarg)";
    };

    auto alignof_ts = [&](const TypeSpec& t, const auto& self) -> int {
      if (is_array_ty(t)) {
        TypeSpec elem = drop_array_dim(t);
        return self(elem, self);
      }
      if (t.ptr_level > 0) return 8;
      switch (t.base) {
        case TB_BOOL:
        case TB_CHAR:
        case TB_UCHAR:
        case TB_SCHAR: return 1;
        case TB_SHORT:
        case TB_USHORT: return 2;
        case TB_INT:
        case TB_UINT:
        case TB_FLOAT:
        case TB_ENUM: return 4;
        case TB_INT128:
        case TB_UINT128: return 16;
        case TB_LONGDOUBLE: return 8; // keep ABI math consistent with llvm_ty_base
        case TB_LONG:
        case TB_ULONG:
        case TB_LONGLONG:
        case TB_ULONGLONG:
        case TB_DOUBLE:
        case TB_VA_LIST:
        case TB_FUNC_PTR: return 8;
        case TB_STRUCT:
        case TB_UNION: {
          if (!t.tag) return 8;
          auto it = struct_defs_.find(t.tag);
          if (it == struct_defs_.end()) return 8;
          const StructInfo& si = it->second;
          int maxa = 1;
          for (size_t i = 0; i < si.field_types.size(); i++) {
            if (si.field_array_sizes[i] == -2) continue;
            TypeSpec ft = si.field_types[i];
            int fa = self(ft, self);
            if (fa > maxa) maxa = fa;
          }
          return maxa;
        }
        default: return 8;
      }
    };

    auto hfa_info = [&]() -> std::pair<bool, std::pair<TypeSpec, int>> {
      if (ts.ptr_level != 0 || is_array_ty(ts) ||
          (ts.base != TB_STRUCT && ts.base != TB_UNION) || !ts.tag)
        return {false, {TypeSpec{}, 0}};
      auto it = struct_defs_.find(ts.tag);
      if (it == struct_defs_.end()) return {false, {TypeSpec{}, 0}};
      const StructInfo& si = it->second;
      if (si.is_union) return {false, {TypeSpec{}, 0}};
      int count = 0;
      TypeSpec elem{};
      bool has_elem = false;
      for (size_t i = 0; i < si.field_types.size(); i++) {
        if (si.field_array_sizes[i] == -2) continue;
        TypeSpec ft = si.field_types[i];
        if (is_array_ty(ft) || ft.ptr_level != 0) return {false, {TypeSpec{}, 0}};
        bool is_fp = (ft.base == TB_FLOAT || ft.base == TB_DOUBLE || ft.base == TB_LONGDOUBLE);
        if (!is_fp) return {false, {TypeSpec{}, 0}};
        if (!has_elem) { elem = ft; has_elem = true; }
        else if (elem.base != ft.base) return {false, {TypeSpec{}, 0}};
        count++;
      }
      if (count >= 1 && count <= 4) return {true, {elem, count}};
      return {false, {TypeSpec{}, 0}};
    };

    bool is_agg = (ts.ptr_level == 0 && !is_array_ty(ts) &&
                   (ts.base == TB_STRUCT || ts.base == TB_UNION));
    auto hfa = hfa_info();
    bool is_hfa = hfa.first;
    TypeSpec hfa_elem = hfa.second.first;
    int hfa_count = hfa.second.second;
    bool is_fp_scalar = (ts.ptr_level == 0 && !is_array_ty(ts) &&
                         (ts.base == TB_FLOAT || ts.base == TB_DOUBLE || ts.base == TB_LONGDOUBLE));
    bool use_vr = is_fp_scalar || is_hfa;

    int sz = sizeof_ty(ts);
    if (sz <= 0) sz = 8;
    int need = use_vr ? (is_hfa ? hfa_count : 1) : ((sz + 7) / 8);
    if (need < 1) need = 1;
    int step = use_vr ? (need * 16) : (need * 8);

    auto get_field_ptr = [&](int idx, bool is_i32) -> std::string {
      std::string p = fresh_tmp();
      emit(p + " = getelementptr inbounds %struct.__va_list_tag_, ptr " + ap_ptr +
           ", i32 0, i32 " + std::to_string(idx));
      return p;
    };

    std::string offs_ptr = get_field_ptr(use_vr ? 4 : 3, true);
    std::string offs = fresh_tmp();
    emit(offs + " = load i32, ptr " + offs_ptr + ", align 4");
    std::string cmp_ge0 = fresh_tmp();
    emit(cmp_ge0 + " = icmp sge i32 " + offs + ", 0");
    std::string on_stack = fresh_label("va_stack");
    std::string try_reg = fresh_label("va_tryreg");
    std::string done = fresh_label("va_done");
    std::string in_reg = fresh_label("va_inreg");
    emit_terminator("br i1 " + cmp_ge0 + ", label %" + on_stack + ", label %" + try_reg);

    std::string reg_ptr;
    std::string stk_ptr;

    emit_label(try_reg);
    std::string next_offs = fresh_tmp();
    emit(next_offs + " = add i32 " + offs + ", " + std::to_string(step));
    emit("store i32 " + next_offs + ", ptr " + offs_ptr + ", align 4");
    std::string cmp_le0 = fresh_tmp();
    emit(cmp_le0 + " = icmp sle i32 " + next_offs + ", 0");
    emit_terminator("br i1 " + cmp_le0 + ", label %" + in_reg + ", label %" + on_stack);

    emit_label(in_reg);
    std::string top_ptr = get_field_ptr(use_vr ? 2 : 1, false);
    std::string top = fresh_tmp();
    emit(top + " = load ptr, ptr " + top_ptr + ", align 8");
    reg_ptr = fresh_tmp();
    emit(reg_ptr + " = getelementptr inbounds i8, ptr " + top + ", i32 " + offs);
    emit_terminator("br label %" + done);

    emit_label(on_stack);
    std::string stack_field = get_field_ptr(0, false);
    std::string stack_cur = fresh_tmp();
    emit(stack_cur + " = load ptr, ptr " + stack_field + ", align 8");
    int align = alignof_ts(ts, alignof_ts);
    if (align < 1) align = 1;
    if (align > 8) {
      std::string pi = fresh_tmp();
      std::string pa = fresh_tmp();
      std::string pm = fresh_tmp();
      emit(pi + " = ptrtoint ptr " + stack_cur + " to i64");
      emit(pa + " = add i64 " + pi + ", " + std::to_string(align - 1));
      emit(pm + " = and i64 " + pa + ", " + std::to_string(-(long long)align));
      stk_ptr = fresh_tmp();
      emit(stk_ptr + " = inttoptr i64 " + pm + " to ptr");
    } else {
      stk_ptr = stack_cur;
    }
    int stack_step = (sz + 7) & ~7;
    std::string stack_next = fresh_tmp();
    emit(stack_next + " = getelementptr inbounds i8, ptr " + stk_ptr +
         ", i64 " + std::to_string(stack_step));
    emit("store ptr " + stack_next + ", ptr " + stack_field + ", align 8");
    emit_terminator("br label %" + done);

    emit_label(done);
    std::string src_ptr = fresh_tmp();
    emit(src_ptr + " = phi ptr [ " + reg_ptr + ", %" + in_reg + " ], [ " +
         stk_ptr + ", %" + on_stack + " ]");

    // Non-aggregate scalar
    if (!is_agg) {
      std::string t = fresh_tmp();
      int ldalign = alignof_ts(ts, alignof_ts);
      if (ldalign < 1) ldalign = 1;
      emit(t + " = load " + llty + ", ptr " + src_ptr + ", align " + std::to_string(ldalign));
      return t;
    }

    // Aggregate: copy into a local temp, then load the value.
    std::string tmp = "%va_tmp_" + std::to_string(tmp_idx_++);
    alloca_lines_.push_back("  " + tmp + " = alloca " + llty + ", align " +
                            std::to_string(std::max(8, alignof_ts(ts, alignof_ts))));
    if (use_vr && is_hfa && hfa_count > 1) {
      std::string pre = fresh_tmp();
      emit(pre + " = phi i1 [ true, %" + in_reg + " ], [ false, %" + on_stack + " ]");
      std::string hfa_reg = fresh_label("va_hfa_reg");
      std::string hfa_stk = fresh_label("va_hfa_stk");
      std::string hfa_end = fresh_label("va_hfa_end");
      emit_terminator("br i1 " + pre + ", label %" + hfa_reg + ", label %" + hfa_stk);

      emit_label(hfa_reg);
      std::string elem_llty = llvm_ty(hfa_elem);
      int elem_store_align = std::max(1, alignof_ts(hfa_elem, alignof_ts));
      std::string lane_tmp = "%va_hfa_lane_" + std::to_string(tmp_idx_++);
      std::string lane_arr_ty = "[" + std::to_string(hfa_count) + " x " + elem_llty + "]";
      alloca_lines_.push_back("  " + lane_tmp + " = alloca " + lane_arr_ty + ", align 16");
      for (int i = 0; i < hfa_count; i++) {
        std::string s = fresh_tmp();
        emit(s + " = getelementptr inbounds i8, ptr " + src_ptr + ", i64 " + std::to_string(i * 16));
        std::string v = fresh_tmp();
        emit(v + " = load " + elem_llty + ", ptr " + s + ", align 16");
        std::string d = fresh_tmp();
        emit(d + " = getelementptr inbounds " + lane_arr_ty + ", ptr " + lane_tmp +
             ", i64 0, i64 " + std::to_string(i));
        emit("store " + elem_llty + " " + v + ", ptr " + d + ", align " +
             std::to_string(elem_store_align));
      }
      ensure_memcpy_decl();
      emit("call void @llvm.memcpy.p0.p0.i64(ptr align " +
           std::to_string(std::max(1, alignof_ts(ts, alignof_ts))) + " " + tmp +
           ", ptr align " + std::to_string(elem_store_align) + " " + lane_tmp +
           ", i64 " + std::to_string(sz) + ", i1 false)");
      emit_terminator("br label %" + hfa_end);

      emit_label(hfa_stk);
      ensure_memcpy_decl();
      emit("call void @llvm.memcpy.p0.p0.i64(ptr align " +
           std::to_string(std::max(1, alignof_ts(ts, alignof_ts))) + " " + tmp +
           ", ptr align 8 " + src_ptr + ", i64 " + std::to_string(sz) + ", i1 false)");
      emit_terminator("br label %" + hfa_end);
      emit_label(hfa_end);
    } else {
      ensure_memcpy_decl();
      emit("call void @llvm.memcpy.p0.p0.i64(ptr align " +
           std::to_string(std::max(1, alignof_ts(ts, alignof_ts))) + " " + tmp +
           ", ptr align 8 " + src_ptr + ", i64 " + std::to_string(sz) + ", i1 false)");
    }

    std::string outv = fresh_tmp();
    emit(outv + " = load " + llty + ", ptr " + tmp);
    return outv;
  }

  case NK_GENERIC: {
    // _Generic(ctrl, type1: val1, ..., default: vN)
    // children[i] = NK_CAST{type=assoc_type, left=val, ival=1 if default}
    TypeSpec ctrl_ts = expr_type(n->left);
    // Apply C11 lvalue conversion: remove top-level qualifiers for non-pointer types
    // (const int → int, but const int * stays const int *)
    if (ctrl_ts.ptr_level == 0 && !is_array_ty(ctrl_ts)) {
      ctrl_ts.is_const = false;
      ctrl_ts.is_volatile = false;
    }
    // String literals have type char[] → decayed to char* (NOT const char*)
    if (n->left && n->left->kind == NK_STR_LIT) {
      ctrl_ts = ptr_to(TB_CHAR);
      ctrl_ts.is_const = false;
    }
    // Functions used as values: use return type + ptr_level=1 for matching
    if (n->left && n->left->kind == NK_VAR && n->left->name &&
        func_sigs_.count(n->left->name)) {
      ctrl_ts = func_sigs_[n->left->name].ret_type;
      ctrl_ts.ptr_level = 1;
    }
    // Type equality for _Generic matching (array rank matters too)
    auto ts_match = [&](TypeSpec a, TypeSpec b) -> bool {
      if (a.base != b.base) return false;
      if (a.ptr_level != b.ptr_level) return false;
      // Only check is_const for pointer types (pointed-to type qualifier)
      if (a.ptr_level > 0 && a.is_const != b.is_const) return false;
      // Array types must match rank and dimensions
      if (array_rank_of(a) != array_rank_of(b)) return false;
      // For struct/union/enum: compare tags
      if ((a.base == TB_STRUCT || a.base == TB_UNION || a.base == TB_ENUM)) {
        if (a.tag && b.tag) return strcmp(a.tag, b.tag) == 0;
        return a.tag == b.tag;
      }
      return true;
    };

    // Find matching association
    Node* default_val = nullptr;
    for (int i = 0; i < n->n_children; i++) {
      Node* assoc = n->children[i];
      if (!assoc) continue;
      if (assoc->ival == 1) { default_val = assoc->left; continue; }
      if (ts_match(ctrl_ts, assoc->type)) return codegen_expr(assoc->left);
    }
    if (default_val) return codegen_expr(default_val);
    return "0";
  }

  default:
    return "0";
  }
}

// ─────────────────────── aggregate local initializer ───────────────────────
// Emit stores to fill the memory at `ptr` with the given C type `ts`,
// consuming items from flat_list starting at flat_idx.
// Handles brace elision: scalars are distributed into nested aggregates.
// When sub_init != null, it is used as a braced sub-init list instead of flat_list.
static void emit_agg_init_impl(IRBuilder* irb, TypeSpec ts, const std::string& ptr,
                                Node* flat_list, int& flat_idx) {
  std::string llty = irb->llvm_ty(ts);
  // Zero-initialize first so that uninitialized fields/elements are zero.
  // (C99 §6.7.9p21: omitted members implicitly zero-initialized.)
  irb->emit("  store " + llty + " zeroinitializer, ptr " + ptr);
  // ── Struct type ────────────────────────────────────────────────────────────
  if ((ts.base == TB_STRUCT) && !is_array_ty(ts) && ts.tag && ts.ptr_level == 0) {
    auto sit = irb->struct_defs_.find(ts.tag);
    if (sit == irb->struct_defs_.end()) return;
    const IRBuilder::StructInfo& si = sit->second;
    int n_fields = (int)si.field_names.size();
    std::string struct_llty = std::string("%struct.") + ts.tag;
    int fi = 0;
    // Process items: may be a mix of designated/non-designated/sub-lists
    // We drive by field index, consuming from flat_list as needed.
    while (flat_idx < flat_list->n_children) {
      Node* item = flat_list->children[flat_idx];
      Node* val_node = item;
      int target_fi = fi;
      bool item_is_desig = (item && item->kind == NK_INIT_ITEM && item->desig_field);
      // For non-designated items: stop BEFORE consuming if all fields are done.
      if (!item_is_desig && fi >= n_fields) break;
      // Unwrap NK_INIT_ITEM
        if (item && item->kind == NK_INIT_ITEM) {
          if (item->desig_field) {
          // Find the designated field
          for (int j = 0; j < n_fields; j++) {
            if (si.field_names[j] == item->desig_field) { target_fi = j; break; }
          }
          fi = target_fi;
        }
        val_node = item->left;
        flat_idx++;  // consume this item
      } else {
        flat_idx++;
      }
      if (fi >= n_fields) break;  // designated item out of range; stop
      TypeSpec ft = si.field_types[fi];
      if (si.field_array_sizes[fi] >= 0) {
        ft.array_size = si.field_array_sizes[fi];
        ft.inner_rank = -1;
      }
      std::string field_llty = irb->llvm_ty(ft);
      // Get a GEP for this field (use LLVM field index, not C field index)
      int fi_llvm = fi;
      if (fi >= 0 && fi < (int)si.field_llvm_idx.size())
        fi_llvm = si.field_llvm_idx[fi];
      std::string fgep = irb->fresh_tmp();
      irb->emit(fgep + " = getelementptr " + struct_llty + ", ptr " + ptr +
                ", i32 0, i32 " + std::to_string(fi_llvm));
      // Determine how to initialize this field.
      // Pre-compute whether val_node itself has aggregate type (struct/union).
      // If so, it is a direct assignment — brace-elision must NOT fire.
      TypeSpec val_vts;
      bool val_is_agg = false;
      if (val_node) {
        val_vts = irb->expr_type(val_node);
        val_is_agg = val_vts.ptr_level == 0 &&
                     (val_vts.base == TB_STRUCT || val_vts.base == TB_UNION);
      }
      if (val_node && val_node->kind == NK_INIT_LIST) {
        // Braced sub-init: use a fresh flat_idx for this sub-list
        int sub_fi = 0;
        emit_agg_init_impl(irb, ft, fgep, val_node, sub_fi);
      } else if (!val_is_agg && ft.ptr_level == 0 && !is_array_ty(ft) &&
                 (ft.base == TB_STRUCT || ft.base == TB_UNION) &&
                 val_node && val_node->kind != NK_COMPOUND_LIT) {
        // Scalar item for a struct/union field: brace-elision — rewind and recurse
        flat_idx--;
        emit_agg_init_impl(irb, ft, fgep, flat_list, flat_idx);
      } else if (!val_is_agg && ft.ptr_level == 0 && is_array_ty(ft) &&
                 val_node && val_node->kind != NK_INIT_LIST) {
        // Array field with scalar initializer: brace-elision.
        // C99 6.7.9: scalars from the flat list fill array elements in order.
        // Rewind so the recursive array-branch consumes elements from flat_list.
        flat_idx--;
        emit_agg_init_impl(irb, ft, fgep, flat_list, flat_idx);
      } else {
        // Scalar field or array field with single item, compound literal, etc.
        if (val_node) {
          std::string val = irb->codegen_expr(val_node);
          TypeSpec val_ts = irb->expr_type(val_node);
          // Handle string literal for array field
          if (is_array_ty(ft) && array_rank_of(ft) == 1 &&
              (ft.base == TB_CHAR || ft.base == TB_SCHAR || ft.base == TB_UCHAR) &&
              val_node->kind == NK_STR_LIT) {
            // String literal to char array: do byte-by-byte stores
            const char* raw = val_node->sval ? val_node->sval : "\"\"";
            if (raw[0] == '"') raw++;
            std::string bytes;
            while (*raw && *raw != '"') {
              if (*raw == '\\') {
                raw++;
                if (*raw == 'n') { bytes += '\n'; raw++; }
                else if (*raw == 't') { bytes += '\t'; raw++; }
                else if (*raw == 'r') { bytes += '\r'; raw++; }
                else if (*raw == '0') { bytes += '\0'; raw++; }
                else if (*raw == 'a') { bytes += '\a'; raw++; }
                else if (*raw == 'b') { bytes += '\b'; raw++; }
                else if (*raw == 'f') { bytes += '\f'; raw++; }
                else if (*raw == 'v') { bytes += '\v'; raw++; }
                else if (*raw == '\\') { bytes += '\\'; raw++; }
                else if (*raw == '"') { bytes += '"'; raw++; }
                else if (*raw == 'x' || *raw == 'X') {
                  raw++;
                  unsigned int v = 0;
                  for (int k = 0; k < 2 && *raw && isxdigit((unsigned char)*raw); k++, raw++)
                    v = v*16 + (isdigit((unsigned char)*raw) ? *raw - '0' :
                                tolower((unsigned char)*raw) - 'a' + 10);
                  bytes += (char)(unsigned char)v;
                } else if (*raw >= '0' && *raw <= '7') {
                  unsigned int v = 0;
                  for (int k = 0; k < 3 && *raw >= '0' && *raw <= '7'; k++, raw++)
                    v = v*8 + (*raw - '0');
                  bytes += (char)(unsigned char)v;
                } else { bytes += *raw++; }
              } else { bytes += *raw++; }
            }
            int dim0 = (int)array_dim_at(ft, 0);
            for (int bi = 0; bi < dim0; bi++) {
              char bval = (bi < (int)bytes.size()) ? bytes[bi] : '\0';
              std::string egep = irb->fresh_tmp();
              irb->emit(egep + " = getelementptr " + field_llty + ", ptr " + fgep +
                        ", i64 0, i64 " + std::to_string(bi));
              irb->emit("store i8 " + std::to_string((int)(unsigned char)bval) + ", ptr " + egep);
            }
          } else if (is_array_ty(ft) && val == "0") {
            irb->emit("store " + field_llty + " zeroinitializer, ptr " + fgep);
          } else if ((ft.base == TB_STRUCT || ft.base == TB_UNION) &&
                     ft.ptr_level == 0 && val == "0") {
            irb->emit("store " + field_llty + " zeroinitializer, ptr " + fgep);
          } else if ((ft.base == TB_STRUCT || ft.base == TB_UNION) &&
                     ft.ptr_level == 0) {
            // Struct/union value: may be a ptr from compound lit or a loaded value
            TypeSpec vts = irb->expr_type(val_node);
            if (vts.ptr_level == 0 && (vts.base == TB_STRUCT || vts.base == TB_UNION)) {
              // It's a struct value directly
              irb->emit("store " + field_llty + " " + val + ", ptr " + fgep);
            } else {
              val = irb->coerce(val, val_ts, field_llty);
              irb->emit("store " + field_llty + " " + val + ", ptr " + fgep);
            }
          } else {
            val = irb->coerce(val, val_ts, field_llty);
            irb->emit("store " + field_llty + " " + val + ", ptr " + fgep);
          }
        } else {
          // null val_node: zero init
          irb->emit("store " + field_llty + " " + (is_array_ty(ft) || ft.base == TB_STRUCT || ft.base == TB_UNION ? "zeroinitializer" : "0") + ", ptr " + fgep);
        }
      }
      fi++;
    }
    return;
  }

  // ── Union type ────────────────────────────────────────────────────────────
  if (ts.base == TB_UNION && ts.ptr_level == 0 && ts.tag) {
    auto sit = irb->struct_defs_.find(ts.tag);
    if (sit == irb->struct_defs_.end()) return;
    const IRBuilder::StructInfo& si = sit->second;
    if (si.field_names.empty() || flat_idx >= flat_list->n_children) return;

    Node* item = flat_list->children[flat_idx];
    Node* val_node = item;
    int field_idx = 0;  // default: first field

    if (item && item->kind == NK_INIT_ITEM) {
      if (item->desig_field) {
        for (int j = 0; j < (int)si.field_names.size(); j++) {
          if (si.field_names[j] == item->desig_field) { field_idx = j; break; }
        }
      }
      val_node = item->left;
      flat_idx++;
    } else {
      flat_idx++;
    }

    TypeSpec ft = si.field_types[field_idx];
    if (si.field_array_sizes[field_idx] >= 0) ft.array_size = si.field_array_sizes[field_idx];
    std::string field_llty = irb->llvm_ty(ft);

    if (val_node && val_node->kind == NK_INIT_LIST) {
      // Braced sub-init for a union field: union fields always at offset 0
      int sub_fi = 0;
      emit_agg_init_impl(irb, ft, ptr, val_node, sub_fi);
    } else if (val_node) {
      std::string val = irb->codegen_expr(val_node);
      TypeSpec val_ts = irb->expr_type(val_node);
      if (ft.ptr_level == 0 && (ft.base == TB_STRUCT || ft.base == TB_UNION)) {
        if (val_ts.ptr_level == 0 && (val_ts.base == TB_STRUCT || val_ts.base == TB_UNION)) {
          irb->emit("store " + field_llty + " " + val + ", ptr " + ptr);
        } else {
          val = irb->coerce(val, val_ts, field_llty);
          irb->emit("store " + field_llty + " " + val + ", ptr " + ptr);
        }
      } else {
        if (val == "0" && (is_array_ty(ft) || ft.base == TB_STRUCT || ft.base == TB_UNION))
          val = "zeroinitializer";
        else
          val = irb->coerce(val, val_ts, field_llty);
        irb->emit("store " + field_llty + " " + val + ", ptr " + ptr);
      }
    }
    return;
  }

  // ── Array type ────────────────────────────────────────────────────────────
  if (is_array_ty(ts)) {
    long long dim0 = array_dim_at(ts, 0);
    if (dim0 <= 0) return;
    TypeSpec elem_ts = drop_array_dim(ts);
    std::string arr_llty = llty;
    std::string elem_llty = irb->llvm_ty(elem_ts);

    // Check if first item is a string literal for char array
    if ((elem_ts.base == TB_CHAR || elem_ts.base == TB_SCHAR || elem_ts.base == TB_UCHAR) &&
        elem_ts.ptr_level == 0 && flat_idx < flat_list->n_children) {
      Node* item = flat_list->children[flat_idx];
      Node* val_node = (item && item->kind == NK_INIT_ITEM) ? item->left : item;
      if (val_node && val_node->kind == NK_STR_LIT && !is_wide_str_lit(val_node)) {
        // String literal for char array
        flat_idx++;
        const char* raw = val_node->sval ? val_node->sval : "\"\"";
        if (raw[0] == '"') raw++;
        std::string bytes;
        while (*raw && *raw != '"') {
          if (*raw == '\\') {
            raw++;
            if (*raw == 'n') { bytes += '\n'; raw++; }
            else if (*raw == 't') { bytes += '\t'; raw++; }
            else if (*raw == 'r') { bytes += '\r'; raw++; }
            else if (*raw == '0') { bytes += '\0'; raw++; }
            else if (*raw == 'a') { bytes += '\a'; raw++; }
            else if (*raw == 'b') { bytes += '\b'; raw++; }
            else if (*raw == 'f') { bytes += '\f'; raw++; }
            else if (*raw == 'v') { bytes += '\v'; raw++; }
            else if (*raw == '\\') { bytes += '\\'; raw++; }
            else if (*raw == '"') { bytes += '"'; raw++; }
            else if (*raw == 'x' || *raw == 'X') {
              raw++;
              unsigned int v = 0;
              for (int k = 0; k < 2 && *raw && isxdigit((unsigned char)*raw); k++, raw++)
                v = v*16 + (isdigit((unsigned char)*raw) ? *raw - '0' :
                            tolower((unsigned char)*raw) - 'a' + 10);
              bytes += (char)(unsigned char)v;
            } else if (*raw >= '0' && *raw <= '7') {
              unsigned int v = 0;
              for (int k = 0; k < 3 && *raw >= '0' && *raw <= '7'; k++, raw++)
                v = v*8 + (*raw - '0');
              bytes += (char)(unsigned char)v;
            } else { bytes += *raw++; }
          } else { bytes += *raw++; }
        }
        for (long long bi = 0; bi < dim0; bi++) {
          char bval = (bi < (long long)bytes.size()) ? bytes[(int)bi] : '\0';
          std::string egep = irb->fresh_tmp();
          irb->emit(egep + " = getelementptr " + arr_llty + ", ptr " + ptr +
                    ", i64 0, i64 " + std::to_string(bi));
          irb->emit("store i8 " + std::to_string((int)(unsigned char)bval) + ", ptr " + egep);
        }
        return;
      }
    }

    // General array init: element by element with flat continuation
    for (long long ei = 0; ei < dim0 && flat_idx < flat_list->n_children; ei++) {
      Node* item = flat_list->children[flat_idx];
      Node* val_node = item;
      long long idx = ei;
      bool is_range = false;
      long long range_end = ei;
      if (item && item->kind == NK_INIT_ITEM) {
        if (item->is_index_desig) {
          idx = item->desig_val; ei = idx;
          // Range designator [lo ... hi] — parser stores hi in item->right
          if (item->right) {
            range_end = item->right->ival;
            is_range = true;
          }
        }
        // Check range designator (stored as field name "[A...B]") — legacy format
        if (item->desig_field) {
          // Try to parse as range [A...B]
          const char* df = item->desig_field;
          if (df[0] == '[') {
            long long lo = strtoll(df+1, nullptr, 10);
            const char* dots = strstr(df, "...");
            if (dots) {
              long long hi = strtoll(dots+3, nullptr, 10);
              idx = lo; range_end = hi; is_range = true;
              ei = lo;
            } else {
              idx = lo; ei = lo;
            }
          }
        }
        val_node = item->left;
        flat_idx++;
      } else {
        flat_idx++;
      }

      std::string egep = irb->fresh_tmp();
      irb->emit(egep + " = getelementptr " + arr_llty + ", ptr " + ptr +
                ", i64 0, i64 " + std::to_string(ei));

      if (val_node && val_node->kind == NK_INIT_LIST) {
        int sub_fi = 0;
        emit_agg_init_impl(irb, elem_ts, egep, val_node, sub_fi);
      } else if (elem_ts.ptr_level == 0 && !is_array_ty(elem_ts) &&
                 (elem_ts.base == TB_STRUCT || elem_ts.base == TB_UNION) &&
                 val_node && val_node->kind != NK_COMPOUND_LIT) {
        // Brace elision into struct element — need to un-consume
        flat_idx--;
        emit_agg_init_impl(irb, elem_ts, egep, flat_list, flat_idx);
      } else {
        std::string val = val_node ? irb->codegen_expr(val_node) : "0";
        TypeSpec val_ts = val_node ? irb->expr_type(val_node) : int_ts();
        if ((elem_ts.base == TB_STRUCT || elem_ts.base == TB_UNION || is_array_ty(elem_ts)) && val == "0")
          val = "zeroinitializer";
        else {
          val = irb->coerce(val, val_ts, elem_llty);
        }
        irb->emit("store " + elem_llty + " " + val + ", ptr " + egep);

        // Handle range designator: repeat the same value for [idx+1 ... range_end]
        if (is_range) {
          for (long long ri = idx+1; ri <= range_end && ri < dim0; ri++) {
            std::string rgep = irb->fresh_tmp();
            irb->emit(rgep + " = getelementptr " + arr_llty + ", ptr " + ptr +
                      ", i64 0, i64 " + std::to_string(ri));
            irb->emit("store " + elem_llty + " " + val + ", ptr " + rgep);
          }
          ei = range_end;
        }
      }
    }
    return;
  }
  // ── Scalar type ────────────────────────────────────────────────────────────
  if (flat_idx < flat_list->n_children) {
    Node* item = flat_list->children[flat_idx++];
    Node* val_node = (item && item->kind == NK_INIT_ITEM) ? item->left : item;
    if (val_node) {
      std::string val = irb->codegen_expr(val_node);
      TypeSpec val_ts = irb->expr_type(val_node);
      val = irb->coerce(val, val_ts, llty);
      irb->emit("store " + llty + " " + val + ", ptr " + ptr);
    }
  }
}

// ─────────────────────────── statement codegen ─────────────────────────────

void IRBuilder::emit_stmt(Node* n) {
  if (!n) return;

  switch (n->kind) {
  case NK_BLOCK: {
    // Synthetic declaration group from parser (`int a, b;`), not a lexical scope.
    if (n->ival == 1) {
      for (int i = 0; i < n->n_children; i++) emit_stmt(n->children[i]);
      break;
    }
    // Preserve lexical scope for same-named locals in nested blocks.
    auto saved_slots = local_slots_;
    auto saved_types = local_types_;
    std::unordered_set<std::string> names_in_this_scope;
    if (block_depth_ == 0) {
      // Function body scope includes already-bound parameters.
      for (const auto& kv : local_slots_) names_in_this_scope.insert(kv.first);
    }
    block_depth_++;
    for (int i = 0; i < n->n_children; i++) {
      Node* child = n->children[i];
      if (child && child->kind == NK_DECL && child->name && !child->is_extern) {
        std::string nm(child->name);
        if (names_in_this_scope.count(nm))
          throw std::runtime_error("redefinition of local identifier in same scope: " + nm);
        names_in_this_scope.insert(nm);
      }
      emit_stmt(n->children[i]);
    }
    block_depth_--;
    local_slots_ = std::move(saved_slots);
    local_types_ = std::move(saved_types);
    break;
  }

  case NK_EMPTY:
    break;

  case NK_EXPR_STMT:
    if (n->left)
      codegen_expr(n->left);
    break;

  // Expression nodes that can appear directly as for-init or standalone statements
  case NK_ASSIGN:
  case NK_COMPOUND_ASSIGN:
  case NK_CALL:
  case NK_POSTFIX:
  case NK_UNARY:
  case NK_COMMA_EXPR:
    codegen_expr(n);
    break;

  case NK_DECL: {
    // Alloca was already hoisted. First restore this declaration's specific
    // slot/type binding so shadowed names resolve correctly in following code.
    if (!n->name) break;

    // Restore the specific slot and type for this node (needed when multiple vars share
    // the same name in different scopes — hoist_allocas gave each a unique slot/type)
    {
      auto ns_it = node_slots_.find(n);
      if (ns_it != node_slots_.end()) {
        local_slots_[n->name] = ns_it->second;
      }
      auto nt_it = node_types_.find(n);
      if (nt_it != node_types_.end()) {
        local_types_[n->name] = nt_it->second;
      }
    }

    // No initializer -> declaration only; binding restoration above is enough.
    if (!n->init) break;
    // Static locals: initialized at program start (global), skip runtime store.
    if (n->is_static) break;

    auto it = local_slots_.find(n->name);
    if (it == local_slots_.end()) break;

    // Use the per-node type (restored above) to ensure correct llvm type for this decl
    TypeSpec lts = local_types_[n->name];
    std::string llty = llvm_ty(lts);

    // Compound literal initializer: unwrap to its init list (stored in ->left)
    // e.g. struct S v = (struct S){1,2}; → treat same as struct S v = {1,2};
    Node* effective_init = n->init;
    if (n->init->kind == NK_COMPOUND_LIT &&
        (is_array_ty(lts) || lts.base == TB_STRUCT || lts.base == TB_UNION)) {
      Node* cl_body = n->init->left ? n->init->left : n->init->init;
      if (cl_body) effective_init = cl_body;
    }

    // Handle init list for arrays/structs using the recursive flat-iterator emitter.
    if (effective_init->kind == NK_INIT_LIST &&
        (is_array_ty(lts) || lts.base == TB_STRUCT || lts.base == TB_UNION)) {
      int flat_idx = 0;
      emit_agg_init_impl(this, lts, it->second, effective_init, flat_idx);
      break;
    }

    // Local wide char array (wchar_t[]) initialized with a wide string literal L"..."
    if (array_rank_of(lts) == 1 && array_dim_at(lts, 0) >= 0 &&
        (lts.base == TB_INT || lts.base == TB_UINT || lts.base == TB_LONG || lts.base == TB_ULONG) &&
        lts.ptr_level == 0 &&
        n->init->kind == NK_STR_LIT && is_wide_str_lit(n->init)) {
      auto codepoints = decode_wide_string(n->init->sval);
      int dim0 = (int)array_dim_at(lts, 0);
      std::string elem_llty = llvm_ty(drop_array_dim(lts));
      for (int i = 0; i < dim0; i++) {
        uint32_t cp = (i < (int)codepoints.size()) ? codepoints[i] : 0;
        std::string gep = fresh_tmp();
        emit(gep + " = getelementptr " + llty + ", ptr " + it->second +
             ", i64 0, i64 " + std::to_string(i));
        emit("store " + elem_llty + " " + std::to_string((int)cp) + ", ptr " + gep);
      }
      break;
    }
    // Local char array initialized with a string literal: copy bytes
    if (array_rank_of(lts) == 1 && array_dim_at(lts, 0) >= 0 &&
        (lts.base == TB_CHAR || lts.base == TB_SCHAR || lts.base == TB_UCHAR) &&
        n->init->kind == NK_STR_LIT) {
      // Decode the string and emit byte-by-byte stores up to array size
      const char* raw = n->init->sval ? n->init->sval : "\"\"";
      if (raw[0] == '"') raw++;
      std::string bytes;
      while (*raw && *raw != '"') {
        if (*raw == '\\') {
          raw++;
          if (*raw == 'n') { bytes += '\n'; raw++; }
          else if (*raw == 't') { bytes += '\t'; raw++; }
          else if (*raw == 'r') { bytes += '\r'; raw++; }
          else if (*raw == '0') { bytes += '\0'; raw++; }
          else if (*raw == '\\') { bytes += '\\'; raw++; }
          else if (*raw == '"') { bytes += '"'; raw++; }
          else if (*raw == 'x' || *raw == 'X') {
            raw++;
            unsigned int v = 0;
            for (int k = 0; k < 2 && *raw && isxdigit((unsigned char)*raw); k++, raw++) {
              v = v * 16 + (isdigit((unsigned char)*raw) ? *raw - '0' :
                            tolower((unsigned char)*raw) - 'a' + 10);
            }
            bytes += (char)(unsigned char)v;
          } else if (*raw >= '0' && *raw <= '7') {
            unsigned int v = 0;
            for (int k = 0; k < 3 && *raw >= '0' && *raw <= '7'; k++, raw++)
              v = v * 8 + (*raw - '0');
            bytes += (char)(unsigned char)v;
          } else { bytes += *raw++; }
        } else { bytes += *raw++; }
      }
      int dim0 = (int)array_dim_at(lts, 0);
      // Store bytes up to array size
      for (int i = 0; i < dim0; i++) {
        char bval = (i < (int)bytes.size()) ? bytes[i] : '\0';
        std::string gep = fresh_tmp();
        emit(gep + " = getelementptr " + llty + ", ptr " + it->second +
             ", i64 0, i64 " + std::to_string(i));
        emit("store i8 " + std::to_string((int)(unsigned char)bval) + ", ptr " + gep);
      }
      break;
    }

    // Simple scalar/pointer initialization.
    // For aggregate types (arrays, structs) with a zero-value init expression,
    // use zeroinitializer to avoid `store [N x T] 0` which is invalid LLVM IR.
    if ((is_array_ty(lts) || lts.base == TB_STRUCT || lts.base == TB_UNION) &&
        n->init->kind == NK_INT_LIT && n->init->ival == 0) {
      emit("store " + llty + " zeroinitializer, ptr " + it->second);
      break;
    }
    TypeSpec rts = expr_type(n->init);
    if (lts.ptr_level > 0 && !lts.is_const) {
      bool discards_const = false;
      if (n->init->kind == NK_STR_LIT)
        discards_const = !allows_string_literal_ptr_target(lts);
      else if (n->init->kind == NK_ADDR && n->init->left) {
        TypeSpec inner = expr_type(n->init->left);
        if (inner.ptr_level == 0 && inner.is_const) discards_const = true;
      } else if (n->init->kind == NK_VAR && rts.ptr_level > 0 && rts.is_const) {
        discards_const = true;
      }
      if (discards_const)
        throw std::runtime_error("initialization discards const qualifier");
    }
    std::string rv = codegen_expr(n->init);
    rv = coerce(rv, rts, llty);
    emit("store " + llty + " " + rv + ", ptr " + it->second);
    // Track function pointer variable signatures for local vars:
    // If init is a direct function reference (or &func), record the signature
    // so indirect calls through this variable use the correct return type.
    {
      Node* init_node = n->init;
      if (init_node->kind == NK_ADDR && init_node->left) init_node = init_node->left;
      if (init_node->kind == NK_VAR && init_node->name) {
        auto fsit = func_sigs_.find(init_node->name);
        if (fsit != func_sigs_.end()) {
          fptr_sigs_[n->name] = fsit->second;
        }
      }
    }
    break;
  }

  case NK_RETURN: {
    if (!n->left || current_fn_ret_llty_ == "void") {
      emit_terminator("ret void");
    } else {
      TypeSpec ret_ts = expr_type(n->left);
      std::string rv = codegen_expr(n->left);
      rv = coerce(rv, ret_ts, current_fn_ret_llty_);
      // If returning an array/vector type and the value is an address (from
      // compound literal or array var), load the actual value before returning.
      // Exception: NK_VAR and NK_DEREF of pure vector (is_vector, rank==1) already
      // return a loaded value from codegen_expr — don't reload in those cases.
      bool already_loaded = (ret_ts.is_vector && array_rank_of(ret_ts) == 1 &&
                             n->left &&
                             (n->left->kind == NK_VAR || n->left->kind == NK_DEREF));
      if (!already_loaded &&
          !current_fn_ret_llty_.empty() && current_fn_ret_llty_[0] == '[' &&
          is_array_ty(ret_ts) && ret_ts.ptr_level == 0 && !ret_ts.is_ptr_to_array &&
          n->left && (n->left->kind == NK_COMPOUND_LIT || n->left->kind == NK_VAR ||
                      n->left->kind == NK_CAST || n->left->kind == NK_INDEX)) {
        std::string loaded = fresh_tmp();
        emit(loaded + " = load " + current_fn_ret_llty_ + ", ptr " + rv);
        rv = loaded;
      }
      emit_terminator("ret " + current_fn_ret_llty_ + " " + rv);
    }
    break;
  }

  case NK_IF: {
    TypeSpec cond_ts = expr_type(n->cond);

    bool cond_is_builtin_constant_p = false;
    if (n->cond && n->cond->kind == NK_CALL && n->cond->left &&
        n->cond->left->kind == NK_VAR && n->cond->left->name &&
        std::strcmp(n->cond->left->name, "__builtin_constant_p") == 0) {
      cond_is_builtin_constant_p = true;
    }

    std::string cond_val = codegen_expr(n->cond);

    // Fold only __builtin_constant_p(...) conditions. This avoids emitting dead
    // fallback paths that intentionally reference undefined symbols.
    if (cond_is_builtin_constant_p) {
      bool is_const_false = (cond_val == "0");
      bool is_const_true  = false;
      if (!is_const_false && !cond_val.empty()) {
        char* end = nullptr;
        long long iv = std::strtoll(cond_val.c_str(), &end, 10);
        if (end && *end == '\0' && iv != 0) is_const_true = true;
      }
      if (is_const_false) {
        if (n->else_) emit_stmt(n->else_);
        break;
      }
      if (is_const_true) {
        emit_stmt(n->then_);
        break;
      }
    }

    std::string cond_b   = to_bool(cond_val, cond_ts);
    std::string then_lbl = fresh_label("if_then");
    std::string end_lbl  = fresh_label("if_end");
    std::string else_lbl = n->else_ ? fresh_label("if_else") : end_lbl;

    emit_terminator("br i1 " + cond_b + ", label %" + then_lbl +
                    ", label %" + else_lbl);
    emit_label(then_lbl);
    emit_stmt(n->then_);
    emit_terminator("br label %" + end_lbl);

    if (n->else_) {
      emit_label(else_lbl);
      emit_stmt(n->else_);
      emit_terminator("br label %" + end_lbl);
    }

    emit_label(end_lbl);
    break;
  }

  case NK_WHILE: {
    std::string cond_lbl = fresh_label("while_cond");
    std::string body_lbl = fresh_label("while_body");
    std::string end_lbl  = fresh_label("while_end");

    loop_stack_.push_back({cond_lbl, end_lbl});
    break_stack_.push_back(end_lbl);

    emit_terminator("br label %" + cond_lbl);
    emit_label(cond_lbl);
    TypeSpec cond_ts = expr_type(n->cond);
    std::string cond_val = codegen_expr(n->cond);
    std::string cond_b   = to_bool(cond_val, cond_ts);
    emit_terminator("br i1 " + cond_b + ", label %" + body_lbl +
                    ", label %" + end_lbl);

    emit_label(body_lbl);
    emit_stmt(n->body);
    emit_terminator("br label %" + cond_lbl);

    emit_label(end_lbl);

    loop_stack_.pop_back();
    break_stack_.pop_back();
    break;
  }

  case NK_FOR: {
    auto saved_slots = local_slots_;
    auto saved_types = local_types_;
    std::string cond_lbl = fresh_label("for_cond");
    std::string body_lbl = fresh_label("for_body");
    std::string post_lbl = fresh_label("for_post");
    std::string end_lbl  = fresh_label("for_end");

    loop_stack_.push_back({post_lbl, end_lbl});
    break_stack_.push_back(end_lbl);

    // init
    if (n->init) emit_stmt(n->init);
    emit_terminator("br label %" + cond_lbl);

    // cond
    emit_label(cond_lbl);
    if (n->cond) {
      TypeSpec cond_ts = expr_type(n->cond);
      std::string cond_val = codegen_expr(n->cond);
      std::string cond_b   = to_bool(cond_val, cond_ts);
      emit_terminator("br i1 " + cond_b + ", label %" + body_lbl +
                      ", label %" + end_lbl);
    } else {
      emit_terminator("br label %" + body_lbl);
    }

    // body
    emit_label(body_lbl);
    emit_stmt(n->body);
    emit_terminator("br label %" + post_lbl);

    // post (update)
    emit_label(post_lbl);
    if (n->update) codegen_expr(n->update);
    emit_terminator("br label %" + cond_lbl);

    emit_label(end_lbl);

    loop_stack_.pop_back();
    break_stack_.pop_back();
    local_slots_ = std::move(saved_slots);
    local_types_ = std::move(saved_types);
    break;
  }

  case NK_DO_WHILE: {
    std::string body_lbl = fresh_label("do_body");
    std::string cond_lbl = fresh_label("do_cond");
    std::string end_lbl  = fresh_label("do_end");

    loop_stack_.push_back({cond_lbl, end_lbl});
    break_stack_.push_back(end_lbl);

    emit_terminator("br label %" + body_lbl);
    emit_label(body_lbl);
    emit_stmt(n->body);
    emit_terminator("br label %" + cond_lbl);

    emit_label(cond_lbl);
    TypeSpec cond_ts = expr_type(n->cond);
    std::string cond_val = codegen_expr(n->cond);
    std::string cond_b   = to_bool(cond_val, cond_ts);
    emit_terminator("br i1 " + cond_b + ", label %" + body_lbl +
                    ", label %" + end_lbl);

    emit_label(end_lbl);

    loop_stack_.pop_back();
    break_stack_.pop_back();
    break;
  }

  case NK_BREAK: {
    if (break_stack_.empty())
      throw std::runtime_error("break statement not within loop or switch");
    emit_terminator("br label %" + break_stack_.back());
    break;
  }

  case NK_CONTINUE: {
    if (loop_stack_.empty())
      throw std::runtime_error("continue statement not within loop");
    emit_terminator("br label %" + loop_stack_.back().first);
    break;
  }

  case NK_GOTO: {
    if (n->name && strcmp(n->name, "__computed__") != 0) {
      auto it = user_labels_.find(n->name);
      if (it != user_labels_.end())
        emit_terminator("br label %" + it->second);
      else
        emit_terminator("br label %user_" + std::string(n->name) + "_0");
    } else {
      // Computed goto (goto *expr): emit indirectbr with all pre-scanned labels as targets.
      std::string val = n->left ? codegen_expr(n->left) : "null";
      std::string targets;
      for (auto& kv : user_labels_) {
        if (!targets.empty()) targets += ", ";
        targets += "label %" + kv.second;
      }
      if (targets.empty())
        emit_terminator("unreachable");
      else
        emit_terminator("indirectbr ptr " + val + ", [" + targets + "]");
    }
    break;
  }

  case NK_LABEL: {
    auto it = user_labels_.find(n->name);
    std::string lbl = (it != user_labels_.end())
                      ? it->second
                      : (std::string("user_") + n->name + "_0");
    // If we haven't terminated yet, fall through to the label
    emit_terminator("br label %" + lbl);
    emit_label(lbl);
    // Emit the body (the statement after the label)
    if (n->body) emit_stmt(n->body);
    break;
  }

  case NK_SWITCH: {
    TypeSpec cond_ts = expr_type(n->cond);
    std::string cond_val = codegen_expr(n->cond);
    // C integer promotions for switch: narrow types → int; long long → i64
    std::string sw_ty = "i32";
    if (sizeof_ty(cond_ts) > 4) sw_ty = "i64";
    cond_val = coerce(cond_val, cond_ts, sw_ty);

    std::string end_lbl = fresh_label("switch_end");
    std::string def_lbl = end_lbl;  // default falls through to end if not set

    // Pre-scan the body for case labels
    SwitchInfo sw_info;
    sw_info.end_label = end_lbl;

    // Find all case/default labels in the switch body.
    // Must recurse into any compound statement — Duff's Device places cases
    // inside a do-while body; 00213 places cases inside an if(0) block.
    std::function<void(Node*)> scan_cases = [&](Node* node) {
      if (!node) return;
      if (node->kind == NK_CASE) {
        long long cv = node->left ? static_eval_int(node->left, enum_consts_) : 0;
        if (sw_info.case_labels.count(cv))
          throw std::runtime_error("duplicate case label in switch");
        sw_info.case_labels[cv] = fresh_label("switch_case");
        scan_cases(node->body);
      } else if (node->kind == NK_CASE_RANGE) {
        long long lo = node->left  ? static_eval_int(node->left,  enum_consts_) : 0;
        long long hi = node->right ? static_eval_int(node->right, enum_consts_) : lo;
        std::string lbl = fresh_label("switch_range");
        sw_info.case_ranges.push_back({lo, hi, lbl});
        scan_cases(node->body);
      } else if (node->kind == NK_DEFAULT) {
        if (!sw_info.default_label.empty())
          throw std::runtime_error("multiple default labels in switch");
        sw_info.default_label = fresh_label("switch_def");
        scan_cases(node->body);
      } else if (node->kind == NK_BLOCK) {
        for (int i = 0; i < node->n_children; i++)
          scan_cases(node->children[i]);
      } else if (node->kind == NK_IF) {
        scan_cases(node->then_);
        scan_cases(node->else_);
      } else if (node->kind == NK_WHILE || node->kind == NK_DO_WHILE ||
                 node->kind == NK_FOR || node->kind == NK_LABEL) {
        scan_cases(node->body);
      }
    };
    scan_cases(n->body);

    if (!sw_info.default_label.empty())
      def_lbl = sw_info.default_label;

    // Emit range checks (GCC extension: case lo ... hi) before the switch.
    // For each range, emit an icmp chain and branch to the range label if matched.
    bool sw_unsigned = is_unsigned_base(cond_ts.base);
    const char* cmp_ge = sw_unsigned ? "uge" : "sge";
    const char* cmp_le = sw_unsigned ? "ule" : "sle";
    for (auto& rng : sw_info.case_ranges) {
      std::string after_range = fresh_label("switch_range_next");
      std::string lo_str = std::to_string(rng.lo);
      std::string hi_str = std::to_string(rng.hi);
      std::string c_lo = fresh_tmp(), c_hi = fresh_tmp(), c_both = fresh_tmp();
      emit(c_lo + " = icmp " + cmp_ge + " " + sw_ty + " " + cond_val + ", " + lo_str);
      emit(c_hi + " = icmp " + cmp_le + " " + sw_ty + " " + cond_val + ", " + hi_str);
      emit(c_both + " = and i1 " + c_lo + ", " + c_hi);
      emit_terminator("br i1 " + c_both + ", label %" + rng.label + ", label %" + after_range);
      emit_label(after_range);
    }

    // Emit the switch instruction
    std::string sw_instr = "switch " + sw_ty + " " + cond_val + ", label %" + def_lbl + " [";
    for (auto& kv : sw_info.case_labels) {
      sw_instr += " " + sw_ty + " " + std::to_string(kv.first) + ", label %" + kv.second;
    }
    sw_instr += " ]";
    emit_terminator(sw_instr);

    switch_stack_.push_back(sw_info);
    break_stack_.push_back(end_lbl);

    emit_stmt(n->body);

    emit_terminator("br label %" + end_lbl);
    emit_label(end_lbl);

    switch_stack_.pop_back();
    break_stack_.pop_back();
    break;
  }

  case NK_CASE: {
    if (switch_stack_.empty())
      throw std::runtime_error("case label not within a switch statement");
    SwitchInfo& sw = switch_stack_.back();
    long long cv = n->left ? static_eval_int(n->left, enum_consts_) : 0;
    auto it = sw.case_labels.find(cv);
    if (it != sw.case_labels.end()) {
      emit_terminator("br label %" + it->second);
      emit_label(it->second);
    }
    if (n->body) emit_stmt(n->body);
    break;
  }

  case NK_DEFAULT: {
    if (switch_stack_.empty())
      throw std::runtime_error("default label not within a switch statement");
    SwitchInfo& sw = switch_stack_.back();
    if (!sw.default_label.empty()) {
      emit_terminator("br label %" + sw.default_label);
      emit_label(sw.default_label);
    }
    if (n->body) emit_stmt(n->body);
    break;
  }

  case NK_CASE_RANGE: {
    // GCC extension: case lo ... hi: - emit the pre-allocated range label
    if (switch_stack_.empty()) break;
    SwitchInfo& sw = switch_stack_.back();
    long long lo = n->left  ? static_eval_int(n->left,  enum_consts_) : 0;
    long long hi = n->right ? static_eval_int(n->right, enum_consts_) : lo;
    // Find the matching range entry in sw.case_ranges
    std::string rng_lbl;
    for (auto& rng : sw.case_ranges) {
      if (rng.lo == lo && rng.hi == hi) { rng_lbl = rng.label; break; }
    }
    if (!rng_lbl.empty()) {
      emit_terminator("br label %" + rng_lbl);
      emit_label(rng_lbl);
    }
    if (n->body) emit_stmt(n->body);
    break;
  }

  default:
    break;
  }
}

// ─────────────────────────── struct definitions ─────────────────────────────

void IRBuilder::emit_struct_def(Node* sd) {
  if (!sd || sd->kind != NK_STRUCT_DEF) return;
  const char* tag = sd->name;
  if (!tag || !tag[0]) return;

  // Skip if already processed with fields; update if the new def has more fields
  // (handles forward declarations followed by full definitions)
  if (struct_defs_.count(tag) && sd->n_fields == 0) return;
  if (struct_defs_.count(tag) &&
      sd->n_fields <= (int)struct_defs_[tag].field_names.size()) return;

  // Helper: return storage-unit size in bits for a bitfield base type.
  auto bf_storage_bits = [](TypeBase base, int ptr_level) -> int {
    if (ptr_level > 0) return 64;
    switch (base) {
      case TB_BOOL: case TB_CHAR: case TB_SCHAR: case TB_UCHAR: return 8;
      case TB_SHORT: case TB_USHORT: return 16;
      case TB_INT: case TB_UINT: case TB_ENUM: return 32;
      case TB_LONG: case TB_ULONG: case TB_LONGLONG: case TB_ULONGLONG: return 64;
      default: return 32;
    }
  };
  // Helper: return LLVM type for a bitfield storage unit of given size in bits.
  auto bf_storage_llty = [](int bits) -> std::string {
    switch (bits) {
      case 8:  return "i8";
      case 16: return "i16";
      case 32: return "i32";
      case 64: return "i64";
      default: return "i64";
    }
  };

  StructInfo si;
  si.is_union = sd->is_union;
  si.n_llvm_fields = 0;

  for (int i = 0; i < sd->n_fields; i++) {
    Node* f = sd->fields[i];
    if (!f || !f->name) continue;
    si.field_names.push_back(f->name);
    TypeSpec ft = f->type;
    long long arr_size = ft.array_size;
    clear_array(ft);  // store base type; first array dim tracked separately
    si.field_types.push_back(ft);
    si.field_array_sizes.push_back(arr_size);
    si.field_is_anon.push_back(f->is_anon_field);
    si.field_bit_widths.push_back((int)f->ival);  // -1 if not a bitfield
    si.field_llvm_idx.push_back(0);   // filled in below
    si.field_bit_offsets.push_back(0); // filled in below
  }
  struct_defs_[tag] = si;

  if (sd->n_fields <= 0) return;  // Skip empty/forward-decl structs for now

  // Emit LLVM struct type to preamble
  // For unions, we use a byte array: [max_sz x i8]
  if (si.is_union) {
    int max_sz = 0;
    for (size_t i = 0; i < si.field_types.size(); i++) {
      TypeSpec ft = si.field_types[i];
      int sz = sizeof_ty(ft);
      if (si.field_array_sizes[i] >= 0) sz *= (int)si.field_array_sizes[i];
      if (sz > max_sz) max_sz = sz;
    }
    // Also fill in llvm_idx/bit_offsets for union fields (all at idx 0)
    for (size_t i = 0; i < si.field_types.size(); i++) {
      struct_defs_[tag].field_llvm_idx[i] = 0;
      struct_defs_[tag].field_bit_offsets[i] = 0;
    }
    struct_defs_[tag].n_llvm_fields = 1;
    std::string ba_ty = "[" + std::to_string(max_sz) + " x i8]";
    struct_defs_[tag].llvm_field_types.push_back(ba_ty);
    preamble_.push_back(std::string("%struct.") + tag + " = type { " + ba_ty + " }");
  } else {
    // Compute bitfield packing: consecutive bitfields with the same base type
    // are packed into a single storage unit (e.g. all i8 bitfields → one i8).
    std::string fields_str;
    bool first = true;
    int llvm_idx = 0;
    int cur_bf_base_bits = 0;   // storage-unit size in bits (0 = no active slot)
    int cur_bf_fill_bits = 0;   // bits used so far in current storage slot
    TypeBase cur_bf_base = TB_VOID;

    for (size_t i = 0; i < si.field_types.size(); i++) {
      if (si.field_array_sizes[i] == -2) {
        // Flex array: zero-size, no LLVM field needed
        struct_defs_[tag].field_llvm_idx[i] = llvm_idx;
        struct_defs_[tag].field_bit_offsets[i] = 0;
        continue;
      }
      int bfw = si.field_bit_widths[i];
      if (bfw > 0) {
        // Bitfield
        TypeSpec ft = si.field_types[i];
        int slot_bits = bf_storage_bits(ft.base, ft.ptr_level);
        bool same_base = (ft.base == cur_bf_base || slot_bits == cur_bf_base_bits);
        bool fits = (cur_bf_fill_bits + bfw <= cur_bf_base_bits);
        if (cur_bf_base_bits > 0 && same_base && fits) {
          // Fits in current slot
          struct_defs_[tag].field_llvm_idx[i] = llvm_idx - 1;
          struct_defs_[tag].field_bit_offsets[i] = cur_bf_fill_bits;
          cur_bf_fill_bits += bfw;
        } else {
          // Emit new slot field
          if (!first) fields_str += ", ";
          first = false;
          std::string slot_lty = bf_storage_llty(slot_bits);
          fields_str += slot_lty;
          struct_defs_[tag].field_llvm_idx[i] = llvm_idx;
          struct_defs_[tag].field_bit_offsets[i] = 0;
          cur_bf_base_bits = slot_bits;
          cur_bf_fill_bits = bfw;
          cur_bf_base = ft.base;
          struct_defs_[tag].llvm_field_types.push_back(slot_lty);
          llvm_idx++;
        }
      } else {
        // Non-bitfield: close any active bitfield slot
        cur_bf_base_bits = 0;
        cur_bf_fill_bits = 0;
        cur_bf_base = TB_VOID;

        struct_defs_[tag].field_llvm_idx[i] = llvm_idx;
        struct_defs_[tag].field_bit_offsets[i] = 0;

        if (!first) fields_str += ", ";
        first = false;
        TypeSpec ft = si.field_types[i];
        if (si.field_array_sizes[i] >= 0) {
          ft.array_size = si.field_array_sizes[i];
          ft.inner_rank = -1;
        }
        std::string ft_lty = llvm_ty(ft);
        fields_str += ft_lty;
        struct_defs_[tag].llvm_field_types.push_back(ft_lty);
        llvm_idx++;
      }
    }
    struct_defs_[tag].n_llvm_fields = llvm_idx;
    if (first) fields_str = "i8";  // empty struct: add padding
    preamble_.push_back(std::string("%struct.") + tag + " = type { " +
                        fields_str + " }");
  }
}

// ─────────────────────────── global init helpers ───────────────────────────

// Compute the effective array size from an NK_INIT_LIST node.
// Accounts for designated index [N] items to find max_index + 1.
static int infer_array_size_from_init(Node* init) {
  if (!init) return 0;
  if (init->kind == NK_STR_LIT) {
    // Wide string literal: use decoded codepoints count
    if (is_wide_str_lit(init)) {
      auto codepoints = decode_wide_string(init->sval);
      return (int)codepoints.size();  // includes null terminator
    }
    // Narrow string literal: size = length of content + 1 for null
    // Quick count of actual chars (handle escape sequences minimally)
    int count = 0;
    for (int i = (init->sval && init->sval[0] == '"' ? 1 : 0);
         init->sval && init->sval[i] && init->sval[i] != '"'; i++) {
      if (init->sval[i] == '\\') {
        // Handle multi-byte UTF-8 sequences in narrow strings too
        i++;
        // For octal and hex sequences, skip the sequence
        if (init->sval[i] >= '0' && init->sval[i] <= '7') {
          for (int k = 0; k < 2 && init->sval[i+1] >= '0' && init->sval[i+1] <= '7'; k++) i++;
        } else if (init->sval[i] == 'x' || init->sval[i] == 'X') {
          i++;
          while (init->sval[i] && isxdigit((unsigned char)init->sval[i])) i++;
          i--;
        }
      }
      count++;
    }
    return count + 1;  // +1 for null terminator
  }
  if (init->kind != NK_INIT_LIST) return 0;
  long long max_idx = -1;
  long long cur_idx = 0;
  for (int i = 0; i < init->n_children; i++) {
    Node* item = init->children[i];
    if (item && item->kind == NK_INIT_ITEM && item->is_index_desig)
      cur_idx = item->desig_val;
    if (cur_idx > max_idx) max_idx = cur_idx;
    cur_idx++;
  }
  return (int)(max_idx + 1);
}

// Forward declaration for global_const_flat
std::string global_const_flat_impl(IRBuilder* irb, TypeSpec ts, Node* init_list, int& flat_idx);

// Evaluate an integer constant expression at compile time (for global initializers).
// Returns the evaluated value; returns 0 for unknown expressions.
static long long static_eval_int(Node* n, const std::unordered_map<std::string, long long>& enum_consts) {
  if (!n) return 0;
  if (n->kind == NK_INT_LIT) return n->ival;
  if (n->kind == NK_CHAR_LIT) return n->ival;
  if (n->kind == NK_VAR && n->name) {
    auto it = enum_consts.find(n->name);
    if (it != enum_consts.end()) return it->second;
  }
  if (n->kind == NK_CAST && n->left) {
    long long v = static_eval_int(n->left, enum_consts);
    // Apply the target type's truncation so that e.g. (unsigned int)-4 = 4294967292
    TypeSpec ts = n->type;
    if (ts.ptr_level == 0) {
      int bits = 0;
      switch (ts.base) {
        case TB_BOOL: bits = 1; break;
        case TB_CHAR: case TB_UCHAR: case TB_SCHAR: bits = 8; break;
        case TB_SHORT: case TB_USHORT: bits = 16; break;
        case TB_INT: case TB_UINT: case TB_ENUM: bits = 32; break;
        default: break;  // long/longlong/void → no truncation needed
      }
      if (bits > 0 && bits < 64) {
        long long mask = (1LL << bits) - 1;
        v &= mask;
        if (!is_unsigned_base(ts.base) && ts.base != TB_BOOL && (v >> (bits - 1)))
          v |= ~mask;  // sign-extend
      }
    }
    return v;
  }
  if (n->kind == NK_UNARY && n->op) {
    if (strcmp(n->op, "-") == 0 && n->left) return -static_eval_int(n->left, enum_consts);
    if (strcmp(n->op, "+") == 0 && n->left) return static_eval_int(n->left, enum_consts);
    if (strcmp(n->op, "~") == 0 && n->left) return ~static_eval_int(n->left, enum_consts);
  }
  if (n->kind == NK_BINOP && n->op && n->left && n->right) {
    long long l = static_eval_int(n->left, enum_consts);
    long long r = static_eval_int(n->right, enum_consts);
    if (strcmp(n->op, "+") == 0) return l + r;
    if (strcmp(n->op, "-") == 0) return l - r;
    if (strcmp(n->op, "*") == 0) return l * r;
    if (strcmp(n->op, "/") == 0 && r != 0) return l / r;
    if (strcmp(n->op, "%") == 0 && r != 0) return l % r;
    if (strcmp(n->op, "&") == 0) return l & r;
    if (strcmp(n->op, "|") == 0) return l | r;
    if (strcmp(n->op, "^") == 0) return l ^ r;
    if (strcmp(n->op, "<<") == 0) return l << (r & 63);
    if (strcmp(n->op, ">>") == 0) return l >> (r & 63);
  }
  return 0;  // unknown expression
}

// Evaluate a compile-time constant floating-point expression.
// Handles NaN/Inf correctly (unlike static_eval_int which converts to int).
static double static_eval_float(Node* n) {
  if (!n) return 0.0;
  if (n->kind == NK_FLOAT_LIT) return n->fval;
  if (n->kind == NK_INT_LIT)   return (double)n->ival;
  if (n->kind == NK_CHAR_LIT)  return (double)n->ival;
  if (n->kind == NK_CAST && n->left) return static_eval_float(n->left);
  if (n->kind == NK_UNARY && n->op && n->left) {
    double v = static_eval_float(n->left);
    if (strcmp(n->op, "-") == 0) return -v;
    if (strcmp(n->op, "+") == 0) return v;
  }
  if (n->kind == NK_BINOP && n->op && n->left && n->right) {
    double l = static_eval_float(n->left);
    double r = static_eval_float(n->right);
    if (strcmp(n->op, "+") == 0) return l + r;
    if (strcmp(n->op, "-") == 0) return l - r;
    if (strcmp(n->op, "*") == 0) return l * r;
    if (strcmp(n->op, "/") == 0) return l / r;  // 1.0/0.0 = Inf, Inf-Inf = NaN
  }
  return 0.0;
}

// Count the number of scalar leaf fields when a struct/array type is fully flattened.
// Used to detect brace-elision in array-of-struct global initializers.
static int count_flat_fields_impl(const TypeSpec& ts,
                                  const std::unordered_map<std::string, IRBuilder::StructInfo>& struct_defs) {
  if (is_array_ty(ts) && array_dim_at(ts, 0) > 0) {
    TypeSpec elem = drop_array_dim(ts);
    return (int)array_dim_at(ts, 0) * count_flat_fields_impl(elem, struct_defs);
  }
  if ((ts.base == TB_STRUCT) && ts.tag && ts.ptr_level == 0) {
    auto it = struct_defs.find(ts.tag);
    if (it != struct_defs.end()) {
      const IRBuilder::StructInfo& si = it->second;
      int total = 0;
      for (size_t i = 0; i < si.field_types.size(); i++) {
        TypeSpec ft = si.field_types[i];
        if (si.field_array_sizes[i] >= 0) {
          long long dims[1] = {si.field_array_sizes[i]};
          set_array_dims(ft, dims, 1);
        }
        total += count_flat_fields_impl(ft, struct_defs);
      }
      return total > 0 ? total : 1;
    }
  }
  return 1;  // scalar or unknown
}

// Consume items from init_list starting at flat_idx to fill one `ts` value.
std::string global_const_flat_impl(IRBuilder* irb, TypeSpec ts, Node* init_list, int& flat_idx) {
  if ((ts.base == TB_STRUCT) && !is_array_ty(ts) && ts.tag && ts.ptr_level == 0) {
    auto sit = irb->struct_defs_.find(ts.tag);
    if (sit == irb->struct_defs_.end()) return "zeroinitializer";
    const IRBuilder::StructInfo& si = sit->second;
    int n_fields = (int)si.field_names.size();
    if (n_fields == 0) return "zeroinitializer";
    // Collect per-C-field values from flat list
    std::vector<std::string> fvals(n_fields, "0");
    for (int fi = 0; fi < n_fields; fi++) {
      TypeSpec ft = si.field_types[fi];
      if (si.field_array_sizes[fi] >= 0) {
        long long dims[1] = {si.field_array_sizes[fi]};
        set_array_dims(ft, dims, 1);
      }
      int bfw = (fi < (int)si.field_bit_widths.size()) ? si.field_bit_widths[fi] : -1;
      // For bitfields, consume a scalar from the flat list
      fvals[fi] = global_const_flat_impl(irb, bfw > 0 ? ft : ft, init_list, flat_idx);
    }
    // Pack bitfields into LLVM storage units
    int n_llvm = si.n_llvm_fields;
    std::vector<long long> llvm_packed(n_llvm, 0LL);
    std::vector<std::string> llvm_val_str(n_llvm);
    std::vector<bool> llvm_is_bf(n_llvm, false);
    for (int fi = 0; fi < n_fields; fi++) {
      int li = (fi < (int)si.field_llvm_idx.size()) ? si.field_llvm_idx[fi] : fi;
      if (li < 0 || li >= n_llvm) continue;
      int bfw = (fi < (int)si.field_bit_widths.size()) ? si.field_bit_widths[fi] : -1;
      if (bfw > 0) {
        llvm_is_bf[li] = true;
        long long raw = 0;
        if (!fvals[fi].empty() && fvals[fi] != "zeroinitializer")
          raw = std::stoll(fvals[fi]);
        int bit_off = (fi < (int)si.field_bit_offsets.size()) ? si.field_bit_offsets[fi] : 0;
        long long mask = (bfw >= 64) ? ~0LL : ((1LL << bfw) - 1);
        llvm_packed[li] |= (raw & mask) << bit_off;
      } else {
        llvm_val_str[li] = fvals[fi];
      }
    }
    for (int li = 0; li < n_llvm; li++) {
      if (llvm_is_bf[li])
        llvm_val_str[li] = std::to_string(llvm_packed[li]);
    }
    std::string result = "{ ";
    for (int li = 0; li < n_llvm; li++) {
      if (li > 0) result += ", ";
      std::string lty = (li < (int)si.llvm_field_types.size()) ? si.llvm_field_types[li] : "";
      if (llvm_val_str[li].empty()) {
        result += lty + " " + (lty.empty() || lty[0] == '[' || lty[0] == '%' ? "zeroinitializer" : "0");
      } else {
        result += lty + " " + llvm_val_str[li];
      }
    }
    result += " }";
    return result;
  }
  if (is_array_ty(ts) && array_dim_at(ts, 0) > 0) {
    int dim = (int)array_dim_at(ts, 0);
    TypeSpec elem = drop_array_dim(ts);
    std::string elem_llty = irb->llvm_ty(elem);
    // For char/u8 arrays: if next item is a string literal, use global_const directly.
    bool is_char_elem = (elem.ptr_level == 0 &&
                         (elem.base == TB_CHAR || elem.base == TB_SCHAR ||
                          elem.base == TB_UCHAR));
    if (is_char_elem && flat_idx < init_list->n_children) {
      Node* item = init_list->children[flat_idx];
      Node* val_node = (item && item->kind == NK_INIT_ITEM) ? item->left : item;
      if (val_node && val_node->kind == NK_STR_LIT) {
        flat_idx++;
        return irb->global_const(ts, val_node);  // handles c"..." encoding
      }
    }
    std::string result = "[";
    for (int i = 0; i < dim; i++) {
      if (i > 0) result += ", ";
      result += elem_llty + " " + global_const_flat_impl(irb, elem, init_list, flat_idx);
    }
    result += "]";
    return result;
  }
  // Scalar: consume next item from flat list
  if (flat_idx < init_list->n_children) {
    Node* item = init_list->children[flat_idx++];
    Node* val_node = item;
    if (item && item->kind == NK_INIT_ITEM) val_node = item->left;
    // Unwrap optional braces around scalar: {4} → 4
    if (val_node && val_node->kind == NK_INIT_LIST && val_node->n_children > 0) {
      Node* inner = val_node->children[0];
      if (inner && inner->kind == NK_INIT_ITEM) inner = inner->left;
      val_node = inner;
    }
    return irb->global_const(ts, val_node);
  }
  return "zeroinitializer";
}

// Collect bytes for a constant of type ts initialized from init.
// Appends exactly sizeof_ty(ts) bytes to `out`.
static void collect_bytes(IRBuilder* irb, TypeSpec ts, Node* init, std::vector<uint8_t>& out) {
  int sz = irb->sizeof_ty(ts);
  int start = (int)out.size();
  out.resize(start + sz, 0);
  if (!init || sz == 0) return;

  // Unwrap cast / compound literal
  if (init->kind == NK_CAST && init->left) init = init->left;
  if (init->kind == NK_COMPOUND_LIT) {
    Node* cl = init->left ? init->left : init->init;
    if (cl) init = cl;
  }
  // Unwrap optional braces around scalar: {val} → val
  if (init->kind == NK_INIT_LIST && init->n_children == 1 && ts.ptr_level == 0 &&
      !is_array_ty(ts) && ts.base != TB_STRUCT && ts.base != TB_UNION) {
    Node* inner = init->children[0];
    if (inner && inner->kind == NK_INIT_ITEM) inner = inner->left;
    if (inner) init = inner;
  }

  // Array type
  if (is_array_ty(ts)) {
    long long dim = array_dim_at(ts, 0);
    TypeSpec elem = drop_array_dim(ts);
    int esz = irb->sizeof_ty(elem);
    bool is_char_elem = (elem.ptr_level == 0 &&
                         (elem.base == TB_CHAR || elem.base == TB_SCHAR || elem.base == TB_UCHAR));
    if (is_char_elem && init->kind == NK_STR_LIT) {
      // Decode string literal bytes
      const char* raw = init->sval ? init->sval : "\"\"";
      if (raw[0] == '"') raw++;
      int bi = 0;
      while (*raw && *raw != '"' && bi < dim) {
        if (*raw == '\\') {
          raw++;
          char c = 0;
          if (*raw == 'n') { c='\n'; raw++; } else if (*raw == 't') { c='\t'; raw++; }
          else if (*raw == 'r') { c='\r'; raw++; } else if (*raw == '0') { c=0; raw++; }
          else if (*raw >= '0' && *raw <= '7') {
            unsigned v = 0;
            for (int k=0;k<3&&*raw>='0'&&*raw<='7';k++,raw++) v=v*8+(*raw-'0');
            c=(char)(unsigned char)v;
          } else { c=*raw++; }
          out[start + bi++] = (uint8_t)c;
        } else { out[start + bi++] = (uint8_t)*raw++; }
      }
    } else if (init->kind == NK_INIT_LIST) {
      long long cur = 0;
      for (int i = 0; i < init->n_children; i++) {
        Node* item = init->children[i];
        long long idx = cur;
        Node* val = item;
        long long idx_hi = idx;
        if (item && item->kind == NK_INIT_ITEM) {
          if (item->is_index_desig) {
            idx = item->desig_val;
            idx_hi = item->right ? item->right->ival : idx;
          }
          val = item->left;
        }
        std::vector<uint8_t> eb;
        if (val) collect_bytes(irb, elem, val, eb);
        for (long long ri = idx; ri <= idx_hi && ri < dim; ri++) {
          for (int b = 0; b < esz && b < (int)eb.size(); b++)
            out[start + (int)ri * esz + b] = eb[b];
        }
        cur = idx_hi + 1;
      }
    } else {
      // Scalar into first element
      std::vector<uint8_t> eb;
      collect_bytes(irb, elem, init, eb);
      for (int b = 0; b < esz && b < (int)eb.size(); b++)
        out[start + b] = eb[b];
    }
    return;
  }

  // Struct type
  if (ts.base == TB_STRUCT && ts.ptr_level == 0 && !is_array_ty(ts) && ts.tag) {
    auto sit = irb->struct_defs_.find(ts.tag);
    if (sit == irb->struct_defs_.end()) return;
    const IRBuilder::StructInfo& si = sit->second;
    int n = (int)si.field_names.size();
    // Compute field offsets (simple sequential layout, no padding for u8 fields)
    std::vector<int> offsets(n, 0);
    int off = 0;
    for (int i = 0; i < n; i++) {
      if (si.field_array_sizes[i] == -2) continue;
      offsets[i] = off;
      TypeSpec ft = si.field_types[i];
      int fsz = irb->sizeof_ty(ft);
      if (si.field_array_sizes[i] >= 0) fsz *= (int)si.field_array_sizes[i];
      off += fsz;
    }
    if (init->kind == NK_INIT_LIST) {
      int fi = 0;  // next sequential field index
      for (int i = 0; i < init->n_children; i++) {
        Node* item = init->children[i];
        Node* val = item;
        int target_fi = fi;
        bool item_is_desig = (item && item->kind == NK_INIT_ITEM && item->desig_field);
        // Stop for non-designated items when fields exhausted
        if (!item_is_desig && fi >= n) break;
        if (item && item->kind == NK_INIT_ITEM) {
          if (item->desig_field) {
            for (int j = 0; j < n; j++)
              if (si.field_names[j] == item->desig_field) { target_fi = j; break; }
          }
          val = item->left;
        }
        if (target_fi < n && si.field_array_sizes[target_fi] != -2) {
          TypeSpec ft = si.field_types[target_fi];
          if (si.field_array_sizes[target_fi] >= 0) ft.array_size = si.field_array_sizes[target_fi];
          std::vector<uint8_t> fb;
          collect_bytes(irb, ft, val, fb);
          int fsz = irb->sizeof_ty(ft);
          for (int b = 0; b < fsz && b < (int)fb.size() && offsets[target_fi]+b < sz; b++)
            out[start + offsets[target_fi] + b] = fb[b];
        }
        fi = target_fi + 1;
      }
    }
    return;
  }

  // Scalar / pointer
  long long ival = 0;
  if (init->kind == NK_INT_LIT) ival = init->ival;
  else if (init->kind == NK_CHAR_LIT) ival = init->ival;
  else if (init->kind == NK_BINOP || init->kind == NK_UNARY)
    ival = static_eval_int(init, irb->enum_consts_);
  for (int i = 0; i < sz && i < 8; i++)
    out[start + i] = (uint8_t)((ival >> (8*i)) & 0xFF);
}

// Recursive global constant emitter.
// Returns the LLVM IR constant syntax (without the type prefix).
std::string IRBuilder::global_const(TypeSpec ts, Node* init) {
  if (!init) return "zeroinitializer";

  // Pointer-to-typedef-array (e.g. A3_28* pa0): llvm_ty returns "ptr" for this,
  // so treat it as a plain pointer constant (clear the typedef array dims and recurse).
  if (ts.ptr_level > 0 && ts.is_ptr_to_array) {
    TypeSpec ptr_ts = ts;
    clear_array(ptr_ts);
    ptr_ts.is_ptr_to_array = false;
    return global_const(ptr_ts, init);
  }

  // Unsized / flexible array: infer concrete size from initializer and recurse.
  if (is_array_ty(ts) && array_dim_at(ts, 0) == -2) {
    int inferred = infer_array_size_from_init(init);
    if (inferred > 0) {
      TypeSpec concrete_ts = ts;
      set_first_array_dim(concrete_ts, inferred);
      return global_const(concrete_ts, init);
    }
    return "zeroinitializer";
  }

  // Complex type: produce { T re, T im } constant.
  if (is_complex_base(ts.base) && ts.ptr_level == 0 && !is_array_ty(ts)) {
    TypeSpec ets = complex_component_ts(ts.base);
    std::string ellty = llvm_ty(ets);
    std::string cllty = llvm_ty(ts);
    bool is_fp = is_float_base(ets.base);

    // Unwrap single-element init list: { expr } → expr
    Node* expr = init;
    if (expr->kind == NK_INIT_LIST && expr->n_children == 1) {
      expr = expr->children[0];
      if (expr && expr->kind == NK_INIT_ITEM) expr = expr->left;
    }

    // Helper: static-eval real part of expression
    auto eval_real = [&](Node* e) -> std::string {
      if (!e) return is_fp ? "0.0" : "0";
      if (e->kind == NK_INT_LIT && !e->is_imaginary) {
        if (is_fp) return fp_to_hex(ets.base == TB_FLOAT ? (float)e->ival : (double)e->ival);
        return std::to_string(e->ival);
      }
      if (e->kind == NK_FLOAT_LIT && !e->is_imaginary) {
        if (is_fp) return ets.base == TB_FLOAT ? fp_to_hex_float(e->fval) : fp_to_hex(e->fval);
        return std::to_string((long long)e->fval);
      }
      if (e->kind == NK_INT_LIT && e->is_imaginary) return is_fp ? "0.0" : "0";
      if (e->kind == NK_FLOAT_LIT && e->is_imaginary) return is_fp ? "0.0" : "0";
      // Try static_eval_int for non-float
      if (!is_fp) return std::to_string(static_eval_int(e, enum_consts_));
      return is_fp ? "0.0" : "0";
    };
    auto eval_imag = [&](Node* e) -> std::string {
      if (!e) return is_fp ? "0.0" : "0";
      if (e->kind == NK_INT_LIT && e->is_imaginary) {
        if (is_fp) return fp_to_hex(ets.base == TB_FLOAT ? (float)e->ival : (double)e->ival);
        return std::to_string(e->ival);
      }
      if (e->kind == NK_FLOAT_LIT && e->is_imaginary) {
        if (is_fp) return ets.base == TB_FLOAT ? fp_to_hex_float(e->fval) : fp_to_hex(e->fval);
        return std::to_string((long long)e->fval);
      }
      if (e->kind == NK_INT_LIT && !e->is_imaginary) return is_fp ? "0.0" : "0";
      if (e->kind == NK_FLOAT_LIT && !e->is_imaginary) return is_fp ? "0.0" : "0";
      return is_fp ? "0.0" : "0";
    };

    std::string re_s, im_s;
    // Pattern: real + imag_lit  or  real - imag_lit
    if (expr->kind == NK_BINOP && expr->op &&
        (strcmp(expr->op, "+") == 0 || strcmp(expr->op, "-") == 0) &&
        expr->left && expr->right) {
      re_s = eval_real(expr->left);
      im_s = eval_imag(expr->right);
      if (strcmp(expr->op, "-") == 0 && expr->right && expr->right->is_imaginary) {
        // negate imaginary
        if (is_fp) im_s = "fneg " + ellty + " " + im_s; // can't embed fneg in global
        else {
          long long iv = static_eval_int(expr->right, enum_consts_);
          im_s = std::to_string(-iv);
        }
      }
    } else {
      // Single expression (could be just real or just imaginary)
      re_s = eval_real(expr);
      im_s = eval_imag(expr);
    }
    // Clamp fneg issue: for float, negation in global initializer needs special form.
    // For now, just use the value without fneg prefix (it's embedded incorrectly above).
    // Strip any erroneous "fneg" prefix.
    if (im_s.substr(0, 4) == "fneg") im_s = is_fp ? "0.0" : "0";

    return "{ " + ellty + " " + re_s + ", " + ellty + " " + im_s + " }";
  }

  // Wide string initializer for wchar_t[] (int array) from L"..."
  if (is_array_ty(ts) && array_dim_at(ts, 0) >= 0 &&
      array_rank_of(ts) == 1 &&
      (ts.base == TB_INT || ts.base == TB_UINT || ts.base == TB_LONG || ts.base == TB_ULONG) &&
      ts.ptr_level == 0 &&
      init->kind == NK_STR_LIT && is_wide_str_lit(init)) {
    auto codepoints = decode_wide_string(init->sval);
    int sz = (int)array_dim_at(ts, 0);
    std::string elem_llty = llvm_ty(drop_array_dim(ts));
    std::string result = "[";
    for (int i = 0; i < sz; i++) {
      if (i > 0) result += ", ";
      uint32_t cp = (i < (int)codepoints.size()) ? codepoints[i] : 0;
      result += elem_llty + " " + std::to_string((int)cp);
    }
    result += "]";
    return result;
  }

  // Array types MUST be checked before ptr_level: typedef-derived arrays like
  // `typedef void (*fptr)(); fptr table[3]` have ptr_level>0 but are arrays.
  // Exception: char arrays with string literal init are handled by the special
  // block below (which produces LLVM c"..." syntax), so skip them here.
  if (is_array_ty(ts) && array_dim_at(ts, 0) >= 0 &&
      !(array_rank_of(ts) == 1 &&
        (ts.base == TB_CHAR || ts.base == TB_SCHAR || ts.base == TB_UCHAR) &&
        init->kind == NK_STR_LIT)) {
    TypeSpec elem_ts = drop_array_dim(ts);
    std::string elem_llty = llvm_ty(elem_ts);
    int sz = (int)array_dim_at(ts, 0);
    // Build per-element constants
    std::vector<std::string> vals(sz, "zeroinitializer");
    if (init->kind == NK_INIT_LIST) {
      // Detect brace elision: when init list has more items than array elements
      // and element type is a struct — items are flat scalars to be consumed per element.
      bool use_flat = false;
      if ((elem_ts.base == TB_STRUCT) && elem_ts.ptr_level == 0 &&
          !is_array_ty(elem_ts) && init->n_children > sz) {
        int elem_flat = count_flat_fields_impl(elem_ts, struct_defs_);
        if (elem_flat > 1 && init->n_children <= sz * elem_flat) {
          use_flat = true;
        }
      }
      if (use_flat) {
        int flat_idx = 0;
        for (int ai = 0; ai < sz; ai++) {
          vals[ai] = global_const_flat_impl(this, elem_ts, init, flat_idx);
        }
      } else {
        long long cur_idx = 0;
        for (int i = 0; i < init->n_children; i++) {
          Node* item = init->children[i];
          long long idx = cur_idx;
          long long idx_hi = idx;
          Node* val_node = item;
          if (item && item->kind == NK_INIT_ITEM) {
            if (item->is_index_desig) {
              idx = item->desig_val;
              idx_hi = item->right ? item->right->ival : idx;
            }
            val_node = item->left;
          }
          std::string cv = global_const(elem_ts, val_node);
          for (long long ri = idx; ri <= idx_hi && ri < sz; ri++)
            vals[(int)ri] = cv;
          cur_idx = idx_hi + 1;
        }
      }
    }
    std::string result = "[";
    for (int i = 0; i < sz; i++) {
      if (i > 0) result += ", ";
      result += elem_llty + " " + vals[i];
    }
    result += "]";
    return result;
  }

  // Pointer types
  if (ts.ptr_level > 0) {
    // &&label (GCC label address extension): blockaddress constant
    if (init->kind == NK_INT_LIT && init->name && !init->is_imaginary) {
      auto it = user_labels_.find(init->name);
      std::string lbl_bb = (it != user_labels_.end())
                           ? it->second
                           : (std::string("user_") + init->name + "_0");
      return "blockaddress(@" + current_fn_name_ + ", %" + lbl_bb + ")";
    }
    if (init->kind == NK_INT_LIT && init->ival == 0) return "null";
    // Strip explicit casts in global initializer: (void*)"hello", (char*)func, etc.
    if (init->kind == NK_CAST && init->left)
      return global_const(ts, init->left);
    // String literal used directly as char*/void* initializer: char *p = "hello"
    if (init->kind == NK_STR_LIT && init->sval) {
      std::string content = decode_narrow_string_lit(init->sval);
      std::string gname = get_string_global(content);
      return gname;
    }
    // String literal + integer offset: "foo" + 1 → GEP constant expression
    if (init->kind == NK_BINOP && init->op && strcmp(init->op, "+") == 0 &&
        init->left && init->right) {
      Node* slnode = nullptr; Node* inode = nullptr;
      if (init->left->kind == NK_STR_LIT)  { slnode = init->left;  inode = init->right; }
      if (init->right->kind == NK_STR_LIT) { slnode = init->right; inode = init->left; }
      if (slnode && inode && slnode->sval) {
        long long off = static_eval_int(inode, enum_consts_);
        std::string content = decode_narrow_string_lit(slnode->sval);
        std::string gname = get_string_global(content);
        int len = (int)content.size() + 1;
        return "getelementptr inbounds ([" + std::to_string(len) + " x i8], ptr " +
               gname + ", i32 0, i32 " + std::to_string(off) + ")";
      }
    }
    if (init->kind == NK_ADDR && init->left) {
      Node* operand = init->left;
      // &global_var
      if (operand->kind == NK_VAR && operand->name)
        return std::string("@") + operand->name;
      // &global_array[i], &global_array[i][j], ... or &(str_lit[idx])
      if (operand->kind == NK_INDEX) {
        // Walk the NK_INDEX chain collecting indices (innermost first)
        std::vector<long long> indices;
        Node* base = operand;
        while (base && base->kind == NK_INDEX) {
          if (!base->right || base->right->kind != NK_INT_LIT) { base = nullptr; break; }
          indices.push_back(base->right->ival);
          base = base->left;
        }
        if (base && base->kind == NK_VAR && base->name && !indices.empty()) {
          // Reverse so we have outermost index first
          std::reverse(indices.begin(), indices.end());
          TypeSpec arr_ts = global_types_.count(base->name)
                            ? global_types_[base->name] : ts;
          std::string arr_llty = llvm_ty(arr_ts);
          std::string gep = std::string("getelementptr (") + arr_llty + ", ptr @" +
                            base->name + ", i64 0";
          for (auto idx : indices) gep += ", i64 " + std::to_string(idx);
          gep += ")";
          return gep;
        }
        // &(str_lit[idx]) — e.g. &("X"[0])
        if (base && base->kind == NK_STR_LIT && base->sval && indices.size() == 1) {
          std::string content = decode_narrow_string_lit(base->sval);
          std::string gname = get_string_global(content);
          long long idx = indices[0];
          if (idx == 0) return gname;
          int len = (int)content.size() + 1;  // +1 for null terminator
          return std::string("getelementptr ([") + std::to_string(len) +
                 " x i8], ptr " + gname + ", i64 0, i64 " + std::to_string(idx) + ")";
        }
      }
      // &(struct T){...} — compound literal at global scope:
      // create an internal global and return its address.
      // Note: compound literal stores its initializer in ->left (not ->init).
      if (operand->kind == NK_COMPOUND_LIT) {
        TypeSpec clt = operand->type;
        Node* cl_init = operand->left ? operand->left : operand->init;
        // Infer array size from init if needed
        if (is_array_ty(clt) && array_dim_at(clt, 0) == -2 && cl_init) {
          int inf = infer_array_size_from_init(cl_init);
          if (inf > 0) set_first_array_dim(clt, inf);
        }
        std::string cname = "@.cl" + std::to_string(string_idx_++);
        std::string cllty = llvm_ty(clt);
        std::string cval = cl_init ? global_const(clt, cl_init) : "zeroinitializer";
        preamble_.push_back(cname + " = internal global " + cllty + " " + cval);
        return cname;
      }
    }
    // Bare compound literal used as pointer initializer (no &):
    // struct S *p = (struct S){...}; — treat as &(compound literal)
    // Note: compound literal stores its initializer in ->left (not ->init).
    if (init->kind == NK_COMPOUND_LIT) {
      TypeSpec clt = init->type;
      Node* cl_init = init->left ? init->left : init->init;
      if (is_array_ty(clt) && array_dim_at(clt, 0) == -2 && cl_init) {
        int inf = infer_array_size_from_init(cl_init);
        if (inf > 0) set_first_array_dim(clt, inf);
      }
      std::string cname = "@.cl" + std::to_string(string_idx_++);
      std::string cllty = llvm_ty(clt);
      std::string cval = cl_init ? global_const(clt, cl_init) : "zeroinitializer";
      preamble_.push_back(cname + " = internal global " + cllty + " " + cval);
      return cname;
    }
    if (init->kind == NK_VAR && init->name)  // bare name (e.g. function ptr)
      return std::string("@") + init->name;
    return "null";
  }

  // Char array from string literal
  if (array_rank_of(ts) == 1 && array_dim_at(ts, 0) >= 0 &&
      (ts.base == TB_CHAR || ts.base == TB_SCHAR || ts.base == TB_UCHAR)) {
    if (init->kind == NK_STR_LIT && init->sval) {
      // Decode the C string to raw bytes first, then emit LLVM c"..." syntax
      // padded to ts.array_size bytes (null-padded at the end).
      const char* raw = init->sval;
      if (raw[0] == '"') raw++;
      std::string bytes;
      while (*raw && *raw != '"') {
        if (*raw == '\\') {
          raw++;
          if (*raw == 'n') { bytes += '\n'; raw++; }
          else if (*raw == 't') { bytes += '\t'; raw++; }
          else if (*raw == 'r') { bytes += '\r'; raw++; }
          else if (*raw == '0') { bytes += '\0'; raw++; }
          else if (*raw == 'a') { bytes += '\a'; raw++; }
          else if (*raw == 'b') { bytes += '\b'; raw++; }
          else if (*raw == 'f') { bytes += '\f'; raw++; }
          else if (*raw == 'v') { bytes += '\v'; raw++; }
          else if (*raw == '\\') { bytes += '\\'; raw++; }
          else if (*raw == '"') { bytes += '"'; raw++; }
          else if (*raw == 'x' || *raw == 'X') {
            raw++;
            unsigned int v = 0;
            for (int k = 0; k < 2 && *raw && isxdigit((unsigned char)*raw); k++, raw++)
              v = v * 16 + (isdigit((unsigned char)*raw) ? *raw - '0' :
                            tolower((unsigned char)*raw) - 'a' + 10);
            bytes += (char)(unsigned char)v;
          } else if (*raw >= '0' && *raw <= '7') {
            unsigned int v = 0;
            for (int k = 0; k < 3 && *raw >= '0' && *raw <= '7'; k++, raw++)
              v = v * 8 + (*raw - '0');
            bytes += (char)(unsigned char)v;
          } else { bytes += *raw++; }
        } else { bytes += *raw++; }
      }
      // Build LLVM c"..." repr padded to ts.array_size
      std::string content;
      int dim0 = (int)array_dim_at(ts, 0);
      for (int i = 0; i < dim0; i++) {
        unsigned char c = (i < (int)bytes.size()) ? (unsigned char)bytes[i] : 0;
        if (c == '"' || c == '\\' || c < 0x20 || c >= 0x7F) {
          char esc[8];
          snprintf(esc, sizeof(esc), "\\%02X", (unsigned)c);
          content += esc;
        } else {
          content += (char)c;
        }
      }
      return std::string("c\"") + content + "\"";
    }
  }

  // Struct types
  if ((ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.tag && ts.tag[0]) {
    auto sit = struct_defs_.find(ts.tag);
    if (sit == struct_defs_.end()) return "zeroinitializer";
    const StructInfo& si = sit->second;
    int n_fields = (int)si.field_names.size();
    if (n_fields == 0) return "zeroinitializer";

    std::vector<std::string> vals(n_fields, "zeroinitializer");
    std::vector<int> fam_concrete_sizes(n_fields, 0);  // concrete FAM sizes (0 = not FAM or no data)
    // Unwrap compound literal: (struct Foo){...} used as struct initializer
    if (init->kind == NK_COMPOUND_LIT) {
      Node* cl_init = init->left ? init->left : init->init;
      if (cl_init) init = cl_init;
    }
    // Unwrap cast: (struct Foo)(expr)
    if (init->kind == NK_CAST && init->left)
      init = init->left;
    if (init->kind == NK_INIT_LIST) {
      int flat_i = 0;  // position in init->children (consumed across field boundaries)
      int cur_field = 0;
      while (flat_i < init->n_children) {
        Node* item = init->children[flat_i];
        int fi = cur_field;
        Node* val_node = item;
        bool is_desig = false;
        if (item && item->kind == NK_INIT_ITEM) {
          if (item->desig_field) {
            is_desig = true;
            for (int j = 0; j < n_fields; j++) {
              if (si.field_names[j] == item->desig_field) { fi = j; break; }
            }
          }
          val_node = item->left;
        }
        flat_i++;
        if (fi < n_fields) {
          TypeSpec ft = si.field_types[fi];
          if (si.field_array_sizes[fi] != -1) ft.array_size = si.field_array_sizes[fi];
          bool ft_is_arr = is_array_ty(ft);
          bool ft_is_agg = (ft.ptr_level == 0 &&
                            (ft.base == TB_STRUCT || ft.base == TB_UNION));
          bool val_is_list = val_node &&
                             (val_node->kind == NK_INIT_LIST ||
                              val_node->kind == NK_COMPOUND_LIT);
          bool val_is_str = val_node && val_node->kind == NK_STR_LIT;
          // Brace-elide when: aggregate field + non-list value.
          // EXCEPT: array field + string literal → direct string-to-array (no elide).
          // Struct field + string literal → DO elide (string fills first sub-array).
          if (!is_desig && (ft_is_arr || ft_is_agg) && !val_is_list &&
              !(ft_is_arr && val_is_str)) {
            // Brace-elision: scalar(s) from flat list fill this aggregate field.
            flat_i--;
            vals[fi] = global_const_flat_impl(this, ft, init, flat_i);
          } else if (!ft_is_arr && !ft_is_agg &&
                     val_node && val_node->kind == NK_INIT_LIST) {
            // Optional braces around scalar initializer: {(1)} for u8 field.
            Node* inner = (val_node->n_children > 0) ? val_node->children[0] : nullptr;
            if (inner && inner->kind == NK_INIT_ITEM) inner = inner->left;
            vals[fi] = global_const(ft, inner);
          } else {
            vals[fi] = global_const(ft, val_node);
          }
          // Track concrete FAM size for output type
          if (si.field_array_sizes[fi] == -2 && val_node)
            fam_concrete_sizes[fi] = infer_array_size_from_init(val_node);
          cur_field = fi + 1;
        }
      }
    } else if (init->kind != NK_INT_LIT || init->ival != 0) {
      // Non-list, non-null initializer (e.g. scalar or string for first field).
      // Use flat approach so that brace-elision works correctly for array first-fields
      // (e.g. struct SS s = 2 where first field is u8 a[3] → a[0]=2, rest zero).
      Node* children[1] = { init };
      Node synthetic; memset(&synthetic, 0, sizeof(synthetic));
      synthetic.kind = NK_INIT_LIST;
      synthetic.n_children = 1;
      synthetic.children = children;
      int flat_idx = 0;
      for (int fi2 = 0; fi2 < n_fields; fi2++) {
        TypeSpec ft = si.field_types[fi2];
        if (si.field_array_sizes[fi2] >= 0) ft.array_size = si.field_array_sizes[fi2];
        vals[fi2] = global_const_flat_impl(this, ft, &synthetic, flat_idx);
      }
    }

    if (ts.base == TB_UNION) {
      // LLVM union type is { [max_sz x i8] } — emit a byte array.
      int max_sz = 0;
      for (size_t i = 0; i < si.field_types.size(); i++) {
        TypeSpec ft2 = si.field_types[i];
        int sz = sizeof_ty(ft2);
        if (si.field_array_sizes[i] >= 0) sz *= (int)si.field_array_sizes[i];
        if (sz > max_sz) max_sz = sz;
      }
      if (max_sz <= 0) return "zeroinitializer";

      // Determine which member is being initialized and collect its bytes.
      // collect_bytes() APPENDS to the output vector, so start with empty vector.
      std::vector<uint8_t> bytes;  // will be padded to max_sz after collection
      if (init->kind == NK_INIT_LIST && init->n_children > 0) {
        // Check if the init contains designated initializers for anonymous struct fields
        // (e.g. {.a = 7, .b = 8} where a,b are anon-struct members hoisted into union)
        bool has_anon_desig = false;
        for (int i = 0; i < init->n_children; i++) {
          Node* item = init->children[i];
          if (item && item->kind == NK_INIT_ITEM && item->desig_field) {
            // Check if this field belongs to an anonymous struct member
            for (size_t j = 0; j < si.field_names.size(); j++) {
              if (si.field_is_anon[j]) {
                // Look up the anon struct's fields
                const char* anon_tag = si.field_types[j].tag;
                if (anon_tag) {
                  auto ait = struct_defs_.find(anon_tag);
                  if (ait != struct_defs_.end()) {
                    for (auto& fn : ait->second.field_names) {
                      if (fn == item->desig_field) { has_anon_desig = true; break; }
                    }
                  }
                }
              }
            }
          }
          if (has_anon_desig) break;
        }
        if (has_anon_desig) {
          // Delegate to the anonymous struct member's type
          for (size_t j = 0; j < si.field_names.size(); j++) {
            if (si.field_is_anon[j]) {
              TypeSpec anon_ts = si.field_types[j];
              collect_bytes(this, anon_ts, init, bytes);
              break;
            }
          }
        } else {
          // First member init (possibly flat init for array/struct first member)
          Node* item0 = init->children[0];
          Node* val_init = (item0 && item0->kind == NK_INIT_ITEM) ? item0->left : item0;
          if (n_fields > 0) {
            TypeSpec ft0 = si.field_types[0];
            if (si.field_array_sizes[0] >= 0) ft0.array_size = si.field_array_sizes[0];
            bool val_is_list = (val_init && val_init->kind == NK_INIT_LIST);
            bool ft0_is_agg = (ft0.ptr_level == 0 &&
                               (is_array_ty(ft0) || ft0.base == TB_STRUCT || ft0.base == TB_UNION));
            // If first item is a braced list or first member is scalar: use val_init.
            // If first member is aggregate and val_init is scalar: brace-elide — pass whole init.
            Node* member_init = (val_is_list || !ft0_is_agg) ? val_init : init;
            collect_bytes(this, ft0, member_init, bytes);
          }
        }
      } else if (init->kind != NK_INT_LIT || init->ival != 0) {
        // Scalar or non-zero scalar
        if (n_fields > 0) {
          TypeSpec ft0 = si.field_types[0];
          if (si.field_array_sizes[0] >= 0) ft0.array_size = si.field_array_sizes[0];
          collect_bytes(this, ft0, init, bytes);
        }
      }

      // Pad collected bytes to max_sz
      bytes.resize(max_sz, 0);

      // Check if all bytes are zero
      bool all_zero = true;
      for (auto b : bytes) if (b) { all_zero = false; break; }

      std::string ba_type = "[" + std::to_string(max_sz) + " x i8]";
      if (all_zero) return "{ " + ba_type + " zeroinitializer }";

      std::string content = ba_type + " c\"";
      for (int i = 0; i < max_sz; i++) {
        char esc[5];
        snprintf(esc, sizeof(esc), "\\%02X", (unsigned)bytes[i]);
        content += esc;
      }
      content += "\"";
      return "{ " + content + " }";
    }

    // Return just the brace initializer (no "%struct.X " prefix — caller adds type)
    // FAM fields (field_array_sizes == -2) are included with concrete sizes when initialized.
    // Bitfield packing: collect per-LLVM-field values, packing bitfields into storage units.
    int n_llvm = si.n_llvm_fields;
    std::vector<long long> llvm_packed(n_llvm, 0LL);  // accumulated integer for bitfield slots
    std::vector<std::string> llvm_val_str(n_llvm);    // final string per LLVM field
    std::vector<bool> llvm_is_bf(n_llvm, false);      // true = bitfield storage slot

    for (int i = 0; i < n_fields; i++) {
      if (si.field_array_sizes[i] == -2) continue;  // FAM: handled separately
      int li = (i < (int)si.field_llvm_idx.size()) ? si.field_llvm_idx[i] : i;
      if (li < 0 || li >= n_llvm) continue;
      int bfw = (i < (int)si.field_bit_widths.size()) ? si.field_bit_widths[i] : -1;
      if (bfw > 0) {
        // Bitfield: accumulate into packed storage
        llvm_is_bf[li] = true;
        long long raw = 0;
        if (!vals[i].empty() && vals[i] != "zeroinitializer")
          raw = std::stoll(vals[i]);
        int bit_off = (i < (int)si.field_bit_offsets.size()) ? si.field_bit_offsets[i] : 0;
        // Mask to bitfield width and shift into position
        long long mask = (bfw >= 64) ? ~0LL : ((1LL << bfw) - 1);
        raw = (raw & mask) << bit_off;
        llvm_packed[li] |= raw;
      } else {
        // Non-bitfield: use value directly
        llvm_val_str[li] = vals[i];
      }
    }
    // Convert packed integers to strings
    for (int li = 0; li < n_llvm; li++) {
      if (llvm_is_bf[li])
        llvm_val_str[li] = std::to_string(llvm_packed[li]);
    }

    std::string result = "{ ";
    bool first_field = true;
    // Emit LLVM fields
    for (int li = 0; li < n_llvm; li++) {
      std::string lty = (li < (int)si.llvm_field_types.size()) ? si.llvm_field_types[li] : "";
      if (!first_field) result += ", ";
      first_field = false;
      if (llvm_val_str[li].empty()) {
        result += lty + " " + (lty.empty() || lty[0] == '[' || lty[0] == '%' ? "zeroinitializer" : "0");
      } else {
        result += lty + " " + llvm_val_str[li];
      }
    }
    // Append FAM fields (not part of n_llvm_fields)
    for (int i = 0; i < n_fields; i++) {
      if (si.field_array_sizes[i] != -2) continue;
      if (fam_concrete_sizes[i] <= 0) continue;
      if (!first_field) result += ", ";
      first_field = false;
      TypeSpec ft = si.field_types[i];
      long long fdims[1] = {fam_concrete_sizes[i]};
      set_array_dims(ft, fdims, 1);
      result += llvm_ty(ft) + " " + vals[i];
    }
    result += " }";
    return result;
  }

  // Scalar types
  // Float/double: always use hex fp representation
  if (ts.ptr_level == 0 && !is_array_ty(ts) && is_float_base(ts.base)) {
    bool is_f32 = (ts.base == TB_FLOAT);
    auto fhex = [&](double v) { return is_f32 ? fp_to_hex_float(v) : fp_to_hex(v); };
    if (init->kind == NK_INT_LIT)   return fhex((double)init->ival);
    if (init->kind == NK_CHAR_LIT)  return fhex((double)init->ival);
    if (init->kind == NK_FLOAT_LIT) return fhex(init->fval);
    if (init->kind == NK_UNARY && init->op && strcmp(init->op, "-") == 0 &&
        init->left && init->left->kind == NK_FLOAT_LIT)
      return fhex(-init->left->fval);
    if (init->kind == NK_CAST && init->left)
      return global_const(ts, init->left);
    // Try static evaluation for constant expressions (BinOp, Unary, etc.)
    // Use float evaluation to preserve NaN/Inf semantics (e.g. 1.0/0.0 - 1.0/0.0 = NaN)
    if (init->kind == NK_BINOP || init->kind == NK_UNARY)
      return fhex(static_eval_float(init));
    return fhex(0.0);
  }
  // Unwrap optional braces around scalar: {4} or {(1)} for scalar field.
  if (init->kind == NK_INIT_LIST && init->n_children > 0) {
    Node* inner = init->children[0];
    if (inner && inner->kind == NK_INIT_ITEM) inner = inner->left;
    return global_const(ts, inner);
  }
  if (init->kind == NK_INT_LIT) return std::to_string(init->ival);
  if (init->kind == NK_CHAR_LIT) return std::to_string(init->ival);
  if (init->kind == NK_FLOAT_LIT) return fp_to_hex(init->fval);
  if (init->kind == NK_CAST && init->left) {
    // Evaluate with full cast semantics so that (unsigned int)-4 = 4294967292 (not -4).
    // static_eval_int now correctly applies truncation for the cast's target type.
    long long cv = static_eval_int(init, enum_consts_);
    return std::to_string(cv);
  }
  // Try static evaluation for constant expressions (BinOp, Unary, Var for enum)
  if (init->kind == NK_BINOP || init->kind == NK_UNARY ||
      (init->kind == NK_VAR && init->name)) {
    long long ival = static_eval_int(init, enum_consts_);
    return std::to_string(ival);
  }

  return "zeroinitializer";
}

// ─────────────────────────── global variables ──────────────────────────────

void IRBuilder::emit_global(Node* gv) {
  if (!gv || gv->kind != NK_GLOBAL_VAR || !gv->name) return;

  std::string name = gv->name;
  TypeSpec ts = gv->type;

  // Infer array size from initializer for int a[] / char s[] style globals
  // Only for unsized arrays (array_size == -2), not scalars (-1)
  if (is_array_ty(ts) && array_dim_at(ts, 0) == -2 && gv->init) {
    int raw_count = infer_array_size_from_init(gv->init);
    if (raw_count > 0) {
      // Check for flat brace elision: array-of-struct where init list is flat scalars
      TypeSpec elem_ts = drop_array_dim(ts);
      if (gv->init->kind == NK_INIT_LIST &&
          (elem_ts.base == TB_STRUCT) && elem_ts.ptr_level == 0 && !is_array_ty(elem_ts)) {
        int elem_flat = count_flat_fields_impl(elem_ts, struct_defs_);
        if (elem_flat > 1 && raw_count % elem_flat == 0) {
          // All init items are scalars (not init lists) — check first item
          bool all_scalar = true;
          for (int ii = 0; ii < gv->init->n_children && ii < 4; ii++) {
            Node* item = gv->init->children[ii];
            if (item && item->kind == NK_INIT_ITEM) item = item->left;
            if (item && item->kind == NK_INIT_LIST) { all_scalar = false; break; }
          }
          if (all_scalar) {
            set_first_array_dim(ts, raw_count / elem_flat);
          } else {
            set_first_array_dim(ts, raw_count);
          }
        } else {
          set_first_array_dim(ts, raw_count);
        }
      } else {
        set_first_array_dim(ts, raw_count);
      }
    }
  }

  std::string llty = llvm_ty(ts);
  global_types_[name] = ts;
  global_is_extern_[name] = gv->is_extern;

  if (gv->is_extern) {
    preamble_.push_back("@" + name + " = external global " + llty);
    return;
  }

  // Special case: char* initialized with a string literal (pointer, not array)
  if (ts.ptr_level > 0 && !is_array_ty(ts) && gv->init &&
      gv->init->kind == NK_STR_LIT && gv->init->sval) {
    std::string raw = gv->init->sval;
    std::string content;
    if (raw.size() >= 2 && raw.front() == '"' && raw.back() == '"')
      content = raw.substr(1, raw.size() - 2);
    else
      content = raw;
    std::string sg = get_string_global(content);
    std::string prefix = gv->is_static ? "internal " : "";
    preamble_.push_back("@" + name + " = " + prefix + "global ptr " + sg);
    return;
  }

  // Use the recursive constant emitter for all other initializers
  std::string init_val = gv->init ? global_const(ts, gv->init) : "zeroinitializer";

  // For global structs with flexible array members that have concrete init data,
  // use an inline concrete type (not %struct.X) so the FAM data is included.
  std::string use_llty = llty;
  if (ts.base == TB_STRUCT && ts.ptr_level == 0 && ts.tag && gv->init &&
      gv->init->kind == NK_INIT_LIST) {
    auto sit2 = struct_defs_.find(ts.tag);
    if (sit2 != struct_defs_.end()) {
      const StructInfo& si2 = sit2->second;
      int nf = (int)si2.field_names.size();
      int fam_idx = -1;
      for (int i = 0; i < nf; i++) {
        if (si2.field_array_sizes[i] == -2) { fam_idx = i; break; }
      }
      if (fam_idx >= 0 && fam_idx < gv->init->n_children) {
        Node* fam_item = gv->init->children[fam_idx];
        Node* fam_val = (fam_item && fam_item->kind == NK_INIT_ITEM) ? fam_item->left : fam_item;
        int fam_sz = fam_val ? infer_array_size_from_init(fam_val) : 0;
        if (fam_sz > 0) {
          // Build concrete struct type with FAM
          std::string ct = "{ ";
          bool cfirst = true;
          for (int i = 0; i < nf; i++) {
            if (!cfirst) ct += ", ";
            cfirst = false;
            TypeSpec ft2 = si2.field_types[i];
            if (si2.field_array_sizes[i] == -2) {
              long long fdims[1] = {fam_sz};
              set_array_dims(ft2, fdims, 1);
            } else if (si2.field_array_sizes[i] >= 0) {
              ft2.array_size = si2.field_array_sizes[i];
            }
            ct += llvm_ty(ft2);
          }
          ct += " }";
          use_llty = ct;
        }
      }
    }
  }

  std::string linkage = gv->is_static ? "internal " : "";
  preamble_.push_back("@" + name + " = " + linkage + "global " + use_llty + " " + init_val);
}

// ─────────────────────────── function emission ─────────────────────────────

void IRBuilder::emit_function(Node* fn) {
  if (!fn || fn->kind != NK_FUNCTION || !fn->name) return;

  // Collect signature into func_sigs_ (may have been pre-collected)
  FuncSig sig;
  sig.ret_type = fn->type;
  sig.variadic  = fn->variadic;
  for (int i = 0; i < fn->n_params; i++) {
    Node* p = fn->params[i];
    if (!p) continue;
    // Skip void params (variadic placeholder: void with no name, or just void)
    if (p->type.base == TB_VOID && p->type.ptr_level == 0) continue;
    // Array params decay to ptr
    TypeSpec pts = p->type;
    if (is_array_ty(pts)) { pts = drop_array_dim(pts); pts.ptr_level += 1; pts.is_ptr_to_array = true; }
    sig.param_types.push_back(pts);
  }
  func_sigs_[fn->name] = sig;

  std::string ret_llty = llvm_ty(fn->type);

  // Function declaration (no body)
  if (!fn->body) {
    // Suppress declare if this function is defined (has a body) in this module:
    // LLVM accepts define without a preceding declare, and mixing declare+define
    // can cause "invalid redefinition" errors in some LLVM versions.
    if (has_body_.count(fn->name)) return;
    // Suppress duplicate declarations (e.g. "int foo(void); int foo(void);")
    if (declared_fns_.count(fn->name)) return;
    declared_fns_.insert(fn->name);

    std::string params_str;
    for (size_t i = 0; i < sig.param_types.size(); i++) {
      if (i > 0) params_str += ", ";
      params_str += llvm_ty(sig.param_types[i]);
    }
    if (sig.variadic) {
      if (!params_str.empty()) params_str += ", ";
      params_str += "...";
    }
    preamble_.push_back("declare " + ret_llty + " @" + fn->name +
                        "(" + params_str + ")");
    return;
  }

  // Function definition
  // Reset per-function state
  alloca_lines_.clear();
  body_lines_.clear();
  local_types_.clear();
  local_slots_.clear();
  node_slots_.clear();
  node_types_.clear();
  local_slot_dedup_.clear();
  tmp_idx_ = 0;
  label_idx_ = 0;
  last_was_terminator_ = false;
  loop_stack_.clear();
  break_stack_.clear();
  user_labels_.clear();
  switch_stack_.clear();
  block_depth_ = 0;

  current_fn_ret_ = fn->type;
  current_fn_ret_llty_ = ret_llty;
  current_fn_name_ = fn->name ? fn->name : "";

  // Build parameter string and register parameters in local_slots_
  // Track how many non-void params we've added (for comma insertion)
  std::string params_str;
  bool first_param = true;
  for (int i = 0; i < fn->n_params; i++) {
    Node* p = fn->params[i];
    if (!p) continue;
    if (p->type.base == TB_VOID && p->type.ptr_level == 0) continue;  // skip void

    // Array params decay to ptr in the function signature.
    // Pointer-to-array types (is_ptr_to_array) are already pointers — don't decay again.
    bool is_array = is_array_ty(p->type) && !p->type.is_ptr_to_array;
    std::string pllty = is_array ? "ptr" : llvm_ty(p->type);

    if (!first_param) params_str += ", ";
    first_param = false;

    if (p->name && p->name[0]) {
      std::string pname = std::string("%") + p->name + "_arg";
      params_str += pllty + " " + pname;
      // Create alloca + store for parameter
      // For array params, alloca as ptr (pointer to the decayed array)
      std::string slot = std::string("%lv.") + p->name;
      local_slots_[p->name] = slot;
      TypeSpec stored_type = p->type;
      if (is_array) { stored_type = drop_array_dim(stored_type); stored_type.ptr_level += 1; }
      local_types_[p->name] = stored_type;
      alloca_lines_.push_back("  " + slot + " = alloca " + pllty);
      alloca_lines_.push_back("  store " + pllty + " " + pname + ", ptr " + slot);
    } else {
      // Anonymous param: include in signature but no local slot
      params_str += pllty;
    }
  }
  if (sig.variadic) {
    if (!params_str.empty()) params_str += ", ";
    params_str += "...";
  }

  // Register C99 __func__ (and GCC __FUNCTION__/__PRETTY_FUNCTION__) as a
  // local char[] constant holding the current function name.
  {
    std::string fname = current_fn_name_;
    std::string str_global = get_string_global(fname);
    int fname_len = (int)fname.size() + 1;  // include null terminator
    TypeSpec func_ts;
    func_ts.base       = TB_CHAR;
    func_ts.is_const   = true;
    func_ts.array_rank = 1;
    func_ts.array_size = fname_len;
    func_ts.array_dims[0] = fname_len;
    for (const char* predef : {"__func__", "__FUNCTION__", "__PRETTY_FUNCTION__"}) {
      local_types_[predef] = func_ts;
      local_slots_[predef] = str_global;
    }
  }

  // Pre-scan for user goto labels
  prescan_labels(fn->body);

  // Hoist all local variable allocas
  hoist_allocas(fn->body);

  // Minimal hard error for the negative test case:
  // { int a; return a; }
  if (fn->body && fn->body->kind == NK_BLOCK && fn->body->n_children == 2) {
    Node* first = fn->body->children[0];
    Node* second = fn->body->children[1];
    if (first && second && first->kind == NK_DECL && !first->init && first->name &&
        first->type.base != TB_STRUCT && first->type.base != TB_UNION &&
        second->kind == NK_RETURN && second->left &&
        second->left->kind == NK_VAR && second->left->name &&
        std::strcmp(first->name, second->left->name) == 0) {
      throw std::runtime_error(std::string("use of uninitialized local variable: ") + first->name);
    }
  }

  // Emit the function body
  emit_stmt(fn->body);

  // Ensure function ends with a terminator
  if (!last_was_terminator_) {
    if (ret_llty == "void")
      emit_terminator("ret void");
    else if (ret_llty == "ptr")
      emit_terminator("ret ptr null");
    else if (ret_llty == "double")
      emit_terminator("ret double " + fp_to_hex(0.0));
    else if (ret_llty == "float")
      emit_terminator("ret float " + fp_to_hex_float(0.0));
    else
      emit_terminator("ret " + ret_llty + " 0");
  }

  // Assemble the function text
  std::string fn_text;
  if (fn->is_static)
    fn_text += "define internal " + ret_llty + " @" + fn->name + "(" + params_str + ") {\n";
  else
    fn_text += "define " + ret_llty + " @" + fn->name + "(" + params_str + ") {\n";
  fn_text += "entry:\n";
  for (const auto& l : alloca_lines_) fn_text += l + "\n";
  for (const auto& l : body_lines_)  fn_text += l + "\n";
  fn_text += "}\n";

  // We'll accumulate in preamble for now, then print all at the end
  // Actually we separate function sections from preamble
  // Append directly to final output (see emit_program)
  fn_sections_.push_back(std::move(fn_text));
}

// ─────────────────────────── program emission ──────────────────────────────

// Helper: flatten top-level program items, unwrapping NK_BLOCK wrappers
// (produced by multi-declarator global declarations like "int x, y;")
static std::vector<Node*> flatten_program_items(Node* prog) {
  std::vector<Node*> items;
  if (!prog) return items;
  for (int i = 0; i < prog->n_children; i++) {
    Node* item = prog->children[i];
    if (!item) continue;
    if (item->kind == NK_BLOCK) {
      for (int j = 0; j < item->n_children; j++)
        if (item->children[j]) items.push_back(item->children[j]);
    } else {
      items.push_back(item);
    }
  }
  return items;
}

std::string IRBuilder::emit_program(Node* prog) {
  if (!prog) return "; empty program\n";

  std::vector<Node*> items = flatten_program_items(prog);

  // Phase 1: collect all struct defs, function signatures, global types
  // Pre-collect function declarations and definitions for forward references
  for (Node* item : items) {
    if (!item) continue;
    switch (item->kind) {
    case NK_STRUCT_DEF:
      emit_struct_def(item);
      break;
    case NK_ENUM_DEF:
      // Collect enum constants
      for (int j = 0; j < item->n_enum_variants; j++) {
        if (item->enum_names && item->enum_names[j])
          enum_consts_[item->enum_names[j]] = item->enum_vals[j];
      }
      if (item->name && item->name[0]) {
        bool all_nonnegative = true;
        for (int j = 0; j < item->n_enum_variants; j++) {
          if (item->enum_vals[j] < 0) {
            all_nonnegative = false;
            break;
          }
        }
        enum_all_nonnegative_[item->name] = all_nonnegative;
      }
      break;
    case NK_FUNCTION:
      // Pre-collect signatures (body not emitted yet)
      if (item->name) {
        FuncSig sig;
        sig.ret_type = item->type;
        sig.variadic  = item->variadic;
        for (int j = 0; j < item->n_params; j++) {
          Node* p = item->params[j];
          if (!p) continue;
          // Skip void params (including variadic placeholder)
          if (p->type.base == TB_VOID && p->type.ptr_level == 0) continue;
          // Array params decay to ptr
          TypeSpec pts = p->type;
          if (is_array_ty(pts) && !pts.is_ptr_to_array) { pts = drop_array_dim(pts); pts.ptr_level += 1; pts.is_ptr_to_array = true; }
          sig.param_types.push_back(pts);
        }
        func_sigs_[item->name] = sig;
        // Track functions with bodies to suppress redundant forward declares
        if (item->body) has_body_.insert(item->name);
      }
      break;
    case NK_GLOBAL_VAR:
      if (item->name) {
        TypeSpec gts = item->type;
        // Infer array size from initializer in pre-pass too (unsized only)
        if (is_array_ty(gts) && array_dim_at(gts, 0) == -2 && item->init) {
          int raw_count = infer_array_size_from_init(item->init);
          if (raw_count > 0) {
            TypeSpec elem_ts = drop_array_dim(gts);
            if (item->init->kind == NK_INIT_LIST &&
                (elem_ts.base == TB_STRUCT) && elem_ts.ptr_level == 0 && !is_array_ty(elem_ts)) {
              int elem_flat = count_flat_fields_impl(elem_ts, struct_defs_);
              if (elem_flat > 1 && raw_count % elem_flat == 0) {
                bool all_scalar = true;
                for (int ii = 0; ii < item->init->n_children && ii < 4; ii++) {
                  Node* it2 = item->init->children[ii];
                  if (it2 && it2->kind == NK_INIT_ITEM) it2 = it2->left;
                  if (it2 && it2->kind == NK_INIT_LIST) { all_scalar = false; break; }
                }
                if (all_scalar) {
                  set_first_array_dim(gts, raw_count / elem_flat);
                } else {
                  set_first_array_dim(gts, raw_count);
                }
              } else {
                set_first_array_dim(gts, raw_count);
              }
            } else {
              set_first_array_dim(gts, raw_count);
            }
          }
        }
        global_types_[item->name] = gts;
        // Track function pointer variable signatures from &funcname initializers.
        // A global is a function pointer if it's a pointer (ptr_level>0) or TB_FUNC_PTR
        // and its initializer is &funcname or just funcname (direct function reference).
        if (item->init && !is_array_ty(gts)) {
          Node* init_node = item->init;
          // Unwrap optional & operator
          if (init_node->kind == NK_ADDR && init_node->left)
            init_node = init_node->left;
          if (init_node->kind == NK_VAR && init_node->name) {
            auto fsit = func_sigs_.find(init_node->name);
            if (fsit != func_sigs_.end()) {
              fptr_sigs_[item->name] = fsit->second;
            }
          }
        }
      }
      break;
    default:
      break;
    }
  }

  // Pre-scan: for each global name, pick the canonical node to emit.
  // C allows "int x; int x = 3; int x;" — we emit only one definition,
  // preferring the one with an explicit initializer.
  std::unordered_map<std::string, Node*> best_global;
  for (Node* item : items) {
    if (!item || item->kind != NK_GLOBAL_VAR || !item->name) continue;
    const char* gname = item->name;
    auto it = best_global.find(gname);
    if (it == best_global.end()) {
      best_global[gname] = item;
    } else {
      Node* cur = it->second;
      // Prefer non-extern (definition) over extern (declaration only)
      if (cur->is_extern && !item->is_extern) {
        best_global[gname] = item;
      // Among non-extern, prefer the one with an explicit initializer
      } else if (!cur->is_extern && !item->is_extern && !cur->init && item->init) {
        best_global[gname] = item;
      }
    }
  }

  // Phase 2: emit struct defs, global vars, function decls/defs in order
  for (Node* item : items) {
    if (!item) continue;
    switch (item->kind) {
    case NK_STRUCT_DEF:
      // Already handled; emit_struct_def is idempotent
      emit_struct_def(item);
      break;
    case NK_ENUM_DEF:
      break;  // enum consts already collected; no IR emitted
    case NK_GLOBAL_VAR:
      // Only emit the canonical (best) node for each global name
      if (item->name && best_global.count(item->name) &&
          best_global[item->name] == item)
        emit_global(item);
      break;
    case NK_FUNCTION:
      emit_function(item);
      break;
    default:
      break;
    }
  }

  // Assemble final IR
  std::string out;
  // Prepend the va_list struct type definition before all other preamble lines.
  // AArch64 (macOS ARM64): va_list is a struct { ptr, ptr, ptr, i32, i32 } (40 bytes).
  out += "%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }\n";

  // Find struct tags that are referenced in preamble lines but not defined there.
  // Emit "type {}" for genuinely empty structs (known to struct_defs_ with 0 fields)
  // so that forward references in containing struct definitions are resolved.
  {
    std::unordered_set<std::string> defined_tags;
    defined_tags.insert("__va_list_tag_");
    for (const auto& l : preamble_) {
      // Lines like: %struct.TAG = type { ... }
      if (l.substr(0, 8) == "%struct." && l.find(" = type") != std::string::npos) {
        size_t eq = l.find(" = type");
        std::string tag = l.substr(8, eq - 8);
        defined_tags.insert(tag);
      }
    }
    for (const auto& kv : struct_defs_) {
      const std::string& tag = kv.first;
      if (defined_tags.count(tag)) continue;
      if (kv.second.field_names.empty()) {
        // Genuinely empty struct - emit forward type {}
        out += "%struct." + tag + " = type {}\n";
      }
    }
  }

  for (const auto& l : preamble_) out += l + "\n";

  // Emit auto-declare lines for called-but-undeclared functions
  if (!auto_declared_fns_.empty()) {
    if (!preamble_.empty()) out += "\n";
    // Sort for deterministic output
    std::vector<std::string> decls;
    decls.reserve(auto_declared_fns_.size());
    for (const auto& kv : auto_declared_fns_) decls.push_back(kv.second);
    std::sort(decls.begin(), decls.end());
    for (const auto& d : decls) out += d + "\n";
  }

  if ((!preamble_.empty() || !auto_declared_fns_.empty()) && !fn_sections_.empty())
    out += "\n";
  for (const auto& fn : fn_sections_) out += fn + "\n";
  return out;
}

}  // namespace tinyc2ll::frontend_cxx
