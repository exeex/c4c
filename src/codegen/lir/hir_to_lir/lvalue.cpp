#include "lowering.hpp"
#include "../../llvm/calling_convention.hpp"

namespace c4c::codegen::lir {

namespace llvm_cc = c4c::codegen::llvm_backend;
using namespace stmt_emitter_detail;

namespace {

StructNameId indexed_gep_structured_name_id(const c4c::hir::Module& mod,
                                            const lir::LirModule* module,
                                            const TypeSpec& elem_ts) {
  if (!module || (elem_ts.base != TB_STRUCT && elem_ts.base != TB_UNION) ||
      elem_ts.ptr_level != 0 || elem_ts.array_rank != 0) {
    return kInvalidStructName;
  }

  const std::optional<HirRecordOwnerKey> owner_key =
      typespec_aggregate_owner_key(elem_ts, mod);
  if (!owner_key) return kInvalidStructName;
  const SymbolName* structured_tag = mod.find_struct_def_tag_by_owner(*owner_key);
  if (!structured_tag || structured_tag->empty()) return kInvalidStructName;

  const StructNameId name_id =
      module->struct_names.find(llvm_struct_type_str(*structured_tag));
  return module->find_struct_decl(name_id) ? name_id : kInvalidStructName;
}

std::string emitted_link_name(const c4c::hir::Module& mod, c4c::LinkNameId id,
                              std::string_view fallback) {
  const std::string_view resolved = mod.link_names.spelling(id);
  return resolved.empty() ? std::string(fallback) : std::string(resolved);
}

std::optional<std::string> member_access_owner_tag_from_type(const c4c::hir::Module& mod,
                                                             const TypeSpec& ts) {
  if (const std::optional<HirRecordOwnerKey> owner_key =
          typespec_aggregate_owner_key(ts, mod)) {
    const SymbolName* structured_tag = mod.find_struct_def_tag_by_owner(*owner_key);
    if (structured_tag && !structured_tag->empty()) return *structured_tag;
  }
  return typespec_aggregate_compatibility_tag(mod, ts);
}

LirTypeRef lir_aggregate_gep_type_ref(const std::string& rendered_text,
                                      lir::LirModule* module, StructNameId name_id,
                                      bool is_union) {
  if (!module) return LirTypeRef(rendered_text);
  if (name_id == kInvalidStructName) name_id = module->struct_names.intern(rendered_text);
  return is_union ? LirTypeRef::union_type(rendered_text, name_id)
                  : LirTypeRef::struct_type(rendered_text, name_id);
}

}

// Draft-only staging file for Step 3 of the stmt_emitter split refactor.
// This file owns the lvalue, member access, and assignable-store cluster.

std::string StmtEmitter::emit_member_gep(FnCtx& ctx, const std::string& base_ptr,
                                         const std::vector<FieldStep>& chain) {
  std::string cur_ptr = base_ptr;
  for (const auto& step : chain) {
    const std::string sty = llvm_struct_type_str(step.tag);
    const LirTypeRef sty_ref =
        lir_aggregate_gep_type_ref(sty, module_, step.structured_name_id, step.is_union);
    if (step.is_union) {
      const std::string tmp = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirGepOp{tmp, sty_ref, cur_ptr, false, {"i32 0", "i32 0"}});
      cur_ptr = tmp;
    } else {
      const std::string tmp = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirGepOp{
                           tmp, sty_ref, cur_ptr, false,
                           {"i32 0", "i32 " + std::to_string(step.llvm_idx)}});
      cur_ptr = tmp;
    }
  }
  return cur_ptr;
}

std::string StmtEmitter::emit_bitfield_load(FnCtx& ctx, const std::string& unit_ptr,
                                            const BitfieldAccess& bf) {
  const std::string result = fresh_tmp(ctx);
  const std::string unit_ty = "i" + std::to_string(bf.storage_unit_bits);
  const int promoted_bits = bitfield_promoted_bits(bf);
  const std::string promoted_ty = "i" + std::to_string(promoted_bits);

  const std::string unit = result + ".bf.unit";
  emit_lir_op(ctx, lir::LirLoadOp{unit, unit_ty, unit_ptr});

  std::string shifted = unit;
  if (bf.bit_offset > 0) {
    shifted = result + ".bf.shr";
    emit_lir_op(
        ctx, lir::LirBinOp{shifted, "lshr", unit_ty, unit, std::to_string(bf.bit_offset)});
  }

  const unsigned long long mask = (bf.bit_width >= 64) ? ~0ULL : ((1ULL << bf.bit_width) - 1);
  const std::string masked = result + ".bf.mask";
  emit_lir_op(ctx, lir::LirBinOp{masked, "and", unit_ty, shifted, std::to_string(mask)});

  std::string cur = masked;
  if (bf.is_signed && bf.bit_width < bf.storage_unit_bits) {
    const int shift_amt = bf.storage_unit_bits - bf.bit_width;
    const std::string shl_tmp = result + ".bf.shl";
    emit_lir_op(ctx, lir::LirBinOp{shl_tmp, "shl", unit_ty, masked, std::to_string(shift_amt)});
    cur = result + ".bf.sext";
    emit_lir_op(ctx, lir::LirBinOp{cur, "ashr", unit_ty, shl_tmp, std::to_string(shift_amt)});
  }

  if (bf.storage_unit_bits != promoted_bits) {
    if (bf.storage_unit_bits > promoted_bits) {
      emit_lir_op(ctx,
                  lir::LirCastOp{result, lir::LirCastKind::Trunc, unit_ty, cur, promoted_ty});
    } else {
      emit_lir_op(ctx,
                  lir::LirCastOp{result,
                                 bf.is_signed ? lir::LirCastKind::SExt : lir::LirCastKind::ZExt,
                                 unit_ty, cur, promoted_ty});
    }
  } else {
    emit_lir_op(ctx, lir::LirBinOp{result, "add", unit_ty, cur, "0"});
  }

  return result;
}

void StmtEmitter::emit_bitfield_store(FnCtx& ctx, const std::string& unit_ptr,
                                      const BitfieldAccess& bf, const std::string& new_val,
                                      const TypeSpec& val_ts) {
  TypeSpec unit_ts{};
  switch (bf.storage_unit_bits) {
    case 8:
      unit_ts.base = TB_UCHAR;
      break;
    case 16:
      unit_ts.base = TB_USHORT;
      break;
    case 32:
      unit_ts.base = TB_UINT;
      break;
    case 64:
      unit_ts.base = TB_ULONGLONG;
      break;
    default:
      unit_ts.base = TB_UINT;
      break;
  }
  std::string val_coerced = coerce(ctx, new_val, val_ts, unit_ts);
  const std::string scratch = fresh_tmp(ctx);
  const std::string unit_ty = "i" + std::to_string(bf.storage_unit_bits);

  const std::string old_unit = scratch + ".bf.old";
  emit_lir_op(ctx, lir::LirLoadOp{old_unit, unit_ty, unit_ptr});

  const unsigned long long field_mask_val =
      (bf.bit_width >= 64) ? ~0ULL : ((1ULL << bf.bit_width) - 1);
  const unsigned long long clear_mask = ~(field_mask_val << bf.bit_offset);
  const std::string cleared = scratch + ".bf.clr";
  emit_lir_op(ctx, lir::LirBinOp{cleared, "and", unit_ty, old_unit,
                                 std::to_string(static_cast<long long>(clear_mask))});

  const std::string new_masked = scratch + ".bf.vm";
  emit_lir_op(
      ctx, lir::LirBinOp{new_masked, "and", unit_ty, val_coerced, std::to_string(field_mask_val)});

  std::string new_shifted = new_masked;
  if (bf.bit_offset > 0) {
    new_shifted = scratch + ".bf.vs";
    emit_lir_op(ctx, lir::LirBinOp{new_shifted, "shl", unit_ty, new_masked,
                                   std::to_string(bf.bit_offset)});
  }

  const std::string combined = scratch + ".bf.comb";
  emit_lir_op(ctx, lir::LirBinOp{combined, "or", unit_ty, cleared, new_shifted});
  emit_lir_op(ctx, lir::LirStoreOp{unit_ty, combined, unit_ptr});
}

std::string StmtEmitter::emit_lval(FnCtx& ctx, ExprId id, TypeSpec& pointee_ts) {
  const Expr& e = get_expr(id);
  return emit_lval_dispatch(ctx, e, pointee_ts);
}

std::string StmtEmitter::emit_va_list_obj_ptr(FnCtx& ctx, ExprId id, TypeSpec& ts) {
  const Expr& e = get_expr(id);
  ts = resolve_expr_type(ctx, id);
  if (const auto* r = std::get_if<DeclRef>(&e.payload)) {
    if (r->param_index && ctx.fn && *r->param_index < ctx.fn->params.size()) {
      const TypeSpec& pts = ctx.fn->params[*r->param_index].type.spec;
      if (pts.base == TB_VA_LIST && pts.ptr_level == 0 && pts.array_rank == 0) {
        const auto spill_it = ctx.param_slots.find(*r->param_index + 0x80000000u);
        if (spill_it != ctx.param_slots.end()) {
          const std::string tmp = fresh_tmp(ctx);
          emit_lir_op(ctx, lir::LirLoadOp{tmp, std::string("ptr"), spill_it->second});
          return tmp;
        }
        const auto it = ctx.param_slots.find(*r->param_index);
        if (it != ctx.param_slots.end()) return it->second;
      }
    }
  }
  return emit_lval(ctx, id, ts);
}

std::string StmtEmitter::emit_lval_dispatch(FnCtx& ctx, const Expr& e, TypeSpec& pts) {
  if (const auto* r = std::get_if<DeclRef>(&e.payload)) {
    if (r->local) {
      pts = ctx.local_types.at(r->local->value);
      return ctx.local_slots.at(r->local->value);
    }
    if (r->param_index && ctx.fn && *r->param_index < ctx.fn->params.size()) {
      const auto& param = ctx.fn->params[*r->param_index];
      pts = param.type.spec;
      const std::string pname = "%p." + sanitize_llvm_ident(param.name);
      if (amd64_fixed_aggregate_byval(mod_, pts)) {
        auto it = ctx.param_slots.find(*r->param_index + 0x80000000u);
        if (it != ctx.param_slots.end()) {
          return it->second;
        }
        const auto direct_it = ctx.param_slots.find(*r->param_index);
        if (direct_it != ctx.param_slots.end()) {
          return direct_it->second;
        }
        ctx.param_slots[*r->param_index] = pname;
        return pname;
      }
      auto it = ctx.param_slots.find(*r->param_index + 0x80000000u);
      if (it != ctx.param_slots.end()) {
        return it->second;
      }
      const std::string slot = "%lv.param." + sanitize_llvm_ident(param.name);
      ctx.alloca_insts.push_back(lir::LirAllocaOp{slot, llvm_alloca_ty(pts), "", 0});
      if (amd64_fixed_aggregate_byval(mod_, pts)) {
        module_->need_memcpy = true;
        emit_lir_op(ctx, lir::LirMemcpyOp{
                             slot, pname, std::to_string(llvm_cc::amd64_type_size_bytes(pts, mod_)),
                             false});
      } else {
        ctx.alloca_insts.push_back(lir::LirStoreOp{llvm_ty(pts), pname, slot});
      }
      ctx.param_slots[*r->param_index + 0x80000000u] = slot;
      return slot;
    }
    if (r->global) {
      size_t gv_idx = r->global->value;
      const auto& gv0 = mod_.globals[gv_idx];
      if (const GlobalVar* best = select_global_object(gv0.name)) gv_idx = best->id.value;
      const auto& gv = mod_.globals[gv_idx];
      pts = gv.type.spec;
      return llvm_global_sym(emitted_link_name(mod_, gv.link_name_id, gv.name));
    }
    pts = e.type.spec;
    if (const Function* fn = find_local_target_function(r->link_name_id, r->name);
        fn != nullptr) {
      return llvm_global_sym(emitted_link_name(mod_, fn->link_name_id, fn->name));
    }
    return llvm_global_sym(r->name);
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
        throw std::runtime_error("StmtEmitter: real/imag lvalue on non-complex expr");
      }
      pts = complex_component_ts(complex_ts.base);
      const std::string tmp = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirGepOp{tmp, llvm_alloca_ty(complex_ts), complex_ptr, false,
                                     {"i32 0", "i32 " +
                                                    std::to_string(u->op == UnaryOp::ImagPart ? 1 : 0)}});
      return tmp;
    }
  }
  if (const auto* idx = std::get_if<IndexExpr>(&e.payload)) {
    TypeSpec base_ts{};
    std::string base;
    if (const TypeSpec resolved_base_ts = resolve_expr_type(ctx, idx->base);
        is_vector_value(resolved_base_ts)) {
      TypeSpec obj_ts{};
      base = emit_lval(ctx, idx->base, obj_ts);
      base_ts = obj_ts;
    } else {
      base = emit_rval_id(ctx, idx->base, base_ts);
    }
    TypeSpec ix_ts{};
    const std::string ix = emit_rval_id(ctx, idx->index, ix_ts);
    TypeSpec i64_ts{};
    i64_ts.base = TB_LONGLONG;
    const std::string ix64 = coerce(ctx, ix, ix_ts, i64_ts);
    pts = base_ts;
    if (is_vector_value(pts)) {
      pts.is_vector = false;
      pts.vector_lanes = 0;
      pts.vector_bytes = 0;
      const std::string tmp = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirGepOp{tmp, llvm_ty(pts), base, false, {"i64 " + ix64}});
      return tmp;
    } else {
      pts = resolve_indexed_gep_pointee_type(pts);
      return emit_indexed_gep(ctx, base, base_ts, ix64,
                              indexed_gep_structured_name_id(mod_, module_, pts));
    }
  }
  if (const auto* m = std::get_if<MemberExpr>(&e.payload)) {
    return emit_member_lval(ctx, *m, pts);
  }
  if (const auto* call = std::get_if<CallExpr>(&e.payload)) {
    TypeSpec storage_ts = resolve_payload_type(ctx, *call);
    if (storage_ts.is_lvalue_ref || storage_ts.is_rvalue_ref) {
      pts = storage_ts;
      if (pts.ptr_level > 0) pts.ptr_level--;
      pts.is_lvalue_ref = false;
      pts.is_rvalue_ref = false;
      return emit_rval_payload(ctx, *call, e);
    }
  }
  if (const auto* assign = std::get_if<AssignExpr>(&e.payload)) {
    TypeSpec rhs_ts{};
    const std::string rhs = emit_rval_id(ctx, assign->rhs, rhs_ts);
    const AssignableLValue lhs = emit_assignable_lval(ctx, assign->lhs);
    if (assign->op == AssignOp::Set) {
      (void)emit_set_assign_value(ctx, lhs, rhs, rhs_ts);
    } else {
      (void)emit_compound_assign_value(ctx, lhs, assign->op, rhs, rhs_ts);
    }
    pts = lhs.pointee_ts;
    return lhs.ptr;
  }
  if (const auto* c = std::get_if<CastExpr>(&e.payload)) {
    if (c->to_type.spec.is_rvalue_ref || c->to_type.spec.is_lvalue_ref) {
      return emit_lval(ctx, c->expr, pts);
    }
  }
  throw std::runtime_error("StmtEmitter: cannot take lval of expr");
}

std::string StmtEmitter::emit_member_lval(FnCtx& ctx, const MemberExpr& m, TypeSpec& out_pts,
                                          BitfieldAccess* out_bf) {
  MemberFieldAccess access = resolve_member_field_access(ctx, m);
  const std::string base_ptr = emit_member_base_ptr(ctx, m, access.base_ts);

  if (!access.has_tag()) {
    throw std::runtime_error("StmtEmitter: MemberExpr base has no struct tag (field='" + m.field +
                             "')");
  }
  if (!access.field_found) {
    throw std::runtime_error("StmtEmitter: field '" + m.field +
                             "' not found in struct/union '" + std::string(access.tag) + "'");
  }
  out_pts = access.field_ts;
  if (out_bf) *out_bf = access.bf;
  return emit_member_gep(ctx, base_ptr, access.chain);
}

AssignableLValue StmtEmitter::emit_assignable_lval(FnCtx& ctx, ExprId id) {
  AssignableLValue access;
  const Expr& e = get_expr(id);
  if (const auto* m = std::get_if<MemberExpr>(&e.payload)) {
    access.ptr = emit_member_lval(ctx, *m, access.pointee_ts, &access.bf);
    return access;
  }
  access.ptr = emit_lval(ctx, id, access.pointee_ts);
  return access;
}

LoadedAssignableValue StmtEmitter::emit_load_assignable_value(FnCtx& ctx,
                                                              const AssignableLValue& lhs) {
  LoadedAssignableValue loaded;
  if (lhs.is_bitfield()) {
    loaded.value_ts = bitfield_promoted_ts(lhs.bf);
    loaded.value = emit_bitfield_load(ctx, lhs.ptr, lhs.bf);
    return loaded;
  }

  loaded.value_ts = lhs.pointee_ts;
  loaded.value = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirLoadOp{loaded.value, llvm_ty(lhs.pointee_ts), lhs.ptr});
  return loaded;
}

std::string StmtEmitter::emit_store_assignable_value(FnCtx& ctx, const AssignableLValue& lhs,
                                                     const std::string& value,
                                                     const TypeSpec& value_ts,
                                                     bool reload_after_store) {
  if (lhs.is_bitfield()) {
    emit_bitfield_store(ctx, lhs.ptr, lhs.bf, value, value_ts);
    return reload_after_store ? emit_bitfield_load(ctx, lhs.ptr, lhs.bf) : value;
  }
  const bool zero_init_aggregate =
      value == "zeroinitializer" &&
      (lhs.pointee_ts.array_rank > 0 ||
       (lhs.pointee_ts.ptr_level == 0 &&
        (lhs.pointee_ts.base == TB_STRUCT || lhs.pointee_ts.base == TB_UNION)));
  if (zero_init_aggregate) {
    module_->need_memset = true;
    emit_lir_op(ctx, lir::LirMemsetOp{lhs.ptr, "0",
                                      std::to_string(sizeof_ts(mod_, lhs.pointee_ts)), false});
    return value;
  }
  emit_lir_op(ctx, lir::LirStoreOp{llvm_ty(lhs.pointee_ts), value, lhs.ptr});
  return value;
}

std::string StmtEmitter::emit_assignable_incdec_value(FnCtx& ctx, const AssignableLValue& lhs,
                                                      bool increment,
                                                      bool return_updated_value) {
  if (lhs.is_bitfield()) {
    const LoadedAssignableValue loaded = emit_load_assignable_value(ctx, lhs);
    const std::string pty = llvm_ty(loaded.value_ts);
    const std::string delta = increment ? "1" : "-1";
    const std::string new_val = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirBinOp{new_val, "add", pty, loaded.value, delta});
    const std::string stored =
        emit_store_assignable_value(ctx, lhs, new_val, loaded.value_ts, return_updated_value);
    return return_updated_value ? stored : loaded.value;
  }

  const LoadedAssignableValue loaded = emit_load_assignable_value(ctx, lhs);
  const std::string pty = llvm_ty(loaded.value_ts);

  const std::string new_val = fresh_tmp(ctx);
  if (pty == "ptr") {
    const std::string delta = increment ? "1" : "-1";
    emit_lir_op(ctx, lir::LirGepOp{new_val, indexed_gep_elem_ty(lhs.pointee_ts), loaded.value,
                                   false, {"i64 " + delta}});
  } else if (is_float_base(loaded.value_ts.base)) {
    const std::string delta = increment ? "1.0" : "-1.0";
    emit_lir_op(ctx, lir::LirBinOp{new_val, "fadd", pty, loaded.value, delta});
  } else {
    const std::string delta = increment ? "1" : "-1";
    emit_lir_op(ctx, lir::LirBinOp{new_val, "add", pty, loaded.value, delta});
  }
  emit_store_assignable_value(ctx, lhs, new_val, lhs.pointee_ts, false);
  return return_updated_value ? new_val : loaded.value;
}

std::string StmtEmitter::emit_set_assign_value(FnCtx& ctx, const AssignableLValue& lhs,
                                               const std::string& rhs,
                                               const TypeSpec& rhs_ts) {
  if (lhs.is_bitfield()) {
    return emit_store_assignable_value(ctx, lhs, rhs, rhs_ts, true);
  }

  std::string coerced_rhs = coerce(ctx, rhs, rhs_ts, lhs.pointee_ts);
  const bool is_agg = (lhs.pointee_ts.base == TB_STRUCT || lhs.pointee_ts.base == TB_UNION) &&
                      lhs.pointee_ts.ptr_level == 0 && lhs.pointee_ts.array_rank == 0;
  if (is_agg && (coerced_rhs == "0" || coerced_rhs.empty())) coerced_rhs = "zeroinitializer";
  return emit_store_assignable_value(ctx, lhs, coerced_rhs, lhs.pointee_ts, false);
}

std::string StmtEmitter::emit_compound_assign_value(FnCtx& ctx, const AssignableLValue& lhs,
                                                    AssignOp op, const std::string& rhs,
                                                    const TypeSpec& rhs_ts) {
  const TypeSpec& lhs_ts = lhs.pointee_ts;
  const std::string lty = llvm_ty(lhs_ts);

  if (lhs.is_bitfield()) {
    const LoadedAssignableValue loaded = emit_load_assignable_value(ctx, lhs);
    const std::string promoted_ty = llvm_ty(loaded.value_ts);
    std::string rhs_op = coerce(ctx, rhs, rhs_ts, loaded.value_ts);
    const bool ls = is_signed_int(loaded.value_ts.base);
    const char* instr = nullptr;
    static const struct {
      AssignOp op;
      const char* is;
      const char* iu;
    } tbl[] = {{AssignOp::Add, "add", "add"},
               {AssignOp::Sub, "sub", "sub"},
               {AssignOp::Mul, "mul", "mul"},
               {AssignOp::Div, "sdiv", "udiv"},
               {AssignOp::Mod, "srem", "urem"},
               {AssignOp::Shl, "shl", "shl"},
               {AssignOp::Shr, "ashr", "lshr"},
               {AssignOp::BitAnd, "and", "and"},
               {AssignOp::BitOr, "or", "or"},
               {AssignOp::BitXor, "xor", "xor"}};
    for (const auto& r : tbl) {
      if (r.op == op) {
        instr = ls ? r.is : r.iu;
        break;
      }
    }
    if (!instr) throw std::runtime_error("StmtEmitter: bitfield compound assign: unknown op");
    const std::string result = fresh_tmp(ctx);
    emit_lir_op(ctx,
                lir::LirBinOp{result, std::string(instr), promoted_ty, loaded.value, rhs_op});
    return emit_store_assignable_value(ctx, lhs, result, loaded.value_ts, false);
  }

  const LoadedAssignableValue loaded = emit_load_assignable_value(ctx, lhs);
  if ((op == AssignOp::Add || op == AssignOp::Sub) && lty == "ptr") {
    TypeSpec i64_ts{};
    i64_ts.base = TB_LONGLONG;
    std::string delta = coerce(ctx, rhs, rhs_ts, i64_ts);
    if (op == AssignOp::Sub) {
      const std::string neg = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirBinOp{neg, "sub", "i64", "0", delta});
      delta = neg;
    }
    const std::string result = fresh_tmp(ctx);
    emit_lir_op(ctx,
                lir::LirGepOp{result, indexed_gep_elem_ty(lhs_ts), loaded.value, false,
                              {"i64 " + delta}});
    return emit_store_assignable_value(ctx, lhs, result, lhs_ts, false);
  }

  static const struct {
    AssignOp op;
    BinaryOp bop;
  } compound_map[] = {{AssignOp::Add, BinaryOp::Add},
                      {AssignOp::Sub, BinaryOp::Sub},
                      {AssignOp::Mul, BinaryOp::Mul},
                      {AssignOp::Div, BinaryOp::Div},
                      {AssignOp::Mod, BinaryOp::Mod},
                      {AssignOp::Shl, BinaryOp::Shl},
                      {AssignOp::Shr, BinaryOp::Shr},
                      {AssignOp::BitAnd, BinaryOp::BitAnd},
                      {AssignOp::BitOr, BinaryOp::BitOr},
                      {AssignOp::BitXor, BinaryOp::BitXor}};
  const char* instr = nullptr;
  TypeSpec op_ts = lhs_ts;
  std::string coerced_rhs = rhs;
  for (const auto& row : compound_map) {
    if (row.op != op) continue;
    if ((row.bop == BinaryOp::Add || row.bop == BinaryOp::Sub || row.bop == BinaryOp::Mul ||
         row.bop == BinaryOp::Div) &&
        (is_complex_base(lhs_ts.base) || is_complex_base(rhs_ts.base))) {
      const std::string result =
          emit_complex_binary_arith(ctx, row.bop, loaded.value, lhs_ts, coerced_rhs, rhs_ts, lhs_ts);
      return emit_store_assignable_value(ctx, lhs, result, lhs_ts, false);
    }
    op_ts = resolve_compound_assign_op_type(row.bop, lhs_ts, rhs_ts);
    coerced_rhs = coerce(ctx, coerced_rhs, rhs_ts, op_ts);
    const bool lf = is_float_base(op_ts.base);
    const bool ls = is_signed_int(op_ts.base);
    static const struct {
      BinaryOp op;
      const char* is;
      const char* iu;
      const char* f;
    } tbl[] = {{BinaryOp::Add, "add", "add", "fadd"},
               {BinaryOp::Sub, "sub", "sub", "fsub"},
               {BinaryOp::Mul, "mul", "mul", "fmul"},
               {BinaryOp::Div, "sdiv", "udiv", "fdiv"},
               {BinaryOp::Mod, "srem", "urem", nullptr},
               {BinaryOp::Shl, "shl", "shl", nullptr},
               {BinaryOp::Shr, "ashr", "lshr", nullptr},
               {BinaryOp::BitAnd, "and", "and", nullptr},
               {BinaryOp::BitOr, "or", "or", nullptr},
               {BinaryOp::BitXor, "xor", "xor", nullptr}};
    for (const auto& r : tbl) {
      if (r.op == row.bop) {
        if (lf) {
          instr = r.f;
        } else if (r.op == BinaryOp::Shr) {
          instr = is_signed_int(lhs_ts.base) ? r.is : r.iu;
        } else {
          instr = ls ? r.is : r.iu;
        }
        break;
      }
    }
    if (!instr) break;
    return emit_nonptr_compound_assign_value(ctx, lhs, loaded, row.bop, instr, rhs, rhs_ts);
  }
  if (!instr) throw std::runtime_error("StmtEmitter: compound assign: unknown op");
  throw std::runtime_error("StmtEmitter: compound assign: unreachable non-pointer path");
}

TypeSpec StmtEmitter::resolve_compound_assign_op_type(BinaryOp op, const TypeSpec& lhs_ts,
                                                      const TypeSpec& rhs_ts) {
  TypeSpec op_ts = lhs_ts;
  if (is_vector_value(lhs_ts)) return op_ts;
  if (lhs_ts.ptr_level != 0 || lhs_ts.array_rank != 0 || rhs_ts.ptr_level != 0 ||
      rhs_ts.array_rank != 0) {
    return op_ts;
  }
  if (is_float_base(lhs_ts.base) || is_float_base(rhs_ts.base)) {
    op_ts.base = (lhs_ts.base == TB_DOUBLE || rhs_ts.base == TB_DOUBLE ||
                  lhs_ts.base == TB_LONGDOUBLE || rhs_ts.base == TB_LONGDOUBLE)
                     ? TB_DOUBLE
                     : TB_FLOAT;
    return op_ts;
  }
  if (is_any_int(lhs_ts.base) && is_any_int(rhs_ts.base)) {
    const bool is_shift = (op == BinaryOp::Shl || op == BinaryOp::Shr);
    op_ts.base = is_shift ? integer_promote(lhs_ts.base)
                          : usual_arith_conv(lhs_ts.base, rhs_ts.base);
  }
  return op_ts;
}

std::string StmtEmitter::emit_nonptr_compound_assign_value(
    FnCtx& ctx, const AssignableLValue& lhs, const LoadedAssignableValue& loaded, BinaryOp op,
    const char* instr, const std::string& rhs, const TypeSpec& rhs_ts) {
  const TypeSpec& lhs_ts = lhs.pointee_ts;
  const TypeSpec op_ts = resolve_compound_assign_op_type(op, lhs_ts, rhs_ts);
  const std::string op_ty = llvm_ty(op_ts);
  std::string lhs_op = loaded.value;
  if (op_ty != llvm_ty(lhs_ts)) lhs_op = coerce(ctx, loaded.value, lhs_ts, op_ts);
  const std::string rhs_op = coerce(ctx, rhs, rhs_ts, op_ts);
  const std::string result = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirBinOp{result, std::string(instr), op_ty, lhs_op, rhs_op});
  std::string store_v = result;
  if (op_ty != llvm_ty(lhs_ts)) store_v = coerce(ctx, result, op_ts, lhs_ts);
  return emit_store_assignable_value(ctx, lhs, store_v, lhs_ts, false);
}

TypeSpec StmtEmitter::resolve_member_base_type(FnCtx& ctx, ExprId base_id, bool is_arrow) {
  const Expr& base_e = get_expr(base_id);
  TypeSpec base_ts = base_e.type.spec;
  if (base_ts.base == TB_VOID && base_ts.ptr_level == 0) base_ts = resolve_expr_type(ctx, base_id);
  if (base_ts.base == TB_TYPEDEF) {
    TypeSpec resolved =
        std::visit([&](const auto& p) -> TypeSpec { return resolve_payload_type(ctx, p); },
                   base_e.payload);
    if (resolved.base != TB_VOID || resolved.ptr_level > 0 || resolved.array_rank > 0) {
      base_ts = resolved;
    }
  }
  if (is_arrow && base_ts.ptr_level > 0) base_ts.ptr_level--;
  return base_ts;
}

MemberFieldAccess StmtEmitter::resolve_member_field_access(FnCtx& ctx, const MemberExpr& m) {
  MemberFieldAccess access;
  access.base_ts = resolve_member_base_type(ctx, m.base, m.is_arrow);
  if (!m.resolved_owner_tag.empty()) {
    access.tag = m.resolved_owner_tag;
  } else if (const std::optional<std::string> owner_tag =
                 member_access_owner_tag_from_type(mod_, access.base_ts)) {
    access.tag = *owner_tag;
  }
  if (!access.has_tag()) return access;
  if (m.member_symbol_id != kInvalidMemberSymbol) {
    access.field_found = resolve_field_access_by_member_symbol_id(
        access.tag, m.member_symbol_id, access.chain, access.field_ts, &access.bf);
  }
  if (!access.field_found) {
    access.field_found =
        resolve_field_access(access.tag, m.field, access.chain, access.field_ts, &access.bf);
  }
  return access;
}

std::string StmtEmitter::emit_member_base_ptr(FnCtx& ctx, const MemberExpr& m, TypeSpec& base_ts) {
  if (m.is_arrow) {
    TypeSpec ptr_ts{};
    return emit_rval_id(ctx, m.base, ptr_ts);
  }

  const Expr& base_e = get_expr(m.base);
  TypeSpec dummy{};
  try {
    return emit_lval_dispatch(ctx, base_e, dummy);
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
    ctx.alloca_insts.push_back(lir::LirAllocaOp{slot, llvm_alloca_ty(base_ts), "", 0});
    emit_lir_op(ctx, lir::LirStoreOp{llvm_ty(base_ts), rval, slot});
    return slot;
  }
}

LirTypeRef StmtEmitter::indexed_gep_elem_ty(const TypeSpec& base_ts,
                                            StructNameId elem_structured_name_id) {
  const TypeSpec elem_ts = resolve_indexed_gep_pointee_type(base_ts);
  if (elem_ts.base == TB_VOID && elem_ts.ptr_level == 0 && elem_ts.array_rank == 0) {
    return LirTypeRef("i8");
  }
  std::string rendered_text;
  if (elem_ts.array_rank > 0 && elem_ts.ptr_level == 0) {
    rendered_text = llvm_alloca_ty(elem_ts);
  } else {
    rendered_text = llvm_ty(elem_ts);
  }
  if ((elem_ts.base == TB_STRUCT || elem_ts.base == TB_UNION) && elem_ts.ptr_level == 0 &&
      elem_ts.array_rank == 0) {
    if (elem_structured_name_id == kInvalidStructName) {
      elem_structured_name_id = indexed_gep_structured_name_id(mod_, module_, elem_ts);
    }
    const char* site = elem_structured_name_id == kInvalidStructName
                           ? "indexed-gep-aggregate-legacy-compat"
                           : "indexed-gep-aggregate";
    const StructuredLayoutLookup layout =
        lookup_structured_layout(mod_, module_, elem_ts, site, elem_structured_name_id);
    if (layout.structured_decl) {
      return lir_aggregate_gep_type_ref(rendered_text, module_, layout.structured_name_id,
                                        elem_ts.base == TB_UNION);
    }
  }
  return LirTypeRef(rendered_text);
}

std::string StmtEmitter::emit_indexed_gep(FnCtx& ctx, const std::string& base_ptr,
                                          const TypeSpec& base_ts, const std::string& idx,
                                          StructNameId elem_structured_name_id) {
  const std::string tmp = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{
                       tmp, indexed_gep_elem_ty(base_ts, elem_structured_name_id), base_ptr,
                       false, {"i64 " + idx}});
  return tmp;
}

std::string StmtEmitter::emit_rval_from_access_ptr(FnCtx& ctx, const std::string& ptr,
                                                   const TypeSpec& access_ts,
                                                   const TypeSpec& load_ts,
                                                   bool decay_from_array_object) {
  if (outer_array_rank(access_ts) > 0) {
    if (!decay_from_array_object) return ptr;
    const std::string arr_alloca_ty = llvm_alloca_ty(access_ts);
    if (arr_alloca_ty == "ptr") {
      const std::string tmp = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirLoadOp{tmp, std::string("ptr"), ptr});
      return tmp;
    }
    const std::string tmp = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirGepOp{tmp, arr_alloca_ty, ptr, false, {"i64 0", "i64 0"}});
    return tmp;
  }
  const std::string ty = llvm_ty(load_ts);
  if (ty == "void") return "";
  const std::string tmp = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirLoadOp{tmp, ty, ptr});
  return tmp;
}

std::string StmtEmitter::emit_rval_from_access_expr(FnCtx& ctx, const Expr& e,
                                                    const std::string& ptr,
                                                    const TypeSpec& access_ts,
                                                    bool decay_from_array_object) {
  TypeSpec load_ts = resolve_expr_type(ctx, e);
  if (!has_concrete_type(load_ts)) load_ts = access_ts;
  return emit_rval_from_access_ptr(ctx, ptr, access_ts, load_ts, decay_from_array_object);
}

}  // namespace c4c::codegen::lir
