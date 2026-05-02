#include "expr.hpp"
#include "consteval.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace c4c::hir {

namespace {

const Function* find_direct_call_target(const Module& mod, const DeclRef& callee_ref) {
  return mod.resolve_direct_call_callee(callee_ref);
}

LinkNameId find_direct_call_carrier_link_name_id(const Module& mod,
                                                 const DeclRef& callee_ref) {
  const Function* fn = find_direct_call_target(mod, callee_ref);
  return fn ? fn->link_name_id : kInvalidLinkName;
}

TypeSpec direct_call_callee_type(const Module& mod, const DeclRef& dr,
                                 const TypeSpec& fallback) {
  if (const Function* fn = find_direct_call_target(mod, dr)) {
    TypeSpec callee_ts = fn->return_type.spec;
    callee_ts.ptr_level++;
    return callee_ts;
  }
  return fallback;
}

void record_nttp_text_binding(const Node* fn_def,
                              int param_index,
                              long long value,
                              NttpTextBindings* out) {
  if (!fn_def || !out || !fn_def->template_param_name_text_ids ||
      param_index < 0 || param_index >= fn_def->n_template_params) {
    return;
  }
  const TextId text_id = fn_def->template_param_name_text_ids[param_index];
  if (text_id == kInvalidText) return;
  (*out)[text_id] = value;
}

DeclRef make_direct_call_decl_ref(Module& mod, std::string name,
                                  LinkNameId link_name_id = kInvalidLinkName) {
  DeclRef dr{};
  dr.name = std::move(name);
  dr.name_text_id = make_text_id(dr.name, mod.link_name_texts.get());
  dr.link_name_id = (link_name_id != kInvalidLinkName)
      ? link_name_id
      : find_direct_call_carrier_link_name_id(mod, dr);
  return dr;
}

void attach_direct_call_callee_link_name_id(Module& mod, Expr& expr) {
  auto* ref = std::get_if<DeclRef>(&expr.payload);
  if (!ref || ref->local || ref->param_index || ref->global ||
      ref->link_name_id != kInvalidLinkName) {
    return;
  }
  ref->link_name_id = find_direct_call_carrier_link_name_id(mod, *ref);
}

}  // namespace

std::optional<ExprId> Lowerer::try_lower_template_struct_call(FunctionCtx* ctx,
                                                              const Node* n) {
  if (!ctx || !n || !n->left || n->left->kind != NK_VAR || !n->left->name ||
      !n->left->has_template_args ||
      !find_template_struct_primary(n->left->name)) {
    return std::nullopt;
  }

  ExprId tmp_expr = lower_expr(ctx, n->left);
  if (n->n_children == 0) return tmp_expr;
  const TypeSpec& tmp_ts = module_->expr_pool[tmp_expr.value].type.spec;
  if (tmp_ts.base != TB_STRUCT || tmp_ts.ptr_level != 0 || !tmp_ts.tag) {
    return tmp_expr;
  }
  const TypeBindings* tpl_bindings = ctx ? &ctx->tpl_bindings : nullptr;
  const NttpBindings* nttp_bindings = ctx ? &ctx->nttp_bindings : nullptr;
  const std::string* current_struct_tag =
      (ctx && !ctx->method_struct_tag.empty()) ? &ctx->method_struct_tag : nullptr;
  const std::string owner_tag = resolve_struct_method_lookup_owner_tag(
      tmp_ts, false, tpl_bindings, nttp_bindings, current_struct_tag, n,
      "template-struct-call");
  if (owner_tag.empty()) return tmp_expr;

  auto resolved = find_struct_method_mangled(owner_tag, "operator_call", false);
  auto resolved_link_name_id =
      find_struct_method_link_name_id(owner_tag, "operator_call", false);
  if (!resolved) {
    resolved = find_struct_method_mangled(owner_tag, "operator_call", true);
    resolved_link_name_id =
        find_struct_method_link_name_id(owner_tag, "operator_call", true);
  }
  if (!resolved) return tmp_expr;

  ExprId this_obj = tmp_expr;
  if (module_->expr_pool[tmp_expr.value].type.category != ValueCategory::LValue) {
    LocalDecl tmp{};
    tmp.id = next_local_id();
    std::string tmp_name = "__call_tmp_" + std::to_string(tmp.id.value);
    tmp.name = tmp_name;
    tmp.type = qtype_from(tmp_ts, ValueCategory::RValue);
    const TypeSpec init_ts = module_->expr_pool[tmp_expr.value].type.spec;
    if (init_ts.base == tmp_ts.base && init_ts.ptr_level == tmp_ts.ptr_level &&
        init_ts.tag == tmp_ts.tag) {
      tmp.init = tmp_expr;
    }
    append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(n)});

    DeclRef tmp_ref{};
    tmp_ref.name = tmp_name;
    tmp_ref.local = LocalId{tmp.id.value};
    this_obj = append_expr(n, tmp_ref, tmp_ts, ValueCategory::LValue);
  }

  std::string resolved_mangled = *resolved;
  CallExpr oc{};
  DeclRef dr = make_direct_call_decl_ref(
      *module_, resolved_mangled,
      resolved_link_name_id.value_or(kInvalidLinkName));
  TypeSpec fn_ts{};
  fn_ts.base = TB_VOID;
  if (const Function* fn = find_direct_call_target(*module_, dr)) {
    fn_ts = fn->return_type.spec;
  }
  if (fn_ts.base == TB_VOID) {
    if (auto rit = find_struct_method_return_type(owner_tag, "operator_call", false)) {
      fn_ts = *rit;
    }
  }
  TypeSpec callee_ts = fn_ts;
  callee_ts.ptr_level++;
  oc.callee = append_expr(n, dr, callee_ts);
  UnaryExpr addr{};
  addr.op = UnaryOp::AddrOf;
  addr.operand = this_obj;
  TypeSpec ptr_ts = tmp_ts;
  ptr_ts.ptr_level++;
  oc.args.push_back(append_expr(n, addr, ptr_ts));
  for (int i = 0; i < n->n_children; ++i) {
    oc.args.push_back(lower_expr(ctx, n->children[i]));
  }
  return append_expr(n, oc, fn_ts);
}

ExprId Lowerer::lower_call_arg(FunctionCtx* ctx,
                               const Node* arg_node,
                               const TypeSpec* param_ts) {
  auto call_returns_ref = [&](const Node* call_node) -> bool {
    if (auto ts = infer_call_result_type(ctx, call_node)) {
      return is_any_ref_ts(*ts);
    }
    return false;
  };

  if (ctx && arg_node && arg_node->kind == NK_INIT_LIST && param_ts &&
      !param_ts->is_lvalue_ref && !param_ts->is_rvalue_ref) {
    TypeSpec direct_ts = *param_ts;
    direct_ts.is_lvalue_ref = false;
    direct_ts.is_rvalue_ref = false;
    resolve_typedef_to_struct(direct_ts);
    if (direct_ts.base == TB_STRUCT && direct_ts.ptr_level == 0) {
      if (describe_initializer_list_struct(direct_ts, nullptr, nullptr, nullptr)) {
        return materialize_initializer_list_arg(ctx, arg_node, direct_ts);
      }
    }
  }
  if (!param_ts || (!param_ts->is_lvalue_ref && !param_ts->is_rvalue_ref)) {
    return lower_expr(ctx, arg_node);
  }
  if (call_returns_ref(arg_node)) return lower_expr(ctx, arg_node);

  TypeSpec storage_ts = reference_storage_ts(*param_ts);
  if (param_ts->is_rvalue_ref) {
    if (auto storage_addr = try_lower_rvalue_ref_storage_addr(ctx, arg_node, storage_ts)) {
      return *storage_addr;
    }
    ExprId arg_val = lower_expr(ctx, arg_node);
    TypeSpec val_ts = reference_value_ts(*param_ts);
    resolve_typedef_to_struct(val_ts);
    LocalDecl tmp{};
    tmp.id = next_local_id();
    tmp.name = "__rref_arg_tmp";
    tmp.type = qtype_from(val_ts, ValueCategory::LValue);
    tmp.init = arg_val;
    const LocalId tmp_lid = tmp.id;
    ctx->locals[tmp.name] = tmp.id;
    ctx->local_types.insert(tmp.id, val_ts);
    append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(arg_node)});
    DeclRef tmp_ref{};
    tmp_ref.name = "__rref_arg_tmp";
    tmp_ref.local = tmp_lid;
    ExprId var_id = append_expr(arg_node, tmp_ref, val_ts, ValueCategory::LValue);
    UnaryExpr addr{};
    addr.op = UnaryOp::AddrOf;
    addr.operand = var_id;
    return append_expr(arg_node, addr, storage_ts);
  }

  UnaryExpr addr{};
  addr.op = UnaryOp::AddrOf;
  addr.operand = lower_expr(ctx, arg_node);
  return append_expr(arg_node, addr, storage_ts);
}

bool Lowerer::try_expand_pack_call_arg(FunctionCtx* ctx,
                                       CallExpr& call,
                                       const Node* arg_node,
                                       const TypeSpec* construct_param_ts) {
  if (!ctx || !arg_node || arg_node->kind != NK_PACK_EXPANSION || !arg_node->left) {
    return false;
  }
  const Node* pattern = arg_node->left;
  if (pattern->kind != NK_CALL || pattern->n_children != 1 || !pattern->left) {
    return false;
  }
  if (pattern->left->kind != NK_VAR || !pattern->left->name ||
      std::strcmp(pattern->left->name, "forward") != 0) {
    return false;
  }
  if (pattern->left->n_template_args != 1 || !pattern->left->template_arg_types ||
      pattern->left->template_arg_types[0].base != TB_TYPEDEF ||
      !pattern->left->template_arg_types[0].tag) {
    return false;
  }

  const Node* forwarded_arg = pattern->children[0];
  if (!forwarded_arg || forwarded_arg->kind != NK_VAR || !forwarded_arg->name) {
    return false;
  }
  auto pit = ctx->pack_params.find(forwarded_arg->name);
  if (pit == ctx->pack_params.end() || pit->second.empty()) return false;

  for (const auto& elem : pit->second) {
    const Node* tpl_fn = ct_state_->find_template_def("forward");
    TypeBindings forward_bindings;
    if (tpl_fn && tpl_fn->n_template_params > 0 && tpl_fn->template_param_names[0]) {
      forward_bindings[tpl_fn->template_param_names[0]] = elem.type;
    } else {
      forward_bindings["T"] = elem.type;
    }
    const std::string callee_name = mangle_template_name("forward", forward_bindings);

    DeclRef callee_ref = make_direct_call_decl_ref(*module_, callee_name);
    TypeSpec callee_ts = pattern->left->type;
    callee_ts = direct_call_callee_type(*module_, callee_ref, callee_ts);

    CallExpr expanded_call{};
    expanded_call.callee = append_expr(pattern->left, callee_ref, callee_ts);
    TemplateCallInfo tci;
    tci.source_template = "forward";
    tci.source_template_text_id =
        make_text_id(tci.source_template, module_ ? module_->link_name_texts.get() : nullptr);
    tci.template_args.push_back(elem.type);
    expanded_call.template_info = std::move(tci);

    DeclRef param_ref{};
    param_ref.name = elem.element_name;
    param_ref.param_index = elem.param_index;
    ExprId param_expr =
        append_expr(forwarded_arg, param_ref, elem.type, ValueCategory::LValue);
    UnaryExpr addr{};
    addr.op = UnaryOp::AddrOf;
    addr.operand = param_expr;
    TypeSpec param_storage_ts = elem.type;
    param_storage_ts.ptr_level += 1;
    expanded_call.args.push_back(append_expr(forwarded_arg, addr, param_storage_ts));

    TypeSpec ret_ts = elem.type;
    ret_ts.is_lvalue_ref = false;
    ret_ts.is_rvalue_ref = true;
    ExprId expanded_id = append_expr(pattern, expanded_call, ret_ts);
    if (!construct_param_ts ||
        (!construct_param_ts->is_lvalue_ref && !construct_param_ts->is_rvalue_ref)) {
      call.args.push_back(expanded_id);
    } else {
      TypeSpec storage_ts = reference_storage_ts(*construct_param_ts);
      UnaryExpr outer_addr{};
      outer_addr.op = UnaryOp::AddrOf;
      outer_addr.operand = expanded_id;
      call.args.push_back(append_expr(pattern, outer_addr, storage_ts));
    }
  }
  return true;
}

std::optional<ExprId> Lowerer::try_lower_member_call_expr(FunctionCtx* ctx,
                                                          const Node* n) {
  if (!n || !n->left || n->left->kind != NK_MEMBER || !n->left->name) {
    return std::nullopt;
  }

  const Node* base_node = n->left->left;
  const char* method_name = n->left->name;
  TypeSpec base_ts = infer_generic_ctrl_type(ctx, base_node);
  TypeSpec lookup_base_ts = base_ts;
  if (n->left->is_arrow && base_ts.ptr_level > 0) base_ts.ptr_level--;
  const TypeBindings* tpl_bindings = ctx ? &ctx->tpl_bindings : nullptr;
  const NttpBindings* nttp_bindings = ctx ? &ctx->nttp_bindings : nullptr;
  const std::string* current_struct_tag =
      (ctx && !ctx->method_struct_tag.empty()) ? &ctx->method_struct_tag : nullptr;
  const std::string tag = resolve_struct_method_lookup_owner_tag(
      lookup_base_ts, n->left->is_arrow, tpl_bindings, nttp_bindings,
      current_struct_tag, n->left, std::string("member-call:") + method_name);
  if (tag.empty()) return std::nullopt;

  if (auto resolved_method_opt =
          find_struct_method_mangled(tag, method_name, base_ts.is_const)) {
    CallExpr call{};
    std::string resolved_method = *resolved_method_opt;
    const std::string base_key = tag + "::" + method_name;
    auto ovit = ref_overload_set_.find(base_key);
    if (ovit == ref_overload_set_.end()) {
      ovit = ref_overload_set_.find(base_key + "_const");
    }
    if (ovit != ref_overload_set_.end() && !ovit->second.empty()) {
      resolved_method = resolve_ref_overload(ovit->first, n, ctx);
    }
    // `resolved_method` is already the concrete overload body selected for this
    // call. Bind the direct-call carrier from that exact callee name instead of
    // re-looking up the coarse `tag::method` registry key, which can collapse
    // ref-qualified overloads onto the last-registered symbol.
    DeclRef dr = make_direct_call_decl_ref(*module_, resolved_method);
    TypeSpec fn_ts{};
    fn_ts.base = TB_VOID;
    if (const Function* fn = find_direct_call_target(*module_, dr)) {
      fn_ts = fn->return_type.spec;
    }
    fn_ts.ptr_level++;
    call.callee = append_expr(n->left, dr, fn_ts);
    ExprId base_id = lower_expr(ctx, base_node);
    const QualType& base_qt = module_->expr_pool[base_id.value].type;
    const bool base_has_reference_storage =
        base_qt.spec.is_lvalue_ref || base_qt.spec.is_rvalue_ref;
    if (ctx && !n->left->is_arrow &&
        base_qt.category != ValueCategory::LValue &&
        !base_has_reference_storage) {
      LocalDecl tmp{};
      tmp.id = next_local_id();
      std::string tmp_name = "__member_call_tmp_" + std::to_string(tmp.id.value);
      tmp.name = tmp_name;
      tmp.type = qtype_from(base_ts, ValueCategory::RValue);
      const TypeSpec init_ts = module_->expr_pool[base_id.value].type.spec;
      if (init_ts.base == base_ts.base && init_ts.ptr_level == base_ts.ptr_level &&
          init_ts.tag == base_ts.tag) {
        tmp.init = base_id;
      }
      const LocalId tmp_lid = tmp.id;
      append_stmt(*ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(base_node)});

      DeclRef tmp_ref{};
      tmp_ref.name = tmp_name;
      tmp_ref.local = tmp_lid;
      base_id = append_expr(base_node, tmp_ref, base_ts, ValueCategory::LValue);
    }
    if (n->left->is_arrow) {
      call.args.push_back(base_id);
    } else {
      UnaryExpr addr{};
      addr.op = UnaryOp::AddrOf;
      addr.operand = base_id;
      TypeSpec ptr_ts = base_ts;
      ptr_ts.ptr_level++;
      call.args.push_back(append_expr(base_node, addr, ptr_ts));
    }
    const Node* callee_method = find_pending_method_by_mangled(resolved_method);
    for (int i = 0; i < n->n_children; ++i) {
      const TypeSpec* param_ts = nullptr;
      if (callee_method && i < callee_method->n_params && callee_method->params[i]) {
        param_ts = &callee_method->params[i]->type;
      }
      if (try_expand_pack_call_arg(ctx, call, n->children[i], param_ts)) continue;
      call.args.push_back(lower_call_arg(ctx, n->children[i], param_ts));
    }
    TypeSpec ts = n->type;
    if (ts.base == TB_VOID && ts.ptr_level == 0) {
      if (const Function* fn = find_direct_call_target(*module_, dr)) {
        ts = fn->return_type.spec;
      }
    }
    return append_expr(n, call, ts);
  }

  return std::nullopt;
}

ExprId Lowerer::lower_call_expr(FunctionCtx* ctx, const Node* n) {
  if (auto consteval_expr = try_lower_consteval_call_expr(ctx, n)) {
    return *consteval_expr;
  }
  if (auto ctor_expr = try_lower_direct_struct_constructor_call(ctx, n)) {
    return *ctor_expr;
  }
  if (auto template_struct_expr = try_lower_template_struct_call(ctx, n)) {
    return *template_struct_expr;
  }

  if (n->left) {
    TypeSpec callee_ts = infer_generic_ctrl_type(ctx, n->left);
    if (callee_ts.base == TB_STRUCT && callee_ts.ptr_level == 0 && callee_ts.tag) {
      std::vector<const Node*> arg_nodes;
      for (int i = 0; i < n->n_children; ++i)
        arg_nodes.push_back(n->children[i]);
      ExprId op = try_lower_operator_call(ctx, n, n->left, "operator_call", arg_nodes);
      if (op.valid()) return op;
    }
  }

  CallExpr c{};
  auto direct_callee_fn = [&](const std::string& name) -> const Function* {
    DeclRef ref = make_direct_call_decl_ref(*module_, name);
    return find_direct_call_target(*module_, ref);
  };
  if (auto member_call = try_lower_member_call_expr(ctx, n)) {
    return *member_call;
  }

  std::string resolved_callee_name;
  if (n->left && n->left->kind == NK_VAR && n->left->name &&
      (n->left->n_template_args > 0 || n->left->has_template_args) &&
      ct_state_->has_template_def(n->left->name) &&
      !ct_state_->has_consteval_def(n->left->name)) {
    const TypeBindings* enc = (ctx && !ctx->tpl_bindings.empty())
                                  ? &ctx->tpl_bindings : nullptr;
    const NttpBindings* enc_nttp = (ctx && !ctx->nttp_bindings.empty())
                                       ? &ctx->nttp_bindings : nullptr;
    const Node* tpl_fn = ct_state_->find_template_def(n->left->name);
    TypeBindings bindings;
    NttpBindings nttp_bindings;
    if (auto ded_it = deduced_template_calls_.find(n);
        ded_it != deduced_template_calls_.end()) {
      resolved_callee_name = ded_it->second.mangled_name;
      bindings = ded_it->second.bindings;
      nttp_bindings = ded_it->second.nttp_bindings;
    } else {
      bindings = merge_explicit_and_deduced_type_bindings(n, n->left, tpl_fn, enc);
      nttp_bindings = build_call_nttp_bindings(n->left, tpl_fn, enc_nttp);
      resolved_callee_name =
          mangle_template_name(n->left->name, bindings, nttp_bindings);
      if (tpl_fn) {
        const auto param_order = get_template_param_order(tpl_fn, &bindings, &nttp_bindings);
        const SpecializationKey spec_key = nttp_bindings.empty()
            ? make_specialization_key(n->left->name, param_order, bindings)
            : make_specialization_key(n->left->name, param_order, bindings, nttp_bindings);
        if (const auto* inst_list = registry_.find_instances(n->left->name)) {
          for (const auto& inst : *inst_list) {
            if (inst.spec_key == spec_key) {
              resolved_callee_name = inst.mangled_name;
              break;
            }
          }
        }
      }
    }
    DeclRef dr = make_direct_call_decl_ref(*module_, resolved_callee_name);
    c.callee = append_expr(n->left, dr, direct_call_callee_type(*module_, dr, n->left->type));
    TemplateCallInfo tci;
    tci.source_template = n->left->name;
    tci.source_template_text_id =
        make_text_id(tci.source_template, module_ ? module_->link_name_texts.get() : nullptr);
    if (tpl_fn) {
      for (const auto& pname : get_template_param_order(tpl_fn, &bindings, &nttp_bindings)) {
        auto bit = bindings.find(pname);
        if (bit != bindings.end()) tci.template_args.push_back(bit->second);
      }
      tci.nttp_args = std::move(nttp_bindings);
    }
    c.template_info = std::move(tci);
  } else if (auto ded_it = deduced_template_calls_.find(n);
             ded_it != deduced_template_calls_.end()) {
    resolved_callee_name = ded_it->second.mangled_name;
    DeclRef dr = make_direct_call_decl_ref(*module_, resolved_callee_name);
    c.callee = append_expr(n->left, dr, direct_call_callee_type(*module_, dr, n->left->type));
    TemplateCallInfo tci;
    tci.source_template = n->left->name;
    tci.source_template_text_id =
        make_text_id(tci.source_template, module_ ? module_->link_name_texts.get() : nullptr);
    const Node* tpl_fn = ct_state_->find_template_def(n->left->name);
    if (tpl_fn) {
      for (const auto& pname : get_template_param_order(
               tpl_fn, &ded_it->second.bindings, &ded_it->second.nttp_bindings)) {
        auto bit = ded_it->second.bindings.find(pname);
        if (bit != ded_it->second.bindings.end())
          tci.template_args.push_back(bit->second);
      }
      tci.nttp_args = ded_it->second.nttp_bindings;
    }
    c.template_info = std::move(tci);
  } else {
    if (n->left && n->left->kind == NK_VAR && n->left->name &&
        rejected_template_calls_.count(n) > 0) {
      std::string diag = "error: no viable template instantiation for call to '";
      diag += n->left->name;
      diag += "'";
      throw std::runtime_error(diag);
    }
      if (n->left && n->left->kind == NK_VAR && n->left->name &&
          ref_overload_set_.count(n->left->name)) {
        resolved_callee_name = resolve_ref_overload(n->left->name, n, ctx);
        DeclRef dr = make_direct_call_decl_ref(*module_, resolved_callee_name);
        c.callee = append_expr(n->left, dr, direct_call_callee_type(*module_, dr, n->left->type));
      } else {
      bool resolved_as_method = false;
      if (ctx && !ctx->method_struct_tag.empty() &&
          n->left && n->left->kind == NK_VAR && n->left->name) {
        const std::string callee_name = n->left->name;
        if (!direct_callee_fn(callee_name)) {
          auto resolved_method =
              find_struct_method_mangled(ctx->method_struct_tag, callee_name, false);
          if (!resolved_method)
            resolved_method =
                find_struct_method_mangled(ctx->method_struct_tag, callee_name, true);
          if (resolved_method) {
            resolved_callee_name = *resolved_method;
            const LinkNameId resolved_link_name_id =
                find_struct_method_link_name_id(
                    ctx->method_struct_tag, callee_name, false)
                    .value_or(kInvalidLinkName);
            DeclRef dr = make_direct_call_decl_ref(
                *module_, resolved_callee_name, resolved_link_name_id);
            TypeSpec fn_ts{};
            fn_ts.base = TB_VOID;
            if (const Function* fn = find_direct_call_target(*module_, dr)) {
              fn_ts = fn->return_type.spec;
            }
            fn_ts.ptr_level++;
            c.callee = append_expr(n->left, dr, fn_ts);
            auto pit = ctx->params.find("this");
            if (pit != ctx->params.end()) {
              DeclRef this_ref{};
              this_ref.name = "this";
              this_ref.param_index = pit->second;
              TypeSpec this_ts{};
              this_ts.base = TB_STRUCT;
              auto sit = module_->struct_defs.find(ctx->method_struct_tag);
              this_ts.tag = sit != module_->struct_defs.end()
                                ? sit->second.tag.c_str()
                                : ctx->method_struct_tag.c_str();
              this_ts.ptr_level = 1;
              c.args.push_back(append_expr(n->left, this_ref, this_ts, ValueCategory::LValue));
            }
            resolved_as_method = true;
          }
        }
      }
      if (!resolved_as_method) {
        c.callee = lower_expr(ctx, n->left);
        attach_direct_call_callee_link_name_id(*module_, module_->expr_pool[c.callee.value]);
      }
    }
  }
  c.builtin_id = n->builtin_id;
  const Function* callee_fn = !resolved_callee_name.empty()
                                  ? direct_callee_fn(resolved_callee_name)
                                  : (n->left && n->left->kind == NK_VAR && n->left->name
                                         ? direct_callee_fn(n->left->name)
                                         : nullptr);
  for (int i = 0; i < n->n_children; ++i) {
    const TypeSpec* param_ts =
        (callee_fn && static_cast<size_t>(i) < callee_fn->params.size())
            ? &callee_fn->params[static_cast<size_t>(i)].type.spec
            : nullptr;
    c.args.push_back(lower_call_arg(ctx, n->children[i], param_ts));
  }
  TypeSpec ts = n->type;
  if (n->builtin_id != BuiltinId::Unknown) {
    bool known = false;
    TypeSpec builtin_ts = classify_known_builtin_return_type(n->builtin_id, &known);
    if (known) ts = builtin_ts;
  } else if (auto inferred = infer_call_result_type_from_callee(ctx, n->left)) {
    ts = *inferred;
  } else if (n->left && n->left->kind == NK_VAR && n->left->name) {
    bool known = false;
    TypeSpec kts = classify_known_call_return_type(n->left->name, &known);
    if (known) ts = kts;
  }
  return append_expr(n, c, ts);
}

std::optional<ExprId> Lowerer::try_lower_consteval_call_expr(FunctionCtx* ctx,
                                                             const Node* n) {
  if (!(n->kind == NK_CALL && n->left && n->left->kind == NK_VAR && n->left->name))
    return std::nullopt;
  const Node* ce_fn_def = ct_state_->find_consteval_def(n->left->name);
  if (!ce_fn_def) return std::nullopt;

  LowererConstEvalStructuredMaps structured_maps;
  ConstEvalEnv arg_env = make_lowerer_consteval_env(
      structured_maps, ctx ? &ctx->local_const_bindings : nullptr);
  TypeBindings tpl_bindings;
  NttpBindings ce_nttp_bindings;
  NttpTextBindings ce_nttp_bindings_by_text;
  const Node* fn_def = ce_fn_def;
  if ((n->left->n_template_args > 0 || n->left->has_template_args) &&
      fn_def->n_template_params > 0) {
    int count = std::min(n->left->n_template_args, fn_def->n_template_params);
    for (int i = 0; i < count; ++i) {
      if (!fn_def->template_param_names[i]) continue;
      if (fn_def->template_param_is_nttp && fn_def->template_param_is_nttp[i]) {
        if (n->left->template_arg_is_value && n->left->template_arg_is_value[i]) {
          if (n->left->template_arg_exprs && n->left->template_arg_exprs[i]) {
            auto expr_value = evaluate_constant_expr(
                n->left->template_arg_exprs[i], arg_env);
            if (expr_value.ok()) {
              const long long value = expr_value.as_int();
              ce_nttp_bindings[fn_def->template_param_names[i]] = value;
              record_nttp_text_binding(fn_def, i, value, &ce_nttp_bindings_by_text);
              continue;
            }
          }
          if (n->left->template_arg_nttp_names && n->left->template_arg_nttp_names[i] &&
              ctx && !ctx->nttp_bindings.empty()) {
            auto it = ctx->nttp_bindings.find(n->left->template_arg_nttp_names[i]);
            if (it != ctx->nttp_bindings.end()) {
              ce_nttp_bindings[fn_def->template_param_names[i]] = it->second;
              record_nttp_text_binding(
                  fn_def, i, it->second, &ce_nttp_bindings_by_text);
            }
          } else {
            const long long value = n->left->template_arg_values[i];
            ce_nttp_bindings[fn_def->template_param_names[i]] = value;
            record_nttp_text_binding(fn_def, i, value, &ce_nttp_bindings_by_text);
          }
        }
        continue;
      }
      TypeSpec arg_ts = n->left->template_arg_types[i];
      if (arg_ts.base == TB_TYPEDEF && arg_ts.tag && ctx && !ctx->tpl_bindings.empty()) {
        auto resolved = ctx->tpl_bindings.find(arg_ts.tag);
        if (resolved != ctx->tpl_bindings.end()) arg_ts = resolved->second;
      }
      tpl_bindings[fn_def->template_param_names[i]] = arg_ts;
    }
    if (fn_def->template_param_has_default) {
      for (int i = count; i < fn_def->n_template_params; ++i) {
        if (!fn_def->template_param_names[i]) continue;
        if (!fn_def->template_param_has_default[i]) continue;
        if (fn_def->template_param_is_nttp && fn_def->template_param_is_nttp[i]) {
          const long long value = fn_def->template_param_default_values[i];
          ce_nttp_bindings[fn_def->template_param_names[i]] = value;
          record_nttp_text_binding(fn_def, i, value, &ce_nttp_bindings_by_text);
        } else {
          tpl_bindings[fn_def->template_param_names[i]] =
              fn_def->template_param_default_types[i];
        }
      }
    }
    arg_env.type_bindings = &tpl_bindings;
    if (!ce_nttp_bindings.empty()) arg_env.nttp_bindings = &ce_nttp_bindings;
    if (!ce_nttp_bindings_by_text.empty()) {
      arg_env.nttp_bindings_by_text = &ce_nttp_bindings_by_text;
    }
  }
  std::vector<ConstValue> args;
  bool all_const = true;
  for (int i = 0; i < n->n_children; ++i) {
    auto r = evaluate_constant_expr(n->children[i], arg_env);
    if (r.ok()) {
      args.push_back(*r.value);
    } else {
      all_const = false;
      break;
    }
  }
  if (!all_const) {
    std::string diag = "error: call to consteval function '";
    diag += n->left->name;
    diag += "' with non-constant arguments";
    throw std::runtime_error(diag);
  }

  PendingConstevalExpr pce;
  pce.fn_name = n->left->name;
  for (const auto& cv : args) pce.const_args.push_back(cv.as_int());
  pce.tpl_bindings = tpl_bindings;
  pce.nttp_bindings = ce_nttp_bindings;
  pce.nttp_bindings_by_text = ce_nttp_bindings_by_text;
  pce.call_span = make_span(n);
  pce.unlocked_by_deferred_instantiation = lowering_deferred_instantiation_;
  TypeSpec ts = n->type;
  return append_expr(n, std::move(pce), ts);
}

}  // namespace c4c::hir
