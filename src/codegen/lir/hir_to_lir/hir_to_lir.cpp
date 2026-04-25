#include "hir_to_lir.hpp"
#include "call_args_ops.hpp"
#include "hir_ir.hpp"
#include "lowering.hpp"
#include "../../llvm/calling_convention.hpp"
#include "../../shared/llvm_helpers.hpp"

#include <algorithm>
#include <functional>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace c4c::codegen::lir {

namespace llvm_cc = c4c::codegen::llvm_backend;

namespace {

int align_to_bytes(int value, int align) {
  if (align <= 1) return value;
  const int rem = value % align;
  return rem == 0 ? value : value + (align - rem);
}

std::string emitted_link_name(const c4c::hir::Module& mod, c4c::LinkNameId id,
                              std::string_view fallback) {
  const std::string_view resolved = mod.link_names.spelling(id);
  return resolved.empty() ? std::string(fallback) : std::string(resolved);
}

}  // namespace

// ── Module-level orchestration helpers ───────────────────────────────────────

int object_align_bytes(const c4c::hir::Module& mod, const TypeSpec& ts) {
  if (ts.array_rank > 0) {
    TypeSpec elem = ts;
    elem.array_rank--;
    if (elem.array_rank > 0) {
      for (int i = 0; i < elem.array_rank; ++i) elem.array_dims[i] = elem.array_dims[i + 1];
    }
    elem.array_size = (elem.array_rank > 0) ? elem.array_dims[0] : -1;
    int align = object_align_bytes(mod, elem);
    if (ts.align_bytes > align) align = ts.align_bytes;
    return align;
  }
  int align = 1;
  if (ts.is_vector && ts.vector_bytes > 0) {
    align = static_cast<int>(ts.vector_bytes);
  } else if (ts.ptr_level > 0 || ts.is_fn_ptr) {
    align = 8;
  } else if ((ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.tag && ts.tag[0]) {
    const auto it = mod.struct_defs.find(ts.tag);
    align = (it != mod.struct_defs.end()) ? std::max(1, it->second.align_bytes) : 8;
  } else if (ts.base == TB_VA_LIST && ts.ptr_level == 0 && ts.array_rank == 0) {
    align = llvm_va_list_alignment(mod.target_profile);
  } else {
    switch (ts.base) {
      case TB_BOOL: case TB_CHAR: case TB_SCHAR: case TB_UCHAR: align = 1; break;
      case TB_SHORT: case TB_USHORT: align = 2; break;
      case TB_INT: case TB_UINT: case TB_FLOAT: case TB_ENUM: align = 4; break;
      case TB_LONG: case TB_ULONG:
      case TB_LONGLONG: case TB_ULONGLONG:
      case TB_DOUBLE: align = 8; break;
      case TB_LONGDOUBLE:
      case TB_INT128: case TB_UINT128: align = 16; break;
      default: align = 8; break;
    }
  }
  if (ts.align_bytes > align) align = ts.align_bytes;
  return align;
}

std::vector<size_t> dedup_globals(const c4c::hir::Module& mod) {
  std::unordered_map<LinkNameId, size_t> best_by_link_name;
  std::unordered_map<TextId, size_t> best_by_name_text;
  std::unordered_map<std::string, size_t> best_by_name; // fallback when no stable ids exist
  for (size_t i = 0; i < mod.globals.size(); ++i) {
    const auto& gv = mod.globals[i];
    auto update_best = [&](size_t& best_index) {
      const bool cur_has_init =
          !std::holds_alternative<std::monostate>(mod.globals[best_index].init);
      const bool new_has_init = !std::holds_alternative<std::monostate>(gv.init);
      if (new_has_init || !cur_has_init) best_index = i;
    };

    if (gv.link_name_id != kInvalidLinkName) {
      auto [it, inserted] = best_by_link_name.emplace(gv.link_name_id, i);
      if (!inserted) update_best(it->second);
      continue;
    }

    if (gv.name_text_id != kInvalidText) {
      auto [it, inserted] = best_by_name_text.emplace(gv.name_text_id, i);
      if (!inserted) update_best(it->second);
      continue;
    }

    auto [it, inserted] = best_by_name.emplace(gv.name, i);
    if (!inserted) update_best(it->second);
  }
  // Collect in original order
  std::vector<size_t> result;
  result.reserve(best_by_link_name.size() + best_by_name_text.size() + best_by_name.size());
  for (size_t i = 0; i < mod.globals.size(); ++i) {
    const auto& gv = mod.globals[i];
    if (gv.link_name_id != kInvalidLinkName) {
      auto it = best_by_link_name.find(gv.link_name_id);
      if (it != best_by_link_name.end() && it->second == i) result.push_back(i);
      continue;
    }
    if (gv.name_text_id != kInvalidText) {
      auto it = best_by_name_text.find(gv.name_text_id);
      if (it != best_by_name_text.end() && it->second == i) result.push_back(i);
      continue;
    }
    auto it = best_by_name.find(gv.name);
    if (it != best_by_name.end() && it->second == i) result.push_back(i);
  }
  return result;
}

std::vector<size_t> dedup_functions(const c4c::hir::Module& mod) {
  std::unordered_map<LinkNameId, size_t> best_by_link_name;
  std::unordered_map<TextId, size_t> best_by_name_text;
  std::unordered_map<std::string, size_t> best_by_name; // fallback when no stable ids exist
  for (size_t i = 0; i < mod.functions.size(); ++i) {
    const auto& fn = mod.functions[i];
    if (!fn.materialized) continue;
    auto update_best = [&](size_t& best_index) {
      const bool cur_is_def = !mod.functions[best_index].blocks.empty();
      const bool new_is_def = !fn.blocks.empty();
      if (new_is_def && !cur_is_def) best_index = i;
    };

    if (fn.link_name_id != kInvalidLinkName) {
      auto [it, inserted] = best_by_link_name.emplace(fn.link_name_id, i);
      if (!inserted) update_best(it->second);
      continue;
    }

    if (fn.name_text_id != kInvalidText) {
      auto [it, inserted] = best_by_name_text.emplace(fn.name_text_id, i);
      if (!inserted) update_best(it->second);
      continue;
    }

    auto [it, inserted] = best_by_name.emplace(fn.name, i);
    if (!inserted) update_best(it->second);
  }
  // Collect in original order
  std::vector<size_t> result;
  result.reserve(best_by_link_name.size() + best_by_name_text.size() + best_by_name.size());
  for (size_t i = 0; i < mod.functions.size(); ++i) {
    const auto& fn = mod.functions[i];
    if (!fn.materialized) continue;
    if (fn.link_name_id != kInvalidLinkName) {
      auto it = best_by_link_name.find(fn.link_name_id);
      if (it != best_by_link_name.end() && it->second == i) result.push_back(i);
      continue;
    }
    if (fn.name_text_id != kInvalidText) {
      auto it = best_by_name_text.find(fn.name_text_id);
      if (it != best_by_name_text.end() && it->second == i) result.push_back(i);
      continue;
    }
    auto it = best_by_name.find(fn.name);
    if (it != best_by_name.end() && it->second == i) result.push_back(i);
  }
  return result;
}

// ── Global variable lowering ─────────────────────────────────────────────────
// Semantic decisions (linkage, alignment, qualifier, type, init) are computed
// here in lowering.  The printer assembles LLVM text from the structured fields.

static void lower_global(const c4c::hir::GlobalVar& gv,
                          const c4c::hir::Module& mod,
                          ConstInitEmitter& const_init,
                          LirModule& module) {
  using namespace c4c::codegen::llvm_helpers;

  const TypeSpec& ts = gv.type.spec;
  const int align = object_align_bytes(mod, ts);

  auto make_linkage_vis = [&](bool is_static, bool is_weak, bool is_extern,
                              Visibility vis) -> std::string {
    if (is_extern) {
      std::string s = is_weak ? "extern_weak " : "external ";
      s += llvm_visibility(vis);
      return s;
    }
    std::string s = is_static ? "internal " : "";
    if (is_weak && !is_static) s = "weak ";
    s += llvm_visibility(vis);
    return s;
  };

  // Check for flexible array member struct — needs a custom literal type/init.
  if (!gv.linkage.is_extern &&
      ts.ptr_level == 0 && ts.array_rank == 0 &&
      ts.tag && ts.tag[0] && ts.base == TB_STRUCT) {
    const auto it = mod.struct_defs.find(ts.tag);
    if (it != mod.struct_defs.end()) {
      const auto& sd = it->second;
      if (!sd.is_union && !sd.fields.empty() && sd.fields.back().is_flexible_array) {
        std::vector<TypeSpec> field_types;
        const auto field_vals = const_init.emit_const_struct_fields(ts, sd, gv.init, &field_types);
        const TypeSpec& last_ts = field_types.back();
        if (last_ts.array_rank > 0 && last_ts.array_size > 0) {
          std::string literal_ty = "{ ";
          std::string literal_init = "{ ";
          for (size_t i = 0; i < sd.fields.size(); ++i) {
            if (i) { literal_ty += ", "; literal_init += ", "; }
            literal_ty += llvm_alloca_ty(field_types[i]);
            literal_init += llvm_alloca_ty(field_types[i]) + " " + field_vals[i];
          }
          literal_ty += " }";
          literal_init += " }";

          LirGlobal lg;
          lg.name = gv.name;
          lg.link_name_id = gv.link_name_id;
          lg.type = ts;
          lg.is_internal = gv.linkage.is_static;
          lg.is_const = gv.is_const;
          lg.linkage_vis = make_linkage_vis(gv.linkage.is_static, gv.linkage.is_weak,
                                            false, gv.linkage.visibility);
          lg.qualifier = (gv.is_const && ts.ptr_level == 0) ? "constant " : "global ";
          lg.llvm_type = literal_ty;
          lg.init_text = literal_init;
          lg.align_bytes = align;
          lg.is_extern_decl = false;
          module.globals.push_back(std::move(lg));
          return;
        }
      }
    }
  }

  LirGlobal lg;
  lg.name = gv.name;
  lg.link_name_id = gv.link_name_id;
  lg.type = ts;
  lg.is_internal = gv.linkage.is_static;
  lg.is_const = gv.is_const;
  lg.llvm_type = llvm_alloca_ty(ts);
  lg.align_bytes = align;

  if (gv.linkage.is_extern) {
    lg.linkage_vis = make_linkage_vis(false, gv.linkage.is_weak, true, gv.linkage.visibility);
    lg.qualifier = "global ";
    lg.is_extern_decl = true;
    // init_text left empty for extern declarations
  } else {
    lg.linkage_vis = make_linkage_vis(gv.linkage.is_static, gv.linkage.is_weak,
                                      false, gv.linkage.visibility);
    lg.qualifier = (gv.is_const && ts.ptr_level == 0) ? "constant " : "global ";
    lg.init_text = const_init.emit_const_init(ts, gv.init);
    lg.is_extern_decl = false;
  }

  module.globals.push_back(std::move(lg));
}

static void lower_globals(const std::vector<size_t>& global_indices,
                           const c4c::hir::Module& mod,
                           ConstInitEmitter& const_init,
                           LirModule& module) {
  for (size_t idx : global_indices)
    lower_global(mod.globals[idx], mod, const_init, module);
}

// ── Type declarations ────────────────────────────────────────────────────────

std::vector<std::string> build_type_decls(const c4c::hir::Module& mod) {
  using namespace c4c::codegen::llvm_helpers;
  std::vector<std::string> decls;

  if (!llvm_va_list_is_pointer_object(mod.target_profile)) {
    decls.push_back(llvm_va_list_struct_decl(mod.target_profile));
  }

  for (const auto& tag : mod.struct_def_order) {
    const auto it = mod.struct_defs.find(tag);
    if (it == mod.struct_defs.end()) continue;
    const auto& sd = it->second;
    const std::string sty = llvm_struct_type_str(tag);

    if (sd.fields.empty() && sd.base_tags.empty()) {
      if (sd.size_bytes == 0) {
        decls.push_back(sty + " = type {}");
      } else {
        decls.push_back(sty + " = type { [" +
                         std::to_string(sd.size_bytes) + " x i8] }");
      }
      continue;
    }
    if (sd.is_union) {
      decls.push_back(sty + " = type { [" +
                       std::to_string(sd.size_bytes) + " x i8] }");
    } else {
      std::ostringstream line;
      line << sty << " = type { ";
      bool first = true;
      int cur_offset = 0;
      for (const auto& base_tag : sd.base_tags) {
        const auto bit = mod.struct_defs.find(base_tag);
        if (bit == mod.struct_defs.end()) continue;
        const auto& base = bit->second;
        const int base_align = std::max(1, base.align_bytes);
        const int base_offset = align_to_bytes(cur_offset, base_align);
        if (base_offset > cur_offset) {
          if (!first) line << ", ";
          first = false;
          line << "[" << (base_offset - cur_offset) << " x i8]";
          cur_offset = base_offset;
        }
        if (!first) line << ", ";
        first = false;
        TypeSpec base_ts{};
        base_ts.base = TB_STRUCT;
        base_ts.tag = base.tag.c_str();
        line << llvm_ty(base_ts);
        cur_offset = base_offset + std::max(0, base.size_bytes);
      }
      int last_idx = -1;
      for (const auto& f : sd.fields) {
        if (f.llvm_idx == last_idx) continue;
        last_idx = f.llvm_idx;
        if (f.offset_bytes > cur_offset) {
          if (!first) line << ", ";
          first = false;
          line << "[" << (f.offset_bytes - cur_offset) << " x i8]";
          cur_offset = f.offset_bytes;
        }
        if (!first) line << ", ";
        first = false;
        line << llvm_field_ty(f);
        cur_offset = f.offset_bytes + std::max(0, f.size_bytes);
      }
      if (sd.size_bytes > cur_offset) {
        if (!first) line << ", ";
        line << "[" << (sd.size_bytes - cur_offset) << " x i8]";
      }
      line << " }";
      decls.push_back(line.str());
    }
  }
  return decls;
}

// ── Function signature building ───────────────────────────────────────────────
// Builds the LLVM IR signature text for a HIR function.  Ownership of this
// logic belongs to hir_to_lir; StmtEmitter consumes the pre-built text.

std::string build_fn_signature(const c4c::hir::Module& mod,
                               const c4c::hir::Function& fn) {
  using namespace c4c::codegen::llvm_helpers;

  std::ostringstream sig_out;
  const std::string ret_ty = llvm_ret_ty(fn.return_type.spec);
  const std::string emitted_name = emitted_link_name(mod, fn.link_name_id, fn.name);

  const bool void_param_list =
      fn.params.size() == 1 &&
      fn.params[0].type.spec.base == TB_VOID &&
      fn.params[0].type.spec.ptr_level == 0 &&
      fn.params[0].type.spec.array_rank == 0;

  // Declaration (extern with no body)
  if (fn.linkage.is_extern && fn.blocks.empty()) {
    const std::string decl_kw = fn.linkage.is_weak ? "declare extern_weak " : "declare ";
    sig_out << decl_kw << llvm_visibility(fn.linkage.visibility) << ret_ty << " "
            << llvm_global_sym(emitted_name) << "(";
    for (size_t i = 0; i < fn.params.size(); ++i) {
      if (void_param_list) break;
      if (i) sig_out << ", ";
      const TypeSpec& param_ts = fn.params[i].type.spec;
      if (llvm_target_is_amd64_sysv(mod.target_profile) &&
          llvm_cc::amd64_fixed_aggregate_passed_byval(param_ts, mod)) {
        sig_out << "ptr byval(" << llvm_ty(param_ts) << ") align "
                << std::max(8, object_align_bytes(mod, param_ts));
      } else {
        sig_out << llvm_ty(param_ts);
      }
    }
    if (fn.attrs.variadic) {
      if (!fn.params.empty() && !void_param_list) sig_out << ", ";
      sig_out << "...";
    }
    sig_out << ")\n\n";
    return sig_out.str();
  }

  // Definition — template comments
  if (!fn.template_origin.empty()) {
    sig_out << "; template-origin: " << fn.template_origin << "\n";
    if (!fn.spec_key.empty()) {
      sig_out << "; spec-key: " << fn.spec_key.canonical << "\n";
    }
  }

  // Definition — linkage + visibility + return type + name
  std::string fn_lk = fn.linkage.is_static ? "internal " : "";
  if (fn.linkage.is_weak && !fn.linkage.is_static) fn_lk = "weak ";
  sig_out << "define " << fn_lk << llvm_visibility(fn.linkage.visibility) << ret_ty << " "
          << llvm_global_sym(emitted_name) << "(";

  // Parameters
  for (size_t i = 0; i < fn.params.size(); ++i) {
    if (void_param_list) break;
    if (i) sig_out << ", ";
    const TypeSpec& param_ts = fn.params[i].type.spec;
    const std::string pname = "%p." + sanitize_llvm_ident(fn.params[i].name);
    if (llvm_target_is_amd64_sysv(mod.target_profile) &&
        llvm_cc::amd64_fixed_aggregate_passed_byval(param_ts, mod)) {
      sig_out << "ptr byval(" << llvm_ty(param_ts) << ") align "
              << std::max(8, object_align_bytes(mod, param_ts)) << " " << pname;
    } else {
      sig_out << llvm_ty(param_ts) << " " << pname;
    }
  }
  if (fn.attrs.variadic) {
    if (!fn.params.empty()) sig_out << ", ";
    sig_out << "...";
  }
  sig_out << ")";

  // Function attributes
  if (fn.attrs.no_inline) sig_out << " noinline";
  if (fn.attrs.always_inline) sig_out << " alwaysinline";
  sig_out << "\n";

  return sig_out.str();
}

// ── Module-level finalization ─────────────────────────────────────────────────
// After per-item lowering, the emitter has written intrinsic flags and extern
// call declarations directly into the LirModule.  This function converts the
// dedup map into the extern_decls vector for the printer.

static void finalize_module(LirModule& module,
                            const c4c::hir::Module& hir_mod) {
  std::unordered_set<LinkNameId> local_fn_link_names;
  local_fn_link_names.reserve(hir_mod.functions.size());
  for (const auto& fn : hir_mod.functions) {
    if (fn.link_name_id != kInvalidLinkName) {
      local_fn_link_names.insert(fn.link_name_id);
    }
  }

  auto push_extern_decl = [&](const LirModule::ExternDeclInfo& decl_info) {
    const std::string fallback_name = decl_info.name.empty()
        ? emitted_link_name(hir_mod, decl_info.link_name_id, "")
        : decl_info.name;
    if (decl_info.link_name_id != kInvalidLinkName &&
        local_fn_link_names.count(decl_info.link_name_id)) {
      return;
    }
    if (decl_info.link_name_id == kInvalidLinkName &&
        hir_mod.fn_index.count(fallback_name)) {
      return;
    }

    LirExternDecl ed;
    ed.name = fallback_name;
    ed.return_type_str = decl_info.return_type_str;
    ed.return_type = c4c::codegen::lir::LirTypeRef(decl_info.return_type_str);
    ed.link_name_id = decl_info.link_name_id;
    module.extern_decls.push_back(std::move(ed));
  };

  // Convert extern declaration dedup state into the printer vector, filtering
  // out functions defined in this TU by semantic link name when available.
  for (const auto& [_, decl_info] : module.extern_decl_link_name_map) {
    push_extern_decl(decl_info);
  }
  for (const auto& [name, decl_info] : module.extern_decl_name_map) {
    (void)name;
    push_extern_decl(decl_info);
  }
}

// ── Block ordering ───────────────────────────────────────────────────────────
// Computes the HIR block iteration order: entry block first, then remaining
// blocks in their original order.  Ownership of this decision belongs to
// hir_to_lir, not StmtEmitter.

std::vector<const c4c::hir::Block*>
build_block_order(const c4c::hir::Function& fn) {
  std::vector<const c4c::hir::Block*> ordered;
  ordered.reserve(fn.blocks.size());
  const c4c::hir::Block* entry_blk = nullptr;
  for (const auto& blk : fn.blocks) {
    if (blk.id.value == fn.entry.value) { entry_blk = &blk; }
  }
  if (entry_blk) ordered.push_back(entry_blk);
  for (const auto& blk : fn.blocks) {
    if (blk.id.value != fn.entry.value) ordered.push_back(&blk);
  }
  return ordered;
}

// ── Fallthrough return injection ─────────────────────────────────────────────
// After per-statement lowering, any LirBlock whose terminator is still
// LirUnreachable did not receive an explicit terminator from the HIR.
// This post-pass injects a default "ret" matching the function's return type.

static void inject_fallthrough_returns(LirFunction& lir_fn,
                                       const c4c::hir::Function& fn) {
  using namespace c4c::codegen::llvm_helpers;
  const auto& rts = fn.return_type.spec;
  const std::string ret_ty = llvm_ret_ty(rts);

  // Pre-compute the default zero value for non-void returns.
  std::optional<std::string> zero_val;
  if (rts.base == TB_VOID && rts.ptr_level == 0 && rts.array_rank == 0 &&
      !rts.is_lvalue_ref && !rts.is_rvalue_ref) {
    // void → no value
  } else if (ret_ty == "ptr") {
    zero_val = "null";
  } else if (is_float_base(rts.base) && rts.ptr_level == 0 && rts.array_rank == 0) {
    zero_val = fp_literal(rts.base, 0.0);
  } else if (is_complex_base(rts.base) ||
             ((rts.base == TB_STRUCT || rts.base == TB_UNION) &&
              rts.ptr_level == 0 && rts.array_rank == 0)) {
    zero_val = "zeroinitializer";
  } else {
    zero_val = "0";
  }

  for (auto& blk : lir_fn.blocks) {
    if (!std::holds_alternative<LirUnreachable>(blk.terminator)) continue;
    blk.terminator = LirRet{zero_val, ret_ty};
  }
}

// ── Per-function skeleton helpers ─────────────────────────────────────────

static const c4c::hir::Expr& lookup_expr(const c4c::hir::Module& mod, c4c::hir::ExprId id) {
  for (const auto& e : mod.expr_pool)
    if (e.id.value == id.value) return e;
  throw std::runtime_error("hir_to_lir: expr not found id=" + std::to_string(id.value));
}

std::unordered_set<uint32_t> find_modified_params(
    const c4c::hir::Module& mod, const c4c::hir::Function& fn) {
  using namespace c4c::hir;
  std::unordered_set<uint32_t> result;
  std::function<void(ExprId)> scan = [&](ExprId id) {
    const Expr& e = lookup_expr(mod, id);
    std::visit([&](const auto& p) {
      using T = std::decay_t<decltype(p)>;
      if constexpr (std::is_same_v<T, UnaryExpr>) {
        if (p.op == UnaryOp::PreInc || p.op == UnaryOp::PostInc ||
            p.op == UnaryOp::PreDec || p.op == UnaryOp::PostDec ||
            p.op == UnaryOp::AddrOf) {
          const Expr& op_e = lookup_expr(mod, p.operand);
          if (const auto* r = std::get_if<DeclRef>(&op_e.payload)) {
            if (r->param_index) result.insert(*r->param_index);
          }
        }
        scan(p.operand);
      } else if constexpr (std::is_same_v<T, AssignExpr>) {
        const Expr& lhs_e = lookup_expr(mod, p.lhs);
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

bool fn_has_vla_locals(const c4c::hir::Function& fn) {
  for (const auto& blk : fn.blocks) {
    for (const auto& stmt : blk.stmts) {
      if (const auto* d = std::get_if<c4c::hir::LocalDecl>(&stmt.payload)) {
        if (d->vla_size.has_value()) return true;
      }
    }
  }
  return false;
}

void hoist_allocas(c4c::codegen::FnCtx& ctx, const c4c::hir::Module& mod,
                   const c4c::hir::Function& fn) {
  using namespace c4c::codegen::llvm_helpers;

  const std::unordered_set<uint32_t> modified_params = find_modified_params(mod, fn);
  for (size_t i = 0; i < fn.params.size(); ++i) {
    if (!modified_params.count(static_cast<uint32_t>(i))) continue;
    const auto& param = fn.params[i];
    if (llvm_target_is_amd64_sysv(mod.target_profile) &&
        llvm_cc::amd64_fixed_aggregate_passed_byval(param.type.spec, mod)) {
      continue;
    }
    const std::string slot = "%lv.param." + sanitize_llvm_ident(param.name);
    const std::string pname = "%p." + sanitize_llvm_ident(param.name);
    ctx.param_slots[static_cast<uint32_t>(i) + 0x80000000u] = slot;
    const int param_align = object_align_bytes(mod, param.type.spec);
    ctx.alloca_insts.push_back(LirAllocaOp{slot, llvm_alloca_ty(param.type.spec), "", param_align});
    ctx.alloca_insts.push_back(LirStoreOp{llvm_ty(param.type.spec), pname, slot});
  }

  std::unordered_map<std::string, int> name_count;
  for (const auto& blk : fn.blocks) {
    for (const auto& stmt : blk.stmts) {
      const auto* d = std::get_if<c4c::hir::LocalDecl>(&stmt.payload);
      if (!d) continue;
      if (ctx.local_slots.count(d->id.value)) continue;
      const int cnt = name_count[d->name]++;
      const std::string base = sanitize_llvm_ident(d->name);
      const std::string slot = cnt == 0
          ? "%lv." + base
          : "%lv." + base + "." + std::to_string(cnt);
      ctx.local_slots[d->id.value] = slot;
      ctx.local_types[d->id.value] = d->type.spec;
      ctx.local_is_vla[d->id.value] = d->vla_size.has_value();
      if (d->vla_size) {
        // VLA: alloca a pointer slot (the actual dynamic alloca happens later)
        TypeSpec ptr_ts{};
        ptr_ts.base = TB_VOID;
        ptr_ts.ptr_level = 1;
        ctx.alloca_insts.push_back(LirAllocaOp{slot, llvm_alloca_ty(ptr_ts), "", 0});
      } else {
        const int stack_align = object_align_bytes(mod, d->type.spec);
        ctx.alloca_insts.push_back(LirAllocaOp{slot, llvm_alloca_ty(d->type.spec), "", stack_align});
      }
    }
  }
}

c4c::codegen::FnCtx init_fn_ctx(const c4c::hir::Module& mod,
                                  const c4c::hir::Function& fn) {
  using namespace c4c::codegen::llvm_helpers;

  c4c::codegen::FnCtx ctx;
  ctx.fn = &fn;

  // Set up fn_ptr_sig metadata for parameters and globals.
  for (size_t i = 0; i < fn.params.size(); ++i) {
    if (fn.params[i].fn_ptr_sig) {
      ctx.param_fn_ptr_sigs[static_cast<uint32_t>(i)] = *fn.params[i].fn_ptr_sig;
    }
  }
  for (const auto& g : mod.globals) {
    if (g.fn_ptr_sig) ctx.global_fn_ptr_sigs[g.id.value] = *g.fn_ptr_sig;
  }

  // Populate param_slots for body lowering.
  const bool void_param_list =
      fn.params.size() == 1 &&
      fn.params[0].type.spec.base == TB_VOID &&
      fn.params[0].type.spec.ptr_level == 0 &&
      fn.params[0].type.spec.array_rank == 0;
  for (size_t i = 0; i < fn.params.size(); ++i) {
    if (void_param_list) break;
    const std::string pname = "%p." + sanitize_llvm_ident(fn.params[i].name);
    ctx.param_slots[static_cast<uint32_t>(i)] = pname;
  }

  // Create entry block.
  LirBlock entry_blk;
  entry_blk.id = LirBlockId{0};
  entry_blk.label = "entry";
  ctx.lir_blocks.push_back(std::move(entry_blk));
  ctx.current_block_idx = 0;

  // Hoist allocas.
  hoist_allocas(ctx, mod, fn);

  // VLA stack save — must happen after alloca hoisting but before statements.
  if (fn_has_vla_locals(fn)) {
    const std::string saved_sp = "%t" + std::to_string(ctx.tmp_idx++);
    ctx.cur_block().insts.push_back(LirStackSaveOp{saved_sp});
    ctx.vla_stack_save_ptr = saved_sp;
  }

  return ctx;
}

// ── Block label helpers ──────────────────────────────────────────────────────
// Ownership of block naming and creation belongs to hir_to_lir, not StmtEmitter.

std::string block_lbl(c4c::hir::BlockId id) {
  return "block_" + std::to_string(id.value);
}

void emit_lbl(c4c::codegen::FnCtx& ctx, const std::string& lbl) {
  LirBlock blk;
  blk.id = LirBlockId{static_cast<uint32_t>(ctx.lir_blocks.size())};
  blk.label = lbl;
  ctx.lir_blocks.push_back(std::move(blk));
  ctx.current_block_idx = ctx.lir_blocks.size() - 1;
  ctx.last_term = false;
}

// ── Dead internal function elimination ────────────────────────────────────────
// Removes unreferenced internal (static) functions from the module.
// This is a semantic decision (what to emit) and belongs in lowering, not the
// printer.  The algorithm is a worklist reachability analysis: external functions
// and global initializers seed the worklist; internal functions are kept only if
// transitively reachable.

// Scan a string for @name / @"quoted name" global references.
static void scan_refs(const std::string& s,
                      std::unordered_set<std::string>& refs) {
  size_t pos = 0;
  while ((pos = s.find('@', pos)) != std::string::npos) {
    ++pos;
    if (pos < s.size() && s[pos] == '"') {
      ++pos;  // skip opening "
      size_t start = pos;
      while (pos < s.size() && s[pos] != '"') ++pos;
      if (pos > start)
        refs.insert("\"" + s.substr(start, pos - start) + "\"");
      if (pos < s.size()) ++pos;  // skip closing "
    } else {
      size_t start = pos;
      while (pos < s.size() &&
             (std::isalnum(static_cast<unsigned char>(s[pos])) ||
              s[pos] == '_' || s[pos] == '.'))
        ++pos;
      if (pos > start) refs.insert(s.substr(start, pos - start));
    }
  }
}

// Collect all global symbol references from a single LIR instruction.
// Scans every string operand field for @-prefixed global references.
static void collect_inst_refs(const LirInst& inst,
                              std::unordered_set<std::string>& refs) {
  auto S = [&](const std::string& s) { scan_refs(s, refs); };
  // Visit each typed LIR op and scan its string-valued operand fields.
  // Types with no string operands (legacy typed ops with no producers) are
  // handled by the default case.
  auto visitor = [&](const auto& op) {
    using T = std::decay_t<decltype(op)>;
    if constexpr (std::is_same_v<T, LirCallOp>) {
      collect_lir_global_symbol_refs_from_call(
          op, [&](std::string_view ref) { refs.insert(std::string(ref)); });
    } else if constexpr (std::is_same_v<T, LirStoreOp>) {
      S(op.val); S(op.ptr);
    } else if constexpr (std::is_same_v<T, LirLoadOp>) {
      S(op.ptr);
    } else if constexpr (std::is_same_v<T, LirGepOp>) {
      S(op.ptr);
      for (const auto& idx : op.indices) S(idx);
    } else if constexpr (std::is_same_v<T, LirCastOp>) {
      S(op.operand);
    } else if constexpr (std::is_same_v<T, LirBinOp>) {
      S(op.lhs); S(op.rhs);
    } else if constexpr (std::is_same_v<T, LirCmpOp>) {
      S(op.lhs); S(op.rhs);
    } else if constexpr (std::is_same_v<T, LirPhiOp>) {
      for (const auto& [v, l] : op.incoming) S(v);
    } else if constexpr (std::is_same_v<T, LirSelectOp>) {
      S(op.cond); S(op.true_val); S(op.false_val);
    } else if constexpr (std::is_same_v<T, LirExtractValueOp>) {
      S(op.agg);
    } else if constexpr (std::is_same_v<T, LirInsertValueOp>) {
      S(op.agg); S(op.elem);
    } else if constexpr (std::is_same_v<T, LirAllocaOp>) {
      S(op.count);
    } else if constexpr (std::is_same_v<T, LirInlineAsmOp>) {
      S(op.args_str);
    } else if constexpr (std::is_same_v<T, LirAbsOp>) {
      S(op.arg);
    } else if constexpr (std::is_same_v<T, LirInsertElementOp>) {
      S(op.vec); S(op.elem);
    } else if constexpr (std::is_same_v<T, LirExtractElementOp>) {
      S(op.vec);
    } else if constexpr (std::is_same_v<T, LirShuffleVectorOp>) {
      S(op.vec1); S(op.vec2);
    } else if constexpr (std::is_same_v<T, LirVaArgOp>) {
      S(op.ap_ptr);
    } else if constexpr (std::is_same_v<T, LirMemcpyOp>) {
      S(op.dst); S(op.src);
    } else if constexpr (std::is_same_v<T, LirVaStartOp>) {
      S(op.ap_ptr);
    } else if constexpr (std::is_same_v<T, LirVaEndOp>) {
      S(op.ap_ptr);
    } else if constexpr (std::is_same_v<T, LirVaCopyOp>) {
      S(op.dst_ptr); S(op.src_ptr);
    } else if constexpr (std::is_same_v<T, LirStackRestoreOp>) {
      S(op.saved_ptr);
    } else if constexpr (std::is_same_v<T, LirIndirectBrOp>) {
      S(op.addr);
    }
    // LirStackSaveOp and legacy typed ops: no string operands to scan.
  };
  std::visit(visitor, inst);
}

// Collect all global references from a function's body.
static std::unordered_set<std::string> collect_fn_refs(const LirFunction& fn) {
  std::unordered_set<std::string> refs;
  // Scan signature text (may reference other functions in attributes/metadata).
  scan_refs(fn.signature_text, refs);
  // Scan hoisted allocas.
  for (const auto& inst : fn.alloca_insts) collect_inst_refs(inst, refs);
  // Scan block instructions.
  for (const auto& blk : fn.blocks) {
    for (const auto& inst : blk.insts) collect_inst_refs(inst, refs);
  }
  return refs;
}

static void eliminate_dead_internals(LirModule& mod) {
  std::unordered_set<std::string> discardable_fns;
  for (const auto& f : mod.functions) {
    if (f.can_elide_if_unreferenced) discardable_fns.insert(f.name);
  }
  if (discardable_fns.empty()) return;

  // Build per-function reference sets.
  std::unordered_map<std::string, size_t> fn_idx_by_name;
  std::vector<std::unordered_set<std::string>> fn_refs(mod.functions.size());
  for (size_t i = 0; i < mod.functions.size(); ++i) {
    fn_idx_by_name[mod.functions[i].name] = i;
    fn_refs[i] = collect_fn_refs(mod.functions[i]);
  }

  std::unordered_set<std::string> reachable;
  std::vector<std::string> worklist;

  auto seed = [&](const std::unordered_set<std::string>& refs) {
    for (const auto& r : refs) {
      if (discardable_fns.count(r) && !reachable.count(r)) {
        reachable.insert(r);
        worklist.push_back(r);
      }
    }
  };

  // Seed from non-discardable function bodies.
  for (size_t i = 0; i < mod.functions.size(); ++i) {
    if (!mod.functions[i].can_elide_if_unreferenced) seed(fn_refs[i]);
  }

  // Seed from global initializers.
  for (const auto& g : mod.globals) {
    std::unordered_set<std::string> grefs;
    scan_refs(g.init_text, grefs);
    seed(grefs);
  }

  // Propagate: internal functions referenced by reachable internal functions.
  while (!worklist.empty()) {
    std::string cur = std::move(worklist.back());
    worklist.pop_back();
    auto it = fn_idx_by_name.find(cur);
    if (it == fn_idx_by_name.end()) continue;
    seed(fn_refs[it->second]);
  }

  // Remove unreachable discardable functions.
  mod.functions.erase(
      std::remove_if(mod.functions.begin(), mod.functions.end(),
                     [&](const LirFunction& f) {
                       return f.can_elide_if_unreferenced &&
                              !reachable.count(f.name);
                     }),
      mod.functions.end());
}

// ── Main lowering entry point ────────────────────────────────────────────────

LirModule lower(const c4c::hir::Module& hir_mod, const LowerOptions& options) {
  using namespace c4c::codegen::llvm_helpers;
  const c4c::TargetProfile& target_profile = hir_mod.target_profile;
  // Module-level orchestration: owned by hir_to_lir, not StmtEmitter.
  set_active_target_profile(target_profile);

  LirModule module;
  module.target_profile = target_profile;
  module.data_layout = !hir_mod.data_layout.empty()
      ? hir_mod.data_layout
      : llvm_default_datalayout(target_profile);
  module.link_name_texts = hir_mod.link_name_texts;
  module.link_names = hir_mod.link_names;
  module.struct_names.attach_text_table(module.link_name_texts.get());
  module.type_decls = build_type_decls(hir_mod);
  module.prefer_semantic_va_ops = options.preserve_semantic_va_ops;

  auto global_indices = dedup_globals(hir_mod);
  auto fn_indices = dedup_functions(hir_mod);

  // Per-item lowering: hir_to_lir owns iteration and the module;
  // Const-init lowering (globals) — standalone, no StmtEmitter dependency.
  ConstInitEmitter const_init(hir_mod, module);

  // StmtEmitter owns statement/expression lowering and writes into module.
  StmtEmitter emitter(hir_mod);
  emitter.set_module(module);

  bool any_vla = false;
  std::vector<LirSpecEntry> spec_entries;

  lower_globals(global_indices, hir_mod, const_init, module);
  for (size_t idx : fn_indices) {
    const auto& fn = hir_mod.functions[idx];
    std::string sig = build_fn_signature(hir_mod, fn);

    if (fn.linkage.is_extern && fn.blocks.empty()) {
      // Declaration — no body to lower; hir_to_lir owns this directly.
      LirFunction lir_fn;
      lir_fn.name = quote_llvm_ident(fn.name);
      lir_fn.link_name_id = fn.link_name_id;
      lir_fn.is_internal = false;
      lir_fn.can_elide_if_unreferenced = false;
      lir_fn.is_declaration = true;
      lir_fn.return_type = fn.return_type.spec;
      for (const auto& param : fn.params) {
        lir_fn.params.push_back(
            {"%p." + sanitize_llvm_ident(param.name), param.type.spec});
      }
      lir_fn.signature_text = sig;
      module.functions.push_back(std::move(lir_fn));
    } else {
      // Definition — hir_to_lir owns FnCtx setup, alloca hoisting, VLA
      // stack save, spec entry collection, and LirFunction construction.
      // StmtEmitter owns statement / expression emission only.
      auto ctx = init_fn_ctx(hir_mod, fn);
      if (ctx.vla_stack_save_ptr) any_vla = true;
      auto block_order = build_block_order(fn);

      // Spec entries — owned by hir_to_lir.
      if (!fn.template_origin.empty() && !fn.spec_key.empty()) {
        spec_entries.push_back(
            {fn.spec_key.canonical, fn.template_origin, std::string(fn.name), fn.link_name_id});
      }

      // Block iteration — owned by hir_to_lir.
      for (size_t bi = 0; bi < block_order.size(); ++bi) {
        const auto* blk = block_order[bi];
        ctx.current_block_id = blk->id.value;
        if (bi > 0) {
          emit_lbl(ctx, block_lbl(blk->id));
        }
        for (const auto& stmt : blk->stmts) {
          emitter.emit_stmt(ctx, stmt);
        }
      }

      // Build LirFunction from accumulated ctx — owned by hir_to_lir.
      LirFunction lir_fn;
      lir_fn.name = quote_llvm_ident(fn.name);
      lir_fn.link_name_id = fn.link_name_id;
      lir_fn.is_internal = fn.linkage.is_static;
      const bool is_std_impl_helper =
          fn.name.rfind("std::__", 0) == 0;
      lir_fn.can_elide_if_unreferenced =
          fn.linkage.is_static || fn.linkage.is_inline || is_std_impl_helper;
      lir_fn.is_declaration = false;
      lir_fn.return_type = fn.return_type.spec;
      lir_fn.signature_text = sig;
      lir_fn.alloca_insts = std::move(ctx.alloca_insts);
      lir_fn.blocks = std::move(ctx.lir_blocks);

      inject_fallthrough_returns(lir_fn, fn);
      module.functions.push_back(std::move(lir_fn));
    }
  }

  if (any_vla) module.need_stacksave = true;

  // Add spec entries collected by hir_to_lir.
  for (auto& e : spec_entries) {
    module.spec_entries.push_back(std::move(e));
  }

  // Module-level finalization: owned by hir_to_lir.
  finalize_module(module, hir_mod);

  // Dead internal function elimination: owned by hir_to_lir.
  eliminate_dead_internals(module);

  return module;
}

}  // namespace c4c::codegen::lir
