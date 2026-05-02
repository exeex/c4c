#include "expr.hpp"
#include "consteval.hpp"

#include <algorithm>
#include <cstring>
#include <functional>
#include <optional>
#include <string>

namespace c4c::hir {

namespace {

bool is_generated_anonymous_record_tag(const char* name) {
  return name && std::strncmp(name, "_anon_", 6) == 0;
}

template <typename T>
auto set_typespec_final_spelling_tag_if_present(T& ts, const char* tag, int)
    -> decltype(ts.tag = tag, void()) {
  ts.tag = tag;
}

void set_typespec_final_spelling_tag_if_present(TypeSpec&, const char*, long) {}

}  // namespace

ExprId Lowerer::hoist_compound_literal_to_global(const Node* addr_node,
                                                 const Node* clit) {
  GlobalVar cg{};
  cg.id = next_global_id();
  const std::string clit_name = "__clit_" + std::to_string(cg.id.value);
  cg.name = clit_name;
  cg.type = qtype_from(clit->type, ValueCategory::LValue);
  cg.linkage = {true, false, false};
  cg.is_const = false;
  cg.span = make_span(clit);
  if (clit->left && clit->left->kind == NK_INIT_LIST) {
    cg.init = lower_init_list(clit->left);
    cg.type.spec = resolve_array_ts(cg.type.spec, cg.init);
    cg.init = normalize_global_init(cg.type.spec, cg.init);
  }
  module_->index_global_decl(cg);
  module_->globals.push_back(std::move(cg));

  TypeSpec ptr_ts = clit->type;
  ptr_ts.ptr_level++;
  DeclRef dr{};
  dr.name = clit_name;
  dr.global = cg.id;
  ExprId dr_id = append_expr(clit, dr, clit->type, ValueCategory::LValue);
  UnaryExpr addr{};
  addr.op = UnaryOp::AddrOf;
  addr.operand = dr_id;
  return append_expr(addr_node, addr, ptr_ts);
}

std::optional<ExprId> Lowerer::try_lower_direct_struct_constructor_call(
    FunctionCtx* ctx,
    const Node* n) {
  if (!ctx || !n || !n->left || n->left->kind != NK_VAR || !n->left->name) {
    return std::nullopt;
  }

  const std::string callee_name = n->left->name;
  std::optional<std::string> structured_callee_tag;
  {
    TypeSpec owner_ts = n->left->type;
    bool has_structured_owner = false;
    if (owner_ts.base == TB_STRUCT) {
      set_typespec_final_spelling_tag_if_present(owner_ts, callee_name.c_str(), 0);
      has_structured_owner =
          owner_ts.record_def || owner_ts.tpl_struct_origin ||
          (owner_ts.tpl_struct_args.data && owner_ts.tpl_struct_args.size > 0);
    } else {
      owner_ts = TypeSpec{};
      owner_ts.base = TB_STRUCT;
      set_typespec_final_spelling_tag_if_present(owner_ts, callee_name.c_str(), 0);
    }
    owner_ts.array_size = -1;
    owner_ts.inner_rank = -1;

    auto sdit = struct_def_nodes_.find(callee_name);
    if (!owner_ts.record_def && sdit != struct_def_nodes_.end() && sdit->second) {
      owner_ts.record_def = const_cast<Node*>(sdit->second);
      has_structured_owner = true;
    }

    const bool has_template_args =
        n->left->has_template_args || n->left->n_template_args > 0;
    const Node* primary_tpl = nullptr;
    if (has_template_args) {
      const char* primary_name =
          (n->left->template_origin_name && n->left->template_origin_name[0])
              ? n->left->template_origin_name
              : callee_name.c_str();
      primary_tpl = find_template_struct_primary(primary_name);
      if (primary_tpl) {
        owner_ts.tpl_struct_origin = primary_name;
        assign_template_arg_refs_from_ast_args(
            &owner_ts, n->left, primary_tpl, ctx, n->left,
            PendingTemplateTypeKind::OwnerStruct, "direct-ctor-owner-arg");
        has_structured_owner = true;
      }
    }

    if (has_structured_owner) {
      TypeBindings tpl_empty;
      NttpBindings nttp_empty;
      seed_and_resolve_pending_template_type_if_needed(
          owner_ts, ctx ? ctx->tpl_bindings : tpl_empty,
          ctx ? ctx->nttp_bindings : nttp_empty, n->left,
          PendingTemplateTypeKind::OwnerStruct, "direct-ctor-owner",
          primary_tpl);
      const std::string* current_struct_tag =
          (ctx && !ctx->method_struct_tag.empty()) ? &ctx->method_struct_tag : nullptr;
      structured_callee_tag = resolve_member_lookup_owner_tag(
          owner_ts, false, ctx ? &ctx->tpl_bindings : nullptr,
          ctx ? &ctx->nttp_bindings : nullptr, current_struct_tag, n->left,
          "direct-ctor-owner");
    }
  }
  const std::string lookup_callee_tag =
      structured_callee_tag.value_or(callee_name);

  if (n->n_children == 0) {
    auto sit = module_->struct_defs.find(lookup_callee_tag);
    auto cit = struct_constructors_.find(lookup_callee_tag);
    if (sit != module_->struct_defs.end() &&
        (cit == struct_constructors_.end() || cit->second.empty())) {
      TypeSpec tmp_ts{};
      tmp_ts.base = TB_STRUCT;
      set_typespec_final_spelling_tag_if_present(
          tmp_ts, sit->second.tag.c_str(), 0);
      tmp_ts.array_size = -1;
      tmp_ts.inner_rank = -1;

      const LocalId tmp_lid = next_local_id();
      const std::string tmp_name = "__brace_tmp_" + std::to_string(tmp_lid.value);
      LocalDecl tmp{};
      tmp.id = tmp_lid;
      tmp.name = tmp_name;
      tmp.type = qtype_from(tmp_ts, ValueCategory::LValue);
      tmp.storage = StorageClass::Auto;
      tmp.span = make_span(n);
      ctx->locals[tmp.name] = tmp.id;
      ctx->local_types.insert(tmp.id, tmp_ts);
      append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(n)});

      DeclRef tmp_ref{};
      tmp_ref.name = tmp_name;
      tmp_ref.local = tmp_lid;
      return append_expr(n, tmp_ref, tmp_ts, ValueCategory::LValue);
    }
  }

  auto cit = struct_constructors_.find(lookup_callee_tag);
  auto sit = module_->struct_defs.find(lookup_callee_tag);
  if (cit == struct_constructors_.end() || sit == module_->struct_defs.end()) {
    return std::nullopt;
  }

  const CtorOverload* best = nullptr;
  if (cit->second.size() == 1) {
    if (cit->second[0].method_node->n_params == n->n_children) {
      best = &cit->second[0];
    }
  } else {
    int best_score = -1;
    for (const auto& ov : cit->second) {
      if (ov.method_node->n_params != n->n_children) continue;
      int score = 0;
      bool viable = true;
      for (int pi = 0; pi < ov.method_node->n_params && viable; ++pi) {
        TypeSpec param_ts = ov.method_node->params[pi]->type;
        resolve_typedef_to_struct(param_ts);
        TypeSpec arg_ts = infer_generic_ctrl_type(ctx, n->children[pi]);
        resolve_typedef_to_struct(arg_ts);
        if (arg_ts.base != TB_STRUCT && n->children[pi] &&
            n->children[pi]->kind == NK_CALL && n->children[pi]->left) {
          TypeSpec constructed_ts = infer_generic_ctrl_type(ctx, n->children[pi]->left);
          resolve_typedef_to_struct(constructed_ts);
          if (constructed_ts.base == TB_STRUCT && constructed_ts.ptr_level == 0) {
            arg_ts = constructed_ts;
          }
        }
        const bool arg_is_lvalue = is_ast_lvalue(n->children[pi], ctx);
        TypeSpec param_base = param_ts;
        param_base.is_lvalue_ref = false;
        param_base.is_rvalue_ref = false;
        if (param_base.base == arg_ts.base && param_base.ptr_level == arg_ts.ptr_level) {
          score += 2;
          if (param_ts.is_rvalue_ref && !arg_is_lvalue) {
            score += 4;
          } else if (param_ts.is_rvalue_ref && arg_is_lvalue) {
            viable = false;
          } else if (param_ts.is_lvalue_ref && arg_is_lvalue) {
            score += 3;
          } else if (param_ts.is_lvalue_ref && !arg_is_lvalue) {
            score += 1;
          }
        } else if (param_base.ptr_level == 0 && arg_ts.ptr_level == 0 &&
                   param_base.base != TB_STRUCT && arg_ts.base != TB_STRUCT &&
                   param_base.base != TB_VOID && arg_ts.base != TB_VOID) {
          score += 1;
        } else {
          viable = false;
        }
      }
      if (viable && score > best_score) {
        best_score = score;
        best = &ov;
      }
    }
  }

  if (!best) return std::nullopt;
  if (best->method_node->is_deleted) {
    std::string diag = "error: call to deleted constructor '";
    diag += lookup_callee_tag;
    diag += "'";
    throw std::runtime_error(diag);
  }

  TypeSpec tmp_ts{};
  tmp_ts.base = TB_STRUCT;
  set_typespec_final_spelling_tag_if_present(tmp_ts, sit->second.tag.c_str(), 0);
  tmp_ts.array_size = -1;
  tmp_ts.inner_rank = -1;

  const LocalId tmp_lid = next_local_id();
  const std::string tmp_name = "__ctor_tmp_" + std::to_string(tmp_lid.value);
  LocalDecl tmp{};
  tmp.id = tmp_lid;
  tmp.name = tmp_name;
  tmp.type = qtype_from(tmp_ts, ValueCategory::LValue);
  tmp.storage = StorageClass::Auto;
  tmp.span = make_span(n);
  ctx->locals[tmp.name] = tmp.id;
  ctx->local_types.insert(tmp.id, tmp_ts);
  append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(n)});

  CallExpr ctor_call{};
  DeclRef callee_ref{};
  callee_ref.name = best->mangled_name;
  attach_decl_ref_link_name_id(callee_ref);
  TypeSpec fn_ts{};
  fn_ts.base = TB_VOID;
  TypeSpec callee_ts = fn_ts;
  callee_ts.ptr_level++;
  ctor_call.callee = append_expr(n, callee_ref, callee_ts);

  DeclRef tmp_ref{};
  tmp_ref.name = tmp_name;
  tmp_ref.local = tmp_lid;
  ExprId tmp_id = append_expr(n, tmp_ref, tmp_ts, ValueCategory::LValue);
  UnaryExpr addr{};
  addr.op = UnaryOp::AddrOf;
  addr.operand = tmp_id;
  TypeSpec ptr_ts = tmp_ts;
  ptr_ts.ptr_level++;
  ctor_call.args.push_back(append_expr(n, addr, ptr_ts));

  for (int i = 0; i < n->n_children; ++i) {
    TypeSpec param_ts = best->method_node->params[i]->type;
    resolve_typedef_to_struct(param_ts);
    if (param_ts.is_rvalue_ref || param_ts.is_lvalue_ref) {
      const Node* arg = n->children[i];
      const Node* inner = arg;
      if (inner->kind == NK_CAST && inner->left) inner = inner->left;
      ExprId arg_val = lower_expr(ctx, inner);
      TypeSpec storage_ts = reference_storage_ts(param_ts);
      UnaryExpr addr_e{};
      addr_e.op = UnaryOp::AddrOf;
      addr_e.operand = arg_val;
      ctor_call.args.push_back(append_expr(arg, addr_e, storage_ts));
    } else {
      ctor_call.args.push_back(lower_expr(ctx, n->children[i]));
    }
  }

  ExprId call_id = append_expr(n, ctor_call, fn_ts);
  ExprStmt es{};
  es.expr = call_id;
  append_stmt(*ctx, Stmt{StmtPayload{es}, make_span(n)});
  return tmp_id;
}

bool Lowerer::describe_initializer_list_struct(const TypeSpec& ts,
                                               TypeSpec* elem_ts,
                                               TypeSpec* data_ptr_ts,
                                               TypeSpec* len_ts) const {
  const HirStructDef* layout = find_struct_def_for_layout_type(ts);
  if (!layout) return false;

  const HirStructField* data_field = nullptr;
  const HirStructField* len_field = nullptr;
  for (const auto& field : layout->fields) {
    if (field.name == "_M_array") {
      data_field = &field;
    } else if (field.name == "_M_len") {
      len_field = &field;
    }
  }
  if (!data_field || !len_field) return false;
  if (data_field->elem_type.ptr_level <= 0) return false;

  if (data_ptr_ts) *data_ptr_ts = data_field->elem_type;
  if (elem_ts) {
    *elem_ts = data_field->elem_type;
    elem_ts->ptr_level--;
  }
  if (len_ts) *len_ts = len_field->elem_type;
  return true;
}

ExprId Lowerer::materialize_initializer_list_arg(FunctionCtx* ctx,
                                                 const Node* list_node,
                                                 const TypeSpec& param_ts) {
  TypeSpec elem_ts{};
  TypeSpec data_ptr_ts{};
  TypeSpec len_ts{};
  if (!describe_initializer_list_struct(param_ts, &elem_ts, &data_ptr_ts, &len_ts)) {
    return append_expr(list_node, IntLiteral{0, false}, param_ts);
  }

  ExprId data_ptr_id{};
  if (list_node && list_node->n_children > 0) {
    TypeSpec array_ts = elem_ts;
    array_ts.array_rank = 1;
    array_ts.array_size = list_node->n_children;
    array_ts.array_dims[0] = list_node->n_children;
    for (int i = 1; i < 8; ++i) array_ts.array_dims[i] = -1;

    Node array_tmp{};
    array_tmp.kind = NK_COMPOUND_LIT;
    array_tmp.type = array_ts;
    array_tmp.left = const_cast<Node*>(list_node);
    array_tmp.line = list_node->line;

    ExprId array_id = lower_expr(ctx, &array_tmp);

    TypeSpec idx_ts{};
    idx_ts.base = TB_INT;
    ExprId zero_idx = append_expr(list_node, IntLiteral{0, false}, idx_ts);
    IndexExpr first_elem{};
    first_elem.base = array_id;
    first_elem.index = zero_idx;
    ExprId first_elem_id =
        append_expr(list_node, first_elem, elem_ts, ValueCategory::LValue);

    UnaryExpr addr{};
    addr.op = UnaryOp::AddrOf;
    addr.operand = first_elem_id;
    data_ptr_id = append_expr(list_node, addr, data_ptr_ts);
  } else {
    data_ptr_id = append_expr(list_node, IntLiteral{0, false}, data_ptr_ts);
  }

  LocalDecl tmp{};
  tmp.id = next_local_id();
  tmp.name = "__init_list_arg_" + std::to_string(tmp.id.value);
  tmp.type = qtype_from(param_ts, ValueCategory::LValue);
  tmp.storage = StorageClass::Auto;
  tmp.span = make_span(list_node);
  const std::string tmp_name = tmp.name;
  TypeSpec int_ts{};
  int_ts.base = TB_INT;
  tmp.init = append_expr(list_node, IntLiteral{0, false}, int_ts);
  const LocalId tmp_lid = tmp.id;
  ctx->locals[tmp_name] = tmp.id;
  ctx->local_types.insert(tmp.id, param_ts);
  append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(list_node)});

  DeclRef tmp_ref{};
  tmp_ref.name = tmp_name;
  tmp_ref.local = tmp_lid;
  ExprId tmp_id = append_expr(list_node, tmp_ref, param_ts, ValueCategory::LValue);

  auto assign_field = [&](const char* field_name, const TypeSpec& field_ts, ExprId rhs_id) {
    MemberExpr lhs{};
    lhs.base = tmp_id;
    lhs.field = field_name;
    lhs.field_text_id = make_text_id(
        lhs.field, module_ ? module_->link_name_texts.get() : nullptr);
    const std::optional<std::string> owner_tag = resolve_member_lookup_owner_tag(
        param_ts, false, ctx ? &ctx->tpl_bindings : nullptr,
        ctx ? &ctx->nttp_bindings : nullptr,
        (ctx && !ctx->method_struct_tag.empty()) ? &ctx->method_struct_tag : nullptr,
        list_node, std::string("init-list-member:") + field_name);
    if (owner_tag) {
      lhs.resolved_owner_tag = *owner_tag;
      lhs.member_symbol_id = find_struct_member_symbol_id(
          param_ts, *owner_tag, field_name, lhs.field_text_id);
    } else if (const HirStructDef* layout = find_struct_def_for_layout_type(param_ts)) {
      lhs.resolved_owner_tag = layout->tag;
      lhs.member_symbol_id = find_struct_member_symbol_id(
          param_ts, layout->tag, field_name, lhs.field_text_id);
    }
    lhs.is_arrow = false;
    ExprId lhs_id = append_expr(list_node, lhs, field_ts, ValueCategory::LValue);
    AssignExpr assign{};
    assign.op = AssignOp::Set;
    assign.lhs = lhs_id;
    assign.rhs = rhs_id;
    ExprId assign_id = append_expr(list_node, assign, field_ts);
    append_stmt(*ctx, Stmt{StmtPayload{ExprStmt{assign_id}}, make_span(list_node)});
  };

  assign_field("_M_array", data_ptr_ts, data_ptr_id);
  ExprId len_id = append_expr(
      list_node, IntLiteral{list_node ? list_node->n_children : 0, false}, len_ts);
  assign_field("_M_len", len_ts, len_id);
  return tmp_id;
}

ExprId Lowerer::lower_compound_literal_expr(FunctionCtx* ctx, const Node* n) {
  if (!ctx) {
    TypeSpec ts = n->type;
    if (ts.base == TB_VOID) ts.base = TB_INT;
    return append_expr(n, IntLiteral{0, false}, ts);
  }
  const TypeSpec clit_ts = n->type;
  const Node* init_list = (n->left && n->left->kind == NK_INIT_LIST) ? n->left : nullptr;
  TypeSpec actual_ts = clit_ts;
  if (actual_ts.array_rank > 0 && actual_ts.array_size < 0 && init_list) {
    long long count = init_list->n_children;
    for (int ci = 0; ci < init_list->n_children; ++ci) {
      const Node* item = init_list->children[ci];
      if (item && item->kind == NK_INIT_ITEM && item->is_designated &&
          item->is_index_desig && item->desig_val + 1 > count) {
        count = item->desig_val + 1;
      }
    }
    actual_ts.array_size = count;
    actual_ts.array_dims[0] = count;
  }
  LocalDecl d{};
  d.id = next_local_id();
  d.name = "<clit>";
  d.type = qtype_from(actual_ts, ValueCategory::LValue);
  d.storage = StorageClass::Auto;
  d.span = make_span(n);
  const LocalId lid = d.id;
  const TypeSpec decl_ts = actual_ts;
  const bool is_struct_or_union =
      (decl_ts.base == TB_STRUCT || decl_ts.base == TB_UNION) && decl_ts.ptr_level == 0 &&
      decl_ts.array_rank == 0;
  const bool is_array = decl_ts.array_rank > 0;
  const bool is_vector = is_vector_ty(decl_ts);
  if ((is_struct_or_union || is_array || is_vector) && init_list) {
    TypeSpec int_ts{};
    int_ts.base = TB_INT;
    d.init = append_expr(n, IntLiteral{0, false}, int_ts);
  } else if (init_list && init_list->n_children > 0) {
    d.init = lower_expr(ctx, unwrap_init_scalar_value(init_list));
  } else if (n->left && !init_list) {
    d.init = lower_expr(ctx, n->left);
  }
  ctx->local_types.insert(lid, decl_ts);
  append_stmt(*ctx, Stmt{StmtPayload{std::move(d)}, make_span(n)});
  DeclRef dr{};
  dr.name = "<clit>";
  dr.local = lid;
  ExprId base_id = append_expr(n, dr, decl_ts, ValueCategory::LValue);
  if (init_list && (is_struct_or_union || is_array || is_vector)) {
    auto is_agg = [](const TypeSpec& ts) {
      return (ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.ptr_level == 0 &&
             ts.array_rank == 0;
    };
    auto append_assign = [&](ExprId lhs_id, const TypeSpec& lhs_ts, const Node* rhs_node) {
      if (!rhs_node) return;
      if (lhs_ts.array_rank == 1 && rhs_node->kind == NK_STR_LIT && rhs_node->sval) {
        TypeSpec elem_ts = lhs_ts;
        elem_ts.array_rank = 0;
        elem_ts.array_size = -1;
        if (elem_ts.ptr_level == 0 &&
            (elem_ts.base == TB_CHAR || elem_ts.base == TB_SCHAR || elem_ts.base == TB_UCHAR)) {
          const bool is_wide = rhs_node->sval[0] == 'L';
          const auto vals = decode_string_literal_values(rhs_node->sval, is_wide);
          const long long max_count = lhs_ts.array_size > 0 ? lhs_ts.array_size
                                                            : static_cast<long long>(vals.size());
          const long long emit_n =
              std::min<long long>(max_count, static_cast<long long>(vals.size()));
          for (long long idx = 0; idx < emit_n; ++idx) {
            TypeSpec idx_ts{};
            idx_ts.base = TB_INT;
            ExprId idx_id = append_expr(n, IntLiteral{idx, false}, idx_ts);
            IndexExpr ie{};
            ie.base = lhs_id;
            ie.index = idx_id;
            ExprId ie_id = append_expr(n, ie, elem_ts, ValueCategory::LValue);
            ExprId val_id =
                append_expr(n, IntLiteral{vals[static_cast<size_t>(idx)], false}, idx_ts);
            AssignExpr ae{};
            ae.lhs = ie_id;
            ae.rhs = val_id;
            ExprId ae_id = append_expr(n, ae, elem_ts);
            ExprStmt es{};
            es.expr = ae_id;
            append_stmt(*ctx, Stmt{StmtPayload{es}, make_span(n)});
          }
          return;
        }
      }
      const Node* val_node = unwrap_init_scalar_value(rhs_node);
      ExprId val_id = lower_expr(ctx, val_node);
      AssignExpr ae{};
      ae.lhs = lhs_id;
      ae.rhs = val_id;
      ExprId ae_id = append_expr(n, ae, lhs_ts);
      ExprStmt es{};
      es.expr = ae_id;
      append_stmt(*ctx, Stmt{StmtPayload{es}, make_span(n)});
    };
    auto can_direct_assign_agg = [&](const TypeSpec& lhs_ts, const Node* rhs_node) -> bool {
      if (!rhs_node) return false;
      if (!is_agg(lhs_ts)) return false;
      const TypeSpec rhs_ts = infer_generic_ctrl_type(ctx, rhs_node);
      if (rhs_ts.ptr_level != 0 || rhs_ts.array_rank != 0) return false;
      if (rhs_ts.base != lhs_ts.base) return false;
      if (rhs_ts.base == TB_STRUCT || rhs_ts.base == TB_UNION || rhs_ts.base == TB_ENUM) {
        const char* lt = lhs_ts.tag ? lhs_ts.tag : "";
        const char* rt = rhs_ts.tag ? rhs_ts.tag : "";
        return std::strcmp(lt, rt) == 0;
      }
      return true;
    };
    auto is_direct_char_array_init = [&](const TypeSpec& lhs_ts, const Node* rhs_node) -> bool {
      if (!rhs_node) return false;
      if (lhs_ts.array_rank != 1 || lhs_ts.ptr_level != 0) return false;
      if (!(lhs_ts.base == TB_CHAR || lhs_ts.base == TB_SCHAR || lhs_ts.base == TB_UCHAR)) {
        return false;
      }
      return rhs_node->kind == NK_STR_LIT;
    };
    struct AggregateOwner {
      std::string tag;
      const HirStructDef* def = nullptr;
    };
    auto has_aggregate_structured_owner_identity = [&](const TypeSpec& owner_ts) {
      const std::optional<HirRecordOwnerKey> record_owner_key =
          (owner_ts.record_def && owner_ts.record_def->kind == NK_STRUCT_DEF)
              ? make_struct_def_node_owner_key(owner_ts.record_def)
              : std::nullopt;
      const bool record_def_has_structured_owner_identity =
          record_owner_key.has_value() &&
          !is_generated_anonymous_record_tag(owner_ts.record_def->name);
      return record_def_has_structured_owner_identity || owner_ts.tpl_struct_origin ||
             (owner_ts.tpl_struct_args.data && owner_ts.tpl_struct_args.size > 0);
    };
    auto resolve_structured_aggregate_owner_tag =
        [&](TypeSpec owner_ts,
            const char* context) -> std::optional<std::string> {
      while (resolve_struct_member_typedef_if_ready(&owner_ts)) {
      }
      resolve_typedef_to_struct(owner_ts);
      if (!is_agg(owner_ts)) return std::nullopt;
      const std::optional<HirRecordOwnerKey> record_owner_key =
          (owner_ts.record_def && owner_ts.record_def->kind == NK_STRUCT_DEF)
              ? make_struct_def_node_owner_key(owner_ts.record_def)
              : std::nullopt;
      if (owner_ts.record_def && owner_ts.record_def->kind == NK_STRUCT_DEF) {
        if (record_owner_key) {
          if (const SymbolName* owner_tag =
                  module_->find_struct_def_tag_by_owner(*record_owner_key)) {
            if (module_->struct_defs.count(*owner_tag)) {
              return std::string(*owner_tag);
            }
          }
        }
      }
      if (owner_ts.tpl_struct_origin && owner_ts.tpl_struct_origin[0]) {
        const std::string incoming_tag =
            (owner_ts.tag && owner_ts.tag[0]) ? std::string(owner_ts.tag)
                                              : std::string{};
        if (ctx && !ctx->tpl_bindings.empty()) {
          seed_and_resolve_pending_template_type_if_needed(
              owner_ts, ctx->tpl_bindings, ctx->nttp_bindings, n,
              PendingTemplateTypeKind::OwnerStruct, context);
        } else {
          TypeBindings empty_tb;
          NttpBindings empty_nttp;
          realize_template_struct_if_needed(
              owner_ts, empty_tb, ctx ? ctx->nttp_bindings : empty_nttp);
        }
        if (owner_ts.tag && owner_ts.tag[0] &&
            (incoming_tag.empty() || incoming_tag != owner_ts.tag) &&
            module_->struct_defs.count(owner_ts.tag)) {
          return std::string(owner_ts.tag);
        }
      }
      return std::nullopt;
    };
    auto resolve_aggregate_owner = [&](const TypeSpec& owner_ts,
                                       const char* context) -> std::optional<AggregateOwner> {
      if (!is_agg(owner_ts)) return std::nullopt;
      const bool has_structured_owner =
          has_aggregate_structured_owner_identity(owner_ts);
      const std::optional<std::string> owner_tag =
          has_structured_owner
              ? resolve_structured_aggregate_owner_tag(owner_ts, context)
              : resolve_member_lookup_owner_tag(
                    owner_ts, false, ctx ? &ctx->tpl_bindings : nullptr,
                    ctx ? &ctx->nttp_bindings : nullptr,
                    ctx && !ctx->method_struct_tag.empty() ? &ctx->method_struct_tag
                                                           : nullptr,
                    n, context);
      if (owner_tag) {
        const auto sit = module_->struct_defs.find(*owner_tag);
        if (sit != module_->struct_defs.end()) {
          return AggregateOwner{*owner_tag, &sit->second};
        }
      }
      if (!has_structured_owner && owner_ts.tag && owner_ts.tag[0]) {
        const auto sit = module_->struct_defs.find(owner_ts.tag);
        if (sit != module_->struct_defs.end()) {
          return AggregateOwner{std::string(owner_ts.tag), &sit->second};
        }
      }
      return std::nullopt;
    };
    auto resolve_aggregate_member_symbol =
        [&](const AggregateOwner& owner, const std::string& member,
            MemberSymbolId fallback_id) -> MemberSymbolId {
      MemberSymbolId member_symbol_id = kInvalidMemberSymbol;
      if (!owner.tag.empty()) {
        member_symbol_id = find_struct_member_symbol_id(owner.tag, member);
      }
      return member_symbol_id == kInvalidMemberSymbol ? fallback_id : member_symbol_id;
    };
    std::function<void(const TypeSpec&, ExprId, const Node*, int&)> consume_from_list;
    consume_from_list = [&](const TypeSpec& cur_ts, ExprId cur_lhs, const Node* list_node,
                            int& cursor) {
      if (!list_node || list_node->kind != NK_INIT_LIST) return;
      if (cursor < 0) cursor = 0;
      if (is_agg(cur_ts)) {
        const std::optional<AggregateOwner> owner =
            resolve_aggregate_owner(cur_ts, "compound-literal-init-member-owner");
        if (!owner || !owner->def) return;
        const auto& sd = *owner->def;
        size_t next_field = 0;
        while (cursor < list_node->n_children && next_field < sd.fields.size()) {
          const Node* item = list_node->children[cursor];
          if (!item) {
            ++cursor;
            ++next_field;
            continue;
          }
          size_t fi = next_field;
          if (item->kind == NK_INIT_ITEM && item->is_designated && !item->is_index_desig &&
              item->desig_field) {
            for (size_t k = 0; k < sd.fields.size(); ++k) {
              if (sd.fields[k].name == item->desig_field) {
                fi = k;
                break;
              }
            }
          }
          if (fi >= sd.fields.size()) {
            ++cursor;
            continue;
          }
          const HirStructField& fld = sd.fields[fi];
          TypeSpec field_ts = field_type_of(fld);
          MemberExpr me{};
          me.base = cur_lhs;
          me.field = fld.name;
          me.field_text_id = make_text_id(
              me.field, module_ ? module_->link_name_texts.get() : nullptr);
          me.resolved_owner_tag = owner->tag;
          me.member_symbol_id =
              resolve_aggregate_member_symbol(*owner, me.field, fld.member_symbol_id);
          me.is_arrow = false;
          ExprId me_id = append_expr(n, me, field_ts, ValueCategory::LValue);
          const Node* val_node = init_item_value_node(item);
          if (is_agg(field_ts) || field_ts.array_rank > 0) {
            if (val_node && val_node->kind == NK_INIT_LIST) {
              int sub_cursor = 0;
              consume_from_list(field_ts, me_id, val_node, sub_cursor);
              ++cursor;
            } else if (is_direct_char_array_init(field_ts, val_node)) {
              append_assign(me_id, field_ts, val_node);
              ++cursor;
            } else if (can_direct_assign_agg(field_ts, val_node)) {
              append_assign(me_id, field_ts, val_node);
              ++cursor;
            } else if (val_node && item->kind == NK_INIT_ITEM && item->is_designated &&
                       !item->is_index_desig && item->desig_field) {
              append_assign(me_id, field_ts, val_node);
              ++cursor;
            } else {
              consume_from_list(field_ts, me_id, list_node, cursor);
            }
          } else {
            if (val_node && val_node->kind == NK_INIT_LIST) {
              int sub_cursor = 0;
              consume_from_list(field_ts, me_id, val_node, sub_cursor);
              ++cursor;
            } else {
              append_assign(me_id, field_ts, val_node);
              ++cursor;
            }
          }
          next_field = fi + 1;
        }
        return;
      }
      if (cur_ts.array_rank > 0 || is_vector_ty(cur_ts)) {
        if (cursor < list_node->n_children) {
          const Node* item0 = list_node->children[cursor];
          const Node* val0 = init_item_value_node(item0);
          if (is_direct_char_array_init(cur_ts, val0)) {
            append_assign(cur_lhs, cur_ts, val0);
            ++cursor;
            return;
          }
        }
        TypeSpec elem_ts = cur_ts;
        long long bound = 0;
        if (is_vector_ty(cur_ts)) {
          elem_ts = vector_element_type(cur_ts);
          bound = cur_ts.vector_lanes > 0 ? cur_ts.vector_lanes : (1LL << 30);
        } else {
          elem_ts.array_rank--;
          elem_ts.array_size = (elem_ts.array_rank > 0) ? elem_ts.array_dims[0] : -1;
          bound = cur_ts.array_size > 0 ? cur_ts.array_size : (1LL << 30);
        }
        long long next_idx = 0;
        while (cursor < list_node->n_children && next_idx < bound) {
          const Node* item = list_node->children[cursor];
          if (!item) {
            ++cursor;
            ++next_idx;
            continue;
          }
          long long idx = next_idx;
          if (item->kind == NK_INIT_ITEM && item->is_designated && item->is_index_desig) {
            idx = item->desig_val;
          }
          TypeSpec idx_ts{};
          idx_ts.base = TB_INT;
          ExprId idx_id = append_expr(n, IntLiteral{idx, false}, idx_ts);
          IndexExpr ie{};
          ie.base = cur_lhs;
          ie.index = idx_id;
          ExprId ie_id = append_expr(n, ie, elem_ts, ValueCategory::LValue);
          const Node* val_node = init_item_value_node(item);
          if (is_agg(elem_ts) || elem_ts.array_rank > 0) {
            if (val_node && val_node->kind == NK_INIT_LIST) {
              int sub_cursor = 0;
              consume_from_list(elem_ts, ie_id, val_node, sub_cursor);
              ++cursor;
            } else if (is_direct_char_array_init(elem_ts, val_node)) {
              append_assign(ie_id, elem_ts, val_node);
              ++cursor;
            } else if (can_direct_assign_agg(elem_ts, val_node)) {
              append_assign(ie_id, elem_ts, val_node);
              ++cursor;
            } else {
              consume_from_list(elem_ts, ie_id, list_node, cursor);
            }
          } else {
            if (val_node && val_node->kind == NK_INIT_LIST) {
              int sub_cursor = 0;
              consume_from_list(elem_ts, ie_id, val_node, sub_cursor);
              ++cursor;
            } else {
              append_assign(ie_id, elem_ts, val_node);
              ++cursor;
            }
          }
          next_idx = idx + 1;
        }
        return;
      }
      if (cursor < list_node->n_children) {
        const Node* item = list_node->children[cursor];
        const Node* val_node = init_item_value_node(item);
        if (val_node && val_node->kind == NK_INIT_LIST) {
          int sub_cursor = 0;
          consume_from_list(cur_ts, cur_lhs, val_node, sub_cursor);
        } else {
          append_assign(cur_lhs, cur_ts, val_node);
        }
        ++cursor;
      }
    };
    int cursor = 0;
    consume_from_list(decl_ts, base_id, init_list, cursor);
  }
  return base_id;
}

ExprId Lowerer::lower_new_expr(FunctionCtx* ctx, const Node* n) {
  TypeSpec alloc_ts = n->type;
  resolve_typedef_to_struct(alloc_ts);
  bool is_array = n->ival != 0;
  SizeofTypeExpr sot{};
  sot.type = qtype_from(alloc_ts, ValueCategory::RValue);
  TypeSpec ulong_ts{};
  ulong_ts.base = TB_ULONG;
  ExprId size_id = append_expr(n, sot, ulong_ts);
  if (is_array && n->right) {
    ExprId count_id = lower_expr(ctx, n->right);
    CastExpr cast_count{};
    cast_count.to_type = qtype_from(ulong_ts, ValueCategory::RValue);
    cast_count.expr = count_id;
    ExprId count_ulong = append_expr(n, cast_count, ulong_ts);
    BinaryExpr mul{};
    mul.op = BinaryOp::Mul;
    mul.lhs = size_id;
    mul.rhs = count_ulong;
    size_id = append_expr(n, mul, ulong_ts);
  }
  std::string op_fn;
  bool is_class_specific = false;
  if (!n->is_global_qualified && alloc_ts.base == TB_STRUCT && alloc_ts.tag) {
    const char* method_name = is_array ? "operator_new_array" : "operator_new";
    const TypeBindings* tpl_bindings = ctx ? &ctx->tpl_bindings : nullptr;
    const NttpBindings* nttp_bindings = ctx ? &ctx->nttp_bindings : nullptr;
    const std::string* current_struct_tag =
        (ctx && !ctx->method_struct_tag.empty()) ? &ctx->method_struct_tag : nullptr;
    const std::string owner_tag = resolve_struct_method_lookup_owner_tag(
        alloc_ts, false, tpl_bindings, nttp_bindings, current_struct_tag, n,
        "new-expression-operator");
    if (!owner_tag.empty()) {
      if (auto method = find_struct_method_mangled(owner_tag, method_name, false)) {
        op_fn = *method;
        is_class_specific = true;
      }
    }
  }
  if (op_fn.empty()) op_fn = is_array ? "operator_new_array" : "operator_new";
  TypeSpec void_ptr_ts{};
  void_ptr_ts.base = TB_VOID;
  void_ptr_ts.ptr_level = 1;
  ExprId raw_ptr;
  if (n->left && n->is_global_qualified) {
    raw_ptr = lower_expr(ctx, n->left);
  } else {
    CallExpr call{};
    DeclRef callee_ref{};
    callee_ref.name = op_fn;
    callee_ref.link_name_id = module_->link_names.find(callee_ref.name);
    TypeSpec fn_ptr_ts{};
    fn_ptr_ts.base = TB_VOID;
    fn_ptr_ts.ptr_level = 1;
    call.callee = append_expr(n, callee_ref, fn_ptr_ts);
    if (is_class_specific) {
      call.args.push_back(append_expr(n, IntLiteral{0, false}, void_ptr_ts));
    }
    call.args.push_back(size_id);
    if (n->left) {
      for (int pi = 0; pi < n->n_params; ++pi) {
        call.args.push_back(lower_expr(ctx, n->params[pi]));
      }
    }
    raw_ptr = append_expr(n, call, void_ptr_ts);
  }
  TypeSpec result_ts = alloc_ts;
  result_ts.ptr_level++;
  CastExpr cast{};
  cast.to_type = qtype_from(result_ts, ValueCategory::RValue);
  cast.expr = raw_ptr;
  ExprId typed_ptr = append_expr(n, cast, result_ts);
  if (n->n_children > 0 && alloc_ts.base == TB_STRUCT && alloc_ts.tag) {
    auto cit = struct_constructors_.find(alloc_ts.tag);
    if (cit != struct_constructors_.end() && !cit->second.empty()) {
      const CtorOverload* best = nullptr;
      if (cit->second.size() == 1) {
        best = &cit->second[0];
      } else {
        for (const auto& ov : cit->second) {
          if (ov.method_node->n_params != n->n_children) continue;
          best = &ov;
          break;
        }
      }
      if (best && !best->method_node->is_deleted) {
        LocalDecl tmp_decl{};
        tmp_decl.id = next_local_id();
        std::string tmp_name = "__new_tmp_" + std::to_string(tmp_decl.id.value);
        tmp_decl.name = tmp_name;
        tmp_decl.type = qtype_from(result_ts, ValueCategory::RValue);
        tmp_decl.init = typed_ptr;
        append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp_decl)}, make_span(n)});
        DeclRef tmp_ref{};
        tmp_ref.name = tmp_name;
        tmp_ref.local = LocalId{tmp_decl.id.value};
        ExprId tmp_id = append_expr(n, tmp_ref, result_ts, ValueCategory::LValue);
        CallExpr ctor_call{};
        DeclRef ctor_ref{};
        ctor_ref.name = best->mangled_name;
        ctor_ref.link_name_id = module_->link_names.find(ctor_ref.name);
        TypeSpec ctor_fn_ts{};
        ctor_fn_ts.base = TB_VOID;
        ctor_fn_ts.ptr_level = 1;
        ctor_call.callee = append_expr(n, ctor_ref, ctor_fn_ts);
        ctor_call.args.push_back(tmp_id);
        for (int i = 0; i < n->n_children; ++i) {
          ctor_call.args.push_back(lower_expr(ctx, n->children[i]));
        }
        TypeSpec void_ts{};
        void_ts.base = TB_VOID;
        ExprId ctor_result = append_expr(n, ctor_call, void_ts);
        ExprStmt es{};
        es.expr = ctor_result;
        append_stmt(*ctx, Stmt{StmtPayload{std::move(es)}, make_span(n)});
        DeclRef ret_ref{};
        ret_ref.name = tmp_name;
        ret_ref.local = LocalId{tmp_decl.id.value};
        return append_expr(n, ret_ref, result_ts, ValueCategory::LValue);
      }
    }
  }
  return typed_ptr;
}

ExprId Lowerer::lower_delete_expr(FunctionCtx* ctx, const Node* n) {
  bool is_array = n->ival != 0;
  ExprId operand = lower_expr(ctx, n->left);
  TypeSpec operand_ts = infer_generic_ctrl_type(ctx, n->left);
  std::string op_fn;
  bool is_class_specific = false;
  if (operand_ts.base == TB_STRUCT && operand_ts.tag && operand_ts.ptr_level > 0) {
    const char* method_name =
        is_array ? "operator_delete_array" : "operator_delete";
    const TypeBindings* tpl_bindings = ctx ? &ctx->tpl_bindings : nullptr;
    const NttpBindings* nttp_bindings = ctx ? &ctx->nttp_bindings : nullptr;
    const std::string* current_struct_tag =
        (ctx && !ctx->method_struct_tag.empty()) ? &ctx->method_struct_tag : nullptr;
    const std::string owner_tag = resolve_struct_method_lookup_owner_tag(
        operand_ts, true, tpl_bindings, nttp_bindings, current_struct_tag, n,
        "delete-expression-operator");
    if (!owner_tag.empty()) {
      if (auto method = find_struct_method_mangled(owner_tag, method_name, false)) {
        op_fn = *method;
        is_class_specific = true;
      }
    }
  }
  if (op_fn.empty()) op_fn = is_array ? "operator_delete_array" : "operator_delete";
  TypeSpec void_ptr_ts{};
  void_ptr_ts.base = TB_VOID;
  void_ptr_ts.ptr_level = 1;
  CastExpr cast{};
  cast.to_type = qtype_from(void_ptr_ts, ValueCategory::RValue);
  cast.expr = operand;
  ExprId void_operand = append_expr(n, cast, void_ptr_ts);
  CallExpr call{};
  DeclRef callee_ref{};
  callee_ref.name = op_fn;
  callee_ref.link_name_id = module_->link_names.find(callee_ref.name);
  TypeSpec fn_ptr_ts{};
  fn_ptr_ts.base = TB_VOID;
  fn_ptr_ts.ptr_level = 1;
  call.callee = append_expr(n, callee_ref, fn_ptr_ts);
  if (is_class_specific) {
    call.args.push_back(append_expr(n, IntLiteral{0, false}, void_ptr_ts));
  }
  call.args.push_back(void_operand);
  TypeSpec void_ts{};
  void_ts.base = TB_VOID;
  ExprId del_call = append_expr(n, call, void_ts);
  ExprStmt es{};
  es.expr = del_call;
  append_stmt(*ctx, Stmt{StmtPayload{std::move(es)}, make_span(n)});
  TypeSpec int_ts{};
  int_ts.base = TB_INT;
  return append_expr(n, IntLiteral{0, false}, int_ts);
}

}  // namespace c4c::hir
