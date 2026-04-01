// Draft-only staging artifact extracted from ast_to_hir.cpp.
// Not yet wired into the build. This file is intentionally incomplete:
// the larger statement orchestration bodies stay in ast_to_hir.cpp for now.
//
// Omitted for now because they are still tightly coupled to expression
// lowering and callable/template coordination:
// - Lowerer::lower_stmt_node
// - Lowerer::lower_local_decl_stmt
// - Lowerer::emit_defaulted_method_body
// - Lowerer::emit_member_dtor_calls
// - Lowerer::emit_dtor_calls
// - Lowerer::lower_stmt_expr_block
// - Lowerer::try_lower_operator_call
// - Lowerer::maybe_bool_convert
// - Lowerer::lower_global
#if 0

#include "ast_to_hir.hpp"
#include "hir_lowerer_internal.hpp"
#include "consteval.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <functional>
#include <stdexcept>

#include "type_utils.hpp"

namespace c4c::hir {

TypeSpec Lowerer::field_type_of(const HirStructField& f) {
  TypeSpec ts = f.elem_type;
  if (f.array_first_dim >= 0) {
    ts.array_rank = 1;
    ts.array_size = f.array_first_dim;
    ts.array_dims[0] = f.array_first_dim;
  }
  return ts;
}

TypeSpec Lowerer::field_init_type_of(const HirStructField& f) {
  TypeSpec ts = field_type_of(f);
  if (f.bit_width >= 0) ts.base = TB_INT;
  return ts;
}

bool Lowerer::is_char_like(TypeBase base) {
  return base == TB_CHAR || base == TB_SCHAR || base == TB_UCHAR;
}

bool Lowerer::is_scalar_init_type(const TypeSpec& ts) {
  if (ts.ptr_level > 0 || ts.array_rank > 0) return false;
  if (is_vector_ty(ts)) return false;
  return ts.base != TB_STRUCT && ts.base != TB_UNION;
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
      [&](const TypeSpec& agg_ts, const InitListItem& item) -> std::optional<size_t> {
    if (agg_ts.base != TB_STRUCT || agg_ts.ptr_level != 0 || !agg_ts.tag) return std::nullopt;
    const auto sit = module_->struct_defs.find(agg_ts.tag);
    if (sit == module_->struct_defs.end()) return std::nullopt;
    const auto& sd = sit->second;
    if (item.field_designator) {
      for (size_t i = 0; i < sd.fields.size(); ++i) {
        if (sd.fields[i].name == *item.field_designator) return i;
      }
      return std::nullopt;
    }
    if (item.index_designator && *item.index_designator >= 0 &&
        static_cast<size_t>(*item.index_designator) < sd.fields.size()) {
      return static_cast<size_t>(*item.index_designator);
    }
    return std::nullopt;
  };

  auto normalize_scalar_or_list = [&](const TypeSpec& cur_ts, const GlobalInit& cur_init) -> GlobalInit {
    if (const auto* scalar = std::get_if<InitScalar>(&cur_init)) {
      if (cur_ts.array_rank == 1 && cur_ts.array_size > 0 &&
          is_char_like(cur_ts.base)) {
        if (const Expr& e = module_->expr_pool[scalar->expr.value];
            std::holds_alternative<StringLiteral>(e.payload)) {
          return cur_init;
        }
      }
      return cur_init;
    }
    if (const auto* list = std::get_if<InitList>(&cur_init)) {
      if (list->items.empty()) return cur_init;
    }
    return cur_init;
  };

  if (const auto* scalar = std::get_if<InitScalar>(&init)) {
    (void)scalar;
    return init;
  }
  if (const auto* list = std::get_if<InitList>(&init)) {
    if (list->items.empty()) return init;
    if (has_designators(*list)) return init;
    if (ts.array_rank == 0 && ts.base != TB_STRUCT && ts.base != TB_UNION) return init;
  }
  return normalize_scalar_or_list(ts, init);
}

GlobalId Lowerer::lower_static_local_global(FunctionCtx& ctx, const Node* n) {
  GlobalVar g{};
  g.id = next_global_id();
  g.name = "__static_local_" + sanitize_symbol(ctx.fn->name) + "_" + std::to_string(g.id.value);
  g.type = qtype_from(n->type, ValueCategory::LValue);
  g.fn_ptr_sig = fn_ptr_sig_from_decl_node(n);
  g.linkage = {true, false, false};  // internal linkage
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

bool Lowerer::struct_has_member_dtors(const std::string& tag) {
  auto sit = module_->struct_defs.find(tag);
  if (sit == module_->struct_defs.end()) return false;
  for (auto it = sit->second.fields.rbegin(); it != sit->second.fields.rend(); ++it) {
    if (it->elem_type.base == TB_STRUCT && it->elem_type.ptr_level == 0 &&
        it->elem_type.tag) {
      std::string ftag = it->elem_type.tag;
      if (struct_destructors_.count(ftag) || struct_has_member_dtors(ftag)) return true;
    }
  }
  return false;
}

}  // namespace c4c::hir
#endif
