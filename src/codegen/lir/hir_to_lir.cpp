#include "hir_to_lir.hpp"
#include "ir.hpp"
#include "../llvm/hir_emitter.hpp"
#include "../llvm/hir_to_llvm_helpers.hpp"

#include <algorithm>
#include <functional>
#include <sstream>
#include <unordered_map>
#include <variant>

namespace c4c::codegen::lir {

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
  if (ts.ptr_level > 0 || ts.is_fn_ptr) {
    align = 8;
  } else if ((ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.tag && ts.tag[0]) {
    const auto it = mod.struct_defs.find(ts.tag);
    align = (it != mod.struct_defs.end()) ? std::max(1, it->second.align_bytes) : 8;
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
  std::unordered_map<std::string, size_t> best; // name -> index
  for (size_t i = 0; i < mod.globals.size(); ++i) {
    const auto& gv = mod.globals[i];
    auto it = best.find(gv.name);
    if (it == best.end()) {
      best[gv.name] = i;
    } else {
      const bool cur_has_init = !std::holds_alternative<std::monostate>(mod.globals[it->second].init);
      const bool new_has_init = !std::holds_alternative<std::monostate>(gv.init);
      if (new_has_init || !cur_has_init) it->second = i;
    }
  }
  // Collect in original order
  std::vector<size_t> result;
  result.reserve(best.size());
  for (size_t i = 0; i < mod.globals.size(); ++i) {
    auto it = best.find(mod.globals[i].name);
    if (it != best.end() && it->second == i) result.push_back(i);
  }
  return result;
}

std::vector<size_t> dedup_functions(const c4c::hir::Module& mod) {
  std::unordered_map<std::string, size_t> best; // name -> index
  for (size_t i = 0; i < mod.functions.size(); ++i) {
    const auto& fn = mod.functions[i];
    if (!fn.materialized) continue;
    auto it = best.find(fn.name);
    if (it == best.end()) {
      best[fn.name] = i;
    } else {
      const bool cur_is_def = !mod.functions[it->second].blocks.empty();
      const bool new_is_def = !fn.blocks.empty();
      if (new_is_def && !cur_is_def) it->second = i;
    }
  }
  // Collect in original order
  std::vector<size_t> result;
  result.reserve(best.size());
  for (size_t i = 0; i < mod.functions.size(); ++i) {
    if (!mod.functions[i].materialized) continue;
    auto it = best.find(mod.functions[i].name);
    if (it != best.end() && it->second == i) result.push_back(i);
  }
  return result;
}

std::vector<std::string> build_type_decls(const c4c::hir::Module& mod) {
  using namespace c4c::codegen::llvm_backend::detail;
  std::vector<std::string> decls;

  if (!llvm_va_list_is_pointer_object(mod.target_triple)) {
    decls.push_back("%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }");
  }

  for (const auto& tag : mod.struct_def_order) {
    const auto it = mod.struct_defs.find(tag);
    if (it == mod.struct_defs.end()) continue;
    const auto& sd = it->second;
    const std::string sty = llvm_struct_type_str(tag);

    if (sd.fields.empty()) {
      decls.push_back(sty + " = type {}");
      continue;
    }
    if (sd.is_union) {
      decls.push_back(sty + " = type { [" +
                       std::to_string(sd.size_bytes) + " x i8] }");
    } else {
      std::ostringstream line;
      line << sty << " = type { ";
      bool first = true;
      int last_idx = -1;
      int cur_offset = 0;
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
// logic belongs to hir_to_lir; HirEmitter consumes the pre-built text.

std::string build_fn_signature(const c4c::hir::Function& fn) {
  using namespace c4c::codegen::llvm_backend::detail;

  std::ostringstream sig_out;
  const std::string ret_ty = llvm_ret_ty(fn.return_type.spec);

  const bool void_param_list =
      fn.params.size() == 1 &&
      fn.params[0].type.spec.base == TB_VOID &&
      fn.params[0].type.spec.ptr_level == 0 &&
      fn.params[0].type.spec.array_rank == 0;

  // Declaration (extern with no body)
  if (fn.linkage.is_extern && fn.blocks.empty()) {
    const std::string decl_kw = fn.linkage.is_weak ? "declare extern_weak " : "declare ";
    sig_out << decl_kw << llvm_visibility(fn.linkage.visibility) << ret_ty << " "
            << llvm_global_sym(fn.name) << "(";
    for (size_t i = 0; i < fn.params.size(); ++i) {
      if (void_param_list) break;
      if (i) sig_out << ", ";
      sig_out << llvm_ty(fn.params[i].type.spec);
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
          << llvm_global_sym(fn.name) << "(";

  // Parameters
  for (size_t i = 0; i < fn.params.size(); ++i) {
    if (void_param_list) break;
    if (i) sig_out << ", ";
    const std::string pty = llvm_ty(fn.params[i].type.spec);
    const std::string pname = "%p." + sanitize_llvm_ident(fn.params[i].name);
    sig_out << pty << " " << pname;
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
// After per-item lowering, the emitter holds accumulated intrinsic flags,
// extern call declarations, and spec entries.  This function transfers them
// into the LirModule so the printer / future backends can consume them.

static void finalize_module(LirModule& module,
                            const c4c::hir::Module& hir_mod,
                            const c4c::codegen::llvm_backend::HirEmitter& emitter) {
  // Intrinsic requirement flags
  if (emitter.needs_va_start())     module.need_va_start = true;
  if (emitter.needs_va_end())       module.need_va_end = true;
  if (emitter.needs_va_copy())      module.need_va_copy = true;
  if (emitter.needs_memcpy())       module.need_memcpy = true;
  if (emitter.needs_stacksave())    module.need_stacksave = true;
  if (emitter.needs_stackrestore()) module.need_stackrestore = true;
  if (emitter.needs_abs())          module.need_abs = true;

  // Extern call declarations (functions called but not defined in this TU)
  for (const auto& [name, ret_ty] : emitter.extern_call_decls()) {
    if (hir_mod.fn_index.count(name)) continue;
    LirExternDecl ed;
    ed.name = name;
    ed.return_type_str = ret_ty;
    module.extern_decls.push_back(std::move(ed));
  }

  // Specialization metadata
  for (const auto& e : emitter.spec_entries()) {
    module.spec_entries.push_back({e.spec_key, e.template_origin, e.mangled_name});
  }
}

// ── Block ordering ───────────────────────────────────────────────────────────
// Computes the HIR block iteration order: entry block first, then remaining
// blocks in their original order.  Ownership of this decision belongs to
// hir_to_lir, not HirEmitter.

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
  using namespace c4c::codegen::llvm_backend::detail;
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
  using namespace c4c::codegen::llvm_backend::detail;

  const std::unordered_set<uint32_t> modified_params = find_modified_params(mod, fn);
  for (size_t i = 0; i < fn.params.size(); ++i) {
    if (!modified_params.count(static_cast<uint32_t>(i))) continue;
    const auto& param = fn.params[i];
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
        if (d->init &&
            (d->type.spec.array_rank > 0 ||
             (d->type.spec.ptr_level == 0 &&
              (d->type.spec.base == TB_STRUCT ||
               d->type.spec.base == TB_UNION)))) {
          ctx.alloca_insts.push_back(LirStoreOp{llvm_alloca_ty(d->type.spec), "zeroinitializer", slot});
        }
      }
    }
  }
}

c4c::codegen::FnCtx init_fn_ctx(const c4c::hir::Module& mod,
                                  const c4c::hir::Function& fn) {
  using namespace c4c::codegen::llvm_backend::detail;

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
// Ownership of block naming and creation belongs to hir_to_lir, not HirEmitter.

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

// ── Main lowering entry point ────────────────────────────────────────────────

LirModule lower(const c4c::hir::Module& hir_mod) {
  using namespace c4c::codegen::llvm_backend::detail;

  // Module-level orchestration: owned by hir_to_lir, not HirEmitter.
  set_active_target_triple(hir_mod.target_triple);

  LirModule module;
  module.target_triple = hir_mod.target_triple;
  module.data_layout = !hir_mod.data_layout.empty()
      ? hir_mod.data_layout
      : llvm_default_datalayout(hir_mod.target_triple);
  module.type_decls = build_type_decls(hir_mod);

  auto global_indices = dedup_globals(hir_mod);
  auto fn_indices = dedup_functions(hir_mod);

  // Per-item lowering: hir_to_lir owns iteration and the module;
  // HirEmitter owns statement/expression lowering and writes into module.
  c4c::codegen::llvm_backend::HirEmitter emitter(hir_mod);
  emitter.set_module(module);

  bool any_vla = false;
  std::vector<LirSpecEntry> spec_entries;

  emitter.lower_globals(global_indices);
  for (size_t idx : fn_indices) {
    const auto& fn = hir_mod.functions[idx];
    std::string sig = build_fn_signature(fn);

    if (fn.linkage.is_extern && fn.blocks.empty()) {
      // Declaration — no body to lower; hir_to_lir owns this directly.
      LirFunction lir_fn;
      lir_fn.name = quote_llvm_ident(fn.name);
      lir_fn.is_internal = false;
      lir_fn.is_declaration = true;
      lir_fn.signature_text = sig;
      module.functions.push_back(std::move(lir_fn));
    } else {
      // Definition — hir_to_lir owns FnCtx setup, alloca hoisting, VLA
      // stack save, spec entry collection, and LirFunction construction.
      // HirEmitter owns statement / expression emission only.
      auto ctx = init_fn_ctx(hir_mod, fn);
      if (ctx.vla_stack_save_ptr) any_vla = true;
      auto block_order = build_block_order(fn);

      // Spec entries — owned by hir_to_lir.
      if (!fn.template_origin.empty() && !fn.spec_key.empty()) {
        spec_entries.push_back({fn.spec_key.canonical, fn.template_origin, std::string(fn.name)});
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
      lir_fn.is_internal = fn.linkage.is_static;
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
  finalize_module(module, hir_mod, emitter);

  return module;
}

}  // namespace c4c::codegen::lir
