#include "call.hpp"
#include "call_args_ops.hpp"

namespace c4c::codegen::lir {

using namespace stmt_emitter_detail;

namespace {

[[deprecated(
    "no-LirModule fixed va_list tag text: no structured name ID is available "
    "at this runtime-text compatibility boundary")]]
LirTypeRef no_module_va_list_tag_type_text(std::string rendered_text) {
  return LirTypeRef(std::move(rendered_text));
}

StructNameId vaarg_aggregate_structured_name_id(const c4c::hir::Module& mod,
                                                const lir::LirModule* module,
                                                const TypeSpec& aggregate_ts) {
  if (!module || (aggregate_ts.base != TB_STRUCT && aggregate_ts.base != TB_UNION) ||
      aggregate_ts.ptr_level != 0 || aggregate_ts.array_rank != 0) {
    return kInvalidStructName;
  }
  if (typespec_aggregate_complete_owner_key_missed(aggregate_ts, mod)) {
    return kInvalidStructName;
  }

  const std::optional<HirRecordOwnerKey> owner_key =
      typespec_aggregate_owner_key(aggregate_ts, mod);
  if (!owner_key) return kInvalidStructName;
  const SymbolName* structured_tag = mod.find_struct_def_tag_by_owner(*owner_key);
  if (!structured_tag || structured_tag->empty()) return kInvalidStructName;

  const StructNameId name_id =
      module->struct_names.find(llvm_struct_type_str(*structured_tag));
  return module->find_struct_decl(name_id) ? name_id : kInvalidStructName;
}

LirTypeRef lir_va_list_tag_type_ref(lir::LirModule* module) {
  constexpr const char* kVaListTagType = "%struct.__va_list_tag_";
  if (!module) return no_module_va_list_tag_type_text(kVaListTagType);
  return LirTypeRef::struct_type(kVaListTagType, module->struct_names.intern(kVaListTagType));
}

}  // namespace

std::string StmtEmitter::emit_aarch64_vaarg_gp_src_ptr(FnCtx& ctx, const std::string& ap_ptr,
                                                       int slot_bytes) {
  const LirTypeRef va_list_tag_ty = lir_va_list_tag_type_ref(module_);
  const std::string offs_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{offs_ptr, va_list_tag_ty, ap_ptr, false,
                                 {"i32 0", "i32 3"}});
  const LirOperand offs = fresh_value(ctx);
  emit_lir_op(ctx, lir::LirLoadOp{offs, std::string("i32"), offs_ptr});

  const auto stack_target = fresh_direct_target(ctx, fresh_lbl(ctx, "vaarg.stack."));
  const auto reg_try_target = fresh_direct_target(ctx, fresh_lbl(ctx, "vaarg.regtry."));
  const auto reg_target = fresh_direct_target(ctx, fresh_lbl(ctx, "vaarg.reg."));
  const auto join_target = fresh_direct_target(ctx, fresh_lbl(ctx, "vaarg.join."));

  const std::string is_stack0 = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirCmpOp{is_stack0, false, LirCmpPredicate::Sge, "i32", offs, "0"});
  TypeSpec bool_ts{};
  bool_ts.base = TB_BOOL;
  emit_condbr_and_open_lbl(ctx, to_bool_operand(ctx, lir::LirOperand::raw(is_stack0), bool_ts),
                           stack_target, reg_try_target, reg_try_target);
  const std::string next_offs = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirBinOp{next_offs, "add", "i32", offs, std::to_string(slot_bytes)});
  emit_lir_op(ctx, lir::LirStoreOp{std::string("i32"), next_offs, offs_ptr});
  const std::string use_reg = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirCmpOp{use_reg, false, LirCmpPredicate::Sle, "i32", next_offs, "0"});
  emit_condbr_and_open_lbl(ctx, to_bool_operand(ctx, lir::LirOperand::raw(use_reg), bool_ts),
                           reg_target, stack_target, reg_target);
  const std::string gr_top_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{gr_top_ptr, va_list_tag_ty, ap_ptr, false,
                                 {"i32 0", "i32 1"}});
  const LirOperand gr_top = fresh_value(ctx);
  emit_lir_op(ctx, lir::LirLoadOp{gr_top, std::string("ptr"), gr_top_ptr});
  const LirOperand reg_addr = fresh_value(ctx);
  emit_lir_op(ctx, lir::LirGepOp{reg_addr, "i8", gr_top, false,
                                 {LirGepIndex::typed(LirTypeRef::integer(32), offs)}, true});
  emit_br_and_open_lbl(ctx, join_target, stack_target);
  const std::string stack_ptr_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{stack_ptr_ptr, va_list_tag_ty, ap_ptr, false,
                                 {"i32 0", "i32 0"}});
  const LirOperand stack_ptr = fresh_value(ctx);
  emit_lir_op(ctx, lir::LirLoadOp{stack_ptr, std::string("ptr"), stack_ptr_ptr, true});
  const LirOperand stack_next = fresh_value(ctx);
  emit_lir_op(ctx, lir::LirGepOp{
      stack_next, "i8", stack_ptr, false,
      {LirGepIndex::typed(LirTypeRef::integer(64),
                          LirOperand::integer(std::to_string(slot_bytes), slot_bytes))}});
  emit_lir_op(ctx, lir::LirStoreOp{std::string("ptr"), stack_next, stack_ptr_ptr});
  emit_fallthrough_lbl(ctx, join_target);
  const LirOperand src_ptr = fresh_value(ctx);
  emit_lir_op(ctx, lir::LirPhiOp{src_ptr, "ptr",
                                 {{reg_addr, reg_target.label, reg_target.id,
                                   lir::LirSuccessorOccurrenceId::direct_branch()},
                                  {stack_ptr, stack_target.label, stack_target.id,
                                   lir::LirSuccessorOccurrenceId::direct_branch()}}});
  return src_ptr.str();
}

std::string StmtEmitter::emit_aarch64_vaarg_fp_src_ptr(FnCtx& ctx, const std::string& ap_ptr,
                                                       int reg_slot_bytes, int stack_slot_bytes,
                                                       int stack_align_bytes) {
  const LirTypeRef va_list_tag_ty = lir_va_list_tag_type_ref(module_);
  const std::string offs_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{offs_ptr, va_list_tag_ty, ap_ptr, false,
                                 {"i32 0", "i32 4"}});
  const LirOperand offs = fresh_value(ctx);
  emit_lir_op(ctx, lir::LirLoadOp{offs, std::string("i32"), offs_ptr});

  const auto stack_target = fresh_direct_target(ctx, fresh_lbl(ctx, "vaarg.fp.stack."));
  const auto reg_try_target = fresh_direct_target(ctx, fresh_lbl(ctx, "vaarg.fp.regtry."));
  const auto reg_target = fresh_direct_target(ctx, fresh_lbl(ctx, "vaarg.fp.reg."));
  const auto join_target = fresh_direct_target(ctx, fresh_lbl(ctx, "vaarg.fp.join."));

  const std::string is_stack0 = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirCmpOp{is_stack0, false, LirCmpPredicate::Sge, "i32", offs, "0"});
  TypeSpec bool_ts{};
  bool_ts.base = TB_BOOL;
  emit_condbr_and_open_lbl(ctx, to_bool_operand(ctx, lir::LirOperand::raw(is_stack0), bool_ts),
                           stack_target, reg_try_target, reg_try_target);
  const std::string next_offs = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirBinOp{next_offs, "add", "i32", offs, std::to_string(reg_slot_bytes)});
  emit_lir_op(ctx, lir::LirStoreOp{std::string("i32"), next_offs, offs_ptr});
  const std::string use_reg = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirCmpOp{use_reg, false, LirCmpPredicate::Sle, "i32", next_offs, "0"});
  emit_condbr_and_open_lbl(ctx, to_bool_operand(ctx, lir::LirOperand::raw(use_reg), bool_ts),
                           reg_target, stack_target, reg_target);
  const std::string vr_top_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{vr_top_ptr, va_list_tag_ty, ap_ptr, false,
                                 {"i32 0", "i32 2"}});
  const LirOperand vr_top = fresh_value(ctx);
  emit_lir_op(ctx, lir::LirLoadOp{vr_top, std::string("ptr"), vr_top_ptr});
  const LirOperand reg_addr = fresh_value(ctx);
  emit_lir_op(ctx, lir::LirGepOp{reg_addr, "i8", vr_top, false,
                                 {LirGepIndex::typed(LirTypeRef::integer(32), offs)}, true});
  emit_br_and_open_lbl(ctx, join_target, stack_target);
  const std::string stack_ptr_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{stack_ptr_ptr, va_list_tag_ty, ap_ptr, false,
                                 {"i32 0", "i32 0"}});
  const LirOperand stack_ptr = fresh_value(ctx);
  emit_lir_op(ctx, lir::LirLoadOp{stack_ptr, std::string("ptr"), stack_ptr_ptr, true});
  LirOperand aligned_stack_ptr = stack_ptr;
  if (stack_align_bytes > 1) {
    if (stack_align_bytes > 8) {
      const LirOperand stack_plus = fresh_value(ctx);
      emit_lir_op(ctx, lir::LirGepOp{
          stack_plus, "i8", stack_ptr, false,
          {LirGepIndex::typed(
              LirTypeRef::integer(64),
              LirOperand::integer(std::to_string(stack_align_bytes - 1),
                                  stack_align_bytes - 1))}});
      aligned_stack_ptr = fresh_value(ctx);
      LirCallOp ptrmask = make_lir_call_op_with_return_type_ref(
          aligned_stack_ptr, LirTypeRef(LirBuiltinType::Pointer),
          "@llvm.ptrmask.p0.i64", "",
          {{"ptr", stack_plus}, {"i64", std::to_string(-stack_align_bytes)}});
      ptrmask.requires_native_result_authority = true;
      emit_lir_op(ctx, std::move(ptrmask));
      module_->need_ptrmask = true;
    } else {
      const std::string stack_i = fresh_tmp(ctx);
      emit_lir_op(ctx,
                  lir::LirCastOp{stack_i, lir::LirCastKind::PtrToInt, "ptr", stack_ptr, "i64"});
      const std::string plus_mask = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirBinOp{plus_mask, "add", "i64", stack_i,
                                     std::to_string(stack_align_bytes - 1)});
      const std::string masked = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirBinOp{masked, "and", "i64", plus_mask,
                                     std::to_string(-stack_align_bytes)});
      aligned_stack_ptr = fresh_value(ctx);
      emit_lir_op(ctx, lir::LirCastOp{aligned_stack_ptr, lir::LirCastKind::IntToPtr, "i64",
                                      masked, "ptr", true});
    }
  }
  const LirOperand stack_next = fresh_value(ctx);
  emit_lir_op(ctx, lir::LirGepOp{
      stack_next, LirTypeRef("i8"), aligned_stack_ptr, false,
      {LirGepIndex::typed(LirTypeRef::integer(64),
                          LirOperand::integer(std::to_string(stack_slot_bytes),
                                              stack_slot_bytes))}});
  emit_lir_op(ctx, lir::LirStoreOp{std::string("ptr"), stack_next, stack_ptr_ptr});
  emit_fallthrough_lbl(ctx, join_target);
  const LirOperand src_ptr = fresh_value(ctx);
  emit_lir_op(
      ctx, lir::LirPhiOp{src_ptr, "ptr",
                          {{reg_addr, reg_target.label, reg_target.id,
                            lir::LirSuccessorOccurrenceId::direct_branch()},
                           {aligned_stack_ptr, stack_target.label, stack_target.id,
                            lir::LirSuccessorOccurrenceId::direct_branch()}}});
  return src_ptr.str();
}

std::string StmtEmitter::emit_aarch64_vaarg_hfa(
    FnCtx& ctx, const std::string& ap_ptr, const TypeSpec& res_ts,
    const std::string& res_ty, const Aarch64HomogeneousFpAggregateInfo& hfa) {
  const LirTypeRef va_list_tag_ty = lir_va_list_tag_type_ref(module_);
  const std::string tmp_addr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirAllocaOp{tmp_addr, llvm_value_ty(mod_, res_ts), {},
                                    hfa.aggregate_align});

  const std::string offs_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{offs_ptr, va_list_tag_ty, ap_ptr, false,
                                 {"i32 0", "i32 4"}});
  const std::string offs = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirLoadOp{offs, std::string("i32"), offs_ptr});

  const auto stack_target = fresh_direct_target(ctx, fresh_lbl(ctx, "vaarg.hfa.stack."));
  const auto reg_try_target = fresh_direct_target(ctx, fresh_lbl(ctx, "vaarg.hfa.regtry."));
  const auto reg_target = fresh_direct_target(ctx, fresh_lbl(ctx, "vaarg.hfa.reg."));
  const auto join_target = fresh_direct_target(ctx, fresh_lbl(ctx, "vaarg.hfa.join."));

  const std::string is_stack0 = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirCmpOp{is_stack0, false, LirCmpPredicate::Sge, "i32", offs, "0"});
  TypeSpec bool_ts{};
  bool_ts.base = TB_BOOL;
  emit_condbr_and_open_lbl(ctx, to_bool_operand(ctx, lir::LirOperand::raw(is_stack0), bool_ts),
                           stack_target, reg_try_target, reg_try_target);

  const int reg_slot_bytes = hfa.elem_count * 16;
  const std::string next_offs = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirBinOp{next_offs, "add", "i32", offs,
                                 std::to_string(reg_slot_bytes)});
  emit_lir_op(ctx, lir::LirStoreOp{std::string("i32"), next_offs, offs_ptr});
  const std::string use_reg = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirCmpOp{use_reg, false, LirCmpPredicate::Sle, "i32", next_offs, "0"});
  emit_condbr_and_open_lbl(ctx, to_bool_operand(ctx, lir::LirOperand::raw(use_reg), bool_ts),
                           reg_target, stack_target, reg_target);

  const std::string vr_top_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{vr_top_ptr, va_list_tag_ty, ap_ptr, false,
                                 {"i32 0", "i32 2"}});
  const std::string vr_top = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirLoadOp{vr_top, std::string("ptr"), vr_top_ptr});
  const std::string reg_base = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{reg_base, "i8", vr_top, false, {"i32 " + offs}});
  for (int lane_index = 0; lane_index < hfa.elem_count; ++lane_index) {
    const std::string lane_src = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirGepOp{lane_src, "i8", reg_base, false,
                                   {"i64 " + std::to_string(lane_index * 16)}});
    const std::string lane = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirLoadOp{lane, hfa.elem_ty, lane_src});
    const std::string lane_dst = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirGepOp{lane_dst, "i8", tmp_addr, false,
                                   {"i64 " + std::to_string(lane_index * hfa.elem_size)}});
    emit_lir_op(ctx, lir::LirStoreOp{hfa.elem_ty, lane, lane_dst});
  }
  emit_br_and_open_lbl(ctx, join_target, stack_target);

  const std::string stack_ptr_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{stack_ptr_ptr, va_list_tag_ty, ap_ptr, false,
                                 {"i32 0", "i32 0"}});
  const std::string stack_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirLoadOp{stack_ptr, std::string("ptr"), stack_ptr_ptr});
  std::string aligned_stack_ptr = stack_ptr;
  const int stack_align_bytes = std::min(std::max(hfa.elem_size, 8), 16);
  if (stack_align_bytes > 1) {
    if (stack_align_bytes > 8) {
      const std::string stack_plus = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirGepOp{stack_plus, "i8", stack_ptr, false,
                                     {"i64 " + std::to_string(stack_align_bytes - 1)}});
      aligned_stack_ptr = fresh_tmp(ctx);
      emit_lir_op(ctx, make_lir_call_op(aligned_stack_ptr, "ptr", "@llvm.ptrmask.p0.i64", "",
                                        {{"ptr", stack_plus},
                                         {"i64", std::to_string(-stack_align_bytes)}}));
      module_->need_ptrmask = true;
    } else {
      const std::string stack_i = fresh_tmp(ctx);
      emit_lir_op(ctx,
                  lir::LirCastOp{stack_i, lir::LirCastKind::PtrToInt, "ptr", stack_ptr, "i64"});
      const std::string plus_mask = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirBinOp{plus_mask, "add", "i64", stack_i,
                                     std::to_string(stack_align_bytes - 1)});
      const std::string masked = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirBinOp{masked, "and", "i64", plus_mask,
                                     std::to_string(-stack_align_bytes)});
      aligned_stack_ptr = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirCastOp{aligned_stack_ptr, lir::LirCastKind::IntToPtr, "i64",
                                      masked, "ptr"});
    }
  }
  module_->need_memcpy = true;
  emit_lir_op(ctx, lir::LirMemcpyOp{tmp_addr, aligned_stack_ptr,
                                    std::to_string(hfa.aggregate_size), false});
  const int stack_slot_bytes = round_up_to(hfa.aggregate_size, stack_align_bytes);
  const std::string stack_next = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{stack_next, "i8", aligned_stack_ptr, false,
                                 {"i64 " + std::to_string(stack_slot_bytes)}});
  emit_lir_op(ctx, lir::LirStoreOp{std::string("ptr"), stack_next, stack_ptr_ptr});

  emit_fallthrough_lbl(ctx, join_target);
  const std::string out = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirLoadOp{out, res_ty, tmp_addr});
  return out;
}

LirOperand StmtEmitter::emit_vaarg_rval_operand(FnCtx& ctx, const VaArgExpr& v,
                                                 const Expr& e) {
  TypeSpec res_ts = e.type.spec;
  if (!has_concrete_type(res_ts)) res_ts = resolve_expr_type(ctx, e);
  const bool is_scalar_or_pointer_payload =
      res_ts.array_rank == 0 &&
      (res_ts.ptr_level > 0 ||
       (res_ts.ptr_level == 0 &&
        (is_any_int(res_ts.base) || res_ts.base == TB_FLOAT || res_ts.base == TB_DOUBLE ||
         res_ts.base == TB_LONGDOUBLE)));
  if (module_ != nullptr && module_->prefer_semantic_va_ops &&
      llvm_target_is_amd64_sysv(mod_.target_profile) && is_scalar_or_pointer_payload) {
    if (const auto ap_authority = native_direct_local_va_pointer(ctx, v.ap)) {
      const LirOperand result = fresh_value(ctx);
      const lir::LirTypeRef result_type(llvm_ty(res_ts));
      emit_lir_op(ctx, lir::LirVaArgOp{
                           .result = result,
                           .ap_ptr = ap_authority->first,
                           .type_str = result_type,
                           .requires_native_memory_va_authority = true,
                           .ap_authority = ap_authority->second,
                           .result_authority = *result.value_id(),
                           .result_type_authority = result_type,
                       });
      return result;
    }
  }
  return LirOperand::raw(emit_rval_payload(ctx, v, e));
}

std::string StmtEmitter::emit_rval_payload(FnCtx& ctx, const VaArgExpr& v, const Expr& e) {
  TypeSpec ap_ts{};
  const std::string ap_ptr = emit_va_list_obj_ptr(ctx, v.ap, ap_ts);
  TypeSpec res_ts = e.type.spec;
  if (!has_concrete_type(res_ts)) res_ts = resolve_expr_type(ctx, e);
  const std::string res_ty = llvm_ty(res_ts);
  if (res_ty == "void") return "";
  const bool is_named_aggregate =
      (res_ts.base == TB_STRUCT || res_ts.base == TB_UNION) && res_ts.ptr_level == 0 &&
      res_ts.array_rank == 0 && is_named_aggregate_value(res_ts);
  StructuredLayoutLookup aggregate_layout;
  std::optional<int> aggregate_payload_sz;
  if (is_named_aggregate) {
    const StructNameId structured_name_id =
        vaarg_aggregate_structured_name_id(mod_, module_, res_ts);
    const char* layout_site = structured_name_id == kInvalidStructName
                                  ? "va_arg-aggregate-legacy-compat"
                                  : "va_arg-aggregate";
    aggregate_layout =
        lookup_structured_layout(mod_, module_, res_ts, layout_site, structured_name_id);
    aggregate_payload_sz =
        structured_layout_size_bytes(mod_, module_, aggregate_layout);
    if (!aggregate_payload_sz && aggregate_layout.legacy_decl) {
      const HirStructDef& sd = *aggregate_layout.legacy_decl;
      int payload_sz = 0;
      if (sd.is_union) {
        for (const auto& f : sd.fields) {
          payload_sz = std::max(payload_sz, f.size_bytes);
        }
      } else {
        payload_sz = sd.size_bytes;
      }
      aggregate_payload_sz = payload_sz;
    }
    if (aggregate_payload_sz && *aggregate_payload_sz == 0) {
      return "zeroinitializer";
    }
  }
  const bool is_scalar_or_pointer_payload =
      res_ts.array_rank == 0 &&
      (res_ts.ptr_level > 0 ||
       (res_ts.ptr_level == 0 &&
        (is_any_int(res_ts.base) || res_ts.base == TB_FLOAT || res_ts.base == TB_DOUBLE ||
         res_ts.base == TB_LONGDOUBLE)));
  const bool is_single_rv64_va_slot_payload =
      is_scalar_or_pointer_payload && sizeof_ts(mod_, res_ts) <= 8;
  if (llvm_target_is_riscv64(mod_.target_profile) && is_single_rv64_va_slot_payload) {
    const std::string slot_ptr = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirLoadOp{slot_ptr, std::string("ptr"), ap_ptr});
    const std::string next_ptr = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirGepOp{next_ptr, "i8", slot_ptr, true, {"i64 8"}});
    emit_lir_op(ctx, lir::LirStoreOp{std::string("ptr"), next_ptr, ap_ptr});
    const std::string out = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirLoadOp{out, res_ty, slot_ptr});
    return out;
  }
  if (llvm_va_list_is_pointer_object(mod_.target_profile)) {
    const std::string out = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirVaArgOp{out, ap_ptr, res_ty});
    return out;
  }
  if (llvm_target_is_amd64_sysv(mod_.target_profile)) {
    if (const auto authority = native_direct_local_va_pointer(ctx, v.ap)) {
      return emit_amd64_va_arg(ctx, res_ts, res_ty, authority->first, authority->second);
    }
    return emit_amd64_va_arg(ctx, res_ts, res_ty, LirOperand::raw(ap_ptr));
  }
  if (const auto hfa = classify_aarch64_hfa(mod_, res_ts)) {
    return emit_aarch64_vaarg_hfa(ctx, ap_ptr, res_ts, res_ty, *hfa);
  }
  if (is_named_aggregate) {
    if (aggregate_payload_sz) {
      const int payload_sz = *aggregate_payload_sz;
      if (payload_sz == 0) return "zeroinitializer";
      if (payload_sz > 0) {
        if (payload_sz > 16) {
          const std::string slot_ptr = emit_aarch64_vaarg_gp_src_ptr(ctx, ap_ptr, 8);
          const std::string src_ptr = fresh_tmp(ctx);
          emit_lir_op(ctx, lir::LirLoadOp{src_ptr, std::string("ptr"), slot_ptr});
          const std::string tmp_addr = fresh_tmp(ctx);
          emit_lir_op(ctx, lir::LirAllocaOp{tmp_addr, res_ty, {}, 0});
          module_->need_memcpy = true;
          emit_lir_op(ctx, lir::LirMemcpyOp{tmp_addr, src_ptr, std::to_string(payload_sz), false});
          const std::string out = fresh_tmp(ctx);
          emit_lir_op(ctx, lir::LirLoadOp{out, res_ty, tmp_addr});
          return out;
        }

        const int slot_bytes = payload_sz > 8 ? 16 : 8;
        const std::string src_ptr = emit_aarch64_vaarg_gp_src_ptr(ctx, ap_ptr, slot_bytes);
        const std::string tmp_addr = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirAllocaOp{tmp_addr, res_ty, {}, 0});
        module_->need_memcpy = true;
        emit_lir_op(ctx, lir::LirMemcpyOp{tmp_addr, src_ptr, std::to_string(payload_sz), false});
        const std::string out = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirLoadOp{out, res_ty, tmp_addr});
        return out;
      }
    }
  }

  const bool is_gp_scalar = (res_ty == "ptr") ||
                            (res_ts.ptr_level == 0 && res_ts.array_rank == 0 &&
                             is_any_int(res_ts.base));
  const bool is_fp_scalar = res_ts.ptr_level == 0 && res_ts.array_rank == 0 &&
                            (res_ts.base == TB_FLOAT || res_ts.base == TB_DOUBLE);
  const bool is_fp128_scalar =
      res_ts.ptr_level == 0 && res_ts.array_rank == 0 && res_ts.base == TB_LONGDOUBLE;

  if (is_gp_scalar) {
    const std::string src_ptr = emit_aarch64_vaarg_gp_src_ptr(ctx, ap_ptr, 8);
    const std::string out = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirLoadOp{out, res_ty, src_ptr});
    return out;
  }
  if (is_fp_scalar) {
    const std::string src_ptr = emit_aarch64_vaarg_fp_src_ptr(ctx, ap_ptr, 16, 8, 8);
    const std::string out = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirLoadOp{out, res_ty, src_ptr});
    return out;
  }
  if (is_fp128_scalar) {
    const std::string src_ptr = emit_aarch64_vaarg_fp_src_ptr(ctx, ap_ptr, 16, 16, 16);
    const std::string out = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirLoadOp{out, res_ty, src_ptr});
    return out;
  }

  const std::string out = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirVaArgOp{out, ap_ptr, res_ty});
  return out;
}

}  // namespace c4c::codegen::lir
