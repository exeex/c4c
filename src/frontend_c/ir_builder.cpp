#include "ir_builder.hpp"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <unordered_set>
#include <cstdint>
#include <cmath>
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
  return ts;
}

static bool is_unsigned_base(TypeBase b) {
  return b == TB_UCHAR || b == TB_USHORT || b == TB_UINT ||
         b == TB_ULONG || b == TB_ULONGLONG || b == TB_BOOL;
}

static bool is_float_base(TypeBase b) {
  return b == TB_FLOAT || b == TB_DOUBLE || b == TB_LONGDOUBLE;
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
      last_was_terminator_(false) {}

// ─────────────────────────── type helpers ──────────────────────────────────

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
  case TB_INT128:
  case TB_UINT128:    return "i128";
  case TB_ENUM:       return "i32";
  case TB_VA_LIST:    // va_list — as a ptr in declarations/params; alloca uses special struct
  case TB_FUNC_PTR:   return "ptr";
  case TB_TYPEDEF:    return "i32";
  default:            return "i32";
  }
}

std::string IRBuilder::llvm_ty(const TypeSpec& ts) {
  if (ts.is_fn_ptr && ts.ptr_level == 0) return "ptr";  // function pointer value
  if (ts.ptr_level > 0 && ts.is_ptr_to_array) return "ptr";
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
  case TB_LONGDOUBLE: return 16;
  case TB_INT128: case TB_UINT128: return 16;
  case TB_ENUM:   return 4;
  case TB_STRUCT: case TB_UNION: {
    if (!ts.tag || !ts.tag[0]) return 8;
    auto it = struct_defs_.find(ts.tag);
    if (it == struct_defs_.end()) return 8;
    const StructInfo& si = it->second;
    int total = 0, max_sz = 0;
    for (size_t i = 0; i < si.field_types.size(); i++) {
      if (si.field_array_sizes[i] == -2) continue;  // flex array: zero size
      TypeSpec ft = si.field_types[i];
      int sz = sizeof_ty(ft);
      if (si.field_array_sizes[i] >= 0)
        sz = (int)si.field_array_sizes[i] * sz;
      if (si.is_union) { if (sz > max_sz) max_sz = sz; }
      else             { total += sz; }
    }
    return si.is_union ? max_sz : total;
  }
  default: return 4;
  }
}

// ─────────────────────────── expression type ───────────────────────────────

TypeSpec IRBuilder::expr_type(Node* n) {
  if (!n) return void_ts();
  switch (n->kind) {
  case NK_INT_LIT: {
    // Check suffix for type: LL/ULL → long long, L/UL → long, U → unsigned int
    const char* sv = n->sval;
    bool is_hex_or_oct = sv && sv[0] == '0' && (sv[1] == 'x' || sv[1] == 'X' ||
                                                 (sv[1] >= '0' && sv[1] <= '7'));
    if (sv) {
      size_t len = strlen(sv);
      // Scan suffix (case-insensitive): strip trailing 'l', 'll', 'u'
      int lcount = 0; bool has_u = false;
      for (int i = (int)len - 1; i >= 0; i--) {
        char c = sv[i];
        if (c == 'l' || c == 'L') { lcount++; }
        else if (c == 'u' || c == 'U') { has_u = true; }
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
    // Shifts: result type = promoted left operand
    if (strcmp(op,"<<")==0 || strcmp(op,">>")==0) {
      TypeSpec lt = expr_type(n->left);
      if (lt.ptr_level > 0) lt = int_ts();
      return lt;
    }
    TypeSpec lt = expr_type(n->left);
    TypeSpec rt = expr_type(n->right);
    // Pointer subtraction: ptr - ptr → ptrdiff_t (i64)
    if (lt.ptr_level > 0 && rt.ptr_level > 0 && op && strcmp(op, "-") == 0)
      return ll_ts();
    // Pointer arithmetic: ptr +/- int → ptr
    if (lt.ptr_level > 0) return lt;
    if (rt.ptr_level > 0) return rt;
    // Usual arithmetic conversions
    if (lt.base == TB_DOUBLE || rt.base == TB_DOUBLE) return double_ts();
    if (lt.base == TB_FLOAT  || rt.base == TB_FLOAT)  return float_ts();
    if (lt.base == TB_LONGDOUBLE || rt.base == TB_LONGDOUBLE) return double_ts();
    if (lt.base == TB_LONGLONG || lt.base == TB_ULONGLONG ||
        rt.base == TB_LONGLONG || rt.base == TB_ULONGLONG)  return ll_ts();
    if (lt.base == TB_LONG || lt.base == TB_ULONG ||
        rt.base == TB_LONG || rt.base == TB_ULONG)          return make_ts(TB_LONG);
    return int_ts();
  }

  case NK_UNARY: {
    if (!n->op) return int_ts();
    if (strcmp(n->op,"!")==0) return int_ts();
    return expr_type(n->left);
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
    if (inner.ptr_level > 0) inner.ptr_level -= 1;
    else if (is_array_ty(inner)) inner = drop_array_dim(inner);
    return inner;
  }
  case NK_ASSIGN:
  case NK_COMPOUND_ASSIGN:  return expr_type(n->left);
  case NK_CALL: {
    if (n->left && n->left->name) {
      const char* cn = n->left->name;
      auto it = func_sigs_.find(cn);
      if (it != func_sigs_.end()) return it->second.ret_type;
      // Builtin return types
      if (strcmp(cn, "__builtin_bswap64") == 0) return make_ts(TB_ULONGLONG);
      if (strcmp(cn, "__builtin_bswap32") == 0) return make_ts(TB_UINT);
      if (strcmp(cn, "__builtin_bswap16") == 0) return make_ts(TB_USHORT);
      if (strcmp(cn, "__builtin_expect") == 0 && n->n_children > 0)
        return expr_type(n->children[0]);
      if (strcmp(cn, "__builtin_fabs") == 0 || strcmp(cn, "__builtin_fabsl") == 0 ||
          strcmp(cn, "__builtin_inf") == 0 || strcmp(cn, "__builtin_infl") == 0)
        return double_ts();
      if (strcmp(cn, "__builtin_fabsf") == 0 || strcmp(cn, "__builtin_inff") == 0)
        return float_ts();
    }
    // Indirect call through function pointer expression.
    // We don't model full function signatures in TypeSpec yet, so preserve
    // pointer/aggregate shape for downstream member/index typing.
    if (n->left) {
      TypeSpec callee_ts = expr_type(n->left);
      if (callee_ts.ptr_level > 0 || is_array_ty(callee_ts))
        return callee_ts;
    }
    return int_ts();
  }
  case NK_INDEX: {
    TypeSpec bt = expr_type(n->left);
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
      if (find_field(bts.tag, n->name, path))
        return field_type_from_path(bts.tag, path);
      throw std::runtime_error(std::string("unknown field ") +
                               (n->name ? n->name : "?") + " in " + bts.tag);
    }
    return int_ts();
  }
  case NK_CAST:
  case NK_COMPOUND_LIT:     return n->type;
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
      if (last && last->kind == NK_EXPR_STMT && last->left)
        return expr_type(last->left);
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
  if (ts.ptr_level > 0) {
    emit(t + " = icmp ne ptr " + val + ", null");
  } else if (llty == "double" || llty == "float") {
    emit(t + " = fcmp one " + llty + " " + val + ", 0.0");
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

  // Array type: val is already a ptr (array decays to pointer)
  if (is_array_ty(from_ts) && to_llty == "ptr") return val;
  if (is_array_ty(from_ts)) {
    // array → integer (e.g. for printf %p): ptrtoint
    emit(t + " = ptrtoint ptr " + val + " to " + to_llty);
    return t;
  }

  // Pointer conversions
  if (from_ts.ptr_level > 0 && to_llty == "ptr") return val;
  if (from_ts.ptr_level > 0) {
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

void IRBuilder::hoist_allocas(Node* node) {
  if (!node) return;
  switch (node->kind) {
  case NK_DECL: {
    if (!node->name) break;
    TypeSpec ts = node->type;
    // Infer array size from initializer for int x[] / char s[] local arrays
    if (is_array_ty(ts) && array_dim_at(ts, 0) == -2 && node->init) {
      int inferred = infer_array_size_from_init(node->init);
      if (inferred > 0) set_first_array_dim(ts, inferred);
    }
    if (node->is_static) {
      // Static local: emit as a global with internal linkage.
      // Use mangled name @__static_FUNCNAME_VARNAME to avoid collisions.
      std::string gname = "@__static_" + current_fn_name_ + "_" + node->name;
      local_slots_[node->name] = gname;
      local_types_[node->name] = ts;
      std::string llty = llvm_ty(ts);
      std::string init_val = "zeroinitializer";
      if (node->init) {
        if (node->init->kind == NK_INT_LIT)
          init_val = std::to_string(node->init->ival);
        else if (node->init->kind == NK_FLOAT_LIT)
          init_val = (ts.base == TB_FLOAT) ? fp_to_hex_float(node->init->fval)
                                           : fp_to_hex(node->init->fval);
        else if (node->init->kind == NK_UNARY && node->init->op &&
                 strcmp(node->init->op, "-") == 0 && node->init->left &&
                 node->init->left->kind == NK_INT_LIT)
          init_val = std::to_string(-node->init->left->ival);
      }
      preamble_.push_back(gname + " = internal global " + llty + " " + init_val);
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
    local_slots_[node->name] = slot;
    local_types_[node->name] = ts;
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
      std::string t = fresh_tmp();
      emit(t + " = getelementptr %struct." + cur_tag + ", ptr " + cur_ptr +
           ", i32 0, i32 " + std::to_string(ai));
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
    std::string t = fresh_tmp();
    emit(t + " = getelementptr %struct." + cur_tag + ", ptr " + cur_ptr +
         ", i32 0, i32 " + std::to_string(path.final_idx));
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
      if (si.field_array_sizes[path.final_idx] >= 0)
        ft.array_size = si.field_array_sizes[path.final_idx];
      return ft;
    }
    for (size_t i = 0; i < si.field_names.size(); i++) {
      if (!si.field_is_anon[i]) {
        TypeSpec ft = si.field_types[i];
        if (si.field_array_sizes[i] >= 0) ft.array_size = si.field_array_sizes[i];
        return ft;
      }
    }
    return int_ts();
  }

  if (path.final_idx >= 0 && path.final_idx < (int)si.field_names.size()) {
    TypeSpec ft = si.field_types[path.final_idx];
    if (si.field_array_sizes[path.final_idx] >= 0)
      ft.array_size = si.field_array_sizes[path.final_idx];
    return ft;
  }
  return int_ts();
}

// ─────────────────────────── lvalue codegen ────────────────────────────────

std::string IRBuilder::codegen_lval(Node* n) {
  if (!n) throw std::runtime_error("null lvalue node");

  switch (n->kind) {
  case NK_VAR: {
    if (!n->name) throw std::runtime_error("var has no name");
    auto it = local_slots_.find(n->name);
    if (it != local_slots_.end()) return it->second;
    // Global variable
    return std::string("@") + n->name;
  }

  case NK_DEREF:
  case NK_UNARY: {
    // *expr: evaluate the pointer value, that IS the address
    std::string ptr_val = codegen_expr(n->left);
    // ptr_val is already the address we want
    return ptr_val;
  }

  case NK_INDEX: {
    // base[index]
    TypeSpec base_ts = expr_type(n->left);
    std::string idx_val = codegen_expr(n->right);
    std::string idx_i64;
    {
      TypeSpec idx_ts = expr_type(n->right);
      idx_i64 = coerce(idx_val, idx_ts, "i64");
    }

    std::string t = fresh_tmp();
    if (is_array_ty(base_ts) && !(base_ts.ptr_level > 0 && base_ts.is_ptr_to_array)) {
      // Array variable: GEP [N x elem], ptr, 0, idx
      TypeSpec elem_ts = drop_array_dim(base_ts);
      std::string arr_llty = llvm_ty(base_ts);
      std::string elem_llty = llvm_ty(elem_ts);
      std::string ptr = codegen_lval(n->left);
      (void)elem_llty;
      emit(t + " = getelementptr " + arr_llty + ", ptr " + ptr +
           ", i64 0, i64 " + idx_i64);
    } else if (base_ts.ptr_level > 0) {
      // Pointer arithmetic: GEP elem, ptr_val, idx
      TypeSpec elem_ts = base_ts;
      elem_ts.ptr_level -= 1;
      if (elem_ts.ptr_level == 0 && elem_ts.is_ptr_to_array)
        elem_ts.is_ptr_to_array = false;
      std::string elem_llty = llvm_ty(elem_ts);
      std::string ptr_val = codegen_expr(n->left);
      emit(t + " = getelementptr " + elem_llty + ", ptr " + ptr_val +
           ", i64 " + idx_i64);
    } else {
      throw std::runtime_error("index of non-pointer non-array");
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
      throw std::runtime_error(
          "member access on non-struct ." + std::string(n->name ? n->name : "?") +
          " (base=" + std::to_string((int)obj_ts.base) +
          ", ptr=" + std::to_string(obj_ts.ptr_level) +
          ", arr_rank=" + std::to_string(array_rank_of(obj_ts)) +
          ", left=" + left_info + ")");
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
  case NK_INT_LIT:
    return std::to_string(n->ival);

  case NK_CHAR_LIT:
    return std::to_string(n->ival);

  case NK_FLOAT_LIT: {
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
            for (int k = 0; k < ndigits && i < (int)raw.size()-1 && isxdigit((unsigned char)raw[i]); k++, i++)
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
      // If this is an array type, just return the pointer (array decays)
      if (is_array_ty(ts) && !ts.is_ptr_to_array) {
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
      if (is_array_ty(ts) && !ts.is_ptr_to_array) {
        // Array global: return ptr to it
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
    return "0";
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
    } else if (is_array_ty(elem_ts)) {
      std::string arr_ptr = ptr_val;
      std::string t_gep = fresh_tmp();
      std::string arr_llty = llvm_ty(elem_ts);
      elem_ts = drop_array_dim(elem_ts);
      ptr_val = t_gep;
      emit(t_gep + " = getelementptr " + arr_llty + ", ptr " + arr_ptr + ", i64 0, i64 0");
    }
    std::string elem_llty = llvm_ty(elem_ts);
    // If the elem type is still a pointer or scalar, load it
    std::string t = fresh_tmp();
    emit(t + " = load " + elem_llty + ", ptr " + ptr_val);
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
      else
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
      std::string t = fresh_tmp();
      emit(t + " = xor " + llty + " " + v + ", -1");
      return t;
    }
    if (strcmp(n->op, "+") == 0) {
      return codegen_expr(n->left);
    }
    if (strcmp(n->op, "++") == 0 || strcmp(n->op, "++pre") == 0) {
      // Prefix ++: load, add 1, store, return new value
      TypeSpec ts = expr_type(n->left);
      std::string llty = llvm_ty(ts);
      std::string ptr = codegen_lval(n->left);
      std::string old_val = fresh_tmp();
      emit(old_val + " = load " + llty + ", ptr " + ptr);
      std::string new_val = fresh_tmp();
      if (ts.ptr_level > 0) {
        TypeSpec elem_ts = ts; elem_ts.ptr_level -= 1;
        emit(new_val + " = getelementptr " + llvm_ty(elem_ts) + ", ptr " + old_val + ", i64 1");
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
      std::string llty = llvm_ty(ts);
      std::string ptr = codegen_lval(n->left);
      std::string old_val = fresh_tmp();
      emit(old_val + " = load " + llty + ", ptr " + ptr);
      std::string new_val = fresh_tmp();
      if (ts.ptr_level > 0) {
        TypeSpec elem_ts = ts; elem_ts.ptr_level -= 1;
        emit(new_val + " = getelementptr " + llvm_ty(elem_ts) + ", ptr " + old_val + ", i64 -1");
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
    std::string llty = llvm_ty(ts);
    std::string ptr = codegen_lval(n->left);
    std::string old_val = fresh_tmp();
    emit(old_val + " = load " + llty + ", ptr " + ptr);
    std::string new_val = fresh_tmp();
    bool is_ptr = ts.ptr_level > 0;
    if (n->op && strcmp(n->op, "++") == 0) {
      if (is_ptr) {
        TypeSpec elem_ts = ts; elem_ts.ptr_level -= 1;
        emit(new_val + " = getelementptr " + llvm_ty(elem_ts) + ", ptr " +
             old_val + ", i64 1");
      } else if (llty == "float" || llty == "double") {
        emit(new_val + " = fadd " + llty + " " + old_val + ", 1.0");
      } else {
        emit(new_val + " = add " + llty + " " + old_val + ", 1");
      }
    } else {
      if (is_ptr) {
        TypeSpec elem_ts = ts; elem_ts.ptr_level -= 1;
        emit(new_val + " = getelementptr " + llvm_ty(elem_ts) + ", ptr " +
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
    std::string lv = codegen_expr(n->left);
    std::string rv = codegen_expr(n->right);

    // Determine result type
    TypeSpec res_ts = expr_type(n);
    std::string res_llty = llvm_ty(res_ts);

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
      bool lptr_only = lt.ptr_level > 0;
      bool rptr_only = rt.ptr_level > 0;
      if (lptr_only && !rptr_only) {
        // ptr +/- int
        TypeSpec elem_ts = lt; elem_ts.ptr_level -= 1;
        std::string elem_llty = llvm_ty(elem_ts);
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
        TypeSpec elem_ts = rt; elem_ts.ptr_level -= 1;
        std::string elem_llty = llvm_ty(elem_ts);
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
      TypeSpec elem_ts2 = lt; elem_ts2.ptr_level -= 1;
      int esz = sizeof_ty(elem_ts2);
      if (esz > 1) {
        std::string t4 = fresh_tmp();
        emit(t4 + " = sdiv i64 " + t3 + ", " + std::to_string(esz));
        return t4;
      }
      return t3;
    }

    bool is_float = is_float_base(res_ts.base);

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
      // Shift: result type = promoted left type.
      // rv was already coerced to res_llty (= shift_llty) by the generic block
      // above — do NOT coerce again or we'll emit "trunc i32 i16_val to i16".
      std::string shift_llty = llvm_ty(lt);
      emit(t + " = shl " + shift_llty + " " + lv + ", " + rv);
    } else if (strcmp(op, ">>") == 0) {
      std::string shift_llty = llvm_ty(lt);
      std::string shr_op = is_unsigned_base(lt.base) ? "lshr" : "ashr";
      emit(t + " = " + shr_op + " " + shift_llty + " " + lv + ", " + rv);
    } else if (strcmp(op, "&") == 0) {
      emit(t + " = and " + res_llty + " " + lv + ", " + rv);
    } else if (strcmp(op, "|") == 0) {
      emit(t + " = or " + res_llty + " " + lv + ", " + rv);
    } else if (strcmp(op, "^") == 0) {
      emit(t + " = xor " + res_llty + " " + lv + ", " + rv);
    } else if (strcmp(op, "==") == 0) {
      return emit_cmp("eq", "oeq");
    } else if (strcmp(op, "!=") == 0) {
      return emit_cmp("ne", "one");
    } else if (strcmp(op, "<") == 0) {
      return emit_cmp(is_unsigned_base(lt.base) ? "ult" : "slt", "olt");
    } else if (strcmp(op, "<=") == 0) {
      return emit_cmp(is_unsigned_base(lt.base) ? "ule" : "sle", "ole");
    } else if (strcmp(op, ">") == 0) {
      return emit_cmp(is_unsigned_base(lt.base) ? "ugt" : "sgt", "ogt");
    } else if (strcmp(op, ">=") == 0) {
      return emit_cmp(is_unsigned_base(lt.base) ? "uge" : "sge", "oge");
    } else {
      // Unknown operator: return 0
      return "0";
    }
    return t;
  }

  case NK_ASSIGN: {
    const char* aop = n->op ? n->op : "=";
    TypeSpec lts = expr_type(n->left);
    std::string llty = llvm_ty(lts);

    if (strcmp(aop, "=") == 0) {
      // Plain assignment
      TypeSpec rts = expr_type(n->right);
      std::string rval = codegen_expr(n->right);
      rval = coerce(rval, rts, llty);
      std::string ptr = codegen_lval(n->left);
      emit("store " + llty + " " + rval + ", ptr " + ptr);
      return rval;
    }

    // Compound assignment: lhs op= rhs
    std::string ptr = codegen_lval(n->left);
    std::string cur = fresh_tmp();
    emit(cur + " = load " + llty + ", ptr " + ptr);

    TypeSpec rts = expr_type(n->right);
    std::string rv = codegen_expr(n->right);
    bool is_float = is_float_base(lts.base);

    // C99: float op= expr does arithmetic in double precision, then truncates to float
    bool needs_fp_upcast = (lts.base == TB_FLOAT && lts.ptr_level == 0);
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

    // For pointer arithmetic, keep rv as integer (coerce to i64 for GEP index)
    // For non-pointer ops, coerce to lhs type
    if (!needs_fp_upcast && lts.ptr_level == 0)
      rv = coerce(rv, rts, llty);

    std::string result = fresh_tmp();

    if (strcmp(aop, "+=") == 0) {
      // Handle pointer arithmetic for +=
      if (lts.ptr_level > 0) {
        TypeSpec elem_ts = lts; elem_ts.ptr_level -= 1;
        std::string idx = coerce(rv, rts, "i64");
        emit(result + " = getelementptr " + llvm_ty(elem_ts) + ", ptr " + cur + ", i64 " + idx);
      } else {
        emit(result + " = " + (is_float ? "fadd" : "add") + " " + llty + " " + cur + ", " + rv);
      }
    } else if (strcmp(aop, "-=") == 0) {
      if (lts.ptr_level > 0) {
        TypeSpec elem_ts = lts; elem_ts.ptr_level -= 1;
        std::string idx = coerce(rv, rts, "i64");
        std::string neg = fresh_tmp();
        emit(neg + " = sub i64 0, " + idx);
        emit(result + " = getelementptr " + llvm_ty(elem_ts) + ", ptr " + cur + ", i64 " + neg);
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
    TypeSpec lts = expr_type(n->left);
    std::string llty = llvm_ty(lts);
    std::string ptr = codegen_lval(n->left);
    std::string cur = fresh_tmp();
    emit(cur + " = load " + llty + ", ptr " + ptr);

    TypeSpec rts = expr_type(n->right);
    std::string rv = codegen_expr(n->right);
    bool is_float = is_float_base(lts.base);

    // C99: float op= expr does arithmetic in double precision, then truncates to float
    bool needs_fp_upcast = (lts.base == TB_FLOAT && lts.ptr_level == 0);
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

    // For pointer arithmetic, keep rv as integer; coerce to lhs type for scalar ops
    if (!needs_fp_upcast && lts.ptr_level == 0)
      rv = coerce(rv, rts, llty);

    std::string result = fresh_tmp();
    const char* op = n->op;

    if (strcmp(op, "+=") == 0) {
      if (lts.ptr_level > 0) {
        TypeSpec elem_ts = lts; elem_ts.ptr_level -= 1;
        std::string idx = coerce(rv, rts, "i64");
        emit(result + " = getelementptr " + llvm_ty(elem_ts) + ", ptr " + cur + ", i64 " + idx);
      } else {
        emit(result + " = " + (is_float ? "fadd" : "add") + " " + llty + " " + cur + ", " + rv);
      }
    } else if (strcmp(op, "-=") == 0) {
      if (lts.ptr_level > 0) {
        TypeSpec elem_ts = lts; elem_ts.ptr_level -= 1;
        std::string idx = coerce(rv, rts, "i64");
        std::string neg = fresh_tmp();
        emit(neg + " = sub i64 0, " + idx);
        emit(result + " = getelementptr " + llvm_ty(elem_ts) + ", ptr " + cur + ", i64 " + neg);
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
    emit("store " + llty + " " + result + ", ptr " + ptr);
    return result;
  }

  case NK_CALL: {
    if (!n->left) return "0";
    std::string fn_name;
    TypeSpec ret_ts = int_ts();
    bool variadic = false;
    std::vector<TypeSpec> param_types;
    bool is_indirect = false;
    std::string fn_ptr;

    if (n->left->kind == NK_VAR && n->left->name) {
      fn_name = n->left->name;
      auto sig_it = func_sigs_.find(fn_name);
      if (sig_it != func_sigs_.end()) {
        ret_ts = sig_it->second.ret_type;
        variadic = sig_it->second.variadic;
        param_types = sig_it->second.param_types;
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
        } else {
          ret_ts = expr_type(n);
        }
        fn_name.clear();
      } else {
        // Unknown function — will need auto-declare; mark as variadic for safety
        variadic = true;
        // Check known pointer-returning functions
        static const char* ptr_fns[] = {
          "malloc","calloc","realloc","strdup","strndup","memcpy","memmove","memset",
          "strcpy","strncpy","strcat","strncat","strchr","strrchr","strstr",
          "fopen","fdopen","freopen","popen","tmpfile","gets","fgets",
          "getenv","getenv","realpath","getcwd","dirname","basename",
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
          }
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
      if (fn_name == "__builtin_inf" || fn_name == "__builtin_infl") {
        ret_ts = double_ts();
        return "0x7FF0000000000000";  // +Inf as double hex
      }
      if (fn_name == "__builtin_inff") {
        ret_ts = float_ts();
        return "0x7FF0000000000000";  // LLVM accepts double-form hex for float +Inf
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

    // Emit argument evaluations
    std::vector<std::string> arg_vals;
    std::vector<TypeSpec> arg_types;
    for (int i = 0; i < n->n_children; i++) {
      TypeSpec ats = expr_type(n->children[i]);
      std::string av = codegen_expr(n->children[i]);
      // Coerce to parameter type if known
      if (i < (int)param_types.size()) {
        av = coerce(av, ats, llvm_ty(param_types[i]));
        ats = param_types[i];
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
                 (arg_types[i].base == TB_STRUCT || arg_types[i].base == TB_UNION)) {
        // ARM64 ABI: struct/union varargs must be coerced to [N x i64] so the
        // LLVM backend places them in the correct integer register/stack slots.
        // (Without coercion, the backend may pass the struct in FP/SIMD slots
        //  or use an incompatible layout, breaking va_arg on the callee side.)
        int sz = sizeof_ty(arg_types[i]);
        int n_slots = (sz + 7) / 8;
        if (n_slots < 1) n_slots = 1;
        std::string slot_ty = "[" + std::to_string(n_slots) + " x i64]";
        std::string struct_llty = llvm_ty(arg_types[i]);

        // Alloca [N x i64] aligned 8 in entry block
        std::string coerce_buf = "%va_coerce_" + std::to_string(tmp_idx_++);
        alloca_lines_.push_back("  " + coerce_buf + " = alloca " + slot_ty + ", align 8");

        // Store struct value to the coerce buffer (opaque ptr: no bitcast needed)
        emit("store " + struct_llty + " " + arg_vals[i] + ", ptr " + coerce_buf);

        // Reload as [N x i64]
        std::string coerced = fresh_tmp();
        emit(coerced + " = load " + slot_ty + ", ptr " + coerce_buf + ", align 8");
        args_str += slot_ty + " " + coerced;
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
    std::string t = fresh_tmp();
    emit(t + " = load " + llty + ", ptr " + ptr);
    return t;
  }

  case NK_CAST: {
    TypeSpec from_ts = expr_type(n->left);
    std::string v = codegen_expr(n->left);
    return coerce(v, from_ts, llvm_ty(n->type));
  }

  case NK_TERNARY: {
    // cond ? then_ : else_
    // Use alloca-based approach: result alloca, store in each branch, load at end
    TypeSpec res_ts = expr_type(n);
    std::string res_llty = llvm_ty(res_ts);

    // Allocate result slot (in alloca area)
    std::string res_slot = "%ternary_" + std::to_string(tmp_idx_++);
    alloca_lines_.push_back("  " + res_slot + " = alloca " + res_llty);

    TypeSpec cond_ts = expr_type(n->cond);
    std::string cond_val = codegen_expr(n->cond);
    std::string cond_b = to_bool(cond_val, cond_ts);

    std::string then_lbl = fresh_label("tern_then");
    std::string else_lbl = fresh_label("tern_else");
    std::string end_lbl  = fresh_label("tern_end");

    emit_terminator("br i1 " + cond_b + ", label %" + then_lbl +
                    ", label %" + (n->else_ ? else_lbl : end_lbl));

    emit_label(then_lbl);
    std::string then_val = codegen_expr(n->then_);
    TypeSpec then_ts = expr_type(n->then_);
    then_val = coerce(then_val, then_ts, res_llty);
    emit("store " + res_llty + " " + then_val + ", ptr " + res_slot);
    emit_terminator("br label %" + end_lbl);

    if (n->else_) {
      emit_label(else_lbl);
      std::string else_val = codegen_expr(n->else_);
      TypeSpec else_ts = expr_type(n->else_);
      else_val = coerce(else_val, else_ts, res_llty);
      emit("store " + res_llty + " " + else_val + ", ptr " + res_slot);
      emit_terminator("br label %" + end_lbl);
    }

    emit_label(end_lbl);
    std::string t = fresh_tmp();
    emit(t + " = load " + res_llty + ", ptr " + res_slot);
    return t;
  }

  case NK_SIZEOF_EXPR: {
    int sz = sizeof_ty(expr_type(n->left));
    return std::to_string(sz);
  }
  case NK_SIZEOF_TYPE: {
    int sz = sizeof_ty(n->type);
    return std::to_string(sz);
  }
  case NK_ALIGNOF_TYPE: {
    // Simplified: return sizeof
    int sz = sizeof_ty(n->type);
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
    std::string llty = llvm_ty(ts);
    std::string slot = "%clit_" + std::to_string(tmp_idx_++);
    alloca_lines_.push_back("  " + slot + " = alloca " + llty);
    // Initializer is in ->left (set by parser). Use emit_agg_init_impl for
    // full brace-elision, sub-list, and designator support.
    Node* cl_init = n->left ? n->left : n->init;
    if (cl_init && cl_init->kind == NK_INIT_LIST &&
        (is_array_ty(ts) || ts.base == TB_STRUCT || ts.base == TB_UNION)) {
      int flat_idx = 0;
      emit_agg_init_impl(this, ts, slot, cl_init, flat_idx);
    }
    return slot;
  }

  case NK_VA_ARG: {
    // va_arg(ap, T): ARM64 ABI.
    // va_list is alloca ptr (a pointer to the next vararg slot on the stack).
    // For struct/union aggregates, clang crashes on `va_arg ptr %ap, %struct.T`;
    // use the manual load/advance/memcpy pattern instead.
    TypeSpec ts = n->type;
    std::string llty = llvm_ty(ts);
    std::string ap_ptr = codegen_lval(n->left);

    bool is_aggregate = (ts.ptr_level == 0 && !is_array_ty(ts) &&
                         (ts.base == TB_STRUCT || ts.base == TB_UNION));
    if (is_aggregate) {
      int sz = sizeof_ty(ts);
      // ARM64: each vararg slot occupies a multiple-of-8 bytes
      int advance = (sz + 7) & ~7;

      // Ensure @llvm.memcpy intrinsic is declared
      if (!auto_declared_fns_.count("llvm.memcpy.p0.p0.i64"))
        auto_declared_fns_["llvm.memcpy.p0.p0.i64"] =
          "declare void @llvm.memcpy.p0.p0.i64(ptr noalias nocapture writeonly, "
          "ptr noalias nocapture readonly, i64, i1 immarg)";

      // Load current slot pointer from va_list
      std::string cur = fresh_tmp();
      emit(cur + " = load ptr, ptr " + ap_ptr + ", align 8");

      // Advance va_list pointer
      std::string nxt = fresh_tmp();
      emit(nxt + " = getelementptr i8, ptr " + cur + ", i64 " + std::to_string(advance));
      emit("store ptr " + nxt + ", ptr " + ap_ptr + ", align 8");

      // Alloca temp for the struct value and memcpy
      std::string tmp = "%va_tmp_" + std::to_string(tmp_idx_++);
      alloca_lines_.push_back("  " + tmp + " = alloca " + llty);
      emit("call void @llvm.memcpy.p0.p0.i64(ptr align 1 " + tmp +
           ", ptr align 8 " + cur + ", i64 " + std::to_string(sz) + ", i1 false)");

      // Load the struct value from the temp
      std::string t = fresh_tmp();
      emit(t + " = load " + llty + ", ptr " + tmp);
      return t;
    }

    // Scalar / pointer: use LLVM's va_arg instruction directly
    std::string t = fresh_tmp();
    emit(t + " = va_arg ptr " + ap_ptr + ", " + llty);
    return t;
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
      bool is_desig = false;
      int target_fi = fi;
      bool item_is_desig = (item && item->kind == NK_INIT_ITEM && item->desig_field);
      // For non-designated items: stop BEFORE consuming if all fields are done.
      if (!item_is_desig && fi >= n_fields) break;
      // Unwrap NK_INIT_ITEM
      if (item && item->kind == NK_INIT_ITEM) {
        if (item->desig_field) {
          is_desig = true;
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
      if (si.field_array_sizes[fi] >= 0) ft.array_size = si.field_array_sizes[fi];
      std::string field_llty = irb->llvm_ty(ft);
      // Get a GEP for this field
      std::string fgep = irb->fresh_tmp();
      irb->emit(fgep + " = getelementptr " + struct_llty + ", ptr " + ptr +
                ", i32 0, i32 " + std::to_string(fi));
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
                     ft.ptr_level == 0 && val_node->kind == NK_COMPOUND_LIT) {
            // Compound literal: val is a ptr to the struct; load it
            std::string ld = irb->fresh_tmp();
            irb->emit(ld + " = load " + field_llty + ", ptr " + val);
            irb->emit("store " + field_llty + " " + ld + ", ptr " + fgep);
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

  // ── Array type ────────────────────────────────────────────────────────────
  if (is_array_ty(ts) && ts.ptr_level == 0) {
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
        else if (elem_ts.ptr_level == 0 && (elem_ts.base == TB_STRUCT || elem_ts.base == TB_UNION) &&
                 val_node && val_node->kind == NK_COMPOUND_LIT) {
          std::string ld = irb->fresh_tmp();
          irb->emit(ld + " = load " + elem_llty + ", ptr " + val);
          val = ld;
        } else {
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
    // Preserve lexical scope for same-named locals in nested blocks.
    auto saved_slots = local_slots_;
    auto saved_types = local_types_;
    for (int i = 0; i < n->n_children; i++)
      emit_stmt(n->children[i]);
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
      emit_terminator("ret " + current_fn_ret_llty_ + " " + rv);
    }
    break;
  }

  case NK_IF: {
    TypeSpec cond_ts = expr_type(n->cond);
    std::string cond_val = codegen_expr(n->cond);
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
    if (!break_stack_.empty())
      emit_terminator("br label %" + break_stack_.back());
    break;
  }

  case NK_CONTINUE: {
    if (!loop_stack_.empty())
      emit_terminator("br label %" + loop_stack_.back().first);
    break;
  }

  case NK_GOTO: {
    if (n->name) {
      auto it = user_labels_.find(n->name);
      if (it != user_labels_.end())
        emit_terminator("br label %" + it->second);
      else
        emit_terminator("br label %user_" + std::string(n->name) + "_0");
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
    cond_val = coerce(cond_val, cond_ts, "i32");

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
        if (!sw_info.case_labels.count(cv)) {
          sw_info.case_labels[cv] = fresh_label("switch_case");
        }
        scan_cases(node->body);
      } else if (node->kind == NK_DEFAULT) {
        if (sw_info.default_label.empty())
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

    // Emit the switch instruction
    std::string sw_instr = "switch i32 " + cond_val + ", label %" + def_lbl + " [";
    for (auto& kv : sw_info.case_labels) {
      sw_instr += " i32 " + std::to_string(kv.first) + ", label %" + kv.second;
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
    if (switch_stack_.empty()) break;
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
    if (switch_stack_.empty()) break;
    SwitchInfo& sw = switch_stack_.back();
    if (!sw.default_label.empty()) {
      emit_terminator("br label %" + sw.default_label);
      emit_label(sw.default_label);
    }
    if (n->body) emit_stmt(n->body);
    break;
  }

  case NK_CASE_RANGE: {
    // GCC extension: case lo ... hi: - treat as single case for lo
    if (switch_stack_.empty()) break;
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

  StructInfo si;
  si.is_union = sd->is_union;

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
    preamble_.push_back(std::string("%struct.") + tag + " = type { [" +
                        std::to_string(max_sz) + " x i8] }");
  } else {
    std::string fields_str;
    bool first = true;
    for (size_t i = 0; i < si.field_types.size(); i++) {
      // Skip flex array members (array_size == -2): zero-size trailing arrays
      if (si.field_array_sizes[i] == -2) continue;
      if (!first) fields_str += ", ";
      first = false;
      TypeSpec ft = si.field_types[i];
      if (si.field_array_sizes[i] >= 0) {
        ft.array_size = si.field_array_sizes[i];
      }
      fields_str += llvm_ty(ft);
    }
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
    const char* raw = init->sval ? init->sval : "\"\"";
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
  if (n->kind == NK_CAST && n->left)
    return static_eval_int(n->left, enum_consts);
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
    if (strcmp(n->op, ">>") == 0) return (long long)((unsigned long long)l >> (r & 63));
  }
  return 0;  // unknown expression
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
    std::vector<std::string> fvals(n_fields, "zeroinitializer");
    for (int fi = 0; fi < n_fields; fi++) {
      TypeSpec ft = si.field_types[fi];
      if (si.field_array_sizes[fi] >= 0) {
        long long dims[1] = {si.field_array_sizes[fi]};
        set_array_dims(ft, dims, 1);
      }
      fvals[fi] = global_const_flat_impl(irb, ft, init_list, flat_idx);
    }
    std::string result = "{ ";
    for (int fi = 0; fi < n_fields; fi++) {
      if (fi > 0) result += ", ";
      TypeSpec ft = si.field_types[fi];
      if (si.field_array_sizes[fi] >= 0) {
        long long dims[1] = {si.field_array_sizes[fi]};
        set_array_dims(ft, dims, 1);
      }
      result += irb->llvm_ty(ft) + " " + fvals[fi];
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
  if (is_array_ty(ts) && ts.ptr_level == 0) {
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
        bool is_desig = false;
        bool item_is_desig = (item && item->kind == NK_INIT_ITEM && item->desig_field);
        // Stop for non-designated items when fields exhausted
        if (!item_is_desig && fi >= n) break;
        if (item && item->kind == NK_INIT_ITEM) {
          if (item->desig_field) {
            is_desig = true;
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
    if (init->kind == NK_INT_LIT && init->ival == 0) return "null";
    if (init->kind == NK_ADDR && init->left) {
      Node* operand = init->left;
      // &global_var
      if (operand->kind == NK_VAR && operand->name)
        return std::string("@") + operand->name;
      // &global_array[0]
      if (operand->kind == NK_INDEX && operand->left &&
          operand->left->kind == NK_VAR && operand->left->name &&
          operand->right && operand->right->kind == NK_INT_LIT) {
        TypeSpec arr_ts = global_types_.count(operand->left->name)
                          ? global_types_[operand->left->name] : ts;
        TypeSpec elem_ts = drop_array_dim(arr_ts);
        std::string arr_llty = llvm_ty(arr_ts);
        std::string elem_llty = llvm_ty(elem_ts);
        return std::string("getelementptr (") + arr_llty + ", ptr @" +
               operand->left->name + ", i64 0, i64 " +
               std::to_string(operand->right->ival) + ")";
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
          if (si.field_array_sizes[fi] >= 0) ft.array_size = si.field_array_sizes[fi];
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
      bool any_set = false;
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
              any_set = true;
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
          any_set = true;
        }
      } else if (init->kind != NK_INT_LIT || init->ival != 0) {
        // Scalar or non-zero scalar
        if (n_fields > 0) {
          TypeSpec ft0 = si.field_types[0];
          if (si.field_array_sizes[0] >= 0) ft0.array_size = si.field_array_sizes[0];
          collect_bytes(this, ft0, init, bytes);
        }
        any_set = true;
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
    // Skip flex array members (field_array_sizes == -2): zero-size trailing arrays
    std::string result = "{ ";
    bool first_field = true;
    for (int i = 0; i < n_fields; i++) {
      if (si.field_array_sizes[i] == -2) continue;  // flex array: skip
      if (!first_field) result += ", ";
      first_field = false;
      TypeSpec ft = si.field_types[i];
      if (si.field_array_sizes[i] >= 0) ft.array_size = si.field_array_sizes[i];
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
    if (init->kind == NK_BINOP || init->kind == NK_UNARY) {
      long long ival = static_eval_int(init, enum_consts_);
      return fhex((double)ival);
    }
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
  if (init->kind == NK_CAST && init->left)
    return global_const(ts, init->left);
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

  std::string linkage = gv->is_static ? "internal " : "";
  preamble_.push_back("@" + name + " = " + linkage + "global " + llty + " " + init_val);
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
    if (is_array_ty(pts)) { pts = drop_array_dim(pts); pts.ptr_level += 1; }
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

    // Array params decay to ptr in the function signature
    bool is_array = is_array_ty(p->type);
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

  // Pre-scan for user goto labels
  prescan_labels(fn->body);

  // Hoist all local variable allocas
  hoist_allocas(fn->body);

  // Emit the function body
  emit_stmt(fn->body);

  // Ensure function ends with a terminator
  if (!last_was_terminator_) {
    if (ret_llty == "void")
      emit_terminator("ret void");
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
          if (is_array_ty(pts)) { pts = drop_array_dim(pts); pts.ptr_level += 1; }
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
