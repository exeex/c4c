#include "../lowering.hpp"

namespace c4c::codegen::lir {

using namespace stmt_emitter_detail;

namespace {

std::string emitted_link_name(const c4c::hir::Module& mod, c4c::LinkNameId id,
                              std::string_view fallback) {
  const std::string_view resolved = mod.link_names.spelling(id);
  return resolved.empty() ? std::string(fallback) : std::string(resolved);
}

}  // namespace

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
  const std::string fn_name =
      ctx.fn ? emitted_link_name(mod_, ctx.fn->link_name_id, ctx.fn->name) : std::string();
  return "blockaddress(@" + fn_name + ", %ulbl_" + la.label_name + ")";
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
