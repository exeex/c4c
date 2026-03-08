#include "hir_to_llvm.hpp"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "ir_builder.hpp"

// Phase 3 bootstrap: delegate to legacy IRBuilder.
// emit_module_native() is the growing native path.

namespace tinyc2ll::codegen::llvm_backend {

std::string emit_module(const Module& /*mod*/, const Node* ast_root) {
  tinyc2ll::frontend_cxx::IRBuilder builder;
  return builder.emit_program(const_cast<tinyc2ll::frontend_cxx::Node*>(ast_root));
}

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

// Non-decayed LLVM type for alloca/struct field (struct/union → %struct.TAG)
static std::string llvm_alloca_ty(const TypeSpec& ts) {
  if (ts.ptr_level > 0 || ts.is_fn_ptr) return "ptr";
  if (ts.array_rank > 0) {
    // Array: [N x elem]
    if (ts.array_size >= 0) {
      return "[" + std::to_string(ts.array_size) + " x " + llvm_base(ts.base) + "]";
    }
    return "ptr";  // unknown size
  }
  if (ts.base == TB_STRUCT || ts.base == TB_UNION) {
    if (ts.tag && ts.tag[0]) return "%struct." + std::string(ts.tag);
  }
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
    default:                                   return 4;
  }
}

static int sizeof_ts(const TypeSpec& ts) {
  if (ts.ptr_level > 0 || ts.array_rank > 0 || ts.is_fn_ptr) return 8;
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
    for (const auto& gv : mod_.globals) emit_global(gv);
    for (const auto& fn : mod_.functions) emit_function(fn);

    std::ostringstream out;
    out << preamble_.str();
    if (!preamble_.str().empty()) out << "\n";
    for (const auto& b : fn_bodies_) out << b;
    return out.str();
  }

 private:
  const Module& mod_;
  std::ostringstream preamble_;
  std::vector<std::string> fn_bodies_;
  std::unordered_map<std::string, std::string> str_pool_;
  int str_idx_ = 0;

  // ── Per-function state ────────────────────────────────────────────────────

  struct FnCtx {
    const Function* fn = nullptr;
    int tmp_idx = 0;
    bool last_term = false;
    // local_id.value → alloca slot (e.g. "%lv.x")
    std::unordered_map<uint32_t, std::string> local_slots;
    // local_id.value → C TypeSpec
    std::unordered_map<uint32_t, TypeSpec> local_types;
    // param_index → SSA name (e.g. "%p.x")
    std::unordered_map<uint32_t, std::string> param_slots;
    std::vector<std::string> alloca_lines;
    std::vector<std::string> body_lines;
    // break/continue targets per block_id
    std::unordered_map<uint32_t, BlockMeta> block_meta;
    // user label name → LLVM label
    std::unordered_map<std::string, std::string> user_labels;
    // current break/continue stacks (filled when entering body/switch blocks)
    std::vector<std::string> break_stack;
    std::vector<std::string> continue_stack;
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
    return bytes;
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

  std::string emit_const_scalar_expr(ExprId id, const TypeSpec& expected_ts) {
    const Expr& e = get_expr(id);
    return std::visit([&](const auto& p) -> std::string {
      using T = std::decay_t<decltype(p)>;
      if constexpr (std::is_same_v<T, IntLiteral>) {
        if (llvm_ty(expected_ts) == "ptr") {
          if (p.value == 0) return "null";
          return "inttoptr (i64 " + std::to_string(p.value) + " to ptr)";
        }
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
        if (!item.index_designator) next_idx = idx + 1;
      }
    }

    std::string out = "[";
    const std::string ety = llvm_ty(elem_ts);
    for (size_t i = 0; i < elems.size(); ++i) {
      if (i) out += ", ";
      out += ety + " " + elems[i];
    }
    out += "]";
    return out;
  }

  std::string emit_const_struct(const TypeSpec& ts, const GlobalInit& init) {
    if (!ts.tag || !ts.tag[0]) return "zeroinitializer";
    const auto it = mod_.struct_defs.find(ts.tag);
    if (it == mod_.struct_defs.end()) return "zeroinitializer";
    const HirStructDef& sd = it->second;
    if (sd.is_union) return "zeroinitializer";

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
        field_vals[idx] = emit_const_init(field_decl_type(sd.fields[idx]), child_init);
        if (!item.field_designator && !item.index_designator) next_idx = idx + 1;
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
    if (ts.base == TB_STRUCT || ts.base == TB_UNION) return emit_const_struct(ts, init);
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

  void emit_global(const GlobalVar& gv) {
    const std::string ty = llvm_alloca_ty(gv.type.spec);
    if (gv.linkage.is_extern) {
      preamble_ << "@" << gv.name << " = external global " << ty << "\n";
      return;
    }
    const std::string lk = gv.linkage.is_static ? "internal " : "";
    const std::string qual = gv.is_const ? "constant " : "global ";
    const std::string init = emit_const_init(gv.type.spec, gv.init);
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
    if (is_any_int(from_ts.base) && is_any_int(to_ts.base)) {
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
        out_field_ts = f.elem_type;
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
      if (r->global) {
        pts = mod_.globals[r->global->value].type.spec;
        return "@" + r->name;
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
      // Element type
      pts = base_ts;
      if (pts.ptr_level > 0) {
        pts.ptr_level--;
      } else if (pts.array_rank > 0) {
        pts.array_rank--;
        pts.array_size = (pts.array_rank > 0) ? pts.array_dims[1] : -1;
      }
      const std::string tmp = fresh_tmp(ctx);
      emit_instr(ctx, tmp + " = getelementptr " + llvm_ty(pts) +
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
    // For arithmetic/bitwise the result type matches the operand type
    const TypeSpec lts = resolve_expr_type(ctx, b.lhs);
    if (lts.base != TB_VOID) return lts;
    return resolve_expr_type(ctx, b.rhs);
  }

  TypeSpec resolve_payload_type(FnCtx& ctx, const UnaryExpr& u) {
    return resolve_expr_type(ctx, u.operand);
  }

  TypeSpec resolve_payload_type(FnCtx& ctx, const AssignExpr& a) {
    return resolve_expr_type(ctx, a.lhs);
  }

  TypeSpec resolve_payload_type(FnCtx& ctx, const CastExpr& c) {
    return c.to_type.spec;
  }

  TypeSpec resolve_payload_type(FnCtx& ctx, const CallExpr& c) {
    const Expr& callee_e = get_expr(c.callee);
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

  template <typename T>
  TypeSpec resolve_payload_type(FnCtx&, const T&) { return {}; }

  std::string emit_rval_id(FnCtx& ctx, ExprId id, TypeSpec& out_ts) {
    const Expr& e = get_expr(id);
    out_ts = e.type.spec;
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
    const std::string gname = intern_str(bytes);
    const size_t len = bytes.size() + 1;
    const std::string tmp = fresh_tmp(ctx);
    emit_instr(ctx, tmp + " = getelementptr [" + std::to_string(len) +
                        " x i8], ptr " + gname + ", i64 0, i64 0");
    return tmp;
  }

  // ── DeclRef ───────────────────────────────────────────────────────────────

  std::string emit_rval_payload(FnCtx& ctx, const DeclRef& r, const Expr& e) {
    // Function reference: return as ptr value
    if (mod_.fn_index.count(r.name)) return "@" + r.name;

    // Param: SSA value
    if (r.param_index) {
      const auto it = ctx.param_slots.find(*r.param_index);
      if (it != ctx.param_slots.end()) return it->second;
    }

    // Local: load
    if (r.local) {
      const auto it = ctx.local_slots.find(r.local->value);
      if (it == ctx.local_slots.end())
        throw std::runtime_error("HirEmitter: local slot not found: " + r.name);
      const TypeSpec ts = ctx.local_types.at(r.local->value);
      const std::string ty = llvm_ty(ts);
      const std::string tmp = fresh_tmp(ctx);
      emit_instr(ctx, tmp + " = load " + ty + ", ptr " + it->second);
      return tmp;
    }

    // Global: load
    if (r.global) {
      const auto& gv = mod_.globals[r.global->value];
      const std::string ty = llvm_ty(gv.type.spec);
      const std::string tmp = fresh_tmp(ctx);
      emit_instr(ctx, tmp + " = load " + ty + ", ptr @" + r.name);
      return tmp;
    }

    // Unresolved: load from external global
    const std::string ty = llvm_ty(e.type.spec);
    const std::string tmp = fresh_tmp(ctx);
    emit_instr(ctx, tmp + " = load " + ty + ", ptr @" + r.name);
    return tmp;
  }

  // ── Unary ─────────────────────────────────────────────────────────────────

  std::string emit_rval_payload(FnCtx& ctx, const UnaryExpr& u, const Expr& e) {
    const std::string ty = llvm_ty(e.type.spec);
    TypeSpec op_ts{};
    const std::string val = emit_rval_id(ctx, u.operand, op_ts);
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
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = load " + ty + ", ptr " + val);
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
        if (is_float_base(pts.base)) {
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
        if (is_float_base(pts.base)) {
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
        const std::string cmp_tmp = fresh_tmp(ctx);
        if (lf) {
          emit_instr(ctx, cmp_tmp + " = fcmp " + row.f + " " + op_ty + " " + lv + ", " + rv);
        } else {
          const char* pred = ls ? row.is : row.iu;
          emit_instr(ctx, cmp_tmp + " = icmp " + pred + " " + op_ty + " " + lv + ", " + rv);
        }
        if (res_ty == "i1") return cmp_tmp;
        const std::string tmp = fresh_tmp(ctx);
        emit_instr(ctx, tmp + " = zext i1 " + cmp_tmp + " to " + res_ty);
        return tmp;
      }
    }

    throw std::runtime_error("HirEmitter: unhandled binary op");
  }

  std::string emit_logical(FnCtx& ctx, const BinaryExpr& b, const Expr& e) {
    const std::string res_ty = llvm_ty(e.type.spec);
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
    const std::string rext = fresh_tmp(ctx);
    emit_instr(ctx, rext + " = zext i1 " + rc + " to " + res_ty);
    emit_term(ctx, "br label %" + end_lbl);

    emit_lbl(ctx, skip_lbl);
    const std::string skip_val = (b.op == BinaryOp::LAnd) ? "0" : "1";
    emit_term(ctx, "br label %" + end_lbl);

    emit_lbl(ctx, end_lbl);
    const std::string tmp = fresh_tmp(ctx);
    emit_instr(ctx, tmp + " = phi " + res_ty +
                        " [ " + rext + ", %" + rhs_lbl + " ]," +
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
    const std::string callee_val = emit_rval_id(ctx, call.callee, callee_ts);
    TypeSpec ret_spec = e.type.spec;
    if (ret_spec.base == TB_VOID && ret_spec.ptr_level == 0 && ret_spec.array_rank == 0) {
      ret_spec = resolve_payload_type(ctx, call);
    }
    const std::string ret_ty = llvm_ty(ret_spec);

    // Look up function signature for argument type coercion
    const Expr& callee_e = get_expr(call.callee);
    std::string fn_name;
    if (const auto* r = std::get_if<DeclRef>(&callee_e.payload)) {
      fn_name = r->name;
    }

    std::string args_str;
    for (size_t i = 0; i < call.args.size(); ++i) {
      TypeSpec arg_ts{};
      std::string arg = emit_rval_id(ctx, call.args[i], arg_ts);
      if (i) args_str += ", ";
      args_str += llvm_ty(arg_ts) + " " + arg;
    }

    if (ret_ty == "void") {
      emit_instr(ctx, "call void " + callee_val + "(" + args_str + ")");
      return "";
    }
    const std::string tmp = fresh_tmp(ctx);
    emit_instr(ctx, tmp + " = call " + ret_ty + " " + callee_val + "(" + args_str + ")");
    return tmp;
  }

  // ── Ternary ───────────────────────────────────────────────────────────────

  std::string emit_rval_payload(FnCtx& ctx, const TernaryExpr& t, const Expr& e) {
    TypeSpec cond_ts{};
    const std::string cond_v = emit_rval_id(ctx, t.cond, cond_ts);
    const std::string cond_i1 = to_bool(ctx, cond_v, cond_ts);

    const std::string then_lbl = fresh_lbl(ctx, "tern.then.");
    const std::string else_lbl = fresh_lbl(ctx, "tern.else.");
    const std::string end_lbl  = fresh_lbl(ctx, "tern.end.");
    const std::string res_ty   = llvm_ty(e.type.spec);

    emit_term(ctx, "br i1 " + cond_i1 + ", label %" + then_lbl + ", label %" + else_lbl);

    emit_lbl(ctx, then_lbl);
    TypeSpec then_ts{};
    std::string then_v = emit_rval_id(ctx, t.then_expr, then_ts);
    then_v = coerce(ctx, then_v, then_ts, e.type.spec);
    emit_term(ctx, "br label %" + end_lbl);

    emit_lbl(ctx, else_lbl);
    TypeSpec else_ts{};
    std::string else_v = emit_rval_id(ctx, t.else_expr, else_ts);
    else_v = coerce(ctx, else_v, else_ts, e.type.spec);
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

  std::string emit_rval_payload(FnCtx&, const SizeofExpr& /*s*/, const Expr& e) {
    return std::to_string(sizeof_ts(e.type.spec));
  }

  std::string emit_rval_payload(FnCtx&, const SizeofTypeExpr& s, const Expr&) {
    return std::to_string(sizeof_ts(s.type.spec));
  }

  // ── IndexExpr (rval: load through computed ptr) ──────────────────────────

  std::string emit_rval_payload(FnCtx& ctx, const IndexExpr&, const Expr& e) {
    TypeSpec pts{};
    const std::string ptr = emit_lval_dispatch(ctx, e, pts);
    const std::string ty  = llvm_ty(e.type.spec);
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
    if (!d.init) return;
    TypeSpec rhs_ts{};
    std::string rhs = emit_rval_id(ctx, *d.init, rhs_ts);
    const std::string ty   = llvm_ty(d.type.spec);
    const std::string slot = ctx.local_slots.at(d.id.value);
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

    // Push break/continue into meta for body block
    ctx.block_meta[s.body_block.value].break_label    = end_lbl;
    ctx.block_meta[s.body_block.value].continue_label = cond_lbl;

    emit_term(ctx, "br label %" + cond_lbl);
    // cond block is emitted separately; record that cond block holds the branch
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
    const std::string cont_lbl = s.continue_target
        ? block_lbl(*s.continue_target)
        : body_lbl;

    ctx.block_meta[s.body_block.value].break_label    = end_lbl;
    ctx.block_meta[s.body_block.value].continue_label = cont_lbl;

    const std::string cond_lbl = fresh_lbl(ctx, "for.cond.");
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
  }

  void emit_stmt_impl(FnCtx& ctx, const DoWhileStmt& s) {
    const std::string body_lbl = block_lbl(s.body_block);
    const std::string end_lbl  = s.break_target
        ? block_lbl(*s.break_target)
        : fresh_lbl(ctx, "dowhile.end.");
    const std::string cont_lbl = s.continue_target
        ? block_lbl(*s.continue_target)
        : body_lbl;

    ctx.block_meta[s.body_block.value].break_label    = end_lbl;
    ctx.block_meta[s.body_block.value].continue_label = cont_lbl;

    emit_term(ctx, "br label %" + body_lbl);
  }

  void emit_stmt_impl(FnCtx& ctx, const SwitchStmt& s) {
    TypeSpec ts{};
    const std::string val = emit_rval_id(ctx, s.cond, ts);
    const std::string ty  = llvm_ty(ts);
    const std::string end_lbl = s.default_block
        ? block_lbl(*s.default_block)
        : fresh_lbl(ctx, "sw.end.");

    ctx.block_meta[s.body_block.value].break_label = end_lbl;

    // Collect case values by scanning body block statements
    const Block* body_blk = nullptr;
    for (const auto& blk : ctx.fn->blocks) {
      if (blk.id.value == s.body_block.value) { body_blk = &blk; break; }
    }

    std::string sw = "switch " + ty + " " + val + ", label %" + end_lbl + " [\n";
    if (body_blk) {
      for (const auto& stmt : body_blk->stmts) {
        if (const auto* cs = std::get_if<CaseStmt>(&stmt.payload)) {
          sw += "    " + ty + " " + std::to_string(cs->value) +
                ", label %" + end_lbl + "\n";
        }
      }
    }
    sw += "  ]";
    emit_term(ctx, sw);
  }

  void emit_stmt_impl(FnCtx& ctx, const GotoStmt& s) {
    emit_term(ctx, "br label %ulbl_" + s.target.user_name);
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

  void emit_stmt_impl(FnCtx& ctx, const BreakStmt&) {
    if (!ctx.break_stack.empty()) {
      emit_term(ctx, "br label %" + ctx.break_stack.back());
    }
  }

  void emit_stmt_impl(FnCtx& ctx, const ContinueStmt&) {
    if (!ctx.continue_stack.empty()) {
      emit_term(ctx, "br label %" + ctx.continue_stack.back());
    }
  }

  void emit_stmt_impl(FnCtx&, const CaseStmt&)      {}
  void emit_stmt_impl(FnCtx&, const CaseRangeStmt&) {}
  void emit_stmt_impl(FnCtx&, const DefaultStmt&)   {}

  // ── Function emission ─────────────────────────────────────────────────────

  void hoist_allocas(FnCtx& ctx, const Function& fn) {
    std::unordered_map<std::string, int> name_count;
    for (const auto& blk : fn.blocks) {
      for (const auto& stmt : blk.stmts) {
        const auto* d = std::get_if<LocalDecl>(&stmt.payload);
        if (!d) continue;
        if (ctx.local_slots.count(d->id.value)) continue;
        // Dedup slot names for shadowing locals
        const int cnt = name_count[d->name]++;
        const std::string slot = cnt == 0
            ? "%lv." + d->name
            : "%lv." + d->name + "." + std::to_string(cnt);
        ctx.local_slots[d->id.value] = slot;
        ctx.local_types[d->id.value] = d->type.spec;
        ctx.alloca_lines.push_back("  " + slot + " = alloca " + llvm_alloca_ty(d->type.spec));
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

    // Signature
    out << "define " << ret_ty << " @" << fn.name << "(";
    for (size_t i = 0; i < fn.params.size(); ++i) {
      if (void_param_list) break;
      if (i) out << ", ";
      const std::string pty = llvm_ty(fn.params[i].type.spec);
      const std::string pname = "%p." + fn.params[i].name;
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

      // Emit block label (except for entry which uses "entry:")
      if (bi > 0) {
        emit_lbl(ctx, block_lbl(blk->id));
      }

      // Push break/continue from meta (set by parent loop/switch stmt)
      const auto it = ctx.block_meta.find(blk->id.value);
      if (it != ctx.block_meta.end()) {
        if (it->second.break_label)
          ctx.break_stack.push_back(*it->second.break_label);
        if (it->second.continue_label)
          ctx.continue_stack.push_back(*it->second.continue_label);
      }

      // Emit statements
      for (const auto& stmt : blk->stmts) {
        emit_stmt(ctx, stmt);
      }

      // Pop break/continue
      if (it != ctx.block_meta.end()) {
        if (it->second.continue_label) ctx.continue_stack.pop_back();
        if (it->second.break_label)    ctx.break_stack.pop_back();
      }

      // If block has no explicit terminator, fall through to next block
      if (!ctx.last_term && bi + 1 < ordered.size()) {
        emit_term(ctx, "br label %" + block_lbl(ordered[bi + 1]->id));
      }

      // If completely no terminator and no next block
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
