#include "expr.hpp"
#include "consteval.hpp"

#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace c4c::hir {

namespace {

template <typename T>
auto set_typespec_final_spelling_tag_if_present(T& ts, const char* tag, int)
    -> decltype(ts.tag = tag, void()) {
  ts.tag = tag;
}

template <typename T>
void set_typespec_final_spelling_tag_if_present(T&, const char*, long) {}

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
    std::string qname = n->name;
    size_t scope_pos = qname.rfind("::");
    if (scope_pos != std::string::npos) {
      std::string struct_tag = qname.substr(0, scope_pos);
      std::string rendered_member = qname.substr(scope_pos + 2);
      const std::string structured_member =
          (n->unqualified_name && n->unqualified_name[0])
              ? std::string(n->unqualified_name)
              : rendered_member;
      TextId structured_member_text_id = n->unqualified_text_id;
      if (structured_member_text_id == kInvalidText && module_ &&
          module_->link_name_texts && !structured_member.empty()) {
        structured_member_text_id = module_->link_name_texts->intern(structured_member);
      }
      const bool has_template_args = n->has_template_args || n->n_template_args > 0;
      struct ScopedStaticMemberLookup {
        std::optional<std::string> owner_tag;
        std::optional<HirStructMemberLookupKey> member_key;
      };
      auto resolve_static_owner_lookup = [&]() -> ScopedStaticMemberLookup {
        TypeSpec owner_ts{};
        owner_ts.base = TB_STRUCT;
        owner_ts.array_size = -1;
        owner_ts.inner_rank = -1;
        set_typespec_final_spelling_tag_if_present(owner_ts, struct_tag.c_str(), 0);
        auto sdit = struct_def_nodes_.find(struct_tag);
        if (sdit != struct_def_nodes_.end() && sdit->second) {
          owner_ts.record_def = const_cast<Node*>(sdit->second);
        }
        const Node* primary_tpl = nullptr;
        if (has_template_args) {
          primary_tpl = find_template_struct_primary(struct_tag);
          if (primary_tpl) {
            owner_ts.tpl_struct_origin = struct_tag.c_str();
            assign_template_arg_refs_from_ast_args(
                &owner_ts, n, primary_tpl, ctx, n, PendingTemplateTypeKind::OwnerStruct,
                "nameref-static-owner-arg");
          }
        }
        if (!owner_ts.record_def && !owner_ts.tpl_struct_origin) return {};
        std::optional<HirRecordOwnerKey> record_owner_key =
            (owner_ts.record_def && owner_ts.record_def->kind == NK_STRUCT_DEF)
                ? make_struct_def_node_owner_key(owner_ts.record_def)
                : std::nullopt;
        const std::string* current_struct_tag =
            (ctx && !ctx->method_struct_tag.empty()) ? &ctx->method_struct_tag : nullptr;
        ScopedStaticMemberLookup out;
        out.owner_tag = resolve_member_lookup_owner_tag(
            owner_ts, false, ctx ? &ctx->tpl_bindings : nullptr,
            ctx ? &ctx->nttp_bindings : nullptr, current_struct_tag, n,
            std::string("nameref-static-member:") + structured_member);
        if (record_owner_key && structured_member_text_id != kInvalidText) {
          out.member_key =
              make_struct_member_lookup_key(*record_owner_key, structured_member_text_id);
        }
        if (!out.member_key && out.owner_tag) {
          out.member_key =
              make_struct_member_lookup_key(*out.owner_tag, structured_member);
        }
        return out;
      };
      const ScopedStaticMemberLookup structured_lookup =
          resolve_static_owner_lookup();
      const std::string lookup_struct_tag =
          structured_lookup.owner_tag.value_or(struct_tag);
      auto find_static_member_decl = [&](const std::string& lookup_tag) -> const Node* {
        if (structured_lookup.member_key) {
          if (const Node* decl = find_struct_static_member_decl(
                  *structured_lookup.member_key, &lookup_tag, &rendered_member)) {
            return decl;
          }
        }
        return find_struct_static_member_decl(lookup_tag, rendered_member);
      };
      auto find_static_member_const_value =
          [&](const std::string& lookup_tag) -> std::optional<long long> {
        if (structured_lookup.member_key) {
          if (auto value = find_struct_static_member_const_value(
                  *structured_lookup.member_key, &lookup_tag, &rendered_member)) {
            return value;
          }
        }
        return find_struct_static_member_const_value(lookup_tag, rendered_member);
      };
      if (has_template_args) {
        if (auto v = try_eval_template_static_member_const(
                ctx, struct_tag, n, structured_member)) {
          TypeSpec ts{};
          if (const Node* decl = find_static_member_decl(lookup_struct_tag)) {
            ts = decl->type;
          }
          if (ts.base == TB_VOID) ts.base = TB_INT;
          return append_expr(n, IntLiteral{*v, false}, ts);
        }
      }
      if (!structured_lookup.owner_tag && has_template_args &&
          find_template_struct_primary(struct_tag)) {
        TypeSpec pending_ts{};
        pending_ts.base = TB_STRUCT;
        pending_ts.array_size = -1;
        pending_ts.inner_rank = -1;
        pending_ts.tpl_struct_origin = ::strdup(struct_tag.c_str());
        const Node* primary_tpl = find_template_struct_primary(struct_tag);
        assign_template_arg_refs_from_ast_args(
            &pending_ts, n, primary_tpl, ctx, n, PendingTemplateTypeKind::OwnerStruct,
            "nameref-scope-tpl-arg");
        TypeBindings tpl_empty;
        NttpBindings nttp_empty;
        seed_and_resolve_pending_template_type_if_needed(
            pending_ts, ctx ? ctx->tpl_bindings : tpl_empty,
            ctx ? ctx->nttp_bindings : nttp_empty, n,
            PendingTemplateTypeKind::OwnerStruct, "nameref-scope-tpl", primary_tpl);
        if (auto pending_owner_tag = resolve_member_lookup_owner_tag(
                pending_ts, false, ctx ? &ctx->tpl_bindings : nullptr,
                ctx ? &ctx->nttp_bindings : nullptr, nullptr, n,
                "nameref-scope-tpl-owner")) {
          struct_tag = *pending_owner_tag;
        }
      }
      const std::string final_lookup_struct_tag =
          structured_lookup.owner_tag.value_or(struct_tag);
      if (auto v = find_static_member_const_value(final_lookup_struct_tag)) {
        TypeSpec ts{};
        if (const Node* decl = find_static_member_decl(final_lookup_struct_tag)) {
          ts = decl->type;
        }
        if (ts.base == TB_VOID) ts.base = TB_INT;
        return append_expr(n, IntLiteral{*v, false}, ts);
      }
      if (const Node* decl = find_static_member_decl(final_lookup_struct_tag)) {
        if (decl->init) {
          long long v = static_eval_int(decl->init, enum_consts_);
          TypeSpec ts = decl->type;
          if (ts.base == TB_VOID) ts.base = TB_INT;
          return append_expr(n, IntLiteral{v, false}, ts);
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
      if (auto nttp_value = lookup_nttp_binding(ctx, n, n->name)) {
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
