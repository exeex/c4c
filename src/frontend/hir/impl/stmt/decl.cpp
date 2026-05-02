#include "stmt.hpp"
#include "consteval.hpp"

#include <cstring>

namespace c4c::hir {

namespace {

bool is_generated_anonymous_record_tag(const char* name) {
  return name && std::strncmp(name, "_anon_", 6) == 0;
}

}  // namespace

void Lowerer::lower_local_decl_stmt(FunctionCtx& ctx, const Node* n) {
  // Local function prototype (e.g. `int f1(char *);` inside a function body):
  // if the name is already registered as a known function, skip creating a
  // local alloca; later references will resolve directly to the global function.
  // Require n_params > 0 to distinguish from a plain variable declaration
  // whose name coincidentally matches a function name.
  if (n->name && !n->init && n->n_params > 0) {
    DeclRef fn_ref{};
    fn_ref.name = n->name;
    fn_ref.name_text_id = make_unqualified_text_id(
        n, module_ ? module_->link_name_texts.get() : nullptr);
    fn_ref.ns_qual = make_ns_qual(
        n, module_ ? module_->link_name_texts.get() : nullptr);
    fn_ref.link_name_id =
        module_ ? module_->link_names.find(fn_ref.name) : kInvalidLinkName;
    if (module_->resolve_function_decl(fn_ref)) return;
  }

  // Local extern declaration: `extern T v;` inside a function refers to
  // the global with that name (C99 6.2.2p4). Erase any shadowing local
  // binding so the global is found by the NK_VAR resolution fallback.
  if (n->is_extern && !n->init && n->name && n->name[0]) {
    DeclRef global_ref{};
    global_ref.name = n->name;
    global_ref.name_text_id = make_unqualified_text_id(
        n, module_ ? module_->link_name_texts.get() : nullptr);
    global_ref.ns_qual = make_ns_qual(
        n, module_ ? module_->link_name_texts.get() : nullptr);
    global_ref.link_name_id =
        module_ ? module_->link_names.find(global_ref.name) : kInvalidLinkName;
    if (const GlobalVar* global = module_->resolve_global_decl(global_ref)) {
      ctx.locals.erase(n->name);
      ctx.static_globals[n->name] = global->id;
    }
    return;
  }

  if (n->is_static) {
    if (n->name && n->name[0]) {
      ctx.static_globals[n->name] = lower_static_local_global(ctx, n);
    }
    return;
  }

  LocalDecl d{};
  d.id = next_local_id();
  d.name = n->name ? n->name : "<anon_local>";
  TypeSpec effective_decl_ts{};
  // Substitute template type parameters in local variable types.
  {
    TypeSpec decl_ts =
        substitute_signature_template_type(n->type, &ctx.tpl_bindings);
    if (decl_ts.base == TB_AUTO && n->init) {
      TypeSpec deduced_ts = infer_generic_ctrl_type(&ctx, n->init);
      deduced_ts.is_const = deduced_ts.is_const || decl_ts.is_const;
      deduced_ts.is_volatile = deduced_ts.is_volatile || decl_ts.is_volatile;
      deduced_ts.is_lvalue_ref = false;
      deduced_ts.is_rvalue_ref = false;
      if (decl_ts.is_lvalue_ref) {
        deduced_ts.is_lvalue_ref = true;
      } else if (decl_ts.is_rvalue_ref) {
        if (is_ast_lvalue(n->init, &ctx)) deduced_ts.is_lvalue_ref = true;
        else deduced_ts.is_rvalue_ref = true;
      }
      decl_ts = deduced_ts;
    }
    effective_decl_ts = decl_ts;
    // Resolve pending template struct types in local variable decls.
    seed_and_resolve_pending_template_type_if_needed(
        effective_decl_ts, ctx.tpl_bindings, ctx.nttp_bindings, n,
        PendingTemplateTypeKind::DeclarationType,
        std::string("local-decl:") + d.name);
    resolve_typedef_to_struct(effective_decl_ts);
    // Parser TypeSpec currently carries function-return ref qualifiers on the
    // surrounding function-pointer declarator. The local itself is still an
    // ordinary function-pointer object, not a reference variable.
    if (effective_decl_ts.is_fn_ptr) {
      effective_decl_ts.is_lvalue_ref = false;
      effective_decl_ts.is_rvalue_ref = false;
    }
    d.type = qtype_from(reference_storage_ts(effective_decl_ts), ValueCategory::LValue);
  }
  d.fn_ptr_sig = fn_ptr_sig_from_decl_node(n);
  if (d.fn_ptr_sig) ctx.local_fn_ptr_sigs[d.name] = *d.fn_ptr_sig;
  // Deduce unsized array dimension from initializer list
  if (n->init && d.type.spec.array_rank > 0 && d.type.spec.array_size < 0) {
    if (n->init->kind == NK_INIT_LIST) {
      long long count = n->init->n_children;
      for (int ci = 0; ci < n->init->n_children; ++ci) {
        const Node* item = n->init->children[ci];
        if (item && item->kind == NK_INIT_ITEM && item->is_designated &&
            item->is_index_desig && item->desig_val + 1 > count)
          count = item->desig_val + 1;
      }
      d.type.spec.array_size = count;
      d.type.spec.array_dims[0] = count;
    } else if (n->init->kind == NK_STR_LIT && n->init->sval) {
      const bool is_wide = n->init->sval[0] == 'L';
      const auto vals = decode_string_literal_values(n->init->sval, is_wide);
      d.type.spec.array_size = static_cast<long long>(vals.size());
      d.type.spec.array_dims[0] = d.type.spec.array_size;
    }
  }
  // VLA: only if size is still unknown after deduction
  if (n->type.array_rank > 0 && n->type.array_size_expr &&
      (d.type.spec.array_size <= 0 || d.type.spec.array_dims[0] <= 0)) {
    d.vla_size = lower_expr(&ctx, n->type.array_size_expr);
  }
  d.storage = n->is_static ? StorageClass::Static : StorageClass::Auto;
  d.span = make_span(n);
  const bool is_array_with_init_list =
      n->init && n->init->kind == NK_INIT_LIST &&
      (d.type.spec.array_rank > 0 || d.type.spec.is_vector);
  const bool is_array_with_string_init =
      n->init && n->init->kind == NK_STR_LIT && d.type.spec.array_rank == 1;
  const bool is_struct_with_init_list =
      n->init && n->init->kind == NK_INIT_LIST &&
      (d.type.spec.base == TB_STRUCT || d.type.spec.base == TB_UNION) &&
      d.type.spec.ptr_level == 0 && d.type.spec.array_rank == 0;
  const bool use_array_init_fast_path =
      is_array_with_init_list && !d.type.spec.is_vector &&
      can_fast_path_scalar_array_init(n->init);
  auto has_decl_structured_owner_identity = [&](const TypeSpec& owner_ts) {
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
  auto resolve_decl_structured_owner_tag =
      [&](TypeSpec owner_ts,
          const std::string& context_name) -> std::optional<std::string> {
    while (resolve_struct_member_typedef_if_ready(&owner_ts)) {
    }
    resolve_typedef_to_struct(owner_ts);
    if ((owner_ts.base != TB_STRUCT && owner_ts.base != TB_UNION) ||
        owner_ts.ptr_level != 0 ||
        owner_ts.array_rank != 0) {
      return std::nullopt;
    }
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
      if (!ctx.tpl_bindings.empty()) {
        seed_and_resolve_pending_template_type_if_needed(
            owner_ts, ctx.tpl_bindings, ctx.nttp_bindings, n,
            PendingTemplateTypeKind::OwnerStruct, context_name);
      } else {
        TypeBindings empty_tb;
        realize_template_struct_if_needed(owner_ts, empty_tb, ctx.nttp_bindings);
      }
      if (owner_ts.tag && owner_ts.tag[0] &&
          (incoming_tag.empty() || incoming_tag != owner_ts.tag) &&
          module_->struct_defs.count(owner_ts.tag)) {
        return std::string(owner_ts.tag);
      }
    }
    return std::nullopt;
  };
  auto resolve_decl_member_symbol =
      [&](const TypeSpec& owner_ts, const std::string& member,
          MemberSymbolId fallback_id) -> std::pair<std::string, MemberSymbolId> {
    const std::optional<std::string> owner_tag =
        has_decl_structured_owner_identity(owner_ts)
            ? resolve_decl_structured_owner_tag(
                  owner_ts, std::string("decl-init-member-owner:") + member)
            : resolve_member_lookup_owner_tag(
                  owner_ts, false, &ctx.tpl_bindings, &ctx.nttp_bindings,
                  !ctx.method_struct_tag.empty() ? &ctx.method_struct_tag : nullptr,
                  n, std::string("decl-init-member-owner:") + member);
    std::string resolved_tag;
    if (owner_tag) {
      resolved_tag = *owner_tag;
    } else if (!has_decl_structured_owner_identity(owner_ts) && owner_ts.tag) {
      resolved_tag = owner_ts.tag;
    }
    MemberSymbolId member_symbol_id = kInvalidMemberSymbol;
    if (!resolved_tag.empty()) {
      const TextId member_text_id =
          make_text_id(member, module_ ? module_->link_name_texts.get() : nullptr);
      member_symbol_id = find_struct_member_symbol_id(
          owner_ts, resolved_tag, member, member_text_id);
    }
    if (member_symbol_id == kInvalidMemberSymbol) {
      member_symbol_id = fallback_id;
    }
    return {std::move(resolved_tag), member_symbol_id};
  };
  auto resolve_decl_struct_owner_tag =
      [&](const TypeSpec& owner_ts,
          const std::string& context_name) -> std::string {
    if ((owner_ts.base != TB_STRUCT && owner_ts.base != TB_UNION) ||
        owner_ts.ptr_level != 0 ||
        owner_ts.array_rank != 0) {
      return {};
    }
    if (has_decl_structured_owner_identity(owner_ts)) {
      if (auto owner_tag =
              resolve_decl_structured_owner_tag(owner_ts, context_name)) {
        return *owner_tag;
      }
      return {};
    }
    return owner_ts.tag ? std::string(owner_ts.tag) : std::string{};
  };
  const std::string decl_struct_owner_tag =
      resolve_decl_struct_owner_tag(
          d.type.spec, std::string("local-decl-struct-owner:") + d.name);
  auto resolve_decl_struct_def =
      [&](const TypeSpec& owner_ts,
          const std::string& context_name)
          -> std::pair<std::string, const HirStructDef*> {
    std::string owner_tag =
        resolve_decl_struct_owner_tag(owner_ts, context_name);
    if (owner_tag.empty() && owner_ts.base == TB_UNION &&
        !has_decl_structured_owner_identity(owner_ts) && owner_ts.tag) {
      owner_tag = owner_ts.tag;
    }
    if (owner_tag.empty()) return {{}, nullptr};
    const auto it = module_->struct_defs.find(owner_tag);
    if (it == module_->struct_defs.end()) return {owner_tag, nullptr};
    return {owner_tag, &it->second};
  };
  // C++ copy initialization: T var = expr; where T has a copy/move constructor.
  // Detect this early so we can skip setting d.init (the ctor call is emitted
  // after the decl, similar to the is_ctor_init path).
  bool is_struct_copy_init = false;
  if (n->init && !n->is_ctor_init && n->init->kind != NK_INIT_LIST &&
      d.type.spec.base == TB_STRUCT && d.type.spec.ptr_level == 0 &&
      d.type.spec.array_rank == 0 && !decl_struct_owner_tag.empty() &&
      !is_lvalue_ref_ts(effective_decl_ts) && !effective_decl_ts.is_rvalue_ref) {
    auto cit = struct_constructors_.find(decl_struct_owner_tag);
    if (cit != struct_constructors_.end()) {
      // Check if any constructor takes a single param of same struct type (copy/move ctor).
      for (const auto& ov : cit->second) {
        if (ov.method_node->n_params == 1) {
          TypeSpec param_ts = ov.method_node->params[0]->type;
          resolve_typedef_to_struct(param_ts);
          const std::string param_owner_tag =
              resolve_decl_struct_owner_tag(
                  param_ts,
                  std::string("local-decl-copy-param-owner:") + d.name);
          if (param_owner_tag == decl_struct_owner_tag &&
              (param_ts.is_lvalue_ref || param_ts.is_rvalue_ref)) {
            is_struct_copy_init = true;
            break;
          }
        }
      }
    }
  }
  if (is_lvalue_ref_ts(effective_decl_ts) && n->init) {
    UnaryExpr addr{};
    addr.op = UnaryOp::AddrOf;
    addr.operand = lower_expr(&ctx, n->init);
    d.init = append_expr(n->init, addr, d.type.spec);
  } else if (effective_decl_ts.is_rvalue_ref && n->init) {
    if (auto storage_addr =
            try_lower_rvalue_ref_storage_addr(&ctx, n->init, d.type.spec)) {
      d.init = *storage_addr;
    } else {
      // Rvalue reference: materialize the rvalue into a temporary, then
      // store a pointer to that temporary as the reference value.
      ExprId init_val = lower_expr(&ctx, n->init);
      TypeSpec val_ts = reference_value_ts(effective_decl_ts);
      // Create a hidden temporary local for the rvalue
      LocalDecl tmp{};
      tmp.id = next_local_id();
      tmp.name = ("__rref_tmp_" + std::to_string(d.id.value)).c_str();
      tmp.type = qtype_from(val_ts, ValueCategory::LValue);
      tmp.init = init_val;
      const LocalId tmp_lid = tmp.id;
      ctx.locals[tmp.name] = tmp.id;
      ctx.local_types.insert(tmp.id, val_ts);
      append_stmt(ctx, Stmt{StmtPayload{std::move(tmp)}, make_span(n)});
      // Take address of temporary
      DeclRef tmp_ref{};
      tmp_ref.name = ("__rref_tmp_" + std::to_string(d.id.value)).c_str();
      tmp_ref.local = tmp_lid;
      ExprId var_id = append_expr(n->init, tmp_ref, val_ts, ValueCategory::LValue);
      UnaryExpr addr{};
      addr.op = UnaryOp::AddrOf;
      addr.operand = var_id;
      d.init = append_expr(n->init, addr, d.type.spec);
    }
  } else if (!is_array_with_init_list && !is_array_with_string_init &&
             !is_struct_with_init_list && !is_struct_copy_init && n->init)
    d.init = lower_expr(&ctx, n->init);
  else if (n->is_ctor_init &&
           !(effective_decl_ts.base == TB_STRUCT && effective_decl_ts.ptr_level == 0 &&
             effective_decl_ts.array_rank == 0 && !decl_struct_owner_tag.empty()) &&
           n->n_children == 1) {
    // Parser reuses is_ctor_init for C++ direct-initialization syntax such as
    // `int i(0)` / `bool b(expr)`. Non-record locals should still lower like a
    // regular single-expression initializer.
    d.init = lower_expr(&ctx, n->children[0]);
  }
  // For aggregate init lists / string array init, emit zeroinitializer first
  // then overlay explicit element/field assignments below.
  if (is_struct_with_init_list || is_array_with_init_list || is_array_with_string_init) {
    TypeSpec int_ts{};
    int_ts.base = TB_INT;
    d.init = append_expr(n, IntLiteral{0, false}, int_ts);
  }
  const LocalId lid = d.id;
  const TypeSpec decl_ts = d.type.spec;
  ctx.locals[d.name] = d.id;
  ctx.local_types.insert(d.id, d.type.spec);
  // Track const/constexpr locals with foldable int initializers.
  if (n->name && n->name[0] && n->init &&
      (effective_decl_ts.is_const || n->is_constexpr) &&
      !effective_decl_ts.is_lvalue_ref && !effective_decl_ts.is_rvalue_ref &&
      effective_decl_ts.ptr_level == 0 && effective_decl_ts.array_rank == 0) {
    LowererConstEvalStructuredMaps structured_maps;
    ConstEvalEnv cenv =
        make_lowerer_consteval_env(structured_maps, &ctx.local_const_bindings);
    if (auto cr = evaluate_constant_expr(n->init, cenv); cr.ok()) {
      ctx.local_const_bindings[n->name] = cr.as_int();
    }
  }
  append_stmt(ctx, Stmt{StmtPayload{std::move(d)}, make_span(n)});

  // C++ constructor invocation: Type var(args) -> call StructTag__StructTag(&var, args...)
  if (n->is_ctor_init && !decl_struct_owner_tag.empty()) {
    auto cit = struct_constructors_.find(decl_struct_owner_tag);
    if (cit != struct_constructors_.end() && !cit->second.empty()) {
      // Resolve best constructor overload by matching argument types.
      const CtorOverload* best = nullptr;
      if (cit->second.size() == 1) {
        best = &cit->second[0];
      } else {
        // Score each constructor overload.
        int best_score = -1;
        for (const auto& ov : cit->second) {
          if (ov.method_node->n_params != n->n_children) continue;
          int score = 0;
          bool viable = true;
          for (int pi = 0; pi < ov.method_node->n_params && viable; ++pi) {
            TypeSpec param_ts = ov.method_node->params[pi]->type;
            resolve_typedef_to_struct(param_ts);
            TypeSpec arg_ts = infer_generic_ctrl_type(&ctx, n->children[pi]);
            bool arg_is_lvalue = is_ast_lvalue(n->children[pi], &ctx);
            // Strip ref qualifiers for base type comparison.
            TypeSpec param_base = param_ts;
            param_base.is_lvalue_ref = false;
            param_base.is_rvalue_ref = false;
            if (param_base.base == arg_ts.base &&
                param_base.ptr_level == arg_ts.ptr_level) {
              score += 2;  // base type match
              // Ref-qualifier scoring: prefer T&& for rvalues, const T& for lvalues.
              if (param_ts.is_rvalue_ref && !arg_is_lvalue) {
                score += 4;  // rvalue ref matches rvalue arg
              } else if (param_ts.is_rvalue_ref && arg_is_lvalue) {
                viable = false;  // T&& cannot bind lvalue
              } else if (param_ts.is_lvalue_ref && arg_is_lvalue) {
                score += 3;  // lvalue ref matches lvalue arg
              } else if (param_ts.is_lvalue_ref && !arg_is_lvalue) {
                score += 1;  // const T& can bind rvalue (lower priority)
              }
            } else if (param_base.ptr_level == 0 && arg_ts.ptr_level == 0 &&
                       param_base.base != TB_STRUCT && arg_ts.base != TB_STRUCT &&
                       param_base.base != TB_VOID && arg_ts.base != TB_VOID) {
              score += 1;  // implicit scalar conversion
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
      if (best) {
        if (best->method_node->is_deleted) {
          std::string diag = "error: call to deleted constructor '";
          diag += decl_struct_owner_tag;
          diag += "'";
          throw std::runtime_error(diag);
        }
        CallExpr c{};
        DeclRef callee_ref{};
        callee_ref.name = best->mangled_name;
        callee_ref.name_text_id = make_text_id(
            callee_ref.name, module_ ? module_->link_name_texts.get() : nullptr);
        callee_ref.link_name_id = module_->link_names.find(callee_ref.name);
        TypeSpec fn_ts{};
        fn_ts.base = TB_VOID;
        TypeSpec callee_ts = fn_ts;
        callee_ts.ptr_level++;
        c.callee = append_expr(n, callee_ref, callee_ts);
        // First arg: &var (this pointer).
        DeclRef var_ref{};
        var_ref.name = n->name ? n->name : "<anon_local>";
        var_ref.name_text_id = make_unqualified_text_id(
            n, module_ ? module_->link_name_texts.get() : nullptr);
        var_ref.local = lid;
        ExprId var_id = append_expr(n, var_ref, decl_ts, ValueCategory::LValue);
        UnaryExpr addr{};
        addr.op = UnaryOp::AddrOf;
        addr.operand = var_id;
        TypeSpec ptr_ts = decl_ts;
        ptr_ts.ptr_level++;
        c.args.push_back(append_expr(n, addr, ptr_ts));
        // Explicit args; handle reference params.
        for (int i = 0; i < n->n_children; ++i) {
          TypeSpec param_ts = best->method_node->params[i]->type;
          resolve_typedef_to_struct(param_ts);
          if (param_ts.is_rvalue_ref || param_ts.is_lvalue_ref) {
            // Reference parameter: pass address of the argument.
            const Node* arg = n->children[i];
            const Node* inner = arg;
            // Unwrap static_cast<T&&>(x) to get x.
            if (inner->kind == NK_CAST && inner->left) inner = inner->left;
            // Check if the argument is a function call returning a reference type.
            // In that case, the call already returns a pointer; no AddrOf needed.
            bool arg_returns_ref = false;
            if (auto ts = infer_call_result_type(&ctx, inner)) {
              arg_returns_ref = is_any_ref_ts(*ts);
            }
            if (arg_returns_ref) {
              // Call already returns a pointer (reference ABI).
              c.args.push_back(lower_expr(&ctx, arg));
            } else {
              ExprId arg_val = lower_expr(&ctx, inner);
              TypeSpec storage_ts = reference_storage_ts(param_ts);
              UnaryExpr addr_e{};
              addr_e.op = UnaryOp::AddrOf;
              addr_e.operand = arg_val;
              c.args.push_back(append_expr(arg, addr_e, storage_ts));
            }
          } else {
            c.args.push_back(lower_expr(&ctx, n->children[i]));
          }
        }
        ExprId call_id = append_expr(n, c, fn_ts);
        ExprStmt es{};
        es.expr = call_id;
        append_stmt(ctx, Stmt{StmtPayload{es}, make_span(n)});
      }
    }
  }

  // C++ implicit default constructor: T var; where T has a zero-arg ctor.
  if (!n->is_ctor_init && !n->init && !decl_struct_owner_tag.empty() &&
      decl_ts.base == TB_STRUCT && decl_ts.ptr_level == 0 &&
      decl_ts.array_rank == 0) {
    auto cit = struct_constructors_.find(decl_struct_owner_tag);
    if (cit != struct_constructors_.end()) {
      // Find a zero-arg constructor.
      const CtorOverload* default_ctor = nullptr;
      for (const auto& ov : cit->second) {
        if (ov.method_node->n_params == 0) {
          default_ctor = &ov;
          break;
        }
      }
      if (default_ctor) {
        if (default_ctor->method_node->is_deleted) {
          std::string diag = "error: call to deleted default constructor '";
          diag += decl_struct_owner_tag;
          diag += "'";
          throw std::runtime_error(diag);
        }
        CallExpr c{};
        DeclRef callee_ref{};
        callee_ref.name = default_ctor->mangled_name;
        callee_ref.name_text_id = make_text_id(
            callee_ref.name, module_ ? module_->link_name_texts.get() : nullptr);
        callee_ref.link_name_id = module_->link_names.find(callee_ref.name);
        TypeSpec fn_ts{};
        fn_ts.base = TB_VOID;
        TypeSpec callee_ts = fn_ts;
        callee_ts.ptr_level++;
        c.callee = append_expr(n, callee_ref, callee_ts);
        // First arg: &var (this pointer).
        DeclRef var_ref{};
        var_ref.name = n->name ? n->name : "<anon_local>";
        var_ref.name_text_id = make_unqualified_text_id(
            n, module_ ? module_->link_name_texts.get() : nullptr);
        var_ref.local = lid;
        ExprId var_id = append_expr(n, var_ref, decl_ts, ValueCategory::LValue);
        UnaryExpr addr{};
        addr.op = UnaryOp::AddrOf;
        addr.operand = var_id;
        TypeSpec ptr_ts = decl_ts;
        ptr_ts.ptr_level++;
        c.args.push_back(append_expr(n, addr, ptr_ts));
        ExprId call_id = append_expr(n, c, fn_ts);
        ExprStmt es{};
        es.expr = call_id;
        append_stmt(ctx, Stmt{StmtPayload{es}, make_span(n)});
      }
    }
  }

  // C++ copy initialization: T var = expr; -> call copy/move constructor.
  if (is_struct_copy_init) {
    auto cit = struct_constructors_.find(decl_struct_owner_tag);
    if (cit != struct_constructors_.end() && !cit->second.empty()) {
      // Score constructors: prefer T&& for rvalue, const T& for lvalue.
      bool init_is_lvalue = is_ast_lvalue(n->init, &ctx);
      const CtorOverload* best = nullptr;
      int best_score = -1;
      for (const auto& ov : cit->second) {
        if (ov.method_node->n_params != 1) continue;
        TypeSpec param_ts = ov.method_node->params[0]->type;
        resolve_typedef_to_struct(param_ts);
        const std::string param_owner_tag =
            resolve_decl_struct_owner_tag(
                param_ts,
                std::string("local-decl-copy-param-owner:") + d.name);
        if (param_owner_tag != decl_struct_owner_tag) continue;
        if (!param_ts.is_lvalue_ref && !param_ts.is_rvalue_ref) continue;
        int score = 0;
        if (param_ts.is_rvalue_ref && !init_is_lvalue) {
          score = 4;  // rvalue ref matches rvalue arg
        } else if (param_ts.is_rvalue_ref && init_is_lvalue) {
          continue;  // T&& cannot bind lvalue; skip
        } else if (param_ts.is_lvalue_ref && init_is_lvalue) {
          score = 3;  // const T& matches lvalue
        } else if (param_ts.is_lvalue_ref && !init_is_lvalue) {
          score = 1;  // const T& can bind rvalue (lower priority)
        }
        if (score > best_score) {
          best_score = score;
          best = &ov;
        }
      }
      if (best) {
        if (best->method_node->is_deleted) {
          std::string diag = "error: call to deleted constructor '";
          diag += decl_struct_owner_tag;
          diag += "'";
          throw std::runtime_error(diag);
        }
        CallExpr c{};
        DeclRef callee_ref{};
        callee_ref.name = best->mangled_name;
        callee_ref.name_text_id = make_text_id(
            callee_ref.name, module_ ? module_->link_name_texts.get() : nullptr);
        callee_ref.link_name_id = module_->link_names.find(callee_ref.name);
        TypeSpec fn_ts{};
        fn_ts.base = TB_VOID;
        TypeSpec callee_ts = fn_ts;
        callee_ts.ptr_level++;
        c.callee = append_expr(n, callee_ref, callee_ts);
        // First arg: &var (this pointer).
        DeclRef var_ref{};
        var_ref.name = n->name ? n->name : "<anon_local>";
        var_ref.name_text_id = make_unqualified_text_id(
            n, module_ ? module_->link_name_texts.get() : nullptr);
        var_ref.local = lid;
        ExprId var_id = append_expr(n, var_ref, decl_ts, ValueCategory::LValue);
        UnaryExpr addr{};
        addr.op = UnaryOp::AddrOf;
        addr.operand = var_id;
        TypeSpec ptr_ts = decl_ts;
        ptr_ts.ptr_level++;
        c.args.push_back(append_expr(n, addr, ptr_ts));
        // Second arg: the init expression (passed as &expr for ref param).
        TypeSpec param_ts = best->method_node->params[0]->type;
        resolve_typedef_to_struct(param_ts);
        if (param_ts.is_lvalue_ref || param_ts.is_rvalue_ref) {
          // Reference param: pass address of init expression.
          ExprId init_val = lower_expr(&ctx, n->init);
          UnaryExpr addr_e{};
          addr_e.op = UnaryOp::AddrOf;
          addr_e.operand = init_val;
          TypeSpec storage_ts = decl_ts;
          storage_ts.ptr_level++;
          c.args.push_back(append_expr(n->init, addr_e, storage_ts));
        } else {
          c.args.push_back(lower_expr(&ctx, n->init));
        }
        ExprId call_id = append_expr(n, c, fn_ts);
        ExprStmt es{};
        es.expr = call_id;
        append_stmt(ctx, Stmt{StmtPayload{es}, make_span(n)});
      }
    }
  }

  // Track struct locals with destructors for implicit scope-exit calls.
  // Also track structs whose members have destructors (even without explicit dtor).
  if (!decl_struct_owner_tag.empty() && decl_ts.base == TB_STRUCT && decl_ts.ptr_level == 0 &&
      decl_ts.array_rank == 0 && !n->type.is_lvalue_ref && !n->type.is_rvalue_ref) {
    auto dit = struct_destructors_.find(decl_struct_owner_tag);
    if (dit != struct_destructors_.end()) {
      if (dit->second.method_node->is_deleted) {
        std::string diag = "error: variable '";
        diag += n->name ? n->name : "<anon>";
        diag += "' of type '";
        diag += decl_struct_owner_tag;
        diag += "' has a deleted destructor";
        throw std::runtime_error(diag);
      }
      ctx.dtor_stack.push_back({lid, decl_struct_owner_tag});
    } else if (struct_has_member_dtors(decl_struct_owner_tag)) {
      ctx.dtor_stack.push_back({lid, decl_struct_owner_tag});
    }
  }

  auto emit_scalar_array_zero_fill = [&](const TypeSpec& array_ts) {
    if (array_ts.array_rank != 1 || array_ts.array_size <= 0) return;
    TypeSpec elem_ts = array_ts;
    elem_ts.array_rank--;
    elem_ts.array_size = -1;
    if ((elem_ts.base == TB_STRUCT || elem_ts.base == TB_UNION) &&
        elem_ts.ptr_level == 0 && elem_ts.array_rank == 0) {
      return;
    }
    for (long long idx = 0; idx < array_ts.array_size; ++idx) {
      DeclRef dr{};
      dr.name = n->name ? n->name : "<anon_local>";
      dr.name_text_id = make_unqualified_text_id(
          n, module_ ? module_->link_name_texts.get() : nullptr);
      dr.local = lid;
      ExprId dr_id = append_expr(n, dr, array_ts, ValueCategory::LValue);
      TypeSpec idx_ts{};
      idx_ts.base = TB_INT;
      ExprId idx_id = append_expr(n, IntLiteral{idx, false}, idx_ts);
      IndexExpr ie{};
      ie.base = dr_id;
      ie.index = idx_id;
      ExprId ie_id = append_expr(n, ie, elem_ts, ValueCategory::LValue);
      ExprId zero_id = append_expr(n, IntLiteral{0, false}, idx_ts);
      AssignExpr ae{};
      ae.lhs = ie_id;
      ae.rhs = zero_id;
      ExprId ae_id = append_expr(n, ae, elem_ts);
      ExprStmt es{};
      es.expr = ae_id;
      append_stmt(ctx, Stmt{StmtPayload{es}, make_span(n)});
    }
  };
  // For array init lists, emit element-by-element assignments.
  if (is_array_with_init_list && use_array_init_fast_path) {
    emit_scalar_array_zero_fill(decl_ts);
    long long next_idx = 0;
    for (int ci = 0; ci < n->init->n_children; ++ci) {
      const Node* item = n->init->children[ci];
      if (!item) {
        ++next_idx;
        continue;
      }
      long long idx = next_idx;
      const Node* val_node = item;
      if (item->kind == NK_INIT_ITEM) {
        if (item->is_designated && item->is_index_desig) idx = item->desig_val;
        val_node = init_item_value_node(item);
      }
      next_idx = idx + 1;
      if (!val_node) continue;
      // Build: x[idx] = val
      TypeSpec elem_ts = decl_ts;
      elem_ts.array_rank--;
      elem_ts.array_size = -1;
      // DeclRef to the local
      DeclRef dr{};
      dr.name = n->name ? n->name : "<anon_local>";
      dr.name_text_id = make_unqualified_text_id(
          n, module_ ? module_->link_name_texts.get() : nullptr);
      dr.local = lid;
      ExprId dr_id = append_expr(n, dr, decl_ts, ValueCategory::LValue);
      // IntLiteral for index
      TypeSpec idx_ts{};
      idx_ts.base = TB_INT;
      ExprId idx_id = append_expr(n, IntLiteral{idx, false}, idx_ts);
      // IndexExpr
      IndexExpr ie{};
      ie.base = dr_id;
      ie.index = idx_id;
      ExprId ie_id = append_expr(n, ie, elem_ts, ValueCategory::LValue);
      const bool elem_is_agg =
          (elem_ts.base == TB_STRUCT || elem_ts.base == TB_UNION) &&
          elem_ts.ptr_level == 0 && elem_ts.array_rank == 0;
      if (elem_is_agg) {
        const Node* scalar_node = val_node;
        if (scalar_node && scalar_node->kind == NK_COMPOUND_LIT) {
          const Node* init_list =
              (scalar_node->left && scalar_node->left->kind == NK_INIT_LIST)
                  ? scalar_node->left
                  : nullptr;
          if (!init_list && scalar_node->n_children > 0) {
            const Node* c0 = scalar_node->children[0];
            if (c0 && c0->kind == NK_INIT_ITEM) {
              const Node* vv = c0->left ? c0->left : c0->right;
              if (!vv) vv = c0->init;
              if (vv) c0 = vv;
            }
            if (c0) scalar_node = c0;
          } else if (init_list && init_list->n_children > 0) {
            const Node* c0 = init_list->children[0];
            if (c0 && c0->kind == NK_INIT_ITEM) {
              const Node* vv = c0->left ? c0->left : c0->right;
              if (!vv) vv = c0->init;
              if (vv) c0 = vv;
            }
            if (c0) scalar_node = c0;
          }
        }
        scalar_node = unwrap_init_scalar_value(scalar_node);
        bool direct_agg = false;
        if (scalar_node) {
          const TypeSpec st = infer_generic_ctrl_type(&ctx, scalar_node);
          if (st.ptr_level == 0 && st.array_rank == 0 &&
              st.base == elem_ts.base) {
            const std::string elem_owner_tag =
                resolve_decl_struct_owner_tag(
                    elem_ts, std::string("decl-array-elem-direct-owner:") + d.name);
            const std::string scalar_owner_tag =
                resolve_decl_struct_owner_tag(
                    st, std::string("decl-array-scalar-direct-owner:") + d.name);
            if (!elem_owner_tag.empty() || !scalar_owner_tag.empty()) {
              direct_agg = elem_owner_tag == scalar_owner_tag;
            } else {
              const char* et = elem_ts.tag ? elem_ts.tag : "";
              const char* stg = st.tag ? st.tag : "";
              direct_agg = std::strcmp(et, stg) == 0;
            }
          }
        }
        if (!direct_agg) {
          const auto [elem_owner_tag, elem_def] =
              resolve_decl_struct_def(
                  elem_ts, std::string("decl-array-elem-owner:") + d.name);
          if (elem_def && !elem_def->fields.empty() && scalar_node) {
            const HirStructField& fld = elem_def->fields[0];
            TypeSpec field_ts = field_type_of(fld);
            MemberExpr me{};
            me.base = ie_id;
            me.field = fld.name;
            me.field_text_id = make_text_id(
                me.field, module_ ? module_->link_name_texts.get() : nullptr);
            auto [owner_tag, member_symbol_id] =
                resolve_decl_member_symbol(elem_ts, me.field, fld.member_symbol_id);
            me.resolved_owner_tag = std::move(owner_tag);
            me.member_symbol_id = member_symbol_id;
            me.is_arrow = false;
            ExprId me_id = append_expr(n, me, field_ts, ValueCategory::LValue);
            ExprId val_id = lower_expr(&ctx, scalar_node);
            AssignExpr ae{};
            ae.lhs = me_id;
            ae.rhs = val_id;
            ExprId ae_id = append_expr(n, ae, field_ts);
            ExprStmt es{};
            es.expr = ae_id;
            append_stmt(ctx, Stmt{StmtPayload{es}, make_span(n)});
            continue;
          }
        }
      }

      ExprId val_id = lower_expr(&ctx, val_node);
      AssignExpr ae{};
      ae.lhs = ie_id;
      ae.rhs = val_id;
      ExprId ae_id = append_expr(n, ae, elem_ts);
      ExprStmt es{};
      es.expr = ae_id;
      append_stmt(ctx, Stmt{StmtPayload{es}, make_span(n)});
    }
  }
  if (is_array_with_string_init && n->init && n->init->sval) {
    emit_scalar_array_zero_fill(decl_ts);
    const bool is_wide = n->init->sval[0] == 'L';
    const auto vals = decode_string_literal_values(n->init->sval, is_wide);
    const long long max_count =
        decl_ts.array_size > 0 ? decl_ts.array_size : static_cast<long long>(vals.size());
    const long long emit_n =
        std::min<long long>(max_count, static_cast<long long>(vals.size()));
    for (long long idx = 0; idx < emit_n; ++idx) {
      TypeSpec elem_ts = decl_ts;
      elem_ts.array_rank--;
      elem_ts.array_size = -1;
      DeclRef dr{};
      dr.name = n->name ? n->name : "<anon_local>";
      dr.name_text_id = make_unqualified_text_id(
          n, module_ ? module_->link_name_texts.get() : nullptr);
      dr.local = lid;
      ExprId dr_id = append_expr(n, dr, decl_ts, ValueCategory::LValue);
      TypeSpec idx_ts{};
      idx_ts.base = TB_INT;
      ExprId idx_id = append_expr(n, IntLiteral{idx, false}, idx_ts);
      IndexExpr ie{};
      ie.base = dr_id;
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
      append_stmt(ctx, Stmt{StmtPayload{es}, make_span(n)});
    }
  }
  if (((is_struct_with_init_list && (!decl_struct_owner_tag.empty() || decl_ts.tag)) ||
       (is_array_with_init_list && !use_array_init_fast_path)) &&
      n->init) {
    auto is_agg = [](const TypeSpec& ts) {
      return (ts.base == TB_STRUCT || ts.base == TB_UNION) &&
             ts.ptr_level == 0 && ts.array_rank == 0;
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
          const long long max_count =
              lhs_ts.array_size > 0 ? lhs_ts.array_size : static_cast<long long>(vals.size());
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
            append_stmt(ctx, Stmt{StmtPayload{es}, make_span(n)});
          }
          return;
        }
      }
      const Node* val_node = unwrap_init_scalar_value(rhs_node);
      ExprId val_id = lower_expr(&ctx, val_node);
      AssignExpr ae{};
      ae.lhs = lhs_id;
      ae.rhs = val_id;
      ExprId ae_id = append_expr(n, ae, lhs_ts);
      ExprStmt es{};
      es.expr = ae_id;
      append_stmt(ctx, Stmt{StmtPayload{es}, make_span(n)});
    };
    auto can_direct_assign_agg = [&](const TypeSpec& lhs_ts,
                                     const Node* rhs_node) -> bool {
      if (!rhs_node) return false;
      if (!is_agg(lhs_ts)) return false;
      const TypeSpec rhs_ts = infer_generic_ctrl_type(&ctx, rhs_node);
      if (rhs_ts.ptr_level != 0 || rhs_ts.array_rank != 0) return false;
      if (rhs_ts.base != lhs_ts.base) return false;
      if (rhs_ts.base == TB_STRUCT || rhs_ts.base == TB_UNION || rhs_ts.base == TB_ENUM) {
        if (rhs_ts.base == TB_STRUCT || rhs_ts.base == TB_UNION) {
          const std::string lhs_owner_tag =
              resolve_decl_struct_owner_tag(lhs_ts, "decl-direct-assign-lhs-owner");
          const std::string rhs_owner_tag =
              resolve_decl_struct_owner_tag(rhs_ts, "decl-direct-assign-rhs-owner");
          if (!lhs_owner_tag.empty() || !rhs_owner_tag.empty()) {
            return lhs_owner_tag == rhs_owner_tag;
          }
        }
        const char* lt = lhs_ts.tag ? lhs_ts.tag : "";
        const char* rt = rhs_ts.tag ? rhs_ts.tag : "";
        return std::strcmp(lt, rt) == 0;
      }
      return true;
    };
    auto is_direct_char_array_init = [&](const TypeSpec& lhs_ts,
                                         const Node* rhs_node) -> bool {
      if (!rhs_node) return false;
      if (lhs_ts.array_rank != 1 || lhs_ts.ptr_level != 0) return false;
      if (!(lhs_ts.base == TB_CHAR || lhs_ts.base == TB_SCHAR || lhs_ts.base == TB_UCHAR))
        return false;
      return rhs_node->kind == NK_STR_LIT;
    };
    auto append_base_lvalue = [&](const TypeSpec& owner_ts, ExprId owner_lhs,
                                  const std::string& base_tag)
        -> std::optional<std::pair<TypeSpec, ExprId>> {
      const auto [owner_tag, owner_def] =
          resolve_decl_struct_def(owner_ts, "decl-base-owner");
      if (!owner_def) return std::nullopt;
      if (owner_def->base_tags.empty() || owner_def->base_tags.front() != base_tag)
        return std::nullopt;
      TypeSpec owner_ptr_ts = owner_ts;
      owner_ptr_ts.ptr_level++;
      UnaryExpr addr{};
      addr.op = UnaryOp::AddrOf;
      addr.operand = owner_lhs;
      ExprId owner_addr = append_expr(n, addr, owner_ptr_ts);

      TypeSpec base_ptr_ts{};
      base_ptr_ts.base = TB_STRUCT;
      base_ptr_ts.tag = base_tag.c_str();
      base_ptr_ts.ptr_level = 1;
      CastExpr cast{};
      cast.to_type = qtype_from(base_ptr_ts, ValueCategory::RValue);
      cast.expr = owner_addr;
      ExprId base_ptr = append_expr(n, cast, base_ptr_ts);

      TypeSpec base_ts = base_ptr_ts;
      base_ts.ptr_level = 0;
      UnaryExpr deref{};
      deref.op = UnaryOp::Deref;
      deref.operand = base_ptr;
      ExprId base_lhs = append_expr(n, deref, base_ts, ValueCategory::LValue);
      return std::make_pair(base_ts, base_lhs);
    };

    std::function<void(const TypeSpec&, ExprId, const Node*, int&)> consume_from_list;
    consume_from_list =
        [&](const TypeSpec& cur_ts, ExprId cur_lhs, const Node* list_node, int& cursor) {
          if (!list_node || list_node->kind != NK_INIT_LIST) return;
          if (cursor < 0) cursor = 0;

          if (is_agg(cur_ts)) {
            const auto [cur_owner_tag, cur_def] =
                resolve_decl_struct_def(cur_ts, "decl-aggregate-owner");
            if (!cur_def) return;
            const auto& sd = *cur_def;
            size_t next_base = 0;
            size_t next_field = 0;
            while (cursor < list_node->n_children) {
              const Node* item = list_node->children[cursor];
              if (!item) {
                ++cursor;
                if (next_base < sd.base_tags.size()) {
                  ++next_base;
                  continue;
                }
                if (next_field < sd.fields.size()) ++next_field;
                continue;
              }
              const bool has_field_designator =
                  item->kind == NK_INIT_ITEM && item->is_designated &&
                  !item->is_index_desig && item->desig_field;
              if (!has_field_designator && next_base < sd.base_tags.size()) {
                const std::string& base_tag = sd.base_tags[next_base];
                auto base_access = append_base_lvalue(cur_ts, cur_lhs, base_tag);
                ++next_base;
                if (!base_access) break;
                const TypeSpec& base_ts = base_access->first;
                ExprId base_lhs = base_access->second;
                const Node* val_node = init_item_value_node(item);
                if (is_agg(base_ts) || base_ts.array_rank > 0) {
                  if (val_node && val_node->kind == NK_INIT_LIST) {
                    int sub_cursor = 0;
                    consume_from_list(base_ts, base_lhs, val_node, sub_cursor);
                    ++cursor;
                  } else if (can_direct_assign_agg(base_ts, val_node)) {
                    append_assign(base_lhs, base_ts, val_node);
                    ++cursor;
                  } else {
                    consume_from_list(base_ts, base_lhs, list_node, cursor);
                  }
                } else {
                  append_assign(base_lhs, base_ts, val_node);
                  ++cursor;
                }
                continue;
              }
              if (!has_field_designator && next_field >= sd.fields.size()) break;
              size_t fi = next_field;
              if (has_field_designator) {
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
              auto [owner_tag, member_symbol_id] =
                  resolve_decl_member_symbol(cur_ts, me.field, fld.member_symbol_id);
              me.resolved_owner_tag = std::move(owner_tag);
              me.member_symbol_id = member_symbol_id;
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
              if (item->kind == NK_INIT_ITEM && item->is_designated &&
                  item->is_index_desig) {
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

    DeclRef dr{};
    dr.name = n->name ? n->name : "<anon_local>";
    dr.name_text_id = make_unqualified_text_id(
        n, module_ ? module_->link_name_texts.get() : nullptr);
    dr.local = lid;
    ExprId base_id = append_expr(n, dr, decl_ts, ValueCategory::LValue);
    int cursor = 0;
    consume_from_list(decl_ts, base_id, n->init, cursor);
  }
}

}  // namespace c4c::hir
