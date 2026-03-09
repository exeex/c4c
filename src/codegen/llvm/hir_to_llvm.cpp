#include "hir_to_llvm.hpp"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstring>
#include <functional>
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

static bool is_signed_int(TypeBase b) {
  switch (b) {
    case TB_CHAR: case TB_SCHAR: case TB_SHORT: case TB_INT:
    case TB_LONG: case TB_LONGLONG: case TB_INT128:
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
    case TB_DOUBLE: case TB_LONGDOUBLE:         return "double";
    case TB_VA_LIST:                            return "ptr";
    default:                                    return "i32";
  }
}

static std::string llvm_ty(const TypeSpec& ts) {
  if (ts.ptr_level > 0 || ts.is_fn_ptr) return "ptr";
  if (ts.array_rank > 0) return "ptr";  // decayed
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
  if (ts.base == TB_VA_LIST) return "%struct.__va_list_tag_";
  if (ts.base == TB_STRUCT || ts.base == TB_UNION) {
    if (ts.tag && ts.tag[0]) return "%struct." + std::string(ts.tag);
  }
  if (ts.base == TB_VOID) return "i8";
  return llvm_base(ts.base);
}

static int sizeof_base(TypeBase b) {
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

static int sizeof_ts(const TypeSpec& ts) {
  if (ts.ptr_level > 0 || ts.is_fn_ptr) return 8;
  if (ts.array_rank > 0) {
    if (ts.array_size > 0) {
      // Recursively compute element size
      TypeSpec elem = ts;
      elem.array_rank--;
      if (elem.array_rank > 0) {
        for (int i = 0; i < elem.array_rank; ++i) elem.array_dims[i] = elem.array_dims[i + 1];
      }
      elem.array_size = (elem.array_rank > 0) ? elem.array_dims[0] : -1;
      return (int)(ts.array_size * sizeof_ts(elem));
    }
    return 8;  // unknown size — treat as pointer
  }
  return sizeof_base(ts.base);
}

// ── Struct/union type helpers ──────────────────────────────────────────────────

// LLVM type string for a struct field (non-decayed: may be array or nested struct)
static std::string llvm_field_ty(const HirStructField& f) {
  if (f.array_first_dim >= 0) {
    // Array field: [N x elem]
    if (f.elem_type.base == TB_STRUCT || f.elem_type.base == TB_UNION) {
      if (f.elem_type.tag && f.elem_type.tag[0])
        return "[" + std::to_string(f.array_first_dim) + " x %struct." +
               std::string(f.elem_type.tag) + "]";
    }
    return "[" + std::to_string(f.array_first_dim) + " x " +
           llvm_base(f.elem_type.base) + "]";
  }
  if (f.elem_type.ptr_level > 0 || f.elem_type.is_fn_ptr) return "ptr";
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
  if (f.array_first_dim > 0) sz *= (int)f.array_first_dim;
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
    return max_sz > 0 ? max_sz : 1;
  }
  int total = 0;
  for (const auto& f : sd.fields) total += compute_field_size(mod, f);
  return total > 0 ? total : 1;
}

static std::string fp_to_hex(double v) {
  uint64_t bits;
  static_assert(sizeof(double) == sizeof(uint64_t));
  std::memcpy(&bits, &v, 8);
  char buf[32];
  std::snprintf(buf, sizeof(buf), "0x%016llX", (unsigned long long)bits);
  return buf;
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
    if (need_llvm_stacksave_) out << "declare ptr @llvm.stacksave()\n";
    if (need_llvm_stackrestore_) out << "declare void @llvm.stackrestore(ptr)\n";
    if (need_llvm_va_start_ || need_llvm_va_end_ || need_llvm_va_copy_ ||
        need_llvm_stacksave_ || need_llvm_stackrestore_) out << "\n";
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
      if (sd.fields.empty()) continue;
      if (sd.is_union) {
        const int sz = compute_struct_size(mod_, tag);
        preamble_ << "%struct." << tag << " = type { ["
                  << sz << " x i8] }\n";
      } else {
        preamble_ << "%struct." << tag << " = type { ";
        bool first = true;
        for (const auto& f : sd.fields) {
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

  std::string emit_const_scalar_expr(ExprId id, const TypeSpec& expected_ts) {
    const Expr& e = get_expr(id);
    return std::visit([&](const auto& p) -> std::string {
      using T = std::decay_t<decltype(p)>;
      if constexpr (std::is_same_v<T, IntLiteral>) {
        if (llvm_ty(expected_ts) == "ptr") {
          if (p.value == 0) return "null";
          return "inttoptr (i64 " + std::to_string(p.value) + " to ptr)";
        }
        if (is_float_base(expected_ts.base) && expected_ts.ptr_level == 0)
          return fp_to_hex(static_cast<double>(p.value));
        return std::to_string(p.value);
      } else if constexpr (std::is_same_v<T, FloatLiteral>) {
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
        }
        if (p.op == UnaryOp::Minus) {
          const Expr& op_e = get_expr(p.operand);
          if (const auto* i = std::get_if<IntLiteral>(&op_e.payload))
            return std::to_string(-i->value);
          if (const auto* f = std::get_if<FloatLiteral>(&op_e.payload))
            return fp_to_hex(-f->value);
        }
        return (llvm_ty(expected_ts) == "ptr") ? "null" : "0";
      } else if constexpr (std::is_same_v<T, CastExpr>) {
        return emit_const_scalar_expr(p.expr, expected_ts);
      } else if constexpr (std::is_same_v<T, BinaryExpr>) {
        auto val = try_const_eval_int(id);
        if (val) {
          if (llvm_ty(expected_ts) == "ptr") return "null";
          return std::to_string(*val);
        }
        return (llvm_ty(expected_ts) == "ptr") ? "null" : "0";
      } else if constexpr (std::is_same_v<T, LabelAddrExpr>) {
        return "blockaddress(@" + p.fn_name + ", %ulbl_" + p.label_name + ")";
      } else {
        return (llvm_ty(expected_ts) == "ptr") ? "null" : "0";
      }
    }, e.payload);
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
      return "zeroinitializer";
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

  std::string emit_const_struct(const TypeSpec& ts, const GlobalInit& init) {
    if (!ts.tag || !ts.tag[0]) return "zeroinitializer";
    const auto it = mod_.struct_defs.find(ts.tag);
    if (it == mod_.struct_defs.end()) return "zeroinitializer";
    const HirStructDef& sd = it->second;
    if (sd.is_union) {
      const int sz = compute_struct_size(mod_, std::string(ts.tag));
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
      return "{ [" + std::to_string(sz) + " x i8] zeroinitializer }";
    }

    std::vector<std::string> field_vals;
    field_vals.reserve(sd.fields.size());
    for (const auto& f : sd.fields) {
      field_vals.push_back(emit_const_init(field_decl_type(f), GlobalInit(std::monostate{})));
    }

    if (const auto* list = std::get_if<InitList>(&init)) {
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
        const TypeSpec field_ts = field_decl_type(sd.fields[idx]);
        if (llvm_field_ty(sd.fields[idx]) == "ptr") {
          if (auto ptr_init = try_emit_ptr_from_char_init(child_init)) {
            field_vals[idx] = *ptr_init;
            next_idx = idx + 1;
            continue;
          }
        }
        field_vals[idx] = emit_const_init(field_ts, child_init);
        next_idx = idx + 1;  // always advance; designators set the base, next follows
      }
    } else if (const auto* scalar = std::get_if<InitScalar>(&init)) {
      // Scalar fallback for aggregates: treat as first field init.
      if (!sd.fields.empty()) {
        field_vals[0] = emit_const_scalar_expr(scalar->expr, field_decl_type(sd.fields[0]));
      }
    }

    std::string out = "{ ";
    for (size_t i = 0; i < sd.fields.size(); ++i) {
      if (i) out += ", ";
      out += llvm_field_ty(sd.fields[i]) + " " + field_vals[i];
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
    const long long deduced = deduce_array_size_from_init(init);
    if (deduced <= 0) return ts;
    TypeSpec out = ts;
    out.array_size = deduced;
    out.array_dims[0] = deduced;
    return out;
  }

  void emit_global(const GlobalVar& gv) {
    // Resolve unsized array dimensions from the initializer (e.g. int a[] = {1,2,3}).
    const TypeSpec ts = resolve_array_ts(gv.type.spec, gv.init);
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
    if (ft == tt) return val;
    if (ft == "ptr" && tt == "ptr") return val;

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
      const int fb = (from_ts.base == TB_FLOAT) ? 32 : 64;
      const int tb = (to_ts.base == TB_FLOAT) ? 32 : 64;
      if (fb == tb) return val;
      const std::string tmp = fresh_tmp(ctx);
      const std::string op = (tb > fb) ? "fpext" : "fptrunc";
      emit_instr(ctx, tmp + " = " + op + " " + ft + " " + val + " to " + tt);
      return tmp;
    }

    // int → float
    if (is_any_int(from_ts.base) && is_float_base(to_ts.base)) {
      const std::string tmp = fresh_tmp(ctx);
      const std::string op = is_signed_int(from_ts.base) ? "sitofp" : "uitofp";
      emit_instr(ctx, tmp + " = " + op + " " + ft + " " + val + " to " + tt);
      return tmp;
    }

    // float → int
    if (is_float_base(from_ts.base) && is_any_int(to_ts.base)) {
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
    if (is_float_base(ts.base)) {
      emit_instr(ctx, tmp + " = fcmp une " + ty + " " + val + ", 0.0");
    } else if (ty == "ptr") {
      emit_instr(ctx, tmp + " = icmp ne ptr " + val + ", null");
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
        chain.push_back({tag, f.llvm_idx, sd.is_union});
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
                               TypeSpec& out_field_ts) {
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
    return cur_ptr;
  }

  // ── Lvalue emission ───────────────────────────────────────────────────────

  std::string emit_lval(FnCtx& ctx, ExprId id, TypeSpec& pointee_ts) {
    const Expr& e = get_expr(id);
    return emit_lval_dispatch(ctx, e, pointee_ts);
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
        const auto& gv = mod_.globals[r->global->value];
        pts = gv.type.spec;
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
      // Check array_rank first so that T*[N] strips the array dim (→ T*),
      // not the pointer level (which would wrongly give T[N]).
      pts = base_ts;
      if (pts.array_rank > 0) {
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

  std::string emit_member_lval(FnCtx& ctx, const MemberExpr& m, TypeSpec& out_pts) {
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
    return emit_member_gep(ctx, base_ptr, tag, m.field, out_pts);
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

  TypeSpec resolve_payload_type(FnCtx& ctx, const DeclRef& r) {
    if (r.local) {
      const auto it = ctx.local_types.find(r.local->value);
      if (it != ctx.local_types.end()) return it->second;
    }
    if (r.param_index && ctx.fn && *r.param_index < ctx.fn->params.size())
      return ctx.fn->params[*r.param_index].type.spec;
    if (r.global && r.global->value < mod_.globals.size())
      return mod_.globals[r.global->value].type.spec;
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

    // For arithmetic/bitwise the result type matches the operand type
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
    return {};
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
    return field_ts;
  }

  TypeSpec resolve_payload_type(FnCtx& ctx, const IndexExpr& idx) {
    TypeSpec base_ts = resolve_expr_type(ctx, idx.base);
    if (base_ts.ptr_level > 0) { base_ts.ptr_level--; return base_ts; }
    if (base_ts.array_rank > 0) { base_ts.array_rank--; return base_ts; }
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

  std::string emit_rval_payload(FnCtx&, const IntLiteral& x, const Expr&) {
    return std::to_string(x.value);
  }

  std::string emit_rval_payload(FnCtx&, const FloatLiteral& x, const Expr&) {
    return fp_to_hex(x.value);
  }

  std::string emit_rval_payload(FnCtx&, const CharLiteral& x, const Expr&) {
    return std::to_string(x.value);
  }

  std::string emit_rval_payload(FnCtx& ctx, const StringLiteral& sl, const Expr& /*e*/) {
    // Strip enclosing quotes from raw, use as bytes
    std::string bytes = sl.raw;
    if (bytes.size() >= 2 && bytes.front() == '"' && bytes.back() == '"') {
      bytes = bytes.substr(1, bytes.size() - 2);
    } else if (bytes.size() >= 3 && bytes[0] == 'L') {
      // Wide string: not fully handled; fall back to empty
      bytes = "";
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
      if (ts.array_rank > 0) {
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
      const auto& gv = mod_.globals[r.global->value];
      if (gv.type.spec.array_rank > 0) {
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
          emit_instr(ctx, result + " = getelementptr " + llvm_ty(elem_ts) +
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
          emit_instr(ctx, result + " = getelementptr " + llvm_ty(elem_ts) +
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
      case UnaryOp::ImagPart:
        throw std::runtime_error("HirEmitter: complex not yet supported");
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
    const std::string lv = emit_rval_id(ctx, b.lhs, lts);
    TypeSpec rts{};
    std::string rv = emit_rval_id(ctx, b.rhs, rts);

    // If result type is unannotated (void), use the operand type
    const TypeSpec& res_spec = (e.type.spec.base != TB_VOID || e.type.spec.ptr_level > 0)
                                   ? e.type.spec : lts;
    const std::string res_ty = llvm_ty(res_spec);
    const std::string op_ty  = llvm_ty(lts);
    const bool lf  = is_float_base(lts.base);
    const bool ls  = is_signed_int(lts.base);

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
        if (elem_ts.ptr_level > 0) elem_ts.ptr_level -= 1;
        else elem_ts.base = TB_CHAR;
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = getelementptr " + llvm_ty(elem_ts) +
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
        const int elem_sz = std::max(1, sizeof_ts(elem_ts));
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
        const char* instr = lf ? row.f : (ls ? row.i_s : row.i_u);
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
        TypeSpec cmp_res_spec{};
        cmp_res_spec.base = TB_INT;
        if (res_spec.base == TB_BOOL && res_spec.ptr_level == 0 && res_spec.array_rank == 0) {
          cmp_res_spec.base = TB_BOOL;
        }
        const std::string cmp_res_ty = llvm_ty(cmp_res_spec);
        const std::string cmp_tmp = fresh_tmp(ctx);
        if (lf) {
          emit_instr(ctx, cmp_tmp + " = fcmp " + row.f + " " + op_ty + " " + lv + ", " + rv);
        } else {
          const char* pred = ls ? row.is : row.iu;
          emit_instr(ctx, cmp_tmp + " = icmp " + pred + " " + op_ty + " " + lv + ", " + rv);
        }
        if (cmp_res_ty == "i1") return cmp_tmp;
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = zext i1 " + cmp_tmp + " to " + cmp_res_ty);
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
                        " [ " + rhs_val + ", %" + rhs_lbl + " ]," +
                        " [ " + skip_val + ", %" + skip_lbl + " ]");
    return tmp;
  }

  // ── Assign ────────────────────────────────────────────────────────────────

  std::string emit_rval_payload(FnCtx& ctx, const AssignExpr& a, const Expr& /*e*/) {
    TypeSpec rhs_ts{};
    std::string rhs = emit_rval_id(ctx, a.rhs, rhs_ts);

    TypeSpec lhs_ts{};
    const std::string lhs_ptr = emit_lval(ctx, a.lhs, lhs_ts);
    const std::string lty = llvm_ty(lhs_ts);

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
        const std::string result = fresh_tmp(ctx);
        emit_instr(ctx, result + " = getelementptr " + llvm_ty(elem_ts) +
                            ", ptr " + loaded + ", i64 " + delta);
        emit_instr(ctx, "store ptr " + result + ", ptr " + lhs_ptr);
        return result;
      }

      rhs = coerce(ctx, rhs, rhs_ts, lhs_ts);

      static const struct { AssignOp op; BinaryOp bop; } compound_map[] = {
        { AssignOp::Add, BinaryOp::Add }, { AssignOp::Sub, BinaryOp::Sub },
        { AssignOp::Mul, BinaryOp::Mul }, { AssignOp::Div, BinaryOp::Div },
        { AssignOp::Mod, BinaryOp::Mod }, { AssignOp::Shl, BinaryOp::Shl },
        { AssignOp::Shr, BinaryOp::Shr }, { AssignOp::BitAnd, BinaryOp::BitAnd },
        { AssignOp::BitOr, BinaryOp::BitOr }, { AssignOp::BitXor, BinaryOp::BitXor },
      };
      const char* instr = nullptr;
      for (const auto& row : compound_map) {
        if (row.op != a.op) continue;
        const bool lf = is_float_base(lhs_ts.base);
        const bool ls = is_signed_int(lhs_ts.base);
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
            instr = lf ? r.f : (ls ? r.is : r.iu);
            break;
          }
        }
        break;
      }
      if (!instr) throw std::runtime_error("HirEmitter: compound assign: unknown op");

      const std::string result = fresh_tmp(ctx);
      emit_instr(ctx, result + " = " + instr + " " + lty + " " + loaded + ", " + rhs);
      emit_instr(ctx, "store " + lty + " " + result + ", ptr " + lhs_ptr);
      return result;
    }

    rhs = coerce(ctx, rhs, rhs_ts, lhs_ts);
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
    TypeSpec ret_spec = e.type.spec;
    if (ret_spec.base == TB_VOID && ret_spec.ptr_level == 0 && ret_spec.array_rank == 0) {
      ret_spec = resolve_payload_type(ctx, call);
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
      if (fn_name == "__builtin_va_start" && call.args.size() >= 1) {
        TypeSpec ap_ts{};
        const std::string ap_ptr = emit_lval(ctx, call.args[0], ap_ts);
        need_llvm_va_start_ = true;
        emit_instr(ctx, "call void @llvm.va_start.p0(ptr " + ap_ptr + ")");
        return "";
      }
      if (fn_name == "__builtin_va_end" && call.args.size() >= 1) {
        TypeSpec ap_ts{};
        const std::string ap_ptr = emit_lval(ctx, call.args[0], ap_ts);
        need_llvm_va_end_ = true;
        emit_instr(ctx, "call void @llvm.va_end.p0(ptr " + ap_ptr + ")");
        return "";
      }
      if (fn_name == "__builtin_va_copy" && call.args.size() >= 2) {
        TypeSpec dst_ts{};
        TypeSpec src_ts{};
        const std::string dst_ptr = emit_lval(ctx, call.args[0], dst_ts);
        const std::string src_ptr = emit_lval(ctx, call.args[1], src_ts);
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
      // Unknown builtin: emit as 0/null
      if (ret_ty == "void") return "";
      return (ret_ty == "ptr") ? "null" : "0";
    }

    const Function* target_fn = nullptr;
    if (!fn_name.empty()) {
      const auto fit = mod_.fn_index.find(fn_name);
      if (fit != mod_.fn_index.end() && fit->second.value < mod_.functions.size()) {
        target_fn = &mod_.functions[fit->second.value];
      }
    }

    std::string args_str;
    for (size_t i = 0; i < call.args.size(); ++i) {
      TypeSpec arg_ts{};
      std::string arg = emit_rval_id(ctx, call.args[i], arg_ts);
      TypeSpec out_arg_ts = arg_ts;
      if (target_fn) {
        const bool has_void_param_list =
            target_fn->params.size() == 1 &&
            target_fn->params[0].type.spec.base == TB_VOID &&
            target_fn->params[0].type.spec.ptr_level == 0 &&
            target_fn->params[0].type.spec.array_rank == 0;
        if (!has_void_param_list && i < target_fn->params.size()) {
          out_arg_ts = target_fn->params[i].type.spec;
          arg = coerce(ctx, arg, arg_ts, out_arg_ts);
        }
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

  std::string emit_rval_payload(FnCtx& ctx, const VaArgExpr& v, const Expr& e) {
    TypeSpec ap_ts{};
    const std::string ap_ptr = emit_lval(ctx, v.ap, ap_ts);
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
    const bool is_gp_scalar =
        (res_ty == "ptr") ||
        (res_ts.ptr_level == 0 && res_ts.array_rank == 0 && is_any_int(res_ts.base));

    // AArch64 va_list matches clang's __va_list_tag layout:
    // { stack, gr_top, vr_top, gr_offs, vr_offs }.
    // For integer/pointer args we first try GR save area, then fall back to stack.
    if (is_gp_scalar) {
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
      emit_instr(ctx, next_offs + " = add i32 " + offs + ", 8");
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
      emit_instr(ctx, stack_next + " = getelementptr i8, ptr " + stack_ptr + ", i64 8");
      emit_instr(ctx, "store ptr " + stack_next + ", ptr " + stack_ptr_ptr);
      emit_term(ctx, "br label %" + join_lbl);

      emit_lbl(ctx, join_lbl);
      const std::string src_ptr = fresh_tmp(ctx);
      emit_instr(ctx, src_ptr + " = phi ptr [ " + reg_addr + ", %" + reg_lbl + " ], [ " +
                               stack_ptr + ", %" + stack_lbl + " ]");
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
    const std::string tmp = fresh_tmp(ctx);
    emit_instr(ctx, tmp + " = phi " + res_ty +
                        " [ " + then_v + ", %" + then_lbl + " ]," +
                        " [ " + else_v + ", %" + else_lbl + " ]");
    return tmp;
  }

  // ── Sizeof ────────────────────────────────────────────────────────────────

  std::string emit_rval_payload(FnCtx& ctx, const SizeofExpr& s, const Expr&) {
    const Expr& op = get_expr(s.expr);
    TypeSpec op_ts = op.type.spec;
    // DeclRef: get type from global/local declaration (NK_VAR nodes have no type set).
    if (const auto* r = std::get_if<DeclRef>(&op.payload)) {
      if (r->global) {
        if (const GlobalVar* gv = mod_.find_global(*r->global)) {
          op_ts = resolve_array_ts(gv->type.spec, gv->init);
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
      }
    }
    if (!has_concrete_type(op_ts)) return "8";
    return std::to_string(sizeof_ts(op_ts));
  }

  std::string emit_rval_payload(FnCtx& ctx, const SizeofTypeExpr& s, const Expr&) {
    return std::to_string(sizeof_ts(s.type.spec));
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
    const std::string gep = emit_member_lval(ctx, m, field_ts);
    // Use stored type if available, else use resolved field type
    const TypeSpec& load_ts = (e.type.spec.base != TB_VOID || e.type.spec.ptr_level > 0)
                                  ? e.type.spec
                                  : field_ts;
    // Array fields: decay to pointer-to-first-element (no load needed)
    if (load_ts.array_rank > 0 || field_ts.array_rank > 0) {
      const TypeSpec& arr_ts = (field_ts.array_rank > 0) ? field_ts : load_ts;
      const std::string arr_alloca_ty = llvm_alloca_ty(arr_ts);
      if (arr_alloca_ty == "ptr") {
        // Flexible-array members are represented as pointer fields in LLVM
        // layout. Decay to pointer by loading the field value directly.
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
    const std::string ty   = llvm_ty(d.type.spec);
    const std::string slot = ctx.local_slots.at(d.id.value);
    rhs = coerce(ctx, rhs, rhs_ts, d.type.spec);
    // Aggregate types can't use integer "0" — use zeroinitializer instead.
    const bool is_agg = (d.type.spec.base == TB_STRUCT || d.type.spec.base == TB_UNION) &&
                        d.type.spec.ptr_level == 0 && d.type.spec.array_rank == 0;
    if (is_agg && (rhs == "0" || rhs.empty())) rhs = "zeroinitializer";
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
    const std::string val = emit_rval_id(ctx, s.cond, ts);
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
          ctx.alloca_lines.push_back("  " + slot + " = alloca " + llvm_alloca_ty(d->type.spec));
        }
      }
    }
  }

  void emit_function(const Function& fn) {
    FnCtx ctx;
    ctx.fn = &fn;

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
        if (fn.return_type.spec.base == TB_VOID) {
          emit_term(ctx, "ret void");
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
