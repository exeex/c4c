#include "templates.hpp"

namespace c4c::hir {

namespace {

bool resolve_member_typedef_from_record_def(const Node* record_def,
                                            const char* member,
                                            TypeSpec* out) {
  if (!record_def || record_def->kind != NK_STRUCT_DEF || !member ||
      !member[0] || !out) {
    return false;
  }
  if (!record_def->member_typedef_names || !record_def->member_typedef_types) {
    return false;
  }
  for (int i = 0; i < record_def->n_member_typedefs; ++i) {
    const char* alias_name = record_def->member_typedef_names[i];
    if (!alias_name || std::string(alias_name) != member) continue;
    *out = record_def->member_typedef_types[i];
    return true;
  }
  return false;
}

}  // namespace

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
  const char* member_name = work_item.pending_type.deferred_member_type_name;
  DeferredTemplateTypeResult result;
  if (!ensure_pending_template_owner_ready(
          owner_ts, work_item, true,
          "no primary template def for member typedef owner",
          "owner struct still pending", &result)) {
    return result;
  }
  bool has_structured_owner = false;
  TypeSpec resolved_member{};
  if (owner_ts.record_def && owner_ts.record_def->kind == NK_STRUCT_DEF) {
    has_structured_owner = true;
    if (resolve_member_typedef_from_record_def(
            owner_ts.record_def, member_name, &resolved_member)) {
      return DeferredTemplateTypeResult::resolved();
    }
    const std::optional<HirRecordOwnerKey> owner_key =
        make_struct_def_node_owner_key(owner_ts.record_def);
    if (owner_key) {
      if (const SymbolName* owner_tag =
              module_->find_struct_def_tag_by_owner(*owner_key)) {
        if (resolve_struct_member_typedef_type(
                *owner_tag, member_name, &resolved_member)) {
          return DeferredTemplateTypeResult::resolved();
        }
      }
    }
  }
  if (const Node* primary_tpl =
          canonical_template_struct_primary(work_item.pending_type,
                                            work_item.owner_primary_def)) {
    has_structured_owner = true;
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
      if (resolve_struct_member_typedef_type(
              structured_owner_tag,
              member_name, &resolved_member)) {
        return DeferredTemplateTypeResult::resolved();
      }
    }
  }
  if (!has_structured_owner) {
    return blocked_deferred_template_type(
        work_item, "owner structured identity unavailable");
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
