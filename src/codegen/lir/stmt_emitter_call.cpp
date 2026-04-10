#include "stmt_emitter.hpp"
#include "call_args_ops.hpp"
#include "canonical_symbol.hpp"
#include "../llvm/calling_convention.hpp"

namespace c4c::codegen::lir {

namespace llvm_cc = c4c::codegen::llvm_backend;
using namespace stmt_emitter_detail;

// Draft-only staging file for Step 2 of the stmt_emitter split refactor.
// This file captures the call / builtin / vararg cluster while the monolith
// remains the only live translation unit.

StmtEmitter::Amd64VaListPtrs StmtEmitter::load_amd64_va_list_ptrs(
    FnCtx& ctx, const std::string& ap_ptr) {
  Amd64VaListPtrs access;
  access.gp_offset_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{access.gp_offset_ptr, "%struct.__va_list_tag_",
                                 ap_ptr, false, {"i32 0", "i32 0"}});
  access.fp_offset_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{access.fp_offset_ptr, "%struct.__va_list_tag_",
                                 ap_ptr, false, {"i32 0", "i32 1"}});
  access.overflow_ptr_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{access.overflow_ptr_ptr, "%struct.__va_list_tag_",
                                 ap_ptr, false, {"i32 0", "i32 2"}});
  const std::string reg_save_ptr_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirGepOp{reg_save_ptr_ptr, "%struct.__va_list_tag_",
                                 ap_ptr, false, {"i32 0", "i32 3"}});
  access.reg_save_area_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirLoadOp{access.reg_save_area_ptr, "ptr", reg_save_ptr_ptr});
  return access;
}

std::string StmtEmitter::emit_amd64_va_arg_from_overflow(
    FnCtx& ctx, const TypeSpec& res_ts, const std::string& res_ty,
    const Amd64VaListPtrs& access, int size_bytes) {
  const std::string stack_ptr = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirLoadOp{stack_ptr, std::string("ptr"), access.overflow_ptr_ptr});
  const int stride = ((size_bytes + 7) / 8) * 8;
  const std::string next_ptr = fresh_tmp(ctx);
  emit_lir_op(
      ctx, lir::LirGepOp{next_ptr, "i8", stack_ptr, false, {"i64 " + std::to_string(stride)}});
  emit_lir_op(ctx, lir::LirStoreOp{std::string("ptr"), next_ptr, access.overflow_ptr_ptr});

  const std::string tmp_addr = fresh_tmp(ctx);
  const int align = object_align_bytes(mod_, res_ts);
  ctx.alloca_insts.push_back(lir::LirAllocaOp{tmp_addr, res_ty, "", align});
  module_->need_memcpy = true;
  emit_lir_op(ctx, lir::LirMemcpyOp{tmp_addr, stack_ptr, std::to_string(size_bytes), false});
  const std::string out = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirLoadOp{out, res_ty, tmp_addr});
  return out;
}

std::string StmtEmitter::emit_amd64_va_arg_from_registers(
    FnCtx& ctx, const TypeSpec& res_ts, const std::string& res_ty,
    const llvm_cc::Amd64VarargInfo& layout, const Amd64VaListPtrs& access,
    const std::string& gp_offset, const std::string& fp_offset) {
  const int size_bytes = layout.size_bytes;
  const int align = object_align_bytes(mod_, res_ts);

  std::string gp_offset_i64;
  if (layout.gp_chunks > 0) {
    gp_offset_i64 = fresh_tmp(ctx);
    emit_lir_op(
        ctx, lir::LirCastOp{gp_offset_i64, lir::LirCastKind::SExt, "i32", gp_offset, "i64"});
  }
  std::string fp_offset_i64;
  if (layout.sse_slots > 0) {
    fp_offset_i64 = fresh_tmp(ctx);
    emit_lir_op(
        ctx, lir::LirCastOp{fp_offset_i64, lir::LirCastKind::SExt, "i32", fp_offset, "i64"});
  }

  std::string gp_base_ptr;
  if (layout.gp_chunks > 0) {
    gp_base_ptr = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirGepOp{
                         gp_base_ptr, "i8", access.reg_save_area_ptr, false,
                         {"i64 " + gp_offset_i64}});
    const std::string new_gp = fresh_tmp(ctx);
    emit_lir_op(
        ctx, lir::LirBinOp{new_gp, "add", "i32", gp_offset, std::to_string(layout.gp_chunks * 8)});
    emit_lir_op(ctx, lir::LirStoreOp{std::string("i32"), new_gp, access.gp_offset_ptr});
  }

  std::string fp_base_ptr;
  if (layout.sse_slots > 0) {
    fp_base_ptr = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirGepOp{
                         fp_base_ptr, "i8", access.reg_save_area_ptr, false,
                         {"i64 " + fp_offset_i64}});
    const std::string new_fp = fresh_tmp(ctx);
    emit_lir_op(ctx,
                lir::LirBinOp{new_fp, "add", "i32", fp_offset,
                              std::to_string(layout.sse_slots * 16)});
    emit_lir_op(ctx, lir::LirStoreOp{std::string("i32"), new_fp, access.fp_offset_ptr});
  }

  const std::string tmp_addr = fresh_tmp(ctx);
  ctx.alloca_insts.push_back(lir::LirAllocaOp{tmp_addr, res_ty, "", align});

  int gp_chunk_index = 0;
  int sse_slot_index = 0;
  std::string active_sse_slot_ptr;
  for (size_t chunk = 0; chunk < layout.classes.size(); ++chunk) {
    if (static_cast<int>(chunk) * 8 >= size_bytes) break;
    const int chunk_offset = static_cast<int>(chunk) * 8;
    const int chunk_size = std::min(8, size_bytes - static_cast<int>(chunk) * 8);
    const auto cls = layout.classes[chunk];
    if (cls == llvm_cc::Amd64ArgClass::Memory || cls == llvm_cc::Amd64ArgClass::None) continue;

    std::string src_ptr;
    if (cls == llvm_cc::Amd64ArgClass::Integer) {
      src_ptr = gp_base_ptr;
      if (gp_chunk_index > 0) {
        src_ptr = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirGepOp{
                             src_ptr, "i8", gp_base_ptr, false,
                             {"i64 " + std::to_string(gp_chunk_index * 8)}});
      }
      ++gp_chunk_index;
    } else if (cls == llvm_cc::Amd64ArgClass::SSE) {
      src_ptr = fp_base_ptr;
      if (sse_slot_index > 0) {
        src_ptr = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirGepOp{
                             src_ptr, "i8", fp_base_ptr, false,
                             {"i64 " + std::to_string(sse_slot_index * 16)}});
      }
      active_sse_slot_ptr = src_ptr;
      ++sse_slot_index;
    } else if (cls == llvm_cc::Amd64ArgClass::SSEUp) {
      if (active_sse_slot_ptr.empty()) active_sse_slot_ptr = fp_base_ptr;
      std::string upper_ptr = fresh_tmp(ctx);
      emit_lir_op(
          ctx, lir::LirGepOp{upper_ptr, "i8", active_sse_slot_ptr, false, {"i64 8"}});
      src_ptr = upper_ptr;
    } else {
      continue;
    }

    std::string dst_ptr = tmp_addr;
    if (chunk_offset > 0) {
      dst_ptr = fresh_tmp(ctx);
      emit_lir_op(
          ctx, lir::LirGepOp{dst_ptr, "i8", tmp_addr, false, {"i64 " + std::to_string(chunk_offset)}});
    }
    module_->need_memcpy = true;
    emit_lir_op(ctx, lir::LirMemcpyOp{dst_ptr, src_ptr, std::to_string(chunk_size), false});
  }

  const std::string out = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirLoadOp{out, res_ty, tmp_addr});
  return out;
}

std::string StmtEmitter::emit_amd64_va_arg(FnCtx& ctx, const TypeSpec& res_ts,
                                           const std::string& res_ty,
                                           const std::string& ap_ptr) {
  const auto layout = llvm_cc::classify_amd64_vararg(res_ts, mod_);
  if (layout.size_bytes <= 0) return "zeroinitializer";
  const auto access = load_amd64_va_list_ptrs(ctx, ap_ptr);
  if (layout.needs_memory) {
    return emit_amd64_va_arg_from_overflow(ctx, res_ts, res_ty, access, layout.size_bytes);
  }

  std::string gp_offset = "0";
  if (layout.gp_chunks > 0) {
    gp_offset = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirLoadOp{gp_offset, std::string("i32"), access.gp_offset_ptr});
  }
  std::string fp_offset = "0";
  if (layout.sse_slots > 0) {
    fp_offset = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirLoadOp{fp_offset, std::string("i32"), access.fp_offset_ptr});
  }

  std::string gp_ok = "true";
  if (layout.gp_chunks > 0) {
    const int gp_limit = 48 - layout.gp_chunks * 8;
    gp_ok = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCmpOp{gp_ok, false, "sle", "i32", gp_offset, std::to_string(gp_limit)});
  }

  std::string fp_ok = "true";
  if (layout.sse_slots > 0) {
    const int fp_limit = 176 - layout.sse_slots * 16;
    fp_ok = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCmpOp{fp_ok, false, "sle", "i32", fp_offset, std::to_string(fp_limit)});
  }

  std::string regs_ok;
  if (layout.gp_chunks > 0 && layout.sse_slots > 0) {
    regs_ok = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirBinOp{regs_ok, "and", "i1", gp_ok, fp_ok});
  } else if (layout.gp_chunks > 0) {
    regs_ok = gp_ok;
  } else {
    regs_ok = fp_ok;
  }

  const std::string reg_lbl = fresh_lbl(ctx, "vaarg.amd64.reg.");
  const std::string stack_lbl = fresh_lbl(ctx, "vaarg.amd64.stack.");
  const std::string join_lbl = fresh_lbl(ctx, "vaarg.amd64.join.");

  emit_condbr_and_open_lbl(ctx, regs_ok, reg_lbl, stack_lbl, reg_lbl);
  const std::string reg_value =
      emit_amd64_va_arg_from_registers(ctx, res_ts, res_ty, layout, access, gp_offset, fp_offset);
  emit_br_and_open_lbl(ctx, join_lbl, stack_lbl);
  const std::string stack_value =
      emit_amd64_va_arg_from_overflow(ctx, res_ts, res_ty, access, layout.size_bytes);
  emit_br_and_open_lbl(ctx, join_lbl, join_lbl);

  const std::string phi = fresh_tmp(ctx);
  emit_lir_op(ctx, lir::LirPhiOp{phi, res_ty, {{reg_value, reg_lbl}, {stack_value, stack_lbl}}});
  return phi;
}

CallTargetInfo StmtEmitter::resolve_call_target_info(FnCtx& ctx, const CallExpr& call,
                                                     const Expr& e) {
  CallTargetInfo info;
  info.builtin_id = call.builtin_id;
  info.builtin = builtin_by_id(info.builtin_id);

  const Expr& callee_e = get_expr(call.callee);
  bool unresolved_external_callee = false;
  std::string unresolved_external_name;
  if (info.builtin && info.builtin->category == BuiltinCategory::AliasCall &&
      !info.builtin->canonical_name.empty()) {
    info.callee_val = llvm_global_sym(std::string(info.builtin->canonical_name));
    unresolved_external_callee = true;
    unresolved_external_name = std::string(info.builtin->canonical_name);
  } else if (const auto* r = std::get_if<DeclRef>(&callee_e.payload);
             r && !r->local && !r->param_index && !r->global) {
    info.callee_val = llvm_global_sym(r->name);
    info.callee_ts = resolve_payload_type(ctx, *r);
    unresolved_external_callee = true;
    unresolved_external_name = r->name;
  } else {
    info.callee_val = emit_rval_id(ctx, call.callee, info.callee_ts);
  }

  info.ret_spec = resolve_payload_type(ctx, call);
  if (info.ret_spec.base == TB_VOID && info.ret_spec.ptr_level == 0 &&
      info.ret_spec.array_rank == 0) {
    info.ret_spec = e.type.spec;
  }
  info.ret_ty = llvm_ret_ty(info.ret_spec);
  info.builtin_special =
      info.builtin && info.builtin->lowering != BuiltinLoweringKind::AliasCall;
  if (unresolved_external_callee && !info.builtin_special) {
    record_extern_call_decl(unresolved_external_name, info.ret_ty);
  }

  if (info.builtin_id != BuiltinId::Unknown) {
    info.fn_name = std::string(builtin_name_from_id(info.builtin_id));
  } else if (const auto* r = std::get_if<DeclRef>(&callee_e.payload)) {
    info.fn_name = r->name;
  }

  if (!info.fn_name.empty()) {
    const auto fit = mod_.fn_index.find(info.fn_name);
    if (fit != mod_.fn_index.end() && fit->second.value < mod_.functions.size()) {
      info.target_fn = &mod_.functions[fit->second.value];
    }
  }
  info.callee_fn_ptr_sig = info.target_fn ? nullptr : resolve_callee_fn_ptr_sig(ctx, callee_e);
  if (info.target_fn) {
    info.callee_type_suffix = llvm_fn_type_suffix_str(mod_, *info.target_fn);
  } else if (info.callee_fn_ptr_sig) {
    info.callee_type_suffix = llvm_fn_type_suffix_str(mod_, *info.callee_fn_ptr_sig);
  }

  return info;
}

void StmtEmitter::emit_void_call(FnCtx& ctx, const CallTargetInfo& call_target,
                                 const std::vector<OwnedLirTypedCallArg>& args) {
  emit_lir_op(ctx, make_lir_call_op("", "void", call_target.callee_val, call_target.callee_type_suffix, args));
}

std::string StmtEmitter::emit_call_with_result(
    FnCtx& ctx, const CallTargetInfo& call_target,
    const std::vector<OwnedLirTypedCallArg>& args) {
  const std::string tmp = fresh_tmp(ctx);
  emit_lir_op(
      ctx,
      make_lir_call_op(tmp, call_target.ret_ty, call_target.callee_val, call_target.callee_type_suffix, args));
  return tmp;
}

}  // namespace c4c::codegen::lir
