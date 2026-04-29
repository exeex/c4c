#include "call.hpp"
#include "call_args_ops.hpp"
#include "canonical_symbol.hpp"

namespace c4c::codegen::lir {

using namespace stmt_emitter_detail;

namespace {

std::string emitted_link_name(const c4c::hir::Module& mod, c4c::LinkNameId id,
                              std::string_view fallback) {
  const std::string_view resolved = mod.link_names.spelling(id);
  return resolved.empty() ? std::string(fallback) : std::string(resolved);
}

LirTypeRef lir_call_type_ref(const std::string& rendered_text, LirModule* lir_module,
                             const TypeSpec& type) {
  if ((type.base != TB_STRUCT && type.base != TB_UNION) || type.ptr_level > 0 ||
      type.array_rank > 0 || !type.tag || !type.tag[0] || !lir_module) {
    return LirTypeRef(rendered_text);
  }
  if (rendered_text != llvm_ty(type)) return LirTypeRef(rendered_text);
  const StructNameId name_id = lir_module->struct_names.intern(rendered_text);
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

  info.ret_ty = llvm_ret_ty(info.ret_spec);
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
          lir_call_type_ref(call_target.ret_ty, module_, call_target.ret_spec),
          call_target.callee_val,
          call_target.callee_type_suffix,
          args,
          call_target.callee_link_name_id));
  return tmp;
}

}  // namespace c4c::codegen::lir
