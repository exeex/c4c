#include "stmt_emitter.hpp"
#include "call_args_ops.hpp"

namespace c4c::codegen::lir {

using namespace stmt_emitter_detail;

// Draft-only staging file for Step 2 of the stmt_emitter split refactor.
// This is the first expression-oriented ownership slice; the monolith still
// owns the full live implementation until Step 4 wires these files into CMake.

TypeSpec StmtEmitter::resolve_expr_type(FnCtx& ctx, ExprId id) {
  const Expr& e = get_expr(id);
  const TypeSpec& ts = e.type.spec;
  if (ts.base != TB_VOID || ts.ptr_level > 0 || ts.array_rank > 0 || is_vector_value(ts)) {
    return ts;
  }
  return std::visit([&](const auto& p) -> TypeSpec { return resolve_payload_type(ctx, p); }, e.payload);
}

TypeSpec StmtEmitter::resolve_expr_type(FnCtx& ctx, const Expr& e) {
  if (has_concrete_type(e.type.spec)) return e.type.spec;
  return std::visit([&](const auto& p) -> TypeSpec { return resolve_payload_type(ctx, p); }, e.payload);
}

const FnPtrSig* StmtEmitter::resolve_callee_fn_ptr_sig(FnCtx& ctx, const Expr& callee_e) {
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
  if (const auto* m = std::get_if<MemberExpr>(&callee_e.payload)) {
    TypeSpec base_ts = resolve_expr_type(ctx, m->base);
    if (m->is_arrow && base_ts.ptr_level > 0) base_ts.ptr_level--;
    if (base_ts.tag && base_ts.tag[0]) {
      const auto sit = mod_.struct_defs.find(base_ts.tag);
      if (sit != mod_.struct_defs.end()) {
        for (const auto& f : sit->second.fields) {
          if (f.name == m->field && f.fn_ptr_sig) {
            return &*f.fn_ptr_sig;
          }
        }
      }
    }
  }
  if (const auto* t = std::get_if<TernaryExpr>(&callee_e.payload)) {
    return resolve_callee_fn_ptr_sig(ctx, get_expr(t->then_expr));
  }
  if (const auto* c = std::get_if<CastExpr>(&callee_e.payload)) {
    if (c->fn_ptr_sig) return &*c->fn_ptr_sig;
  }
  if (const auto* idx = std::get_if<IndexExpr>(&callee_e.payload)) {
    return resolve_callee_fn_ptr_sig(ctx, get_expr(idx->base));
  }
  if (const auto* call = std::get_if<CallExpr>(&callee_e.payload)) {
    auto find_function = [&](const std::string& name) -> const Function* {
      const auto fit = mod_.fn_index.find(name);
      if (fit == mod_.fn_index.end() || fit->second.value >= mod_.functions.size()) return nullptr;
      return &mod_.functions[fit->second.value];
    };
    auto build_fn_sig = [&](const Function& fn) -> const FnPtrSig* {
      auto [it, _] = inferred_direct_fn_sigs_.try_emplace(fn.id.value);
      FnPtrSig& sig = it->second;
      if (sig.params.empty() && !sig.variadic && sig.return_type.spec.base == TB_VOID &&
          sig.return_type.spec.ptr_level == 0 && sig.return_type.spec.array_rank == 0) {
        sig.return_type = fn.return_type;
        sig.variadic = fn.attrs.variadic;
        sig.unspecified_params = false;
        for (const auto& param : fn.params) sig.params.push_back(param.type);
      }
      return &sig;
    };
    auto infer_returned_function = [&](auto&& self, const Function& fn) -> const Function* {
      for (const Block& bb : fn.blocks) {
        for (const Stmt& stmt : bb.stmts) {
          const auto* ret = std::get_if<ReturnStmt>(&stmt.payload);
          if (!ret || !ret->expr) continue;
          const Expr& expr = get_expr(*ret->expr);
          if (const auto* uret = std::get_if<UnaryExpr>(&expr.payload)) {
            if (uret->op == UnaryOp::AddrOf) {
              const Expr& inner = get_expr(uret->operand);
              if (const auto* inner_dr = std::get_if<DeclRef>(&inner.payload)) {
                if (const Function* returned = find_function(inner_dr->name)) return returned;
              }
            }
          }
          if (const auto* dr2 = std::get_if<DeclRef>(&expr.payload)) {
            if (const Function* returned = find_function(dr2->name)) return returned;
          }
          if (const auto* ternary = std::get_if<TernaryExpr>(&expr.payload)) {
            const Expr& then_e = get_expr(ternary->then_expr);
            if (const auto* then_dr = std::get_if<DeclRef>(&then_e.payload)) {
              if (const Function* returned = find_function(then_dr->name)) return returned;
            }
            const Expr& else_e = get_expr(ternary->else_expr);
            if (const auto* else_dr = std::get_if<DeclRef>(&else_e.payload)) {
              if (const Function* returned = find_function(else_dr->name)) return returned;
            }
          }
          if (const auto* call_expr = std::get_if<CallExpr>(&expr.payload)) {
            const Expr& nested_callee = get_expr(call_expr->callee);
            if (const auto* nested_dr = std::get_if<DeclRef>(&nested_callee.payload)) {
              if (const Function* called = find_function(nested_dr->name)) {
                if (const Function* returned = self(self, *called)) return returned;
              }
            }
          }
        }
      }
      return nullptr;
    };

    const Expr& inner_callee = get_expr(call->callee);
    if (const auto* dr = std::get_if<DeclRef>(&inner_callee.payload)) {
      if (const Function* target = find_function(dr->name)) {
        const FnPtrSig* cached_sig = target->ret_fn_ptr_sig ? &*target->ret_fn_ptr_sig : nullptr;
        if (cached_sig && sig_has_meaningful_prototype(*cached_sig)) return cached_sig;
        if (const Function* returned = infer_returned_function(infer_returned_function, *target)) {
          return build_fn_sig(*returned);
        }
        if (cached_sig) return cached_sig;
      }
    }
    if (const auto* nested_call = std::get_if<CallExpr>(&inner_callee.payload)) {
      const Expr& nested_callee = get_expr(nested_call->callee);
      if (const auto* nested_dr = std::get_if<DeclRef>(&nested_callee.payload)) {
        if (const Function* target = find_function(nested_dr->name)) {
          if (const Function* returned = infer_returned_function(infer_returned_function, *target)) {
            if (const Function* nested_returned =
                    infer_returned_function(infer_returned_function, *returned)) {
              return build_fn_sig(*nested_returned);
            }
            return build_fn_sig(*returned);
          }
        }
      }
    }
  }
  return nullptr;
}

TypeSpec StmtEmitter::resolve_payload_type(FnCtx& ctx, const DeclRef& r) {
  if (r.local) {
    const auto it = ctx.local_types.find(r.local->value);
    if (it != ctx.local_types.end()) return it->second;
  }
  if (r.param_index && ctx.fn && *r.param_index < ctx.fn->params.size()) {
    return ctx.fn->params[*r.param_index].type.spec;
  }
  if (r.global) {
    if (const GlobalVar* gv = select_global_object(*r.global)) return gv->type.spec;
  }
  return {};
}

TypeSpec StmtEmitter::resolve_payload_type(FnCtx& ctx, const BinaryExpr& b) {
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
      const Expr& rhs_e = get_expr(b.rhs);
      return std::visit([&](const auto& p) -> TypeSpec { return resolve_payload_type(ctx, p); },
                        rhs_e.payload);
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
  if ((b.op == BinaryOp::Add || b.op == BinaryOp::Sub) && llvm_ty(lts) == "ptr" &&
      rts.ptr_level == 0 && rts.array_rank == 0 && is_any_int(rts.base)) {
    return lts;
  }
  if (b.op == BinaryOp::Add && llvm_ty(rts) == "ptr" && lts.ptr_level == 0 &&
      lts.array_rank == 0 && is_any_int(lts.base)) {
    return rts;
  }
  if (is_complex_base(lts.base) || is_complex_base(rts.base)) {
    if (!is_complex_base(lts.base)) return rts;
    if (!is_complex_base(rts.base)) return lts;
    return sizeof_ts(mod_, lts) >= sizeof_ts(mod_, rts) ? lts : rts;
  }
  if (is_vector_value(lts)) return lts;
  if (is_vector_value(rts)) return rts;

  if (lts.base != TB_VOID && rts.base != TB_VOID && lts.ptr_level == 0 && rts.ptr_level == 0 &&
      lts.array_rank == 0 && rts.array_rank == 0 && is_any_int(lts.base) &&
      is_any_int(rts.base)) {
    const bool is_shift = (b.op == BinaryOp::Shl || b.op == BinaryOp::Shr);
    TypeSpec ts{};
    ts.base = is_shift ? integer_promote(lts.base) : usual_arith_conv(lts.base, rts.base);
    return ts;
  }
  if (lts.base != TB_VOID) return lts;
  return resolve_expr_type(ctx, b.rhs);
}

TypeSpec StmtEmitter::resolve_payload_type(FnCtx& ctx, const UnaryExpr& u) {
  TypeSpec ts = resolve_expr_type(ctx, u.operand);
  switch (u.op) {
    case UnaryOp::AddrOf:
      if (ts.array_rank > 0 && !is_vector_value(ts)) {
        ts.is_ptr_to_array = true;
      }
      ts.ptr_level += 1;
      return ts;
    case UnaryOp::Deref:
      if (ts.ptr_level > 0) {
        ts.ptr_level -= 1;
        if (ts.ptr_level == 0) ts.is_ptr_to_array = false;
      } else if (ts.array_rank > 0) {
        ts.array_rank -= 1;
      }
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
    case UnaryOp::BitNot:
    case UnaryOp::Plus:
    case UnaryOp::Minus:
      if (ts.ptr_level == 0 && !is_vector_value(ts)) ts.base = integer_promote(ts.base);
      return ts;
    default:
      return ts;
  }
}

TypeSpec StmtEmitter::resolve_payload_type(FnCtx& ctx, const AssignExpr& a) {
  return resolve_expr_type(ctx, a.lhs);
}

TypeSpec StmtEmitter::resolve_payload_type(FnCtx&, const CastExpr& c) { return c.to_type.spec; }
TypeSpec StmtEmitter::resolve_payload_type(FnCtx& ctx, const CallExpr& c) {
  const BuiltinId builtin_id = c.builtin_id;
  if (builtin_id != BuiltinId::Unknown) {
    if (builtin_id == BuiltinId::Expect && !c.args.empty()) return resolve_expr_type(ctx, c.args[0]);
    switch (builtin_id) {
      case BuiltinId::Unreachable:
      case BuiltinId::VaStart:
      case BuiltinId::VaEnd:
      case BuiltinId::VaCopy:
      case BuiltinId::Prefetch:
        return {};
      case BuiltinId::Bswap16:
      case BuiltinId::Bswap32:
      case BuiltinId::Bswap64:
        if (!c.args.empty()) return resolve_expr_type(ctx, c.args[0]);
        break;
      default:
        break;
    }
    if (auto builtin_ts =
            type_spec_from_builtin_result_kind(builtin_result_kind(builtin_id))) {
      return *builtin_ts;
    }
  }
  const Expr& callee_e = get_expr(c.callee);
  if (const auto* r = std::get_if<DeclRef>(&callee_e.payload)) {
    const auto fit = mod_.fn_index.find(r->name);
    if (fit != mod_.fn_index.end() && fit->second.value < mod_.functions.size()) {
      TypeSpec ret = mod_.functions[fit->second.value].return_type.spec;
      if ((ret.is_lvalue_ref || ret.is_rvalue_ref) && ret.ptr_level == 0) ret.ptr_level++;
      return ret;
    }
  }
  if (const FnPtrSig* sig = resolve_callee_fn_ptr_sig(ctx, callee_e)) {
    return sig_return_type(*sig);
  }
  if (const auto* r = std::get_if<DeclRef>(&callee_e.payload)) {
    if (auto kts =
            type_spec_from_builtin_result_kind(known_call_result_kind(r->name.c_str()))) {
      return *kts;
    }
  }
  TypeSpec implicit_int{};
  implicit_int.base = TB_INT;
  return implicit_int;
}

TypeSpec StmtEmitter::resolve_payload_type(FnCtx&, const IntLiteral&) {
  TypeSpec ts{};
  ts.base = TB_INT;
  return ts;
}

TypeSpec StmtEmitter::resolve_payload_type(FnCtx&, const FloatLiteral&) {
  TypeSpec ts{};
  ts.base = TB_DOUBLE;
  return ts;
}

TypeSpec StmtEmitter::resolve_payload_type(FnCtx&, const CharLiteral&) {
  TypeSpec ts{};
  ts.base = TB_CHAR;
  return ts;
}

TypeSpec StmtEmitter::resolve_payload_type(FnCtx&, const StringLiteral& sl) {
  TypeSpec ts{};
  ts.base = sl.is_wide ? TB_INT : TB_CHAR;
  ts.ptr_level = 1;
  return ts;
}

TypeSpec StmtEmitter::resolve_payload_type(FnCtx& ctx, const MemberExpr& m) {
  const MemberFieldAccess access = resolve_member_field_access(ctx, m);
  if (!access.has_tag() || !access.field_found) return {};
  if (access.bf.is_bitfield()) return bitfield_promoted_ts(access.bf);
  return access.field_ts;
}

TypeSpec StmtEmitter::resolve_payload_type(FnCtx& ctx, const IndexExpr& idx) {
  TypeSpec base_ts = resolve_expr_type(ctx, idx.base);
  if (is_vector_value(base_ts)) {
    base_ts.is_vector = false;
    base_ts.vector_lanes = 0;
    base_ts.vector_bytes = 0;
    return base_ts;
  }
  return drop_one_indexed_element_type(base_ts);
}

TypeSpec StmtEmitter::resolve_payload_type(FnCtx& ctx, const TernaryExpr& t) {
  TypeSpec ts = resolve_expr_type(ctx, t.then_expr);
  if (ts.base != TB_VOID || ts.ptr_level > 0 || ts.array_rank > 0 || is_vector_value(ts)) {
    return ts;
  }
  return resolve_expr_type(ctx, t.else_expr);
}

TypeSpec StmtEmitter::resolve_payload_type(FnCtx&, const VaArgExpr&) { return {}; }

TypeSpec StmtEmitter::resolve_payload_type(FnCtx&, const SizeofExpr&) {
  TypeSpec ts{};
  ts.base = TB_ULONGLONG;
  return ts;
}

TypeSpec StmtEmitter::resolve_payload_type(FnCtx&, const SizeofTypeExpr&) {
  TypeSpec ts{};
  ts.base = TB_ULONGLONG;
  return ts;
}

TypeSpec StmtEmitter::resolve_payload_type(FnCtx&, const LabelAddrExpr&) {
  TypeSpec ts{};
  ts.base = TB_VOID;
  ts.ptr_level = 1;
  return ts;
}

TypeSpec StmtEmitter::resolve_payload_type(FnCtx&, const PendingConstevalExpr&) { return {}; }

template <typename T>
TypeSpec StmtEmitter::resolve_payload_type(FnCtx&, const T&) {
  return {};
}

std::string StmtEmitter::emit_rval_id(FnCtx& ctx, ExprId id, TypeSpec& out_ts) {
  const Expr& e = get_expr(id);
  out_ts = e.type.spec;
  if (const auto* b = std::get_if<BinaryExpr>(&e.payload)) {
    out_ts = resolve_payload_type(ctx, *b);
  } else if (const auto* c = std::get_if<CallExpr>(&e.payload)) {
    out_ts = resolve_payload_type(ctx, *c);
  }
  if (out_ts.base == TB_VOID && out_ts.ptr_level == 0 && out_ts.array_rank == 0) {
    out_ts = resolve_expr_type(ctx, id);
  }
  return emit_rval_expr(ctx, e);
}

std::string StmtEmitter::emit_rval_expr(FnCtx& ctx, const Expr& e) {
  return std::visit([&](const auto& p) -> std::string { return emit_rval_payload(ctx, p, e); }, e.payload);
}

template <typename T>
std::string StmtEmitter::emit_rval_payload(FnCtx&, const T&, const Expr&) {
  throw std::runtime_error(std::string("StmtEmitter: unimplemented expr: ") + typeid(T).name());
}

std::string StmtEmitter::emit_rval_payload(FnCtx&, const IntLiteral& x, const Expr& e) {
  if (is_complex_base(e.type.spec.base)) {
    const TypeSpec elem_ts = complex_component_ts(e.type.spec.base);
    return "{ " + llvm_ty(elem_ts) + " " + emit_const_int_like(0, elem_ts) + ", " +
           llvm_ty(elem_ts) + " " + emit_const_int_like(x.value, elem_ts) + " }";
  }
  return std::to_string(x.value);
}

std::string StmtEmitter::emit_rval_payload(FnCtx&, const FloatLiteral& x, const Expr& e) {
  if (is_complex_base(e.type.spec.base)) {
    const TypeSpec elem_ts = complex_component_ts(e.type.spec.base);
    const std::string imag_v = fp_literal(elem_ts.base, x.value);
    return "{ " + llvm_ty(elem_ts) + " " + emit_const_int_like(0, elem_ts) + ", " +
           llvm_ty(elem_ts) + " " + imag_v + " }";
  }
  return is_float_base(e.type.spec.base) ? fp_literal(e.type.spec.base, x.value) : fp_to_hex(x.value);
}

std::string StmtEmitter::emit_rval_payload(FnCtx&, const CharLiteral& x, const Expr&) {
  return std::to_string(x.value);
}

std::string StmtEmitter::emit_rval_payload(FnCtx& ctx, const StringLiteral& sl, const Expr&) {
  if (sl.is_wide) {
    const auto vals = decode_wide_string_values(sl.raw);
    const std::string gname = "@.str" + std::to_string(module_->str_pool_idx++);
    std::string init = "[";
    for (size_t i = 0; i < vals.size(); ++i) {
      if (i) init += ", ";
      init += "i32 " + std::to_string(vals[i]);
    }
    init += "]";
    {
      lir::LirStringConst sc;
      sc.pool_name = gname;
      sc.raw_bytes = "[" + std::to_string(vals.size()) + " x i32] " + init;
      sc.byte_length = -1;
      module_->string_pool.push_back(std::move(sc));
    }
    const std::string tmp = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirGepOp{tmp, "[" + std::to_string(vals.size()) + " x i32]", gname,
                                   false, {"i64 0", "i64 0"}});
    return tmp;
  }
  std::string bytes = sl.raw;
  if (bytes.size() >= 2 && bytes.front() == '"' && bytes.back() == '"') {
    bytes = bytes.substr(1, bytes.size() - 2);
  }
  bytes = decode_c_escaped_bytes(bytes);
  const std::string gname = intern_str(bytes);
  const size_t len = bytes.size() + 1;
  const std::string tmp = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{tmp, "[" + std::to_string(len) + " x i8]", gname, false,
                                 {"i64 0", "i64 0"}});
  return tmp;
}

std::string StmtEmitter::emit_rval_payload(FnCtx& ctx, const DeclRef& r, const Expr& e) {
  if (r.param_index && ctx.fn && *r.param_index < ctx.fn->params.size()) {
    const auto spill_it = ctx.param_slots.find(*r.param_index + 0x80000000u);
    if (spill_it != ctx.param_slots.end()) {
      const TypeSpec& pts = ctx.fn->params[*r.param_index].type.spec;
      const std::string ty = llvm_ty(pts);
      const std::string tmp = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirLoadOp{tmp, ty, spill_it->second});
      return tmp;
    }
    const auto it = ctx.param_slots.find(*r.param_index);
    if (it != ctx.param_slots.end()) {
      const TypeSpec& pts = ctx.fn->params[*r.param_index].type.spec;
      if (amd64_fixed_aggregate_byval(mod_, pts)) {
        const std::string tmp = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirLoadOp{tmp, llvm_ty(pts), it->second});
        return tmp;
      }
      return it->second;
    }
    if (amd64_fixed_aggregate_byval(mod_, ctx.fn->params[*r.param_index].type.spec)) {
      const TypeSpec& pts = ctx.fn->params[*r.param_index].type.spec;
      const std::string pname = "%p." + sanitize_llvm_ident(ctx.fn->params[*r.param_index].name);
      ctx.param_slots[*r.param_index] = pname;
      const std::string tmp = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirLoadOp{tmp, llvm_ty(pts), pname});
      return tmp;
    }
  }

  if (r.local) {
    const auto it = ctx.local_slots.find(r.local->value);
    if (it == ctx.local_slots.end())
      throw std::runtime_error("StmtEmitter: local slot not found: " + r.name);
    const TypeSpec ts = ctx.local_types.at(r.local->value);
    const auto vit = ctx.local_is_vla.find(r.local->value);
    if (vit != ctx.local_is_vla.end() && vit->second) {
      const std::string tmp = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirLoadOp{tmp, std::string("ptr"), it->second});
      return tmp;
    }
    if (ts.base == TB_VA_LIST && ts.ptr_level == 0 && ts.array_rank == 0) {
      return it->second;
    }
    if (ts.array_rank > 0 && !ts.is_ptr_to_array) {
      const std::string tmp = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirGepOp{tmp, llvm_alloca_ty(ts), it->second, false,
                                     {"i64 0", "i64 0"}});
      return tmp;
    }
    const std::string ty = llvm_ty(ts);
    if (ty == "void") return "0";
    const std::string tmp = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirLoadOp{tmp, ty, it->second});
    return tmp;
  }

  if (r.global) {
    size_t gv_idx = r.global->value;
    const auto& gv0 = mod_.globals[gv_idx];
    if (const GlobalVar* best = select_global_object(gv0.name)) gv_idx = best->id.value;
    const auto& gv = mod_.globals[gv_idx];
    if (gv.type.spec.array_rank > 0 && !gv.type.spec.is_ptr_to_array) {
      const std::string tmp = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirGepOp{tmp, llvm_alloca_ty(gv.type.spec), llvm_global_sym(gv.name),
                                     false, {"i64 0", "i64 0"}});
      return tmp;
    }
    const std::string ty = llvm_ty(gv.type.spec);
    if (ty == "void") return "0";
    const std::string tmp = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirLoadOp{tmp, ty, llvm_global_sym(gv.name)});
    return tmp;
  }

  if (mod_.fn_index.count(r.name)) return llvm_global_sym(r.name);

  if ((r.name == "__func__" || r.name == "__FUNCTION__" || r.name == "__PRETTY_FUNCTION__") &&
      ctx.fn) {
    const std::string gname = intern_str(ctx.fn->name);
    const size_t len = ctx.fn->name.size() + 1;
    const std::string tmp = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirGepOp{tmp, "[" + std::to_string(len) + " x i8]", gname, false,
                                   {"i64 0", "i64 0"}});
    return tmp;
  }

  const TypeSpec ets = resolve_expr_type(ctx, e);
  if (!has_concrete_type(ets)) return "0";
  const std::string ty = llvm_ty(ets);
  if (ty == "void") return "0";
  const std::string tmp = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirLoadOp{tmp, ty, "@" + r.name});
  return tmp;
}

std::string StmtEmitter::emit_rval_payload(FnCtx& ctx, const UnaryExpr& u, const Expr& e) {
  if (u.op == UnaryOp::AddrOf) {
    TypeSpec pts{};
    try {
      return emit_lval(ctx, u.operand, pts);
    } catch (const std::runtime_error&) {
      TypeSpec obj_ts{};
      const std::string rval = emit_rval_id(ctx, u.operand, obj_ts);
      if (!has_concrete_type(obj_ts) || llvm_ty(obj_ts) == "void") throw;
      const std::string slot = fresh_tmp(ctx) + ".addrtmp";
      ctx.alloca_insts.push_back(lir::LirAllocaOp{slot, llvm_alloca_ty(obj_ts), "", 0});
      emit_lir_op(ctx, lir::LirStoreOp{llvm_ty(obj_ts), rval, slot});
      return slot;
    }
  }

  TypeSpec et = resolve_expr_type(ctx, e);
  TypeSpec op_ts{};
  const bool skip_rval = (u.op == UnaryOp::PreInc || u.op == UnaryOp::PreDec ||
                          u.op == UnaryOp::PostInc || u.op == UnaryOp::PostDec);
  const std::string val = skip_rval ? "" : emit_rval_id(ctx, u.operand, op_ts);
  if (skip_rval) op_ts = resolve_expr_type(ctx, u.operand);
  if (!has_concrete_type(et)) et = op_ts;
  const std::string ty = llvm_ty(et);
  const std::string op_ty = llvm_ty(op_ts);

  switch (u.op) {
    case UnaryOp::Plus: {
      if (op_ty != ty && !is_vector_value(op_ts) && !is_float_base(op_ts.base)) {
        const std::string ext = fresh_tmp(ctx);
        const bool is_signed = is_signed_int(op_ts.base);
        emit_lir_op(ctx, lir::LirCastOp{
                             ext, is_signed ? lir::LirCastKind::SExt : lir::LirCastKind::ZExt,
                             op_ty, val, ty});
        return ext;
      }
      return val;
    }
    case UnaryOp::Minus: {
      std::string promoted_val = val;
      std::string promoted_ty = op_ty;
      if (op_ty != ty && !is_vector_value(op_ts) && !is_float_base(op_ts.base)) {
        const std::string ext = fresh_tmp(ctx);
        const bool is_signed = is_signed_int(op_ts.base);
        emit_lir_op(ctx, lir::LirCastOp{
                             ext, is_signed ? lir::LirCastKind::SExt : lir::LirCastKind::ZExt,
                             op_ty, val, ty});
        promoted_val = ext;
        promoted_ty = ty;
      }
      const std::string tmp = fresh_tmp(ctx);
      if (is_vector_value(op_ts)) {
        emit_lir_op(ctx, lir::LirBinOp{tmp, "sub", ty, "zeroinitializer", val});
      } else if (is_float_base(op_ts.base)) {
        emit_lir_op(ctx, lir::LirBinOp{tmp, "fneg", ty, val, ""});
      } else {
        emit_lir_op(ctx, lir::LirBinOp{tmp, "sub", promoted_ty, "0", promoted_val});
      }
      return tmp;
    }
    case UnaryOp::Not: {
      const std::string cmp = to_bool(ctx, val, op_ts);
      const std::string inv = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirBinOp{inv, "xor", "i1", cmp, "true"});
      if (ty == "i1") return inv;
      const std::string tmp = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirCastOp{tmp, lir::LirCastKind::ZExt, "i1", inv, ty});
      return tmp;
    }
    case UnaryOp::BitNot: {
      if (is_complex_base(op_ts.base)) {
        const TypeSpec elem_ts = complex_component_ts(op_ts.base);
        const std::string real_v = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirExtractValueOp{real_v, op_ty, val, 0});
        const std::string imag_v0 = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirExtractValueOp{imag_v0, op_ty, val, 1});
        const std::string imag_v = fresh_tmp(ctx);
        if (is_float_base(elem_ts.base)) {
          emit_lir_op(ctx, lir::LirBinOp{imag_v, "fneg", llvm_ty(elem_ts), imag_v0, ""});
        } else {
          emit_lir_op(ctx, lir::LirBinOp{imag_v, "sub", llvm_ty(elem_ts), "0", imag_v0});
        }
        const std::string elem_ty_str = llvm_ty(elem_ts);
        const std::string with_real = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirInsertValueOp{with_real, op_ty, "undef", elem_ty_str, real_v, 0});
        const std::string out = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirInsertValueOp{out, op_ty, with_real, elem_ty_str, imag_v, 1});
        return out;
      }
      const std::string tmp = fresh_tmp(ctx);
      if (is_vector_value(op_ts)) {
        TypeSpec elem_ts = op_ts;
        elem_ts.is_vector = false;
        elem_ts.vector_lanes = 0;
        elem_ts.vector_bytes = 0;
        std::string mask = "<";
        for (long long i = 0; i < op_ts.vector_lanes; ++i) {
          if (i) mask += ", ";
          mask += llvm_ty(elem_ts) + " -1";
        }
        mask += ">";
        emit_lir_op(ctx, lir::LirBinOp{tmp, "xor", ty, val, mask});
      } else {
        std::string promoted_val = val;
        std::string promoted_ty = op_ty;
        if (op_ty != ty && !is_vector_value(op_ts)) {
          const std::string ext = fresh_tmp(ctx);
          const bool is_signed = is_signed_int(op_ts.base);
          emit_lir_op(ctx, lir::LirCastOp{
                               ext, is_signed ? lir::LirCastKind::SExt : lir::LirCastKind::ZExt,
                               op_ty, val, ty});
          promoted_val = ext;
          promoted_ty = ty;
        }
        emit_lir_op(ctx, lir::LirBinOp{tmp, "xor", promoted_ty, promoted_val, "-1"});
      }
      return tmp;
    }
    case UnaryOp::AddrOf:
      __builtin_unreachable();
    case UnaryOp::Deref: {
      TypeSpec load_ts = op_ts;
      if (load_ts.ptr_level > 0) {
        load_ts.ptr_level -= 1;
      } else if (load_ts.array_rank > 0) {
        load_ts.array_rank--;
        load_ts.array_size = -1;
      }
      if (load_ts.array_rank > 0) return val;
      if (load_ts.is_fn_ptr && load_ts.ptr_level == 0) return val;
      const std::string load_ty = llvm_ty(load_ts);
      const std::string tmp = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirLoadOp{tmp, load_ty, val});
      return tmp;
    }
    case UnaryOp::PreInc:
    case UnaryOp::PreDec: {
      const AssignableLValue lhs = emit_assignable_lval(ctx, u.operand);
      return emit_assignable_incdec_value(ctx, lhs, u.op == UnaryOp::PreInc, true);
    }
    case UnaryOp::PostInc:
    case UnaryOp::PostDec: {
      const AssignableLValue lhs = emit_assignable_lval(ctx, u.operand);
      return emit_assignable_incdec_value(ctx, lhs, u.op == UnaryOp::PostInc, false);
    }
    case UnaryOp::RealPart:
    case UnaryOp::ImagPart: {
      try {
        TypeSpec complex_ts{};
        const std::string complex_ptr = emit_lval(ctx, u.operand, complex_ts);
        if (!is_complex_base(complex_ts.base)) throw std::runtime_error("non-complex");
        const TypeSpec part_ts = complex_component_ts(complex_ts.base);
        const std::string part_ptr = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirGepOp{part_ptr, llvm_alloca_ty(complex_ts), complex_ptr, false,
                                       {"i32 0", "i32 " +
                                                     std::to_string(u.op == UnaryOp::ImagPart ? 1 : 0)}});
        const std::string tmp = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirLoadOp{tmp, llvm_ty(part_ts), part_ptr});
        return tmp;
      } catch (const std::runtime_error&) {
      }
      if (!is_complex_base(op_ts.base)) return "0";
      const std::string tmp = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirExtractValueOp{tmp, llvm_ty(op_ts), val,
                                              u.op == UnaryOp::ImagPart ? 1 : 0});
      return tmp;
    }
  }
  return "0";
}


std::string StmtEmitter::emit_rval_payload(FnCtx& ctx, const AssignExpr& a, const Expr&) {
  TypeSpec rhs_ts{};
  std::string rhs = emit_rval_id(ctx, a.rhs, rhs_ts);
  const AssignableLValue lhs = emit_assignable_lval(ctx, a.lhs);
  if (a.op == AssignOp::Set) return emit_set_assign_value(ctx, lhs, rhs, rhs_ts);
  return emit_compound_assign_value(ctx, lhs, a.op, rhs, rhs_ts);
}

std::string StmtEmitter::emit_rval_payload(FnCtx& ctx, const CastExpr& c, const Expr&) {
  TypeSpec from_ts{};
  const std::string val = emit_rval_id(ctx, c.expr, from_ts);
  return coerce(ctx, val, from_ts, c.to_type.spec);
}

std::string StmtEmitter::emit_rval_payload(FnCtx& ctx, const TernaryExpr& t, const Expr& e) {
  TypeSpec cond_ts{};
  const std::string cond_v = emit_rval_id(ctx, t.cond, cond_ts);
  const std::string cond_i1 = to_bool(ctx, cond_v, cond_ts);

  const std::string then_lbl = fresh_lbl(ctx, "tern.then.");
  const std::string then_end_lbl = fresh_lbl(ctx, "tern.then.end.");
  const std::string else_lbl = fresh_lbl(ctx, "tern.else.");
  const std::string else_end_lbl = fresh_lbl(ctx, "tern.else.end.");
  const std::string end_lbl = fresh_lbl(ctx, "tern.end.");
  TypeSpec res_spec = resolve_expr_type(ctx, e);
  if (!has_concrete_type(res_spec)) res_spec.base = TB_INT;
  const std::string res_ty = llvm_ty(res_spec);

  emit_condbr_and_open_lbl(ctx, cond_i1, then_lbl, else_lbl, then_lbl);
  TypeSpec then_ts{};
  std::string then_v = emit_rval_id(ctx, t.then_expr, then_ts);
  then_v = coerce(ctx, then_v, then_ts, res_spec);
  emit_fallthrough_lbl(ctx, then_end_lbl);
  emit_br_and_open_lbl(ctx, end_lbl, else_lbl);
  TypeSpec else_ts{};
  std::string else_v = emit_rval_id(ctx, t.else_expr, else_ts);
  else_v = coerce(ctx, else_v, else_ts, res_spec);
  emit_fallthrough_lbl(ctx, else_end_lbl);
  emit_fallthrough_lbl(ctx, end_lbl);
  if (res_ty == "void") return "";
  auto void_to_zero = [&](const std::string& v) -> std::string {
    if (!v.empty()) return v;
    if (res_ty == "ptr") return "null";
    if (res_ty == "float" || res_ty == "double") return "0.0";
    return "0";
  };
  const std::string tmp = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirPhiOp{
                       tmp, res_ty, {{void_to_zero(then_v), then_end_lbl}, {void_to_zero(else_v), else_end_lbl}}});
  return tmp;
}

std::string StmtEmitter::emit_rval_payload(FnCtx& ctx, const SizeofExpr& s, const Expr&) {
  const Expr& op = get_expr(s.expr);
  TypeSpec op_ts = resolve_expr_type(ctx, op);
  if (!has_concrete_type(op_ts)) {
    op_ts = std::visit([&](const auto& p) -> TypeSpec { return resolve_payload_type(ctx, p); },
                       op.payload);
  }
  if (const auto* sl = std::get_if<StringLiteral>(&op.payload)) {
    if (sl->is_wide) {
      const auto vals = decode_wide_string_values(sl->raw);
      return std::to_string(vals.size() * 4);
    }
    const auto bytes = bytes_from_string_literal(*sl);
    return std::to_string(bytes.size() + 1);
  }
  if (const auto* r = std::get_if<DeclRef>(&op.payload)) {
    auto resolve_named_global_object_type = [&](const std::string& name) -> std::optional<TypeSpec> {
      if (const GlobalVar* best = select_global_object(name)) return best->type.spec;
      return std::nullopt;
    };
    if (r->global) {
      if (const GlobalVar* gv = select_global_object(*r->global)) op_ts = gv->type.spec;
    } else if (r->local) {
      const auto it = ctx.local_types.find(r->local->value);
      if (it != ctx.local_types.end()) op_ts = it->second;
    } else if (r->param_index && ctx.fn && *r->param_index < ctx.fn->params.size()) {
      op_ts = ctx.fn->params[*r->param_index].type.spec;
      if (op_ts.array_rank > 0) {
        op_ts.array_rank = 0;
        op_ts.array_size = -1;
        op_ts.ptr_level = 1;
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

std::string StmtEmitter::emit_rval_payload(FnCtx&, const SizeofTypeExpr& s, const Expr&) {
  return std::to_string(sizeof_ts(mod_, s.type.spec));
}

std::string StmtEmitter::emit_rval_payload(FnCtx& ctx, const LabelAddrExpr& la, const Expr&) {
  return "blockaddress(@" + ctx.fn->name + ", %ulbl_" + la.label_name + ")";
}

std::string StmtEmitter::emit_rval_payload(FnCtx&, const PendingConstevalExpr& p, const Expr&) {
  throw std::runtime_error(
      "StmtEmitter: unevaluated PendingConstevalExpr reached codegen for '" + p.fn_name + "'");
}

std::string StmtEmitter::emit_rval_payload(FnCtx& ctx, const IndexExpr&, const Expr& e) {
  if (const auto* idx = std::get_if<IndexExpr>(&e.payload)) {
    const TypeSpec base_ts = resolve_expr_type(ctx, idx->base);
    if (is_vector_value(base_ts)) {
      TypeSpec vec_ts{};
      const std::string vec = emit_rval_id(ctx, idx->base, vec_ts);
      TypeSpec ix_ts{};
      std::string ix = emit_rval_id(ctx, idx->index, ix_ts);
      TypeSpec i32_ts{};
      i32_ts.base = TB_INT;
      ix = coerce(ctx, ix, ix_ts, i32_ts);
      TypeSpec elem_ts = base_ts;
      elem_ts.is_vector = false;
      elem_ts.vector_lanes = 0;
      elem_ts.vector_bytes = 0;
      const std::string tmp = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirExtractElementOp{tmp, llvm_ty(base_ts), vec, "i32", ix});
      return tmp;
    }
  }
  TypeSpec pts{};
  const std::string ptr = emit_lval_dispatch(ctx, e, pts);
  return emit_rval_from_access_expr(ctx, e, ptr, pts, false);
}

std::string StmtEmitter::emit_rval_payload(FnCtx& ctx, const MemberExpr& m, const Expr& e) {
  TypeSpec field_ts{};
  BitfieldAccess bf;
  const std::string gep = emit_member_lval(ctx, m, field_ts, &bf);
  if (bf.is_bitfield()) {
    return emit_bitfield_load(ctx, gep, bf);
  }
  return emit_rval_from_access_expr(ctx, e, gep, field_ts, true);
}

}  // namespace c4c::codegen::lir
