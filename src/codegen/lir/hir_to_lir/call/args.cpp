#include "call.hpp"
#include "call_args_ops.hpp"
#include "canonical_symbol.hpp"
#include "../../../llvm/calling_convention.hpp"

namespace c4c::codegen::lir {

namespace llvm_cc = c4c::codegen::llvm_backend;
using namespace stmt_emitter_detail;

namespace {

std::optional<HirRecordOwnerKey> call_aggregate_owner_key_from_type(const TypeSpec& ts) {
  if (ts.record_def && ts.record_def->kind == NK_STRUCT_DEF) {
    const TextId declaration_text_id = ts.record_def->unqualified_text_id;
    if (declaration_text_id != kInvalidText) {
      NamespaceQualifier ns_qual;
      ns_qual.context_id = ts.record_def->namespace_context_id;
      ns_qual.is_global_qualified = ts.record_def->is_global_qualified;
      if (ts.record_def->qualifier_text_ids && ts.record_def->n_qualifier_segments > 0) {
        ns_qual.segment_text_ids.assign(
            ts.record_def->qualifier_text_ids,
            ts.record_def->qualifier_text_ids + ts.record_def->n_qualifier_segments);
      }
      const HirRecordOwnerKey owner_key =
          make_hir_record_owner_key(ns_qual, declaration_text_id);
      if (hir_record_owner_key_has_complete_metadata(owner_key)) return owner_key;
    }
  }

  if (ts.tag_text_id == kInvalidText) return std::nullopt;
  NamespaceQualifier ns_qual;
  ns_qual.context_id = ts.namespace_context_id;
  ns_qual.is_global_qualified = ts.is_global_qualified;
  if (ts.qualifier_text_ids && ts.n_qualifier_segments > 0) {
    ns_qual.segment_text_ids.assign(ts.qualifier_text_ids,
                                    ts.qualifier_text_ids + ts.n_qualifier_segments);
  }
  const HirRecordOwnerKey owner_key = make_hir_record_owner_key(ns_qual, ts.tag_text_id);
  if (hir_record_owner_key_has_complete_metadata(owner_key)) return owner_key;
  return std::nullopt;
}

StructNameId call_aggregate_structured_name_id(const c4c::hir::Module& mod,
                                               const lir::LirModule* module,
                                               const TypeSpec& aggregate_ts) {
  if (!module || (aggregate_ts.base != TB_STRUCT && aggregate_ts.base != TB_UNION) ||
      aggregate_ts.ptr_level != 0 || aggregate_ts.array_rank != 0) {
    return kInvalidStructName;
  }

  const std::optional<HirRecordOwnerKey> owner_key =
      call_aggregate_owner_key_from_type(aggregate_ts);
  if (!owner_key) return kInvalidStructName;
  const SymbolName* structured_tag = mod.find_struct_def_tag_by_owner(*owner_key);
  if (!structured_tag || structured_tag->empty()) return kInvalidStructName;

  const StructNameId name_id =
      module->struct_names.find(llvm_struct_type_str(*structured_tag));
  return module->find_struct_decl(name_id) ? name_id : kInvalidStructName;
}

LirTypeRef lir_call_type_ref(const std::string& rendered_text, LirModule* lir_module,
                             const TypeSpec& type) {
  if ((type.base != TB_STRUCT && type.base != TB_UNION) || type.ptr_level > 0 ||
      type.array_rank > 0 || !type.tag || !type.tag[0] || !lir_module) {
    return LirTypeRef();
  }
  if (rendered_text != llvm_ty(type)) return LirTypeRef();
  const StructNameId name_id = lir_module->struct_names.intern(rendered_text);
  return type.base == TB_UNION ? LirTypeRef::union_type(rendered_text, name_id)
                               : LirTypeRef::struct_type(rendered_text, name_id);
}

}  // namespace

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
    const int va_align = llvm_va_list_alignment(mod_.target_profile);
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
    const StructNameId structured_name_id =
        call_aggregate_structured_name_id(mod_, module_, arg_ts);
    const char* layout_site = structured_name_id == kInvalidStructName
                                  ? "variadic-aggregate-arg-legacy-compat"
                                  : "variadic-aggregate-arg";
    const StructuredLayoutLookup layout =
        lookup_structured_layout(mod_, module_, arg_ts, layout_site, structured_name_id);
    const std::optional<int> structured_payload_sz =
        structured_layout_size_bytes(mod_, module_, layout);
    const int payload_sz =
        structured_payload_sz ? *structured_payload_sz
                              : (layout.legacy_decl ? layout.legacy_decl->size_bytes : 0);
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

    if (llvm_target_is_amd64_sysv(mod_.target_profile)) {
      return prepare_amd64_variadic_aggregate_arg(ctx, arg_ts, obj_ptr, payload_sz, amd64_state);
    }

    module_->need_memcpy = true;
    if (llvm_target_is_aarch64(mod_.target_profile) &&
        !llvm_target_is_apple(mod_.target_profile)) {
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
    const int align = std::max(8, object_align_bytes(mod_, module_, out_arg_ts));
    PreparedCallArg out{{{"ptr byval(" + llvm_ty(out_arg_ts) + ") align " + std::to_string(align),
                          obj_ptr}},
                        false};
    return out;
  }

  if (fixed_param_ts && llvm_cc::aarch64_fixed_vector_passed_as_i32(*fixed_param_ts, mod_)) {
    const std::string packed = fresh_tmp(ctx);
    emit_lir_op(ctx, lir::LirCastOp{packed, lir::LirCastKind::Bitcast,
                                    lir::LirTypeRef(llvm_ty(out_arg_ts)), arg,
                                    lir::LirTypeRef("i32")});
    return {{{"i32", packed}}, false};
  }

  const std::string out_llvm_ty = llvm_ty(out_arg_ts);
  PreparedCallArg out_arg{
      {{out_llvm_ty, arg, lir_call_type_ref(out_llvm_ty, module_, out_arg_ts)}},
      false};
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
    const int align = std::max(8, object_align_bytes(mod_, module_, arg_ts));
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
  if (llvm_target_is_amd64_sysv(mod_.target_profile)) {
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

}  // namespace c4c::codegen::lir
