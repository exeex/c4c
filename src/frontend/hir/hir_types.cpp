// Unintegrated draft extracted from hir_lowering_core.cpp.
// This file is a staging artifact for the type/struct/layout/init-normalization
// cluster and is not yet wired into the build.
//
// Omitted for now: the template / program coordinator paths. Those stay in
// hir_lowering_core.cpp until the shared Lowerer declaration surface is hoisted into
// impl/lowerer.hpp.

#include "impl/hir_impl.hpp"
#include "impl/lowerer.hpp"
#include "consteval.hpp"
#include "type_utils.hpp"
#include "../parser/parser_support.hpp"

#include <algorithm>
#include <cstring>
#include <functional>
#include <set>
#include <sstream>
#include <stdexcept>

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

bool init_list_has_field_designator(const InitListItem& item) {
  return item.field_designator_text_id != kInvalidText || item.field_designator.has_value();
}

std::string_view init_list_field_designator_text(const InitListItem& item,
                                                 const TextTable* texts) {
  if (item.field_designator_text_id != kInvalidText && texts) {
    return texts->lookup(item.field_designator_text_id);
  }
  if (item.field_designator) return *item.field_designator;
  return {};
}

std::optional<ConstEvalStructuredNameKey> to_consteval_name_key(
    const CompileTimeValueBindingKey& key) {
  if (!key.valid()) return std::nullopt;
  ConstEvalStructuredNameKey out;
  out.namespace_context_id = key.namespace_context_id;
  out.is_global_qualified = key.is_global_qualified;
  out.qualifier_text_ids = key.qualifier_segment_text_ids;
  out.base_text_id = key.unqualified_text_id;
  return out;
}

std::optional<ConstEvalStructuredNameKey> consteval_name_key_from_node(
    const Node* n) {
  if (!n || n->namespace_context_id < 0 ||
      n->unqualified_text_id == kInvalidText) {
    return std::nullopt;
  }
  if (n->n_qualifier_segments < 0) return std::nullopt;
  if (n->n_qualifier_segments > 0 && !n->qualifier_text_ids) {
    return std::nullopt;
  }
  ConstEvalStructuredNameKey key;
  key.namespace_context_id = n->namespace_context_id;
  key.is_global_qualified = n->is_global_qualified;
  key.base_text_id = n->unqualified_text_id;
  key.qualifier_text_ids.reserve(
      static_cast<std::size_t>(n->n_qualifier_segments));
  for (int i = 0; i < n->n_qualifier_segments; ++i) {
    const TextId segment = n->qualifier_text_ids[i];
    if (segment == kInvalidText) return std::nullopt;
    key.qualifier_text_ids.push_back(segment);
  }
  return key.valid() ? std::optional<ConstEvalStructuredNameKey>(key)
                     : std::nullopt;
}

template <typename SourceMap>
void copy_consteval_structured_bindings(const SourceMap& source,
                                        ConstStructuredMap& out) {
  out.clear();
  out.reserve(source.size());
  for (const auto& [key, value] : source) {
    auto consteval_key = to_consteval_name_key(key);
    if (!consteval_key.has_value() || !consteval_key->valid()) continue;
    out[*consteval_key] = value;
  }
}

DeclRef make_function_lookup_decl_ref(
    Module& module, std::string name, const Node* source = nullptr,
    LinkNameId known_link_name_id = kInvalidLinkName) {
  DeclRef ref;
  ref.name = std::move(name);
  TextTable* texts = module.link_name_texts.get();
  ref.name_text_id = source ? make_unqualified_text_id(source, texts)
                            : make_text_id(ref.name, texts);
  ref.ns_qual = source ? make_ns_qual(source, texts) : NamespaceQualifier{};
  ref.link_name_id = known_link_name_id != kInvalidLinkName
                         ? known_link_name_id
                         : module.link_names.find(ref.name);
  return ref;
}

LinkNameId find_template_instantiation_link_name_id(
    const Module& module, std::string_view template_name,
    const SpecializationKey& spec_key,
    const SpecializationKey* legacy_spec_key) {
  const auto tdef_it = module.template_defs.find(std::string(template_name));
  if (tdef_it == module.template_defs.end()) return kInvalidLinkName;
  const auto& instances = tdef_it->second.instances;
  for (const auto& inst : instances) {
    if (inst.mangled_link_name_id == kInvalidLinkName) continue;
    if (inst.spec_key == spec_key ||
        (legacy_spec_key && inst.spec_key == *legacy_spec_key)) {
      return inst.mangled_link_name_id;
    }
  }
  return kInvalidLinkName;
}

DeclRef make_global_lookup_decl_ref(Module& module, std::string name,
                                    const Node* source = nullptr) {
  DeclRef ref;
  ref.name = std::move(name);
  TextTable* texts = module.link_name_texts.get();
  ref.name_text_id = source ? make_unqualified_text_id(source, texts)
                            : make_text_id(ref.name, texts);
  ref.ns_qual = source ? make_ns_qual(source, texts) : NamespaceQualifier{};
  ref.link_name_id = module.link_names.find(ref.name);
  return ref;
}

const HirStructDef* find_base_struct_def_for_layout(
    const Module& module,
    const HirStructDef& def,
    size_t base_index) {
  if (base_index >= def.base_tags.size()) return nullptr;
  const auto& base_tag = def.base_tags[base_index];
  if (base_tag.empty()) return nullptr;
  if (base_index < def.base_tag_text_ids.size()) {
    const HirRecordOwnerKey owner_key =
        make_hir_record_owner_key(def.ns_qual, def.base_tag_text_ids[base_index]);
    if (hir_record_owner_key_has_complete_metadata(owner_key)) {
      return module.find_struct_def_by_owner_structured(owner_key);
    }
  }
  // Compatibility bridge for legacy base metadata that still lacks a complete
  // structured owner key; `base_tags` remains the rendered final spelling.
  const auto it = module.struct_defs.find(base_tag);
  return it == module.struct_defs.end() ? nullptr : &it->second;
}

const HirStructField* find_struct_instance_field_including_bases(
    const Module& module,
    const HirStructDef& layout,
    const std::string& field) {
  for (const auto& fld : layout.fields) {
    if (fld.name == field) return &fld;
  }
  for (size_t base_index = 0; base_index < layout.base_tags.size(); ++base_index) {
    const HirStructDef* base_layout =
        find_base_struct_def_for_layout(module, layout, base_index);
    if (!base_layout) continue;
    if (const HirStructField* inherited =
            find_struct_instance_field_including_bases(module, *base_layout, field)) {
      return inherited;
    }
  }
  return nullptr;
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
  if (!n || n->n_qualifier_segments <= 0) return std::nullopt;
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

template <typename T>
auto typespec_legacy_tag_if_present(const T& ts, int)
    -> decltype(ts.tag, std::string_view{}) {
  return ts.tag ? std::string_view(ts.tag) : std::string_view{};
}

template <typename T>
std::string_view typespec_legacy_tag_if_present(const T&, long) {
  return {};
}

const HirStructDef* find_struct_def_by_layout_compatibility_tag(
    const Module& module,
    const TypeSpec& ts) {
  if (ts.tag_text_id != kInvalidText && module.link_name_texts) {
    const std::string_view rendered_tag =
        module.link_name_texts->lookup(ts.tag_text_id);
    if (!rendered_tag.empty()) {
      const auto it = module.struct_defs.find(std::string(rendered_tag));
      if (it != module.struct_defs.end()) return &it->second;
    }
  }
  const std::string_view legacy_tag = typespec_legacy_tag_if_present(ts, 0);
  if (legacy_tag.empty()) return nullptr;
  const auto it = module.struct_defs.find(std::string(legacy_tag));
  return it == module.struct_defs.end() ? nullptr : &it->second;
}

std::optional<std::string> rendered_typespec_tag_for_compatibility(
    const Module& module,
    const TypeSpec& ts) {
  if (ts.tag_text_id != kInvalidText && module.link_name_texts) {
    const std::string_view rendered_tag =
        module.link_name_texts->lookup(ts.tag_text_id);
    if (!rendered_tag.empty()) return std::string(rendered_tag);
  }
  const std::string_view legacy_tag = typespec_legacy_tag_if_present(ts, 0);
  if (!legacy_tag.empty()) return std::string(legacy_tag);
  return std::nullopt;
}

}  // namespace

bool Lowerer::is_lvalue_ref_ts(const TypeSpec& ts) {
  return ts.is_lvalue_ref;
}

std::shared_ptr<CompileTimeState> Lowerer::ct_state() const { return ct_state_; }

void Lowerer::refresh_global_consteval_structured_maps(
    LowererConstEvalStructuredMaps& maps) const {
  maps.enum_consts_by_key.clear();
  maps.named_consts_by_key.clear();
  if (!ct_state_) return;
  copy_consteval_structured_bindings(ct_state_->enum_consts_by_key(),
                                     maps.enum_consts_by_key);
  copy_consteval_structured_bindings(ct_state_->const_int_bindings_by_key(),
                                     maps.named_consts_by_key);
}

ConstEvalEnv Lowerer::make_lowerer_consteval_env(
    LowererConstEvalStructuredMaps& maps,
    const ConstMap* local_consts,
    const ConstTextMap* local_consts_by_text,
    const ConstStructuredMap* local_consts_by_key,
    bool include_named_consts) const {
  refresh_global_consteval_structured_maps(maps);
  ConstEvalEnv env{&enum_consts_,
                   include_named_consts ? &const_int_bindings_ : nullptr,
                   local_consts};
  env.enum_consts_by_key = &maps.enum_consts_by_key;
  env.enum_scopes_by_text = &enum_const_scopes_by_text_;
  env.enum_scopes_by_key = &enum_const_scopes_by_key_;
  env.local_consts_by_text = local_consts_by_text;
  env.local_consts_by_key = local_consts_by_key;
  if (include_named_consts) env.named_consts_by_key = &maps.named_consts_by_key;
  if (module_) {
    env.struct_defs = &module_->struct_defs;
    env.struct_def_owner_index = &module_->struct_def_owner_index;
    env.link_name_texts = module_->link_name_texts.get();
  }
  env.lookup_template_struct_primary =
      [](const TypeSpec& owner, const void* ctx) -> const Node* {
    if (!ctx || owner.tpl_struct_origin_key.base_text_id == kInvalidText) {
      return nullptr;
    }
    const auto* lowerer = static_cast<const Lowerer*>(ctx);
    if (owner.tpl_struct_origin_key.qualifier_path_id != kInvalidNamePath) {
      return nullptr;
    }
    NamespaceQualifier ns_qual;
    ns_qual.context_id = owner.tpl_struct_origin_key.context_id;
    ns_qual.is_global_qualified =
        owner.tpl_struct_origin_key.is_global_qualified;
    const HirRecordOwnerKey owner_key =
        make_hir_record_owner_key(ns_qual,
                                  owner.tpl_struct_origin_key.base_text_id);
    if (!hir_record_owner_key_has_complete_metadata(owner_key)) return nullptr;
    auto it = lowerer->template_struct_defs_by_owner_.find(owner_key);
    return it == lowerer->template_struct_defs_by_owner_.end() ? nullptr
                                                               : it->second;
  };
  env.template_struct_lookup_ctx = this;
  return env;
}

void Lowerer::resolve_typedef_to_struct(TypeSpec& ts) const {
  if (ts.base != TB_TYPEDEF || !module_) return;
  auto apply_struct_def = [&](const HirStructDef& def,
                              const std::optional<HirRecordOwnerKey>& owner_key) {
    ts.base = def.is_union ? TB_UNION : TB_STRUCT;
    set_typespec_final_spelling_tag_if_present(ts, def.tag.c_str(), 0);
    ts.tag_text_id = def.tag_text_id;
    ts.namespace_context_id = def.ns_qual.context_id;
    ts.is_global_qualified = def.ns_qual.is_global_qualified;
    if (owner_key) {
      const auto node_it = struct_def_nodes_by_owner_.find(*owner_key);
      if (node_it != struct_def_nodes_by_owner_.end()) {
        ts.record_def = const_cast<Node*>(node_it->second);
      }
    }
  };

  if (ts.record_def && ts.record_def->kind == NK_STRUCT_DEF) {
    if (const std::optional<HirRecordOwnerKey> owner_key =
            make_struct_def_node_owner_key(ts.record_def)) {
      if (const HirStructDef* structured =
              module_->find_struct_def_by_owner_structured(*owner_key)) {
        apply_struct_def(*structured, owner_key);
      }
      return;
    }
  }

  if (ts.namespace_context_id >= 0 && ts.tag_text_id != kInvalidText) {
    NamespaceQualifier ns_qual;
    ns_qual.context_id = ts.namespace_context_id;
    ns_qual.is_global_qualified = ts.is_global_qualified;
    if (ts.qualifier_text_ids && ts.n_qualifier_segments > 0) {
      ns_qual.segment_text_ids.assign(
          ts.qualifier_text_ids,
          ts.qualifier_text_ids + ts.n_qualifier_segments);
    }
    const HirRecordOwnerKey owner_key =
        make_hir_record_owner_key(ns_qual, ts.tag_text_id);
    if (hir_record_owner_key_has_complete_metadata(owner_key)) {
      if (const HirStructDef* structured =
              module_->find_struct_def_by_owner_structured(owner_key)) {
        apply_struct_def(*structured, owner_key);
      }
      return;
    }
  }

  // Compatibility bridge for legacy typedef TypeSpecs that still lack a
  // complete structured owner key but do carry parser TextId spelling.
  auto sit = module_->struct_defs.end();
  if (ts.tag_text_id != kInvalidText && module_->link_name_texts) {
    const std::string_view rendered_tag =
        module_->link_name_texts->lookup(ts.tag_text_id);
    if (!rendered_tag.empty()) {
      sit = module_->struct_defs.find(std::string(rendered_tag));
    }
  }
  if (sit == module_->struct_defs.end()) {
    const std::string_view legacy_tag = typespec_legacy_tag_if_present(ts, 0);
    if (legacy_tag.empty()) return;
    sit = module_->struct_defs.find(std::string(legacy_tag));
  }
  if (sit != module_->struct_defs.end()) {
    apply_struct_def(sit->second, std::nullopt);
  }
}

FunctionId Lowerer::next_fn_id() { return module_->alloc_function_id(); }

GlobalId Lowerer::next_global_id() { return module_->alloc_global_id(); }

LocalId Lowerer::next_local_id() { return module_->alloc_local_id(); }

BlockId Lowerer::next_block_id() { return module_->alloc_block_id(); }

ExprId Lowerer::next_expr_id() { return module_->alloc_expr_id(); }

bool Lowerer::contains_stmt_expr(const Node* n) {
  if (!n) return false;
  if (n->kind == NK_STMT_EXPR) return true;
  if (contains_stmt_expr(n->left)) return true;
  if (contains_stmt_expr(n->right)) return true;
  if (contains_stmt_expr(n->cond)) return true;
  if (contains_stmt_expr(n->then_)) return true;
  if (contains_stmt_expr(n->else_)) return true;
  if (contains_stmt_expr(n->body)) return true;
  if (contains_stmt_expr(n->init)) return true;
  if (contains_stmt_expr(n->update)) return true;
  for (int i = 0; i < n->n_children; ++i)
    if (contains_stmt_expr(n->children[i])) return true;
  return false;
}

QualType Lowerer::qtype_from(const TypeSpec& t, ValueCategory c) {
  QualType qt{};
  qt.spec = t;
  qt.category = c;
  return qt;
}

std::optional<FnPtrSig> Lowerer::fn_ptr_sig_from_decl_node(const Node* n) {
  if (!n) return std::nullopt;
  if (resolved_types_) {
    auto ct = resolved_types_->lookup(n);
    if (ct && sema::is_callable_type(*ct)) {
      const auto* fsig = sema::get_function_sig(*ct);
      if (fsig) {
        FnPtrSig sig{};
        sig.canonical_sig = ct;
        if (fsig->return_type) {
          TypeSpec ret_ts = sema::typespec_from_canonical(*fsig->return_type);
          if ((n->n_ret_fn_ptr_params > 0 || n->ret_fn_ptr_variadic) &&
              !ret_ts.is_fn_ptr) {
            ret_ts.is_fn_ptr = true;
            ret_ts.ptr_level = std::max(ret_ts.ptr_level, 1);
          }
          sig.return_type = qtype_from(ret_ts);
        }
        sig.variadic = fsig->is_variadic;
        sig.unspecified_params = fsig->unspecified_params;
        for (const auto& param : fsig->params) {
          sig.params.push_back(qtype_from(sema::typespec_from_canonical(param),
                                          ValueCategory::LValue));
        }
        if (sig.params.empty() && (n->n_fn_ptr_params > 0 || n->fn_ptr_variadic)) {
          sig.variadic = n->fn_ptr_variadic;
          sig.unspecified_params = false;
          for (int i = 0; i < n->n_fn_ptr_params; ++i) {
            const Node* param = n->fn_ptr_params[i];
            sig.params.push_back(qtype_from(param->type, ValueCategory::LValue));
          }
        }
        return sig;
      }
    }
  }
  return std::nullopt;
}

long long Lowerer::eval_const_int_with_nttp_bindings(
    const Node* n,
    const NttpBindings& nttp_bindings,
    const NttpTextBindings* nttp_bindings_by_text,
    const ConstStructuredMap* enum_consts_by_key) const {
  if (!n) return 0;
  if (n->kind == NK_INT_LIT || n->kind == NK_CHAR_LIT) return n->ival;
  if (n->kind == NK_VAR && n->name) {
    if (nttp_bindings_by_text && n->unqualified_text_id != kInvalidText) {
      auto text_it = nttp_bindings_by_text->find(n->unqualified_text_id);
      if (text_it != nttp_bindings_by_text->end()) return text_it->second;
    }
    auto nttp_it = nttp_bindings.find(n->name);
    if (nttp_it != nttp_bindings.end()) return nttp_it->second;
    if (enum_consts_by_key) {
      const std::optional<ConstEvalStructuredNameKey> key =
          consteval_name_key_from_node(n);
      if (key.has_value()) {
        auto enum_it = enum_consts_by_key->find(*key);
        if (enum_it != enum_consts_by_key->end()) return enum_it->second;
        return 0;
      }
    }
    // No structured/TextId carrier: retain rendered-name enum compatibility.
    auto enum_it = enum_consts_.find(n->name);
    if (enum_it != enum_consts_.end()) return enum_it->second;
    return 0;
  }
  if (n->kind == NK_CAST && n->left) {
    long long v =
        eval_const_int_with_nttp_bindings(
            n->left, nttp_bindings, nttp_bindings_by_text, enum_consts_by_key);
    TypeSpec ts = n->type;
    if (ts.ptr_level == 0) {
      int bits = 0;
      switch (ts.base) {
        case TB_BOOL: bits = 1; break;
        case TB_CHAR:
        case TB_UCHAR:
        case TB_SCHAR: bits = 8; break;
        case TB_SHORT:
        case TB_USHORT: bits = 16; break;
        case TB_INT:
        case TB_UINT:
        case TB_ENUM: bits = 32; break;
        default: break;
      }
      if (bits > 0 && bits < 64) {
        long long mask = (1LL << bits) - 1;
        v &= mask;
        if (!is_unsigned_base(ts.base) && ts.base != TB_BOOL && (v >> (bits - 1)))
          v |= ~mask;
      }
    }
    return v;
  }
  if (n->kind == NK_UNARY && n->op && n->left) {
    if (strcmp(n->op, "-") == 0)
      return -eval_const_int_with_nttp_bindings(
          n->left, nttp_bindings, nttp_bindings_by_text, enum_consts_by_key);
    if (strcmp(n->op, "+") == 0)
      return eval_const_int_with_nttp_bindings(
          n->left, nttp_bindings, nttp_bindings_by_text, enum_consts_by_key);
    if (strcmp(n->op, "~") == 0)
      return ~eval_const_int_with_nttp_bindings(
          n->left, nttp_bindings, nttp_bindings_by_text, enum_consts_by_key);
  }
  if (n->kind == NK_BINOP && n->op && n->left && n->right) {
    long long l =
        eval_const_int_with_nttp_bindings(
            n->left, nttp_bindings, nttp_bindings_by_text, enum_consts_by_key);
    long long r =
        eval_const_int_with_nttp_bindings(
            n->right, nttp_bindings, nttp_bindings_by_text, enum_consts_by_key);
    if (strcmp(n->op, "+") == 0) return l + r;
    if (strcmp(n->op, "-") == 0) return l - r;
    if (strcmp(n->op, "*") == 0) return l * r;
    if (strcmp(n->op, "/") == 0 && r != 0) return l / r;
    if (strcmp(n->op, "%") == 0 && r != 0) return l % r;
    if (strcmp(n->op, "&") == 0) return l & r;
    if (strcmp(n->op, "|") == 0) return l | r;
    if (strcmp(n->op, "^") == 0) return l ^ r;
    if (strcmp(n->op, "<<") == 0) return l << (r & 63);
    if (strcmp(n->op, ">>") == 0) return l >> (r & 63);
    if (strcmp(n->op, "<") == 0) return l < r;
    if (strcmp(n->op, ">") == 0) return l > r;
    if (strcmp(n->op, "<=") == 0) return l <= r;
    if (strcmp(n->op, ">=") == 0) return l >= r;
    if (strcmp(n->op, "==") == 0) return l == r;
    if (strcmp(n->op, "!=") == 0) return l != r;
    if (strcmp(n->op, "&&") == 0) return (l != 0) && (r != 0);
    if (strcmp(n->op, "||") == 0) return (l != 0) || (r != 0);
  }
  return 0;
}

std::string Lowerer::pack_binding_name(const std::string& base, int index) {
  return base + "#" + std::to_string(index);
}

bool Lowerer::parse_pack_binding_name(const std::string& key,
                                      const std::string& base,
                                      int* out_index) {
  if (key.size() <= base.size() + 1) return false;
  if (key.compare(0, base.size(), base) != 0) return false;
  if (key[base.size()] != '#') return false;
  int index = 0;
  try {
    index = std::stoi(key.substr(base.size() + 1));
  } catch (...) {
    return false;
  }
  if (index < 0) return false;
  if (out_index) *out_index = index;
  return true;
}

long long Lowerer::count_pack_bindings_for_name(const TypeBindings& bindings,
                                                const NttpBindings& nttp_bindings,
                                                const std::string& base) {
  int max_index = -1;
  for (const auto& [key, _] : bindings) {
    int pack_index = 0;
    if (parse_pack_binding_name(key, base, &pack_index))
      max_index = std::max(max_index, pack_index);
  }
  for (const auto& [key, _] : nttp_bindings) {
    int pack_index = 0;
    if (parse_pack_binding_name(key, base, &pack_index))
      max_index = std::max(max_index, pack_index);
  }
  return max_index + 1;
}

bool Lowerer::is_any_ref_ts(const TypeSpec& ts) {
  return ts.is_lvalue_ref || ts.is_rvalue_ref;
}

TypeSpec Lowerer::reference_storage_ts(TypeSpec ts) {
  if (ts.is_lvalue_ref || ts.is_rvalue_ref) ts.ptr_level += 1;
  return ts;
}

TypeSpec Lowerer::reference_value_ts(TypeSpec ts) {
  if (!ts.is_lvalue_ref && !ts.is_rvalue_ref) return ts;
  ts.is_lvalue_ref = false;
  ts.is_rvalue_ref = false;
  if (ts.ptr_level > 0) ts.ptr_level -= 1;
  return ts;
}

TypeSpec Lowerer::field_type_of(const HirStructField& f) {
  TypeSpec ts = f.elem_type;
  ts.inner_rank = -1;
  if (f.array_first_dim >= 0) {
    for (int i = std::min(ts.array_rank, 7); i > 0; --i) {
      ts.array_dims[i] = ts.array_dims[i - 1];
    }
    ts.array_rank = std::min(ts.array_rank + 1, 8);
    ts.array_size = f.array_first_dim;
    ts.array_dims[0] = f.array_first_dim;
  }
  return ts;
}

TypeSpec Lowerer::field_init_type_of(const HirStructField& f) {
  TypeSpec ts = field_type_of(f);
  if (f.is_flexible_array && ts.array_rank > 0) {
    ts.array_size = -1;
    ts.array_dims[0] = -1;
  }
  return ts;
}

bool Lowerer::is_char_like(TypeBase base) {
  return base == TB_CHAR || base == TB_SCHAR || base == TB_UCHAR;
}

bool Lowerer::is_scalar_init_type(const TypeSpec& ts) {
  return !is_vector_ty(ts) &&
         ts.array_rank == 0 &&
         !((ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.ptr_level == 0);
}

GlobalInit Lowerer::child_init_of(const InitListItem& item) {
  return std::visit(
      [&](const auto& v) -> GlobalInit {
        using V = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<V, InitScalar>) {
          return GlobalInit(v);
        } else {
          return GlobalInit(*v);
        }
      },
      item.value);
}

std::optional<InitListItem> Lowerer::make_init_item(const GlobalInit& init) {
  if (std::holds_alternative<std::monostate>(init)) return std::nullopt;
  InitListItem item{};
  if (const auto* scalar = std::get_if<InitScalar>(&init)) {
    item.value = *scalar;
  } else {
    item.value = std::make_shared<InitList>(std::get<InitList>(init));
  }
  return item;
}

std::optional<HirStructMethodLookupKey> Lowerer::make_struct_method_lookup_key(
    const std::string& tag,
    const std::string& method,
    bool is_const_method) const {
  if (!module_ || !module_->link_name_texts || tag.empty() || method.empty()) {
    return std::nullopt;
  }
  TextTable* texts = module_->link_name_texts.get();
  const TextId method_text_id = texts->find(method);
  if (method_text_id == kInvalidText) return std::nullopt;
  for (const auto& [owner_key, rendered_tag] : module_->struct_def_owner_index) {
    if (rendered_tag != tag) continue;
    HirStructMethodLookupKey key;
    key.owner_key = owner_key;
    key.method_text_id = method_text_id;
    key.is_const_method = is_const_method;
    if (hir_struct_method_lookup_key_has_complete_metadata(key)) return key;
  }
  return std::nullopt;
}

std::optional<HirStructMemberLookupKey> Lowerer::make_struct_member_lookup_key(
    const std::string& tag,
    const std::string& member) const {
  if (!module_ || !module_->link_name_texts || tag.empty() || member.empty()) {
    return std::nullopt;
  }
  TextTable* texts = module_->link_name_texts.get();
  const TextId member_text_id = texts->find(member);
  if (member_text_id == kInvalidText) return std::nullopt;
  for (const auto& [owner_key, rendered_tag] : module_->struct_def_owner_index) {
    if (rendered_tag != tag) continue;
    HirStructMemberLookupKey key;
    key.owner_key = owner_key;
    key.member_text_id = member_text_id;
    if (hir_struct_member_lookup_key_has_complete_metadata(key)) return key;
  }
  return std::nullopt;
}

std::optional<HirStructMemberLookupKey> Lowerer::make_struct_member_lookup_key(
    const HirRecordOwnerKey& owner_key,
    TextId member_text_id) const {
  HirStructMemberLookupKey key;
  key.owner_key = owner_key;
  key.member_text_id = member_text_id;
  if (!hir_struct_member_lookup_key_has_complete_metadata(key)) return std::nullopt;
  return key;
}

std::optional<HirStructMemberLookupKey> Lowerer::make_struct_member_lookup_key(
    const TypeSpec& owner_ts,
    TextId member_text_id) const {
  if (owner_ts.record_def && owner_ts.record_def->kind == NK_STRUCT_DEF) {
    const auto owner_key = make_struct_def_node_owner_key(owner_ts.record_def);
    if (!owner_key) return std::nullopt;
    return make_struct_member_lookup_key(*owner_key, member_text_id);
  }
  if (owner_ts.namespace_context_id >= 0 &&
      owner_ts.tag_text_id != kInvalidText) {
    NamespaceQualifier ns_qual;
    ns_qual.context_id = owner_ts.namespace_context_id;
    ns_qual.is_global_qualified = owner_ts.is_global_qualified;
    if (owner_ts.qualifier_text_ids && owner_ts.n_qualifier_segments > 0) {
      ns_qual.segment_text_ids.assign(
          owner_ts.qualifier_text_ids,
          owner_ts.qualifier_text_ids + owner_ts.n_qualifier_segments);
    }
    return make_struct_member_lookup_key(
        make_hir_record_owner_key(ns_qual, owner_ts.tag_text_id),
        member_text_id);
  }
  return std::nullopt;
}

std::string Lowerer::resolve_struct_method_lookup_owner_tag(
    const TypeSpec& owner_ts,
    bool is_arrow,
    const TypeBindings* tpl_bindings,
    const NttpBindings* nttp_bindings,
    const std::string* current_struct_tag,
    const Node* span_node,
    const std::string& context_name) {
  if (auto owner_tag = resolve_member_lookup_owner_tag(
          owner_ts, is_arrow, tpl_bindings, nttp_bindings, current_struct_tag,
          span_node, context_name)) {
    return *owner_tag;
  }
  if (module_ && module_->link_name_texts &&
      owner_ts.tag_text_id != kInvalidText) {
    const std::string_view rendered_tag =
        module_->link_name_texts->lookup(owner_ts.tag_text_id);
    if (!rendered_tag.empty()) return std::string(rendered_tag);
  }
  const std::string_view legacy_tag = typespec_legacy_tag_if_present(owner_ts, 0);
  return legacy_tag.empty() ? std::string{} : std::string(legacy_tag);
}

void Lowerer::record_struct_method_mangled_lookup_parity(
    const std::string& tag,
    const std::string& method,
    bool is_const_method,
    const std::string& rendered_mangled) const {
  const auto key = make_struct_method_lookup_key(tag, method, is_const_method);
  if (!key) return;
  ++struct_method_mangled_lookup_parity_checks_;
  const auto it = struct_methods_by_owner_.find(*key);
  if (it == struct_methods_by_owner_.end() || it->second != rendered_mangled) {
    ++struct_method_mangled_lookup_parity_mismatches_;
  }
}

void Lowerer::record_struct_method_link_name_lookup_parity(
    const std::string& tag,
    const std::string& method,
    bool is_const_method,
    LinkNameId rendered_link_name_id) const {
  const auto key = make_struct_method_lookup_key(tag, method, is_const_method);
  if (!key) return;
  ++struct_method_link_name_lookup_parity_checks_;
  const auto it = struct_method_link_name_ids_by_owner_.find(*key);
  if (it == struct_method_link_name_ids_by_owner_.end() ||
      it->second != rendered_link_name_id) {
    ++struct_method_link_name_lookup_parity_mismatches_;
  }
}

namespace {

bool same_type_spec_for_struct_method_lookup_parity(
    const TypeSpec& a,
    const TypeSpec& b) {
  if (a.base != b.base || a.enum_underlying_base != b.enum_underlying_base ||
      a.ptr_level != b.ptr_level || a.is_lvalue_ref != b.is_lvalue_ref ||
      a.is_rvalue_ref != b.is_rvalue_ref || a.align_bytes != b.align_bytes ||
      a.array_size != b.array_size || a.array_rank != b.array_rank ||
      a.is_ptr_to_array != b.is_ptr_to_array || a.inner_rank != b.inner_rank ||
      a.is_vector != b.is_vector || a.vector_lanes != b.vector_lanes ||
      a.vector_bytes != b.vector_bytes || a.array_size_expr != b.array_size_expr ||
      a.is_const != b.is_const || a.is_volatile != b.is_volatile ||
      a.is_fn_ptr != b.is_fn_ptr || a.is_packed != b.is_packed ||
      a.is_noinline != b.is_noinline || a.is_always_inline != b.is_always_inline ||
      a.tpl_struct_origin != b.tpl_struct_origin ||
      a.tpl_struct_args.data != b.tpl_struct_args.data ||
      a.tpl_struct_args.size != b.tpl_struct_args.size ||
      !(a.deferred_member_type_owner_key ==
        b.deferred_member_type_owner_key) ||
      a.deferred_member_type_text_id != b.deferred_member_type_text_id ||
      a.deferred_member_type_name != b.deferred_member_type_name ||
      a.n_qualifier_segments != b.n_qualifier_segments ||
      a.is_global_qualified != b.is_global_qualified) {
    return false;
  }
  auto same_record_owner_identity = [](const TypeSpec& lhs,
                                       const TypeSpec& rhs) -> bool {
    if (lhs.record_def || rhs.record_def) {
      return lhs.record_def && rhs.record_def && lhs.record_def == rhs.record_def;
    }
    const bool lhs_has_text_identity = lhs.tag_text_id != kInvalidText;
    const bool rhs_has_text_identity = rhs.tag_text_id != kInvalidText;
    if (lhs_has_text_identity || rhs_has_text_identity) {
      if (!lhs_has_text_identity || !rhs_has_text_identity ||
          lhs.tag_text_id != rhs.tag_text_id ||
          lhs.namespace_context_id != rhs.namespace_context_id ||
          lhs.is_global_qualified != rhs.is_global_qualified ||
          lhs.n_qualifier_segments != rhs.n_qualifier_segments) {
        return false;
      }
      for (int i = 0; i < lhs.n_qualifier_segments; ++i) {
        const TextId lhs_segment =
            lhs.qualifier_text_ids ? lhs.qualifier_text_ids[i] : kInvalidText;
        const TextId rhs_segment =
            rhs.qualifier_text_ids ? rhs.qualifier_text_ids[i] : kInvalidText;
        if (lhs_segment == kInvalidText || rhs_segment == kInvalidText ||
            lhs_segment != rhs_segment) {
          return false;
        }
      }
      return true;
    }
    return typespec_legacy_tag_if_present(lhs, 0) ==
           typespec_legacy_tag_if_present(rhs, 0);
  };
  if (!same_record_owner_identity(a, b)) return false;
  for (int i = 0; i < a.array_rank && i < 8; ++i) {
    if (a.array_dims[i] != b.array_dims[i]) return false;
  }
  for (int i = 0; i < a.n_qualifier_segments; ++i) {
    const std::string a_segment =
        a.qualifier_segments && a.qualifier_segments[i] ? a.qualifier_segments[i] : "";
    const std::string b_segment =
        b.qualifier_segments && b.qualifier_segments[i] ? b.qualifier_segments[i] : "";
    if (a_segment != b_segment) return false;
  }
  return true;
}

}  // namespace

void Lowerer::record_struct_method_return_type_lookup_parity(
    const std::string& tag,
    const std::string& method,
    bool is_const_method,
    const TypeSpec& rendered_return_type) const {
  const auto key = make_struct_method_lookup_key(tag, method, is_const_method);
  if (!key) return;
  ++struct_method_return_type_lookup_parity_checks_;
  const auto it = struct_method_ret_types_by_owner_.find(*key);
  if (it == struct_method_ret_types_by_owner_.end() ||
      !same_type_spec_for_struct_method_lookup_parity(it->second, rendered_return_type)) {
    ++struct_method_return_type_lookup_parity_mismatches_;
  }
}

void Lowerer::record_struct_static_member_decl_lookup_parity(
    const std::string& tag,
    const std::string& member,
    const Node* rendered_decl) const {
  const auto key = make_struct_member_lookup_key(tag, member);
  if (!key) return;
  ++struct_static_member_decl_lookup_parity_checks_;
  const auto it = struct_static_member_decls_by_owner_.find(*key);
  if (it == struct_static_member_decls_by_owner_.end() ||
      it->second != rendered_decl) {
    ++struct_static_member_decl_lookup_parity_mismatches_;
  }
}

void Lowerer::record_struct_static_member_const_value_lookup_parity(
    const std::string& tag,
    const std::string& member,
    long long rendered_value) const {
  const auto key = make_struct_member_lookup_key(tag, member);
  if (!key) return;
  ++struct_static_member_const_value_lookup_parity_checks_;
  const auto it = struct_static_member_const_values_by_owner_.find(*key);
  if (it == struct_static_member_const_values_by_owner_.end() ||
      it->second != rendered_value) {
    ++struct_static_member_const_value_lookup_parity_mismatches_;
  }
}

void Lowerer::record_struct_member_symbol_id_lookup_parity(
    const std::string& tag,
    const std::string& member,
    MemberSymbolId rendered_member_symbol_id) const {
  const auto key = make_struct_member_lookup_key(tag, member);
  if (!key) return;
  ++struct_member_symbol_id_lookup_parity_checks_;
  const auto it = struct_member_symbol_ids_by_owner_.find(*key);
  if (it == struct_member_symbol_ids_by_owner_.end() ||
      it->second != rendered_member_symbol_id) {
    ++struct_member_symbol_id_lookup_parity_mismatches_;
  }
}

std::optional<std::string> Lowerer::find_struct_method_mangled(
    const std::string& tag,
    const std::string& method,
    bool is_const_obj) const {
  const std::string base_key = tag + "::" + method;
  const std::string const_key = base_key + "_const";
  auto rendered_key_for = [&](bool is_const_method) -> const std::string& {
    return is_const_method ? const_key : base_key;
  };
  auto try_owner = [&](bool is_const_method) -> std::optional<std::string> {
    const auto owner_key =
        make_struct_method_lookup_key(tag, method, is_const_method);
    if (!owner_key) return std::nullopt;
    const auto owner_it = struct_methods_by_owner_.find(*owner_key);
    if (owner_it == struct_methods_by_owner_.end()) return std::nullopt;
    const auto rendered_it = struct_methods_.find(rendered_key_for(is_const_method));
    if (rendered_it != struct_methods_.end()) {
      record_struct_method_mangled_lookup_parity(
          tag, method, is_const_method, rendered_it->second);
    }
    return owner_it->second;
  };
  auto try_rendered = [&](bool is_const_method) -> std::optional<std::string> {
    auto it = struct_methods_.find(rendered_key_for(is_const_method));
    if (it != struct_methods_.end()) {
      record_struct_method_mangled_lookup_parity(
          tag, method, is_const_method, it->second);
      return it->second;
    }
    return std::nullopt;
  };
  const bool preferred_const = is_const_obj;
  const bool alternate_const = !is_const_obj;
  if (auto local = try_owner(preferred_const)) return local;
  if (auto local = try_owner(alternate_const)) return local;
  if (auto local = try_rendered(preferred_const)) return local;
  if (auto local = try_rendered(alternate_const)) return local;
  auto dit = module_->struct_defs.find(tag);
  if (dit != module_->struct_defs.end()) {
    for (const auto& base_tag : dit->second.base_tags) {
      if (auto inherited =
              find_struct_method_mangled(base_tag, method, is_const_obj)) {
        return inherited;
      }
    }
  }
  return std::nullopt;
}

std::optional<LinkNameId> Lowerer::find_struct_method_link_name_id(
    const std::string& tag,
    const std::string& method,
    bool is_const_obj) const {
  const std::string base_key = tag + "::" + method;
  const std::string const_key = base_key + "_const";
  auto rendered_key_for = [&](bool is_const_method) -> const std::string& {
    return is_const_method ? const_key : base_key;
  };
  auto try_owner = [&](bool is_const_method) -> std::optional<LinkNameId> {
    const auto owner_key =
        make_struct_method_lookup_key(tag, method, is_const_method);
    if (!owner_key) return std::nullopt;
    const auto owner_it = struct_method_link_name_ids_by_owner_.find(*owner_key);
    if (owner_it == struct_method_link_name_ids_by_owner_.end()) {
      return std::nullopt;
    }
    const auto rendered_it =
        struct_method_link_name_ids_.find(rendered_key_for(is_const_method));
    if (rendered_it != struct_method_link_name_ids_.end()) {
      record_struct_method_link_name_lookup_parity(
          tag, method, is_const_method, rendered_it->second);
    }
    return owner_it->second;
  };
  auto try_rendered = [&](bool is_const_method) -> std::optional<LinkNameId> {
    auto it = struct_method_link_name_ids_.find(rendered_key_for(is_const_method));
    if (it != struct_method_link_name_ids_.end()) {
      record_struct_method_link_name_lookup_parity(
          tag, method, is_const_method, it->second);
      return it->second;
    }
    return std::nullopt;
  };
  const bool preferred_const = is_const_obj;
  const bool alternate_const = !is_const_obj;
  if (auto local = try_owner(preferred_const)) return local;
  if (auto local = try_owner(alternate_const)) return local;
  if (auto local = try_rendered(preferred_const)) return local;
  if (auto local = try_rendered(alternate_const)) return local;
  auto dit = module_->struct_defs.find(tag);
  if (dit != module_->struct_defs.end()) {
    for (const auto& base_tag : dit->second.base_tags) {
      if (auto inherited =
              find_struct_method_link_name_id(base_tag, method, is_const_obj)) {
        return inherited;
      }
    }
  }
  return std::nullopt;
}

std::optional<TypeSpec> Lowerer::find_struct_method_return_type(
    const std::string& tag,
    const std::string& method,
    bool is_const_obj) const {
  const std::string base_key = tag + "::" + method;
  const std::string const_key = base_key + "_const";
  auto rendered_key_for = [&](bool is_const_method) -> const std::string& {
    return is_const_method ? const_key : base_key;
  };
  auto try_owner = [&](bool is_const_method) -> std::optional<TypeSpec> {
    const auto owner_key =
        make_struct_method_lookup_key(tag, method, is_const_method);
    if (!owner_key) return std::nullopt;
    const auto owner_it = struct_method_ret_types_by_owner_.find(*owner_key);
    if (owner_it == struct_method_ret_types_by_owner_.end()) return std::nullopt;
    const auto rendered_it =
        struct_method_ret_types_.find(rendered_key_for(is_const_method));
    if (rendered_it != struct_method_ret_types_.end()) {
      record_struct_method_return_type_lookup_parity(
          tag, method, is_const_method, rendered_it->second);
    }
    return owner_it->second;
  };
  auto try_rendered = [&](bool is_const_method) -> std::optional<TypeSpec> {
    auto it = struct_method_ret_types_.find(rendered_key_for(is_const_method));
    if (it != struct_method_ret_types_.end()) {
      record_struct_method_return_type_lookup_parity(
          tag, method, is_const_method, it->second);
      return it->second;
    }
    return std::nullopt;
  };
  const bool preferred_const = is_const_obj;
  const bool alternate_const = !is_const_obj;
  if (auto local = try_owner(preferred_const)) return local;
  if (auto local = try_owner(alternate_const)) return local;
  if (auto local = try_rendered(preferred_const)) return local;
  if (auto local = try_rendered(alternate_const)) return local;
  auto dit = module_->struct_defs.find(tag);
  if (dit != module_->struct_defs.end()) {
    for (const auto& base_tag : dit->second.base_tags) {
      if (auto inherited =
              find_struct_method_return_type(base_tag, method, is_const_obj)) {
        return inherited;
      }
    }
  }
  return std::nullopt;
}

void Lowerer::register_struct_method_owner_lookup(
    const HirRecordOwnerKey& owner_key,
    const Node* method,
    bool is_const_method,
    const std::string& rendered_key,
    const std::string& mangled,
    const TypeSpec& return_type) {
  if (!module_ || !method || !method->name || !method->name[0] ||
      rendered_key.empty() || mangled.empty()) {
    return;
  }
  HirStructMethodLookupKey key;
  key.owner_key = owner_key;
  key.method_text_id = make_unqualified_text_id(method, module_->link_name_texts.get());
  key.is_const_method = is_const_method;
  if (!hir_struct_method_lookup_key_has_complete_metadata(key)) return;

  struct_methods_by_owner_[key] = mangled;
  struct_method_link_name_ids_by_owner_[key] = module_->link_names.intern(mangled);
  struct_method_ret_types_by_owner_[key] = return_type;
}

void Lowerer::register_struct_static_member_owner_lookup(
    const HirRecordOwnerKey& owner_key,
    const Node* member,
    const std::optional<long long>& const_value) {
  if (!module_ || !member || !member->name || !member->name[0]) return;
  HirStructMemberLookupKey key;
  key.owner_key = owner_key;
  key.member_text_id = make_unqualified_text_id(member, module_->link_name_texts.get());
  if (!hir_struct_member_lookup_key_has_complete_metadata(key)) return;

  struct_static_member_decls_by_owner_[key] = member;
  if (const_value) {
    struct_static_member_const_values_by_owner_[key] = *const_value;
  }
}

void Lowerer::register_struct_member_symbol_owner_lookup(
    const HirRecordOwnerKey& owner_key,
    TextId member_text_id,
    MemberSymbolId member_symbol_id) {
  if (member_symbol_id == kInvalidMemberSymbol) return;
  HirStructMemberLookupKey key;
  key.owner_key = owner_key;
  key.member_text_id = member_text_id;
  if (!hir_struct_member_lookup_key_has_complete_metadata(key)) return;

  struct_member_symbol_ids_by_owner_[key] = member_symbol_id;
}

std::optional<TypeSpec> Lowerer::infer_call_result_type_from_callee(
    const FunctionCtx* ctx, const Node* callee) {
  if (!callee) return std::nullopt;
  if (callee->kind == NK_DEREF) {
    return infer_call_result_type_from_callee(ctx, callee->left);
  }
  if (callee->kind != NK_VAR || !callee->name || !callee->name[0]) {
    return std::nullopt;
  }
  const std::string name = callee->name;
  if (ctx) {
    if (callee->unqualified_text_id != kInvalidText) {
      const auto local_it =
          ctx->local_ids_by_text_id.find(callee->unqualified_text_id);
      if (local_it != ctx->local_ids_by_text_id.end()) {
        if (ctx->local_fn_ptr_sigs_by_id.contains(local_it->second)) {
          return ctx->local_fn_ptr_sigs_by_id.at(local_it->second)
              .return_type.spec;
        }
        return std::nullopt;
      }
      if (ctx->rendered_compat_local_text_ids.find(
              callee->unqualified_text_id) !=
              ctx->rendered_compat_local_text_ids.end() ||
          ctx->rendered_compat_local_names.find(name) !=
              ctx->rendered_compat_local_names.end()) {
        const auto lit = ctx->local_fn_ptr_sigs.find(name);
        if (lit != ctx->local_fn_ptr_sigs.end()) return lit->second.return_type.spec;
      }
    } else {
      const auto lit = ctx->local_fn_ptr_sigs.find(name);
      if (lit != ctx->local_fn_ptr_sigs.end()) return lit->second.return_type.spec;
    }
    if (callee->unqualified_text_id != kInvalidText) {
      const auto param_it =
          ctx->param_indices_by_text_id.find(callee->unqualified_text_id);
      if (param_it != ctx->param_indices_by_text_id.end()) {
        const auto sig_it = ctx->param_fn_ptr_sigs_by_index.find(param_it->second);
        if (sig_it != ctx->param_fn_ptr_sigs_by_index.end()) {
          return sig_it->second.return_type.spec;
        }
        return std::nullopt;
      } else if (ctx->rendered_compat_param_text_ids.find(
                     callee->unqualified_text_id) !=
                     ctx->rendered_compat_param_text_ids.end() ||
                 ctx->rendered_compat_param_names.find(name) !=
                     ctx->rendered_compat_param_names.end()) {
        const auto pit = ctx->param_fn_ptr_sigs.find(name);
        if (pit != ctx->param_fn_ptr_sigs.end()) return pit->second.return_type.spec;
      }
    } else {
      const auto pit = ctx->param_fn_ptr_sigs.find(name);
      if (pit != ctx->param_fn_ptr_sigs.end()) return pit->second.return_type.spec;
    }
    if (const std::optional<GlobalId> static_global =
            lookup_static_global_bridge(*ctx, callee, name)) {
      if (const GlobalVar* gv = module_->find_global(*static_global)) {
        if (gv->fn_ptr_sig) return gv->fn_ptr_sig->return_type.spec;
      }
    }
  }
  DeclRef global_ref = make_global_lookup_decl_ref(*module_, name, callee);
  if (const GlobalVar* gv = module_->resolve_global_decl(global_ref)) {
    if (gv->fn_ptr_sig) return gv->fn_ptr_sig->return_type.spec;
  }
  DeclRef fn_ref = make_function_lookup_decl_ref(*module_, name, callee);
  if (const Function* fn = module_->resolve_function_decl(fn_ref)) {
    return fn->return_type.spec;
  }
  return std::nullopt;
}

std::optional<GlobalId> Lowerer::lookup_static_global_bridge(
    const FunctionCtx& ctx, const Node* n, const std::string& name) const {
  if (n && n->unqualified_text_id != kInvalidText) {
    const auto structured_it =
        ctx.static_global_ids_by_text_id.find(n->unqualified_text_id);
    if (structured_it != ctx.static_global_ids_by_text_id.end()) {
      return structured_it->second;
    }
    if (ctx.rendered_compat_static_global_text_ids.find(
            n->unqualified_text_id) ==
            ctx.rendered_compat_static_global_text_ids.end() &&
        ctx.rendered_compat_static_global_names.find(name) ==
            ctx.rendered_compat_static_global_names.end()) {
      return std::nullopt;
    }
  }
  const auto rendered_it = ctx.static_globals.find(name);
  if (rendered_it != ctx.static_globals.end()) return rendered_it->second;
  return std::nullopt;
}

std::optional<TypeSpec> Lowerer::infer_call_result_type(
    const FunctionCtx* ctx, const Node* call) {
  if (!call || call->kind != NK_CALL || !call->left) return std::nullopt;

  if (auto dit = deduced_template_calls_.find(call);
      dit != deduced_template_calls_.end()) {
    LinkNameId link_name_id = kInvalidLinkName;
    if (call->left && call->left->kind == NK_VAR && call->left->name) {
      if (const Node* tpl_fn = ct_state_->find_template_def(call->left->name)) {
        const auto param_order =
            get_template_param_order(tpl_fn, &dit->second.bindings,
                                     &dit->second.nttp_bindings);
        const SpecializationKey spec_key =
            dit->second.nttp_bindings.empty()
                ? make_specialization_key(call->left->name, param_order,
                                          dit->second.bindings, tpl_fn)
                : make_specialization_key(call->left->name, param_order,
                                          dit->second.bindings,
                                          dit->second.nttp_bindings, tpl_fn);
        link_name_id = find_template_instantiation_link_name_id(
            *module_, call->left->name, spec_key, nullptr);
      }
    }
    DeclRef fn_ref = make_function_lookup_decl_ref(
        *module_, dit->second.mangled_name, nullptr, link_name_id);
    if (const Function* fn = module_->resolve_function_decl(fn_ref)) {
      return fn->return_type.spec;
    }
  }

  if (call->left->kind == NK_VAR && call->left->name &&
      (call->left->n_template_args > 0 || call->left->has_template_args) &&
      ct_state_->has_template_def(call->left->name) &&
      !ct_state_->has_consteval_def(call->left->name)) {
    const TypeBindings* enc =
        (ctx && !ctx->tpl_bindings.empty()) ? &ctx->tpl_bindings : nullptr;
    const NttpBindings* enc_nttp =
        (ctx && !ctx->nttp_bindings.empty()) ? &ctx->nttp_bindings : nullptr;
    const NttpTextBindings* enc_nttp_by_text =
        (ctx && !ctx->nttp_bindings_by_text.empty())
            ? &ctx->nttp_bindings_by_text
            : nullptr;
    const Node* tpl_fn = ct_state_->find_template_def(call->left->name);
    if (tpl_fn) {
      HirTemplateTypeBindings structured_bindings;
      TypeBindings bindings =
          ctx ? merge_explicit_and_ctx_deduced_type_bindings(
                    call, call->left, tpl_fn, const_cast<FunctionCtx*>(ctx),
                    &structured_bindings)
              : merge_explicit_and_deduced_type_bindings(
                    call, call->left, tpl_fn, enc, nullptr,
                    &structured_bindings);
      HirTemplateNttpBindings structured_nttp_bindings;
      NttpBindings nttp_bindings = build_call_nttp_bindings(
          call->left, tpl_fn, enc_nttp, enc_nttp_by_text,
          &structured_nttp_bindings);
      std::string resolved_name =
          mangle_template_name(call->left->name, bindings, nttp_bindings);
      const auto param_order =
          get_template_param_order(tpl_fn, &bindings, &nttp_bindings);
      const std::optional<SpecializationKey> structured_spec_key =
          try_make_structured_specialization_key(
              call->left->name, param_order, bindings, structured_bindings,
              nttp_bindings, structured_nttp_bindings, tpl_fn);
      const SpecializationKey legacy_spec_key =
          nttp_bindings.empty()
              ? make_specialization_key(call->left->name, param_order,
                                        bindings, tpl_fn)
              : make_specialization_key(call->left->name, param_order,
                                        bindings, nttp_bindings, tpl_fn);
      const SpecializationKey spec_key =
          structured_spec_key ? *structured_spec_key : legacy_spec_key;
      LinkNameId resolved_link_name_id = kInvalidLinkName;
      if (const auto* inst_list = registry_.find_instances(call->left->name)) {
        for (const auto& inst : *inst_list) {
          if (inst.spec_key == spec_key ||
              (structured_spec_key && inst.spec_key == legacy_spec_key)) {
            resolved_name = inst.mangled_name;
            break;
          }
        }
      }
      resolved_link_name_id = find_template_instantiation_link_name_id(
          *module_, call->left->name, spec_key,
          structured_spec_key ? &legacy_spec_key : nullptr);
      DeclRef fn_ref = make_function_lookup_decl_ref(
          *module_, resolved_name, nullptr, resolved_link_name_id);
      if (const Function* fn = module_->resolve_function_decl(fn_ref)) {
        return fn->return_type.spec;
      }
    }
  }

  return infer_call_result_type_from_callee(ctx, call->left);
}

std::optional<long long> Lowerer::lookup_nttp_binding(
    const FunctionCtx* ctx,
    const Node* name_node,
    const char* rendered_name,
    TextId query_text_id,
    bool allow_rendered_mirror_fallback,
    const HirTemplateParameterBindingKey* query_key) const {
  if (!ctx) return std::nullopt;
  const TextId text_id = query_text_id != kInvalidText
                             ? query_text_id
                             : (name_node ? name_node->unqualified_text_id
                                          : kInvalidText);
  std::optional<long long> text_mirror_value;
  if (text_id != kInvalidText && !ctx->nttp_bindings_by_text.empty()) {
    auto text_it = ctx->nttp_bindings_by_text.find(text_id);
    if (text_it != ctx->nttp_bindings_by_text.end()) {
      text_mirror_value = text_it->second;
    }
  }
  std::optional<long long> rendered_mirror_value;
  if (allow_rendered_mirror_fallback && rendered_name && rendered_name[0]) {
    auto rendered_it = ctx->nttp_bindings.find(rendered_name);
    if (rendered_it != ctx->nttp_bindings.end()) {
      rendered_mirror_value = rendered_it->second;
    }
  }

  bool has_complete_structured_nttp_bindings = false;
  bool raw_text_matches_complete_structured_domain = false;
  const bool has_complete_query_key =
      query_key && query_key->parameter_kind ==
                       HirTemplateParameterBindingKind::NonType &&
      hir_template_parameter_binding_key_has_complete_metadata(*query_key);
  if (has_complete_query_key) {
    auto structured_it = ctx->structured_nttp_bindings.find(*query_key);
    if (structured_it != ctx->structured_nttp_bindings.end()) {
      return structured_it->second;
    }
  }
  for (const auto& [key, value] : ctx->structured_nttp_bindings) {
    (void)value;
    if (key.parameter_kind != HirTemplateParameterBindingKind::NonType ||
        !hir_template_parameter_binding_key_has_complete_metadata(key)) {
      continue;
    }
    has_complete_structured_nttp_bindings = true;
    if (!has_complete_query_key && text_id != kInvalidText &&
        key.parameter_text_id == text_id) {
      raw_text_matches_complete_structured_domain = true;
    }
  }
  if (has_complete_query_key && has_complete_structured_nttp_bindings) {
    return std::nullopt;
  }
  if (!has_complete_query_key && has_complete_structured_nttp_bindings &&
      text_id != kInvalidText && !raw_text_matches_complete_structured_domain &&
      text_mirror_value) {
    return std::nullopt;
  }
  if (text_mirror_value) return *text_mirror_value;
  if (rendered_mirror_value) return *rendered_mirror_value;
  return std::nullopt;
}

std::optional<TypeSpec> Lowerer::storage_type_for_declref(
    FunctionCtx* ctx, const DeclRef& r) {
  if (r.local && ctx) {
    if (ctx->local_types.contains(*r.local)) return ctx->local_types.at(*r.local);
  }
  if (r.param_index && ctx && ctx->fn && *r.param_index < ctx->fn->params.size()) {
    return ctx->fn->params[*r.param_index].type.spec;
  }
  if (r.global) {
    if (const GlobalVar* gv = module_->find_global(*r.global)) return gv->type.spec;
  }
  return std::nullopt;
}

bool Lowerer::is_string_scalar(const GlobalInit& init) const {
  const auto* scalar = std::get_if<InitScalar>(&init);
  if (!scalar) return false;
  const Expr& e = module_->expr_pool[scalar->expr.value];
  return std::holds_alternative<StringLiteral>(e.payload);
}

const HirStructDef* Lowerer::find_struct_def_for_layout_type(const TypeSpec& ts) const {
  if (!module_ ||
      (ts.base != TB_STRUCT && ts.base != TB_UNION) ||
      ts.ptr_level != 0) {
    return nullptr;
  }
  if (ts.record_def && ts.record_def->kind == NK_STRUCT_DEF) {
    if (const std::optional<HirRecordOwnerKey> owner_key =
            make_struct_def_node_owner_key(ts.record_def)) {
      if (hir_record_owner_key_has_complete_metadata(*owner_key)) {
        if (const HirStructDef* structured =
                module_->find_struct_def_by_owner_structured(*owner_key)) {
          return structured;
        }
        return nullptr;
      }
      if (const HirStructDef* structured =
              module_->find_struct_def_by_owner_structured(*owner_key)) {
        return structured;
      }
    }
  }
  if (ts.namespace_context_id >= 0 && ts.tag_text_id != kInvalidText) {
    NamespaceQualifier ns_qual;
    ns_qual.context_id = ts.namespace_context_id;
    ns_qual.is_global_qualified = ts.is_global_qualified;
    if (ts.qualifier_text_ids && ts.n_qualifier_segments > 0) {
      ns_qual.segment_text_ids.assign(
          ts.qualifier_text_ids,
          ts.qualifier_text_ids + ts.n_qualifier_segments);
    }
    const HirRecordOwnerKey owner_key =
        make_hir_record_owner_key(ns_qual, ts.tag_text_id);
    if (hir_record_owner_key_has_complete_metadata(owner_key)) {
      if (const HirStructDef* structured =
              module_->find_struct_def_by_owner_structured(owner_key)) {
        return structured;
      }
      return nullptr;
    }
  }
  // Compatibility bridge for legacy TypeSpec producers that still lack
  // complete structured owner metadata.
  return find_struct_def_by_layout_compatibility_tag(*module_, ts);
}

const HirStructField* Lowerer::find_struct_instance_field_including_bases(
    const TypeSpec& owner_ts,
    const std::string& field) const {
  const HirStructDef* layout = find_struct_def_for_layout_type(owner_ts);
  if (!layout) return nullptr;
  return ::c4c::hir::find_struct_instance_field_including_bases(
      *module_, *layout, field);
}

long long Lowerer::flat_scalar_count(const TypeSpec& ts) const {
  if (is_vector_ty(ts)) return ts.vector_lanes > 0 ? ts.vector_lanes : 1;
  if (ts.array_rank > 0) {
    if (ts.array_size <= 0) return 1;
    TypeSpec elem_ts = ts;
    elem_ts.array_rank--;
    elem_ts.array_size = (elem_ts.array_rank > 0) ? elem_ts.array_dims[0] : -1;
    return ts.array_size * flat_scalar_count(elem_ts);
  }
  if ((ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.ptr_level == 0) {
    const HirStructDef* layout = find_struct_def_for_layout_type(ts);
    if (!layout) return 1;
    const auto& sd = *layout;
    if (sd.fields.empty()) return 1;
    if (sd.is_union) return flat_scalar_count(field_type_of(sd.fields.front()));
    long long count = 0;
    for (const auto& f : sd.fields) count += flat_scalar_count(field_type_of(f));
    return count > 0 ? count : 1;
  }
  return 1;
}

long long Lowerer::deduce_array_size_from_init(const GlobalInit& init) const {
  if (const auto* list = std::get_if<InitList>(&init)) {
    long long max_idx = -1;
    long long next = 0;
    for (const auto& item : list->items) {
      long long idx = next;
      if (item.index_designator && *item.index_designator >= 0) idx = *item.index_designator;
      if (idx > max_idx) max_idx = idx;
      next = idx + 1;
    }
    return max_idx + 1;
  }
  if (is_string_scalar(init)) {
    const auto& scalar = std::get<InitScalar>(init);
    const Expr& e = module_->expr_pool[scalar.expr.value];
    const auto& sl = std::get<StringLiteral>(e.payload);
    if (sl.is_wide) {
      return static_cast<long long>(decode_string_literal_values(sl.raw.c_str(), true).size());
    }
    return static_cast<long long>(bytes_from_string_literal(sl).size()) + 1;
  }
  return -1;
}

TypeSpec Lowerer::resolve_array_ts(const TypeSpec& ts, const GlobalInit& init) const {
  if (ts.array_rank <= 0 || ts.array_size >= 0) return ts;
  long long deduced = deduce_array_size_from_init(init);
  if (deduced < 0) return ts;
  TypeSpec elem_ts = ts;
  elem_ts.array_rank--;
  elem_ts.array_size = (elem_ts.array_rank > 0) ? elem_ts.array_dims[0] : -1;
  const bool elem_is_agg = is_vector_ty(elem_ts) ||
                           elem_ts.array_rank > 0 ||
                           ((elem_ts.base == TB_STRUCT || elem_ts.base == TB_UNION) &&
                            elem_ts.ptr_level == 0);
  if (elem_is_agg) {
    if (const auto* list = std::get_if<InitList>(&init)) {
      bool all_scalar = true;
      for (const auto& item : list->items) {
        if (!std::holds_alternative<InitScalar>(item.value)) {
          all_scalar = false;
          break;
        }
      }
      if (all_scalar) {
        const long long fc = flat_scalar_count(elem_ts);
        if (fc > 1) deduced = (deduced + fc - 1) / fc;
      }
    }
  }
  TypeSpec out = ts;
  out.array_size = deduced;
  out.array_dims[0] = deduced;
  return out;
}

bool Lowerer::is_direct_char_array_init(const TypeSpec& ts, const GlobalInit& init) const {
  if (ts.array_rank != 1 || ts.ptr_level != 0) return false;
  if (!is_char_like(ts.base)) return false;
  return is_string_scalar(init);
}

bool Lowerer::union_allows_init_normalization(const TypeSpec& ts) const {
  if (ts.base != TB_UNION || ts.ptr_level != 0) return false;
  const HirStructDef* layout = find_struct_def_for_layout_type(ts);
  if (!layout) return false;
  const auto& sd = *layout;
  if (!sd.is_union || sd.fields.empty()) return false;
  for (const auto& field : sd.fields) {
    TypeSpec field_ts = field_init_type_of(field);
    if (field_ts.array_rank > 0 || is_vector_ty(field_ts)) continue;
    if (field_ts.base == TB_STRUCT && field_ts.ptr_level == 0 &&
        !struct_allows_init_normalization(field_ts)) {
      return false;
    }
    if (field_ts.base == TB_UNION && field_ts.ptr_level == 0 &&
        !union_allows_init_normalization(field_ts)) {
      return false;
    }
  }
  return true;
}

bool Lowerer::struct_allows_init_normalization(const TypeSpec& ts) const {
  if (ts.base != TB_STRUCT || ts.ptr_level != 0) return false;
  const HirStructDef* layout = find_struct_def_for_layout_type(ts);
  if (!layout) return false;
  const auto& sd = *layout;
  if (sd.is_union) return false;
  for (size_t fi = 0; fi < sd.fields.size(); ++fi) {
    const auto& field = sd.fields[fi];
    if (field.is_flexible_array) {
      if (fi + 1 != sd.fields.size()) return false;
      continue;
    }
    TypeSpec field_ts = field_init_type_of(field);
    if (field_ts.array_rank > 0 || is_vector_ty(field_ts)) continue;
    if (field_ts.base == TB_STRUCT && field_ts.ptr_level == 0 &&
        !struct_allows_init_normalization(field_ts)) {
      return false;
    }
    if (field_ts.base == TB_UNION && field_ts.ptr_level == 0 &&
        !union_allows_init_normalization(field_ts)) {
      return false;
    }
  }
  return true;
}

GlobalInit Lowerer::normalize_global_init(const TypeSpec& ts, const GlobalInit& init) {
  std::function<GlobalInit(const TypeSpec&, const InitList&, size_t&)> consume_from_flat;
  auto has_designators = [](const InitList& list) {
    return std::any_of(
        list.items.begin(), list.items.end(),
        [](const InitListItem& item) {
          return init_list_has_field_designator(item) || item.index_designator.has_value();
        });
  };
  auto find_aggregate_field_index =
      [&](const HirStructDef& sd, const InitListItem& item, size_t next_idx)
      -> std::optional<size_t> {
    size_t idx = next_idx;
    const std::string_view designator =
        init_list_field_designator_text(item, module_ ? module_->link_name_texts.get() : nullptr);
    if (!designator.empty()) {
      const auto fit = std::find_if(
          sd.fields.begin(), sd.fields.end(),
          [&](const HirStructField& field) { return field.name == designator; });
      if (fit == sd.fields.end()) return std::nullopt;
      idx = static_cast<size_t>(std::distance(sd.fields.begin(), fit));
    } else if (item.index_designator && *item.index_designator >= 0) {
      idx = static_cast<size_t>(*item.index_designator);
    }
    if (idx >= sd.fields.size()) return std::nullopt;
    return idx;
  };
  auto make_field_mapped_item =
      [&](const HirStructDef& sd, size_t idx, const TypeSpec& target_ts,
          const GlobalInit& child) -> std::optional<InitListItem> {
    auto item = make_init_item(child);
    if (!item) return std::nullopt;
    if (!sd.fields[idx].name.empty()) {
      item->field_designator = sd.fields[idx].name;
      item->field_designator_text_id = make_text_id(
          sd.fields[idx].name, module_ ? module_->link_name_texts.get() : nullptr);
    } else {
      item->index_designator = static_cast<long long>(idx);
      item->field_designator_text_id = kInvalidText;
    }
    if (target_ts.array_rank > 0 && target_ts.array_size >= 0) {
      item->resolved_array_bound = target_ts.array_size;
    }
    return item;
  };
  auto make_indexed_item =
      [&](long long idx, const TypeSpec& target_ts, const GlobalInit& child)
      -> std::optional<InitListItem> {
    auto item = make_init_item(child);
    if (!item) return std::nullopt;
    item->index_designator = idx;
    item->field_designator.reset();
    item->field_designator_text_id = kInvalidText;
    if (target_ts.array_rank > 0 && target_ts.array_size >= 0) {
      item->resolved_array_bound = target_ts.array_size;
    }
    return item;
  };

  auto normalize_scalar_like = [&](const TypeSpec& cur_ts, const GlobalInit& cur_init)
      -> GlobalInit {
    if (const auto* scalar = std::get_if<InitScalar>(&cur_init)) return GlobalInit(*scalar);
    if (const auto* list = std::get_if<InitList>(&cur_init)) {
      if (!list->items.empty()) return normalize_global_init(cur_ts, child_init_of(list->items.front()));
    }
    return std::monostate{};
  };

  consume_from_flat = [&](const TypeSpec& cur_ts, const InitList& list, size_t& cursor)
      -> GlobalInit {
    if (cursor >= list.items.size()) return std::monostate{};
    const auto& item = list.items[cursor];
    if (std::holds_alternative<std::shared_ptr<InitList>>(item.value)) {
      auto sub = std::get<std::shared_ptr<InitList>>(item.value);
      ++cursor;
      return normalize_global_init(cur_ts, GlobalInit(*sub));
    }
    if (is_scalar_init_type(cur_ts)) {
      ++cursor;
      if (const auto* scalar = std::get_if<InitScalar>(&item.value)) return GlobalInit(*scalar);
      return std::monostate{};
    }
    if (is_vector_ty(cur_ts) || cur_ts.array_rank > 0) {
      TypeSpec elem_ts = cur_ts;
      long long bound = 0;
      if (is_vector_ty(cur_ts)) {
        elem_ts = vector_element_type(cur_ts);
        bound = cur_ts.vector_lanes > 0 ? cur_ts.vector_lanes : 0;
      } else {
        for (int di = 0; di < elem_ts.array_rank - 1; ++di)
          elem_ts.array_dims[di] = elem_ts.array_dims[di + 1];
        elem_ts.array_dims[elem_ts.array_rank - 1] = -1;
        elem_ts.array_rank--;
        elem_ts.array_size = (elem_ts.array_rank > 0) ? elem_ts.array_dims[0] : -1;
        bound = resolve_array_ts(cur_ts, GlobalInit(list)).array_size;
      }
      if (!is_vector_ty(cur_ts) && is_direct_char_array_init(cur_ts, child_init_of(item))) {
        ++cursor;
        return child_init_of(item);
      }
      InitList out{};
      for (long long i = 0; i < bound && cursor < list.items.size(); ++i) {
        auto child = consume_from_flat(elem_ts, list, cursor);
        if (auto it = make_init_item(child)) out.items.push_back(std::move(*it));
      }
      return out;
    }
    if ((cur_ts.base == TB_STRUCT || cur_ts.base == TB_UNION) &&
        cur_ts.ptr_level == 0) {
      const HirStructDef* layout = find_struct_def_for_layout_type(cur_ts);
      if (!layout) {
        ++cursor;
        return std::monostate{};
      }
      const auto& sd = *layout;
      if (sd.is_union) {
        InitList out{};
        if (!sd.fields.empty()) {
          TypeSpec field_ts = field_init_type_of(sd.fields.front());
          auto child = consume_from_flat(field_ts, list, cursor);
          field_ts = resolve_array_ts(field_ts, child);
          child = normalize_global_init(field_ts, child);
          if (auto it = make_field_mapped_item(sd, 0, field_ts, child)) {
            out.items.push_back(std::move(*it));
          }
        }
        return out;
      }
      InitList out{};
      for (size_t fi = 0; fi < sd.fields.size() && cursor < list.items.size(); ++fi) {
        TypeSpec field_ts = field_init_type_of(sd.fields[fi]);
        auto child = consume_from_flat(field_ts, list, cursor);
        field_ts = resolve_array_ts(field_ts, child);
        child = normalize_global_init(field_ts, child);
        if (auto it = make_field_mapped_item(sd, fi, field_ts, child)) {
          out.items.push_back(std::move(*it));
        }
      }
      return out;
    }
    ++cursor;
    return std::monostate{};
  };

  if (is_scalar_init_type(ts)) return normalize_scalar_like(ts, init);

  if (is_vector_ty(ts) || ts.array_rank > 0) {
    const auto* list = std::get_if<InitList>(&init);
    if (!list) return init;
    TypeSpec elem_ts = ts;
    long long bound = 0;
    if (is_vector_ty(ts)) {
      elem_ts = vector_element_type(ts);
      bound = ts.vector_lanes > 0 ? ts.vector_lanes : 0;
    } else {
      for (int di = 0; di < elem_ts.array_rank - 1; ++di)
        elem_ts.array_dims[di] = elem_ts.array_dims[di + 1];
      elem_ts.array_dims[elem_ts.array_rank - 1] = -1;
      elem_ts.array_rank--;
      elem_ts.array_size = (elem_ts.array_rank > 0) ? elem_ts.array_dims[0] : -1;
      bound = resolve_array_ts(ts, init).array_size;
    }
    if (!list->items.empty() && !is_vector_ty(ts) &&
        is_direct_char_array_init(ts, child_init_of(list->items.front()))) {
      return child_init_of(list->items.front());
    }

    if (!is_vector_ty(ts) && has_designators(*list)) {
      if (bound <= 0) return init;
      std::vector<std::optional<GlobalInit>> slots(static_cast<size_t>(bound));
      long long next_idx = 0;
      for (const auto& item : list->items) {
        long long idx = next_idx;
        if (item.index_designator && *item.index_designator >= 0) idx = *item.index_designator;
        if (idx >= 0 && idx < bound) {
          slots[static_cast<size_t>(idx)] = normalize_global_init(elem_ts, child_init_of(item));
        }
        next_idx = idx + 1;
      }
      InitList out{};
      for (long long idx = 0; idx < bound; ++idx) {
        const auto& slot = slots[static_cast<size_t>(idx)];
        if (!slot) continue;
        if (auto item = make_indexed_item(idx, elem_ts, *slot)) out.items.push_back(std::move(*item));
      }
      return out;
    }

    InitList out{};
    size_t cursor = 0;
    for (long long i = 0; i < bound && cursor < list->items.size(); ++i) {
      auto child = consume_from_flat(elem_ts, *list, cursor);
      if (!is_vector_ty(ts)) {
        if (auto it = make_indexed_item(i, elem_ts, child)) out.items.push_back(std::move(*it));
      } else if (auto it = make_init_item(child)) {
        out.items.push_back(std::move(*it));
      }
    }
    return out;
  }

  if (ts.base == TB_UNION && ts.ptr_level == 0) {
    const HirStructDef* layout = find_struct_def_for_layout_type(ts);
    if (!layout) return init;
    const auto& sd = *layout;
    if (!sd.is_union || sd.fields.empty()) return init;

    size_t idx = 0;
    GlobalInit child = std::monostate{};
    bool has_child = false;
    if (const auto* list = std::get_if<InitList>(&init)) {
      if (list->items.empty()) return init;
      const auto& item0 = list->items.front();
      const auto maybe_idx = find_aggregate_field_index(sd, item0, 0);
      if (maybe_idx && (init_list_has_field_designator(item0) || item0.index_designator)) {
        idx = *maybe_idx;
        child = child_init_of(item0);
      } else {
        child = (list->items.size() == 1 &&
                 std::holds_alternative<std::shared_ptr<InitList>>(item0.value))
            ? GlobalInit(*std::get<std::shared_ptr<InitList>>(item0.value))
            : GlobalInit(*list);
      }
      has_child = true;
    } else {
      child = normalize_scalar_like(field_init_type_of(sd.fields.front()), init);
      has_child = !std::holds_alternative<std::monostate>(child);
    }
    if (!has_child) return init;
    TypeSpec field_ts = field_init_type_of(sd.fields[idx]);
    field_ts = resolve_array_ts(field_ts, child);
    auto normalized_child = normalize_global_init(field_ts, child);
    InitList out{};
    if (auto item = make_field_mapped_item(sd, idx, field_ts, normalized_child)) {
      out.items.push_back(std::move(*item));
    }
    return out;
  }

  if ((ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.ptr_level == 0) {
    const auto* list = std::get_if<InitList>(&init);
    const HirStructDef* layout = find_struct_def_for_layout_type(ts);
    if (!layout) {
      return list ? init : normalize_scalar_like(ts, init);
    }
    const auto& sd = *layout;
    if (sd.is_union) return init;
    if (!struct_allows_init_normalization(ts)) {
      return list ? init : normalize_scalar_like(ts, init);
    }
    if (!list) {
      if (sd.fields.empty()) return std::monostate{};
      TypeSpec field_ts = field_init_type_of(sd.fields.front());
      field_ts = resolve_array_ts(field_ts, init);
      auto child = normalize_global_init(field_ts, init);
      InitList out{};
      if (auto item = make_field_mapped_item(sd, 0, field_ts, child)) {
        out.items.push_back(std::move(*item));
      }
      return out;
    }

    InitList out{};
    if (has_designators(*list)) {
      size_t next_idx = 0;
      for (const auto& item : list->items) {
        const auto maybe_idx = find_aggregate_field_index(sd, item, next_idx);
        if (!maybe_idx) continue;
        const size_t idx = *maybe_idx;
        TypeSpec field_ts = field_init_type_of(sd.fields[idx]);
        field_ts = resolve_array_ts(field_ts, child_init_of(item));
        auto child = normalize_global_init(field_ts, child_init_of(item));
        auto normalized_item = make_field_mapped_item(sd, idx, field_ts, child);
        if (!normalized_item) {
          next_idx = idx + 1;
          continue;
        }
        out.items.push_back(std::move(*normalized_item));
        next_idx = idx + 1;
      }
      return out;
    }

    size_t cursor = 0;
    for (size_t fi = 0; fi < sd.fields.size(); ++fi) {
      if (cursor >= list->items.size()) break;
      TypeSpec field_ts = field_init_type_of(sd.fields[fi]);
      auto child = consume_from_flat(field_ts, *list, cursor);
      field_ts = resolve_array_ts(field_ts, child);
      child = normalize_global_init(field_ts, child);
      if (auto it = make_field_mapped_item(sd, fi, field_ts, child))
        out.items.push_back(std::move(*it));
    }
    return out;
  }

  return init;
}

const Node* Lowerer::find_struct_static_member_decl(
    const std::string& tag, const std::string& member) const {
  const auto owner_key = make_struct_member_lookup_key(tag, member);
  if (owner_key) {
    const auto owner_it = struct_static_member_decls_by_owner_.find(*owner_key);
    if (owner_it != struct_static_member_decls_by_owner_.end()) {
      auto sit = struct_static_member_decls_.find(tag);
      if (sit != struct_static_member_decls_.end()) {
        auto mit = sit->second.find(member);
        if (mit != sit->second.end()) {
          record_struct_static_member_decl_lookup_parity(tag, member, mit->second);
        }
      }
      return owner_it->second;
    }
    if (module_) {
      if (const HirStructDef* structured =
              module_->find_struct_def_by_owner_structured(owner_key->owner_key)) {
        for (const auto& base_tag : structured->base_tags) {
          if (const Node* from_base =
                  find_struct_static_member_decl(base_tag, member)) {
            return from_base;
          }
        }
      }
    }
    return nullptr;
  }
  auto sit = struct_static_member_decls_.find(tag);
  if (sit != struct_static_member_decls_.end()) {
    auto mit = sit->second.find(member);
    if (mit != sit->second.end()) {
      record_struct_static_member_decl_lookup_parity(tag, member, mit->second);
      return mit->second;
    }
  }
  if (module_) {
    auto dit = module_->struct_defs.find(tag);
    if (dit != module_->struct_defs.end()) {
      for (const auto& base_tag : dit->second.base_tags) {
        if (const Node* from_base = find_struct_static_member_decl(base_tag, member))
          return from_base;
      }
    }
  }
  return nullptr;
}

const Node* Lowerer::find_struct_static_member_decl(
    const HirStructMemberLookupKey& key,
    const std::string* rendered_tag,
    const std::string* rendered_member) const {
  if (!hir_struct_member_lookup_key_has_complete_metadata(key)) return nullptr;
  const auto owner_it = struct_static_member_decls_by_owner_.find(key);
  if (owner_it != struct_static_member_decls_by_owner_.end() &&
      rendered_tag && rendered_member) {
    auto sit = struct_static_member_decls_.find(*rendered_tag);
    if (sit != struct_static_member_decls_.end()) {
      auto mit = sit->second.find(*rendered_member);
      if (mit != sit->second.end()) {
        record_struct_static_member_decl_lookup_parity(
            *rendered_tag, *rendered_member, mit->second);
      }
    }
  }
  if (owner_it != struct_static_member_decls_by_owner_.end()) {
    return owner_it->second;
  }
  if (!module_ || !module_->link_name_texts) return nullptr;
  const std::string_view member_text =
      module_->link_name_texts->lookup(key.member_text_id);
  if (member_text.empty()) return nullptr;
  const HirStructDef* structured =
      module_->find_struct_def_by_owner_structured(key.owner_key);
  if (!structured) return nullptr;
  const std::string member(member_text);
  for (const auto& base_tag : structured->base_tags) {
    if (const Node* from_base = find_struct_static_member_decl(base_tag, member)) {
      return from_base;
    }
  }
  return nullptr;
}

std::optional<long long> Lowerer::find_struct_static_member_const_value(
    const std::string& tag, const std::string& member) const {
  auto try_selected_template_pattern = [&]() -> std::optional<long long> {
    const auto node_it = struct_def_nodes_.find(tag);
    if (node_it == struct_def_nodes_.end() || !node_it->second) {
      return std::nullopt;
    }
    const Node* owner = node_it->second;
    const char* origin =
        (owner->template_origin_name && owner->template_origin_name[0])
            ? owner->template_origin_name
            : nullptr;
    if (!origin || owner->n_template_args <= 0) return std::nullopt;
    std::vector<HirTemplateArg> actual_args;
    actual_args.reserve(owner->n_template_args);
    for (int i = 0; i < owner->n_template_args; ++i) {
      HirTemplateArg arg{};
      arg.is_value =
          owner->template_arg_is_value && owner->template_arg_is_value[i];
      if (arg.is_value) {
        arg.value = owner->template_arg_values ? owner->template_arg_values[i] : 0;
      } else if (owner->template_arg_types) {
        arg.type = owner->template_arg_types[i];
      }
      actual_args.push_back(arg);
    }
    const Node* primary = find_template_struct_primary(origin);
    if (!primary) {
      const auto primary_it = struct_def_nodes_.find(origin);
      if (primary_it != struct_def_nodes_.end()) primary = primary_it->second;
    }
    if (!primary) return std::nullopt;
    TemplateStructEnv env = build_template_struct_env(primary);
    SelectedTemplateStructPattern selected =
        select_template_struct_pattern_hir(actual_args, env);
    if (!selected.selected_pattern) return std::nullopt;
    long long value = 0;
    if (eval_struct_static_member_value_hir(
            selected.selected_pattern, struct_def_nodes_, member,
            &selected.nttp_bindings, &value)) {
      return value;
    }
    if (selected.selected_pattern == primary) return std::nullopt;
    for (int bi = 0; bi < selected.selected_pattern->n_bases; ++bi) {
      const TypeSpec& base_ts = selected.selected_pattern->base_types[bi];
      if (!base_ts.tpl_struct_origin || !base_ts.tpl_struct_origin[0]) continue;
      const Node* base_primary = find_template_struct_primary(base_ts.tpl_struct_origin);
      if (!base_primary) {
        const auto base_primary_it =
            struct_def_nodes_.find(base_ts.tpl_struct_origin);
        if (base_primary_it != struct_def_nodes_.end()) {
          base_primary = base_primary_it->second;
        }
      }
      if (!base_primary) continue;
      ResolvedTemplateArgs resolved_base;
      const int base_arg_count =
          base_ts.tpl_struct_args.data ? base_ts.tpl_struct_args.size : 0;
      resolved_base.concrete_args.reserve(base_arg_count);
      for (int ai = 0; ai < base_arg_count; ++ai) {
        const TemplateArgRef& ref = base_ts.tpl_struct_args.data[ai];
        HirTemplateArg arg{};
        arg.is_value = ref.kind == TemplateArgKind::Value;
        if (arg.is_value) {
          arg.value = ref.value;
          if (ref.nttp_param_kind == TemplateParamDomainKind::NonType &&
              ref.debug_text && ref.debug_text[0]) {
            auto bound = selected.nttp_bindings.find(ref.debug_text);
            if (bound != selected.nttp_bindings.end()) arg.value = bound->second;
          }
        } else {
          arg.type = ref.type;
          if (arg.type.template_param_text_id != kInvalidText) {
            for (const auto& [name, bound] : selected.type_bindings) {
              if (ref.debug_text && name == ref.debug_text) {
                arg.type = bound;
                break;
              }
            }
          }
        }
        resolved_base.concrete_args.push_back(arg);
      }
      for (int pi = 0; pi < base_primary->n_template_params &&
                       pi < static_cast<int>(resolved_base.concrete_args.size());
           ++pi) {
        if (!base_primary->template_param_names ||
            !base_primary->template_param_names[pi]) {
          continue;
        }
        const char* param_name = base_primary->template_param_names[pi];
        const HirTemplateArg& arg = resolved_base.concrete_args[pi];
        if (base_primary->template_param_is_nttp &&
            base_primary->template_param_is_nttp[pi]) {
          resolved_base.nttp_bindings.push_back({param_name, arg.value});
        } else {
          resolved_base.type_bindings.push_back({param_name, arg.type});
        }
      }
      TemplateStructEnv base_env = build_template_struct_env(base_primary);
      SelectedTemplateStructPattern selected_base =
          select_template_struct_pattern_hir(resolved_base.concrete_args, base_env);
      const Node* base_pattern =
          selected_base.selected_pattern ? selected_base.selected_pattern : base_primary;
      const std::string base_tag = build_template_mangled_name(
          base_primary, base_pattern, base_ts.tpl_struct_origin, resolved_base);
      if (auto inherited = find_struct_static_member_const_value(base_tag, member)) {
        return inherited;
      }
    }
    return std::nullopt;
  };
  const auto owner_key = make_struct_member_lookup_key(tag, member);
  if (owner_key) {
    const auto owner_it = struct_static_member_const_values_by_owner_.find(*owner_key);
    if (owner_it != struct_static_member_const_values_by_owner_.end()) {
      auto sit = struct_static_member_const_values_.find(tag);
      if (sit != struct_static_member_const_values_.end()) {
        auto mit = sit->second.find(member);
        if (mit != sit->second.end()) {
          record_struct_static_member_const_value_lookup_parity(
              tag, member, mit->second);
        }
      }
      return owner_it->second;
    }
    if (auto selected_value = try_selected_template_pattern()) {
      return selected_value;
    }
    if (module_) {
      if (const HirStructDef* structured =
              module_->find_struct_def_by_owner_structured(owner_key->owner_key)) {
        for (const auto& base_tag : structured->base_tags) {
          if (auto from_base =
                  find_struct_static_member_const_value(base_tag, member)) {
            return from_base;
          }
        }
      }
    }
    if (auto trait_value =
            try_eval_instantiated_struct_static_member_const(tag, member)) {
      return trait_value;
    }
    return std::nullopt;
  }
  auto sit = struct_static_member_const_values_.find(tag);
  if (sit != struct_static_member_const_values_.end()) {
    auto mit = sit->second.find(member);
    if (mit != sit->second.end()) {
      record_struct_static_member_const_value_lookup_parity(
          tag, member, mit->second);
      return mit->second;
    }
  }
  if (auto trait_value = try_eval_instantiated_struct_static_member_const(tag, member)) {
    return trait_value;
  }
  auto dit = module_->struct_defs.find(tag);
  if (dit != module_->struct_defs.end()) {
    for (const auto& base_tag : dit->second.base_tags) {
      if (auto from_base = find_struct_static_member_const_value(base_tag, member))
        return from_base;
    }
  }
  return std::nullopt;
}

std::optional<long long> Lowerer::find_struct_static_member_const_value(
    const HirStructMemberLookupKey& key,
    const std::string* rendered_tag,
    const std::string* rendered_member) const {
  if (!hir_struct_member_lookup_key_has_complete_metadata(key)) {
    if (rendered_tag && rendered_member) {
      return find_struct_static_member_const_value(*rendered_tag, *rendered_member);
    }
    return std::nullopt;
  }
  const auto owner_it = struct_static_member_const_values_by_owner_.find(key);
  if (owner_it != struct_static_member_const_values_by_owner_.end() &&
      rendered_tag && rendered_member) {
    auto sit = struct_static_member_const_values_.find(*rendered_tag);
    if (sit != struct_static_member_const_values_.end()) {
      auto mit = sit->second.find(*rendered_member);
      if (mit != sit->second.end()) {
        record_struct_static_member_const_value_lookup_parity(
            *rendered_tag, *rendered_member, mit->second);
      }
    }
  }
  if (owner_it != struct_static_member_const_values_by_owner_.end()) {
    return owner_it->second;
  }

  if (!module_ || !module_->link_name_texts) return std::nullopt;
  const std::string_view member_text =
      module_->link_name_texts->lookup(key.member_text_id);
  if (member_text.empty()) return std::nullopt;
  const HirStructDef* structured =
      module_->find_struct_def_by_owner_structured(key.owner_key);
  if (!structured) return std::nullopt;
  const std::string member(member_text);
  for (const auto& base_tag : structured->base_tags) {
    if (auto from_base = find_struct_static_member_const_value(base_tag, member)) {
      return from_base;
    }
  }
  if (rendered_tag && *rendered_tag == structured->tag) {
    if (auto trait_value =
            try_eval_instantiated_struct_static_member_const(*rendered_tag, member)) {
      return trait_value;
    }
  }
  return std::nullopt;
}

MemberSymbolId Lowerer::find_struct_member_symbol_id(
    const std::string& tag, const std::string& member) const {
  const auto owner_key = make_struct_member_lookup_key(tag, member);
  if (owner_key) {
    const MemberSymbolId owner_id = find_struct_member_symbol_id(
        *owner_key, &tag, &member);
    if (owner_id != kInvalidMemberSymbol) return owner_id;
    if (const HirStructDef* structured =
            module_->find_struct_def_by_owner_structured(owner_key->owner_key)) {
      for (const auto& base_tag : structured->base_tags) {
        const MemberSymbolId inherited_id =
            find_struct_member_symbol_id(base_tag, member);
        if (inherited_id != kInvalidMemberSymbol) return inherited_id;
      }
    }
    return kInvalidMemberSymbol;
  }
  const MemberSymbolId direct_id =
      module_->member_symbols.find(tag + "::" + member);
  if (direct_id != kInvalidMemberSymbol) {
    record_struct_member_symbol_id_lookup_parity(tag, member, direct_id);
    return direct_id;
  }
  auto dit = module_->struct_defs.find(tag);
  if (dit != module_->struct_defs.end()) {
    for (const auto& base_tag : dit->second.base_tags) {
      const MemberSymbolId inherited_id =
          find_struct_member_symbol_id(base_tag, member);
      if (inherited_id != kInvalidMemberSymbol) return inherited_id;
    }
  }
  return kInvalidMemberSymbol;
}

MemberSymbolId Lowerer::find_struct_member_symbol_id(
    const HirStructMemberLookupKey& key,
    const std::string* rendered_tag,
    const std::string* rendered_member) const {
  if (!hir_struct_member_lookup_key_has_complete_metadata(key)) {
    return kInvalidMemberSymbol;
  }
  const auto owner_it = struct_member_symbol_ids_by_owner_.find(key);
  if (owner_it == struct_member_symbol_ids_by_owner_.end()) {
    if (!module_ || !module_->link_name_texts) return kInvalidMemberSymbol;
    const std::string_view member_text =
        module_->link_name_texts->lookup(key.member_text_id);
    if (member_text.empty()) return kInvalidMemberSymbol;
    const HirStructDef* structured =
        module_->find_struct_def_by_owner_structured(key.owner_key);
    if (!structured) return kInvalidMemberSymbol;
    for (const auto& field : structured->fields) {
      const bool field_matches =
          (field.field_text_id != kInvalidText &&
           field.field_text_id == key.member_text_id) ||
          field.name == member_text;
      if (field_matches && field.member_symbol_id != kInvalidMemberSymbol) {
        return field.member_symbol_id;
      }
    }
    for (const auto& base_tag : structured->base_tags) {
      const MemberSymbolId inherited_id =
          find_struct_member_symbol_id(base_tag, std::string(member_text));
      if (inherited_id != kInvalidMemberSymbol) return inherited_id;
    }
    return kInvalidMemberSymbol;
  }
  if (rendered_tag && rendered_member) {
    const MemberSymbolId rendered_id =
        module_->member_symbols.find(*rendered_tag + "::" + *rendered_member);
    if (rendered_id != kInvalidMemberSymbol) {
      ++struct_member_symbol_id_lookup_parity_checks_;
      if (rendered_id != owner_it->second) {
        ++struct_member_symbol_id_lookup_parity_mismatches_;
      }
    }
  }
  return owner_it->second;
}

MemberSymbolId Lowerer::find_struct_member_symbol_id(
    const TypeSpec& owner_ts,
    const std::string& rendered_tag,
    const std::string& member,
    TextId member_text_id) const {
  if (const auto key = make_struct_member_lookup_key(owner_ts, member_text_id)) {
    const MemberSymbolId owner_id =
        find_struct_member_symbol_id(*key, &rendered_tag, &member);
    if (owner_id != kInvalidMemberSymbol) return owner_id;
    if (const HirStructDef* structured =
            module_->find_struct_def_by_owner_structured(key->owner_key)) {
      for (const auto& base_tag : structured->base_tags) {
        const MemberSymbolId inherited_id =
            find_struct_member_symbol_id(base_tag, member);
        if (inherited_id != kInvalidMemberSymbol) return inherited_id;
      }
    }
    return kInvalidMemberSymbol;
  }
  if (!rendered_tag.empty()) {
    return find_struct_member_symbol_id(rendered_tag, member);
  }
  if (module_) {
    if (std::optional<std::string> compatibility_tag =
            rendered_typespec_tag_for_compatibility(*module_, owner_ts)) {
      return find_struct_member_symbol_id(*compatibility_tag, member);
    }
  }
  return kInvalidMemberSymbol;
}

bool Lowerer::has_side_effect_expr(const Node* n) const {
  if (!n) return false;
  switch (n->kind) {
    case NK_CALL:
    case NK_BUILTIN_CALL:
    case NK_ASSIGN:
    case NK_COMPOUND_ASSIGN:
    case NK_COMMA_EXPR:
      return true;
    case NK_POSTFIX: {
      const char* op = n->op ? n->op : "";
      if (std::strcmp(op, "++") == 0 || std::strcmp(op, "--") == 0) return true;
      break;
    }
    case NK_UNARY: {
      const char* op = n->op ? n->op : "";
      if (std::strcmp(op, "++pre") == 0 || std::strcmp(op, "--pre") == 0) return true;
      break;
    }
    default:
      break;
  }
  if (has_side_effect_expr(n->left)) return true;
  if (has_side_effect_expr(n->right)) return true;
  if (has_side_effect_expr(n->cond)) return true;
  if (has_side_effect_expr(n->then_)) return true;
  if (has_side_effect_expr(n->else_)) return true;
  if (has_side_effect_expr(n->init)) return true;
  if (has_side_effect_expr(n->update)) return true;
  if (has_side_effect_expr(n->body)) return true;
  for (int i = 0; i < n->n_children; ++i) {
    if (has_side_effect_expr(n->children ? n->children[i] : nullptr)) return true;
  }
  return false;
}

bool Lowerer::is_simple_constant_expr(const Node* n) const {
  if (!n) return false;
  switch (n->kind) {
    case NK_INT_LIT:
    case NK_FLOAT_LIT:
    case NK_CHAR_LIT:
      return true;
    case NK_CAST:
      return is_simple_constant_expr(n->left);
    case NK_UNARY: {
      const char* op = n->op ? n->op : "";
      if (std::strcmp(op, "+") == 0 || std::strcmp(op, "-") == 0 ||
          std::strcmp(op, "~") == 0) {
        return is_simple_constant_expr(n->left);
      }
      return false;
    }
    default:
      return false;
  }
}

bool Lowerer::can_fast_path_scalar_array_init(const Node* init_list) const {
  if (!init_list || init_list->kind != NK_INIT_LIST) return false;
  for (int i = 0; i < init_list->n_children; ++i) {
    const Node* item = init_list->children ? init_list->children[i] : nullptr;
    if (!item) continue;
    if (item->kind == NK_INIT_ITEM && item->is_designated) return false;
    const Node* v = init_item_value_node(item);
    if (!v) return false;
    if (v->kind == NK_INIT_LIST || v->kind == NK_COMPOUND_LIT) return false;
    if (has_side_effect_expr(v)) return false;
    if (!is_simple_constant_expr(v)) return false;
  }
  return true;
}

const Node* Lowerer::init_item_value_node(const Node* item) const {
  if (!item) return nullptr;
  if (item->kind != NK_INIT_ITEM) return item;
  const Node* v = item->left ? item->left : item->right;
  if (!v) v = item->init;
  if (!v && item->n_children > 0) {
    for (int i = 0; i < item->n_children; ++i) {
      if (item->children && item->children[i]) {
        v = item->children[i];
        break;
      }
    }
  }
  return v;
}

const Node* Lowerer::unwrap_init_scalar_value(const Node* node) const {
  const Node* cur = node;
  for (int guard = 0; guard < 32 && cur; ++guard) {
    if (cur->kind == NK_INIT_ITEM) {
      const Node* next = init_item_value_node(cur);
      if (!next || next == cur) break;
      cur = next;
      continue;
    }
    if (cur->kind == NK_INIT_LIST) {
      const Node* first = nullptr;
      for (int i = 0; i < cur->n_children; ++i) {
        if (cur->children && cur->children[i]) {
          first = cur->children[i];
          break;
        }
      }
      if (!first) break;
      const Node* next = init_item_value_node(first);
      if (!next || next == cur) break;
      cur = next;
      continue;
    }
    break;
  }
  return cur;
}

GlobalId Lowerer::lower_static_local_global(FunctionCtx& ctx, const Node* n) {
  GlobalVar g{};
  g.id = next_global_id();
  g.name = "__static_local_" + sanitize_symbol(ctx.fn->name) + "_" + std::to_string(g.id.value);
  g.name_text_id = kInvalidText;
  g.type = qtype_from(n->type, ValueCategory::LValue);
  g.fn_ptr_sig = fn_ptr_sig_from_decl_node(n);
  g.linkage = {true, false, false};
  g.is_const = n->type.is_const;
  g.span = make_span(n);
  if (n->init) {
    g.init = lower_global_init(n->init, &ctx);
    g.type.spec = resolve_array_ts(g.type.spec, g.init);
    g.init = normalize_global_init(g.type.spec, g.init);
  }
  module_->index_global_decl(g);
  module_->globals.push_back(std::move(g));
  return g.id;
}

GlobalInit Lowerer::lower_global_init(const Node* n,
                                      FunctionCtx* ctx,
                                      bool allow_named_const_fold) {
  if (!n) return std::monostate{};
  if (n->kind == NK_INIT_LIST) {
    return lower_init_list(n, ctx);
  }
  if (n->kind == NK_COMPOUND_LIT && n->left && n->left->kind == NK_INIT_LIST) {
    return lower_init_list(n->left, ctx);
  }
  InitScalar s{};
  if (!ctx && allow_named_const_fold) {
    LowererConstEvalStructuredMaps structured_maps;
    ConstEvalEnv env = make_lowerer_consteval_env(structured_maps);
    if (auto r = evaluate_constant_expr(n, env); r.ok()) {
      TypeSpec ts = n->type;
      if (ts.base == TB_VOID) ts.base = TB_INT;
      s.expr = append_expr(n, IntLiteral{r.as_int(), false}, ts);
      return s;
    }
  }
  if (ctx && allow_named_const_fold && n->kind == NK_VAR && n->name &&
      n->name[0] && (n->has_template_args || n->n_template_args > 0)) {
    if (const std::optional<HirRecordOwnerKey> owner_key =
            structured_owner_key_from_qualified_ref(
                n, module_ ? module_->link_name_texts.get() : nullptr)) {
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
          }
        };
        try_generated_member_payload(owner_name);
        if (owner_tag) try_generated_member_payload(*owner_tag);
      }

      const std::string& primitive_trait_owner =
          !owner_name.empty() ? owner_name : (owner_tag ? *owner_tag : owner_name);
      std::optional<long long> primitive_trait_value;
      if (!primitive_trait_owner.empty() && !member_name.empty()) {
        if (auto v = try_eval_template_static_member_const(
                ctx, primitive_trait_owner, n, member_name)) {
          if (*v != 0) {
            TypeSpec ts = n->type;
            if (ts.base == TB_VOID) ts.base = TB_INT;
            s.expr = append_expr(n, IntLiteral{*v, false}, ts);
            return s;
          }
          primitive_trait_value = *v;
        }
      }
      if (!primitive_trait_owner.empty() && !unqualified_member_name.empty() &&
          unqualified_member_name != member_name) {
        if (auto v = try_eval_template_static_member_const(
                ctx, primitive_trait_owner, n, unqualified_member_name)) {
          if (*v != 0) {
            TypeSpec ts = n->type;
            if (ts.base == TB_VOID) ts.base = TB_INT;
            s.expr = append_expr(n, IntLiteral{*v, false}, ts);
            return s;
          }
          primitive_trait_value = *v;
        }
      }

      std::optional<std::string> realized_owner_tag;
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
              candidate_key.is_global_qualified != owner_key->is_global_qualified ||
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
            "global-init-static-owner-structured-arg");
        TypeBindings tpl_empty;
        NttpBindings nttp_empty;
        seed_and_resolve_pending_template_type_if_needed(
            pending_ts, ctx ? ctx->tpl_bindings : tpl_empty,
            ctx ? ctx->nttp_bindings : nttp_empty, n,
            PendingTemplateTypeKind::OwnerStruct,
            "global-init-static-owner-structured", primary_tpl);
        realized_owner_tag = resolve_member_lookup_owner_tag(
            pending_ts, false, ctx ? &ctx->tpl_bindings : nullptr,
            ctx ? &ctx->nttp_bindings : nullptr, nullptr, n,
            "global-init-static-owner-structured");
      }

      std::string template_owner = owner_template_origin_name;
      if (template_owner.empty()) template_owner = owner_name;
      if (template_owner.empty() && realized_owner_tag) {
        template_owner = *realized_owner_tag;
      }
      if (template_owner.empty() && owner_tag) template_owner = *owner_tag;
      if (!template_owner.empty() && !member_name.empty()) {
        if (auto v = try_eval_template_static_member_const(
                ctx, template_owner, n, member_name)) {
          TypeSpec ts = n->type;
          if (ts.base == TB_VOID) ts.base = TB_INT;
          s.expr = append_expr(n, IntLiteral{*v, false}, ts);
          return s;
        }
      }
      if (!template_owner.empty() && !unqualified_member_name.empty() &&
          unqualified_member_name != member_name) {
        if (auto v = try_eval_template_static_member_const(
                ctx, template_owner, n, unqualified_member_name)) {
          TypeSpec ts = n->type;
          if (ts.base == TB_VOID) ts.base = TB_INT;
          s.expr = append_expr(n, IntLiteral{*v, false}, ts);
          return s;
        }
      }
      if (primitive_trait_value) {
        TypeSpec ts = n->type;
        if (ts.base == TB_VOID) ts.base = TB_INT;
        s.expr = append_expr(n, IntLiteral{*primitive_trait_value, false}, ts);
        return s;
      }
    }
  }
  s.expr = lower_expr(ctx, n);
  return s;
}

InitList Lowerer::lower_init_list(const Node* n, FunctionCtx* ctx) {
  InitList out{};
  for (int i = 0; i < n->n_children; ++i) {
    const Node* it = n->children[i];
    if (!it) continue;
    InitListItem item{};
    const Node* value_node = it;
    if (it->kind == NK_INIT_ITEM) {
      if (it->is_designated) {
        if (it->is_index_desig) {
          item.index_designator = it->desig_val;
        } else if (it->desig_field) {
          item.field_designator = std::string(it->desig_field);
          item.field_designator_text_id = make_text_id(
              *item.field_designator, module_ ? module_->link_name_texts.get() : nullptr);
        }
      }
      value_node = it->left ? it->left : it->right;
      if (!value_node) value_node = it->init;
    }

    if (value_node && value_node->kind == NK_INIT_LIST) {
      item.value = std::make_shared<InitList>(lower_init_list(value_node, ctx));
    } else if (value_node && value_node->kind == NK_COMPOUND_LIT &&
               value_node->left && value_node->left->kind == NK_INIT_LIST) {
      item.value = std::make_shared<InitList>(lower_init_list(value_node->left, ctx));
    } else if (!ctx && value_node && value_node->kind == NK_ADDR &&
               value_node->left && value_node->left->kind == NK_COMPOUND_LIT) {
      InitScalar s{};
      s.expr = hoist_compound_literal_to_global(value_node, value_node->left);
      item.value = s;
    } else {
      InitScalar s{};
      s.expr = lower_expr(ctx, value_node);
      item.value = s;
    }
    out.items.push_back(std::move(item));
  }
  return out;
}

void Lowerer::lower_struct_def(const Node* sd) {
  if (!sd || sd->kind != NK_STRUCT_DEF) return;
  // Skip primary templates and specialization patterns that still depend on
  // template parameters. Concrete explicit specializations and realized
  // instantiations are still lowered as ordinary structs.
  if (is_primary_template_struct_def(sd) ||
      is_dependent_template_struct_specialization(sd)) {
    return;
  }
  const char* tag = sd->name;
  if (!tag || !tag[0]) return;

  // Gather fields: they may be in sd->fields[] (n_fields) OR sd->children[] (n_children)
  // The IR builder uses sd->fields; but some parsers store struct fields in children.
  int num_fields = sd->n_fields > 0 ? sd->n_fields : sd->n_children;
  auto get_field = [&](int i) -> const Node* {
    return sd->n_fields > 0 ? sd->fields[i] : sd->children[i];
  };

  HirStructDef def;
  def.tag = tag;
  def.tag_text_id = make_ast_node_unqualified_text_id_for_owner_key(
      sd, module_ ? module_->link_name_texts.get() : nullptr);
  if (def.tag_text_id == kInvalidText) {
    def.tag_text_id = make_text_id(
        def.tag, module_ ? module_->link_name_texts.get() : nullptr);
  }
  def.ns_qual = make_ast_node_ns_qual_for_owner_key(
      sd, module_ ? module_->link_name_texts.get() : nullptr);
  const HirRecordOwnerKey struct_owner_key = make_hir_record_owner_key(def);
  const bool has_struct_owner_key =
      hir_record_owner_key_has_complete_metadata(struct_owner_key);
  if (has_struct_owner_key) {
    struct_def_owner_by_node_[sd] = struct_owner_key;
  }
  auto find_existing_struct_def = [&]() -> const HirStructDef* {
    if (has_struct_owner_key) {
      return module_->find_struct_def_by_owner_structured(struct_owner_key);
    }
    const auto rendered = module_->struct_defs.find(tag);
    return rendered == module_->struct_defs.end() ? nullptr : &rendered->second;
  };

  // If already fully populated, skip (forward decl followed by full def is OK)
  const HirStructDef* existing_struct_def = find_existing_struct_def();
  if (existing_struct_def && !existing_struct_def->fields.empty() &&
      num_fields == 0) {
    return;
  }

  def.is_union = sd->is_union;
  def.pack_align = sd->pack_align;
  def.struct_align = sd->struct_align;
  TypeBindings struct_tpl_bindings;
  NttpBindings struct_nttp_bindings;
  std::unordered_map<TextId, TypeSpec> struct_tpl_bindings_by_text;
  if (sd->n_template_args > 0 && sd->template_param_names) {
    for (int i = 0; i < sd->n_template_args; ++i) {
      const char* pname = sd->template_param_names[i];
      if (!pname) continue;
      if (sd->template_param_is_nttp && sd->template_param_is_nttp[i]) {
        struct_nttp_bindings[pname] = sd->template_arg_values[i];
      } else if (sd->template_arg_types) {
        struct_tpl_bindings[pname] = sd->template_arg_types[i];
        if (sd->template_param_name_text_ids &&
            sd->template_param_name_text_ids[i] != kInvalidText) {
          struct_tpl_bindings_by_text[sd->template_param_name_text_ids[i]] =
              sd->template_arg_types[i];
        }
      }
    }
  }
  for (int bi = 0; bi < sd->n_bases; ++bi) {
    TypeSpec base = sd->base_types[bi];
    if (base.tpl_struct_origin) {
      // Resolve pending template base type (e.g. deferred $expr: NTTP args).
      // Build bindings from the struct's own template args if available.
      TypeBindings base_tpl_bindings;
      NttpBindings base_nttp_bindings;
      if (sd->n_template_args > 0 && sd->n_template_params > 0) {
        for (int pi = 0; pi < sd->n_template_params && pi < sd->n_template_args; ++pi) {
          const char* pname = sd->template_param_names[pi];
          if (sd->template_param_is_nttp[pi]) {
            base_nttp_bindings[pname] = sd->template_arg_values[pi];
          } else {
            base_tpl_bindings[pname] = sd->template_arg_types[pi];
          }
        }
      }
      seed_and_resolve_pending_template_type_if_needed(
          base, base_tpl_bindings, base_nttp_bindings, sd,
          PendingTemplateTypeKind::BaseType,
          std::string("struct-base:") + (tag ? tag : ""));
    }
    std::optional<std::string> rendered_base_tag =
        module_ ? rendered_typespec_tag_for_compatibility(*module_, base)
                : std::nullopt;
    std::optional<std::string> base_tag;
    const HirStructDef* base_layout = find_struct_def_for_layout_type(base);
    if (base_layout) {
      base_tag = base_layout->tag;
    }
    const auto base_has_complete_owner_metadata = [&]() {
      if (base.record_def && base.record_def->kind == NK_STRUCT_DEF) {
        return make_struct_def_node_owner_key(base.record_def).has_value();
      }
      if (base.namespace_context_id >= 0 && base.tag_text_id != kInvalidText) {
        NamespaceQualifier ns_qual;
        ns_qual.context_id = base.namespace_context_id;
        ns_qual.is_global_qualified = base.is_global_qualified;
        if (base.qualifier_text_ids && base.n_qualifier_segments > 0) {
          ns_qual.segment_text_ids.assign(
              base.qualifier_text_ids,
              base.qualifier_text_ids + base.n_qualifier_segments);
        }
        return hir_record_owner_key_has_complete_metadata(
            make_hir_record_owner_key(ns_qual, base.tag_text_id));
      }
      return false;
    }();
    if ((!base_tag || base_tag->empty()) &&
        !base_has_complete_owner_metadata &&
        rendered_base_tag && !rendered_base_tag->empty() &&
        module_->struct_defs.count(*rendered_base_tag)) {
      base_tag = rendered_base_tag;
    }
    if ((!base_tag || base_tag->empty()) && base.record_def &&
        base.record_def->kind == NK_STRUCT_DEF && base.record_def->name &&
        base.record_def->name[0]) {
      base_tag = base.record_def->name;
    }
    if ((!base_tag || base_tag->empty()) &&
        !base_has_complete_owner_metadata && rendered_base_tag &&
        !rendered_base_tag->empty()) {
      base_tag = rendered_base_tag;
    }
    if (!resolve_struct_member_typedef_if_ready(&base) &&
        base.deferred_member_type_name && base_tag && !base_tag->empty()) {
      TypeBindings empty_tb;
      NttpBindings empty_nb;
      seed_pending_template_type(
          base, empty_tb, empty_nb, sd, PendingTemplateTypeKind::MemberTypedef,
          std::string("struct-base-member:") + (tag ? tag : ""));
    }
    if (base_tag && !base_tag->empty()) {
      def.base_tags.push_back(*base_tag);
      TextId base_tag_text_id = kInvalidText;
      if (base.tag_text_id != kInvalidText && module_ && module_->link_name_texts) {
        const std::string_view rendered_text =
            module_->link_name_texts->lookup(base.tag_text_id);
        if (rendered_text == *base_tag) base_tag_text_id = base.tag_text_id;
      }
      if (base_tag_text_id == kInvalidText && base_layout &&
          base_layout->tag_text_id != kInvalidText) {
        base_tag_text_id = base_layout->tag_text_id;
      }
      if (base_tag_text_id == kInvalidText) {
        base_tag_text_id = make_text_id(
            *base_tag, module_ ? module_->link_name_texts.get() : nullptr);
      }
      def.base_tag_text_ids.push_back(
          base_tag_text_id);
    }
  }

  int llvm_idx = 0;
  // Bitfield packing state (for structs only; unions always use offset 0)
  int bf_unit_start_bit = -1;  // bit position where current storage unit starts (-1 = none)
  int bf_unit_bits = 0;        // size of current storage unit in bits
  int bf_current_bit = 0;      // current bit position within the storage unit

  // Build NTTP bindings for template instantiations so static constexpr
  // members that reference NTTP parameters (e.g. `static constexpr T value = v;`)
  // are evaluated correctly.
  NttpTextBindings struct_nttp_bindings_by_text;
  if (sd->n_template_args > 0 && sd->n_template_params > 0) {
    for (int pi = 0; pi < sd->n_template_params && pi < sd->n_template_args; ++pi) {
      const char* pname = sd->template_param_names[pi];
      if (sd->template_param_is_nttp[pi]) {
        struct_nttp_bindings[pname] = sd->template_arg_values[pi];
        if (sd->template_param_name_text_ids &&
            sd->template_param_name_text_ids[pi] != kInvalidText) {
          struct_nttp_bindings_by_text[sd->template_param_name_text_ids[pi]] =
              sd->template_arg_values[pi];
        }
      }
    }
  }
  LowererConstEvalStructuredMaps static_member_consteval_maps;
  refresh_global_consteval_structured_maps(static_member_consteval_maps);
  // Static member initializers are evaluated against global enum mirrors and
  // rendered compatibility only. Local/block enum authority requires scoped
  // lifetime metadata, which this layout path does not carry.
  StaticEvalIntEnumLookupInput static_member_enum_lookup;
  static_member_enum_lookup.rendered_enum_consts = &enum_consts_;
  static_member_enum_lookup.enum_consts_by_key =
      &static_member_consteval_maps.enum_consts_by_key;

  for (int i = 0; i < num_fields; ++i) {
    const Node* f = get_field(i);
    if (!f) continue;
    // Struct methods live in sd->children[] alongside fields for some parser
    // paths, but they are not data members and must not participate in
    // layout. They are collected separately after the layout pass.
    if (f->kind == NK_FUNCTION) continue;
    std::optional<long long> static_const_value;
    if (f->is_static && f->is_constexpr && f->name && f->name[0] && f->init) {
      static_const_value =
          struct_nttp_bindings.empty()
              ? static_eval_int(f->init, static_member_enum_lookup)
              : eval_const_int_with_nttp_bindings(
                    f->init, struct_nttp_bindings,
                    struct_nttp_bindings_by_text.empty()
                        ? nullptr
                        : &struct_nttp_bindings_by_text,
                    &static_member_consteval_maps.enum_consts_by_key);
    }
    if (f->name && f->name[0]) {
      struct_static_member_decls_[tag][f->name] = f;
      if (has_struct_owner_key) {
        register_struct_static_member_owner_lookup(
            struct_owner_key, f, static_const_value);
      }
    }
    if (static_const_value) {
      struct_static_member_const_values_[tag][f->name] = *static_const_value;
    }
    if (f->is_static)
      continue;

    const int bit_width = static_cast<int>(f->ival);  // -1 = not bitfield, 0+ = bitfield width
    const bool is_bitfield = (bit_width >= 0);

    // Skip anonymous non-bitfield fields (they have no name) but keep named fields
    // and bitfield fields (including anonymous bitfields for layout purposes).
    if (!f->name && !is_bitfield) continue;
    // Zero-width bitfield: force alignment, close current storage unit
    if (is_bitfield && bit_width == 0) {
      if (!sd->is_union && bf_unit_start_bit >= 0) {
        bf_unit_start_bit = -1;
        bf_current_bit = 0;
      }
      continue;
    }
    // Anonymous bitfields with width > 0 but no name: skip as field but advance bit position
    if (!f->name && is_bitfield) {
      if (!sd->is_union && bf_unit_start_bit >= 0) {
        bf_current_bit += bit_width;
        if (bf_current_bit > bf_unit_bits) {
          // Doesn't fit, start new unit
          bf_unit_start_bit = -1;
          bf_current_bit = 0;
        }
      }
      continue;
    }

    HirStructField hf;
    hf.name = f->name;
    hf.field_text_id = make_text_id(
        f->name, module_ ? module_->link_name_texts.get() : nullptr);
    hf.member_symbol_id = module_->member_symbols.intern(
        std::string(tag) + "::" + f->name);
    if (has_struct_owner_key) {
      register_struct_member_symbol_owner_lookup(
          struct_owner_key, hf.field_text_id, hf.member_symbol_id);
    }
    TypeSpec ft = f->type;
    if (resolved_types_) {
      if (auto ct = resolved_types_->lookup(f)) {
        TypeSpec resolved_ft = sema::typespec_from_canonical(*ct);
        if (resolved_ft.base != TB_VOID || ft.base == TB_VOID) {
          ft = resolved_ft;
        }
      }
    }
    auto apply_field_template_bindings =
        [&](TypeSpec& target, const auto& self) -> void {
          apply_template_typedef_bindings(
              target, struct_tpl_bindings, struct_tpl_bindings_by_text, sd);
          if (!target.tpl_struct_args.data || target.tpl_struct_args.size <= 0) {
            return;
          }
          for (int ai = 0; ai < target.tpl_struct_args.size; ++ai) {
            TemplateArgRef& arg = target.tpl_struct_args.data[ai];
            if (arg.kind != TemplateArgKind::Type) continue;
            self(arg.type, self);
            const std::string debug_text = encode_template_type_arg_ref_hir(arg.type);
            if (!(arg.type.base == TB_VOID && arg.debug_text &&
                  arg.debug_text[0] &&
                  std::strcmp(arg.debug_text, "void") != 0)) {
              arg.debug_text =
                  debug_text.empty() ? nullptr : ::strdup(debug_text.c_str());
            }
          }
        };
    apply_field_template_bindings(ft, apply_field_template_bindings);
    if (ft.base == TB_TYPEDEF && ft.tag_text_id != kInvalidText &&
        sd->member_typedef_text_ids && sd->member_typedef_types) {
      for (int mi = 0; mi < sd->n_member_typedefs; ++mi) {
        if (sd->member_typedef_text_ids[mi] != ft.tag_text_id) continue;
        ft = sd->member_typedef_types[mi];
        apply_field_template_bindings(ft, apply_field_template_bindings);
        break;
      }
    }
    if (ft.deferred_member_type_name) {
      seed_and_resolve_pending_template_type_if_needed(
          ft, struct_tpl_bindings, struct_nttp_bindings, f,
          PendingTemplateTypeKind::MemberTypedef,
          std::string("struct-field-member:") + tag);
      while (resolve_struct_member_typedef_if_ready(&ft)) {
      }
    }
    auto realize_field_template_structs =
        [&](TypeSpec& target, const auto& self) -> void {
          if (!target.tpl_struct_origin &&
              target.tpl_struct_origin_key.base_text_id != kInvalidText) {
            if (const Node* primary =
                    canonical_template_struct_primary(target, nullptr)) {
              if (primary->template_origin_name &&
                  primary->template_origin_name[0]) {
                target.tpl_struct_origin = primary->template_origin_name;
              } else if (primary->name && primary->name[0]) {
                target.tpl_struct_origin = primary->name;
              } else if (primary->unqualified_name &&
                         primary->unqualified_name[0]) {
                target.tpl_struct_origin = primary->unqualified_name;
              }
            }
          }
          if (target.tpl_struct_args.data && target.tpl_struct_args.size > 0) {
            for (int ai = 0; ai < target.tpl_struct_args.size; ++ai) {
              TemplateArgRef& arg = target.tpl_struct_args.data[ai];
              if (arg.kind != TemplateArgKind::Type) continue;
              self(arg.type, self);
              const std::string debug_text =
                  encode_template_type_arg_ref_hir(arg.type);
              arg.debug_text =
                  debug_text.empty() ? nullptr : ::strdup(debug_text.c_str());
            }
          }
          if (!target.tpl_struct_origin &&
              target.tpl_struct_origin_key.base_text_id == kInvalidText) {
            return;
          }
          if (target.tpl_struct_args.data && target.tpl_struct_args.size > 0) {
            for (int ai = 0; ai < target.tpl_struct_args.size; ++ai) {
              const TemplateArgRef& arg = target.tpl_struct_args.data[ai];
              if (arg.kind != TemplateArgKind::Type) continue;
              if (!has_concrete_type(arg.type) && !arg.type.tpl_struct_origin &&
                  arg.type.tpl_struct_origin_key.base_text_id == kInvalidText) {
                return;
              }
            }
          }
          realize_template_struct_if_needed(
              target, struct_tpl_bindings, struct_nttp_bindings);
          resolve_typedef_to_struct(target);
        };
    realize_field_template_structs(ft, realize_field_template_structs);
    if (ft.base == TB_STRUCT || ft.base == TB_UNION) {
      auto apply_record_owner_key = [&](const HirRecordOwnerKey& owner_key,
                                        const Node* owner_node) {
        ft.tag_text_id = owner_key.declaration_text_id;
        ft.namespace_context_id = owner_key.namespace_context_id;
        ft.is_global_qualified = owner_key.is_global_qualified;
        if (owner_node) ft.record_def = const_cast<Node*>(owner_node);
      };
      if (ft.record_def) {
        const auto owner_it = struct_def_owner_by_node_.find(ft.record_def);
        if (owner_it != struct_def_owner_by_node_.end()) {
          apply_record_owner_key(owner_it->second, ft.record_def);
        }
      } else if (ft.tag_text_id != kInvalidText) {
        const HirRecordOwnerKey* matched_key = nullptr;
        const Node* matched_node = nullptr;
        for (const auto& [candidate_node, owner_key] :
             struct_def_owner_by_node_) {
          if (!candidate_node ||
              candidate_node->unqualified_text_id != ft.tag_text_id) {
            continue;
          }
          if (ft.namespace_context_id >= 0 &&
              candidate_node->namespace_context_id !=
                  ft.namespace_context_id) {
            continue;
          }
          if (matched_key && *matched_key != owner_key) {
            matched_key = nullptr;
            matched_node = nullptr;
            break;
          }
          matched_key = &owner_key;
          matched_node = candidate_node;
        }
        if (matched_key) apply_record_owner_key(*matched_key, matched_node);
      }
    }

    if (is_bitfield && !sd->is_union) {
      // Determine signedness from original declared type
      const bool bf_signed = (ft.base == TB_INT || ft.base == TB_CHAR ||
                              ft.base == TB_SCHAR || ft.base == TB_SHORT ||
                              ft.base == TB_LONG || ft.base == TB_LONGLONG ||
                              ft.base == TB_INT128);
      // Determine storage unit size from declared type
      int decl_unit_bits = static_cast<int>(sizeof_base(ft.base) * 8);
      if (decl_unit_bits < 8) decl_unit_bits = 8;

      // Can we pack into current storage unit?
      bool can_pack = (bf_unit_start_bit >= 0) &&
                      (bf_current_bit + bit_width <= bf_unit_bits);

      if (can_pack) {
        hf.bit_offset = bf_current_bit;
        hf.storage_unit_bits = bf_unit_bits;
        hf.bit_width = bit_width;
        hf.llvm_idx = llvm_idx - 1;  // same LLVM field as previous
      } else {
        // Start new storage unit
        bf_unit_start_bit = 0;
        bf_unit_bits = decl_unit_bits;
        bf_current_bit = 0;
        hf.bit_offset = 0;
        hf.storage_unit_bits = decl_unit_bits;
        hf.bit_width = bit_width;
        hf.llvm_idx = llvm_idx;
        ++llvm_idx;
      }
      bf_current_bit = hf.bit_offset + bit_width;
      hf.is_bf_signed = bf_signed;

      // Set elem_type to the storage unit's integer type
      TypeSpec sft{};
      switch (hf.storage_unit_bits) {
        case 8:  sft.base = TB_UCHAR; break;
        case 16: sft.base = TB_USHORT; break;
        case 32: sft.base = TB_UINT; break;
        case 64: sft.base = TB_ULONGLONG; break;
        default: sft.base = TB_UINT; break;
      }
      hf.elem_type = sft;
      hf.is_anon_member = f->is_anon_field;
      def.fields.push_back(std::move(hf));
      continue;
    }

    if (is_bitfield && sd->is_union) {
      const bool bf_signed = (ft.base == TB_INT || ft.base == TB_CHAR ||
                              ft.base == TB_SCHAR || ft.base == TB_SHORT ||
                              ft.base == TB_LONG || ft.base == TB_LONGLONG ||
                              ft.base == TB_INT128);
      int decl_unit_bits = static_cast<int>(sizeof_base(ft.base) * 8);
      if (decl_unit_bits < 8) decl_unit_bits = 8;
      hf.bit_width = bit_width;
      hf.bit_offset = 0;
      hf.storage_unit_bits = decl_unit_bits;
      hf.is_bf_signed = bf_signed;
      hf.llvm_idx = 0;
      TypeSpec sft{};
      switch (hf.storage_unit_bits) {
        case 8:  sft.base = TB_UCHAR; break;
        case 16: sft.base = TB_USHORT; break;
        case 32: sft.base = TB_UINT; break;
        case 64: sft.base = TB_ULONGLONG; break;
        default: sft.base = TB_UINT; break;
      }
      hf.elem_type = sft;
      hf.is_anon_member = f->is_anon_field;
      def.fields.push_back(std::move(hf));
      continue;
    }

    // Non-bitfield: flush any open bitfield storage unit
    if (!sd->is_union && bf_unit_start_bit >= 0) {
      bf_unit_start_bit = -1;
      bf_current_bit = 0;
    }

    // Extract first array dimension (keep base element type for LLVM)
    if (ft.array_rank > 0) {
      hf.is_flexible_array = (ft.array_size < 0);
      // Flexible array member is represented as zero-length for the nominal
      // struct layout, while remaining explicit in HIR metadata.
      hf.array_first_dim = (ft.array_size >= 0) ? ft.array_size : 0;
      for (int i = 0; i + 1 < ft.array_rank; ++i) ft.array_dims[i] = ft.array_dims[i + 1];
      if (ft.array_rank > 0) ft.array_dims[ft.array_rank - 1] = -1;
      --ft.array_rank;
      ft.array_size = (ft.array_rank > 0) ? ft.array_dims[0] : -1;
    }
    hf.elem_type = ft;
    hf.align_bytes = ft.align_bytes > 0 ? ft.align_bytes : 0;
    hf.is_anon_member = f->is_anon_field;
    hf.llvm_idx = sd->is_union ? 0 : llvm_idx;
    // Phase C: capture fn_ptr signature for callable struct fields.
    // Use canonical type directly (struct fields aren't in ResolvedTypeTable).
    {
      auto ct = std::make_shared<sema::CanonicalType>(sema::canonicalize_declarator_type(f));
      if (sema::is_callable_type(*ct)) {
        const auto* fsig = sema::get_function_sig(*ct);
        if (fsig) {
          FnPtrSig sig{};
          sig.canonical_sig = ct;
          if (fsig->return_type)
            sig.return_type = qtype_from(sema::typespec_from_canonical(*fsig->return_type));
          sig.variadic = fsig->is_variadic;
          sig.unspecified_params = fsig->unspecified_params;
          for (const auto& param : fsig->params)
            sig.params.push_back(qtype_from(sema::typespec_from_canonical(param), ValueCategory::LValue));
          hf.fn_ptr_sig = std::move(sig);
        }
      }
    }
    def.fields.push_back(std::move(hf));
    if (!sd->is_union) ++llvm_idx;
  }

  compute_struct_layout(module_, def);

  const bool append_struct_def_order = find_existing_struct_def() == nullptr;
  module_->index_struct_def_owner(def, append_struct_def_order);
  if (append_struct_def_order)
    module_->struct_def_order.push_back(tag);
  module_->struct_defs[tag] = std::move(def);

  // Collect struct methods (stored in sd->children[]) for later lowering.
  // If the struct was parser-instantiated from a template, extract the
  // template bindings so method bodies can resolve pending template types.
  TypeBindings method_tpl_bindings;
  NttpBindings method_nttp_bindings;
  NttpTextBindings method_nttp_bindings_by_text;
  if (sd->n_template_args > 0 && sd->template_param_names) {
    for (int i = 0; i < sd->n_template_args; ++i) {
      const char* pname = sd->template_param_names[i];
      if (!pname) continue;
      if (sd->template_param_is_nttp && sd->template_param_is_nttp[i]) {
        method_nttp_bindings[pname] = sd->template_arg_values[i];
        if (sd->template_param_name_text_ids &&
            sd->template_param_name_text_ids[i] != kInvalidText) {
          method_nttp_bindings_by_text[sd->template_param_name_text_ids[i]] =
              sd->template_arg_values[i];
        }
      } else if (sd->template_arg_types) {
        method_tpl_bindings[pname] = sd->template_arg_types[i];
      }
    }
  }
  for (int i = 0; i < sd->n_children; ++i) {
    const Node* method = sd->children[i];
    if (!method || method->kind != NK_FUNCTION || !method->name) continue;
    const char* const_suffix = method->is_const_method ? "_const" : "";
    // Constructors get special handling: stored in struct_constructors_
    // with unique mangled names to support overloading by parameter types.
    if (method->is_constructor) {
      if (sd->n_template_args > 0 && sd->template_origin_name &&
          method->n_template_params == 0) {
        continue;
      }
      auto& ctors = struct_constructors_[tag];
      int ctor_idx = static_cast<int>(ctors.size());
      std::string mangled = std::string(tag) + "__" + method->name;
      if (ctor_idx > 0) mangled += "__" + std::to_string(ctor_idx);
      ctors.push_back({mangled, method, method_tpl_bindings,
                       method_nttp_bindings, method_nttp_bindings_by_text});
      // Also register in struct_methods_ so the first ctor is findable.
      if (ctor_idx == 0) {
        std::string key = std::string(tag) + "::" + method->name;
        struct_methods_[key] = mangled;
        struct_method_link_name_ids_[key] = module_->link_names.intern(mangled);
        struct_method_ret_types_[key] = method->type;
        if (has_struct_owner_key) {
          register_struct_method_owner_lookup(
              struct_owner_key, method, false, key, mangled, method->type);
        }
      }
      pending_methods_.push_back({mangled, std::string(tag), method,
                                  method_tpl_bindings, method_nttp_bindings,
                                  method_nttp_bindings_by_text});
      continue;
    }
    if (method->is_destructor) {
      std::string mangled = std::string(tag) + "__dtor";
      struct_destructors_[tag] = {mangled, method};
      pending_methods_.push_back({mangled, std::string(tag), method,
                                  method_tpl_bindings, method_nttp_bindings,
                                  method_nttp_bindings_by_text});
      continue;
    }
    std::string mangled = std::string(tag) + "__" + method->name + const_suffix;
    std::string key = std::string(tag) + "::" + method->name + const_suffix;
    // Detect ref-overloaded methods: if this key already exists, check for
    // ref-qualifier difference and register in the overload set.
    if (struct_methods_.count(key)) {
      // This is a second overload of the same method -- mangle differently.
      mangled += "__rref";
      // Register both in the ref_overload_set_ for call-site resolution.
      auto& ovset = ref_overload_set_[key];
      if (ovset.empty()) {
        // Register the first overload.
        RefOverloadEntry e0;
        e0.mangled_name = struct_methods_[key];
        // Find the first method's params from pending_methods_.
        for (const auto& pm : pending_methods_) {
          if (pm.mangled == struct_methods_[key]) {
            e0.method_is_lvalue_ref = pm.method_node->is_lvalue_ref_method;
            e0.method_is_rvalue_ref = pm.method_node->is_rvalue_ref_method;
            for (int pi = 0; pi < pm.method_node->n_params; ++pi) {
              e0.param_is_rvalue_ref.push_back(pm.method_node->params[pi]->type.is_rvalue_ref);
              e0.param_is_lvalue_ref.push_back(pm.method_node->params[pi]->type.is_lvalue_ref);
            }
            break;
          }
        }
        ovset.push_back(std::move(e0));
      }
      RefOverloadEntry e1;
      e1.mangled_name = mangled;
      e1.method_is_lvalue_ref = method->is_lvalue_ref_method;
      e1.method_is_rvalue_ref = method->is_rvalue_ref_method;
      for (int pi = 0; pi < method->n_params; ++pi) {
        e1.param_is_rvalue_ref.push_back(method->params[pi]->type.is_rvalue_ref);
        e1.param_is_lvalue_ref.push_back(method->params[pi]->type.is_lvalue_ref);
      }
      ovset.push_back(std::move(e1));
    }
    struct_methods_[key] = mangled;
    struct_method_link_name_ids_[key] = module_->link_names.intern(mangled);
    struct_method_ret_types_[key] = method->type;
    if (has_struct_owner_key) {
      register_struct_method_owner_lookup(
          struct_owner_key, method, method->is_const_method, key, mangled, method->type);
    }
    pending_methods_.push_back({mangled, std::string(tag), method,
                                method_tpl_bindings, method_nttp_bindings,
                                method_nttp_bindings_by_text});
  }
}

void Lowerer::lower_global(const Node* gv,
                           const std::string* name_override,
                           const TypeBindings* tpl_override,
                           const NttpBindings* nttp_override,
                           const NttpTextBindings* nttp_text_override) {
  GlobalInit computed_init{};
  bool has_init = false;
  const bool has_tpl_overrides =
      tpl_override != nullptr || nttp_override != nullptr;
  FunctionCtx init_ctx{};
  if (nttp_override) init_ctx.nttp_bindings = *nttp_override;
  if (nttp_text_override) init_ctx.nttp_bindings_by_text = *nttp_text_override;
  if (tpl_override) {
    init_ctx.tpl_bindings = *tpl_override;
    populate_template_type_text_bindings(init_ctx, gv, tpl_override);
  }
  populate_structured_template_binding_mirrors(
      init_ctx, gv, tpl_override, nttp_override);
  if (tpl_override) {
    for (auto& [name, bound_ts] : init_ctx.tpl_bindings) {
      (void)name;
      seed_and_resolve_pending_template_type_if_needed(
          bound_ts, *tpl_override,
          nttp_override ? *nttp_override : NttpBindings{}, gv,
          PendingTemplateTypeKind::DeclarationType,
          "template-global-binding");
      while (resolve_struct_member_typedef_if_ready(&bound_ts)) {
      }
      resolve_typedef_to_struct(bound_ts);
    }
  }

  // Handle compound literal at global scope: `T *p = &(T){...};`
  // The compound literal must be lowered to a separate static global, and
  // the parent global initialized to point at it.
  if (gv->init && gv->init->kind == NK_ADDR &&
      gv->init->left && gv->init->left->kind == NK_COMPOUND_LIT) {
    ExprId addr_id = hoist_compound_literal_to_global(gv->init, gv->init->left);
    InitScalar sc{};
    sc.expr = addr_id;
    computed_init = sc;
    has_init = true;
  }

  // For init lists that may contain nested &(compound_lit) items, lower the
  // init BEFORE allocating this global's id so that compound-literal globals
  // are created first (their DeclRef exprs need valid GlobalIds).
  GlobalInit early_init{};
  bool early_init_done = false;
  if (!has_init && gv->init) {
    early_init = lower_global_init(
        gv->init, has_tpl_overrides ? &init_ctx : nullptr,
        gv->type.is_const || gv->is_constexpr);
    early_init_done = true;
  }

  GlobalVar g{};
  g.id = next_global_id();
  g.name = name_override ? *name_override : (gv->name ? gv->name : "<anon_global>");
  g.name_text_id = make_unqualified_text_id(
      gv, module_ ? module_->link_name_texts.get() : nullptr);
  g.link_name_id = module_->link_names.intern(g.name);
  g.ns_qual = make_ns_qual(gv, module_ ? module_->link_name_texts.get() : nullptr);
  {
    TypeSpec global_ts = gv->type;
    seed_and_resolve_pending_template_type_if_needed(
        global_ts, tpl_override ? *tpl_override : TypeBindings{},
        nttp_override ? *nttp_override : NttpBindings{}, gv,
        PendingTemplateTypeKind::DeclarationType,
        std::string("global-decl:") + g.name);
    resolve_typedef_to_struct(global_ts);
    g.type = qtype_from(global_ts, ValueCategory::LValue);
  }
  g.fn_ptr_sig = fn_ptr_sig_from_decl_node(gv);
  g.linkage = {gv->is_static, gv->is_extern, false, weak_symbols_.count(g.name) > 0,
               static_cast<Visibility>(gv->visibility)};
  g.execution_domain = gv->execution_domain;
  g.is_const = gv->type.is_const || gv->is_constexpr;
  g.span = make_span(gv);

  // Deduce unsized array dimension from wide string literal initializer.
  if (gv->init && g.type.spec.array_rank > 0 && g.type.spec.array_size < 0 &&
      gv->init->kind == NK_STR_LIT && gv->init->sval && gv->init->sval[0] == 'L') {
    const auto vals = decode_string_literal_values(gv->init->sval, true);
    g.type.spec.array_size = static_cast<long long>(vals.size());
    g.type.spec.array_dims[0] = g.type.spec.array_size;
  }

  if (has_init) {
    g.init = computed_init;
  } else if (early_init_done) {
    g.init = early_init;
    g.type.spec = resolve_array_ts(g.type.spec, g.init);
    g.init = normalize_global_init(g.type.spec, g.init);
  }

  if (g.is_const || gv->is_constexpr) {
    if (const auto* scalar = std::get_if<InitScalar>(&g.init)) {
      const Expr& e = module_->expr_pool[scalar->expr.value];
      if (const auto* lit = std::get_if<IntLiteral>(&e.payload)) {
        const_int_bindings_[g.name] = lit->value;
        if (auto key = make_global_const_int_value_binding_key(gv)) {
          ct_state_->register_const_int_binding(*key, g.name, lit->value);
        } else {
          ct_state_->register_const_int_binding(g.name, lit->value);
        }
      } else if (const auto* ch = std::get_if<CharLiteral>(&e.payload)) {
        const_int_bindings_[g.name] = ch->value;
        if (auto key = make_global_const_int_value_binding_key(gv)) {
          ct_state_->register_const_int_binding(*key, g.name, ch->value);
        } else {
          ct_state_->register_const_int_binding(g.name, ch->value);
        }
      }
    }
  }

  module_->index_global_decl(g);
  module_->globals.push_back(std::move(g));
}

TypeSpec Lowerer::infer_generic_ctrl_type(FunctionCtx* ctx, const Node* n) {
  if (!n) return {};
  auto find_template_type_binding = [&](const TypeSpec& ts) -> const TypeSpec* {
    if (!ctx) return nullptr;
    return find_template_type_binding_for_call(
        &ctx->tpl_bindings, &ctx->structured_tpl_bindings,
        &ctx->tpl_bindings_by_text, module_, ts);
  };
  auto apply_template_type_binding = [&](TypeSpec& target) -> bool {
    const TypeSpec* concrete = find_template_type_binding(target);
    if (!concrete) return false;
    const int outer_ptr_level = target.ptr_level;
    const bool outer_lref = target.is_lvalue_ref;
    const bool outer_rref = target.is_rvalue_ref;
    const bool outer_const = target.is_const;
    const bool outer_volatile = target.is_volatile;

    target = *concrete;
    target.ptr_level += outer_ptr_level;
    target.is_lvalue_ref = target.is_lvalue_ref || outer_lref;
    target.is_rvalue_ref =
        !target.is_lvalue_ref && (target.is_rvalue_ref || outer_rref);
    target.is_const = target.is_const || outer_const;
    target.is_volatile = target.is_volatile || outer_volatile;
    return true;
  };
  if (has_concrete_type(n->type)) {
    const bool needs_tpl_typedef_substitution =
        ctx && !ctx->tpl_bindings.empty() &&
        n->type.base == TB_TYPEDEF &&
        find_template_type_binding(n->type) != nullptr;
    const bool needs_pending_template_resolution =
        ctx && !ctx->tpl_bindings.empty() && n->type.tpl_struct_origin;
    if (!needs_tpl_typedef_substitution && !needs_pending_template_resolution)
      return n->type;
  }
  switch (n->kind) {
    case NK_INT_LIT:
      return infer_int_literal_type(n);
    case NK_FLOAT_LIT: {
      TypeSpec ts = n->type;
      if (!has_concrete_type(ts)) ts = classify_float_literal_type(const_cast<Node*>(n));
      return ts;
    }
    case NK_CHAR_LIT: {
      TypeSpec ts = n->type;
      if (!has_concrete_type(ts)) ts.base = TB_INT;
      return ts;
    }
    case NK_STR_LIT: {
      TypeSpec ts{};
      ts.base = TB_CHAR;
      ts.ptr_level = 1;
      return ts;
    }
    case NK_VAR: {
      const std::string name = n->name ? n->name : "";
      if (ctx && !name.empty()) {
        const std::optional<HirTemplateParameterBindingKey> query_key =
            make_nttp_binding_key_from_owner_text_id(
                ctx->template_binding_owner_node, n->unqualified_text_id);
        if (query_key
                ? lookup_nttp_binding(
                      ctx, n, name.c_str(), n->unqualified_text_id,
                      false /* allow_rendered_mirror_fallback */, &*query_key)
                : lookup_nttp_binding(ctx, n, name.c_str())) {
          TypeSpec ts{};
          ts.base = TB_INT;
          ts.array_size = -1;
          ts.inner_rank = -1;
          return ts;
        }
      }
      if (n->name && n->name[0] &&
          n->has_template_args && find_template_struct_primary(n->name)) {
        TypeSpec tmp_ts{};
        tmp_ts.base = TB_STRUCT;
        tmp_ts.array_size = -1;
        tmp_ts.inner_rank = -1;
        tmp_ts.tpl_struct_origin = n->name;
        const Node* primary_tpl = find_template_struct_primary(n->name);
        assign_template_arg_refs_from_ast_args(
            &tmp_ts, n, primary_tpl, ctx, n,
            PendingTemplateTypeKind::OwnerStruct,
            "generic-ctrl-type-var-arg");
        TypeBindings tpl_empty;
        NttpBindings nttp_empty;
        seed_and_resolve_pending_template_type_if_needed(
            tmp_ts,
            ctx ? ctx->tpl_bindings : tpl_empty,
            ctx ? ctx->nttp_bindings : nttp_empty,
            n, PendingTemplateTypeKind::OwnerStruct, "generic-ctrl-type-var",
            primary_tpl);
        if (find_struct_def_for_layout_type(tmp_ts)) return tmp_ts;
      }
      if (ctx) {
        if (n->unqualified_text_id != kInvalidText) {
          auto lit = ctx->local_ids_by_text_id.find(n->unqualified_text_id);
          if (lit != ctx->local_ids_by_text_id.end()) {
            if (ctx->local_types.contains(lit->second)) {
              return reference_value_ts(ctx->local_types.at(lit->second));
            }
          } else if (ctx->rendered_compat_local_text_ids.find(
                         n->unqualified_text_id) !=
                         ctx->rendered_compat_local_text_ids.end() ||
                     ctx->rendered_compat_local_names.find(name) !=
                         ctx->rendered_compat_local_names.end()) {
            auto rendered_lit = ctx->locals.find(name);
            if (rendered_lit != ctx->locals.end() &&
                ctx->local_types.contains(rendered_lit->second)) {
              return reference_value_ts(
                  ctx->local_types.at(rendered_lit->second));
            }
          }
        } else {
          auto lit = ctx->locals.find(name);
          if (lit != ctx->locals.end()) {
            if (ctx->local_types.contains(lit->second)) {
              return reference_value_ts(ctx->local_types.at(lit->second));
            }
          }
        }
        if (n->unqualified_text_id != kInvalidText) {
          auto pit = ctx->param_indices_by_text_id.find(n->unqualified_text_id);
          if (pit != ctx->param_indices_by_text_id.end() && ctx->fn &&
              pit->second < ctx->fn->params.size()) {
            return reference_value_ts(ctx->fn->params[pit->second].type.spec);
          } else if (ctx->rendered_compat_param_text_ids.find(
                         n->unqualified_text_id) !=
                         ctx->rendered_compat_param_text_ids.end() ||
                     ctx->rendered_compat_param_names.find(name) !=
                         ctx->rendered_compat_param_names.end()) {
            auto rendered_pit = ctx->params.find(name);
            if (rendered_pit != ctx->params.end() && ctx->fn &&
                rendered_pit->second < ctx->fn->params.size()) {
              return reference_value_ts(
                  ctx->fn->params[rendered_pit->second].type.spec);
            }
          }
        } else {
          auto pit = ctx->params.find(name);
          if (pit != ctx->params.end() && ctx->fn &&
              pit->second < ctx->fn->params.size()) {
            return reference_value_ts(ctx->fn->params[pit->second].type.spec);
          }
        }
        if (const std::optional<GlobalId> static_global =
                lookup_static_global_bridge(*ctx, n, name)) {
          if (const GlobalVar* gv = module_->find_global(*static_global)) {
            return reference_value_ts(gv->type.spec);
          }
        }
      }
      DeclRef global_ref = make_global_lookup_decl_ref(*module_, name, n);
      if (const GlobalVar* gv = module_->resolve_global_decl(global_ref)) {
        return reference_value_ts(gv->type.spec);
      }
      DeclRef fn_ref = make_function_lookup_decl_ref(*module_, name, n);
      if (const Function* fn = module_->resolve_function_decl(fn_ref)) {
        TypeSpec ts = fn->return_type.spec;
        ts.is_fn_ptr = true;
        ts.ptr_level = 0;
        ts.array_rank = 0;
        ts.array_size = -1;
        return ts;
      }
      return n->type;
    }
    case NK_ADDR: {
      TypeSpec ts = infer_generic_ctrl_type(ctx, n->left);
      if (ts.array_rank > 0 && !is_vector_ty(ts)) {
        ts.array_rank = 0;
        ts.array_size = -1;
      }
      ts.ptr_level += 1;
      return ts;
    }
    case NK_DEREF: {
      TypeSpec ts = infer_generic_ctrl_type(ctx, n->left);
      if (ts.ptr_level == 0 && ts.base == TB_STRUCT) {
        const TypeBindings* tpl_bindings = ctx ? &ctx->tpl_bindings : nullptr;
        const NttpBindings* nttp_bindings = ctx ? &ctx->nttp_bindings : nullptr;
        const std::string* current_struct_tag =
            (ctx && !ctx->method_struct_tag.empty()) ? &ctx->method_struct_tag : nullptr;
        const std::string owner_tag = resolve_struct_method_lookup_owner_tag(
            ts, false, tpl_bindings, nttp_bindings, current_struct_tag, n,
            "generic-ctrl-type-deref");
        if (auto ret =
                find_struct_method_return_type(owner_tag, "operator_deref", false)) {
          return *ret;
        }
      }
      if (ts.ptr_level > 0) ts.ptr_level -= 1;
      else if (ts.array_rank > 0) ts.array_rank -= 1;
      return ts;
    }
    case NK_MEMBER: {
      TypeSpec base_ts = infer_generic_ctrl_type(ctx, n->left);
      if (n->is_arrow && base_ts.ptr_level > 0) base_ts.ptr_level -= 1;
      if ((base_ts.base == TB_STRUCT || base_ts.base == TB_UNION) &&
          base_ts.ptr_level == 0) {
        if (const HirStructDef* layout = find_struct_def_for_layout_type(base_ts)) {
          for (const auto& f : layout->fields) {
            if (f.name == (n->name ? n->name : "")) return field_type_of(f);
          }
        }
      }
      return n->type;
    }
    case NK_INDEX: {
      TypeSpec ts = infer_generic_ctrl_type(ctx, n->left);
      if (ts.ptr_level > 0) ts.ptr_level -= 1;
      else if (is_vector_ty(ts)) return vector_element_type(ts);
      else if (ts.array_rank > 0) {
        ts.array_rank -= 1;
        ts.array_size = (ts.array_rank > 0) ? ts.array_dims[0] : -1;
      }
      return ts;
    }
    case NK_CAST:
    case NK_COMPOUND_LIT: {
      TypeSpec ts = n->type;
      if (ctx && !ctx->tpl_bindings.empty() &&
          ts.base == TB_TYPEDEF) {
        apply_template_type_binding(ts);
      }
      if (ctx && !ctx->tpl_bindings.empty() && ts.tpl_struct_origin) {
        seed_and_resolve_pending_template_type_if_needed(
            ts, ctx->tpl_bindings, ctx->nttp_bindings, n,
            PendingTemplateTypeKind::DeclarationType, "generic-ctrl-type");
      }
      return ts;
    }
    case NK_BINOP: {
      const TypeSpec l = infer_generic_ctrl_type(ctx, n->left);
      const TypeSpec r = infer_generic_ctrl_type(ctx, n->right);
      if (is_vector_ty(l)) return l;
      if (is_vector_ty(r)) return r;
      const bool ptr_l = l.ptr_level > 0 || l.array_rank > 0;
      const bool ptr_r = r.ptr_level > 0 || r.array_rank > 0;
      if (n->op && n->op[0] && !n->op[1]) {
        const char op = n->op[0];
        if ((op == '+' || op == '-') && ptr_l && !ptr_r) return normalize_generic_type(l);
        if (op == '+' && ptr_r && !ptr_l) return normalize_generic_type(r);
      }
      if (l.base == TB_LONGLONG || l.base == TB_ULONGLONG ||
          r.base == TB_LONGLONG || r.base == TB_ULONGLONG) {
        TypeSpec ts{};
        ts.base = (l.base == TB_ULONGLONG || r.base == TB_ULONGLONG)
                      ? TB_ULONGLONG
                      : TB_LONGLONG;
        return ts;
      }
      if (l.base == TB_LONG || l.base == TB_ULONG || r.base == TB_LONG || r.base == TB_ULONG) {
        TypeSpec ts{};
        ts.base = (l.base == TB_ULONG || r.base == TB_ULONG) ? TB_ULONG : TB_LONG;
        return ts;
      }
      if (l.base == TB_DOUBLE || r.base == TB_DOUBLE) {
        TypeSpec ts{};
        ts.base = TB_DOUBLE;
        return ts;
      }
      if (l.base == TB_FLOAT || r.base == TB_FLOAT) {
        TypeSpec ts{};
        ts.base = TB_FLOAT;
        return ts;
      }
      TypeSpec ts{};
      ts.base = TB_INT;
      return ts;
    }
    case NK_CALL:
    case NK_BUILTIN_CALL: {
      if (n->left && n->left->kind == NK_VAR && n->left->name &&
          n->n_children == 0 && n->left->has_template_args &&
          find_template_struct_primary(n->left->name)) {
        TypeSpec callee_ts = infer_generic_ctrl_type(ctx, n->left);
        if (callee_ts.base == TB_STRUCT) return callee_ts;
      }
      if (n->left && n->left->kind == NK_VAR && n->left->name) {
        auto sit = module_->struct_defs.find(n->left->name);
        if (sit != module_->struct_defs.end()) {
          TypeSpec ts{};
          ts.base = TB_STRUCT;
          set_typespec_final_spelling_tag_if_present(
              ts, sit->second.tag.c_str(), 0);
          ts.tag_text_id = sit->second.tag_text_id;
          ts.namespace_context_id = sit->second.ns_qual.context_id;
          ts.is_global_qualified = sit->second.ns_qual.is_global_qualified;
          return ts;
        }
      }
      if (n->left && n->left->kind == NK_VAR && n->left->name) {
        const std::string callee_name = n->left->name;
        auto dit = deduced_template_calls_.find(n);
        if (dit != deduced_template_calls_.end()) {
          LinkNameId link_name_id = kInvalidLinkName;
          if (const Node* tpl_fn = ct_state_->find_template_def(callee_name)) {
            const auto param_order =
                get_template_param_order(tpl_fn, &dit->second.bindings,
                                         &dit->second.nttp_bindings);
            const SpecializationKey spec_key =
                dit->second.nttp_bindings.empty()
                    ? make_specialization_key(callee_name, param_order,
                                              dit->second.bindings, tpl_fn)
                    : make_specialization_key(callee_name, param_order,
                                              dit->second.bindings,
                                              dit->second.nttp_bindings, tpl_fn);
            link_name_id = find_template_instantiation_link_name_id(
                *module_, callee_name, spec_key, nullptr);
          }
          DeclRef fn_ref = make_function_lookup_decl_ref(
              *module_, dit->second.mangled_name, nullptr, link_name_id);
          if (const Function* fn = module_->resolve_function_decl(fn_ref)) {
            return reference_value_ts(fn->return_type.spec);
          }
        }
        DeclRef fn_ref = make_function_lookup_decl_ref(*module_, callee_name, n->left);
        if (const Function* fn = module_->resolve_function_decl(fn_ref)) {
          return reference_value_ts(fn->return_type.spec);
        }
        TypeSpec callee_ts = infer_generic_ctrl_type(ctx, n->left);
        if (callee_ts.base == TB_STRUCT && callee_ts.ptr_level == 0) {
          const TypeBindings* tpl_bindings = ctx ? &ctx->tpl_bindings : nullptr;
          const NttpBindings* nttp_bindings = ctx ? &ctx->nttp_bindings : nullptr;
          const std::string* current_struct_tag =
              (ctx && !ctx->method_struct_tag.empty()) ? &ctx->method_struct_tag : nullptr;
          const std::string owner_tag = resolve_struct_method_lookup_owner_tag(
              callee_ts, false, tpl_bindings, nttp_bindings, current_struct_tag,
              n, "generic-ctrl-type-operator-call");
          if (auto rit = find_struct_method_return_type(owner_tag, "operator_call",
                                                        callee_ts.is_const)) {
            return reference_value_ts(*rit);
          }
        }
      }
      break;
    }
    default:
      break;
  }
  return n->type;
}

// Intentionally left out of this draft:
// - build_call_* / template deduction / template realization coordinators
// - collect_* / lower_initial_program / build_initial_hir

}  // namespace c4c::hir
