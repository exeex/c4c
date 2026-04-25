#include "templates.hpp"

namespace c4c::hir {

void Lowerer::apply_template_typedef_bindings(
    TypeSpec& ts,
    const TypeBindings& type_bindings) {
  for (const auto& [pname, pts] : type_bindings) {
    if (ts.base == TB_TYPEDEF && ts.tag && std::string(ts.tag) == pname) {
      ts.base = pts.base;
      ts.tag = pts.tag;
      ts.ptr_level += pts.ptr_level;
      ts.is_const |= pts.is_const;
      ts.is_volatile |= pts.is_volatile;
    }
  }
}

void Lowerer::materialize_template_array_extent(
    TypeSpec& ts,
    const NttpBindings& nttp_bindings) {
  if (!ts.array_size_expr) return;
  Node* ase = ts.array_size_expr;
  if (ase->kind != NK_VAR || !ase->name) return;
  for (const auto& [npname, nval] : nttp_bindings) {
    if (std::string(ase->name) == npname) {
      if (ts.array_rank > 0) {
        ts.array_dims[0] = nval;
        ts.array_size = nval;
      }
      ts.array_size_expr = nullptr;
      return;
    }
  }
}

void Lowerer::append_instantiated_template_struct_bases(
    HirStructDef& def,
    const Node* tpl_def,
    const TypeBindings& method_tpl_bindings,
    const NttpBindings& method_nttp_bindings) {
  for (int bi = 0; bi < tpl_def->n_bases; ++bi) {
    TypeSpec base_ts = tpl_def->base_types[bi];
    apply_template_typedef_bindings(base_ts, method_tpl_bindings);
    if (base_ts.tpl_struct_origin) {
      const Node* base_primary = find_template_struct_primary(base_ts.tpl_struct_origin);
      seed_and_resolve_pending_template_type_if_needed(
          base_ts, method_tpl_bindings, method_nttp_bindings, tpl_def,
          PendingTemplateTypeKind::BaseType, "instantiation-base",
          base_primary);
    }
    while (resolve_struct_member_typedef_if_ready(&base_ts)) {
    }
    if (base_ts.deferred_member_type_name && base_ts.tag && base_ts.tag[0]) {
      TypeSpec resolved_member{};
      if (resolve_struct_member_typedef_type(
              base_ts.tag, base_ts.deferred_member_type_name, &resolved_member)) {
        base_ts = resolved_member;
      }
    }
    if (base_ts.tag && base_ts.tag[0]) {
      def.base_tags.push_back(base_ts.tag);
      def.base_tag_text_ids.push_back(
          make_text_id(base_ts.tag, module_ ? module_->link_name_texts.get() : nullptr));
    }
  }
}

void Lowerer::register_instantiated_template_struct_methods(
    const std::string& mangled,
    const Node* tpl_def,
    const TypeBindings& method_tpl_bindings,
    const NttpBindings& method_nttp_bindings) {
  for (int mi = 0; mi < tpl_def->n_children; ++mi) {
    const Node* method = tpl_def->children[mi];
    if (!method || method->kind != NK_FUNCTION || !method->name) continue;
    const char* csuf = method->is_const_method ? "_const" : "";
    std::string mmangled = mangled + "__" + method->name + csuf;
    std::string mkey = mangled + "::" + method->name + csuf;
    if (struct_methods_.count(mkey)) continue;
    struct_methods_[mkey] = mmangled;
    struct_method_link_name_ids_[mkey] = module_->link_names.intern(mmangled);
    struct_method_ret_types_[mkey] = method->type;
    lower_struct_method(mmangled, mangled, method,
                        &method_tpl_bindings, &method_nttp_bindings);
  }
}

void Lowerer::record_instantiated_template_struct_field_metadata(
    const std::string& mangled,
    const Node* orig_f,
    const NttpBindings& selected_nttp_bindings_map) {
  if (!orig_f || !orig_f->name) return;
  struct_static_member_decls_[mangled][orig_f->name] = orig_f;
  if (orig_f->is_static && orig_f->is_constexpr && orig_f->init) {
    struct_static_member_const_values_[mangled][orig_f->name] =
        eval_const_int_with_nttp_bindings(orig_f->init, selected_nttp_bindings_map);
  }
}

std::optional<HirStructField> Lowerer::instantiate_template_struct_field(
    const Node* orig_f,
    const std::string& owner_tag,
    const TypeBindings& selected_type_bindings,
    const NttpBindings& selected_nttp_bindings_map,
    const Node* tpl_def,
    int llvm_idx) {
  if (!orig_f || !orig_f->name || orig_f->is_static) return std::nullopt;

  TypeSpec ft = orig_f->type;
  apply_template_typedef_bindings(ft, selected_type_bindings);
  materialize_template_array_extent(ft, selected_nttp_bindings_map);

  HirStructField hf;
  hf.name = orig_f->name;
  hf.field_text_id = make_text_id(
      hf.name, module_ ? module_->link_name_texts.get() : nullptr);
  hf.member_symbol_id =
      module_->member_symbols.intern(owner_tag + "::" + orig_f->name);
  if (ft.array_rank > 0) {
    hf.is_flexible_array = (ft.array_size < 0);
    hf.array_first_dim = (ft.array_size >= 0) ? ft.array_size : 0;
    for (int i = 0; i + 1 < ft.array_rank; ++i) ft.array_dims[i] = ft.array_dims[i + 1];
    if (ft.array_rank > 0) ft.array_dims[ft.array_rank - 1] = -1;
    --ft.array_rank;
    ft.array_size = (ft.array_rank > 0) ? ft.array_dims[0] : -1;
  }
  hf.elem_type = ft;
  hf.is_anon_member = orig_f->is_anon_field;
  hf.llvm_idx = tpl_def->is_union ? 0 : llvm_idx;
  return hf;
}

void Lowerer::append_instantiated_template_struct_fields(
    HirStructDef& def,
    const std::string& mangled,
    const Node* tpl_def,
    const TypeBindings& selected_type_bindings,
    const NttpBindings& selected_nttp_bindings_map) {
  const int num_fields = tpl_def->n_fields > 0 ? tpl_def->n_fields : 0;
  int llvm_idx = 0;
  for (int fi = 0; fi < num_fields; ++fi) {
    const Node* orig_f = tpl_def->fields[fi];
    record_instantiated_template_struct_field_metadata(
        mangled, orig_f, selected_nttp_bindings_map);
    std::optional<HirStructField> hf = instantiate_template_struct_field(
        orig_f, mangled, selected_type_bindings, selected_nttp_bindings_map,
        tpl_def, llvm_idx);
    if (!hf) continue;
    def.fields.push_back(std::move(*hf));
    if (!tpl_def->is_union) ++llvm_idx;
  }
}

void Lowerer::register_template_struct_instance_owner(
    const HirStructDef& def,
    const Node* primary_tpl,
    const Node* struct_node,
    const TemplateStructInstanceKey& instance_key,
    bool append_order) {
  if (!module_ || !primary_tpl || instance_key.spec_key.empty()) return;
  TextTable* texts = module_->link_name_texts.get();
  const TextId primary_text_id = make_unqualified_text_id(primary_tpl, texts);
  HirRecordOwnerTemplateIdentity identity;
  identity.primary_declaration_text_id = primary_text_id;
  identity.specialization_key = instance_key.spec_key.canonical;
  HirRecordOwnerKey key =
      make_hir_template_record_owner_key(def.ns_qual, primary_text_id, std::move(identity));
  if (!hir_record_owner_key_has_complete_metadata(key)) return;
  module_->index_struct_def_owner(key, def.tag, append_order);
  struct_def_nodes_by_owner_[key] = struct_node ? struct_node : primary_tpl;
}

void Lowerer::instantiate_template_struct_body(
    const std::string& mangled,
    const Node* primary_tpl,
    const Node* tpl_def,
    const SelectedTemplateStructPattern& selected_pattern,
    const std::vector<HirTemplateArg>& concrete_args,
    const TemplateStructInstanceKey& instance_key) {
  if (module_->struct_defs.count(mangled) ||
      instantiated_tpl_struct_keys_.count(instance_key)) {
    return;
  }
  instantiated_tpl_struct_keys_.insert(instance_key);

  const TypeBindings& selected_type_bindings = selected_pattern.type_bindings;
  const NttpBindings& selected_nttp_bindings_map = selected_pattern.nttp_bindings;

  HirStructDef def;
  def.tag = mangled;
  def.tag_text_id = make_text_id(def.tag, module_ ? module_->link_name_texts.get() : nullptr);
  def.ns_qual = make_ns_qual(
      tpl_def, module_ ? module_->link_name_texts.get() : nullptr);
  def.is_union = tpl_def->is_union;
  def.pack_align = tpl_def->pack_align;
  def.struct_align = tpl_def->struct_align;
  TypeBindings method_tpl_bindings = selected_type_bindings;
  NttpBindings method_nttp_bindings = selected_nttp_bindings_map;
  append_instantiated_template_struct_bases(
      def, tpl_def, method_tpl_bindings, method_nttp_bindings);
  append_instantiated_template_struct_fields(
      def, mangled, tpl_def, selected_type_bindings, selected_nttp_bindings_map);

  compute_struct_layout(module_, def);
  register_template_struct_instance_owner(def, primary_tpl, tpl_def, instance_key, true);
  module_->struct_def_order.push_back(mangled);
  module_->struct_defs[mangled] = std::move(def);
  struct_def_nodes_[mangled] = tpl_def;
  register_instantiated_template_struct_methods(
      mangled, tpl_def, method_tpl_bindings, method_nttp_bindings);
}

}  // namespace c4c::hir
