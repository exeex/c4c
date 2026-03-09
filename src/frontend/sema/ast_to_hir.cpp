#include "ast_to_hir.hpp"

#include <cstdio>
#include <stdexcept>
#include <unordered_map>

namespace tinyc2ll::frontend_cxx::sema_ir::phase2::hir {
namespace {

using namespace tinyc2ll::frontend_cxx::sema_ir;

SourceSpan make_span(const Node* n) {
  if (!n) {
    return {};
  }
  SourceSpan s{};
  s.begin.line = n->line;
  s.end.line = n->line;
  return s;
}

UnaryOp map_unary_op(const char* op) {
  if (!op) return UnaryOp::Plus;
  const std::string s(op);
  if (s.rfind("++", 0) == 0) return UnaryOp::PreInc;
  if (s.rfind("--", 0) == 0) return UnaryOp::PreDec;
  if (s == "+") return UnaryOp::Plus;
  if (s == "-") return UnaryOp::Minus;
  if (s == "!") return UnaryOp::Not;
  if (s == "~") return UnaryOp::BitNot;
  return UnaryOp::Plus;
}

BinaryOp map_binary_op(const char* op) {
  if (!op) return BinaryOp::Add;
  const std::string s(op);
  if (s == "+") return BinaryOp::Add;
  if (s == "-") return BinaryOp::Sub;
  if (s == "*") return BinaryOp::Mul;
  if (s == "/") return BinaryOp::Div;
  if (s == "%") return BinaryOp::Mod;
  if (s == "<<") return BinaryOp::Shl;
  if (s == ">>") return BinaryOp::Shr;
  if (s == "&") return BinaryOp::BitAnd;
  if (s == "|") return BinaryOp::BitOr;
  if (s == "^") return BinaryOp::BitXor;
  if (s == "<") return BinaryOp::Lt;
  if (s == "<=") return BinaryOp::Le;
  if (s == ">") return BinaryOp::Gt;
  if (s == ">=") return BinaryOp::Ge;
  if (s == "==") return BinaryOp::Eq;
  if (s == "!=") return BinaryOp::Ne;
  if (s == "&&") return BinaryOp::LAnd;
  if (s == "||") return BinaryOp::LOr;
  if (s == ",") return BinaryOp::Comma;
  return BinaryOp::Add;
}

AssignOp map_assign_op(const char* op, NodeKind kind) {
  if (!op) return AssignOp::Set;
  const std::string s(op);
  if (s == "=") return AssignOp::Set;
  if (s == "+=" || s == "+") return AssignOp::Add;
  if (s == "-=" || s == "-") return AssignOp::Sub;
  if (s == "*=" || s == "*") return AssignOp::Mul;
  if (s == "/=" || s == "/") return AssignOp::Div;
  if (s == "%=" || s == "%") return AssignOp::Mod;
  if (s == "<<=" || s == "<<") return AssignOp::Shl;
  if (s == ">>=" || s == ">>") return AssignOp::Shr;
  if (s == "&=" || s == "&") return AssignOp::BitAnd;
  if (s == "|=" || s == "|") return AssignOp::BitOr;
  if (s == "^=" || s == "^") return AssignOp::BitXor;
  if (kind == NK_ASSIGN) return AssignOp::Set;
  return AssignOp::Set;
}

class Lowerer {
 public:
  Module lower_program(const Node* root) {
    if (!root || root->kind != NK_PROGRAM) {
      throw std::runtime_error("ast_to_hir: root is not NK_PROGRAM");
    }

    Module m{};
    module_ = &m;

    // Flatten top-level items (may be wrapped in NK_BLOCK)
    std::vector<const Node*> items;
    for (int i = 0; i < root->n_children; ++i) {
      const Node* item = root->children[i];
      if (!item) continue;
      if (item->kind == NK_BLOCK) {
        for (int j = 0; j < item->n_children; ++j)
          if (item->children[j]) items.push_back(item->children[j]);
      } else {
        items.push_back(item);
      }
    }

    // Phase 1: collect type definitions used by later lowering
    for (const Node* item : items) {
      if (item->kind == NK_STRUCT_DEF) lower_struct_def(item);
      if (item->kind == NK_ENUM_DEF) collect_enum_def(item);
    }

    // Phase 2: lower functions and globals
    for (const Node* item : items) {
      if (item->kind == NK_FUNCTION) {
        lower_function(item);
      } else if (item->kind == NK_GLOBAL_VAR) {
        lower_global(item);
      }
    }

    return m;
  }

 private:
  struct SwitchCtx {
    BlockId parent_block{};
    std::vector<std::pair<long long, BlockId>> cases;
    std::optional<BlockId> default_block;
  };

  struct FunctionCtx {
    Function* fn = nullptr;
    std::unordered_map<std::string, LocalId> locals;
    std::unordered_map<std::string, uint32_t> params;
    BlockId current_block{};
    std::vector<BlockId> break_stack;
    std::vector<BlockId> continue_stack;
    std::unordered_map<std::string, BlockId> label_blocks;
    std::vector<SwitchCtx> switch_stack;
  };

  FunctionId next_fn_id() { return module_->alloc_function_id(); }
  GlobalId next_global_id() { return module_->alloc_global_id(); }
  LocalId next_local_id() { return module_->alloc_local_id(); }
  BlockId next_block_id() { return module_->alloc_block_id(); }
  ExprId next_expr_id() { return module_->alloc_expr_id(); }

  QualType qtype_from(const TypeSpec& t, ValueCategory c = ValueCategory::RValue) {
    QualType qt{};
    qt.spec = t;
    qt.category = c;
    return qt;
  }

  Block& ensure_block(FunctionCtx& ctx, BlockId id) {
    for (auto& b : ctx.fn->blocks) {
      if (b.id.value == id.value) return b;
    }
    ctx.fn->blocks.push_back(Block{id, {}, false});
    return ctx.fn->blocks.back();
  }

  BlockId create_block(FunctionCtx& ctx) {
    const BlockId id = next_block_id();
    ctx.fn->blocks.push_back(Block{id, {}, false});
    return id;
  }

  void append_stmt(FunctionCtx& ctx, Stmt stmt) {
    Block& b = ensure_block(ctx, ctx.current_block);
    b.stmts.push_back(std::move(stmt));
  }

  ExprId append_expr(const Node* src, ExprPayload payload, const TypeSpec& ts,
                     ValueCategory c = ValueCategory::RValue) {
    Expr e{};
    e.id = next_expr_id();
    e.span = make_span(src);
    e.type = qtype_from(ts, c);
    e.payload = std::move(payload);
    module_->expr_pool.push_back(std::move(e));
    return module_->expr_pool.back().id;
  }

  void lower_struct_def(const Node* sd) {
    if (!sd || sd->kind != NK_STRUCT_DEF) return;
    const char* tag = sd->name;
    if (!tag || !tag[0]) return;

    // Gather fields: they may be in sd->fields[] (n_fields) OR sd->children[] (n_children)
    // The IR builder uses sd->fields; but some parsers store struct fields in children.
    int num_fields = sd->n_fields > 0 ? sd->n_fields : sd->n_children;
    auto get_field = [&](int i) -> const Node* {
      return sd->n_fields > 0 ? sd->fields[i] : sd->children[i];
    };

    // If already fully populated, skip (forward decl followed by full def is OK)
    if (module_->struct_defs.count(tag) &&
        !module_->struct_defs.at(tag).fields.empty() &&
        num_fields == 0) return;

    HirStructDef def;
    def.tag = tag;
    def.is_union = sd->is_union;

    int llvm_idx = 0;
    for (int i = 0; i < num_fields; ++i) {
      const Node* f = get_field(i);
      if (!f || !f->name) continue;

      HirStructField hf;
      hf.name = f->name;
      TypeSpec ft = f->type;
      // Extract first array dimension (keep base element type for LLVM)
      if (ft.array_rank > 0 && ft.array_size >= 0) {
        hf.array_first_dim = ft.array_size;
        ft.array_rank = 0;
        ft.array_size = -1;
      }
      hf.elem_type = ft;
      hf.is_anon_member = f->is_anon_field;
      hf.llvm_idx = sd->is_union ? 0 : llvm_idx;
      def.fields.push_back(std::move(hf));
      if (!sd->is_union) ++llvm_idx;
    }

    if (!module_->struct_defs.count(tag))
      module_->struct_def_order.push_back(tag);
    module_->struct_defs[tag] = std::move(def);
  }

  void collect_enum_def(const Node* ed) {
    if (!ed || ed->kind != NK_ENUM_DEF || ed->n_enum_variants <= 0) return;
    if (!ed->enum_names || !ed->enum_vals) return;
    for (int i = 0; i < ed->n_enum_variants; ++i) {
      const char* name = ed->enum_names[i];
      if (!name || !name[0]) continue;
      enum_consts_[name] = ed->enum_vals[i];
    }
  }

  void lower_function(const Node* fn_node) {
    Function fn{};
    fn.id = next_fn_id();
    fn.name = fn_node->name ? fn_node->name : "<anon_fn>";
    fn.return_type = qtype_from(fn_node->type);
    fn.linkage = {fn_node->is_static, fn_node->is_extern || fn_node->body == nullptr, fn_node->is_inline};
    fn.attrs.variadic = fn_node->variadic;
    fn.span = make_span(fn_node);

    FunctionCtx ctx{};
    ctx.fn = &fn;

    for (int i = 0; i < fn_node->n_params; ++i) {
      const Node* p = fn_node->params[i];
      if (!p) continue;
      Param param{};
      param.name = p->name ? p->name : "<anon_param>";
      param.type = qtype_from(p->type, ValueCategory::LValue);
      param.span = make_span(p);
      ctx.params[param.name] = static_cast<uint32_t>(fn.params.size());
      fn.params.push_back(std::move(param));
    }

    if (!fn_node->body) {
      module_->fn_index[fn.name] = fn.id;
      module_->functions.push_back(std::move(fn));
      return;
    }

    const BlockId entry = create_block(ctx);
    fn.entry = entry;
    ctx.current_block = entry;

    lower_stmt_node(ctx, fn_node->body);

    if (fn.blocks.empty()) {
      fn.blocks.push_back(Block{entry, {}, false});
    }

    module_->fn_index[fn.name] = fn.id;
    module_->functions.push_back(std::move(fn));
  }

  void lower_global(const Node* gv) {
    GlobalInit computed_init{};
    bool has_init = false;

    // Handle compound literal at global scope: `T *p = &(T){...};`
    // The compound literal must be lowered to a separate static global, and
    // the parent global initialized to point at it.
    // IMPORTANT: the synthetic global must be pushed BEFORE allocating g.id,
    // since hir_to_llvm.cpp uses GlobalId.value as a direct vector index.
    if (gv->init && gv->init->kind == NK_ADDR &&
        gv->init->left && gv->init->left->kind == NK_COMPOUND_LIT) {
      const Node* clit = gv->init->left;
      GlobalVar cg{};
      cg.id = next_global_id();
      const std::string clit_name = "__clit_" + std::to_string(cg.id.value);
      cg.name = clit_name;
      cg.type = qtype_from(clit->type, ValueCategory::LValue);
      cg.linkage = {true, false, false};  // internal linkage
      cg.is_const = false;
      cg.span = make_span(clit);
      // Lower the compound literal's init list (stored in clit->left).
      if (clit->left && clit->left->kind == NK_INIT_LIST) {
        cg.init = lower_init_list(clit->left);
      }
      module_->global_index[clit_name] = cg.id;
      module_->globals.push_back(std::move(cg));

      // Build a scalar init: AddrOf(DeclRef{clit_name})
      TypeSpec ptr_ts = clit->type;
      ptr_ts.ptr_level++;
      DeclRef dr{};
      dr.name = clit_name;
      dr.global = module_->global_index[clit_name];
      ExprId dr_id = append_expr(clit, dr, clit->type, ValueCategory::LValue);
      UnaryExpr addr{};
      addr.op = UnaryOp::AddrOf;
      addr.operand = dr_id;
      ExprId addr_id = append_expr(gv->init, addr, ptr_ts);
      InitScalar sc{};
      sc.expr = addr_id;
      computed_init = sc;
      has_init = true;
    }

    GlobalVar g{};
    g.id = next_global_id();  // allocated AFTER any synthetic globals
    g.name = gv->name ? gv->name : "<anon_global>";
    g.type = qtype_from(gv->type, ValueCategory::LValue);
    g.linkage = {gv->is_static, gv->is_extern, false};
    g.is_const = gv->type.is_const;
    g.span = make_span(gv);

    if (has_init) {
      g.init = computed_init;
    } else if (gv->init) {
      g.init = lower_global_init(gv->init);
    }

    module_->global_index[g.name] = g.id;
    module_->globals.push_back(std::move(g));
  }

  GlobalInit lower_global_init(const Node* n) {
    if (!n) return std::monostate{};
    if (n->kind == NK_INIT_LIST) {
      return lower_init_list(n);
    }
    InitScalar s{};
    s.expr = lower_expr(nullptr, n);
    return s;
  }

  InitList lower_init_list(const Node* n) {
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
        item.value = std::make_shared<InitList>(lower_init_list(value_node));
      } else {
        InitScalar s{};
        s.expr = lower_expr(nullptr, value_node);
        item.value = s;
      }
      out.items.push_back(std::move(item));
    }
    return out;
  }

  void lower_stmt_node(FunctionCtx& ctx, const Node* n) {
    if (!n) return;

    switch (n->kind) {
      case NK_BLOCK: {
        // ival==1 marks synthetic multi-declarator blocks (e.g. int a, b;).
        // They share the current scope and must not discard bindings.
        const bool new_scope = (n->ival != 1);
        const auto saved_locals = ctx.locals;
        for (int i = 0; i < n->n_children; ++i) {
          lower_stmt_node(ctx, n->children[i]);
        }
        if (new_scope) ctx.locals = saved_locals;
        return;
      }
      case NK_DECL: {
        // Local function prototype (e.g. `int f1(char *);` inside a function body):
        // if the name is already registered as a known function, skip creating a
        // local alloca — later references will resolve directly to the global function.
        if (n->name && !n->init && module_->fn_index.count(n->name)) return;

        LocalDecl d{};
        d.id = next_local_id();
        d.name = n->name ? n->name : "<anon_local>";
        d.type = qtype_from(n->type, ValueCategory::LValue);
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
          }
        }
        d.storage = n->is_static ? StorageClass::Static : StorageClass::Auto;
        d.span = make_span(n);
        const bool is_array_with_init_list =
            n->init && n->init->kind == NK_INIT_LIST && d.type.spec.array_rank == 1;
        if (!is_array_with_init_list && n->init) d.init = lower_expr(&ctx, n->init);
        const LocalId lid = d.id;
        const TypeSpec decl_ts = d.type.spec;
        ctx.locals[d.name] = d.id;
        append_stmt(ctx, Stmt{StmtPayload{std::move(d)}, make_span(n)});
        // For array init lists, emit element-by-element assignments.
        if (is_array_with_init_list) {
          long long next_idx = 0;
          for (int ci = 0; ci < n->init->n_children; ++ci) {
            const Node* item = n->init->children[ci];
            if (!item) { ++next_idx; continue; }
            long long idx = next_idx;
            const Node* val_node = item;
            if (item->kind == NK_INIT_ITEM) {
              if (item->is_designated && item->is_index_desig) idx = item->desig_val;
              val_node = item->left ? item->left : item->right;
              if (!val_node) val_node = item->init;
            }
            next_idx = idx + 1;
            if (!val_node) continue;
            // Build: x[idx] = val
            TypeSpec elem_ts = decl_ts;
            elem_ts.array_rank--;
            elem_ts.array_size = -1;
            // DeclRef to the local
            DeclRef dr{}; dr.name = n->name ? n->name : "<anon_local>"; dr.local = lid;
            ExprId dr_id = append_expr(n, dr, decl_ts, ValueCategory::LValue);
            // IntLiteral for index
            TypeSpec idx_ts{}; idx_ts.base = TB_INT;
            ExprId idx_id = append_expr(n, IntLiteral{idx, false}, idx_ts);
            // IndexExpr
            IndexExpr ie{}; ie.base = dr_id; ie.index = idx_id;
            ExprId ie_id = append_expr(n, ie, elem_ts, ValueCategory::LValue);
            // Value expr
            ExprId val_id = lower_expr(&ctx, val_node);
            // AssignExpr
            AssignExpr ae{}; ae.lhs = ie_id; ae.rhs = val_id;
            ExprId ae_id = append_expr(n, ae, elem_ts);
            ExprStmt es{}; es.expr = ae_id;
            append_stmt(ctx, Stmt{StmtPayload{es}, make_span(n)});
          }
        }
        return;
      }
      case NK_EXPR_STMT: {
        ExprStmt s{};
        if (n->left) s.expr = lower_expr(&ctx, n->left);
        append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
        return;
      }
      case NK_RETURN: {
        ReturnStmt s{};
        if (n->left) s.expr = lower_expr(&ctx, n->left);
        append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
        ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
        return;
      }
      case NK_IF: {
        IfStmt s{};
        s.cond = lower_expr(&ctx, n->cond ? n->cond : n->left);
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
        s.cond = lower_expr(&ctx, n->cond ? n->cond : n->left);
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
        if (n->init) s.init = lower_expr(&ctx, n->init);
        if (n->cond) s.cond = lower_expr(&ctx, n->cond);
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
      case NK_DO_WHILE: {
        const BlockId body_b = create_block(ctx);
        const BlockId after_b = create_block(ctx);

        if (!ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
          GotoStmt j{};
          j.target.resolved_block = body_b;
          append_stmt(ctx, Stmt{StmtPayload{j}, make_span(n)});
          ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
        }

        ctx.break_stack.push_back(after_b);
        ctx.continue_stack.push_back(body_b);
        ctx.current_block = body_b;
        lower_stmt_node(ctx, n->body);

        DoWhileStmt s{};
        s.body_block = body_b;
        s.cond = lower_expr(&ctx, n->cond ? n->cond : n->left);
        s.continue_target = body_b;
        s.break_target = after_b;
        append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
        ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;

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

        ctx.switch_stack.push_back({parent_b, {}, {}});
        ctx.break_stack.push_back(after_b);
        ctx.current_block = body_b;
        lower_stmt_node(ctx, n->body);
        ctx.break_stack.pop_back();

        // If the current block (last one in the switch body) has no explicit
        // terminator, branch to after_b (covers Duff's device after-do-while case).
        if (!ensure_block(ctx, ctx.current_block).has_explicit_terminator) {
          GotoStmt j{};
          j.target.resolved_block = after_b;
          append_stmt(ctx, Stmt{StmtPayload{j}, make_span(n)});
          ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
        }

        // Update the SwitchStmt with collected case blocks
        if (!ctx.switch_stack.empty()) {
          auto& sw_ctx = ctx.switch_stack.back();
          for (auto& blk : ctx.fn->blocks) {
            if (blk.id.value != parent_b.value) continue;
            for (auto& stmt : blk.stmts) {
              if (auto* sw = std::get_if<SwitchStmt>(&stmt.payload)) {
                sw->case_blocks = std::move(sw_ctx.cases);
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
        const long long case_val = (n->left && n->left->kind == NK_INT_LIT) ? n->left->ival : 0;
        if (!ctx.switch_stack.empty()) {
          // Create a dedicated block for this case entry point.
          const BlockId case_b = create_block(ctx);
          // Fall-through from previous block (if not already terminated).
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
        CaseRangeStmt s{};
        if (n->left && n->left->kind == NK_INT_LIT) s.range.lo = n->left->ival;
        if (n->right && n->right->kind == NK_INT_LIT) s.range.hi = n->right->ival;
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
        // Append LabelStmt to current block (hir_to_llvm will emit "ulbl_name:" here).
        append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
        // Always branch from the LabelStmt's sub-block to lb (the body block).
        // LabelStmt resets hir_to_llvm's last_term state (starts a new LLVM basic block),
        // so we always need a terminator for the "ulbl_name:" sub-block regardless of
        // the HIR block's existing has_explicit_terminator flag.
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
        GotoStmt s{};
        s.target.user_name = n->name ? n->name : "<anon_label>";
        auto it = ctx.label_blocks.find(s.target.user_name);
        if (it != ctx.label_blocks.end()) {
          // Backward goto: label already defined, use its block directly.
          s.target.resolved_block = it->second;
        }
        // Forward goto: leave resolved_block invalid; hir_to_llvm emits "br label %ulbl_name"
        // which connects to the "ulbl_name:" emitted by the LabelStmt later.
        append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
        ensure_block(ctx, ctx.current_block).has_explicit_terminator = true;
        return;
      }
      case NK_EMPTY: {
        append_stmt(ctx, Stmt{StmtPayload{ExprStmt{}}, make_span(n)});
        return;
      }
      case NK_ENUM_DEF: {
        collect_enum_def(n);
        return;
      }
      default: {
        ExprStmt s{};
        s.expr = lower_expr(&ctx, n);
        append_stmt(ctx, Stmt{StmtPayload{s}, make_span(n)});
        return;
      }
    }
  }

  ExprId lower_expr(FunctionCtx* ctx, const Node* n) {
    if (!n) {
      TypeSpec ts{};
      ts.base = TB_INT;
      return append_expr(nullptr, IntLiteral{0, false}, ts);
    }

    switch (n->kind) {
      case NK_INT_LIT:
        return append_expr(n, IntLiteral{n->ival, false}, n->type);
      case NK_FLOAT_LIT:
        return append_expr(n, FloatLiteral{n->fval}, n->type);
      case NK_STR_LIT: {
        StringLiteral s{};
        s.raw = n->sval ? n->sval : "";
        s.is_wide = s.raw.size() >= 2 && s.raw[0] == 'L' && s.raw[1] == '"';
        TypeSpec ts = n->type;
        return append_expr(n, std::move(s), ts, ValueCategory::LValue);
      }
      case NK_CHAR_LIT:
      {
        // In C, character constants have type int.
        TypeSpec ts = n->type;
        ts.base = TB_INT;
        return append_expr(n, CharLiteral{n->ival}, ts);
      }
      case NK_VAR: {
        if (n->name && n->name[0]) {
          auto it = enum_consts_.find(n->name);
          if (it != enum_consts_.end()) {
            TypeSpec ts{};
            ts.base = TB_INT;
            return append_expr(n, IntLiteral{it->second, false}, ts);
          }
        }
        DeclRef r{};
        r.name = n->name ? n->name : "<anon_var>";
        if (ctx) {
          auto lit = ctx->locals.find(r.name);
          if (lit != ctx->locals.end()) r.local = lit->second;
          auto pit = ctx->params.find(r.name);
          if (pit != ctx->params.end()) r.param_index = pit->second;
        }
        auto git = module_->global_index.find(r.name);
        if (git != module_->global_index.end()) r.global = git->second;
        return append_expr(n, std::move(r), n->type, ValueCategory::LValue);
      }
      case NK_UNARY: {
        UnaryExpr u{};
        u.op = map_unary_op(n->op);
        u.operand = lower_expr(ctx, n->left);
        return append_expr(n, u, n->type);
      }
      case NK_POSTFIX: {
        UnaryExpr u{};
        const std::string op = n->op ? n->op : "";
        u.op = (op == "--") ? UnaryOp::PostDec : UnaryOp::PostInc;
        u.operand = lower_expr(ctx, n->left);
        return append_expr(n, u, n->type);
      }
      case NK_ADDR: {
        UnaryExpr u{};
        u.op = UnaryOp::AddrOf;
        u.operand = lower_expr(ctx, n->left);
        return append_expr(n, u, n->type);
      }
      case NK_DEREF: {
        UnaryExpr u{};
        u.op = UnaryOp::Deref;
        u.operand = lower_expr(ctx, n->left);
        return append_expr(n, u, n->type, ValueCategory::LValue);
      }
      case NK_BINOP:
      case NK_COMMA_EXPR: {
        BinaryExpr b{};
        b.op = map_binary_op(n->op);
        b.lhs = lower_expr(ctx, n->left);
        b.rhs = lower_expr(ctx, n->right);
        return append_expr(n, b, n->type);
      }
      case NK_ASSIGN:
      case NK_COMPOUND_ASSIGN: {
        AssignExpr a{};
        a.op = map_assign_op(n->op, n->kind);
        a.lhs = lower_expr(ctx, n->left);
        a.rhs = lower_expr(ctx, n->right);
        return append_expr(n, a, n->type);
      }
      case NK_CAST: {
        CastExpr c{};
        c.to_type = qtype_from(n->type);
        c.expr = lower_expr(ctx, n->left);
        return append_expr(n, c, n->type);
      }
      case NK_CALL: {
        CallExpr c{};
        c.callee = lower_expr(ctx, n->left);
        for (int i = 0; i < n->n_children; ++i) c.args.push_back(lower_expr(ctx, n->children[i]));
        return append_expr(n, c, n->type);
      }
      case NK_VA_ARG: {
        VaArgExpr v{};
        v.ap = lower_expr(ctx, n->left);
        return append_expr(n, v, n->type);
      }
      case NK_INDEX: {
        IndexExpr idx{};
        idx.base = lower_expr(ctx, n->left);
        idx.index = lower_expr(ctx, n->right);
        return append_expr(n, idx, n->type, ValueCategory::LValue);
      }
      case NK_MEMBER: {
        MemberExpr m{};
        m.base = lower_expr(ctx, n->left);
        m.field = n->name ? n->name : "<anon_field>";
        m.is_arrow = n->is_arrow;
        return append_expr(n, m, n->type, ValueCategory::LValue);
      }
      case NK_TERNARY: {
        TernaryExpr t{};
        t.cond = lower_expr(ctx, n->cond ? n->cond : n->left);
        t.then_expr = lower_expr(ctx, n->then_);
        t.else_expr = lower_expr(ctx, n->else_);
        return append_expr(n, t, n->type);
      }
      case NK_SIZEOF_EXPR: {
        SizeofExpr s{};
        s.expr = lower_expr(ctx, n->left);
        // sizeof always returns an integer (size_t ~ unsigned long)
        TypeSpec ts{}; ts.base = TB_ULONG;
        return append_expr(n, s, ts);
      }
      case NK_SIZEOF_TYPE: {
        SizeofTypeExpr s{};
        s.type = qtype_from(n->type);
        // sizeof always returns an integer (size_t ~ unsigned long)
        TypeSpec ts{}; ts.base = TB_ULONG;
        return append_expr(n, s, ts);
      }
      default: {
        TypeSpec ts = n->type;
        if (ts.base == TB_VOID && n->kind != NK_CALL) {
          ts.base = TB_INT;
        }
        return append_expr(n, IntLiteral{0, false}, ts);
      }
    }
  }

  Module* module_ = nullptr;
  std::unordered_map<std::string, long long> enum_consts_;

};

}  // namespace

Module lower_ast_to_hir(const Node* program_root) {
  Lowerer l;
  return l.lower_program(program_root);
}

std::string format_summary(const Module& module) {
  const ProgramSummary s = module.summary();
  char buf[256];
  std::snprintf(
      buf,
      sizeof(buf),
      "HIR summary: functions=%zu globals=%zu blocks=%zu statements=%zu expressions=%zu",
      s.functions,
      s.globals,
      s.blocks,
      s.statements,
      s.expressions);
  return std::string(buf);
}

}  // namespace tinyc2ll::frontend_cxx::sema_ir::phase2::hir
