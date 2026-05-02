#include "templates.hpp"

namespace c4c::hir {

namespace {

std::unordered_map<TextId, TypeSpec> build_template_type_bindings_by_text(
    const Node* template_owner,
    const TypeBindings& type_bindings) {
  std::unordered_map<TextId, TypeSpec> out;
  if (!template_owner || template_owner->n_template_params <= 0 ||
      !template_owner->template_param_names ||
      !template_owner->template_param_name_text_ids) {
    return out;
  }
  for (int i = 0; i < template_owner->n_template_params; ++i) {
    if (template_owner->template_param_is_nttp &&
        template_owner->template_param_is_nttp[i]) {
      continue;
    }
    const char* param_name = template_owner->template_param_names[i];
    if (!param_name) continue;
    const TextId text_id = template_owner->template_param_name_text_ids[i];
    if (text_id == kInvalidText) continue;
    auto it = type_bindings.find(param_name);
    if (it != type_bindings.end()) out[text_id] = it->second;
  }
  return out;
}

bool template_param_owner_matches(const TypeSpec& ts, const Node* template_owner) {
  if (!template_owner || ts.template_param_owner_text_id == kInvalidText ||
      template_owner->unqualified_text_id == kInvalidText) {
    return true;
  }
  if (ts.template_param_owner_text_id != template_owner->unqualified_text_id) {
    return false;
  }
  return ts.template_param_owner_namespace_context_id < 0 ||
         ts.template_param_owner_namespace_context_id ==
             template_owner->namespace_context_id;
}

const TypeSpec* find_template_typedef_binding(
    const TypeSpec& ts,
    const TypeBindings& type_bindings,
    const std::unordered_map<TextId, TypeSpec>& type_bindings_by_text,
    const Node* template_owner) {
  if (ts.base != TB_TYPEDEF || !template_param_owner_matches(ts, template_owner)) {
    return nullptr;
  }

  const TextId carrier_text_id =
      ts.template_param_text_id != kInvalidText ? ts.template_param_text_id
                                                : ts.tag_text_id;
  if (carrier_text_id != kInvalidText) {
    auto text_it = type_bindings_by_text.find(carrier_text_id);
    if (text_it != type_bindings_by_text.end()) return &text_it->second;
  }

  if (!template_owner || !template_owner->template_param_names ||
      ts.template_param_index < 0 ||
      ts.template_param_index >= template_owner->n_template_params) {
    return nullptr;
  }
  if (template_owner->template_param_is_nttp &&
      template_owner->template_param_is_nttp[ts.template_param_index]) {
    return nullptr;
  }
  const char* param_name =
      template_owner->template_param_names[ts.template_param_index];
  if (!param_name) return nullptr;
  auto it = type_bindings.find(param_name);
  return it == type_bindings.end() ? nullptr : &it->second;
}

void apply_template_typedef_concrete(TypeSpec& target, const TypeSpec& concrete) {
  const TypeSpec outer = target;
  target = concrete;
  target.ptr_level += outer.ptr_level;
  target.is_lvalue_ref = concrete.is_lvalue_ref || outer.is_lvalue_ref;
  target.is_rvalue_ref =
      !target.is_lvalue_ref && (concrete.is_rvalue_ref || outer.is_rvalue_ref);
  target.array_size = outer.array_size;
  target.array_rank = outer.array_rank;
  for (int i = 0; i < 8; ++i) target.array_dims[i] = outer.array_dims[i];
  target.is_ptr_to_array = outer.is_ptr_to_array;
  target.inner_rank = outer.inner_rank;
  target.array_size_expr = outer.array_size_expr;
  target.is_const = concrete.is_const || outer.is_const;
  target.is_volatile = concrete.is_volatile || outer.is_volatile;
  target.is_fn_ptr = outer.is_fn_ptr;
  target.is_packed = concrete.is_packed || outer.is_packed;
  target.is_noinline = concrete.is_noinline || outer.is_noinline;
  target.is_always_inline = concrete.is_always_inline || outer.is_always_inline;
}

}  // namespace

void Lowerer::apply_template_typedef_bindings(
    TypeSpec& ts,
    const TypeBindings& type_bindings,
    const std::unordered_map<TextId, TypeSpec>& type_bindings_by_text,
    const Node* template_owner) {
  if (const TypeSpec* bound = find_template_typedef_binding(
          ts, type_bindings, type_bindings_by_text, template_owner)) {
    apply_template_typedef_concrete(ts, *bound);
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
    const std::unordered_map<TextId, TypeSpec>& method_tpl_bindings_by_text,
    const NttpBindings& method_nttp_bindings) {
  auto base_tag_from_metadata =
      [&](const TypeSpec& base_ts) -> std::optional<std::string> {
    if (!module_) return std::nullopt;
    if (base_ts.record_def && base_ts.record_def->kind == NK_STRUCT_DEF) {
      if (const std::optional<HirRecordOwnerKey> owner_key =
              make_struct_def_node_owner_key(base_ts.record_def)) {
        if (const SymbolName* owner_tag =
                module_->find_struct_def_tag_by_owner(*owner_key)) {
          return *owner_tag;
        }
      }
    }
    if (const HirStructDef* base_layout =
            find_struct_def_for_layout_type(base_ts)) {
      if (!base_layout->tag.empty()) return base_layout->tag;
    }
    if (base_ts.record_def && base_ts.record_def->kind == NK_STRUCT_DEF &&
        base_ts.record_def->name && base_ts.record_def->name[0]) {
      return std::string(base_ts.record_def->name);
    }
    if (base_ts.tag_text_id != kInvalidText && module_->link_name_texts) {
      const std::string_view text =
          module_->link_name_texts->lookup(base_ts.tag_text_id);
      if (!text.empty()) return std::string(text);
    }
    return std::nullopt;
  };

  auto base_tag_text_id_from_metadata =
      [&](const TypeSpec& base_ts,
          const std::optional<std::string>& base_tag) -> TextId {
    if (!module_ || !base_tag || base_tag->empty()) return kInvalidText;
    if (base_ts.tag_text_id != kInvalidText && module_->link_name_texts) {
      const std::string_view rendered_text =
          module_->link_name_texts->lookup(base_ts.tag_text_id);
      if (rendered_text == *base_tag) return base_ts.tag_text_id;
    }
    if (const HirStructDef* base_layout =
            find_struct_def_for_layout_type(base_ts)) {
      if (base_layout->tag == *base_tag &&
          base_layout->tag_text_id != kInvalidText) {
        return base_layout->tag_text_id;
      }
    }
    return make_text_id(*base_tag, module_->link_name_texts.get());
  };

  for (int bi = 0; bi < tpl_def->n_bases; ++bi) {
    TypeSpec base_ts = tpl_def->base_types[bi];
    apply_template_typedef_bindings(
        base_ts, method_tpl_bindings, method_tpl_bindings_by_text, tpl_def);
    if (base_ts.tpl_struct_origin) {
      const Node* base_primary = find_template_struct_primary(base_ts.tpl_struct_origin);
      seed_and_resolve_pending_template_type_if_needed(
          base_ts, method_tpl_bindings, method_nttp_bindings, tpl_def,
          PendingTemplateTypeKind::BaseType, "instantiation-base",
          base_primary);
    }
    while (resolve_struct_member_typedef_if_ready(&base_ts)) {
    }
    std::optional<std::string> base_tag = base_tag_from_metadata(base_ts);
    if (base_ts.deferred_member_type_name && base_tag && !base_tag->empty()) {
      TypeSpec resolved_member{};
      if (resolve_struct_member_typedef_type(
              *base_tag, base_ts.deferred_member_type_name, &resolved_member)) {
        base_ts = resolved_member;
        while (resolve_struct_member_typedef_if_ready(&base_ts)) {
        }
        base_tag = base_tag_from_metadata(base_ts);
      }
    }
    if (base_tag && !base_tag->empty()) {
      def.base_tags.push_back(*base_tag);
      def.base_tag_text_ids.push_back(
          base_tag_text_id_from_metadata(base_ts, base_tag));
    }
  }
}

void Lowerer::register_instantiated_template_struct_methods(
    const std::string& mangled,
    const std::optional<HirRecordOwnerKey>& owner_key,
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
    if (owner_key) {
      register_struct_method_owner_lookup(
          *owner_key, method, method->is_const_method, mkey, mmangled, method->type);
    }
    NttpTextBindings method_nttp_bindings_by_text =
        build_call_nttp_text_bindings(nullptr, tpl_def, method_nttp_bindings);
    lower_struct_method(mmangled, mangled, method,
                        &method_tpl_bindings, &method_nttp_bindings,
                        method_nttp_bindings_by_text.empty()
                            ? nullptr
                            : &method_nttp_bindings_by_text);
  }
}

void Lowerer::record_instantiated_template_struct_field_metadata(
    const std::string& mangled,
    const std::optional<HirRecordOwnerKey>& owner_key,
    const Node* orig_f,
    const NttpBindings& selected_nttp_bindings_map,
    const NttpTextBindings* selected_nttp_bindings_by_text) {
  if (!orig_f || !orig_f->name) return;
  std::optional<long long> static_const_value;
  if (orig_f->is_static && orig_f->is_constexpr && orig_f->init) {
    static_const_value =
        eval_const_int_with_nttp_bindings(
            orig_f->init, selected_nttp_bindings_map,
            selected_nttp_bindings_by_text);
  }
  struct_static_member_decls_[mangled][orig_f->name] = orig_f;
  if (static_const_value) {
    struct_static_member_const_values_[mangled][orig_f->name] = *static_const_value;
  }
  if (owner_key) {
    register_struct_static_member_owner_lookup(
        *owner_key, orig_f, static_const_value);
  }
}

std::optional<HirStructField> Lowerer::instantiate_template_struct_field(
    const Node* orig_f,
    const std::string& owner_tag,
    const std::optional<HirRecordOwnerKey>& owner_key,
    const TypeBindings& selected_type_bindings,
    const std::unordered_map<TextId, TypeSpec>& selected_type_bindings_by_text,
    const NttpBindings& selected_nttp_bindings_map,
    const Node* tpl_def,
    int llvm_idx) {
  if (!orig_f || !orig_f->name || orig_f->is_static) return std::nullopt;

  TypeSpec ft = orig_f->type;
  apply_template_typedef_bindings(
      ft, selected_type_bindings, selected_type_bindings_by_text, tpl_def);
  materialize_template_array_extent(ft, selected_nttp_bindings_map);

  HirStructField hf;
  hf.name = orig_f->name;
  hf.field_text_id = make_text_id(
      hf.name, module_ ? module_->link_name_texts.get() : nullptr);
  hf.member_symbol_id =
      module_->member_symbols.intern(owner_tag + "::" + orig_f->name);
  if (owner_key) {
    register_struct_member_symbol_owner_lookup(
        *owner_key, hf.field_text_id, hf.member_symbol_id);
  }
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
    const std::optional<HirRecordOwnerKey>& owner_key,
    const Node* tpl_def,
    const TypeBindings& selected_type_bindings,
    const std::unordered_map<TextId, TypeSpec>& selected_type_bindings_by_text,
    const NttpBindings& selected_nttp_bindings_map,
    const NttpTextBindings* selected_nttp_bindings_by_text) {
  const int num_fields = tpl_def->n_fields > 0 ? tpl_def->n_fields : 0;
  int llvm_idx = 0;
  for (int fi = 0; fi < num_fields; ++fi) {
    const Node* orig_f = tpl_def->fields[fi];
    record_instantiated_template_struct_field_metadata(
        mangled, owner_key, orig_f, selected_nttp_bindings_map,
        selected_nttp_bindings_by_text);
    std::optional<HirStructField> hf = instantiate_template_struct_field(
        orig_f, mangled, owner_key, selected_type_bindings,
        selected_type_bindings_by_text, selected_nttp_bindings_map, tpl_def, llvm_idx);
    if (!hf) continue;
    def.fields.push_back(std::move(*hf));
    if (!tpl_def->is_union) ++llvm_idx;
  }
}

std::optional<HirRecordOwnerKey> Lowerer::make_template_struct_instance_owner_key(
    const HirStructDef& def,
    const Node* primary_tpl,
    const TemplateStructInstanceKey& instance_key) const {
  if (!module_ || !primary_tpl || instance_key.spec_key.empty()) return std::nullopt;
  TextTable* texts = module_->link_name_texts.get();
  const TextId primary_text_id = make_unqualified_text_id(primary_tpl, texts);
  HirRecordOwnerTemplateIdentity identity;
  identity.primary_declaration_text_id = primary_text_id;
  identity.specialization_key = instance_key.spec_key.canonical;
  HirRecordOwnerKey key =
      make_hir_template_record_owner_key(def.ns_qual, primary_text_id, std::move(identity));
  if (!hir_record_owner_key_has_complete_metadata(key)) return std::nullopt;
  return key;
}

std::optional<HirRecordOwnerKey> Lowerer::register_template_struct_instance_owner(
    const HirStructDef& def,
    const Node* primary_tpl,
    const Node* struct_node,
    const TemplateStructInstanceKey& instance_key,
    bool append_order) {
  const auto key = make_template_struct_instance_owner_key(def, primary_tpl, instance_key);
  if (!key) return std::nullopt;
  module_->index_struct_def_owner(*key, def.tag, append_order);
  struct_def_nodes_by_owner_[*key] = struct_node ? struct_node : primary_tpl;
  return key;
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
  std::unordered_map<TextId, TypeSpec> selected_type_bindings_by_text =
      build_template_type_bindings_by_text(tpl_def, selected_type_bindings);
  NttpTextBindings selected_nttp_bindings_by_text =
      build_call_nttp_text_bindings(nullptr, primary_tpl, selected_nttp_bindings_map);

  HirStructDef def;
  def.tag = mangled;
  def.tag_text_id = make_text_id(def.tag, module_ ? module_->link_name_texts.get() : nullptr);
  def.ns_qual = make_ns_qual(
      tpl_def, module_ ? module_->link_name_texts.get() : nullptr);
  def.is_union = tpl_def->is_union;
  def.pack_align = tpl_def->pack_align;
  def.struct_align = tpl_def->struct_align;
  TypeBindings method_tpl_bindings = selected_type_bindings;
  std::unordered_map<TextId, TypeSpec> method_tpl_bindings_by_text =
      selected_type_bindings_by_text;
  NttpBindings method_nttp_bindings = selected_nttp_bindings_map;
  append_instantiated_template_struct_bases(
      def, tpl_def, method_tpl_bindings, method_tpl_bindings_by_text,
      method_nttp_bindings);
  std::optional<HirRecordOwnerKey> owner_key =
      make_template_struct_instance_owner_key(def, primary_tpl, instance_key);
  append_instantiated_template_struct_fields(
      def, mangled, owner_key, tpl_def, selected_type_bindings,
      selected_type_bindings_by_text, selected_nttp_bindings_map,
      selected_nttp_bindings_by_text.empty()
          ? nullptr
          : &selected_nttp_bindings_by_text);

  compute_struct_layout(module_, def);
  owner_key =
      register_template_struct_instance_owner(def, primary_tpl, tpl_def, instance_key, true);
  module_->struct_def_order.push_back(mangled);
  module_->struct_defs[mangled] = std::move(def);
  struct_def_nodes_[mangled] = tpl_def;
  register_instantiated_template_struct_methods(
      mangled, owner_key, tpl_def, method_tpl_bindings, method_nttp_bindings);
}

}  // namespace c4c::hir
