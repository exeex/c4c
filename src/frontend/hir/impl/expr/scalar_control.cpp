#include "expr.hpp"
#include "consteval.hpp"

#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace c4c::hir {

namespace {

std::optional<HirTemplateParameterBindingKey>
make_nttp_binding_key_from_owner_text_id(const Node* owner,
                                         TextId parameter_text_id) {
  if (!owner || parameter_text_id == kInvalidText ||
      owner->n_template_params <= 0 || !owner->template_param_name_text_ids ||
      !owner->template_param_is_nttp) {
    return std::nullopt;
  }
  for (int i = 0; i < owner->n_template_params; ++i) {
    if (!owner->template_param_is_nttp[i]) continue;
    if (owner->template_param_name_text_ids[i] != parameter_text_id) continue;
    return make_hir_template_parameter_binding_key(
        owner, i, HirTemplateParameterBindingKind::NonType);
  }
  return std::nullopt;
}

template <typename T>
auto set_typespec_final_spelling_tag_if_present(T& ts, const char* tag, int)
    -> decltype(ts.tag = tag, void()) {
  ts.tag = tag;
}

template <typename T>
void set_typespec_final_spelling_tag_if_present(T&, const char*, long) {}

std::optional<HirRecordOwnerKey> structured_owner_key_from_qualified_ref(
    const Node* n,
    TextTable* texts) {
  if (!n || n->n_qualifier_segments <= 0) {
    return std::nullopt;
  }
  auto segment_text_id = [&](int index) -> TextId {
    if (n->qualifier_text_ids) {
      const TextId text_id = n->qualifier_text_ids[index];
      if (text_id != kInvalidText) return text_id;
    }
    if (texts && n->qualifier_segments && n->qualifier_segments[index] &&
        n->qualifier_segments[index][0]) {
      return texts->intern(n->qualifier_segments[index]);
    }
    return kInvalidText;
  };

  const TextId owner_text_id = segment_text_id(n->n_qualifier_segments - 1);
  if (owner_text_id == kInvalidText) return std::nullopt;

  NamespaceQualifier owner_ns;
  owner_ns.context_id = n->namespace_context_id;
  owner_ns.is_global_qualified = n->is_global_qualified;
  owner_ns.segment_text_ids.reserve(n->n_qualifier_segments - 1);
  for (int i = 0; i + 1 < n->n_qualifier_segments; ++i) {
    const TextId text_id = segment_text_id(i);
    if (text_id == kInvalidText) return std::nullopt;
    owner_ns.segment_text_ids.push_back(text_id);
  }

  HirRecordOwnerKey owner_key =
      make_hir_record_owner_key(owner_ns, owner_text_id);
  if (!hir_record_owner_key_has_complete_metadata(owner_key)) {
    return std::nullopt;
  }
  return owner_key;
}

std::optional<HirRecordOwnerKey> structured_owner_key_from_qualified_segments(
    const Node* n,
    TextTable* texts) {
  if (!n || !texts || n->n_qualifier_segments <= 0 || !n->qualifier_segments) {
    return std::nullopt;
  }
  auto segment_text_id = [&](int index) -> TextId {
    if (n->qualifier_segments[index] && n->qualifier_segments[index][0]) {
      return texts->intern(n->qualifier_segments[index]);
    }
    return kInvalidText;
  };

  const TextId owner_text_id = segment_text_id(n->n_qualifier_segments - 1);
  if (owner_text_id == kInvalidText) return std::nullopt;

  NamespaceQualifier owner_ns;
  owner_ns.context_id = n->namespace_context_id;
  owner_ns.is_global_qualified = n->is_global_qualified;
  owner_ns.segment_text_ids.reserve(n->n_qualifier_segments - 1);
  for (int i = 0; i + 1 < n->n_qualifier_segments; ++i) {
    const TextId text_id = segment_text_id(i);
    if (text_id == kInvalidText) return std::nullopt;
    owner_ns.segment_text_ids.push_back(text_id);
  }

  HirRecordOwnerKey owner_key =
      make_hir_record_owner_key(owner_ns, owner_text_id);
  if (!hir_record_owner_key_has_complete_metadata(owner_key)) {
    return std::nullopt;
  }
  return owner_key;
}

std::string structured_owner_name_from_qualified_ref(const Node* n,
                                                     TextTable* texts) {
  if (!n || n->n_qualifier_segments <= 0) return {};
  const int owner_index = n->n_qualifier_segments - 1;
  TextId owner_text_id = kInvalidText;
  if (n->qualifier_text_ids) owner_text_id = n->qualifier_text_ids[owner_index];
  if (owner_text_id != kInvalidText && texts) {
    const std::string_view owner_text = texts->lookup(owner_text_id);
    if (!owner_text.empty()) return std::string(owner_text);
  }
  if (n->qualifier_segments && n->qualifier_segments[owner_index] &&
      n->qualifier_segments[owner_index][0]) {
    return n->qualifier_segments[owner_index];
  }
  return {};
}

}  // namespace

const HirStructField* find_struct_instance_field_including_bases(
    const hir::Module& module,
    const std::string& tag,
    const std::string& field) {
  auto sit = module.struct_defs.find(tag);
  if (sit == module.struct_defs.end()) return nullptr;
  for (const auto& fld : sit->second.fields) {
    if (fld.name == field) return &fld;
  }
  for (const auto& base_tag : sit->second.base_tags) {
    if (const HirStructField* inherited =
            find_struct_instance_field_including_bases(module, base_tag, field)) {
      return inherited;
    }
  }
  return nullptr;
}

void Lowerer::attach_decl_ref_link_name_id(DeclRef& ref) const {
  if (!module_) return;
  if (ref.link_name_id != kInvalidLinkName) return;
  if (const GlobalVar* gv = module_->resolve_global_decl(ref)) {
    ref.link_name_id = gv->link_name_id;
    return;
  }
  if (const Function* fn = module_->resolve_function_decl(ref)) {
    ref.link_name_id = fn->link_name_id;
  }
}

ExprId Lowerer::lower_var_expr(FunctionCtx* ctx, const Node* n) {
  if (n->is_concept_id) {
    TypeSpec ts{};
    ts.base = TB_INT;
    ts.array_size = -1;
    ts.inner_rank = -1;
    return append_expr(n, IntLiteral{1, false}, ts);
  }
  if (n->name && n->name[0]) {
    if (n->has_template_args && find_template_struct_primary(n->name)) {
      TypeSpec tmp_ts{};
      tmp_ts.base = TB_STRUCT;
      tmp_ts.array_size = -1;
      tmp_ts.inner_rank = -1;
      tmp_ts.tpl_struct_origin = n->name;
      const Node* primary_tpl = find_template_struct_primary(n->name);
      assign_template_arg_refs_from_ast_args(
          &tmp_ts, n, primary_tpl, ctx, n, PendingTemplateTypeKind::OwnerStruct,
          "nameref-tpl-ctor-arg");
      TypeBindings tpl_empty;
      NttpBindings nttp_empty;
      seed_and_resolve_pending_template_type_if_needed(
          tmp_ts, ctx ? ctx->tpl_bindings : tpl_empty,
          ctx ? ctx->nttp_bindings : nttp_empty, n,
          PendingTemplateTypeKind::OwnerStruct, "nameref-tpl-ctor", primary_tpl);
      if (find_struct_def_for_layout_type(tmp_ts)) {
        const LocalId tmp_lid = next_local_id();
        const std::string tmp_name = "__tmp_struct_" + std::to_string(tmp_lid.value);
        LocalDecl tmp{};
        tmp.id = tmp_lid;
        tmp.name = tmp_name;
        tmp.type = qtype_from(tmp_ts, ValueCategory::LValue);
        tmp.storage = StorageClass::Auto;
        tmp.span = make_span(n);
        if (ctx) {
          ctx->locals[tmp.name] = tmp.id;
          ctx->local_types.insert(tmp.id, tmp_ts);
          append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(n)});
          DeclRef tmp_ref{};
          tmp_ref.name = tmp_name;
          tmp_ref.local = tmp_lid;
          return append_expr(n, tmp_ref, tmp_ts, ValueCategory::LValue);
        }
      }
    }
    if (const std::optional<HirRecordOwnerKey> owner_key =
            structured_owner_key_from_qualified_ref(
                n, module_ ? module_->link_name_texts.get() : nullptr)) {
      std::vector<HirRecordOwnerKey> owner_lookup_keys;
      owner_lookup_keys.push_back(*owner_key);
      if (const std::optional<HirRecordOwnerKey> segment_owner_key =
              structured_owner_key_from_qualified_segments(
                  n, module_ ? module_->link_name_texts.get() : nullptr)) {
        if (*segment_owner_key != *owner_key) {
          owner_lookup_keys.push_back(*segment_owner_key);
        }
      }
      const std::string* owner_tag =
          module_ ? module_->find_struct_def_tag_by_owner(*owner_key) : nullptr;
      if (!owner_tag && module_) {
        const std::string* unique_owner_tag = nullptr;
        for (const auto& [candidate_key, rendered_tag] :
             module_->struct_def_owner_index) {
          if (candidate_key.kind != owner_key->kind ||
              candidate_key.namespace_context_id !=
                  owner_key->namespace_context_id ||
              candidate_key.is_global_qualified != owner_key->is_global_qualified ||
              candidate_key.declaration_text_id !=
                  owner_key->declaration_text_id) {
            continue;
          }
          if (unique_owner_tag && unique_owner_tag != &rendered_tag) {
            unique_owner_tag = nullptr;
            break;
          }
          unique_owner_tag = &rendered_tag;
        }
        owner_tag = unique_owner_tag;
      }
      std::string owner_name = structured_owner_name_from_qualified_ref(
          n, module_ ? module_->link_name_texts.get() : nullptr);
      std::string owner_template_origin_name;
      auto recover_owner_template_origin = [&](const std::string& tag) {
        if (tag.empty() || !owner_template_origin_name.empty()) return;
        auto it = struct_def_nodes_.find(tag);
        if (it == struct_def_nodes_.end() || !it->second) return;
        const Node* owner_def = it->second;
        if (owner_def->template_origin_name &&
            owner_def->template_origin_name[0]) {
          owner_template_origin_name = owner_def->template_origin_name;
        }
      };
      if (owner_tag) recover_owner_template_origin(*owner_tag);
      recover_owner_template_origin(owner_name);
      if (owner_template_origin_name.empty() &&
          (n->has_template_args || n->n_template_args > 0) &&
          n->qualifier_segments && n->n_qualifier_segments > 0 &&
          n->qualifier_segments[n->n_qualifier_segments - 1] &&
          n->qualifier_segments[n->n_qualifier_segments - 1][0]) {
        const std::string segment_owner =
            n->qualifier_segments[n->n_qualifier_segments - 1];
        if (segment_owner != owner_name &&
            find_template_struct_primary(segment_owner)) {
          owner_template_origin_name = segment_owner;
        }
      }
      TextId member_text_id = n->unqualified_text_id;
      std::string member_name;
      if (member_text_id != kInvalidText && module_ && module_->link_name_texts) {
        const std::string_view text =
            module_->link_name_texts->lookup(member_text_id);
        if (!text.empty()) member_name = std::string(text);
      }
      TextId unqualified_member_text_id = kInvalidText;
      std::string unqualified_member_name;
      if (n->unqualified_name && n->unqualified_name[0]) {
        unqualified_member_name = n->unqualified_name;
        if (module_ && module_->link_name_texts) {
          unqualified_member_text_id =
              module_->link_name_texts->intern(unqualified_member_name);
        }
        if (member_name.empty()) {
          member_name = unqualified_member_name;
          member_text_id = unqualified_member_text_id;
        }
      }
      if (member_name.empty() && n->name && n->name[0]) {
        const std::string_view rendered_name(n->name);
        auto try_generated_member_payload = [&](const std::string& owner_prefix) {
          if (owner_prefix.empty() || !member_name.empty()) return;
          const std::string prefix = owner_prefix + "::";
          if (rendered_name.rfind(prefix, 0) == 0) {
            member_name = std::string(rendered_name.substr(prefix.size()));
            if (member_text_id == kInvalidText && module_ &&
                module_->link_name_texts) {
              member_text_id = module_->link_name_texts->intern(member_name);
            }
            return;
          }
          const std::string scoped_suffix = "::" + owner_prefix + "::";
          const size_t suffix_pos = rendered_name.rfind(scoped_suffix);
          if (suffix_pos != std::string_view::npos &&
              suffix_pos + scoped_suffix.size() < rendered_name.size()) {
            member_name =
                std::string(rendered_name.substr(suffix_pos + scoped_suffix.size()));
            if (member_text_id == kInvalidText && module_ &&
                module_->link_name_texts) {
              member_text_id = module_->link_name_texts->intern(member_name);
            }
          }
        };
        try_generated_member_payload(owner_name);
        if (owner_tag) try_generated_member_payload(*owner_tag);
        try_generated_member_payload(owner_template_origin_name);
      }
      std::string generated_member_name;
      TextId generated_member_text_id = kInvalidText;
      if (n->name && n->name[0]) {
        const std::string_view rendered_name(n->name);
        auto capture_generated_member_payload = [&](const std::string& owner_prefix) {
          if (owner_prefix.empty() || !generated_member_name.empty()) return;
          const std::string prefix = owner_prefix + "::";
          if (rendered_name.rfind(prefix, 0) == 0) {
            generated_member_name =
                std::string(rendered_name.substr(prefix.size()));
            if (module_ && module_->link_name_texts &&
                !generated_member_name.empty()) {
              generated_member_text_id =
                  module_->link_name_texts->intern(generated_member_name);
            }
            return;
          }
          const std::string scoped_suffix = "::" + owner_prefix + "::";
          const size_t suffix_pos = rendered_name.rfind(scoped_suffix);
          if (suffix_pos != std::string_view::npos &&
              suffix_pos + scoped_suffix.size() < rendered_name.size()) {
            generated_member_name =
                std::string(rendered_name.substr(suffix_pos + scoped_suffix.size()));
            if (module_ && module_->link_name_texts &&
                !generated_member_name.empty()) {
              generated_member_text_id =
                  module_->link_name_texts->intern(generated_member_name);
            }
          }
        };
        capture_generated_member_payload(owner_name);
        if (owner_tag) capture_generated_member_payload(*owner_tag);
        capture_generated_member_payload(owner_template_origin_name);
      }
      const bool has_template_args =
          n->has_template_args || n->n_template_args > 0;
      if (member_text_id != kInvalidText && !member_name.empty() &&
          (!has_template_args || !generated_member_name.empty() || owner_tag ||
           !owner_template_origin_name.empty())) {
        std::optional<std::string> realized_owner_tag;
        if (has_template_args) {
          const std::string& owner_identity =
              !owner_template_origin_name.empty()
                  ? owner_template_origin_name
                  : (owner_tag ? *owner_tag : owner_name);
          const Node* primary_tpl = nullptr;
          auto owner_primary_it = template_struct_defs_by_owner_.find(*owner_key);
          if (owner_primary_it != template_struct_defs_by_owner_.end()) {
            primary_tpl = owner_primary_it->second;
          }
          if (!primary_tpl) {
            const Node* unique_primary = nullptr;
            for (const auto& [candidate_key, candidate_primary] :
                 template_struct_defs_by_owner_) {
              if (candidate_key.kind != owner_key->kind ||
                  candidate_key.namespace_context_id !=
                      owner_key->namespace_context_id ||
                  candidate_key.is_global_qualified !=
                      owner_key->is_global_qualified ||
                  candidate_key.declaration_text_id !=
                      owner_key->declaration_text_id) {
                continue;
              }
              if (unique_primary && unique_primary != candidate_primary) {
                unique_primary = nullptr;
                break;
              }
              unique_primary = candidate_primary;
            }
            primary_tpl = unique_primary;
          }
          if (!primary_tpl && !owner_identity.empty()) {
            primary_tpl = find_template_struct_primary(owner_identity);
          }
          if (!primary_tpl && !owner_name.empty() &&
              owner_name != owner_identity) {
            primary_tpl = find_template_struct_primary(owner_name);
          }
          if (primary_tpl) {
            std::vector<TextId> owner_qualifiers =
                owner_key->qualifier_segment_text_ids;
            TypeSpec pending_ts{};
            pending_ts.base = TB_STRUCT;
            pending_ts.array_size = -1;
            pending_ts.inner_rank = -1;
            pending_ts.namespace_context_id = owner_key->namespace_context_id;
            pending_ts.is_global_qualified = owner_key->is_global_qualified;
            pending_ts.tag_text_id = owner_key->declaration_text_id;
            pending_ts.qualifier_text_ids = owner_qualifiers.data();
            pending_ts.n_qualifier_segments =
                static_cast<int>(owner_qualifiers.size());
            if (!owner_identity.empty()) {
              set_typespec_final_spelling_tag_if_present(
                  pending_ts, owner_identity.c_str(), 0);
              pending_ts.tpl_struct_origin = owner_identity.c_str();
            }
            assign_template_arg_refs_from_ast_args(
                &pending_ts, n, primary_tpl, ctx, n,
                PendingTemplateTypeKind::OwnerStruct,
                "nameref-static-owner-structured-arg");
            TypeBindings tpl_empty;
            NttpBindings nttp_empty;
            seed_and_resolve_pending_template_type_if_needed(
                pending_ts, ctx ? ctx->tpl_bindings : tpl_empty,
                ctx ? ctx->nttp_bindings : nttp_empty, n,
                PendingTemplateTypeKind::OwnerStruct,
                "nameref-static-owner-structured", primary_tpl);
            realized_owner_tag = resolve_member_lookup_owner_tag(
                pending_ts, false, ctx ? &ctx->tpl_bindings : nullptr,
                ctx ? &ctx->nttp_bindings : nullptr, nullptr, n,
                "nameref-static-owner-structured");
          }
        }
        struct MemberCandidate {
          TextId text_id = kInvalidText;
          std::string name;
        };
        std::vector<MemberCandidate> member_candidates;
        auto add_member_candidate = [&](TextId text_id, const std::string& name) {
          if (name.empty()) return;
          for (const MemberCandidate& existing : member_candidates) {
            if (existing.name == name ||
                (text_id != kInvalidText && existing.text_id == text_id)) {
              return;
            }
          }
          member_candidates.push_back(MemberCandidate{text_id, name});
        };
        add_member_candidate(member_text_id, member_name);
        add_member_candidate(unqualified_member_text_id,
                             unqualified_member_name);
        add_member_candidate(generated_member_text_id, generated_member_name);
        auto find_static_member_decl = [&]() -> const Node* {
          const std::string& lookup_owner =
              realized_owner_tag ? *realized_owner_tag
                                 : (owner_tag ? *owner_tag : owner_name);
          for (const MemberCandidate& candidate : member_candidates) {
            if (candidate.text_id != kInvalidText) {
              for (const HirRecordOwnerKey& lookup_key : owner_lookup_keys) {
                if (const std::optional<HirStructMemberLookupKey> key =
                        make_struct_member_lookup_key(lookup_key,
                                                      candidate.text_id)) {
                  if (const Node* decl =
                          find_struct_static_member_decl(*key, nullptr, nullptr)) {
                    return decl;
                  }
                }
              }
            }
            if (!lookup_owner.empty()) {
              if (const Node* decl = find_struct_static_member_decl(
                      lookup_owner, candidate.name)) {
                return decl;
              }
            }
          }
          return nullptr;
        };
        auto find_static_member_const_value = [&]() -> std::optional<long long> {
          const std::string& lookup_owner =
              realized_owner_tag ? *realized_owner_tag
                                 : (owner_tag ? *owner_tag : owner_name);
          for (const MemberCandidate& candidate : member_candidates) {
            if (candidate.text_id != kInvalidText) {
              for (const HirRecordOwnerKey& lookup_key : owner_lookup_keys) {
                if (const std::optional<HirStructMemberLookupKey> key =
                        make_struct_member_lookup_key(lookup_key,
                                                      candidate.text_id)) {
                  if (auto value = find_struct_static_member_const_value(
                          *key, nullptr, nullptr)) {
                    return value;
                  }
                }
              }
            }
            if (!lookup_owner.empty()) {
              if (auto value = find_struct_static_member_const_value(
                      lookup_owner, candidate.name)) {
                return value;
              }
            }
          }
          return std::nullopt;
        };
        const bool has_static_member_payload =
            !generated_member_name.empty() || owner_tag || realized_owner_tag ||
            !owner_template_origin_name.empty();
        if (has_template_args && has_static_member_payload) {
          std::string template_owner = owner_template_origin_name;
          if (template_owner.empty()) template_owner = owner_name;
          if (template_owner.empty() && realized_owner_tag) {
            template_owner = *realized_owner_tag;
          }
          if (template_owner.empty() && owner_tag) template_owner = *owner_tag;
          for (const MemberCandidate& candidate : member_candidates) {
            if (auto v = try_eval_template_static_member_const(
                    ctx, template_owner, n, candidate.name)) {
              TypeSpec ts{};
              if (const Node* decl = find_static_member_decl()) {
                ts = decl->type;
              }
              if (ts.base == TB_VOID) ts.base = TB_INT;
              return append_expr(n, IntLiteral{*v, false}, ts);
            }
          }
        }
        if (auto v = find_static_member_const_value()) {
          TypeSpec ts{};
          if (const Node* decl = find_static_member_decl()) {
            ts = decl->type;
          }
          if (ts.base == TB_VOID) ts.base = TB_INT;
          return append_expr(n, IntLiteral{*v, false}, ts);
        }
        if (const Node* decl = find_static_member_decl()) {
          if (decl->init) {
            LowererConstEvalStructuredMaps static_member_consteval_maps;
            refresh_global_consteval_structured_maps(static_member_consteval_maps);
            StaticEvalIntEnumLookupInput static_member_enum_lookup;
            static_member_enum_lookup.rendered_enum_consts = &enum_consts_;
            static_member_enum_lookup.enum_consts_by_key =
                &static_member_consteval_maps.enum_consts_by_key;
            long long v = static_eval_int(decl->init, static_member_enum_lookup);
            TypeSpec ts = decl->type;
            if (ts.base == TB_VOID) ts.base = TB_INT;
            return append_expr(n, IntLiteral{v, false}, ts);
          }
        }
      }
    }
    auto it = enum_consts_.find(n->name);
    if (it != enum_consts_.end()) {
      TypeSpec ts{};
      ts.base = TB_INT;
      return append_expr(n, IntLiteral{it->second, false}, ts);
    }
    if (ctx) {
      const std::optional<HirTemplateParameterBindingKey> query_key =
          make_nttp_binding_key_from_owner_text_id(
              ctx->template_binding_owner_node, n->unqualified_text_id);
      if (auto nttp_value =
              query_key
                  ? lookup_nttp_binding(
                        ctx, n, n->name, n->unqualified_text_id,
                        false /* allow_rendered_mirror_fallback */, &*query_key)
                  : lookup_nttp_binding(ctx, n, n->name)) {
        TypeSpec ts{};
        ts.base = TB_INT;
        ts.array_size = -1;
        ts.inner_rank = -1;
        return append_expr(n, IntLiteral{*nttp_value, false}, ts);
      }
    }
  }
  DeclRef r{};
  r.name = n->name ? n->name : "<anon_var>";
  r.name_text_id = make_unqualified_text_id(
      n, module_ ? module_->link_name_texts.get() : nullptr);
  r.ns_qual = make_ns_qual(n, module_ ? module_->link_name_texts.get() : nullptr);
  bool has_local_binding = false;
  if (ctx) {
    auto lit = ctx->locals.find(r.name);
    if (lit != ctx->locals.end()) {
      r.local = lit->second;
      has_local_binding = true;
    }
    auto sit = ctx->static_globals.find(r.name);
    if (sit != ctx->static_globals.end()) {
      r.global = sit->second;
      if (const GlobalVar* gv = module_->find_global(*r.global)) r.name = gv->name;
      has_local_binding = true;
    }
    if (!has_local_binding) {
      auto pit = ctx->params.find(r.name);
      if (pit != ctx->params.end()) r.param_index = pit->second;
    }
  }
  if (!has_local_binding) {
    if (auto instantiated = ensure_template_global_instance(ctx, n)) {
      r.global = *instantiated;
      if (const GlobalVar* gv = module_->find_global(*r.global)) r.name = gv->name;
    }
  }
  if (!has_local_binding) {
    if (const GlobalVar* gv = module_->resolve_global_decl(r)) {
      r.global = gv->id;
      r.name = gv->name;
    }
  }
  if (ctx && !ctx->method_struct_tag.empty() && !has_local_binding &&
      !r.param_index && !r.global) {
    if (auto v = find_struct_static_member_const_value(ctx->method_struct_tag, r.name)) {
      TypeSpec ts{};
      if (const Node* decl = find_struct_static_member_decl(ctx->method_struct_tag, r.name)) {
        ts = decl->type;
      }
      if (ts.base == TB_VOID) ts.base = TB_INT;
      return append_expr(n, IntLiteral{*v, false}, ts);
    }
    TypeSpec this_owner_ts{};
    populate_struct_owner_typespec(this_owner_ts, ctx->method_struct_tag, 0);
    const HirStructDef* owner_layout = nullptr;
    if (ctx->method_struct_owner_key) {
      owner_layout =
          module_->find_struct_def_by_owner_structured(*ctx->method_struct_owner_key);
    }
    if (!owner_layout) {
      owner_layout = find_struct_def_for_layout_type(this_owner_ts);
    }
    const HirStructField* implicit_field =
        owner_layout ? ::c4c::hir::find_struct_instance_field_including_bases(
                           *module_, owner_layout->tag, r.name)
                     : nullptr;
    if (owner_layout && implicit_field) {
      DeclRef this_ref{};
      this_ref.name = "this";
      auto pit = ctx->params.find("this");
      if (pit != ctx->params.end()) this_ref.param_index = pit->second;
      TypeSpec this_ts = this_owner_ts;
      set_typespec_final_spelling_tag_if_present(
          this_ts, owner_layout->tag.c_str(), 0);
      this_ts.tag_text_id = owner_layout->tag_text_id;
      this_ts.namespace_context_id = owner_layout->ns_qual.context_id;
      this_ts.is_global_qualified = owner_layout->ns_qual.is_global_qualified;
      this_ts.ptr_level = 1;
      ExprId this_id = append_expr(n, this_ref, this_ts, ValueCategory::LValue);
      MemberExpr me{};
      me.base = this_id;
      me.field = r.name;
      me.field_text_id = make_text_id(
          me.field, module_ ? module_->link_name_texts.get() : nullptr);
      const std::optional<std::string> owner_tag = resolve_member_lookup_owner_tag(
          this_ts, true, &ctx->tpl_bindings, &ctx->nttp_bindings,
          &ctx->method_struct_tag, n, std::string("implicit-this-member:") + r.name);
      me.resolved_owner_tag = owner_tag.value_or(owner_layout->tag);
      me.member_symbol_id =
          find_struct_member_symbol_id(
              this_owner_ts, me.resolved_owner_tag, r.name, me.field_text_id);
      me.is_arrow = true;
      return append_expr(n, me, n->type, ValueCategory::LValue);
    }
  }
  TypeSpec var_ts = n->type;
  std::optional<TypeSpec> storage_ts;
  if (ctx) storage_ts = storage_type_for_declref(ctx, r);
  if (storage_ts) {
    var_ts = is_any_ref_ts(*storage_ts) ? reference_value_ts(*storage_ts) : *storage_ts;
  }
  if (var_ts.base == TB_VOID && var_ts.ptr_level == 0 && var_ts.array_rank == 0 &&
      !r.local && !r.param_index && !r.global) {
    attach_decl_ref_link_name_id(r);
    if (const Function* fn = module_->resolve_function_decl(r)) {
      var_ts = fn->return_type.spec;
      var_ts.ptr_level++;
      var_ts.is_fn_ptr = true;
    }
  }
  attach_decl_ref_link_name_id(r);
  ExprId ref_id = append_expr(n, r, var_ts, ValueCategory::LValue);
  if (storage_ts && is_any_ref_ts(*storage_ts)) {
    UnaryExpr u{};
    u.op = UnaryOp::Deref;
    u.operand = ref_id;
    return append_expr(n, u, reference_value_ts(*storage_ts), ValueCategory::LValue);
  }
  return ref_id;
}

ExprId Lowerer::lower_unary_expr(FunctionCtx* ctx, const Node* n) {
  if (n->op) {
    const char* op_name = nullptr;
    if (std::string(n->op) == "++pre") op_name = "operator_preinc";
    else if (std::string(n->op) == "--pre") op_name = "operator_predec";
    else if (std::string(n->op) == "+") op_name = "operator_plus";
    else if (std::string(n->op) == "-") op_name = "operator_minus";
    else if (std::string(n->op) == "!") op_name = "operator_not";
    else if (std::string(n->op) == "~") op_name = "operator_bitnot";
    if (op_name) {
      ExprId op = try_lower_operator_call(ctx, n, n->left, op_name, {});
      if (op.valid()) return op;
    }
  }
  UnaryExpr u{};
  u.op = map_unary_op(n->op);
  u.operand = lower_expr(ctx, n->left);
  if (u.op == UnaryOp::Not) u.operand = maybe_bool_convert(ctx, u.operand, n->left);
  return append_expr(n, u, n->type);
}

ExprId Lowerer::lower_postfix_expr(FunctionCtx* ctx, const Node* n) {
  if (n->op) {
    const char* op_name = nullptr;
    std::string op_str(n->op);
    if (op_str == "++") op_name = "operator_postinc";
    else if (op_str == "--") op_name = "operator_postdec";
    if (op_name) {
      TypeSpec int_ts{};
      int_ts.base = TB_INT;
      ExprId dummy = append_expr(n, IntLiteral{0, false}, int_ts);
      ExprId op = try_lower_operator_call(ctx, n, n->left, op_name, {}, {dummy});
      if (op.valid()) return op;
    }
  }
  UnaryExpr u{};
  const std::string op = n->op ? n->op : "";
  u.op = (op == "--") ? UnaryOp::PostDec : UnaryOp::PostInc;
  u.operand = lower_expr(ctx, n->left);
  return append_expr(n, u, n->type);
}

ExprId Lowerer::lower_addr_expr(FunctionCtx* ctx, const Node* n) {
  ExprId op = try_lower_operator_call(ctx, n, n->left, "operator_addr", {});
  if (op.valid()) return op;
  if (n->left && n->left->kind == NK_VAR && n->left->name &&
      ct_state_->has_consteval_def(n->left->name)) {
    std::string diag = "error: cannot take address of consteval function '";
    diag += n->left->name;
    diag += "'";
    throw std::runtime_error(diag);
  }
  UnaryExpr u{};
  u.op = UnaryOp::AddrOf;
  u.operand = lower_expr(ctx, n->left);
  return append_expr(n, u, n->type);
}

ExprId Lowerer::lower_deref_expr(FunctionCtx* ctx, const Node* n) {
  ExprId op = try_lower_operator_call(ctx, n, n->left, "operator_deref", {});
  if (op.valid()) return op;
  UnaryExpr u{};
  u.op = UnaryOp::Deref;
  u.operand = lower_expr(ctx, n->left);
  return append_expr(n, u, n->type, ValueCategory::LValue);
}

ExprId Lowerer::lower_comma_expr(FunctionCtx* ctx, const Node* n) {
  BinaryExpr b{};
  b.op = BinaryOp::Comma;
  b.lhs = lower_expr(ctx, n->left);
  b.rhs = lower_expr(ctx, n->right);
  return append_expr(n, b, n->type);
}

ExprId Lowerer::lower_binary_expr(FunctionCtx* ctx, const Node* n) {
  if (n->op && n->right) {
    const char* op_name = nullptr;
    std::string op_str(n->op);
    if (op_str == "==") op_name = "operator_eq";
    else if (op_str == "!=") op_name = "operator_neq";
    else if (op_str == "+") op_name = "operator_plus";
    else if (op_str == "-") op_name = "operator_minus";
    else if (op_str == "*") op_name = "operator_mul";
    else if (op_str == "/") op_name = "operator_div";
    else if (op_str == "%") op_name = "operator_mod";
    else if (op_str == "&") op_name = "operator_bitand";
    else if (op_str == "|") op_name = "operator_bitor";
    else if (op_str == "^") op_name = "operator_bitxor";
    else if (op_str == "<<") op_name = "operator_shl";
    else if (op_str == ">>") op_name = "operator_shr";
    else if (op_str == "<") op_name = "operator_lt";
    else if (op_str == ">") op_name = "operator_gt";
    else if (op_str == "<=") op_name = "operator_le";
    else if (op_str == ">=") op_name = "operator_ge";
    if (op_name) {
      ExprId op = try_lower_operator_call(ctx, n, n->left, op_name, {n->right});
      if (op.valid()) return op;
    }
  }
  BinaryExpr b{};
  b.op = map_binary_op(n->op);
  b.lhs = lower_expr(ctx, n->left);
  b.rhs = lower_expr(ctx, n->right);
  if (b.op == BinaryOp::LAnd || b.op == BinaryOp::LOr) {
    b.lhs = maybe_bool_convert(ctx, b.lhs, n->left);
    b.rhs = maybe_bool_convert(ctx, b.rhs, n->right);
  }
  return append_expr(n, b, n->type);
}

ExprId Lowerer::lower_assign_expr(FunctionCtx* ctx, const Node* n) {
  if (ctx && n->op) {
    const char* op_name = nullptr;
    std::string op_str(n->op);
    if (op_str == "=") op_name = "operator_assign";
    else if (op_str == "+=") op_name = "operator_plus_assign";
    else if (op_str == "-=") op_name = "operator_minus_assign";
    else if (op_str == "*=") op_name = "operator_mul_assign";
    else if (op_str == "/=") op_name = "operator_div_assign";
    else if (op_str == "%=") op_name = "operator_mod_assign";
    else if (op_str == "&=") op_name = "operator_and_assign";
    else if (op_str == "|=") op_name = "operator_or_assign";
    else if (op_str == "^=") op_name = "operator_xor_assign";
    else if (op_str == "<<=") op_name = "operator_shl_assign";
    else if (op_str == ">>=") op_name = "operator_shr_assign";
    if (op_name) {
      ExprId op = try_lower_operator_call(ctx, n, n->left, op_name, {n->right});
      if (op.valid()) return op;
    }
  }
  AssignExpr a{};
  a.op = map_assign_op(n->op, n->kind);
  a.lhs = lower_expr(ctx, n->left);
  a.rhs = lower_expr(ctx, n->right);
  return append_expr(n, a, n->type, ValueCategory::LValue);
}

ExprId Lowerer::lower_compound_assign_expr(FunctionCtx* ctx, const Node* n) {
  if (ctx && n->op) {
    const char* op_name = nullptr;
    std::string op_str(n->op);
    if (op_str == "+=") op_name = "operator_plus_assign";
    else if (op_str == "-=") op_name = "operator_minus_assign";
    else if (op_str == "*=") op_name = "operator_mul_assign";
    else if (op_str == "/=") op_name = "operator_div_assign";
    else if (op_str == "%=") op_name = "operator_mod_assign";
    else if (op_str == "&=") op_name = "operator_and_assign";
    else if (op_str == "|=") op_name = "operator_or_assign";
    else if (op_str == "^=") op_name = "operator_xor_assign";
    else if (op_str == "<<=") op_name = "operator_shl_assign";
    else if (op_str == ">>=") op_name = "operator_shr_assign";
    if (op_name) {
      ExprId op = try_lower_operator_call(ctx, n, n->left, op_name, {n->right});
      if (op.valid()) return op;
    }
  }
  AssignExpr a{};
  a.op = map_assign_op(n->op, n->kind);
  a.lhs = lower_expr(ctx, n->left);
  a.rhs = lower_expr(ctx, n->right);
  return append_expr(n, a, n->type, ValueCategory::LValue);
}

ExprId Lowerer::lower_cast_expr(FunctionCtx* ctx, const Node* n) {
  CastExpr c{};
  TypeSpec cast_ts = substitute_signature_template_type(
      n->type, ctx ? &ctx->tpl_bindings : nullptr);
  if (ctx && !ctx->tpl_bindings.empty() && cast_ts.tpl_struct_origin) {
    seed_and_resolve_pending_template_type_if_needed(
        cast_ts, ctx->tpl_bindings, ctx->nttp_bindings, n,
        PendingTemplateTypeKind::CastTarget, "cast-target");
  }
  c.to_type = qtype_from(cast_ts);
  c.expr = lower_expr(ctx, n->left);
  if (cast_ts.is_fn_ptr) c.fn_ptr_sig = fn_ptr_sig_from_decl_node(n);
  return append_expr(n, c, cast_ts);
}

ExprId Lowerer::lower_va_arg_expr(FunctionCtx* ctx, const Node* n) {
  VaArgExpr v{};
  v.ap = lower_expr(ctx, n->left);
  return append_expr(n, v, n->type);
}

ExprId Lowerer::lower_index_expr(FunctionCtx* ctx, const Node* n) {
  if (n->right) {
    ExprId op = try_lower_operator_call(ctx, n, n->left, "operator_subscript", {n->right});
    if (op.valid()) return op;
  }
  IndexExpr idx{};
  idx.base = lower_expr(ctx, n->left);
  idx.index = lower_expr(ctx, n->right);
  return append_expr(n, idx, n->type, ValueCategory::LValue);
}

ExprId Lowerer::lower_ternary_expr(FunctionCtx* ctx, const Node* n) {
  if (const Node* cond = (n->cond ? n->cond : n->left)) {
    LowererConstEvalStructuredMaps structured_maps;
    ConstEvalEnv env = make_lowerer_consteval_env(
        structured_maps, ctx ? &ctx->local_const_bindings : nullptr, false);
    if (auto r = evaluate_constant_expr(cond, env); r.ok()) {
      return lower_expr(ctx, (r.as_int() != 0) ? n->then_ : n->else_);
    }
  }
  if (ctx && (contains_stmt_expr(n->then_) || contains_stmt_expr(n->else_))) {
    TypeSpec result_ts = n->type;
    if (result_ts.base == TB_VOID) result_ts.base = TB_INT;
    LocalDecl tmp{};
    tmp.id = next_local_id();
    tmp.name = "__tern_tmp";
    tmp.type = qtype_from(result_ts, ValueCategory::LValue);
    TypeSpec zero_ts{};
    zero_ts.base = TB_INT;
    tmp.init = append_expr(n, IntLiteral{0, false}, zero_ts);
    const LocalId tmp_lid = tmp.id;
    ctx->locals[tmp.name] = tmp.id;
    ctx->local_types.insert(tmp.id, result_ts);
    append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(n)});
    const Node* cond_n = n->cond ? n->cond : n->left;
    ExprId cond_expr = maybe_bool_convert(ctx, lower_expr(ctx, cond_n), cond_n);
    const BlockId then_b = create_block(*ctx);
    const BlockId else_b = create_block(*ctx);
    const BlockId after_b = create_block(*ctx);
    IfStmt ifs{};
    ifs.cond = cond_expr;
    ifs.then_block = then_b;
    ifs.else_block = else_b;
    ifs.after_block = after_b;
    append_stmt(*ctx, Stmt{StmtPayload{ifs}, make_span(n)});
    ensure_block(*ctx, ctx->current_block).has_explicit_terminator = true;
    auto emit_branch = [&](BlockId blk, const Node* branch) {
      ctx->current_block = blk;
      ExprId val = lower_expr(ctx, branch);
      DeclRef lhs_ref{};
      lhs_ref.name = "__tern_tmp";
      lhs_ref.local = tmp_lid;
      ExprId lhs_id = append_expr(n, lhs_ref, result_ts, ValueCategory::LValue);
      AssignExpr assign{};
      assign.op = AssignOp::Set;
      assign.lhs = lhs_id;
      assign.rhs = val;
      ExprId assign_id = append_expr(n, assign, result_ts);
      append_stmt(*ctx, Stmt{StmtPayload{ExprStmt{assign_id}}, make_span(n)});
      if (!ensure_block(*ctx, ctx->current_block).has_explicit_terminator) {
        GotoStmt j{};
        j.target.resolved_block = after_b;
        append_stmt(*ctx, Stmt{StmtPayload{j}, make_span(n)});
        ensure_block(*ctx, ctx->current_block).has_explicit_terminator = true;
      }
    };
    emit_branch(then_b, n->then_);
    emit_branch(else_b, n->else_);
    ctx->current_block = after_b;
    DeclRef ref{};
    ref.name = "__tern_tmp";
    ref.local = tmp_lid;
    return append_expr(n, ref, result_ts, ValueCategory::LValue);
  }
  TernaryExpr t{};
  const Node* cond_n = n->cond ? n->cond : n->left;
  t.cond = maybe_bool_convert(ctx, lower_expr(ctx, cond_n), cond_n);
  t.then_expr = lower_expr(ctx, n->then_);
  t.else_expr = lower_expr(ctx, n->else_);
  return append_expr(n, t, n->type);
}

ExprId Lowerer::lower_generic_expr(FunctionCtx* ctx, const Node* n) {
  TypeSpec ctrl_ts = infer_generic_ctrl_type(ctx, n->left);
  const Node* selected = nullptr;
  const Node* default_expr = nullptr;
  for (int i = 0; i < n->n_children; ++i) {
    const Node* assoc = n->children[i];
    if (!assoc) continue;
    const Node* expr = assoc->left;
    if (!expr) continue;
    if (assoc->ival == 1) {
      if (!default_expr) default_expr = expr;
      continue;
    }
    if (generic_type_compatible(ctrl_ts, assoc->type)) {
      selected = expr;
      break;
    }
  }
  if (!selected) selected = default_expr;
  if (selected) return lower_expr(ctx, selected);
  TypeSpec ts = n->type;
  if (ts.base == TB_VOID) ts.base = TB_INT;
  return append_expr(n, IntLiteral{0, false}, ts);
}

ExprId Lowerer::lower_stmt_expr(FunctionCtx* ctx, const Node* n) {
  TypeSpec ts = n->type;
  if (ts.base == TB_VOID) ts.base = TB_INT;
  if (ctx && n->body) return lower_stmt_expr_block(*ctx, n->body, ts);
  return append_expr(n, IntLiteral{0, false}, ts);
}

ExprId Lowerer::lower_complex_part_expr(FunctionCtx* ctx, const Node* n) {
  UnaryExpr u{};
  u.op = (n->kind == NK_REAL_PART) ? UnaryOp::RealPart : UnaryOp::ImagPart;
  u.operand = lower_expr(ctx, n->left);
  const ValueCategory category =
      (n->left && n->left->type.ptr_level == 0 && n->left->type.array_rank == 0)
          ? ValueCategory::LValue
          : ValueCategory::RValue;
  return append_expr(n, u, n->type, category);
}

ExprId Lowerer::lower_sizeof_expr(FunctionCtx* ctx, const Node* n) {
  SizeofExpr s{};
  s.expr = lower_expr(ctx, n->left);
  TypeSpec ts{};
  ts.base = TB_ULONG;
  return append_expr(n, s, ts);
}

ExprId Lowerer::lower_sizeof_pack_expr(FunctionCtx* ctx, const Node* n) {
  long long pack_size = 0;
  std::string pack_name = n->sval ? n->sval : "";
  if (pack_name.empty() && n->left && n->left->kind == NK_VAR && n->left->name) {
    pack_name = n->left->name;
  }
  if (ctx && !pack_name.empty()) {
    pack_size = count_pack_bindings_for_name(ctx->tpl_bindings, ctx->nttp_bindings, pack_name);
  }
  IntLiteral lit{};
  lit.value = pack_size;
  TypeSpec ts{};
  ts.base = TB_ULONG;
  return append_expr(n, lit, ts);
}

}  // namespace c4c::hir
