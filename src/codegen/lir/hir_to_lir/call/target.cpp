#include "call.hpp"
#include "call_args_ops.hpp"
#include "canonical_symbol.hpp"
#include "../../../llvm/calling_convention.hpp"

namespace c4c::codegen::lir {

namespace llvm_cc = c4c::codegen::llvm_backend;
using namespace stmt_emitter_detail;

namespace {

StructNameId call_target_aggregate_structured_name_id(const c4c::hir::Module& mod,
                                                      const lir::LirModule* module,
                                                      const std::string& rendered_text,
                                                      const TypeSpec& aggregate_ts) {
  if (!module || (aggregate_ts.base != TB_STRUCT && aggregate_ts.base != TB_UNION) ||
      aggregate_ts.ptr_level != 0 || aggregate_ts.array_rank != 0) {
    return kInvalidStructName;
  }

  const std::optional<HirRecordOwnerKey> owner_key =
      typespec_aggregate_owner_key(aggregate_ts, mod);
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

LirTypeRef lir_call_type_ref(const std::string& rendered_text, LirModule* lir_module,
                             const c4c::hir::Module& mod, const TypeSpec& type) {
  if ((type.base != TB_STRUCT && type.base != TB_UNION) || type.ptr_level > 0 ||
      type.array_rank > 0 || !lir_module) {
    return LirTypeRef(rendered_text);
  }
  if (typespec_aggregate_complete_owner_key_missed(type, mod)) {
    return LirTypeRef(rendered_text);
  }

  StructNameId name_id =
      call_target_aggregate_structured_name_id(mod, lir_module, rendered_text, type);
  if (name_id == kInvalidStructName) {
    const std::optional<std::string> structured_text = llvm_aggregate_value_ty(mod, type);
    if (structured_text && *structured_text == rendered_text) {
      name_id = normalize_lir_aggregate_struct_name_id(
          lir_module, rendered_text, lir_module->struct_names.find(rendered_text), true);
    }
  }
  if (name_id == kInvalidStructName &&
      !typespec_legacy_tag_if_present(type, 0).empty()) {
    // Legacy no-owner compatibility for aggregate carriers that still only have a
    // rendered tag. Complete owner-key misses return above so this cannot become
    // secondary structured authority.
    name_id = normalize_lir_aggregate_struct_name_id(
        lir_module, rendered_text, lir_module->struct_names.find(rendered_text), true);
  }
  if (name_id == kInvalidStructName) return LirTypeRef(rendered_text);
  return type.base == TB_UNION ? LirTypeRef::union_type(rendered_text, name_id)
                               : LirTypeRef::struct_type(rendered_text, name_id);
}

std::string rendered_call_signature_param_type(const c4c::hir::Module& mod,
                                               const LirModule* lir_module,
                                               const TypeSpec& param_ts) {
  if (amd64_fixed_aggregate_byval(mod, param_ts)) {
    const std::string byval_pointee_ty = llvm_value_ty(mod, param_ts);
    return "ptr byval(" + byval_pointee_ty + ") align " +
           std::to_string(std::max(8, object_align_bytes(mod, lir_module, param_ts)));
  }
  if (llvm_cc::aarch64_fixed_vector_passed_as_i32(param_ts, mod)) return "i32";
  return llvm_value_ty(mod, param_ts);
}

bool is_aarch64_fixed_hfa_param(const c4c::hir::Module& mod, const TypeSpec& ts) {
  using namespace c4c::codegen::llvm_helpers;
  return llvm_target_is_aarch64(mod.target_profile) &&
         !llvm_target_is_apple(mod.target_profile) &&
         classify_aarch64_hfa(mod, ts).has_value();
}

bool llvm_target_is_rv64(const c4c::TargetProfile& target_profile) {
  return target_profile.arch == c4c::TargetArch::Riscv64;
}

LirExtAttr rv64_integer_ext_attr_for_abi_type(const c4c::hir::Module& mod,
                                              const TypeSpec& ts) {
  if (!llvm_target_is_rv64(mod.target_profile) ||
      ts.ptr_level != 0 || ts.array_rank != 0 ||
      !is_any_int(ts.base) || int_bits(ts.base) != 32) {
    return LirExtAttr::None;
  }
  return is_signed_int(ts.base) ? LirExtAttr::SignExt : LirExtAttr::ZeroExt;
}

void append_call_signature_param(LirCallSignature& out,
                                 const c4c::hir::Module& mod,
                                 LirModule* lir_module,
                                 const TypeSpec& param_ts) {
  if (const auto hfa = is_aarch64_fixed_hfa_param(mod, param_ts)
                           ? classify_aarch64_hfa(mod, param_ts)
                           : std::nullopt;
      hfa.has_value()) {
    for (int lane_index = 0; lane_index < hfa->elem_count; ++lane_index) {
      out.fixed_param_types.push_back(hfa->elem_ty);
      out.fixed_param_type_refs.push_back(LirTypeRef(hfa->elem_ty));
    }
    return;
  }
  const std::string rendered_type =
      rendered_call_signature_param_type(mod, lir_module, param_ts);
  out.fixed_param_types.push_back(rendered_type);
  out.fixed_param_type_refs.push_back(
      lir_call_type_ref(rendered_type, lir_module, mod, param_ts));
}

LirCallSignature lir_call_signature_from_fn_ptr_sig(const c4c::hir::Module& mod,
                                                    LirModule* lir_module,
                                                    const FnPtrSig& sig) {
  LirCallSignature out;
  const TypeSpec return_ts = sig_return_type(sig);
  out.return_type_ref = lir_call_type_ref(
      llvm_return_ty(mod, return_ts), lir_module, mod, return_ts);
  out.is_variadic = sig_is_variadic(sig);
  out.has_unspecified_params = sig.unspecified_params;
  out.has_void_param_list = sig_has_void_param_list(sig);
  if (out.has_void_param_list) return out;

  const size_t param_count = sig_param_count(sig);
  out.fixed_param_types.reserve(param_count);
  out.fixed_param_type_refs.reserve(param_count);
  for (size_t index = 0; index < param_count; ++index) {
    const TypeSpec param_ts = sig_param_type(sig, index);
    append_call_signature_param(out, mod, lir_module, param_ts);
  }
  return out;
}

bool function_has_void_param_list(const Function& fn) {
  return fn.params.size() == 1 &&
         fn.params[0].type.spec.base == TB_VOID &&
         fn.params[0].type.spec.ptr_level == 0 &&
         fn.params[0].type.spec.array_rank == 0;
}

LirCallSignature lir_call_signature_from_function(const c4c::hir::Module& mod,
                                                  LirModule* lir_module,
                                                  const Function& fn) {
  LirCallSignature out;
  out.return_type_ref = lir_call_type_ref(
      llvm_return_ty(mod, fn.return_type.spec), lir_module, mod, fn.return_type.spec);
  out.is_variadic = fn.attrs.variadic;
  out.has_unspecified_params = fn.attrs.unspecified_params;
  out.has_void_param_list = function_has_void_param_list(fn);
  if (out.has_void_param_list) return out;
  if (out.has_unspecified_params) return out;

  out.fixed_param_types.reserve(fn.params.size());
  out.fixed_param_type_refs.reserve(fn.params.size());
  for (const auto& param : fn.params) {
    const TypeSpec& param_ts = param.type.spec;
    append_call_signature_param(out, mod, lir_module, param_ts);
  }
  return out;
}

std::optional<LirCallSignature> structured_callee_signature(
    const c4c::hir::Module& mod,
    LirModule* lir_module,
    const CallTargetInfo& call_target) {
  if (call_target.target_fn) {
    LirCallSignature sig =
        lir_call_signature_from_function(mod, lir_module, *call_target.target_fn);
    sig.return_ext_attr = call_target.return_ext_attr;
    return sig;
  }
  if (!call_target.callee_fn_ptr_sig) return std::nullopt;
  LirCallSignature sig = lir_call_signature_from_fn_ptr_sig(
      mod, lir_module, *call_target.callee_fn_ptr_sig);
  sig.return_ext_attr = call_target.return_ext_attr;
  return sig;
}

}  // namespace

const Function* StmtEmitter::find_local_target_function(
    LinkNameId link_name_id, std::string_view fallback_name) const {
  if (const Function* fn = mod_.find_function(link_name_id)) return fn;
  if (link_name_id != kInvalidLinkName) return nullptr;
  if (fallback_name.empty()) return nullptr;
  return mod_.find_function_by_name_legacy(fallback_name);
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
    const std::string callee_name = emitted_link_name(mod_, r->link_name_id, r->name);
    info.callee_val = llvm_global_sym(callee_name);
    info.callee_ts = resolve_payload_type(ctx, *r);
    unresolved_external_callee = true;
    unresolved_external_name = callee_name;
    info.callee_link_name_id = r->link_name_id;
  } else {
    info.callee_val = emit_rval_id(ctx, call.callee, info.callee_ts);
  }

  info.ret_spec = resolve_payload_type(ctx, call);
  if (info.ret_spec.base == TB_VOID && info.ret_spec.ptr_level == 0 &&
      info.ret_spec.array_rank == 0) {
    info.ret_spec = e.type.spec;
  }

  if (info.builtin_id != BuiltinId::Unknown) {
    info.fn_name = std::string(builtin_name_from_id(info.builtin_id));
  } else if (const auto* r = std::get_if<DeclRef>(&callee_e.payload)) {
    info.fn_name = r->name;
  }

  info.target_fn = find_local_target_function(info.callee_link_name_id, info.fn_name);
  if (info.target_fn != nullptr) {
    info.callee_link_name_id = info.target_fn->link_name_id;
    info.fn_name = emitted_link_name(mod_, info.target_fn->link_name_id, info.target_fn->name);
    if (!info.fn_name.empty()) {
      info.callee_val = llvm_global_sym(info.fn_name);
    }
    info.ret_spec = info.target_fn->return_type.spec;
    if ((info.ret_spec.is_lvalue_ref || info.ret_spec.is_rvalue_ref) &&
        info.ret_spec.ptr_level == 0) {
      info.ret_spec.ptr_level++;
    }
    unresolved_external_callee = false;
  }

  info.ret_ty = llvm_return_ty(mod_, info.ret_spec);
  if (info.target_fn && info.target_fn->linkage.is_extern &&
      info.target_fn->blocks.empty() && info.target_fn->attrs.variadic) {
    info.return_ext_attr =
        rv64_integer_ext_attr_for_abi_type(mod_, info.target_fn->return_type.spec);
    if (info.return_ext_attr == LirExtAttr::None &&
        llvm_target_is_rv64(mod_.target_profile) &&
        info.ret_ty == "i32") {
      info.return_ext_attr = LirExtAttr::SignExt;
    }
  }
  info.builtin_special =
      info.builtin && info.builtin->lowering != BuiltinLoweringKind::AliasCall;
  info.callee_fn_ptr_sig = info.target_fn ? nullptr : resolve_callee_fn_ptr_sig(ctx, callee_e);
  if (info.target_fn) {
    info.callee_type_suffix = llvm_fn_type_suffix_str(mod_, *info.target_fn);
  } else if (info.callee_fn_ptr_sig) {
    info.callee_type_suffix = llvm_fn_type_suffix_str(mod_, *info.callee_fn_ptr_sig);
  }
  const bool unresolved_external_variadic =
      unresolved_external_callee && !info.builtin_special &&
      info.callee_fn_ptr_sig && sig_is_variadic(*info.callee_fn_ptr_sig);
  if (unresolved_external_variadic) {
    info.return_ext_attr =
        rv64_integer_ext_attr_for_abi_type(mod_, sig_return_type(*info.callee_fn_ptr_sig));
    if (info.return_ext_attr == LirExtAttr::None &&
        llvm_target_is_rv64(mod_.target_profile) &&
        info.ret_ty == "i32") {
      info.return_ext_attr = LirExtAttr::SignExt;
    }
  }
  if (unresolved_external_callee && !info.builtin_special) {
    record_extern_call_decl(unresolved_external_name, info.ret_ty,
                            info.callee_link_name_id, info.return_ext_attr);
  }

  return info;
}

void StmtEmitter::emit_void_call(FnCtx& ctx, const CallTargetInfo& call_target,
                                 const std::vector<OwnedLirTypedCallArg>& args) {
  emit_lir_op(ctx,
              make_lir_call_op_with_return_type_ref("",
                                                    LirTypeRef("void"),
                                                    call_target.callee_val,
                                                    call_target.callee_type_suffix,
                                                    args,
                                                    call_target.callee_link_name_id,
                                                    structured_callee_signature(
                                                        mod_, module_, call_target)));
}

std::string StmtEmitter::emit_call_with_result(
    FnCtx& ctx, const CallTargetInfo& call_target,
    const std::vector<OwnedLirTypedCallArg>& args) {
  const std::string tmp = fresh_tmp(ctx);
  emit_lir_op(
      ctx,
      make_lir_call_op_with_return_type_ref(
          tmp,
          lir_call_type_ref(call_target.ret_ty, module_, mod_, call_target.ret_spec),
          call_target.callee_val,
          call_target.callee_type_suffix,
          args,
          call_target.callee_link_name_id,
          structured_callee_signature(mod_, module_, call_target)));
  return tmp;
}

}  // namespace c4c::codegen::lir
