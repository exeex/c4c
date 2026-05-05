#include "call.hpp"
#include "call_args_ops.hpp"
#include "canonical_symbol.hpp"

namespace c4c::codegen::lir {

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
    // Legacy compatibility for aggregate carriers that still only have a rendered tag.
    name_id = normalize_lir_aggregate_struct_name_id(
        lir_module, rendered_text, lir_module->struct_names.find(rendered_text), true);
  }
  if (name_id == kInvalidStructName) return LirTypeRef(rendered_text);
  return type.base == TB_UNION ? LirTypeRef::union_type(rendered_text, name_id)
                               : LirTypeRef::struct_type(rendered_text, name_id);
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
  info.builtin_special =
      info.builtin && info.builtin->lowering != BuiltinLoweringKind::AliasCall;
  if (unresolved_external_callee && !info.builtin_special) {
    record_extern_call_decl(unresolved_external_name, info.ret_ty, info.callee_link_name_id);
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
  emit_lir_op(ctx,
              make_lir_call_op_with_return_type_ref("",
                                                    LirTypeRef("void"),
                                                    call_target.callee_val,
                                                    call_target.callee_type_suffix,
                                                    args,
                                                    call_target.callee_link_name_id));
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
          call_target.callee_link_name_id));
  return tmp;
}

}  // namespace c4c::codegen::lir
