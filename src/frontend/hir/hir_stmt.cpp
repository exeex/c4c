#include "hir_lowerer_internal.hpp"
#include "consteval.hpp"

namespace c4c::hir {

void Lowerer::lower_local_decl_stmt(FunctionCtx& ctx, const Node* n) {
  // Local function prototype (e.g. `int f1(char *);` inside a function body):
  // if the name is already registered as a known function, skip creating a
  // local alloca; later references will resolve directly to the global function.
  // Require n_params > 0 to distinguish from a plain variable declaration
  // whose name coincidentally matches a function name.
  if (n->name && !n->init && n->n_params > 0 && module_->fn_index.count(n->name)) return;

  // Local extern declaration: `extern T v;` inside a function refers to
  // the global with that name (C99 6.2.2p4). Erase any shadowing local
  // binding so the global is found by the NK_VAR resolution fallback.
  if (n->is_extern && !n->init && n->name && n->name[0]) {
    const auto git = module_->global_index.find(n->name);
    if (git != module_->global_index.end()) {
      ctx.locals.erase(n->name);
      ctx.static_globals[n->name] = git->second;
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
  // Substitute template type parameters in local variable types.
  {
    TypeSpec decl_ts =
        substitute_signature_template_type(n->type, &ctx.tpl_bindings);
    // Resolve pending template struct types in local variable decls.
    seed_and_resolve_pending_template_type_if_needed(
        decl_ts, ctx.tpl_bindings, ctx.nttp_bindings, n,
        PendingTemplateTypeKind::DeclarationType,
        std::string("local-decl:") + d.name);
    resolve_typedef_to_struct(decl_ts);
    d.type = qtype_from(reference_storage_ts(decl_ts), ValueCategory::LValue);
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
  // C++ copy initialization: T var = expr; where T has a copy/move constructor.
  // Detect this early so we can skip setting d.init (the ctor call is emitted
  // after the decl, similar to the is_ctor_init path).
  bool is_struct_copy_init = false;
  if (n->init && !n->is_ctor_init && n->init->kind != NK_INIT_LIST &&
      d.type.spec.base == TB_STRUCT && d.type.spec.ptr_level == 0 &&
      d.type.spec.array_rank == 0 && d.type.spec.tag &&
      !is_lvalue_ref_ts(n->type) && !n->type.is_rvalue_ref) {
    auto cit = struct_constructors_.find(d.type.spec.tag);
    if (cit != struct_constructors_.end()) {
      // Check if any constructor takes a single param of same struct type (copy/move ctor).
      for (const auto& ov : cit->second) {
        if (ov.method_node->n_params == 1) {
          TypeSpec param_ts = ov.method_node->params[0]->type;
          resolve_typedef_to_struct(param_ts);
          if (param_ts.tag && strcmp(param_ts.tag, d.type.spec.tag) == 0 &&
              (param_ts.is_lvalue_ref || param_ts.is_rvalue_ref)) {
            is_struct_copy_init = true;
            break;
          }
        }
      }
    }
  }
  if (is_lvalue_ref_ts(n->type) && n->init) {
    UnaryExpr addr{};
    addr.op = UnaryOp::AddrOf;
    addr.operand = lower_expr(&ctx, n->init);
    d.init = append_expr(n->init, addr, d.type.spec);
  } else if (n->type.is_rvalue_ref && n->init) {
    if (auto storage_addr =
            try_lower_rvalue_ref_storage_addr(&ctx, n->init, d.type.spec)) {
      d.init = *storage_addr;
    } else {
      // Rvalue reference: materialize the rvalue into a temporary, then
      // store a pointer to that temporary as the reference value.
      ExprId init_val = lower_expr(&ctx, n->init);
      TypeSpec val_ts = reference_value_ts(n->type);
      // Create a hidden temporary local for the rvalue
      LocalDecl tmp{};
      tmp.id = next_local_id();
      tmp.name = ("__rref_tmp_" + std::to_string(d.id.value)).c_str();
      tmp.type = qtype_from(val_ts, ValueCategory::LValue);
      tmp.init = init_val;
      const LocalId tmp_lid = tmp.id;
      ctx.locals[tmp.name] = tmp.id;
      ctx.local_types[tmp.id.value] = val_ts;
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
  ctx.local_types[d.id.value] = d.type.spec;
  // Track const/constexpr locals with foldable int initializers.
  if (n->name && n->name[0] && n->init &&
      (n->type.is_const || n->is_constexpr) &&
      !n->type.is_lvalue_ref && !n->type.is_rvalue_ref &&
      n->type.ptr_level == 0 && n->type.array_rank == 0) {
    ConstEvalEnv cenv{&enum_consts_, &const_int_bindings_, &ctx.local_const_bindings};
    if (auto cr = evaluate_constant_expr(n->init, cenv); cr.ok()) {
      ctx.local_const_bindings[n->name] = cr.as_int();
    }
  }
  append_stmt(ctx, Stmt{StmtPayload{std::move(d)}, make_span(n)});

  // C++ constructor invocation: Type var(args) -> call StructTag__StructTag(&var, args...)
  if (n->is_ctor_init && decl_ts.tag) {
    auto cit = struct_constructors_.find(decl_ts.tag);
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
          diag += decl_ts.tag;
          diag += "'";
          throw std::runtime_error(diag);
        }
        CallExpr c{};
        DeclRef callee_ref{};
        callee_ref.name = best->mangled_name;
        TypeSpec fn_ts{};
        fn_ts.base = TB_VOID;
        TypeSpec callee_ts = fn_ts;
        callee_ts.ptr_level++;
        c.callee = append_expr(n, callee_ref, callee_ts);
        // First arg: &var (this pointer).
        DeclRef var_ref{};
        var_ref.name = n->name ? n->name : "<anon_local>";
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
            if (inner->kind == NK_CALL && inner->left &&
                inner->left->kind == NK_VAR && inner->left->name) {
              TypeSpec call_ret = infer_generic_ctrl_type(&ctx, arg);
              (void)call_ret;
              // infer_generic_ctrl_type strips refs, so check original return type.
              auto dit = deduced_template_calls_.find(inner);
              if (dit != deduced_template_calls_.end()) {
                auto fit = module_->fn_index.find(dit->second.mangled_name);
                if (fit != module_->fn_index.end()) {
                  const Function* fn = module_->find_function(fit->second);
                  if (fn && is_any_ref_ts(fn->return_type.spec)) arg_returns_ref = true;
                }
              }
              if (!arg_returns_ref) {
                auto fit = module_->fn_index.find(inner->left->name);
                if (fit != module_->fn_index.end()) {
                  const Function* fn = module_->find_function(fit->second);
                  if (fn && is_any_ref_ts(fn->return_type.spec)) arg_returns_ref = true;
                }
              }
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
  if (!n->is_ctor_init && !n->init && decl_ts.tag &&
      decl_ts.base == TB_STRUCT && decl_ts.ptr_level == 0 &&
      decl_ts.array_rank == 0) {
    auto cit = struct_constructors_.find(decl_ts.tag);
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
          diag += decl_ts.tag;
          diag += "'";
          throw std::runtime_error(diag);
        }
        CallExpr c{};
        DeclRef callee_ref{};
        callee_ref.name = default_ctor->mangled_name;
        TypeSpec fn_ts{};
        fn_ts.base = TB_VOID;
        TypeSpec callee_ts = fn_ts;
        callee_ts.ptr_level++;
        c.callee = append_expr(n, callee_ref, callee_ts);
        // First arg: &var (this pointer).
        DeclRef var_ref{};
        var_ref.name = n->name ? n->name : "<anon_local>";
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
    auto cit = struct_constructors_.find(decl_ts.tag);
    if (cit != struct_constructors_.end() && !cit->second.empty()) {
      // Score constructors: prefer T&& for rvalue, const T& for lvalue.
      bool init_is_lvalue = is_ast_lvalue(n->init, &ctx);
      const CtorOverload* best = nullptr;
      int best_score = -1;
      for (const auto& ov : cit->second) {
        if (ov.method_node->n_params != 1) continue;
        TypeSpec param_ts = ov.method_node->params[0]->type;
        resolve_typedef_to_struct(param_ts);
        if (!param_ts.tag || strcmp(param_ts.tag, decl_ts.tag) != 0) continue;
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
          diag += decl_ts.tag;
          diag += "'";
          throw std::runtime_error(diag);
        }
        CallExpr c{};
        DeclRef callee_ref{};
        callee_ref.name = best->mangled_name;
        TypeSpec fn_ts{};
        fn_ts.base = TB_VOID;
        TypeSpec callee_ts = fn_ts;
        callee_ts.ptr_level++;
        c.callee = append_expr(n, callee_ref, callee_ts);
        // First arg: &var (this pointer).
        DeclRef var_ref{};
        var_ref.name = n->name ? n->name : "<anon_local>";
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
  if (decl_ts.tag && decl_ts.base == TB_STRUCT && decl_ts.ptr_level == 0 &&
      decl_ts.array_rank == 0 && !n->type.is_lvalue_ref && !n->type.is_rvalue_ref) {
    auto dit = struct_destructors_.find(decl_ts.tag);
    if (dit != struct_destructors_.end()) {
      if (dit->second.method_node->is_deleted) {
        std::string diag = "error: variable '";
        diag += n->name ? n->name : "<anon>";
        diag += "' of type '";
        diag += decl_ts.tag;
        diag += "' has a deleted destructor";
        throw std::runtime_error(diag);
      }
      ctx.dtor_stack.push_back({lid, std::string(decl_ts.tag)});
    } else if (struct_has_member_dtors(decl_ts.tag)) {
      ctx.dtor_stack.push_back({lid, std::string(decl_ts.tag)});
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
      if (elem_is_agg && elem_ts.tag) {
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
            const char* et = elem_ts.tag ? elem_ts.tag : "";
            const char* stg = st.tag ? st.tag : "";
            direct_agg = std::strcmp(et, stg) == 0;
          }
        }
        if (!direct_agg) {
          const auto sit = module_->struct_defs.find(elem_ts.tag);
          if (sit != module_->struct_defs.end() && !sit->second.fields.empty() &&
              scalar_node) {
            const HirStructField& fld = sit->second.fields[0];
            TypeSpec field_ts = field_type_of(fld);
            MemberExpr me{};
            me.base = ie_id;
            me.field = fld.name;
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
  if (((is_struct_with_init_list && decl_ts.tag) ||
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

    std::function<void(const TypeSpec&, ExprId, const Node*, int&)> consume_from_list;
    consume_from_list =
        [&](const TypeSpec& cur_ts, ExprId cur_lhs, const Node* list_node, int& cursor) {
          if (!list_node || list_node->kind != NK_INIT_LIST) return;
          if (cursor < 0) cursor = 0;

          if (is_agg(cur_ts) && cur_ts.tag) {
            const auto sit = module_->struct_defs.find(cur_ts.tag);
            if (sit == module_->struct_defs.end()) return;
            const auto& sd = sit->second;
            size_t next_field = 0;
            while (cursor < list_node->n_children) {
              const Node* item = list_node->children[cursor];
              if (!item) {
                ++cursor;
                if (next_field < sd.fields.size()) ++next_field;
                continue;
              }
              const bool has_field_designator =
                  item->kind == NK_INIT_ITEM && item->is_designated &&
                  !item->is_index_desig && item->desig_field;
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
    dr.local = lid;
    ExprId base_id = append_expr(n, dr, decl_ts, ValueCategory::LValue);
    int cursor = 0;
    consume_from_list(decl_ts, base_id, n->init, cursor);
  }
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

void Lowerer::emit_defaulted_method_body(FunctionCtx& ctx,
                                         Function& fn,
                                         const std::string& struct_tag,
                                         const Node* method_node) {
  auto sit = module_->struct_defs.find(struct_tag);
  bool is_copy_or_move_ctor = method_node->is_constructor && method_node->n_params == 1;
  bool is_copy_or_move_assign =
      (method_node->operator_kind == OP_ASSIGN) && method_node->n_params == 1;

  if (is_copy_or_move_ctor || is_copy_or_move_assign) {
    if (sit != module_->struct_defs.end()) {
      DeclRef this_ref{};
      this_ref.name = "this";
      auto pit = ctx.params.find("this");
      if (pit != ctx.params.end()) this_ref.param_index = pit->second;
      TypeSpec this_ts{};
      this_ts.base = TB_STRUCT;
      this_ts.tag = sit->second.tag.c_str();
      this_ts.ptr_level = 1;
      ExprId this_id = append_expr(method_node, this_ref, this_ts, ValueCategory::LValue);

      std::string other_name =
          method_node->params[0]->name ? method_node->params[0]->name : "<anon_param>";
      DeclRef other_ref{};
      other_ref.name = other_name;
      auto opit = ctx.params.find(other_name);
      if (opit != ctx.params.end()) other_ref.param_index = opit->second;
      TypeSpec other_ts = this_ts;
      ExprId other_id =
          append_expr(method_node, other_ref, other_ts, ValueCategory::LValue);

      for (const auto& field : sit->second.fields) {
        TypeSpec field_ts = field.elem_type;
        if (field.array_first_dim >= 0) {
          field_ts.array_rank = 1;
          field_ts.array_size = field.array_first_dim;
        }

        MemberExpr lhs_me{};
        lhs_me.base = this_id;
        lhs_me.field = field.name;
        lhs_me.is_arrow = true;
        ExprId lhs_member =
            append_expr(method_node, lhs_me, field_ts, ValueCategory::LValue);

        MemberExpr rhs_me{};
        rhs_me.base = other_id;
        rhs_me.field = field.name;
        rhs_me.is_arrow = true;
        ExprId rhs_member =
            append_expr(method_node, rhs_me, field_ts, ValueCategory::LValue);

        AssignExpr ae{};
        ae.lhs = lhs_member;
        ae.rhs = rhs_member;
        ExprId assign_id = append_expr(method_node, ae, field_ts);
        ExprStmt es{};
        es.expr = assign_id;
        append_stmt(ctx, Stmt{StmtPayload{es}, make_span(method_node)});
      }
    }
  }

  if (is_copy_or_move_assign) {
    DeclRef this_ref2{};
    this_ref2.name = "this";
    auto pit2 = ctx.params.find("this");
    if (pit2 != ctx.params.end()) this_ref2.param_index = pit2->second;
    TypeSpec this_ts2{};
    this_ts2.base = TB_STRUCT;
    this_ts2.tag =
        sit != module_->struct_defs.end() ? sit->second.tag.c_str() : struct_tag.c_str();
    this_ts2.ptr_level = 1;
    ExprId this_ret =
        append_expr(method_node, this_ref2, this_ts2, ValueCategory::LValue);
    ReturnStmt rs{};
    rs.expr = this_ret;
    append_stmt(ctx, Stmt{StmtPayload{rs}, make_span(method_node)});
  } else if (!method_node->is_destructor) {
    ReturnStmt rs{};
    append_stmt(ctx, Stmt{StmtPayload{rs}, make_span(method_node)});
  }
}

void Lowerer::emit_member_dtor_calls(FunctionCtx& ctx,
                                     const std::string& struct_tag,
                                     ExprId this_ptr_id,
                                     const Node* span_node) {
  auto sit = module_->struct_defs.find(struct_tag);
  if (sit == module_->struct_defs.end()) return;
  const auto& fields = sit->second.fields;
  for (auto it = fields.rbegin(); it != fields.rend(); ++it) {
    const auto& field = *it;
    if (field.elem_type.base != TB_STRUCT || field.elem_type.ptr_level != 0 ||
        !field.elem_type.tag) {
      continue;
    }
    std::string ftag = field.elem_type.tag;
    bool has_explicit_dtor = struct_destructors_.count(ftag) > 0;
    bool has_member_dtors = struct_has_member_dtors(ftag);
    if (!has_explicit_dtor && !has_member_dtors) continue;

    MemberExpr me{};
    me.base = this_ptr_id;
    me.field = field.name;
    me.is_arrow = true;
    TypeSpec field_ts = field.elem_type;
    ExprId member_id = append_expr(span_node, me, field_ts, ValueCategory::LValue);
    UnaryExpr addr{};
    addr.op = UnaryOp::AddrOf;
    addr.operand = member_id;
    TypeSpec ptr_ts = field_ts;
    ptr_ts.ptr_level++;
    ExprId member_ptr_id = append_expr(span_node, addr, ptr_ts);

    if (has_explicit_dtor) {
      auto dit = struct_destructors_.find(ftag);
      CallExpr c{};
      DeclRef callee_ref{};
      callee_ref.name = dit->second.mangled_name;
      TypeSpec fn_ts{};
      fn_ts.base = TB_VOID;
      TypeSpec callee_ts = fn_ts;
      callee_ts.ptr_level++;
      c.callee = append_expr(span_node, callee_ref, callee_ts);
      c.args.push_back(member_ptr_id);
      ExprId call_id = append_expr(span_node, c, fn_ts);
      ExprStmt es{};
      es.expr = call_id;
      append_stmt(ctx, Stmt{StmtPayload{es}, make_span(span_node)});
    } else {
      emit_member_dtor_calls(ctx, ftag, member_ptr_id, span_node);
    }
  }
}

void Lowerer::emit_dtor_calls(FunctionCtx& ctx, size_t since, const Node* span_node) {
  for (size_t i = ctx.dtor_stack.size(); i > since; --i) {
    const auto& dl = ctx.dtor_stack[i - 1];
    auto dit = struct_destructors_.find(dl.struct_tag);

    DeclRef var_ref{};
    var_ref.local = dl.local_id;
    auto lt = ctx.local_types.find(dl.local_id.value);
    TypeSpec var_ts{};
    if (lt != ctx.local_types.end()) var_ts = lt->second;
    ExprId var_id = append_expr(span_node, var_ref, var_ts, ValueCategory::LValue);
    UnaryExpr addr{};
    addr.op = UnaryOp::AddrOf;
    addr.operand = var_id;
    TypeSpec ptr_ts = var_ts;
    ptr_ts.ptr_level++;
    ExprId addr_id = append_expr(span_node, addr, ptr_ts);

    if (dit != struct_destructors_.end()) {
      CallExpr c{};
      DeclRef callee_ref{};
      callee_ref.name = dit->second.mangled_name;
      TypeSpec fn_ts{};
      fn_ts.base = TB_VOID;
      TypeSpec callee_ts = fn_ts;
      callee_ts.ptr_level++;
      c.callee = append_expr(span_node, callee_ref, callee_ts);
      c.args.push_back(addr_id);
      ExprId call_id = append_expr(span_node, c, fn_ts);
      ExprStmt es{};
      es.expr = call_id;
      append_stmt(ctx, Stmt{StmtPayload{es}, make_span(span_node)});
    } else {
      emit_member_dtor_calls(ctx, dl.struct_tag, addr_id, span_node);
    }
  }
}

void Lowerer::lower_stmt_node(FunctionCtx& ctx, const Node* n) {
  if (!n) return;

  switch (n->kind) {
    case NK_BLOCK: {
      // ival==1 marks synthetic multi-declarator blocks (e.g. int a, b;).
      // They share the current scope and must not discard bindings.
      const bool new_scope = (n->ival != 1);
      const auto saved_locals = ctx.locals;
      const auto saved_static_globals = ctx.static_globals;
      const auto saved_enum_consts = enum_consts_;
      const auto saved_local_consts = ctx.local_const_bindings;
      const size_t saved_dtor_depth = ctx.dtor_stack.size();
      for (int i = 0; i < n->n_children; ++i) {
        lower_stmt_node(ctx, n->children[i]);
      }
      if (new_scope) {
        emit_dtor_calls(ctx, saved_dtor_depth, n);
        ctx.dtor_stack.resize(saved_dtor_depth);
        ctx.locals = saved_locals;
        ctx.static_globals = saved_static_globals;
        enum_consts_ = saved_enum_consts;
        ctx.local_const_bindings = saved_local_consts;
      }
      return;
    }
    case NK_DECL: {
      lower_local_decl_stmt(ctx, n);
      return;
    }
    case NK_EXPR_STMT: {
      ExprStmt s{};
      if (n->left) s.expr = lower_expr(&ctx, n->left);
      append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
      return;
    }
    case NK_ASM: {
      InlineAsmStmt s{};
      s.asm_template = rewrite_gcc_asm_template(decode_string_node(n->left));
      s.has_side_effects = true;
      if (n->asm_num_outputs == 1 && n->children[0]) {
        s.output = lower_expr(&ctx, n->children[0]);
        s.output_type.spec = n->children[0]->type;
        s.output_type.category = ValueCategory::LValue;
      }
      for (int i = 0; i < n->asm_num_inputs; ++i) {
        const Node* input = n->children[n->asm_num_outputs + i];
        if (!input) continue;
        s.inputs.push_back(lower_expr(&ctx, input));
      }
      for (int i = 0; i < n->asm_n_constraints; ++i) {
        const std::string constraint = strip_quoted_string(n->asm_constraints[i]);
        if (!s.constraints.empty()) s.constraints += ",";
        s.constraints += constraint;
      }
      for (int i = 0; i < n->asm_num_clobbers; ++i) {
        const Node* clobber =
            n->children[n->asm_num_outputs + n->asm_num_inputs + i];
        std::string name = decode_string_node(clobber);
        if (name.empty()) continue;
        if (!s.constraints.empty()) s.constraints += ",";
        s.constraints += "~{" + name + "}";
      }
      append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
      return;
    }
    case NK_RETURN: {
      ReturnStmt s{};
      if (n->left) {
        ExprId val = lower_expr(&ctx, n->left);
        if (ctx.fn && is_any_ref_ts(ctx.fn->return_type.spec)) {
          UnaryExpr addr{};
          addr.op = UnaryOp::AddrOf;
          addr.operand = val;
          TypeSpec ptr_ts = ctx.fn->return_type.spec;
          ptr_ts.is_lvalue_ref = false;
          ptr_ts.is_rvalue_ref = false;
          ptr_ts.ptr_level++;
          val = append_expr(n, addr, ptr_ts);
        }
        s.expr = val;
      }
      emit_dtor_calls(ctx, 0, n);
      append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
      ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
      return;
    }
    case NK_IF: {
      if (n->is_constexpr) {
        const Node* cond_node = n->cond ? n->cond : n->left;
        ConstEvalEnv cenv{&enum_consts_, &const_int_bindings_, &ctx.local_const_bindings};
        auto cr = evaluate_constant_expr(cond_node, cenv);
        if (cr.ok()) {
          if (cr.as_int()) {
            lower_stmt_node(ctx, n->then_);
          } else if (n->else_) {
            lower_stmt_node(ctx, n->else_);
          }
          return;
        }
      }

      IfStmt s{};
      {
        const Node* cond_n = n->cond ? n->cond : n->left;
        s.cond = maybe_bool_convert(&ctx, lower_expr(&ctx, cond_n), cond_n);
      }
      const BlockId then_b = create_block(ctx);
      s.then_block = then_b;
      if (n->else_) s.else_block = create_block(ctx);
      const BlockId after_b = create_block(ctx);
      s.after_block = after_b;
      append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
      ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;

      ctx.current_block = then_b;
      lower_stmt_node(ctx, n->then_);
      if (!ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
        GotoStmt j{};
        j.target.resolved_block = after_b;
        append_stmt(ctx, Stmt{StmtPayload{j}, make_span(n)});
        ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
      }

      if (n->else_) {
        ctx.current_block = *s.else_block;
        lower_stmt_node(ctx, n->else_);
        if (!ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
          GotoStmt j{};
          j.target.resolved_block = after_b;
          append_stmt(ctx, Stmt{StmtPayload{j}, make_span(n)});
          ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
        }
      }

      ctx.current_block = after_b;
      return;
    }
    case NK_WHILE: {
      const BlockId cond_b = create_block(ctx);
      const BlockId body_b = create_block(ctx);
      const BlockId after_b = create_block(ctx);

      if (!ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
        GotoStmt j{};
        j.target.resolved_block = cond_b;
        append_stmt(ctx, Stmt{StmtPayload{j}, make_span(n)});
        ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
      }

      ctx.current_block = cond_b;
      WhileStmt s{};
      {
        const Node* cond_n = n->cond ? n->cond : n->left;
        s.cond = maybe_bool_convert(&ctx, lower_expr(&ctx, cond_n), cond_n);
      }
      s.body_block = body_b;
      s.continue_target = cond_b;
      s.break_target = after_b;
      append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
      ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;

      ctx.break_stack.push_back(after_b);
      ctx.continue_stack.push_back(cond_b);
      ctx.current_block = body_b;
      lower_stmt_node(ctx, n->body);
      if (ctx.current_block.value != after_b.value &&
          !ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
        ContinueStmt c{};
        c.target = cond_b;
        append_stmt(ctx, Stmt{StmtPayload{c}, make_span(n)});
        ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
      }
      ctx.break_stack.pop_back();
      ctx.continue_stack.pop_back();

      ctx.current_block = after_b;
      return;
    }
    case NK_FOR: {
      ForStmt s{};
      if (n->init) {
        const bool init_is_decl = (n->init->kind == NK_DECL ||
                                   n->init->kind == NK_BLOCK);
        if (init_is_decl) {
          lower_stmt_node(ctx, n->init);
        } else {
          s.init = lower_expr(&ctx, n->init);
        }
      }
      if (n->cond) s.cond = maybe_bool_convert(&ctx, lower_expr(&ctx, n->cond), n->cond);
      if (n->update) s.update = lower_expr(&ctx, n->update);
      const BlockId body_b = create_block(ctx);
      const BlockId after_b = create_block(ctx);
      s.body_block = body_b;
      s.continue_target = body_b;
      s.break_target = after_b;
      append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
      ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;

      ctx.break_stack.push_back(after_b);
      ctx.continue_stack.push_back(body_b);
      ctx.current_block = body_b;
      lower_stmt_node(ctx, n->body);
      if (ctx.current_block.value != after_b.value &&
          !ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
        ContinueStmt c{};
        c.target = body_b;
        append_stmt(ctx, Stmt{StmtPayload{c}, make_span(n)});
        ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
      }
      ctx.break_stack.pop_back();
      ctx.continue_stack.pop_back();

      ctx.current_block = after_b;
      return;
    }
    case NK_RANGE_FOR: {
      const Node* decl_node = n->init;
      const Node* range_node = n->right;

      TypeSpec range_ts = infer_generic_ctrl_type(&ctx, range_node);
      if (range_ts.base != TB_STRUCT || !range_ts.tag) {
        throw std::runtime_error(
            std::string("error: range-for expression is not a struct type (line ") +
            std::to_string(n->line) + ")");
      }

      auto find_method = [&](const char* method_name) -> std::string {
        std::string base_key = std::string(range_ts.tag) + "::" + method_name;
        std::string const_key = base_key + "_const";
        decltype(struct_methods_)::iterator mit;
        if (range_ts.is_const) {
          mit = struct_methods_.find(const_key);
          if (mit == struct_methods_.end()) mit = struct_methods_.find(base_key);
        } else {
          mit = struct_methods_.find(base_key);
          if (mit == struct_methods_.end()) mit = struct_methods_.find(const_key);
        }
        if (mit == struct_methods_.end()) {
          throw std::runtime_error(
              std::string("error: range-for: no ") + method_name +
              "() method on struct " + range_ts.tag + " (line " +
              std::to_string(n->line) + ")");
        }
        return mit->second;
      };

      std::string begin_mangled = find_method("begin");
      std::string end_mangled = find_method("end");

      TypeSpec iter_ts{};
      iter_ts.base = TB_VOID;
      {
        auto fit = module_->fn_index.find(begin_mangled);
        if (fit != module_->fn_index.end() &&
            fit->second.value < module_->functions.size()) {
          iter_ts = module_->functions[fit->second.value].return_type.spec;
        }
      }
      if (iter_ts.base == TB_VOID) {
        auto rit = struct_method_ret_types_.find(std::string(range_ts.tag) + "::begin");
        if (rit == struct_method_ret_types_.end()) {
          rit = struct_method_ret_types_.find(std::string(range_ts.tag) + "::begin_const");
        }
        if (rit != struct_method_ret_types_.end()) iter_ts = rit->second;
      }
      if (iter_ts.base == TB_VOID) {
        throw std::runtime_error(
            std::string("error: range-for: cannot determine iterator type from begin() (line ") +
            std::to_string(n->line) + ")");
      }

      ExprId range_id = lower_expr(&ctx, range_node);

      auto build_method_call = [&](const std::string& mangled, ExprId obj_id) -> ExprId {
        CallExpr cc{};
        DeclRef dr{};
        dr.name = mangled;
        TypeSpec callee_ts = iter_ts;
        callee_ts.ptr_level++;
        cc.callee = append_expr(n, dr, callee_ts);
        UnaryExpr addr{};
        addr.op = UnaryOp::AddrOf;
        addr.operand = obj_id;
        TypeSpec ptr_ts = range_ts;
        ptr_ts.ptr_level++;
        cc.args.push_back(append_expr(n, addr, ptr_ts));
        return append_expr(n, cc, iter_ts);
      };

      ExprId begin_call = build_method_call(begin_mangled, range_id);
      ExprId end_call = build_method_call(end_mangled, range_id);

      auto make_iter_local = [&](const char* name, ExprId init_expr) -> LocalId {
        LocalDecl ld{};
        ld.id = next_local_id();
        ld.name = name;
        ld.type = qtype_from(iter_ts, ValueCategory::LValue);
        ld.init = init_expr;
        LocalId lid = ld.id;
        ctx.locals[name] = lid;
        ctx.local_types[lid.value] = iter_ts;
        append_stmt(ctx, Stmt{StmtPayload{std::move(ld)}, make_span(n)});
        return lid;
      };

      LocalId begin_lid = make_iter_local("__range_begin", begin_call);
      LocalId end_lid = make_iter_local("__range_end", end_call);

      auto ref_local = [&](const char* name, LocalId lid) -> ExprId {
        DeclRef dr{};
        dr.name = name;
        dr.local = lid;
        return append_expr(n, dr, iter_ts, ValueCategory::LValue);
      };

      ExprId cond_expr;
      {
        std::string neq_base = std::string(iter_ts.tag) + "::operator_neq";
        std::string neq_const = neq_base + "_const";
        auto mit = struct_methods_.find(neq_base);
        if (mit == struct_methods_.end()) mit = struct_methods_.find(neq_const);
        if (mit == struct_methods_.end()) {
          throw std::runtime_error(
              std::string("error: range-for: iterator type ") + iter_ts.tag +
              " has no operator!= (line " + std::to_string(n->line) + ")");
        }
        CallExpr cc{};
        DeclRef dr{};
        dr.name = mit->second;
        TypeSpec bool_ts{};
        bool_ts.base = TB_BOOL;
        TypeSpec callee_ts = bool_ts;
        callee_ts.ptr_level++;
        cc.callee = append_expr(n, dr, callee_ts);
        UnaryExpr addr{};
        addr.op = UnaryOp::AddrOf;
        addr.operand = ref_local("__range_begin", begin_lid);
        TypeSpec ptr_ts = iter_ts;
        ptr_ts.ptr_level++;
        cc.args.push_back(append_expr(n, addr, ptr_ts));
        cc.args.push_back(ref_local("__range_end", end_lid));
        cond_expr = append_expr(n, cc, bool_ts);
      }

      ExprId update_expr;
      {
        std::string inc_base = std::string(iter_ts.tag) + "::operator_preinc";
        std::string inc_const = inc_base + "_const";
        auto mit = struct_methods_.find(inc_base);
        if (mit == struct_methods_.end()) mit = struct_methods_.find(inc_const);
        if (mit == struct_methods_.end()) {
          throw std::runtime_error(
              std::string("error: range-for: iterator type ") + iter_ts.tag +
              " has no prefix operator++ (line " + std::to_string(n->line) + ")");
        }
        CallExpr cc{};
        DeclRef dr{};
        dr.name = mit->second;
        TypeSpec inc_ret_ts = iter_ts;
        {
          auto fit2 = module_->fn_index.find(mit->second);
          if (fit2 != module_->fn_index.end() &&
              fit2->second.value < module_->functions.size()) {
            inc_ret_ts = module_->functions[fit2->second.value].return_type.spec;
          }
        }
        TypeSpec callee_ts = inc_ret_ts;
        callee_ts.ptr_level++;
        cc.callee = append_expr(n, dr, callee_ts);
        UnaryExpr addr{};
        addr.op = UnaryOp::AddrOf;
        addr.operand = ref_local("__range_begin", begin_lid);
        TypeSpec ptr_ts = iter_ts;
        ptr_ts.ptr_level++;
        cc.args.push_back(append_expr(n, addr, ptr_ts));
        update_expr = append_expr(n, cc, inc_ret_ts);
      }

      ForStmt fs{};
      fs.cond = cond_expr;
      fs.update = update_expr;
      const BlockId body_b = create_block(ctx);
      const BlockId after_b = create_block(ctx);
      fs.body_block = body_b;
      fs.continue_target = body_b;
      fs.break_target = after_b;
      append_stmt(ctx, Stmt{StmtPayload{fs}, make_span(n)});
      ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;

      ctx.break_stack.push_back(after_b);
      ctx.continue_stack.push_back(body_b);
      ctx.current_block = body_b;

      {
        ExprId deref_expr;
        TypeSpec deref_ret_ts{};
        deref_ret_ts.base = TB_INT;
        {
          std::string deref_base = std::string(iter_ts.tag) + "::operator_deref";
          std::string deref_const = deref_base + "_const";
          auto mit = struct_methods_.find(deref_base);
          if (mit == struct_methods_.end()) mit = struct_methods_.find(deref_const);
          if (mit == struct_methods_.end()) {
            throw std::runtime_error(
                std::string("error: range-for: iterator type ") + iter_ts.tag +
                " has no operator* (line " + std::to_string(n->line) + ")");
          }
          CallExpr cc{};
          DeclRef dr{};
          dr.name = mit->second;
          {
            auto fit2 = module_->fn_index.find(mit->second);
            if (fit2 != module_->fn_index.end() &&
                fit2->second.value < module_->functions.size()) {
              deref_ret_ts = module_->functions[fit2->second.value].return_type.spec;
            }
          }
          if (deref_ret_ts.base == TB_VOID || deref_ret_ts.base == TB_INT) {
            auto rit = struct_method_ret_types_.find(
                std::string(iter_ts.tag) + "::operator_deref");
            if (rit == struct_method_ret_types_.end()) {
              rit = struct_method_ret_types_.find(
                  std::string(iter_ts.tag) + "::operator_deref_const");
            }
            if (rit != struct_method_ret_types_.end()) deref_ret_ts = rit->second;
          }
          TypeSpec callee_ts = deref_ret_ts;
          callee_ts.ptr_level++;
          cc.callee = append_expr(n, dr, callee_ts);
          UnaryExpr addr{};
          addr.op = UnaryOp::AddrOf;
          addr.operand = ref_local("__range_begin", begin_lid);
          TypeSpec ptr_ts = iter_ts;
          ptr_ts.ptr_level++;
          cc.args.push_back(append_expr(n, addr, ptr_ts));
          deref_expr = append_expr(n, cc, reference_storage_ts(deref_ret_ts));
        }

        if (decl_node) {
          LocalDecl ld{};
          ld.id = next_local_id();
          ld.name = decl_node->name ? decl_node->name : "__range_var";
          TypeSpec var_ts = decl_node->type;
          bool is_ref = var_ts.is_lvalue_ref;
          if (var_ts.base == TB_AUTO) {
            bool was_const = var_ts.is_const;
            var_ts = deref_ret_ts;
            var_ts.is_lvalue_ref = false;
            if (was_const) var_ts.is_const = true;
            if (is_ref) var_ts.is_lvalue_ref = true;
          }
          resolve_typedef_to_struct(var_ts);
          ld.type = qtype_from(reference_storage_ts(var_ts), ValueCategory::LValue);
          if (is_ref) {
            if (deref_ret_ts.is_lvalue_ref) {
              ld.init = deref_expr;
            } else {
              UnaryExpr addr{};
              addr.op = UnaryOp::AddrOf;
              addr.operand = deref_expr;
              ld.init = append_expr(n, addr, ld.type.spec);
            }
          } else {
            ld.init = deref_expr;
          }
          ctx.locals[ld.name] = ld.id;
          ctx.local_types[ld.id.value] = var_ts;
          append_stmt(ctx, Stmt{StmtPayload{std::move(ld)}, make_span(n)});
        }
      }

      if (n->body) lower_stmt_node(ctx, n->body);

      if (ctx.current_block.value != after_b.value &&
          !ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
        ContinueStmt cont{};
        cont.target = body_b;
        append_stmt(ctx, Stmt{StmtPayload{cont}, make_span(n)});
        ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
      }
      ctx.break_stack.pop_back();
      ctx.continue_stack.pop_back();
      ctx.current_block = after_b;
      return;
    }
    case NK_DO_WHILE: {
      const BlockId body_b = create_block(ctx);
      const BlockId cond_b = create_block(ctx);
      const BlockId after_b = create_block(ctx);

      if (!ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
        GotoStmt j{};
        j.target.resolved_block = body_b;
        append_stmt(ctx, Stmt{StmtPayload{j}, make_span(n)});
        ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
      }

      ctx.break_stack.push_back(after_b);
      ctx.continue_stack.push_back(cond_b);
      ctx.current_block = body_b;
      lower_stmt_node(ctx, n->body);
      if (!ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
        GotoStmt j{};
        j.target.resolved_block = cond_b;
        append_stmt(ctx, Stmt{StmtPayload{j}, make_span(n)});
        ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
      }

      ctx.current_block = cond_b;
      DoWhileStmt s{};
      s.body_block = body_b;
      {
        const Node* cond_n = n->cond ? n->cond : n->left;
        s.cond = maybe_bool_convert(&ctx, lower_expr(&ctx, cond_n), cond_n);
      }
      s.continue_target = cond_b;
      s.break_target = after_b;
      append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
      ensure_block(ctx, cond_b).has_explicit_terminator = true;

      ctx.break_stack.pop_back();
      ctx.continue_stack.pop_back();
      ctx.current_block = after_b;
      return;
    }
    case NK_SWITCH: {
      SwitchStmt s{};
      s.cond = lower_expr(&ctx, n->cond ? n->cond : n->left);
      const BlockId body_b = create_block(ctx);
      s.body_block = body_b;
      const BlockId after_b = create_block(ctx);
      s.break_block = after_b;
      const BlockId parent_b = ctx.current_block;
      append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
      ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;

      ctx.switch_stack.push_back({parent_b, {}, {}, {}});
      ctx.break_stack.push_back(after_b);
      ctx.current_block = body_b;
      lower_stmt_node(ctx, n->body);
      ctx.break_stack.pop_back();

      if (!ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
        GotoStmt j{};
        j.target.resolved_block = after_b;
        append_stmt(ctx, Stmt{StmtPayload{j}, make_span(n)});
        ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
      }

      if (!ctx.switch_stack.empty()) {
        auto& sw_ctx = ctx.switch_stack.back();
        for (auto& blk : ctx.fn->blocks) {
          if (blk.id.value != parent_b.value) continue;
          for (auto& stmt : blk.stmts) {
            if (auto* sw = std::get_if<SwitchStmt>(&stmt.payload)) {
              sw->case_blocks = std::move(sw_ctx.cases);
              sw->case_range_blocks = std::move(sw_ctx.case_ranges);
              if (sw_ctx.default_block) sw->default_block = sw_ctx.default_block;
              break;
            }
          }
          break;
        }
        ctx.switch_stack.pop_back();
      }

      ctx.current_block = after_b;
      return;
    }
    case NK_CASE: {
      long long case_val = 0;
      if (n->left) {
        if (n->left->kind == NK_INT_LIT) {
          case_val = n->left->ival;
        } else {
          ConstEvalEnv env{&enum_consts_, nullptr, &ctx.local_const_bindings};
          if (auto r = evaluate_constant_expr(n->left, env); r.ok()) case_val = r.as_int();
        }
      }
      if (!ctx.switch_stack.empty()) {
        const BlockId case_b = create_block(ctx);
        if (!ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
          GotoStmt j{};
          j.target.resolved_block = case_b;
          append_stmt(ctx, Stmt{StmtPayload{j}, make_span(n)});
          ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
        }
        ctx.switch_stack.back().cases.push_back({case_val, case_b});
        ctx.current_block = case_b;
      }
      CaseStmt s{};
      s.value = case_val;
      append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
      lower_stmt_node(ctx, n->body);
      return;
    }
    case NK_CASE_RANGE: {
      long long lo = 0;
      long long hi = 0;
      {
        ConstEvalEnv env{&enum_consts_, nullptr, &ctx.local_const_bindings};
        if (n->left) {
          if (n->left->kind == NK_INT_LIT) lo = n->left->ival;
          else if (auto r = evaluate_constant_expr(n->left, env); r.ok()) lo = r.as_int();
        }
        if (n->right) {
          if (n->right->kind == NK_INT_LIT) hi = n->right->ival;
          else if (auto r = evaluate_constant_expr(n->right, env); r.ok()) hi = r.as_int();
        }
      }
      if (!ctx.switch_stack.empty()) {
        const BlockId case_b = create_block(ctx);
        if (!ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
          GotoStmt j{};
          j.target.resolved_block = case_b;
          append_stmt(ctx, Stmt{StmtPayload{j}, make_span(n)});
          ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
        }
        ctx.switch_stack.back().case_ranges.push_back({lo, hi, case_b});
        ctx.current_block = case_b;
      }
      CaseRangeStmt s{};
      s.range.lo = lo;
      s.range.hi = hi;
      append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
      lower_stmt_node(ctx, n->body);
      return;
    }
    case NK_DEFAULT: {
      if (!ctx.switch_stack.empty()) {
        const BlockId def_b = create_block(ctx);
        if (!ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
          GotoStmt j{};
          j.target.resolved_block = def_b;
          append_stmt(ctx, Stmt{StmtPayload{j}, make_span(n)});
          ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
        }
        ctx.switch_stack.back().default_block = def_b;
        ctx.current_block = def_b;
      }
      append_stmt(ctx, Stmt{StmtPayload{DefaultStmt{}}, make_span(n)});
      lower_stmt_node(ctx, n->body);
      return;
    }
    case NK_BREAK: {
      BreakStmt s{};
      if (!ctx.break_stack.empty()) s.target = ctx.break_stack.back();
      append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
      ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
      return;
    }
    case NK_CONTINUE: {
      ContinueStmt s{};
      if (!ctx.continue_stack.empty()) s.target = ctx.continue_stack.back();
      append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
      ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
      return;
    }
    case NK_LABEL: {
      LabelStmt s{};
      s.name = n->name ? n->name : "<anon_label>";
      const BlockId lb = create_block(ctx);
      ctx.label_blocks[s.name] = lb;
      append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
      {
        GotoStmt j{};
        j.target.resolved_block = lb;
        append_stmt(ctx, Stmt{StmtPayload{j}, make_span(n)});
      }
      ctx.current_block = lb;
      lower_stmt_node(ctx, n->body);
      return;
    }
    case NK_GOTO: {
      if (n->name && std::string(n->name) == "__computed__" && n->left) {
        IndirBrStmt ib{};
        ib.target = lower_expr(&ctx, n->left);
        append_stmt(ctx, Stmt{StmtPayload{ib}, make_span(n)});
        ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
        return;
      }
      GotoStmt s{};
      s.target.user_name = n->name ? n->name : "<anon_label>";
      auto it = ctx.label_blocks.find(s.target.user_name);
      if (it != ctx.label_blocks.end()) s.target.resolved_block = it->second;
      append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
      ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
      return;
    }
    case NK_EMPTY:
    case NK_STATIC_ASSERT: {
      append_stmt(ctx, Stmt{StmtPayload{ExprStmt{}}, make_span(n)});
      return;
    }
    case NK_ENUM_DEF: {
      collect_enum_def(n);
      return;
    }
    case NK_INVALID_EXPR:
    case NK_INVALID_STMT:
      return;
    default: {
      ExprStmt s{};
      s.expr = lower_expr(&ctx, n);
      append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
      return;
    }
  }
}

ExprId Lowerer::lower_stmt_expr_block(FunctionCtx& ctx,
                                      const Node* block,
                                      const TypeSpec& result_ts) {
  if (!block || block->kind != NK_BLOCK) {
    TypeSpec ts = result_ts;
    if (ts.base == TB_VOID) ts.base = TB_INT;
    return append_expr(nullptr, IntLiteral{0, false}, ts);
  }

  const bool new_scope = (block->ival != 1);
  const auto saved_locals = ctx.locals;
  const auto saved_static_globals = ctx.static_globals;

  ExprId result{};
  bool have_result = false;
  for (int i = 0; i < block->n_children; ++i) {
    const Node* child = block->children[i];
    const bool is_last = (i + 1 == block->n_children);
    if (is_last && child && child->kind == NK_EXPR_STMT && child->left &&
        result_ts.base != TB_VOID) {
      result = lower_expr(&ctx, child->left);
      have_result = true;
      continue;
    }
    lower_stmt_node(ctx, child);
  }

  if (new_scope) {
    ctx.locals = saved_locals;
    ctx.static_globals = saved_static_globals;
  }

  if (have_result) return result;
  TypeSpec ts = result_ts;
  if (ts.base == TB_VOID) ts.base = TB_INT;
  return append_expr(nullptr, IntLiteral{0, false}, ts);
}

void Lowerer::lower_struct_method(const std::string& mangled_name,
                                  const std::string& struct_tag,
                                  const Node* method_node,
                                  const TypeBindings* tpl_bindings,
                                  const NttpBindings* nttp_bindings) {
  if (method_node->is_deleted) return;
  Function fn{};
  fn.id = next_fn_id();
  fn.name = mangled_name;
  fn.ns_qual = make_ns_qual(method_node);
  {
    TypeSpec ret_ts = prepare_callable_return_type(
        method_node->type, tpl_bindings, nttp_bindings, method_node,
        std::string("method-return:") + mangled_name, true);
    fn.return_type = qtype_from(ret_ts);
  }
  fn.linkage = {true, false, false, false, Visibility::Default};
  fn.attrs.variadic = method_node->variadic;
  fn.span = make_span(method_node);

  FunctionCtx ctx{};
  ctx.fn = &fn;
  if (tpl_bindings) ctx.tpl_bindings = *tpl_bindings;
  if (nttp_bindings) ctx.nttp_bindings = *nttp_bindings;

  {
    Param this_param{};
    this_param.name = "this";
    TypeSpec this_ts{};
    this_ts.base = TB_STRUCT;
    auto sit = module_->struct_defs.find(struct_tag);
    this_ts.tag = sit != module_->struct_defs.end() ? sit->second.tag.c_str()
                                                    : struct_tag.c_str();
    this_ts.ptr_level = 1;
    this_param.type = qtype_from(this_ts, ValueCategory::LValue);
    this_param.span = make_span(method_node);
    ctx.params[this_param.name] = static_cast<uint32_t>(fn.params.size());
    fn.params.push_back(std::move(this_param));
  }

  append_callable_params(
      fn, ctx, method_node, tpl_bindings, nttp_bindings, "method-param:", true,
      false);
  ctx.method_struct_tag = struct_tag;

  if (maybe_register_bodyless_callable(
          &fn, method_node->body != nullptr || method_node->is_defaulted)) {
    return;
  }

  const BlockId entry = begin_callable_body_lowering(fn, ctx);

  if (method_node->is_defaulted) {
    emit_defaulted_method_body(ctx, fn, struct_tag, method_node);
    if (method_node->is_destructor) {
      DeclRef this_ref{};
      this_ref.name = "this";
      auto pit = ctx.params.find("this");
      if (pit != ctx.params.end()) this_ref.param_index = pit->second;
      TypeSpec this_ts{};
      this_ts.base = TB_STRUCT;
      auto sit2 = module_->struct_defs.find(struct_tag);
      this_ts.tag = sit2 != module_->struct_defs.end()
                        ? sit2->second.tag.c_str()
                        : struct_tag.c_str();
      this_ts.ptr_level = 1;
      ExprId this_ptr = append_expr(
          method_node, this_ref, this_ts, ValueCategory::LValue);
      emit_member_dtor_calls(ctx, struct_tag, this_ptr, method_node);
    }
    if (method_node->is_destructor) {
      ReturnStmt rs{};
      append_stmt(ctx, Stmt{StmtPayload{rs}, make_span(method_node)});
    }
    finish_lowered_callable(&fn, entry);
    return;
  }

  if (method_node->is_constructor && method_node->n_ctor_inits > 0) {
    auto sit = module_->struct_defs.find(struct_tag);
    for (int i = 0; i < method_node->n_ctor_inits; ++i) {
      const char* mem_name = method_node->ctor_init_names[i];
      int nargs = method_node->ctor_init_nargs[i];
      Node** args = method_node->ctor_init_args[i];

      if (mem_name == struct_tag || (mem_name && struct_tag == mem_name)) {
        auto cit = struct_constructors_.find(struct_tag);
        if (cit == struct_constructors_.end() || cit->second.empty()) {
          throw std::runtime_error(
              "error: no constructors found for delegating constructor call to '" +
              struct_tag + "'");
        }
        const CtorOverload* best = nullptr;
        if (cit->second.size() == 1 &&
            cit->second[0].method_node->n_params == nargs) {
          best = &cit->second[0];
        } else {
          int best_score = -1;
          for (const auto& ov : cit->second) {
            if (ov.method_node->n_params != nargs) continue;
            if (ov.mangled_name == mangled_name) continue;
            int score = 0;
            bool viable = true;
            for (int pi = 0; pi < nargs && viable; ++pi) {
              TypeSpec param_ts = ov.method_node->params[pi]->type;
              resolve_typedef_to_struct(param_ts);
              TypeSpec arg_ts = infer_generic_ctrl_type(&ctx, args[pi]);
              bool arg_is_lvalue = is_ast_lvalue(args[pi], &ctx);
              TypeSpec param_base = param_ts;
              param_base.is_lvalue_ref = false;
              param_base.is_rvalue_ref = false;
              if (param_base.base == arg_ts.base &&
                  param_base.ptr_level == arg_ts.ptr_level) {
                score += 2;
                if (param_ts.is_rvalue_ref && !arg_is_lvalue) score += 4;
                else if (param_ts.is_rvalue_ref && arg_is_lvalue) viable = false;
                else if (param_ts.is_lvalue_ref && arg_is_lvalue) score += 3;
                else if (param_ts.is_lvalue_ref && !arg_is_lvalue) score += 1;
              } else if (param_base.ptr_level == 0 && arg_ts.ptr_level == 0 &&
                         param_base.base != TB_STRUCT &&
                         arg_ts.base != TB_STRUCT) {
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
        if (!best) {
          throw std::runtime_error(
              "error: no matching constructor for delegating constructor call to '" +
              struct_tag + "'");
        }
        if (best->method_node->is_deleted) {
          throw std::runtime_error(
              "error: call to deleted constructor '" + struct_tag + "'");
        }
        DeclRef this_ref{};
        this_ref.name = "this";
        auto pit = ctx.params.find("this");
        if (pit != ctx.params.end()) this_ref.param_index = pit->second;
        TypeSpec this_ts{};
        this_ts.base = TB_STRUCT;
        this_ts.tag = struct_tag.c_str();
        this_ts.ptr_level = 1;
        ExprId this_id = append_expr(
            method_node, this_ref, this_ts, ValueCategory::LValue);

        CallExpr c{};
        DeclRef callee_ref{};
        callee_ref.name = best->mangled_name;
        TypeSpec fn_ts{};
        fn_ts.base = TB_VOID;
        TypeSpec callee_ts = fn_ts;
        callee_ts.ptr_level++;
        c.callee = append_expr(method_node, callee_ref, callee_ts);
        c.args.push_back(this_id);
        for (int ai = 0; ai < nargs; ++ai) {
          TypeSpec param_ts = best->method_node->params[ai]->type;
          resolve_typedef_to_struct(param_ts);
          if (param_ts.is_rvalue_ref || param_ts.is_lvalue_ref) {
            ExprId arg_val = lower_expr(&ctx, args[ai]);
            TypeSpec storage_ts = reference_storage_ts(param_ts);
            UnaryExpr addr_e{};
            addr_e.op = UnaryOp::AddrOf;
            addr_e.operand = arg_val;
            c.args.push_back(append_expr(args[ai], addr_e, storage_ts));
          } else {
            c.args.push_back(lower_expr(&ctx, args[ai]));
          }
        }
        ExprId call_id = append_expr(method_node, c, fn_ts);
        ExprStmt es{};
        es.expr = call_id;
        append_stmt(ctx, Stmt{StmtPayload{es}, make_span(method_node)});
        continue;
      }

      DeclRef this_ref{};
      this_ref.name = "this";
      auto pit = ctx.params.find("this");
      if (pit != ctx.params.end()) this_ref.param_index = pit->second;
      TypeSpec this_ts{};
      this_ts.base = TB_STRUCT;
      this_ts.tag = sit != module_->struct_defs.end() ? sit->second.tag.c_str()
                                                      : struct_tag.c_str();
      this_ts.ptr_level = 1;
      ExprId this_id = append_expr(
          method_node, this_ref, this_ts, ValueCategory::LValue);
      TypeSpec field_ts{};
      if (sit != module_->struct_defs.end()) {
        for (const auto& fld : sit->second.fields) {
          if (fld.name == mem_name) {
            field_ts = fld.elem_type;
            if (fld.array_first_dim >= 0) {
              field_ts.array_rank = 1;
              field_ts.array_size = fld.array_first_dim;
            }
            break;
          }
        }
      }
      MemberExpr me{};
      me.base = this_id;
      me.field = mem_name;
      me.is_arrow = true;
      ExprId lhs_id = append_expr(method_node, me, field_ts, ValueCategory::LValue);

      bool did_ctor_call = false;
      if (field_ts.base == TB_STRUCT && field_ts.tag && field_ts.ptr_level == 0) {
        auto cit = struct_constructors_.find(field_ts.tag);
        if (cit != struct_constructors_.end() && !cit->second.empty()) {
          const CtorOverload* best = nullptr;
          if (cit->second.size() == 1 &&
              cit->second[0].method_node->n_params == nargs) {
            best = &cit->second[0];
          } else {
            int best_score = -1;
            for (const auto& ov : cit->second) {
              if (ov.method_node->n_params != nargs) continue;
              int score = 0;
              bool viable = true;
              for (int pi = 0; pi < nargs && viable; ++pi) {
                TypeSpec param_ts = ov.method_node->params[pi]->type;
                resolve_typedef_to_struct(param_ts);
                TypeSpec arg_ts = infer_generic_ctrl_type(&ctx, args[pi]);
                bool arg_is_lvalue = is_ast_lvalue(args[pi], &ctx);
                TypeSpec param_base = param_ts;
                param_base.is_lvalue_ref = false;
                param_base.is_rvalue_ref = false;
                if (param_base.base == arg_ts.base &&
                    param_base.ptr_level == arg_ts.ptr_level) {
                  score += 2;
                  if (param_ts.is_rvalue_ref && !arg_is_lvalue) score += 4;
                  else if (param_ts.is_rvalue_ref && arg_is_lvalue) viable = false;
                  else if (param_ts.is_lvalue_ref && arg_is_lvalue) score += 3;
                  else if (param_ts.is_lvalue_ref && !arg_is_lvalue) score += 1;
                } else if (param_base.ptr_level == 0 && arg_ts.ptr_level == 0 &&
                           param_base.base != TB_STRUCT &&
                           arg_ts.base != TB_STRUCT) {
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
          if (best) {
            if (best->method_node->is_deleted) {
              std::string diag = "error: call to deleted constructor '";
              diag += field_ts.tag;
              diag += "'";
              throw std::runtime_error(diag);
            }
            CallExpr c{};
            DeclRef callee_ref{};
            callee_ref.name = best->mangled_name;
            TypeSpec fn_ts{};
            fn_ts.base = TB_VOID;
            TypeSpec callee_ts = fn_ts;
            callee_ts.ptr_level++;
            c.callee = append_expr(method_node, callee_ref, callee_ts);
            UnaryExpr addr{};
            addr.op = UnaryOp::AddrOf;
            addr.operand = lhs_id;
            TypeSpec ptr_ts = field_ts;
            ptr_ts.ptr_level++;
            c.args.push_back(append_expr(method_node, addr, ptr_ts));
            for (int ai = 0; ai < nargs; ++ai) {
              TypeSpec param_ts = best->method_node->params[ai]->type;
              resolve_typedef_to_struct(param_ts);
              if (param_ts.is_rvalue_ref || param_ts.is_lvalue_ref) {
                ExprId arg_val = lower_expr(&ctx, args[ai]);
                TypeSpec storage_ts = reference_storage_ts(param_ts);
                UnaryExpr addr_e{};
                addr_e.op = UnaryOp::AddrOf;
                addr_e.operand = arg_val;
                c.args.push_back(append_expr(args[ai], addr_e, storage_ts));
              } else {
                c.args.push_back(lower_expr(&ctx, args[ai]));
              }
            }
            ExprId call_id = append_expr(method_node, c, fn_ts);
            ExprStmt es{};
            es.expr = call_id;
            append_stmt(ctx, Stmt{StmtPayload{es}, make_span(method_node)});
            did_ctor_call = true;
          }
        }
      }

      if (!did_ctor_call) {
        if (nargs != 1) {
          std::string diag = "error: initializer for scalar member '";
          diag += mem_name;
          diag += "' must have exactly one argument";
          throw std::runtime_error(diag);
        }
        ExprId rhs_id = lower_expr(&ctx, args[0]);
        AssignExpr ae{};
        ae.op = AssignOp::Set;
        ae.lhs = lhs_id;
        ae.rhs = rhs_id;
        ExprId ae_id = append_expr(method_node, ae, field_ts);
        ExprStmt es{};
        es.expr = ae_id;
        append_stmt(ctx, Stmt{StmtPayload{es}, make_span(method_node)});
      }
    }
  }

  lower_stmt_node(ctx, method_node->body);

  if (method_node->is_destructor) {
    DeclRef this_ref{};
    this_ref.name = "this";
    auto pit = ctx.params.find("this");
    if (pit != ctx.params.end()) this_ref.param_index = pit->second;
    TypeSpec this_ts{};
    this_ts.base = TB_STRUCT;
    auto sit2 = module_->struct_defs.find(struct_tag);
    this_ts.tag = sit2 != module_->struct_defs.end()
                      ? sit2->second.tag.c_str()
                      : struct_tag.c_str();
    this_ts.ptr_level = 1;
    ExprId this_ptr = append_expr(
        method_node, this_ref, this_ts, ValueCategory::LValue);
    emit_member_dtor_calls(ctx, struct_tag, this_ptr, method_node);
  }

  finish_lowered_callable(&fn, entry);
}

}  // namespace c4c::hir
