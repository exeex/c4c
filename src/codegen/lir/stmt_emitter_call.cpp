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

bool StmtEmitter::callee_needs_va_list_by_value_copy(const CallTargetInfo& call_target,
                                                     size_t arg_index) const {
  const Function* target_fn = call_target.target_fn;
  const FnPtrSig* callee_fn_ptr_sig = call_target.callee_fn_ptr_sig;
  if (target_fn) {
    if (arg_index < target_fn->params.size()) {
      const TypeSpec& param_ts = target_fn->params[arg_index].type.spec;
      return param_ts.base == TB_VA_LIST && param_ts.ptr_level == 0 && param_ts.array_rank == 0;
    }
    return false;
  }
  if (callee_fn_ptr_sig) {
    if (arg_index < sig_param_count(*callee_fn_ptr_sig)) {
      return sig_param_is_va_list_value(*callee_fn_ptr_sig, arg_index);
    }
    return false;
  }
  return call_target.fn_name == "vprintf" || call_target.fn_name == "vfprintf" ||
         call_target.fn_name == "vsprintf" || call_target.fn_name == "vsnprintf" ||
         call_target.fn_name == "vscanf" || call_target.fn_name == "vfscanf" ||
         call_target.fn_name == "vsscanf" || call_target.fn_name == "vasprintf" ||
         call_target.fn_name == "vdprintf";
}

void StmtEmitter::apply_default_arg_promotion(FnCtx& ctx, std::string& arg, TypeSpec& out_ts,
                                              const TypeSpec& in_ts) {
  TypeSpec promoted = in_ts;
  if (promoted.array_rank > 0 && !promoted.is_ptr_to_array) {
    promoted.array_rank = 0;
    promoted.array_size = -1;
    promoted.ptr_level += 1;
  }
  if (promoted.ptr_level == 0 && promoted.array_rank == 0) {
    if (promoted.base == TB_FLOAT) {
      promoted.base = TB_DOUBLE;
    } else if (promoted.base == TB_BOOL || promoted.base == TB_CHAR ||
               promoted.base == TB_SCHAR || promoted.base == TB_UCHAR ||
               promoted.base == TB_SHORT || promoted.base == TB_USHORT) {
      promoted.base = TB_INT;
    }
  }
  arg = coerce(ctx, arg, in_ts, promoted);
  out_ts = promoted;
}

PreparedCallArg StmtEmitter::prepare_call_arg(FnCtx& ctx, const CallExpr& call,
                                              const CallTargetInfo& call_target,
                                              size_t arg_index,
                                              Amd64CallArgState* amd64_state) {
  const Function* target_fn = call_target.target_fn;
  const FnPtrSig* callee_fn_ptr_sig = call_target.callee_fn_ptr_sig;
  const TypeSpec* fixed_param_ts = nullptr;
  TypeSpec fn_ptr_param_ts{};
  if (target_fn) {
    const bool has_void_param_list = target_fn->params.size() == 1 &&
                                     target_fn->params[0].type.spec.base == TB_VOID &&
                                     target_fn->params[0].type.spec.ptr_level == 0 &&
                                     target_fn->params[0].type.spec.array_rank == 0;
    if (!has_void_param_list && arg_index < target_fn->params.size()) {
      fixed_param_ts = &target_fn->params[arg_index].type.spec;
    }
  } else if (callee_fn_ptr_sig) {
    const bool has_void_pl = sig_has_void_param_list(*callee_fn_ptr_sig);
    if (!has_void_pl && arg_index < sig_param_count(*callee_fn_ptr_sig)) {
      fn_ptr_param_ts = sig_param_type(*callee_fn_ptr_sig, arg_index);
      fixed_param_ts = &fn_ptr_param_ts;
    }
  }

  TypeSpec arg_ts{};
  std::string arg;
  const bool is_fixed_byval_aggregate =
      fixed_param_ts && amd64_fixed_aggregate_byval(mod_, *fixed_param_ts);
  if (is_fixed_byval_aggregate &&
      get_expr(call.args[arg_index]).type.category == ValueCategory::LValue) {
    TypeSpec obj_ts{};
    arg = emit_lval(ctx, call.args[arg_index], obj_ts);
    arg_ts = obj_ts;
  } else if (fixed_param_ts &&
             (fixed_param_ts->is_lvalue_ref || fixed_param_ts->is_rvalue_ref)) {
    try {
      TypeSpec pointee_ts{};
      arg = emit_lval(ctx, call.args[arg_index], pointee_ts);
      arg_ts = pointee_ts;
      arg_ts.ptr_level += 1;
      arg_ts.is_lvalue_ref = fixed_param_ts->is_lvalue_ref;
      arg_ts.is_rvalue_ref = fixed_param_ts->is_rvalue_ref;
    } catch (const std::runtime_error&) {
      arg = emit_rval_id(ctx, call.args[arg_index], arg_ts);
    }
  } else {
    arg = emit_rval_id(ctx, call.args[arg_index], arg_ts);
  }

  TypeSpec out_arg_ts = arg_ts;
  const bool is_va_list_value =
      arg_ts.base == TB_VA_LIST && arg_ts.ptr_level == 0 && arg_ts.array_rank == 0;
  const bool is_variadic_aggregate =
      target_fn && target_fn->attrs.variadic && arg_index >= target_fn->params.size() &&
      (arg_ts.base == TB_STRUCT || arg_ts.base == TB_UNION) && arg_ts.ptr_level == 0 &&
      arg_ts.array_rank == 0 && arg_ts.tag && arg_ts.tag[0];
  if (target_fn) {
    const bool has_void_param_list = target_fn->params.size() == 1 &&
                                     target_fn->params[0].type.spec.base == TB_VOID &&
                                     target_fn->params[0].type.spec.ptr_level == 0 &&
                                     target_fn->params[0].type.spec.array_rank == 0;
    if (!has_void_param_list && arg_index < target_fn->params.size()) {
      out_arg_ts = target_fn->params[arg_index].type.spec;
      if (!is_fixed_byval_aggregate) {
        arg = coerce(ctx, arg, arg_ts, out_arg_ts);
      }
    } else if (target_fn->attrs.variadic && !is_variadic_aggregate) {
      apply_default_arg_promotion(ctx, arg, out_arg_ts, arg_ts);
    }
  } else if (callee_fn_ptr_sig) {
    const bool has_void_pl = sig_has_void_param_list(*callee_fn_ptr_sig);
    if (!has_void_pl && arg_index < sig_param_count(*callee_fn_ptr_sig)) {
      out_arg_ts = sig_param_type(*callee_fn_ptr_sig, arg_index);
      if (!is_fixed_byval_aggregate) {
        arg = coerce(ctx, arg, arg_ts, out_arg_ts);
      }
    } else if (sig_is_variadic(*callee_fn_ptr_sig) && !is_variadic_aggregate) {
      apply_default_arg_promotion(ctx, arg, out_arg_ts, arg_ts);
    }
  }
  if (is_va_list_value && callee_needs_va_list_by_value_copy(call_target, arg_index)) {
    TypeSpec ap_ts{};
    const std::string src_ptr = emit_va_list_obj_ptr(ctx, call.args[arg_index], ap_ts);
    const std::string tmp_addr = fresh_tmp(ctx);
    const int va_align = llvm_va_list_alignment(mod_.target_triple);
    ctx.alloca_insts.push_back(
        lir::LirAllocaOp{tmp_addr, llvm_va_list_storage_ty(), {}, va_align});
    module_->need_memcpy = true;
    emit_lir_op(
        ctx,
        lir::LirMemcpyOp{tmp_addr, src_ptr, std::to_string(llvm_va_list_storage_size()), false});
    arg = tmp_addr;
    out_arg_ts = arg_ts;
  }
  if (is_variadic_aggregate) {
    const auto sit = mod_.struct_defs.find(arg_ts.tag);
    const int payload_sz = sit == mod_.struct_defs.end() ? 0 : sit->second.size_bytes;
    if (payload_sz == 0) return {{}, true};

    std::string obj_ptr;
    if (get_expr(call.args[arg_index]).type.category == ValueCategory::LValue) {
      TypeSpec obj_ts{};
      obj_ptr = emit_lval(ctx, call.args[arg_index], obj_ts);
    } else {
      const std::string tmp_addr = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirAllocaOp{tmp_addr, llvm_ty(arg_ts), {}, 0});
      emit_lir_op(ctx, lir::LirStoreOp{llvm_ty(arg_ts), arg, tmp_addr});
      obj_ptr = tmp_addr;
    }

    if (llvm_target_is_amd64_sysv(mod_.target_triple)) {
      return prepare_amd64_variadic_aggregate_arg(ctx, arg_ts, obj_ptr, payload_sz, amd64_state);
    }

    module_->need_memcpy = true;
    if (llvm_target_is_aarch64(mod_.target_triple) &&
        !llvm_target_is_apple(mod_.target_triple)) {
      if (const auto hfa = classify_aarch64_hfa(mod_, arg_ts)) {
        const std::string coerced_ty =
            "[" + std::to_string(hfa->elem_count) + " x " + hfa->elem_ty + "]";
        const std::string tmp_addr = fresh_tmp(ctx);
        emit_lir_op(ctx,
                    lir::LirAllocaOp{tmp_addr, coerced_ty, {}, hfa->aggregate_align});
        emit_lir_op(ctx, lir::LirMemcpyOp{tmp_addr, obj_ptr, std::to_string(hfa->aggregate_size),
                                          false});
        const std::string packed = fresh_tmp(ctx);
        emit_lir_op(ctx, lir::LirLoadOp{packed, coerced_ty, tmp_addr});
        return {{{coerced_ty + " alignstack(" + std::to_string(std::max(8, hfa->aggregate_align)) +
                      ")",
                  packed}},
                false};
      }
    }
    if (payload_sz > 16) {
      return {{{"ptr", obj_ptr}}, false};
    }
    if (payload_sz <= 8) {
      const std::string tmp_addr = fresh_tmp(ctx);
      module_->need_memcpy = true;
      emit_lir_op(ctx, lir::LirAllocaOp{tmp_addr, "i64", {}, 0});
      emit_lir_op(ctx, lir::LirMemcpyOp{tmp_addr, obj_ptr, std::to_string(payload_sz), false});
      const std::string packed = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirLoadOp{packed, std::string("i64"), tmp_addr});
      return {{{"i64", packed}}, false};
    }

    const std::string tmp_addr = fresh_tmp(ctx);
    module_->need_memcpy = true;
    emit_lir_op(ctx, lir::LirAllocaOp{tmp_addr, "[2 x i64]", {}, 0});
    emit_lir_op(ctx, lir::LirMemcpyOp{tmp_addr, obj_ptr, std::to_string(payload_sz), false});
    const std::string packed = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirLoadOp{packed, std::string("[2 x i64]"), tmp_addr});
    return {{{"[2 x i64]", packed}}, false};
  }

  if (is_fixed_byval_aggregate) {
    std::string obj_ptr;
    if (get_expr(call.args[arg_index]).type.category == ValueCategory::LValue) {
      TypeSpec obj_ts{};
      obj_ptr = emit_lval(ctx, call.args[arg_index], obj_ts);
    } else {
      const std::string tmp_addr = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirAllocaOp{tmp_addr, llvm_ty(out_arg_ts), {}, 0});
      emit_lir_op(ctx, lir::LirStoreOp{llvm_ty(out_arg_ts), arg, tmp_addr});
      obj_ptr = tmp_addr;
    }
    const int align = std::max(8, object_align_bytes(mod_, out_arg_ts));
    PreparedCallArg out{{{"ptr byval(" + llvm_ty(out_arg_ts) + ") align " + std::to_string(align),
                          obj_ptr}},
                        false};
    return out;
  }

  PreparedCallArg out_arg{{{llvm_ty(out_arg_ts), arg}}, false};
  if (amd64_state && !out_arg.skip) {
    amd64_account_type_if_needed(mod_, out_arg_ts, amd64_state);
  }
  return out_arg;
}

PreparedCallArg StmtEmitter::prepare_amd64_variadic_aggregate_arg(
    FnCtx& ctx, const TypeSpec& arg_ts, const std::string& obj_ptr, int payload_sz,
    Amd64CallArgState* amd64_state) {
  PreparedCallArg out;
  const auto layout = llvm_cc::classify_amd64_vararg(arg_ts, mod_);
  if (layout.size_bytes <= 0) {
    out.skip = true;
    return out;
  }
  bool force_memory = layout.needs_memory || layout.size_bytes > 16;
  if (!force_memory && amd64_state && !amd64_registers_available(layout, *amd64_state)) {
    force_memory = true;
  }
  if (force_memory) {
    const int align = std::max(8, object_align_bytes(mod_, arg_ts));
    out.args.push_back({"ptr byval(" + llvm_ty(arg_ts) + ") align " + std::to_string(align),
                        obj_ptr});
    return out;
  }

  const auto make_byte_ptr = [&](const std::string& base, int offset) {
    if (offset == 0) {
      const std::string zero_ptr = fresh_tmp(ctx);
      emit_lir_op(ctx, lir::LirGepOp{zero_ptr, "i8", base, false, {"i64 0"}});
      return zero_ptr;
    }
    const std::string ptr = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirGepOp{ptr, "i8", base, false,
                                   {"i64 " + std::to_string(offset)}});
    return ptr;
  };

  const auto copy_chunk = [&](const std::string& llvm_ty, const std::string& zero_value,
                              const std::string& src_ptr, int copy_bytes) {
    const std::string tmp_addr = fresh_tmp(ctx);
    ctx.alloca_insts.push_back(lir::LirAllocaOp{tmp_addr, llvm_ty, "", 0});
    emit_lir_op(ctx, lir::LirStoreOp{llvm_ty, zero_value, tmp_addr});
    module_->need_memcpy = true;
    emit_lir_op(ctx, lir::LirMemcpyOp{tmp_addr, src_ptr, std::to_string(copy_bytes), false});
    const std::string loaded = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirLoadOp{loaded, llvm_ty, tmp_addr});
    return loaded;
  };

  const std::string base_ptr = make_byte_ptr(obj_ptr, 0);
  for (size_t chunk = 0; chunk < layout.classes.size(); ++chunk) {
    const int chunk_offset = static_cast<int>(chunk) * 8;
    if (chunk_offset >= payload_sz) break;
    const int chunk_size = std::min(8, payload_sz - chunk_offset);
    const auto cls = layout.classes[chunk];
    if (cls == llvm_cc::Amd64ArgClass::None) continue;
    std::string chunk_ptr = base_ptr;
    if (chunk_offset > 0) {
      chunk_ptr = make_byte_ptr(base_ptr, chunk_offset);
    }
    if (cls == llvm_cc::Amd64ArgClass::Integer) {
      const std::string loaded = copy_chunk("i64", "0", chunk_ptr, chunk_size);
      out.args.push_back({"i64", loaded});
      continue;
    }
    if (cls == llvm_cc::Amd64ArgClass::SSE) {
      const bool combine = (chunk + 1 < layout.classes.size() &&
                            layout.classes[chunk + 1] == llvm_cc::Amd64ArgClass::SSEUp &&
                            chunk_offset + 8 < payload_sz);
      if (combine) {
        const int combined_size = std::min(16, payload_sz - chunk_offset);
        const std::string loaded =
            copy_chunk("<2 x i64>", "zeroinitializer", chunk_ptr, combined_size);
        out.args.push_back({"<2 x i64>", loaded});
        ++chunk;
        continue;
      }
      const std::string loaded = copy_chunk("double", "0.0", chunk_ptr, chunk_size);
      out.args.push_back({"double", loaded});
      continue;
    }
    if (cls == llvm_cc::Amd64ArgClass::SSEUp) {
      const std::string loaded = copy_chunk("double", "0.0", chunk_ptr, chunk_size);
      out.args.push_back({"double", loaded});
      continue;
    }
    out.args.clear();
    out.args.push_back({"ptr", obj_ptr});
    return out;
  }
  if (amd64_state) {
    amd64_track_usage(layout, *amd64_state);
  }
  return out;
}

std::vector<OwnedLirTypedCallArg> StmtEmitter::prepare_call_args(
    FnCtx& ctx, const CallExpr& call, const CallTargetInfo& call_target) {
  std::vector<OwnedLirTypedCallArg> args;
  Amd64CallArgState amd64_state;
  amd64_state.sse_bytes = kAmd64GpAreaBytes;
  Amd64CallArgState* amd64_state_ptr = nullptr;
  if (llvm_target_is_amd64_sysv(mod_.target_triple)) {
    amd64_state_ptr = &amd64_state;
  }
  for (size_t i = 0; i < call.args.size(); ++i) {
    PreparedCallArg prepared = prepare_call_arg(ctx, call, call_target, i, amd64_state_ptr);
    if (prepared.skip) continue;
    for (const OwnedLirTypedCallArg& arg : prepared.args) {
      if (arg.type.empty() || arg.operand.empty()) continue;
      args.push_back(arg);
    }
  }
  return args;
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
