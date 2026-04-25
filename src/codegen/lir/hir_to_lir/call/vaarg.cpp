#include "call.hpp"
#include "call_args_ops.hpp"

namespace c4c::codegen::lir {

using namespace stmt_emitter_detail;

namespace {

LirTypeRef lir_va_list_tag_type_ref(lir::LirModule* module) {
  constexpr const char* kVaListTagType = "%struct.__va_list_tag_";
  if (!module) return LirTypeRef(kVaListTagType);
  return LirTypeRef::struct_type(kVaListTagType, module->struct_names.intern(kVaListTagType));
}

}  // namespace

std::string StmtEmitter::emit_aarch64_vaarg_gp_src_ptr(FnCtx& ctx, const std::string& ap_ptr,
                                                       int slot_bytes) {
  const LirTypeRef va_list_tag_ty = lir_va_list_tag_type_ref(module_);
  const std::string offs_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{offs_ptr, va_list_tag_ty, ap_ptr, false,
                                 {"i32 0", "i32 3"}});
  const std::string offs = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirLoadOp{offs, std::string("i32"), offs_ptr});

  const std::string stack_lbl = fresh_lbl(ctx, "vaarg.stack.");
  const std::string reg_try_lbl = fresh_lbl(ctx, "vaarg.regtry.");
  const std::string reg_lbl = fresh_lbl(ctx, "vaarg.reg.");
  const std::string join_lbl = fresh_lbl(ctx, "vaarg.join.");

  const std::string is_stack0 = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirCmpOp{is_stack0, false, "sge", "i32", offs, "0"});
  emit_condbr_and_open_lbl(ctx, is_stack0, stack_lbl, reg_try_lbl, reg_try_lbl);
  const std::string next_offs = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirBinOp{next_offs, "add", "i32", offs, std::to_string(slot_bytes)});
  emit_lir_op(ctx, lir::LirStoreOp{std::string("i32"), next_offs, offs_ptr});
  const std::string use_reg = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirCmpOp{use_reg, false, "sle", "i32", next_offs, "0"});
  emit_condbr_and_open_lbl(ctx, use_reg, reg_lbl, stack_lbl, reg_lbl);
  const std::string gr_top_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{gr_top_ptr, va_list_tag_ty, ap_ptr, false,
                                 {"i32 0", "i32 1"}});
  const std::string gr_top = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirLoadOp{gr_top, std::string("ptr"), gr_top_ptr});
  const std::string reg_addr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{reg_addr, "i8", gr_top, false, {"i32 " + offs}});
  emit_br_and_open_lbl(ctx, join_lbl, stack_lbl);
  const std::string stack_ptr_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{stack_ptr_ptr, va_list_tag_ty, ap_ptr, false,
                                 {"i32 0", "i32 0"}});
  const std::string stack_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirLoadOp{stack_ptr, std::string("ptr"), stack_ptr_ptr});
  const std::string stack_next = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{stack_next, "i8", stack_ptr, false,
                                 {"i64 " + std::to_string(slot_bytes)}});
  emit_lir_op(ctx, lir::LirStoreOp{std::string("ptr"), stack_next, stack_ptr_ptr});
  emit_fallthrough_lbl(ctx, join_lbl);
  const std::string src_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirPhiOp{src_ptr, "ptr", {{reg_addr, reg_lbl}, {stack_ptr, stack_lbl}}});
  return src_ptr;
}

std::string StmtEmitter::emit_aarch64_vaarg_fp_src_ptr(FnCtx& ctx, const std::string& ap_ptr,
                                                       int reg_slot_bytes, int stack_slot_bytes,
                                                       int stack_align_bytes) {
  const LirTypeRef va_list_tag_ty = lir_va_list_tag_type_ref(module_);
  const std::string offs_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{offs_ptr, va_list_tag_ty, ap_ptr, false,
                                 {"i32 0", "i32 4"}});
  const std::string offs = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirLoadOp{offs, std::string("i32"), offs_ptr});

  const std::string stack_lbl = fresh_lbl(ctx, "vaarg.fp.stack.");
  const std::string reg_try_lbl = fresh_lbl(ctx, "vaarg.fp.regtry.");
  const std::string reg_lbl = fresh_lbl(ctx, "vaarg.fp.reg.");
  const std::string join_lbl = fresh_lbl(ctx, "vaarg.fp.join.");

  const std::string is_stack0 = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirCmpOp{is_stack0, false, "sge", "i32", offs, "0"});
  emit_condbr_and_open_lbl(ctx, is_stack0, stack_lbl, reg_try_lbl, reg_try_lbl);
  const std::string next_offs = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirBinOp{next_offs, "add", "i32", offs, std::to_string(reg_slot_bytes)});
  emit_lir_op(ctx, lir::LirStoreOp{std::string("i32"), next_offs, offs_ptr});
  const std::string use_reg = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirCmpOp{use_reg, false, "sle", "i32", next_offs, "0"});
  emit_condbr_and_open_lbl(ctx, use_reg, reg_lbl, stack_lbl, reg_lbl);
  const std::string vr_top_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{vr_top_ptr, va_list_tag_ty, ap_ptr, false,
                                 {"i32 0", "i32 2"}});
  const std::string vr_top = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirLoadOp{vr_top, std::string("ptr"), vr_top_ptr});
  const std::string reg_addr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{reg_addr, "i8", vr_top, false, {"i32 " + offs}});
  emit_br_and_open_lbl(ctx, join_lbl, stack_lbl);
  const std::string stack_ptr_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{stack_ptr_ptr, va_list_tag_ty, ap_ptr, false,
                                 {"i32 0", "i32 0"}});
  const std::string stack_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirLoadOp{stack_ptr, std::string("ptr"), stack_ptr_ptr});
  std::string aligned_stack_ptr = stack_ptr;
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
  const std::string stack_next = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{stack_next, "i8", aligned_stack_ptr, false,
                                 {"i64 " + std::to_string(stack_slot_bytes)}});
  emit_lir_op(ctx, lir::LirStoreOp{std::string("ptr"), stack_next, stack_ptr_ptr});
  emit_fallthrough_lbl(ctx, join_lbl);
  const std::string src_ptr = fresh_tmp(ctx);
  emit_lir_op(
      ctx, lir::LirPhiOp{src_ptr, "ptr", {{reg_addr, reg_lbl}, {aligned_stack_ptr, stack_lbl}}});
  return src_ptr;
}

std::string StmtEmitter::emit_rval_payload(FnCtx& ctx, const VaArgExpr& v, const Expr& e) {
  TypeSpec ap_ts{};
  const std::string ap_ptr = emit_va_list_obj_ptr(ctx, v.ap, ap_ts);
  TypeSpec res_ts = e.type.spec;
  if (!has_concrete_type(res_ts)) res_ts = resolve_expr_type(ctx, e);
  const std::string res_ty = llvm_ty(res_ts);
  if (res_ty == "void") return "";
  if ((res_ts.base == TB_STRUCT || res_ts.base == TB_UNION) && res_ts.ptr_level == 0 &&
      res_ts.array_rank == 0 && res_ts.tag && res_ts.tag[0]) {
    const auto it = mod_.struct_defs.find(res_ts.tag);
    if (it != mod_.struct_defs.end()) {
      const HirStructDef& sd = it->second;
      int payload_sz = 0;
      if (sd.is_union) {
        for (const auto& f : sd.fields) {
          payload_sz = std::max(payload_sz, f.size_bytes);
        }
      } else {
        payload_sz = sd.size_bytes;
      }
      if (payload_sz == 0) return "zeroinitializer";
    }
  }
  if (llvm_va_list_is_pointer_object(mod_.target_profile)) {
    const std::string out = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirVaArgOp{out, ap_ptr, res_ty});
    return out;
  }
  if (llvm_target_is_amd64_sysv(mod_.target_profile)) {
    return emit_amd64_va_arg(ctx, res_ts, res_ty, ap_ptr);
  }
  if (const auto hfa = classify_aarch64_hfa(mod_, res_ts)) {
    const LirTypeRef va_list_tag_ty = lir_va_list_tag_type_ref(module_);
    const int stack_align = std::max(8, hfa->aggregate_align);
    const int stack_slot_bytes = round_up_to(hfa->aggregate_size, stack_align);
    const std::string reg_tmp_ty =
        "[" + std::to_string(hfa->elem_count) + " x " + hfa->elem_ty + "]";

    const std::string offs_ptr = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirGepOp{offs_ptr, va_list_tag_ty, ap_ptr, false,
                                   {"i32 0", "i32 4"}});
    const std::string offs = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirLoadOp{offs, std::string("i32"), offs_ptr});

    const std::string stack_lbl = fresh_lbl(ctx, "vaarg.fp.stack.");
    const std::string reg_try_lbl = fresh_lbl(ctx, "vaarg.fp.regtry.");
    const std::string reg_lbl = fresh_lbl(ctx, "vaarg.fp.reg.");
    const std::string join_lbl = fresh_lbl(ctx, "vaarg.fp.join.");

    const std::string is_stack0 = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCmpOp{is_stack0, false, "sge", "i32", offs, "0"});
    emit_condbr_and_open_lbl(ctx, is_stack0, stack_lbl, reg_try_lbl, reg_try_lbl);
    const std::string next_offs = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirBinOp{next_offs, "add", "i32", offs,
                                   std::to_string(hfa->elem_count * 16)});
    emit_lir_op(ctx, lir::LirStoreOp{std::string("i32"), next_offs, offs_ptr});
    const std::string use_reg = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCmpOp{use_reg, false, "sle", "i32", next_offs, "0"});
    emit_condbr_and_open_lbl(ctx, use_reg, reg_lbl, stack_lbl, reg_lbl);

    const std::string vr_top_ptr = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirGepOp{vr_top_ptr, va_list_tag_ty, ap_ptr, false,
                                   {"i32 0", "i32 2"}});
    const std::string vr_top = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirLoadOp{vr_top, std::string("ptr"), vr_top_ptr});
    const std::string reg_addr = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirGepOp{reg_addr, "i8", vr_top, false, {"i32 " + offs}});

    const std::string reg_tmp = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirAllocaOp{reg_tmp, reg_tmp_ty, {}, hfa->aggregate_align});
    for (int i = 0; i < hfa->elem_count; ++i) {
      const std::string lane_src = (i == 0) ? reg_addr : fresh_tmp(ctx);
      if (i != 0) {
        emit_lir_op(ctx, lir::LirGepOp{lane_src, "i8", reg_addr, false,
                                       {"i64 " + std::to_string(i * 16)}});
      }
      const std::string lane_val = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirLoadOp{lane_val, hfa->elem_ty, lane_src});
      const std::string lane_dst = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirGepOp{lane_dst, reg_tmp_ty, reg_tmp, false,
                                     {"i64 0", "i64 " + std::to_string(i)}});
      emit_lir_op(ctx, lir::LirStoreOp{hfa->elem_ty, lane_val, lane_dst});
    }
    emit_br_and_open_lbl(ctx, join_lbl, stack_lbl);

    const std::string stack_ptr_ptr = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirGepOp{stack_ptr_ptr, va_list_tag_ty, ap_ptr, false,
                                   {"i32 0", "i32 0"}});
    const std::string stack_ptr = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirLoadOp{stack_ptr, std::string("ptr"), stack_ptr_ptr});
    std::string aligned_stack_ptr = stack_ptr;
    if (stack_align > 1) {
      const std::string stack_i = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirCastOp{stack_i, lir::LirCastKind::PtrToInt, "ptr", stack_ptr,
                                      "i64"});
      const std::string plus_mask = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirBinOp{plus_mask, "add", "i64", stack_i,
                                     std::to_string(stack_align - 1)});
      const std::string masked = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirBinOp{masked, "and", "i64", plus_mask,
                                     std::to_string(-stack_align)});
      aligned_stack_ptr = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirCastOp{aligned_stack_ptr, lir::LirCastKind::IntToPtr, "i64",
                                      masked, "ptr"});
    }
    const std::string stack_next = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirGepOp{stack_next, "i8", aligned_stack_ptr, false,
                                   {"i64 " + std::to_string(stack_slot_bytes)}});
    emit_lir_op(ctx, lir::LirStoreOp{std::string("ptr"), stack_next, stack_ptr_ptr});
    emit_fallthrough_lbl(ctx, join_lbl);

    const std::string src_ptr = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirPhiOp{
                         src_ptr, "ptr", {{reg_tmp, reg_lbl}, {aligned_stack_ptr, stack_lbl}}});
    const std::string tmp_addr = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirAllocaOp{tmp_addr, res_ty, {}, hfa->aggregate_align});
    module_->need_memcpy = true;
    emit_lir_op(
        ctx, lir::LirMemcpyOp{tmp_addr, src_ptr, std::to_string(hfa->aggregate_size), false});
    const std::string out = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirLoadOp{out, res_ty, tmp_addr});
    return out;
  }
  if ((res_ts.base == TB_STRUCT || res_ts.base == TB_UNION) && res_ts.ptr_level == 0 &&
      res_ts.array_rank == 0 && res_ts.tag && res_ts.tag[0]) {
    const auto it = mod_.struct_defs.find(res_ts.tag);
    if (it != mod_.struct_defs.end()) {
      const int payload_sz = it->second.size_bytes;
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
