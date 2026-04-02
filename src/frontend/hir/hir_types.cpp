// Unintegrated draft extracted from ast_to_hir.cpp.
// This file is a staging artifact for the type/struct/layout/init-normalization
// cluster and is not yet wired into the build.
//
// Omitted for now: lower_struct_def, infer_generic_ctrl_type, and the template /
// program coordinator paths. Those stay in ast_to_hir.cpp until the shared
// Lowerer declaration surface is hoisted into ast_to_hir.hpp.

#include "ast_to_hir.hpp"
#include "hir_lowerer_internal.hpp"
#include "consteval.hpp"
#include "type_utils.hpp"
#include "../parser/parser_internal.hpp"

#include <algorithm>
#include <cstring>
#include <functional>
#include <set>
#include <sstream>
#include <stdexcept>

namespace c4c::hir {

bool Lowerer::is_lvalue_ref_ts(const TypeSpec& ts) {
  return ts.is_lvalue_ref;
}

std::shared_ptr<CompileTimeState> Lowerer::ct_state() const { return ct_state_; }

void Lowerer::resolve_typedef_to_struct(TypeSpec& ts) const {
  if (ts.base != TB_TYPEDEF || !ts.tag) return;
  auto sit = module_->struct_defs.find(ts.tag);
  if (sit != module_->struct_defs.end()) {
    ts.base = TB_STRUCT;
    ts.tag = sit->second.tag.c_str();
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
    const Node* n, const NttpBindings& nttp_bindings) const {
  if (!n) return 0;
  if (n->kind == NK_INT_LIT || n->kind == NK_CHAR_LIT) return n->ival;
  if (n->kind == NK_VAR && n->name) {
    auto nttp_it = nttp_bindings.find(n->name);
    if (nttp_it != nttp_bindings.end()) return nttp_it->second;
    auto enum_it = enum_consts_.find(n->name);
    if (enum_it != enum_consts_.end()) return enum_it->second;
    return 0;
  }
  if (n->kind == NK_CAST && n->left) {
    long long v = eval_const_int_with_nttp_bindings(n->left, nttp_bindings);
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
      return -eval_const_int_with_nttp_bindings(n->left, nttp_bindings);
    if (strcmp(n->op, "+") == 0)
      return eval_const_int_with_nttp_bindings(n->left, nttp_bindings);
    if (strcmp(n->op, "~") == 0)
      return ~eval_const_int_with_nttp_bindings(n->left, nttp_bindings);
  }
  if (n->kind == NK_BINOP && n->op && n->left && n->right) {
    long long l = eval_const_int_with_nttp_bindings(n->left, nttp_bindings);
    long long r = eval_const_int_with_nttp_bindings(n->right, nttp_bindings);
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

std::optional<std::string> Lowerer::find_struct_method_mangled(
    const std::string& tag,
    const std::string& method,
    bool is_const_obj) const {
  const std::string base_key = tag + "::" + method;
  const std::string const_key = base_key + "_const";
  auto try_local = [&]() -> std::optional<std::string> {
    auto it = is_const_obj ? struct_methods_.find(const_key)
                           : struct_methods_.find(base_key);
    if (it != struct_methods_.end()) return it->second;
    it = is_const_obj ? struct_methods_.find(base_key)
                      : struct_methods_.find(const_key);
    if (it != struct_methods_.end()) return it->second;
    return std::nullopt;
  };
  if (auto local = try_local()) return local;
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

std::optional<TypeSpec> Lowerer::find_struct_method_return_type(
    const std::string& tag,
    const std::string& method,
    bool is_const_obj) const {
  const std::string base_key = tag + "::" + method;
  const std::string const_key = base_key + "_const";
  auto try_local = [&]() -> std::optional<TypeSpec> {
    auto it = is_const_obj ? struct_method_ret_types_.find(const_key)
                           : struct_method_ret_types_.find(base_key);
    if (it != struct_method_ret_types_.end()) return it->second;
    it = is_const_obj ? struct_method_ret_types_.find(base_key)
                      : struct_method_ret_types_.find(const_key);
    if (it != struct_method_ret_types_.end()) return it->second;
    return std::nullopt;
  };
  if (auto local = try_local()) return local;
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

std::optional<TypeSpec> Lowerer::infer_call_result_type_from_callee(
    FunctionCtx* ctx, const Node* callee) {
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
  const auto git = module_->global_index.find(name);
  if (git != module_->global_index.end()) {
    if (const GlobalVar* gv = module_->find_global(git->second)) {
      if (gv->fn_ptr_sig) return gv->fn_ptr_sig->return_type.spec;
    }
  }
  const auto fit = module_->fn_index.find(name);
  if (fit != module_->fn_index.end() && fit->second.value < module_->functions.size()) {
    return module_->functions[fit->second.value].return_type.spec;
  }
  return std::nullopt;
}

std::optional<TypeSpec> Lowerer::storage_type_for_declref(
    FunctionCtx* ctx, const DeclRef& r) {
  if (r.local && ctx) {
    auto it = ctx->local_types.find(r.local->value);
    if (it != ctx->local_types.end()) return it->second;
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

long long Lowerer::flat_scalar_count(const TypeSpec& ts) const {
  if (is_vector_ty(ts)) return ts.vector_lanes > 0 ? ts.vector_lanes : 1;
  if (ts.array_rank > 0) {
    if (ts.array_size <= 0) return 1;
    TypeSpec elem_ts = ts;
    elem_ts.array_rank--;
    elem_ts.array_size = (elem_ts.array_rank > 0) ? elem_ts.array_dims[0] : -1;
    return ts.array_size * flat_scalar_count(elem_ts);
  }
  if ((ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.ptr_level == 0 && ts.tag) {
    const auto it = module_->struct_defs.find(ts.tag);
    if (it == module_->struct_defs.end()) return 1;
    const auto& sd = it->second;
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
  if (ts.base != TB_UNION || ts.ptr_level != 0 || !ts.tag || !ts.tag[0]) return false;
  const auto it = module_->struct_defs.find(ts.tag);
  if (it == module_->struct_defs.end()) return false;
  const auto& sd = it->second;
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
  if (ts.base != TB_STRUCT || ts.ptr_level != 0 || !ts.tag || !ts.tag[0]) return false;
  const auto it = module_->struct_defs.find(ts.tag);
  if (it == module_->struct_defs.end()) return false;
  const auto& sd = it->second;
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
          return item.field_designator.has_value() || item.index_designator.has_value();
        });
  };
  auto find_aggregate_field_index =
      [&](const HirStructDef& sd, const InitListItem& item, size_t next_idx)
      -> std::optional<size_t> {
    size_t idx = next_idx;
    if (item.field_designator) {
      const auto fit = std::find_if(
          sd.fields.begin(), sd.fields.end(),
          [&](const HirStructField& field) { return field.name == *item.field_designator; });
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
    if (!sd.fields[idx].name.empty()) item->field_designator = sd.fields[idx].name;
    else item->index_designator = static_cast<long long>(idx);
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
        cur_ts.ptr_level == 0 && cur_ts.tag) {
      const auto sit = module_->struct_defs.find(cur_ts.tag);
      if (sit == module_->struct_defs.end()) {
        ++cursor;
        return std::monostate{};
      }
      const auto& sd = sit->second;
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

  if (ts.base == TB_UNION && ts.ptr_level == 0 && ts.tag) {
    const auto sit = module_->struct_defs.find(ts.tag);
    if (sit == module_->struct_defs.end()) return init;
    const auto& sd = sit->second;
    if (!sd.is_union || sd.fields.empty()) return init;

    size_t idx = 0;
    GlobalInit child = std::monostate{};
    bool has_child = false;
    if (const auto* list = std::get_if<InitList>(&init)) {
      if (list->items.empty()) return init;
      const auto& item0 = list->items.front();
      const auto maybe_idx = find_aggregate_field_index(sd, item0, 0);
      if (maybe_idx && (item0.field_designator || item0.index_designator)) {
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

  if ((ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.ptr_level == 0 && ts.tag) {
    const auto* list = std::get_if<InitList>(&init);
    const auto sit = module_->struct_defs.find(ts.tag);
    if (sit == module_->struct_defs.end()) {
      return list ? init : normalize_scalar_like(ts, init);
    }
    const auto& sd = sit->second;
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
  auto sit = struct_static_member_decls_.find(tag);
  if (sit != struct_static_member_decls_.end()) {
    auto mit = sit->second.find(member);
    if (mit != sit->second.end()) return mit->second;
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

std::optional<long long> Lowerer::find_struct_static_member_const_value(
    const std::string& tag, const std::string& member) const {
  auto sit = struct_static_member_const_values_.find(tag);
  if (sit != struct_static_member_const_values_.end()) {
    auto mit = sit->second.find(member);
    if (mit != sit->second.end()) return mit->second;
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
  module_->global_index[g.name] = g.id;
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
    ConstEvalEnv env{&enum_consts_, &const_int_bindings_};
    if (auto r = evaluate_constant_expr(n, env); r.ok()) {
      TypeSpec ts = n->type;
      if (ts.base == TB_VOID) ts.base = TB_INT;
      s.expr = append_expr(n, IntLiteral{r.as_int(), false}, ts);
      return s;
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

void Lowerer::lower_global(const Node* gv) {
  GlobalInit computed_init{};
  bool has_init = false;

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
    early_init = lower_global_init(gv->init, nullptr, gv->type.is_const || gv->is_constexpr);
    early_init_done = true;
  }

  GlobalVar g{};
  g.id = next_global_id();
  g.name = gv->name ? gv->name : "<anon_global>";
  g.ns_qual = make_ns_qual(gv);
  {
    TypeBindings empty_tpl_bindings;
    NttpBindings empty_nttp_bindings;
    TypeSpec global_ts = gv->type;
    seed_and_resolve_pending_template_type_if_needed(
        global_ts, empty_tpl_bindings, empty_nttp_bindings, gv,
        PendingTemplateTypeKind::DeclarationType,
        std::string("global-decl:") + g.name);
    resolve_typedef_to_struct(global_ts);
    g.type = qtype_from(global_ts, ValueCategory::LValue);
  }
  g.fn_ptr_sig = fn_ptr_sig_from_decl_node(gv);
  g.linkage = {gv->is_static, gv->is_extern, false, weak_symbols_.count(g.name) > 0,
               static_cast<Visibility>(gv->visibility)};
  g.is_const = gv->type.is_const;
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

  if (g.is_const) {
    if (const auto* scalar = std::get_if<InitScalar>(&g.init)) {
      const Expr& e = module_->expr_pool[scalar->expr.value];
      if (const auto* lit = std::get_if<IntLiteral>(&e.payload)) {
        const_int_bindings_[g.name] = lit->value;
        ct_state_->register_const_int_binding(g.name, lit->value);
      } else if (const auto* ch = std::get_if<CharLiteral>(&e.payload)) {
        const_int_bindings_[g.name] = ch->value;
        ct_state_->register_const_int_binding(g.name, ch->value);
      }
    }
  }

  module_->global_index[g.name] = g.id;
  module_->globals.push_back(std::move(g));
}

// Intentionally left out of this draft:
// - lower_struct_def
// - infer_generic_ctrl_type
// - build_call_* / template deduction / template realization coordinators
// - collect_* / lower_initial_program / build_initial_hir

}  // namespace c4c::hir
