#include "lowering.hpp"
#include "../../llvm/calling_convention.hpp"

namespace c4c::codegen::lir {

namespace llvm_cc = c4c::codegen::llvm_backend;
using namespace stmt_emitter_detail;

namespace stmt_emitter_detail {

LirOperand integer_store_operand_after_coercion(const LirOperand& source,
                                                std::string presentation,
                                                bool same_representation) {
  if (same_representation) {
    if (const LirIntegerImmediate* immediate = source.integer_immediate()) {
      return LirOperand::integer(std::move(presentation), immediate->value);
    }
  }
  return LirOperand::raw(std::move(presentation));
}

}  // namespace stmt_emitter_detail

namespace {

// Indexed-GEP element text is rendered from a resolved HIR TypeSpec. It can
// legitimately be an array, pointer, vector, or other non-builtin spelling,
// so retain it through this local, auditable runtime-text boundary.
[[nodiscard, deprecated(
                  "HIR-rendered indexed-GEP element type text: audit this runtime-text "
                  "compatibility boundary")]]
LirTypeRef hir_rendered_indexed_gep_element_type_text(
    std::string rendered_text) {
  return LirTypeRef::runtime_text(std::move(rendered_text));
}

StructNameId indexed_gep_structured_name_id(const c4c::hir::Module& mod,
                                            const lir::LirModule* module,
                                            const std::string& rendered_text,
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
  return normalize_lir_aggregate_struct_name_id(module, rendered_text, name_id, true);
}

std::string emitted_link_name(const c4c::hir::Module& mod, c4c::LinkNameId id,
                              std::string_view fallback) {
  const std::string_view resolved = mod.link_names.spelling(id);
  return resolved.empty() ? std::string(fallback) : std::string(resolved);
}

std::optional<std::string> member_access_owner_tag_from_type(const c4c::hir::Module& mod,
                                                             const TypeSpec& ts) {
  if (typespec_aggregate_complete_owner_key_missed(ts, mod)) return std::nullopt;
  if (const std::optional<HirRecordOwnerKey> owner_key =
          typespec_aggregate_owner_key(ts, mod)) {
    const SymbolName* structured_tag = mod.find_struct_def_tag_by_owner(*owner_key);
    if (structured_tag && !structured_tag->empty()) return *structured_tag;
    return std::nullopt;
  }
  return typespec_aggregate_compatibility_tag(mod, ts);
}

void populate_selected_byval_parameter_materialization_authority(
    lir::LirFunction& function) {
  if (function.selected_memcpy_pointer_authority.has_value()) return;
  if (function.link_name_id == kInvalidLinkName) return;

  function.selected_memcpy_pointer_authority =
      lir::LirSelectedMemcpyPointerAuthority{
          .byval_parameter = lir::LirCurrentFunctionPointerDefinition{
              .value = function.alloc_value(),
              .pointer_type = lir::LirTypeRef(lir::LirBuiltinType::Pointer),
              .object = function.alloc_object(),
              .object_owner = function.link_name_id,
              .role = lir::LirSelectedMemcpyPointerRole::ByvalParameter,
              .live_at_selected_site = true,
          },
          .destination_alloca = lir::LirCurrentFunctionPointerDefinition{
              .value = function.alloc_value(),
              .pointer_type = lir::LirTypeRef(lir::LirBuiltinType::Pointer),
              .object = function.alloc_object(),
              .object_owner = function.link_name_id,
              .role = lir::LirSelectedMemcpyPointerRole::DestinationAlloca,
              .live_at_selected_site = true,
          },
      };
}

std::optional<lir::LirSelectedMemcpyAuthority>
selected_byval_parameter_materialization_memcpy_authority(
    const lir::LirFunction& function, long long size_bytes) {
  const auto& selected = function.selected_memcpy_pointer_authority;
  if (!selected.has_value()) return std::nullopt;

  const auto& source = selected->byval_parameter;
  const auto& destination = selected->destination_alloca;
  if (!source.value.valid() || !destination.value.valid() ||
      !source.object.valid() || !destination.object.valid() ||
      source.pointer_type.kind() != lir::LirTypeKind::Pointer ||
      destination.pointer_type.kind() != lir::LirTypeKind::Pointer ||
      source.object_owner != function.link_name_id ||
      destination.object_owner != function.link_name_id ||
      !source.live_at_selected_site || !destination.live_at_selected_site ||
      size_bytes <= 0) {
    return std::nullopt;
  }

  return lir::LirSelectedMemcpyAuthority{
      .destination = destination.value,
      .source = source.value,
      .size = lir::LirIntegerImmediate{size_bytes},
      .destination_object = destination.object,
      .source_object = source.object,
      .destination_object_owner = destination.object_owner,
      .source_object_owner = source.object_owner,
      .destination_live_at_site = destination.live_at_selected_site,
      .source_live_at_site = source.live_at_selected_site,
  };
}

LirTypeRef lir_aggregate_gep_type_ref(const std::string& rendered_text,
                                      lir::LirModule* module, StructNameId name_id,
                                      bool is_union) {
  if (!module) return LirTypeRef(rendered_text);
  name_id = normalize_lir_aggregate_struct_name_id(module, rendered_text, name_id, false);
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
      if (step.packed_storage_offset_bytes >= 0) {
        const std::string byte_ptr = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirGepOp{
                             byte_ptr, "i8", cur_ptr, false,
                             {"i64 " + std::to_string(step.packed_storage_offset_bytes)}});
        cur_ptr = byte_ptr;
      }
    }
  }
  return cur_ptr;
}

LirOperand StmtEmitter::emit_bitfield_load(FnCtx& ctx, const std::string& unit_ptr,
                                           const BitfieldAccess& bf) {
  const std::string unit_ty = "i" + std::to_string(bf.storage_unit_bits);
  const int promoted_bits = bitfield_promoted_bits(bf);
  const std::string promoted_ty = "i" + std::to_string(promoted_bits);

  const LirOperand unit = fresh_value(ctx);
  emit_lir_op(ctx, lir::LirLoadOp{unit, unit_ty, unit_ptr});

  LirOperand shifted = unit;
  if (bf.bit_offset > 0) {
    shifted = fresh_value(ctx);
    emit_lir_op(ctx, lir::LirBinOp{
                         shifted, "lshr", unit_ty, unit,
                         LirOperand::integer(std::to_string(bf.bit_offset), bf.bit_offset)});
  }

  const unsigned long long mask = (bf.bit_width >= 64) ? ~0ULL : ((1ULL << bf.bit_width) - 1);
  const LirOperand masked = fresh_value(ctx);
  emit_lir_op(ctx, lir::LirBinOp{
                       masked, "and", unit_ty, shifted,
                       LirOperand::integer(std::to_string(mask), static_cast<long long>(mask))});

  LirOperand cur = masked;
  if (bf.is_signed && bf.bit_width < bf.storage_unit_bits) {
    const int shift_amt = bf.storage_unit_bits - bf.bit_width;
    const LirOperand shl_tmp = fresh_value(ctx);
    emit_lir_op(ctx, lir::LirBinOp{
                         shl_tmp, "shl", unit_ty, masked,
                         LirOperand::integer(std::to_string(shift_amt), shift_amt)});
    cur = fresh_value(ctx);
    emit_lir_op(ctx, lir::LirBinOp{
                         cur, "ashr", unit_ty, shl_tmp,
                         LirOperand::integer(std::to_string(shift_amt), shift_amt)});
  }

  const LirOperand result = fresh_value(ctx);
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
    emit_lir_op(ctx, lir::LirBinOp{result, "add", unit_ty, cur,
                                   LirOperand::integer("0", 0)});
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
  return emit_lval_operand(ctx, id, pointee_ts).str();
}

LirOperand StmtEmitter::emit_lval_operand(FnCtx& ctx, ExprId id,
                                          TypeSpec& pointee_ts) {
  const Expr& e = get_expr(id);
  if (const auto* ref = std::get_if<DeclRef>(&e.payload); ref && ref->global) {
    const GlobalVar* selected = select_global_object(*ref);
    if (!selected) selected = mod_.find_global(*ref->global);
    if (!selected) {
      throw std::runtime_error("StmtEmitter: global lvalue not found: " +
                               ref->name);
    }
    pointee_ts = selected->type.spec;
    const std::string display = llvm_global_sym(
        emitted_link_name(mod_, selected->link_name_id, selected->name));
    return LirOperand::global(display, selected->link_name_id);
  }
  return LirOperand::raw(emit_lval_dispatch(ctx, e, pointee_ts));
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
      auto it = ctx.param_slots.find(*r->param_index + 0x80000000u);
      if (it != ctx.param_slots.end()) {
        return it->second;
      }
      const std::string slot = "%lv.param." + sanitize_llvm_ident(param.name);
      ctx.alloca_insts.push_back(lir::LirAllocaOp{slot, llvm_alloca_ty(mod_, pts), "", 0});
      if (amd64_fixed_aggregate_byval(mod_, pts)) {
        module_->need_memcpy = true;
        if (!ctx.lir_function) {
          throw std::runtime_error(
              "StmtEmitter: selected byval memcpy requires current-function authority");
        }
        populate_selected_byval_parameter_materialization_authority(
            *ctx.lir_function);
        const long long size_bytes = llvm_cc::amd64_type_size_bytes(pts, mod_);
        const auto authority = selected_byval_parameter_materialization_memcpy_authority(
            *ctx.lir_function, size_bytes);
        if (!authority.has_value()) {
          throw std::runtime_error(
              "StmtEmitter: selected byval memcpy has incomplete pointer authority");
        }
        emit_lir_op(ctx, lir::LirMemcpyOp{
                             slot, pname, std::to_string(size_bytes), false,
                             std::move(authority)});
      } else {
        ctx.alloca_insts.push_back(lir::LirStoreOp{llvm_value_ty(mod_, pts), pname, slot});
      }
      ctx.param_slots[*r->param_index + 0x80000000u] = slot;
      return slot;
    }
    if (r->global) {
      const GlobalVar* selected = select_global_object(*r);
      if (!selected) selected = mod_.find_global(*r->global);
      if (!selected) {
        throw std::runtime_error("StmtEmitter: global lvalue not found: " + r->name);
      }
      const auto& gv = *selected;
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
      if (pts.ptr_level > 0) {
        pts.ptr_level--;
        if (pts.ptr_level == 0) pts.is_ptr_to_array = false;
      }
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
    const TypeSpec resolved_base_ts = resolve_expr_type(ctx, idx->base);
    const Expr& base_expr = get_expr(idx->base);
    auto base_is_array_object_lvalue = [&]() -> bool {
      if (outer_array_rank(resolved_base_ts) <= 0) return false;
      if (std::holds_alternative<MemberExpr>(base_expr.payload) ||
          std::holds_alternative<IndexExpr>(base_expr.payload)) {
        return true;
      }
      if (const auto* u = std::get_if<UnaryExpr>(&base_expr.payload)) {
        return u->op == UnaryOp::Deref;
      }
      if (const auto* r = std::get_if<DeclRef>(&base_expr.payload)) {
        if (r->param_index.has_value()) return false;
        if (r->local) {
          const auto type_it = ctx.local_types.find(r->local->value);
          if (type_it == ctx.local_types.end()) return false;
          const auto vla_it = ctx.local_is_vla.find(r->local->value);
          if (vla_it != ctx.local_is_vla.end() && vla_it->second) return false;
          return outer_array_rank(type_it->second) > 0 &&
                 llvm_alloca_ty(mod_, type_it->second) != "ptr";
        }
        return r->global.has_value();
      }
      return false;
    };
    if (is_vector_value(resolved_base_ts) || base_is_array_object_lvalue()) {
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
                              indexed_gep_structured_name_id(mod_, module_, llvm_ty(pts), pts));
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
    const LirOperand rhs = emit_rval_operand(ctx, assign->rhs, rhs_ts);
    const AssignableLValue lhs = emit_assignable_lval(ctx, assign->lhs);
    if (assign->op == AssignOp::Set) {
      (void)emit_set_assign_value(ctx, lhs, rhs, rhs_ts);
    } else {
      (void)emit_compound_assign_value(ctx, lhs, assign->op, rhs.str(), rhs_ts);
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
  access.ptr = emit_lval_operand(ctx, id, access.pointee_ts);
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
  emit_lir_op(ctx, lir::LirLoadOp{loaded.value,
                                  llvm_value_ty(mod_, lhs.pointee_ts),
                                  LirOperand::raw(lhs.ptr.str())});
  return loaded;
}

std::string StmtEmitter::emit_store_assignable_value(FnCtx& ctx, const AssignableLValue& lhs,
                                                     const LirOperand& value,
                                                     const TypeSpec& value_ts,
                                                     bool reload_after_store) {
  if (lhs.is_bitfield()) {
    emit_bitfield_store(ctx, lhs.ptr.str(), lhs.bf, value.str(), value_ts);
    return reload_after_store ? emit_bitfield_load(ctx, lhs.ptr.str(), lhs.bf).str()
                              : value.str();
  }
  const bool zero_init_aggregate =
      value == "zeroinitializer" &&
      (lhs.pointee_ts.array_rank > 0 ||
       (lhs.pointee_ts.ptr_level == 0 &&
        (lhs.pointee_ts.base == TB_STRUCT || lhs.pointee_ts.base == TB_UNION)));
  if (zero_init_aggregate) {
    module_->need_memset = true;
    emit_lir_op(ctx, lir::LirMemsetOp{lhs.ptr.str(), "0",
                                      std::to_string(sizeof_ts(mod_, lhs.pointee_ts)), false});
    return value.str();
  }
  emit_lir_op(ctx, lir::LirStoreOp{llvm_value_ty(mod_, lhs.pointee_ts), value, lhs.ptr});
  return value.str();
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
                                               const LirOperand& rhs,
                                               const TypeSpec& rhs_ts) {
  if (lhs.is_bitfield()) {
    return emit_store_assignable_value(ctx, lhs, rhs, rhs_ts, true);
  }

  std::string coerced_rhs = coerce(ctx, rhs.str(), rhs_ts, lhs.pointee_ts);
  const bool same_representation =
      llvm_value_ty(mod_, rhs_ts) == llvm_value_ty(mod_, lhs.pointee_ts);
  LirOperand stored_rhs = integer_store_operand_after_coercion(
      rhs, coerced_rhs, same_representation);
  const bool is_agg = (lhs.pointee_ts.base == TB_STRUCT || lhs.pointee_ts.base == TB_UNION) &&
                      lhs.pointee_ts.ptr_level == 0 && lhs.pointee_ts.array_rank == 0;
  if (is_agg && (coerced_rhs == "0" || coerced_rhs.empty())) {
    stored_rhs = LirOperand::raw("zeroinitializer");
  }
  return emit_store_assignable_value(ctx, lhs, stored_rhs,
                                     lhs.pointee_ts, false);
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
  if (!access.field_found && !m.resolved_owner_tag.empty()) {
    if (const std::optional<std::string> owner_tag =
            member_access_owner_tag_from_type(mod_, access.base_ts);
        owner_tag && *owner_tag != access.tag) {
      std::vector<FieldStep> recovered_chain;
      TypeSpec recovered_field_ts{};
      BitfieldAccess recovered_bf{};
      if (resolve_field_access(*owner_tag, m.field, recovered_chain, recovered_field_ts,
                               &recovered_bf)) {
        access.tag = *owner_tag;
        access.chain = std::move(recovered_chain);
        access.field_ts = recovered_field_ts;
        access.bf = recovered_bf;
        access.field_found = true;
      }
    }
  }
  if (!access.field_found && m.member_symbol_id != kInvalidMemberSymbol) {
    std::optional<MemberFieldAccess> unique_match;
    bool ambiguous = false;
    for (const auto& tag : mod_.struct_def_order) {
      std::vector<FieldStep> recovered_chain;
      TypeSpec recovered_field_ts{};
      BitfieldAccess recovered_bf{};
      if (!resolve_field_access_by_member_symbol_id(tag, m.member_symbol_id,
                                                    recovered_chain,
                                                    recovered_field_ts,
                                                    &recovered_bf)) {
        continue;
      }
      MemberFieldAccess candidate = access;
      candidate.tag = tag;
      candidate.chain = std::move(recovered_chain);
      candidate.field_ts = recovered_field_ts;
      candidate.bf = recovered_bf;
      candidate.field_found = true;
      if (unique_match.has_value()) {
        ambiguous = true;
        break;
      }
      unique_match = std::move(candidate);
    }
    if (!ambiguous && unique_match.has_value()) access = std::move(*unique_match);
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
    return LirTypeRef(lir::LirBuiltinType::I8);
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
      elem_structured_name_id =
          indexed_gep_structured_name_id(mod_, module_, rendered_text, elem_ts);
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
  return hir_rendered_indexed_gep_element_type_text(std::move(rendered_text));
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
    const std::string arr_alloca_ty = llvm_alloca_ty(mod_, access_ts);
    if (arr_alloca_ty == "ptr") {
      const std::string tmp = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirLoadOp{tmp, std::string("ptr"),
                                      LirOperand::raw(ptr)});
      return tmp;
    }
    const std::string tmp = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirGepOp{tmp, arr_alloca_ty, ptr, false, {"i64 0", "i64 0"}});
    return tmp;
  }
  const std::string ty = llvm_value_ty(mod_, load_ts);
  if (ty == "void") return "";
  const std::string tmp = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirLoadOp{tmp, ty, LirOperand::raw(ptr)});
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
