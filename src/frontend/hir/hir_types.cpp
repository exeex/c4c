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

DeclRef make_function_lookup_decl_ref(Module& module, std::string name,
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
    bool include_named_consts) const {
  refresh_global_consteval_structured_maps(maps);
  ConstEvalEnv env{&enum_consts_,
                   include_named_consts ? &const_int_bindings_ : nullptr,
                   local_consts};
  env.enum_consts_by_key = &maps.enum_consts_by_key;
  if (include_named_consts) env.named_consts_by_key = &maps.named_consts_by_key;
  if (module_) {
    env.struct_defs = &module_->struct_defs;
    env.struct_def_owner_index = &module_->struct_def_owner_index;
    env.link_name_texts = module_->link_name_texts.get();
  }
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
    const NttpTextBindings* nttp_bindings_by_text) const {
  if (!n) return 0;
  if (n->kind == NK_INT_LIT || n->kind == NK_CHAR_LIT) return n->ival;
  if (n->kind == NK_VAR && n->name) {
    if (nttp_bindings_by_text && n->unqualified_text_id != kInvalidText) {
      auto text_it = nttp_bindings_by_text->find(n->unqualified_text_id);
      if (text_it != nttp_bindings_by_text->end()) return text_it->second;
    }
    auto nttp_it = nttp_bindings.find(n->name);
    if (nttp_it != nttp_bindings.end()) return nttp_it->second;
    auto enum_it = enum_consts_.find(n->name);
    if (enum_it != enum_consts_.end()) return enum_it->second;
    return 0;
  }
  if (n->kind == NK_CAST && n->left) {
    long long v =
        eval_const_int_with_nttp_bindings(n->left, nttp_bindings, nttp_bindings_by_text);
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
          n->left, nttp_bindings, nttp_bindings_by_text);
    if (strcmp(n->op, "+") == 0)
      return eval_const_int_with_nttp_bindings(
          n->left, nttp_bindings, nttp_bindings_by_text);
    if (strcmp(n->op, "~") == 0)
      return ~eval_const_int_with_nttp_bindings(
          n->left, nttp_bindings, nttp_bindings_by_text);
  }
  if (n->kind == NK_BINOP && n->op && n->left && n->right) {
    long long l =
        eval_const_int_with_nttp_bindings(n->left, nttp_bindings, nttp_bindings_by_text);
    long long r =
        eval_const_int_with_nttp_bindings(n->right, nttp_bindings, nttp_bindings_by_text);
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
    const auto pit = ctx->param_fn_ptr_sigs.find(name);
    if (pit != ctx->param_fn_ptr_sigs.end()) return pit->second.return_type.spec;
    const auto lit = ctx->local_fn_ptr_sigs.find(name);
    if (lit != ctx->local_fn_ptr_sigs.end()) return lit->second.return_type.spec;
    const auto sit = ctx->static_globals.find(name);
    if (sit != ctx->static_globals.end()) {
      if (const GlobalVar* gv = module_->find_global(sit->second)) {
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

std::optional<TypeSpec> Lowerer::infer_call_result_type(
    const FunctionCtx* ctx, const Node* call) {
  if (!call || call->kind != NK_CALL || !call->left) return std::nullopt;

  if (auto dit = deduced_template_calls_.find(call);
      dit != deduced_template_calls_.end()) {
    DeclRef fn_ref = make_function_lookup_decl_ref(*module_, dit->second.mangled_name);
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
      TypeBindings bindings =
          merge_explicit_and_deduced_type_bindings(call, call->left, tpl_fn, enc);
      NttpBindings nttp_bindings =
          build_call_nttp_bindings(call->left, tpl_fn, enc_nttp, enc_nttp_by_text);
      std::string resolved_name =
          mangle_template_name(call->left->name, bindings, nttp_bindings);
      const auto param_order =
          get_template_param_order(tpl_fn, &bindings, &nttp_bindings);
      const SpecializationKey spec_key = nttp_bindings.empty()
          ? make_specialization_key(call->left->name, param_order, bindings)
          : make_specialization_key(call->left->name, param_order, bindings,
                                    nttp_bindings);
      if (const auto* inst_list = registry_.find_instances(call->left->name)) {
        for (const auto& inst : *inst_list) {
          if (inst.spec_key == spec_key) {
            resolved_name = inst.mangled_name;
            break;
          }
        }
      }
      DeclRef fn_ref = make_function_lookup_decl_ref(*module_, resolved_name);
      if (const Function* fn = module_->resolve_function_decl(fn_ref)) {
        return fn->return_type.spec;
      }
    }
  }

  return infer_call_result_type_from_callee(ctx, call->left);
}

std::optional<long long> Lowerer::lookup_nttp_binding(
    const FunctionCtx* ctx, const Node* name_node, const char* rendered_name) const {
  if (!ctx) return std::nullopt;
  const TextId text_id = name_node ? name_node->unqualified_text_id : kInvalidText;
  if (text_id != kInvalidText && !ctx->nttp_bindings_by_text.empty()) {
    auto text_it = ctx->nttp_bindings_by_text.find(text_id);
    if (text_it != ctx->nttp_bindings_by_text.end()) return text_it->second;
  }
  if (rendered_name && rendered_name[0]) {
    auto it = ctx->nttp_bindings.find(rendered_name);
    if (it != ctx->nttp_bindings.end()) return it->second;
  }
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
  }
  auto sit = struct_static_member_decls_.find(tag);
  if (sit != struct_static_member_decls_.end()) {
    auto mit = sit->second.find(member);
    if (mit != sit->second.end()) {
      record_struct_static_member_decl_lookup_parity(tag, member, mit->second);
      return mit->second;
    }
  }
  auto dit = module_->struct_defs.find(tag);
  if (dit != module_->struct_defs.end()) {
    for (const auto& base_tag : dit->second.base_tags) {
      if (const Node* from_base = find_struct_static_member_decl(base_tag, member))
        return from_base;
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
  if (owner_it == struct_static_member_decls_by_owner_.end()) return nullptr;
  if (rendered_tag && rendered_member) {
    auto sit = struct_static_member_decls_.find(*rendered_tag);
    if (sit != struct_static_member_decls_.end()) {
      auto mit = sit->second.find(*rendered_member);
      if (mit != sit->second.end()) {
        record_struct_static_member_decl_lookup_parity(
            *rendered_tag, *rendered_member, mit->second);
      }
    }
  }
  return owner_it->second;
}

std::optional<long long> Lowerer::find_struct_static_member_const_value(
    const std::string& tag, const std::string& member) const {
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

  const std::string* fallback_tag = rendered_tag;
  if (module_) {
    if (const std::string* owner_tag =
            module_->find_struct_def_tag_by_owner(key.owner_key)) {
      fallback_tag = owner_tag;
    }
  }

  std::string member_from_key;
  const std::string* fallback_member = rendered_member;
  if (module_ && module_->link_name_texts &&
      key.member_text_id != kInvalidText) {
    const std::string_view member_text =
        module_->link_name_texts->lookup(key.member_text_id);
    if (!member_text.empty()) {
      member_from_key = std::string(member_text);
      fallback_member = &member_from_key;
    }
  }

  if (fallback_tag && fallback_member) {
    return find_struct_static_member_const_value(*fallback_tag, *fallback_member);
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
    const std::string qname = n->name;
    const size_t scope_pos = qname.rfind("::");
    if (scope_pos != std::string::npos) {
      const std::string owner_name = qname.substr(0, scope_pos);
      const std::string member_name = qname.substr(scope_pos + 2);
      if (auto v = try_eval_template_static_member_const(
              ctx, owner_name, n, member_name)) {
        TypeSpec ts = n->type;
        if (ts.base == TB_VOID) ts.base = TB_INT;
        s.expr = append_expr(n, IntLiteral{*v, false}, ts);
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
  def.tag_text_id = make_unqualified_text_id(
      sd, module_ ? module_->link_name_texts.get() : nullptr);
  if (def.tag_text_id == kInvalidText) {
    def.tag_text_id = make_text_id(
        def.tag, module_ ? module_->link_name_texts.get() : nullptr);
  }
  def.ns_qual = make_ns_qual(sd, module_ ? module_->link_name_texts.get() : nullptr);
  const HirRecordOwnerKey struct_owner_key = make_hir_record_owner_key(def);
  const bool has_struct_owner_key =
      hir_record_owner_key_has_complete_metadata(struct_owner_key);
  if (has_struct_owner_key) {
    struct_def_owner_by_node_[sd] = struct_owner_key;
  }
  auto find_existing_struct_def = [&]() -> const HirStructDef* {
    if (has_struct_owner_key) {
      if (const HirStructDef* structured =
              module_->find_struct_def_by_owner_structured(struct_owner_key)) {
        return structured;
      }
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
    if (rendered_base_tag && !rendered_base_tag->empty() &&
        module_->struct_defs.count(*rendered_base_tag)) {
      base_tag = rendered_base_tag;
    }
    const HirStructDef* base_layout = find_struct_def_for_layout_type(base);
    if ((!base_tag || base_tag->empty()) && base_layout) {
      base_tag = base_layout->tag;
    }
    if ((!base_tag || base_tag->empty()) && base.record_def &&
        base.record_def->kind == NK_STRUCT_DEF && base.record_def->name &&
        base.record_def->name[0]) {
      base_tag = base.record_def->name;
    }
    if ((!base_tag || base_tag->empty()) && rendered_base_tag &&
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
  NttpBindings struct_nttp_bindings;
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
              ? static_eval_int(f->init, enum_consts_)
              : eval_const_int_with_nttp_bindings(
                    f->init, struct_nttp_bindings,
                    struct_nttp_bindings_by_text.empty()
                        ? nullptr
                        : &struct_nttp_bindings_by_text);
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
    if ((ft.base == TB_STRUCT || ft.base == TB_UNION) && ft.record_def) {
      const auto owner_it = struct_def_owner_by_node_.find(ft.record_def);
      if (owner_it != struct_def_owner_by_node_.end()) {
        const HirRecordOwnerKey& owner_key = owner_it->second;
        ft.tag_text_id = owner_key.declaration_text_id;
        ft.namespace_context_id = owner_key.namespace_context_id;
        ft.is_global_qualified = owner_key.is_global_qualified;
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
      auto& ctors = struct_constructors_[tag];
      int ctor_idx = static_cast<int>(ctors.size());
      std::string mangled = std::string(tag) + "__" + method->name;
      if (ctor_idx > 0) mangled += "__" + std::to_string(ctor_idx);
      ctors.push_back({mangled, method});
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
    if (!ctx || ctx->tpl_bindings.empty()) return nullptr;
    if (ts.template_param_text_id != kInvalidText) {
      auto text_it = ctx->tpl_bindings_by_text.find(ts.template_param_text_id);
      if (text_it != ctx->tpl_bindings_by_text.end()) return &text_it->second;
      if (module_ && module_->link_name_texts) {
        const std::string key(
            module_->link_name_texts->lookup(ts.template_param_text_id));
        auto it = ctx->tpl_bindings.find(key);
        if (it != ctx->tpl_bindings.end()) return &it->second;
      }
    }
    if (ts.tag_text_id != kInvalidText && module_ && module_->link_name_texts) {
      const std::string key(module_->link_name_texts->lookup(ts.tag_text_id));
      auto it = ctx->tpl_bindings.find(key);
      if (it != ctx->tpl_bindings.end()) return &it->second;
    }
    const std::string_view legacy_tag = typespec_legacy_tag_if_present(ts, 0);
    if (legacy_tag.empty()) return nullptr;
    auto it = ctx->tpl_bindings.find(std::string(legacy_tag));
    return it == ctx->tpl_bindings.end() ? nullptr : &it->second;
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
        if (lookup_nttp_binding(ctx, n, name.c_str())) {
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
        auto lit = ctx->locals.find(name);
        if (lit != ctx->locals.end()) {
          if (ctx->local_types.contains(lit->second)) {
            return reference_value_ts(ctx->local_types.at(lit->second));
          }
        }
        auto pit = ctx->params.find(name);
        if (pit != ctx->params.end() && ctx->fn &&
            pit->second < ctx->fn->params.size()) {
          return reference_value_ts(ctx->fn->params[pit->second].type.spec);
        }
        auto sit = ctx->static_globals.find(name);
        if (sit != ctx->static_globals.end()) {
          if (const GlobalVar* gv = module_->find_global(sit->second)) {
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
          DeclRef fn_ref = make_function_lookup_decl_ref(*module_, dit->second.mangled_name);
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
