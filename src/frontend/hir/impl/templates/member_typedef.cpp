#include "templates.hpp"

namespace c4c::hir {

const Node* Lowerer::require_pending_template_type_primary(
    const TypeSpec& ts,
    const PendingTemplateTypeWorkItem& work_item,
    const char* missing_detail,
    DeferredTemplateTypeResult* out_result) const {
  const Node* primary_tpl =
      canonical_template_struct_primary(ts, work_item.owner_primary_def);
  if (primary_tpl) return primary_tpl;
  if (out_result) {
    *out_result = terminal_deferred_template_type(work_item, missing_detail);
  }
  return nullptr;
}

bool Lowerer::resolve_pending_template_owner_if_ready(
    TypeSpec& owner_ts,
    const PendingTemplateTypeWorkItem& work_item,
    const Node* primary_tpl,
    bool spawned_owner_work,
    const char* pending_detail,
    DeferredTemplateTypeResult* out_result) {
  realize_template_struct_if_needed(
      owner_ts, work_item.type_bindings, work_item.nttp_bindings,
      primary_tpl);
  if (!owner_ts.tpl_struct_origin) return true;
  if (out_result) {
    *out_result = blocked_deferred_template_type(
        work_item, pending_detail, spawned_owner_work);
  }
  return false;
}

bool Lowerer::spawn_pending_template_owner_work(
    const PendingTemplateTypeWorkItem& work_item,
    const TypeSpec& owner_ts) {
  const Node* owner_primary_def = work_item.owner_primary_def;
  if (!owner_primary_def && owner_ts.tpl_struct_origin) {
    owner_primary_def = canonical_template_struct_primary(owner_ts);
  }
  return ct_state_->record_pending_template_type(
      PendingTemplateTypeKind::OwnerStruct,
      owner_ts,
      owner_primary_def,
      work_item.type_bindings,
      work_item.nttp_bindings,
      work_item.span,
      work_item.context_name.empty()
          ? "owner-struct"
          : work_item.context_name + ":owner");
}

bool Lowerer::ensure_pending_template_owner_ready(
    TypeSpec& owner_ts,
    const PendingTemplateTypeWorkItem& work_item,
    bool spawn_owner_work,
    const char* missing_detail,
    const char* pending_detail,
    DeferredTemplateTypeResult* out_result) {
  if (!owner_ts.tpl_struct_origin) return true;
  const Node* primary_tpl = require_pending_template_type_primary(
      owner_ts, work_item, missing_detail, out_result);
  if (!primary_tpl) return false;
  bool spawned_owner_work = false;
  if (spawn_owner_work) {
    spawned_owner_work = spawn_pending_template_owner_work(work_item, owner_ts);
  }
  return resolve_pending_template_owner_if_ready(
      owner_ts, work_item, primary_tpl, spawned_owner_work,
      pending_detail, out_result);
}

DeferredTemplateTypeResult Lowerer::resolve_deferred_member_typedef_type(
    const PendingTemplateTypeWorkItem& work_item) {
  TypeSpec owner_ts = work_item.pending_type;
  owner_ts.deferred_member_type_name = nullptr;
  DeferredTemplateTypeResult result;
  if (!ensure_pending_template_owner_ready(
          owner_ts, work_item, true,
          "no primary template def for member typedef owner",
          "owner struct still pending", &result)) {
    return result;
  }
  if (const Node* primary_tpl =
          canonical_template_struct_primary(work_item.pending_type,
                                            work_item.owner_primary_def)) {
    ResolvedTemplateArgs resolved = materialize_template_args(
        primary_tpl, work_item.pending_type, work_item.type_bindings,
        work_item.nttp_bindings);
    SelectedTemplateStructPattern selected;
    if (template_struct_has_pack_params(primary_tpl)) {
      selected.primary_def = primary_tpl;
      selected.selected_pattern = primary_tpl;
    } else {
      selected = select_template_struct_pattern_hir(
          resolved.concrete_args, build_template_struct_env(primary_tpl));
    }
    const Node* selected_def =
        selected.selected_pattern ? selected.selected_pattern : primary_tpl;
    const std::string structured_owner_tag = build_template_mangled_name(
        primary_tpl, selected_def,
        work_item.pending_type.tpl_struct_origin
            ? work_item.pending_type.tpl_struct_origin
            : (primary_tpl->name ? primary_tpl->name : ""),
        resolved);
    if (!structured_owner_tag.empty()) {
      TypeSpec resolved_member{};
      if (resolve_struct_member_typedef_type(
              structured_owner_tag,
              work_item.pending_type.deferred_member_type_name,
              &resolved_member)) {
        return DeferredTemplateTypeResult::resolved();
      }
    }
  }
  if (!owner_ts.tag || !owner_ts.tag[0]) {
    return blocked_deferred_template_type(
        work_item, "owner tag unavailable");
  }
  TypeSpec resolved_member{};
  if (resolve_struct_member_typedef_type(
          owner_ts.tag, work_item.pending_type.deferred_member_type_name,
          &resolved_member)) {
    return DeferredTemplateTypeResult::resolved();
  }
  return terminal_deferred_template_type(
      work_item, "member typedef lookup failed");
}

void Lowerer::seed_template_type_dependency_if_needed(
    const TypeSpec& ts,
    const TypeBindings& tpl_bindings,
    const NttpBindings& nttp_bindings,
    PendingTemplateTypeKind kind,
    const std::string& context_name,
    const Node* span_node) {
  if (!ts.tpl_struct_origin) return;
  seed_pending_template_type(
      ts, tpl_bindings, nttp_bindings, span_node, kind, context_name);
}

void Lowerer::seed_and_resolve_pending_template_type_if_needed(
    TypeSpec& ts,
    const TypeBindings& tpl_bindings,
    const NttpBindings& nttp_bindings,
    const Node* span_node,
    PendingTemplateTypeKind kind,
    const std::string& context_name,
    const Node* primary_tpl) {
  if (!ts.tpl_struct_origin) return;
  seed_pending_template_type(
      ts, tpl_bindings, nttp_bindings, span_node, kind, context_name);
  realize_template_struct_if_needed(
      ts, tpl_bindings, nttp_bindings, primary_tpl);
}

}  // namespace c4c::hir
