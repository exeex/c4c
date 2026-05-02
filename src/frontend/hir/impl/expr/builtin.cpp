#include "expr.hpp"

#include <algorithm>
#include <optional>
#include <string>

namespace c4c::hir {

namespace {

class LayoutQueries {
 public:
  explicit LayoutQueries(const hir::Module& module) : module_(module) {}

  int struct_size_bytes(const TypeSpec& ts) const {
    const HirStructDef* layout = find_struct_layout(ts);
    return layout ? layout->size_bytes : 4;
  }

  int struct_align_bytes(const TypeSpec& ts) const {
    const HirStructDef* layout = find_struct_layout(ts);
    return layout ? std::max(1, layout->align_bytes) : 4;
  }

  int type_size_bytes(const TypeSpec& ts) const {
    if (ts.array_rank > 0) {
      if (ts.array_size == 0) return 0;
      if (ts.array_size > 0) {
        const TypeSpec elem = array_element_type(ts);
        return static_cast<int>(ts.array_size) * type_size_bytes(elem);
      }
      return 8;
    }
    if (ts.is_vector && ts.vector_bytes > 0) return static_cast<int>(ts.vector_bytes);
    if (ts.ptr_level > 0 && ts.is_ptr_to_array) return 8;
    if (ts.ptr_level > 0 || ts.is_fn_ptr) return 8;
    if ((ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.ptr_level == 0) {
      return struct_size_bytes(ts);
    }
    return sizeof_type_spec(ts);
  }

  int type_align_bytes(const TypeSpec& ts) const {
    int natural = 1;
    if (ts.array_rank > 0) {
      natural = type_align_bytes(array_element_type(ts));
    } else if (ts.is_vector && ts.vector_bytes > 0) {
      natural = static_cast<int>(ts.vector_bytes);
    } else if (ts.ptr_level > 0 || ts.is_fn_ptr) {
      natural = 8;
    } else if ((ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.ptr_level == 0) {
      natural = struct_align_bytes(ts);
    } else {
      natural = std::max(1, static_cast<int>(alignof_type_spec(ts)));
    }
    if (ts.align_bytes > 0) natural = std::max(natural, ts.align_bytes);
    return natural;
  }

 private:
  std::optional<HirRecordOwnerKey> record_owner_key_from_node(
      const Node* record_def) const {
    if (!record_def || record_def->kind != NK_STRUCT_DEF) {
      return std::nullopt;
    }

    TextTable* texts = module_.link_name_texts.get();
    const TextId record_text_id = make_unqualified_text_id(record_def, texts);
    if (record_text_id == kInvalidText) return std::nullopt;

    HirRecordOwnerKey key =
        make_hir_record_owner_key(make_ns_qual(record_def, texts), record_text_id);
    if (!hir_record_owner_key_has_complete_metadata(key)) return std::nullopt;
    return key;
  }

  std::optional<HirRecordOwnerKey> record_owner_key_from_typespec(
      const TypeSpec& ts) const {
    if (auto key = record_owner_key_from_node(ts.record_def); key.has_value()) {
      return key;
    }
    if (ts.namespace_context_id < 0 || ts.tag_text_id == kInvalidText) {
      return std::nullopt;
    }

    NamespaceQualifier ns_qual;
    ns_qual.context_id = ts.namespace_context_id;
    ns_qual.is_global_qualified = ts.is_global_qualified;
    if (ts.qualifier_text_ids && ts.n_qualifier_segments > 0) {
      ns_qual.segment_text_ids.assign(
          ts.qualifier_text_ids,
          ts.qualifier_text_ids + ts.n_qualifier_segments);
    }

    HirRecordOwnerKey key = make_hir_record_owner_key(ns_qual, ts.tag_text_id);
    if (!hir_record_owner_key_has_complete_metadata(key)) return std::nullopt;
    return key;
  }

  const HirStructDef* find_struct_layout(const TypeSpec& ts) const {
    if (auto key = record_owner_key_from_typespec(ts); key.has_value()) {
      if (const HirStructDef* structured =
              module_.find_struct_def_by_owner_structured(*key)) {
        return structured;
      }
      return nullptr;
    }
    const auto it = module_.struct_defs.find(layout_compat_name_from_text(ts));
    return it == module_.struct_defs.end() ? nullptr : &it->second;
  }

  static TypeSpec array_element_type(const TypeSpec& ts) {
    TypeSpec elem = ts;
    elem.array_rank--;
    if (elem.array_rank > 0) {
      for (int i = 0; i < elem.array_rank; ++i) elem.array_dims[i] = elem.array_dims[i + 1];
    }
    elem.array_size = (elem.array_rank > 0) ? elem.array_dims[0] : -1;
    return elem;
  }

  std::string layout_compat_name_from_text(const TypeSpec& ts) const {
    if (module_.link_name_texts) {
      if (ts.tag_text_id != kInvalidText) {
        std::string text(module_.link_name_texts->lookup(ts.tag_text_id));
        if (!text.empty()) return text;
      }
      if (ts.record_def && ts.record_def->unqualified_text_id != kInvalidText) {
        std::string text(
            module_.link_name_texts->lookup(ts.record_def->unqualified_text_id));
        if (!text.empty()) return text;
      }
    }
    if (ts.record_def) {
      if (ts.record_def->name && ts.record_def->name[0]) {
        return std::string(ts.record_def->name);
      }
      if (ts.record_def->unqualified_name && ts.record_def->unqualified_name[0]) {
        return std::string(ts.record_def->unqualified_name);
      }
    }
    return {};
  }

  const hir::Module& module_;
};

TypeSpec apply_builtin_query_template_binding(TypeSpec target,
                                              const TypeSpec& concrete) {
  const int target_ptr_level = target.ptr_level;
  target.base = concrete.base;
  target.enum_underlying_base = concrete.enum_underlying_base;
  target.tag_text_id = concrete.tag_text_id;
  target.template_param_owner_namespace_context_id =
      concrete.template_param_owner_namespace_context_id;
  target.template_param_owner_text_id = concrete.template_param_owner_text_id;
  target.template_param_index = concrete.template_param_index;
  target.template_param_text_id = concrete.template_param_text_id;
  target.record_def = concrete.record_def;
  target.qualifier_segments = concrete.qualifier_segments;
  target.qualifier_text_ids = concrete.qualifier_text_ids;
  target.n_qualifier_segments = concrete.n_qualifier_segments;
  target.is_global_qualified = concrete.is_global_qualified;
  target.namespace_context_id = concrete.namespace_context_id;
  target.deferred_member_type_name = concrete.deferred_member_type_name;
  target.deferred_member_type_text_id = concrete.deferred_member_type_text_id;
  target.tpl_struct_origin = concrete.tpl_struct_origin;
  target.tpl_struct_origin_key = concrete.tpl_struct_origin_key;
  target.tpl_struct_args = concrete.tpl_struct_args;
  target.ptr_level = target_ptr_level + concrete.ptr_level;
  return target;
}

}  // namespace

TypeSpec Lowerer::builtin_query_result_type() const {
  TypeSpec ts{};
  ts.base = TB_ULONG;
  return ts;
}

TypeSpec Lowerer::resolve_builtin_query_type(FunctionCtx* ctx, TypeSpec target) const {
  if (!ctx || ctx->tpl_bindings.empty() || target.base != TB_TYPEDEF) {
    return target;
  }
  if (target.template_param_text_id != kInvalidText) {
    auto text_it = ctx->tpl_bindings_by_text.find(target.template_param_text_id);
    if (text_it != ctx->tpl_bindings_by_text.end()) {
      return apply_builtin_query_template_binding(target, text_it->second);
    }
  }
  if (target.template_param_text_id == kInvalidText || !module_ ||
      !module_->link_name_texts) {
    return target;
  }

  const std::string binding_key(
      module_->link_name_texts->lookup(target.template_param_text_id));
  if (!binding_key.empty()) {
    auto it = ctx->tpl_bindings.find(binding_key);
    if (it != ctx->tpl_bindings.end()) {
      return apply_builtin_query_template_binding(target, it->second);
    }
  }
  if (binding_key.empty()) return target;
  return target;
}

ExprId Lowerer::lower_builtin_sizeof_type(FunctionCtx* ctx, const Node* n) {
  const LayoutQueries queries(*module_);
  const TypeSpec sizeof_target = resolve_builtin_query_type(ctx, n->type);
  const TypeSpec result_ts = builtin_query_result_type();
  if (ctx && sizeof_target.array_rank > 0 && n->type.array_size_expr &&
      (sizeof_target.array_size <= 0 || sizeof_target.array_dims[0] <= 0)) {
    TypeSpec elem_ts = sizeof_target;
    elem_ts.array_rank--;
    if (elem_ts.array_rank > 0) {
      for (int i = 0; i < elem_ts.array_rank; ++i) {
        elem_ts.array_dims[i] = elem_ts.array_dims[i + 1];
      }
    }
    elem_ts.array_size = (elem_ts.array_rank > 0) ? elem_ts.array_dims[0] : -1;

    const ExprId count_id = lower_expr(ctx, n->type.array_size_expr);
    const ExprId elem_size_id = append_expr(
        n, IntLiteral{static_cast<long long>(queries.type_size_bytes(elem_ts)), false},
        result_ts);
    BinaryExpr mul{};
    mul.op = BinaryOp::Mul;
    mul.lhs = count_id;
    mul.rhs = elem_size_id;
    return append_expr(n, mul, result_ts);
  }

  const int size = queries.type_size_bytes(sizeof_target);
  return append_expr(n, IntLiteral{static_cast<long long>(size), false}, result_ts);
}

ExprId Lowerer::lower_builtin_alignof_type(FunctionCtx* ctx, const Node* n) {
  const LayoutQueries queries(*module_);
  const TypeSpec alignof_target = resolve_builtin_query_type(ctx, n->type);
  const int align = queries.type_align_bytes(alignof_target);
  return append_expr(
      n, IntLiteral{static_cast<long long>(align), false}, builtin_query_result_type());
}

int Lowerer::builtin_alignof_expr_bytes(FunctionCtx* ctx, const Node* expr) {
  const LayoutQueries queries(*module_);
  const TypeSpec expr_ts = infer_generic_ctrl_type(ctx, expr);
  int align = 0;
  if (expr && expr->kind == NK_VAR && expr->name) {
    DeclRef ref{};
    ref.name = expr->name;
    ref.name_text_id = make_unqualified_text_id(
        expr, module_ ? module_->link_name_texts.get() : nullptr);
    ref.ns_qual = make_ns_qual(
        expr, module_ ? module_->link_name_texts.get() : nullptr);
    attach_decl_ref_link_name_id(ref);
    if (const Function* fn = module_->resolve_function_decl(ref)) {
      int fn_align = fn->attrs.align_bytes;
      if (fn_align > 0) align = fn_align;
    }
  }
  if (align != 0) return align;
  if (expr_ts.align_bytes > 0) return expr_ts.align_bytes;
  return queries.type_align_bytes(expr_ts);
}

ExprId Lowerer::lower_builtin_alignof_expr(FunctionCtx* ctx, const Node* n) {
  const int align = builtin_alignof_expr_bytes(ctx, n->left);
  return append_expr(
      n, IntLiteral{static_cast<long long>(align), false}, builtin_query_result_type());
}

}  // namespace c4c::hir
