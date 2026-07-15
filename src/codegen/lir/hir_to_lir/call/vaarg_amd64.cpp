#include "call.hpp"
#include "../../../llvm/calling_convention.hpp"

namespace c4c::codegen::lir {

namespace llvm_cc = c4c::codegen::llvm_backend;
using namespace stmt_emitter_detail;

namespace {

[[deprecated(
    "no-LirModule fixed AMD64 va_list tag text: no structured name ID is "
    "available at this runtime-text compatibility boundary")]]
LirTypeRef no_module_amd64_va_list_tag_type_text(std::string rendered_text) {
  return LirTypeRef::runtime_text(std::move(rendered_text));
}

LirTypeRef lir_va_list_tag_type_ref(lir::LirModule* module) {
  constexpr const char* kVaListTagType = "%struct.__va_list_tag_";
  if (!module) return no_module_amd64_va_list_tag_type_text(kVaListTagType);
  return LirTypeRef::struct_type(kVaListTagType, module->struct_names.intern(kVaListTagType));
}

}  // namespace

StmtEmitter::Amd64VaListPtrs StmtEmitter::load_amd64_va_list_ptrs(
    FnCtx& ctx, const std::string& ap_ptr) {
  Amd64VaListPtrs access;
  const LirTypeRef va_list_tag_ty = lir_va_list_tag_type_ref(module_);
  access.gp_offset_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{access.gp_offset_ptr, va_list_tag_ty,
                                 ap_ptr, false, {"i32 0", "i32 0"}});
  access.fp_offset_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{access.fp_offset_ptr, va_list_tag_ty,
                                 ap_ptr, false, {"i32 0", "i32 1"}});
  access.overflow_ptr_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{access.overflow_ptr_ptr, va_list_tag_ty,
                                 ap_ptr, false, {"i32 0", "i32 2"}});
  const std::string reg_save_ptr_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{reg_save_ptr_ptr, va_list_tag_ty,
                                 ap_ptr, false, {"i32 0", "i32 3"}});
  access.reg_save_area_ptr = fresh_value(ctx);
  emit_lir_op(ctx, lir::LirLoadOp{access.reg_save_area_ptr, "ptr", reg_save_ptr_ptr});
  return access;
}

LirOperand StmtEmitter::emit_amd64_va_arg_from_overflow(
    FnCtx& ctx, const TypeSpec& res_ts, const std::string& res_ty,
    const Amd64VaListPtrs& access, int size_bytes) {
  const LirOperand stack_ptr = fresh_value(ctx);
  emit_lir_op(ctx, lir::LirLoadOp{stack_ptr, std::string("ptr"), access.overflow_ptr_ptr});
  const int stride = ((size_bytes + 7) / 8) * 8;
  const LirOperand next_ptr = fresh_value(ctx);
  emit_lir_op(ctx, lir::LirGepOp{
                       next_ptr, "i8", stack_ptr, false,
                       {LirGepIndex::typed(LirTypeRef::integer(64),
                                           LirOperand::integer(std::to_string(stride), stride))}});
  emit_lir_op(ctx, lir::LirStoreOp{std::string("ptr"), next_ptr, access.overflow_ptr_ptr});

  const std::string tmp_addr = fresh_tmp(ctx);
  const int align = object_align_bytes(mod_, module_, res_ts);
  ctx.alloca_insts.push_back(lir::LirAllocaOp{tmp_addr, res_ty, "", align});
  module_->need_memcpy = true;
  emit_lir_op(ctx, lir::LirMemcpyOp{tmp_addr, stack_ptr, std::to_string(size_bytes), false});
  const LirOperand out = fresh_value(ctx);
  emit_lir_op(ctx, lir::LirLoadOp{out, res_ty, tmp_addr, true});
  return out;
}

std::string StmtEmitter::emit_amd64_va_arg(FnCtx& ctx, const TypeSpec& res_ts,
                                           const std::string& res_ty,
                                           const std::string& ap_ptr) {
  if (module_ != nullptr && module_->prefer_semantic_va_ops) {
    const std::string out = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirVaArgOp{out, ap_ptr, res_ty});
    return out;
  }

  const auto layout = llvm_cc::classify_amd64_vararg(res_ts, mod_);
  if (layout.size_bytes <= 0) return "zeroinitializer";
  const auto access = load_amd64_va_list_ptrs(ctx, ap_ptr);
  if (layout.needs_memory) {
    return emit_amd64_va_arg_from_overflow(ctx, res_ts, res_ty, access, layout.size_bytes).str();
  }

  LirOperand gp_offset = LirOperand::integer("0", 0);
  if (layout.gp_chunks > 0) {
    gp_offset = fresh_value(ctx);
    emit_lir_op(ctx, lir::LirLoadOp{gp_offset, std::string("i32"), access.gp_offset_ptr});
  }
  LirOperand fp_offset = LirOperand::integer("0", 0);
  if (layout.sse_slots > 0) {
    fp_offset = fresh_value(ctx);
    emit_lir_op(ctx, lir::LirLoadOp{fp_offset, std::string("i32"), access.fp_offset_ptr});
  }

  LirOperand gp_ok = LirOperand::raw("true");
  if (layout.gp_chunks > 0) {
    const int gp_limit = 48 - layout.gp_chunks * 8;
    gp_ok = fresh_value(ctx);
    emit_lir_op(ctx, lir::LirCmpOp{gp_ok, false, LirCmpPredicate::Sle, "i32", gp_offset, std::to_string(gp_limit)});
  }

  LirOperand fp_ok = LirOperand::raw("true");
  if (layout.sse_slots > 0) {
    const int fp_limit = 176 - layout.sse_slots * 16;
    fp_ok = fresh_value(ctx);
    emit_lir_op(ctx, lir::LirCmpOp{fp_ok, false, LirCmpPredicate::Sle, "i32", fp_offset, std::to_string(fp_limit)});
  }

  LirOperand regs_ok;
  if (layout.gp_chunks > 0 && layout.sse_slots > 0) {
    regs_ok = fresh_value(ctx);
    emit_lir_op(ctx, lir::LirBinOp{regs_ok, "and", "i1", gp_ok, fp_ok});
  } else if (layout.gp_chunks > 0) {
    regs_ok = gp_ok;
  } else {
    regs_ok = fp_ok;
  }

  const auto reg_target = fresh_direct_target(ctx, fresh_lbl(ctx, "vaarg.amd64.reg."));
  const auto stack_target = fresh_direct_target(ctx, fresh_lbl(ctx, "vaarg.amd64.stack."));
  const auto join_target = fresh_direct_target(ctx, fresh_lbl(ctx, "vaarg.amd64.join."));

  TypeSpec bool_ts{};
  bool_ts.base = TB_BOOL;
  emit_condbr_and_open_lbl(ctx, to_bool_operand(ctx, regs_ok, bool_ts),
                           reg_target, stack_target, reg_target);
  const LirOperand reg_value =
      emit_amd64_va_arg_from_registers(ctx, res_ts, res_ty, layout, access, gp_offset, fp_offset);
  emit_br_and_open_lbl(ctx, join_target, stack_target);
  const LirOperand stack_value =
      emit_amd64_va_arg_from_overflow(ctx, res_ts, res_ty, access, layout.size_bytes);
  emit_br_and_open_lbl(ctx, join_target, join_target);

  const LirOperand phi = fresh_value(ctx);
  emit_lir_op(ctx, lir::LirPhiOp{phi, res_ty,
                                 {{reg_value, reg_target.label}, {stack_value, stack_target.label}}});
  return phi.str();
}

}  // namespace c4c::codegen::lir
