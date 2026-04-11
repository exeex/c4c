#include "inline_expand.hpp"

#include <cassert>
#include <cstdio>

namespace c4c::hir {

// ── Phase 7: Diagnostics ────────────────────────────────────────────────────

const char* inline_reject_reason_str(InlineRejectReason reason) {
  switch (reason) {
    case InlineRejectReason::None:            return "eligible";
    case InlineRejectReason::IndirectCall:     return "indirect call (not a direct function reference)";
    case InlineRejectReason::CalleeMissing:    return "callee not found in module";
    case InlineRejectReason::NoBody:           return "callee has no body (extern declaration)";
    case InlineRejectReason::Variadic:         return "callee is variadic";
    case InlineRejectReason::SelfRecursive:    return "self-recursive call";
    case InlineRejectReason::CallerIsNoinline: return "caller is marked noinline";
    case InlineRejectReason::CalleeIsNoinline: return "callee is marked noinline";
    case InlineRejectReason::NotAlwaysInline:  return "callee is not marked always_inline";
    case InlineRejectReason::ArrayParam:       return "callee has array-typed parameter";
    case InlineRejectReason::VaArgInBody:      return "callee body contains va_arg";
  }
  return "unknown reason";
}

// ── Phase 2: InlineCloneContext ──────────────────────────────────────────────

LocalId InlineCloneContext::remap_local(LocalId old_id) {
  return local_map.get_or_create(old_id, [&] { return module->alloc_local_id(); });
}

BlockId InlineCloneContext::remap_block(BlockId old_id) {
  return block_map.get_or_create(old_id, [&] { return module->alloc_block_id(); });
}

ExprId InlineCloneContext::remap_expr(ExprId old_id) {
  return expr_map.get_or_create(old_id, [&] { return module->alloc_expr_id(); });
}

LocalId InlineCloneContext::lookup_local(LocalId old_id) const {
  if (const auto* mapped = local_map.find(old_id)) return *mapped;
  return LocalId::invalid();
}

BlockId InlineCloneContext::lookup_block(BlockId old_id) const {
  if (const auto* mapped = block_map.find(old_id)) return *mapped;
  return BlockId::invalid();
}

ExprId InlineCloneContext::lookup_expr(ExprId old_id) const {
  if (const auto* mapped = expr_map.find(old_id)) return *mapped;
  return ExprId::invalid();
}

// ── clone_expr ───────────────────────────────────────────────────────────────

/// Helper: remap an ExprId that is a sub-expression reference.
/// Deep-clones the referenced expression if not yet cloned.
static ExprId remap_sub_expr(InlineCloneContext& ctx, ExprId old_id) {
  // Check if already cloned.
  ExprId existing = ctx.lookup_expr(old_id);
  if (existing.valid()) return existing;

  // Find and deep-clone the source expression.
  const Expr* src = ctx.module->find_expr(old_id);
  if (!src) return old_id;  // external expr, keep as-is
  return clone_expr(ctx, *src);
}

/// Helper: remap an optional ExprId.
static std::optional<ExprId> remap_opt_expr(InlineCloneContext& ctx,
                                            const std::optional<ExprId>& opt) {
  if (!opt) return std::nullopt;
  return remap_sub_expr(ctx, *opt);
}

/// Helper: remap a BlockId (must already be preallocated).
static BlockId remap_block_ref(InlineCloneContext& ctx, BlockId old_id) {
  if (!old_id.valid()) return old_id;
  BlockId mapped = ctx.lookup_block(old_id);
  assert(mapped.valid() && "block must be preallocated before cloning");
  return mapped;
}

/// Helper: remap an optional BlockId.
static std::optional<BlockId> remap_opt_block(InlineCloneContext& ctx,
                                              const std::optional<BlockId>& opt) {
  if (!opt) return std::nullopt;
  return remap_block_ref(ctx, *opt);
}

ExprId clone_expr(InlineCloneContext& ctx, const Expr& src) {
  // Copy src by value to avoid iterator invalidation: recursive cloning
  // pushes to expr_pool, which may reallocate and invalidate references
  // into the pool.
  Expr src_copy = src;

  ExprId new_id = ctx.remap_expr(src_copy.id);

  Expr cloned;
  cloned.id = new_id;
  cloned.type = src_copy.type;
  cloned.span = src_copy.span;

  // Deep-clone payload, remapping sub-expression references.
  std::visit(
      [&](const auto& p) {
        using T = std::decay_t<decltype(p)>;
        if constexpr (std::is_same_v<T, IntLiteral> ||
                      std::is_same_v<T, FloatLiteral> ||
                      std::is_same_v<T, StringLiteral> ||
                      std::is_same_v<T, CharLiteral> ||
                      std::is_same_v<T, LabelAddrExpr> ||
                      std::is_same_v<T, SizeofTypeExpr>) {
          // Leaf nodes: copy as-is.
          cloned.payload = p;
        } else if constexpr (std::is_same_v<T, DeclRef>) {
          DeclRef r = p;
          // Phase 3: rewrite callee parameter references to synthetic locals.
          if (r.param_index) {
            auto pit = ctx.param_to_local.find(*r.param_index);
            if (pit != ctx.param_to_local.end()) {
              r.local = pit->second;
              r.param_index = std::nullopt;
            }
          }
          // Remap local reference if it was cloned.
          if (r.local) {
            LocalId mapped = ctx.lookup_local(*r.local);
            if (mapped.valid()) r.local = mapped;
          }
          cloned.payload = r;
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
          UnaryExpr u = p;
          u.operand = remap_sub_expr(ctx, u.operand);
          cloned.payload = u;
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
          BinaryExpr b = p;
          b.lhs = remap_sub_expr(ctx, b.lhs);
          b.rhs = remap_sub_expr(ctx, b.rhs);
          cloned.payload = b;
        } else if constexpr (std::is_same_v<T, AssignExpr>) {
          AssignExpr a = p;
          a.lhs = remap_sub_expr(ctx, a.lhs);
          a.rhs = remap_sub_expr(ctx, a.rhs);
          cloned.payload = a;
        } else if constexpr (std::is_same_v<T, CastExpr>) {
          CastExpr c = p;
          c.expr = remap_sub_expr(ctx, c.expr);
          cloned.payload = c;
        } else if constexpr (std::is_same_v<T, CallExpr>) {
          CallExpr c = p;
          c.callee = remap_sub_expr(ctx, c.callee);
          for (auto& arg : c.args) {
            arg = remap_sub_expr(ctx, arg);
          }
          cloned.payload = c;
        } else if constexpr (std::is_same_v<T, VaArgExpr>) {
          VaArgExpr v = p;
          v.ap = remap_sub_expr(ctx, v.ap);
          cloned.payload = v;
        } else if constexpr (std::is_same_v<T, IndexExpr>) {
          IndexExpr i = p;
          i.base = remap_sub_expr(ctx, i.base);
          i.index = remap_sub_expr(ctx, i.index);
          cloned.payload = i;
        } else if constexpr (std::is_same_v<T, MemberExpr>) {
          MemberExpr m = p;
          m.base = remap_sub_expr(ctx, m.base);
          cloned.payload = m;
        } else if constexpr (std::is_same_v<T, TernaryExpr>) {
          TernaryExpr t = p;
          t.cond = remap_sub_expr(ctx, t.cond);
          t.then_expr = remap_sub_expr(ctx, t.then_expr);
          t.else_expr = remap_sub_expr(ctx, t.else_expr);
          cloned.payload = t;
        } else if constexpr (std::is_same_v<T, SizeofExpr>) {
          SizeofExpr s = p;
          s.expr = remap_sub_expr(ctx, s.expr);
          cloned.payload = s;
        }
      },
      src_copy.payload);

  ctx.module->expr_pool.push_back(std::move(cloned));
  return new_id;
}

// ── clone_stmt ───────────────────────────────────────────────────────────────

Stmt clone_stmt(InlineCloneContext& ctx, const Stmt& src) {
  Stmt cloned;
  cloned.span = src.span;

  std::visit(
      [&](const auto& p) {
        using T = std::decay_t<decltype(p)>;
        if constexpr (std::is_same_v<T, LocalDecl>) {
          LocalDecl d = p;
          d.id = ctx.remap_local(d.id);
          d.init = remap_opt_expr(ctx, d.init);
          d.vla_size = remap_opt_expr(ctx, d.vla_size);
          cloned.payload = d;
        } else if constexpr (std::is_same_v<T, ExprStmt>) {
          ExprStmt s = p;
          s.expr = remap_opt_expr(ctx, s.expr);
          cloned.payload = s;
        } else if constexpr (std::is_same_v<T, InlineAsmStmt>) {
          InlineAsmStmt a = p;
          if (a.output) a.output = remap_sub_expr(ctx, *a.output);
          for (auto& inp : a.inputs) inp = remap_sub_expr(ctx, inp);
          cloned.payload = a;
        } else if constexpr (std::is_same_v<T, ReturnStmt>) {
          ReturnStmt r = p;
          r.expr = remap_opt_expr(ctx, r.expr);
          cloned.payload = r;
        } else if constexpr (std::is_same_v<T, IfStmt>) {
          IfStmt s = p;
          s.cond = remap_sub_expr(ctx, s.cond);
          s.then_block = remap_block_ref(ctx, s.then_block);
          s.else_block = remap_opt_block(ctx, s.else_block);
          s.after_block = remap_block_ref(ctx, s.after_block);
          cloned.payload = s;
        } else if constexpr (std::is_same_v<T, WhileStmt>) {
          WhileStmt s = p;
          s.cond = remap_sub_expr(ctx, s.cond);
          s.body_block = remap_block_ref(ctx, s.body_block);
          s.continue_target = remap_opt_block(ctx, s.continue_target);
          s.break_target = remap_opt_block(ctx, s.break_target);
          cloned.payload = s;
        } else if constexpr (std::is_same_v<T, ForStmt>) {
          ForStmt s = p;
          s.init = remap_opt_expr(ctx, s.init);
          s.cond = remap_opt_expr(ctx, s.cond);
          s.update = remap_opt_expr(ctx, s.update);
          s.body_block = remap_block_ref(ctx, s.body_block);
          s.continue_target = remap_opt_block(ctx, s.continue_target);
          s.break_target = remap_opt_block(ctx, s.break_target);
          cloned.payload = s;
        } else if constexpr (std::is_same_v<T, DoWhileStmt>) {
          DoWhileStmt s = p;
          s.body_block = remap_block_ref(ctx, s.body_block);
          s.cond = remap_sub_expr(ctx, s.cond);
          s.continue_target = remap_opt_block(ctx, s.continue_target);
          s.break_target = remap_opt_block(ctx, s.break_target);
          cloned.payload = s;
        } else if constexpr (std::is_same_v<T, SwitchStmt>) {
          SwitchStmt s = p;
          s.cond = remap_sub_expr(ctx, s.cond);
          s.body_block = remap_block_ref(ctx, s.body_block);
          s.default_block = remap_opt_block(ctx, s.default_block);
          s.break_block = remap_opt_block(ctx, s.break_block);
          for (auto& [val, bid] : s.case_blocks) {
            bid = remap_block_ref(ctx, bid);
          }
          for (auto& [lo, hi, bid] : s.case_range_blocks) {
            bid = remap_block_ref(ctx, bid);
          }
          cloned.payload = s;
        } else if constexpr (std::is_same_v<T, GotoStmt>) {
          GotoStmt g = p;
          g.target.resolved_block = remap_block_ref(ctx, g.target.resolved_block);
          cloned.payload = g;
        } else if constexpr (std::is_same_v<T, IndirBrStmt>) {
          IndirBrStmt i = p;
          i.target = remap_sub_expr(ctx, i.target);
          cloned.payload = i;
        } else if constexpr (std::is_same_v<T, BreakStmt>) {
          BreakStmt b = p;
          b.target = remap_opt_block(ctx, b.target);
          cloned.payload = b;
        } else if constexpr (std::is_same_v<T, ContinueStmt>) {
          ContinueStmt c = p;
          c.target = remap_opt_block(ctx, c.target);
          cloned.payload = c;
        } else {
          // LabelStmt, CaseStmt, CaseRangeStmt, DefaultStmt: no IDs to remap.
          cloned.payload = p;
        }
      },
      src.payload);

  return cloned;
}

// ── clone_block ──────────────────────────────────────────────────────────────

Block clone_block(InlineCloneContext& ctx, const Block& src) {
  Block cloned;
  cloned.id = remap_block_ref(ctx, src.id);
  cloned.has_explicit_terminator = src.has_explicit_terminator;
  cloned.stmts.reserve(src.stmts.size());
  for (const auto& stmt : src.stmts) {
    cloned.stmts.push_back(clone_stmt(ctx, stmt));
  }
  return cloned;
}

// ── preallocate_block_ids ────────────────────────────────────────────────────

void preallocate_block_ids(InlineCloneContext& ctx, const Function& callee) {
  for (const auto& bb : callee.blocks) {
    ctx.remap_block(bb.id);
  }
}

// Forward declaration for use in eligibility check.
static bool contains_va_arg(const Module& module, const Function& fn);

// ── resolve_direct_callee ────────────────────────────────────────────────────

const Function* resolve_direct_callee(const Module& module, const CallExpr& call) {
  // The callee expr must be a DeclRef naming a function in fn_index.
  const Expr* callee_expr = module.find_expr(call.callee);
  if (!callee_expr) return nullptr;

  const auto* ref = std::get_if<DeclRef>(&callee_expr->payload);
  if (!ref) return nullptr;

  auto it = module.fn_index.find(ref->name);
  if (it == module.fn_index.end()) return nullptr;

  return module.find_function(it->second);
}

// ── check_inline_eligibility ─────────────────────────────────────────────────

InlineCandidate check_inline_eligibility(
    const Module& module,
    const Function& caller,
    const CallExpr& call,
    ExprId call_expr_id,
    BlockId block_id,
    size_t stmt_index) {

  InlineCandidate cand;
  cand.caller = &caller;
  cand.call_expr_id = call_expr_id;
  cand.caller_block = block_id;
  cand.stmt_index = stmt_index;

  // 1. Resolve direct callee.
  const Function* callee = resolve_direct_callee(module, call);
  if (!callee) {
    cand.reject = InlineRejectReason::IndirectCall;
    return cand;
  }
  cand.callee = callee;

  // 2. Callee must have a body (not just a declaration).
  if (callee->blocks.empty()) {
    cand.reject = InlineRejectReason::NoBody;
    return cand;
  }

  // 3. Callee must not be variadic.
  if (callee->attrs.variadic) {
    cand.reject = InlineRejectReason::Variadic;
    return cand;
  }

  // 4. Reject self-recursive calls.
  if (caller.id.value == callee->id.value) {
    cand.reject = InlineRejectReason::SelfRecursive;
    return cand;
  }

  // 5. Callee must not be noinline.
  if (callee->attrs.no_inline) {
    cand.reject = InlineRejectReason::CalleeIsNoinline;
    return cand;
  }

  // 6. Current policy: only expand always_inline callees.
  if (!callee->attrs.always_inline) {
    cand.reject = InlineRejectReason::NotAlwaysInline;
    return cand;
  }

  // 7. Skip callees whose body contains va_arg (ABI-sensitive).
  if (contains_va_arg(module, *callee)) {
    cand.reject = InlineRejectReason::VaArgInBody;
    return cand;
  }

  // All checks passed.
  cand.reject = InlineRejectReason::None;
  return cand;
}

// ── discover_inline_candidates ───────────────────────────────────────────────

/// Walk all expressions in a block's statements looking for CallExpr nodes.
/// For each found, run eligibility check.
static void scan_block_for_calls(
    const Module& module,
    const Function& fn,
    const Block& bb,
    std::vector<InlineCandidate>& out) {

  for (size_t si = 0; si < bb.stmts.size(); ++si) {
    const auto& stmt = bb.stmts[si];

    // Collect ExprIds that might contain calls.
    // We check top-level expression statements and return statements,
    // plus initializers in local declarations.
    auto check_expr = [&](ExprId eid) {
      const Expr* e = module.find_expr(eid);
      if (!e) return;
      if (const auto* call = std::get_if<CallExpr>(&e->payload)) {
        out.push_back(check_inline_eligibility(module, fn, *call, eid, bb.id, si));
      }
    };

    std::visit(
        [&](const auto& s) {
          using T = std::decay_t<decltype(s)>;
          if constexpr (std::is_same_v<T, ExprStmt>) {
            if (s.expr) check_expr(*s.expr);
          } else if constexpr (std::is_same_v<T, ReturnStmt>) {
            if (s.expr) check_expr(*s.expr);
          } else if constexpr (std::is_same_v<T, LocalDecl>) {
            if (s.init) check_expr(*s.init);
          }
        },
        stmt.payload);
  }
}

std::vector<InlineCandidate> discover_inline_candidates(const Module& module) {
  std::vector<InlineCandidate> candidates;

  for (const auto& fn : module.functions) {
    for (const auto& bb : fn.blocks) {
      scan_block_for_calls(module, fn, bb, candidates);
    }
  }

  return candidates;
}

// ── Phase 3: bind_callee_params ──────────────────────────────────────────────

std::vector<Stmt> bind_callee_params(
    InlineCloneContext& ctx,
    const Function& callee,
    const CallExpr& call) {

  std::vector<Stmt> param_decls;
  size_t n = callee.params.size();

  // Skip binding for (void) parameter lists: single void param with no args.
  if (n == 1 && call.args.empty() &&
      callee.params[0].type.spec.base == TB_VOID &&
      callee.params[0].type.spec.ptr_level == 0) {
    return param_decls;
  }
  assert(call.args.size() >= n && "not enough arguments for callee parameters");

  for (size_t i = 0; i < n; ++i) {
    const Param& param = callee.params[i];

    // Allocate a fresh local for this parameter capture.
    LocalId new_local = ctx.module->alloc_local_id();
    ctx.param_to_local[static_cast<uint32_t>(i)] = new_local;

    // Build a LocalDecl statement.
    // In C, array-typed parameters decay to pointers:
    //   int arr[]       → int*
    //   int arr[10]     → int*
    //   int arr[][3][3] → int (*)[3][3]
    // Strip the outermost dimension and add a pointer level.
    QualType param_type = param.type;
    if (param_type.spec.array_rank > 0) {
      param_type.spec.ptr_level += 1;
      param_type.spec.array_rank -= 1;
      if (param_type.spec.array_rank > 0) {
        // Multi-dimensional: shift dims left, keep inner dims.
        for (int d = 0; d < param_type.spec.array_rank; ++d) {
          param_type.spec.array_dims[d] = param_type.spec.array_dims[d + 1];
        }
        param_type.spec.array_size = param_type.spec.array_dims[0];
        param_type.spec.is_ptr_to_array = true;
      } else {
        // Single dimension: fully decayed to plain pointer.
        param_type.spec.array_size = -1;
      }
      param_type.spec.array_size_expr = nullptr;
    }

    LocalDecl decl;
    decl.id = new_local;
    decl.name = ctx.debug_prefix + param.name;
    decl.type = param_type;
    decl.fn_ptr_sig = param.fn_ptr_sig;
    decl.storage = StorageClass::Auto;
    decl.init = call.args[i];  // argument expression, already evaluated once

    Stmt stmt;
    stmt.payload = decl;
    param_decls.push_back(std::move(stmt));
  }

  return param_decls;
}

// ── Phase 4+5: CFG Splice ────────────────────────────────────────────────────

/// Check if any expression in the module's expr_pool that is reachable from
/// a function's body contains VaArgExpr.
static bool contains_va_arg(const Module& module, const Function& fn) {
  std::vector<ExprId> worklist;
  auto enqueue = [&](ExprId id) { worklist.push_back(id); };

  for (const auto& bb : fn.blocks) {
    for (const auto& stmt : bb.stmts) {
      std::visit([&](const auto& s) {
        using T = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<T, ExprStmt>) {
          if (s.expr) enqueue(*s.expr);
        } else if constexpr (std::is_same_v<T, ReturnStmt>) {
          if (s.expr) enqueue(*s.expr);
        } else if constexpr (std::is_same_v<T, LocalDecl>) {
          if (s.init) enqueue(*s.init);
        } else if constexpr (std::is_same_v<T, IfStmt>) {
          enqueue(s.cond);
        }
      }, stmt.payload);
    }
  }

  // BFS through expression tree.
  std::vector<bool> visited(module.expr_pool.size(), false);
  while (!worklist.empty()) {
    ExprId eid = worklist.back();
    worklist.pop_back();
    const Expr* e = module.find_expr(eid);
    if (!e) continue;
    size_t idx = 0;
    for (size_t i = 0; i < module.expr_pool.size(); ++i) {
      if (module.expr_pool[i].id.value == eid.value) { idx = i; break; }
    }
    if (visited[idx]) continue;
    visited[idx] = true;

    if (std::holds_alternative<VaArgExpr>(e->payload)) return true;

    // Recurse into sub-expressions.
    std::visit([&](const auto& p) {
      using T = std::decay_t<decltype(p)>;
      if constexpr (std::is_same_v<T, UnaryExpr>) { enqueue(p.operand); }
      else if constexpr (std::is_same_v<T, BinaryExpr>) { enqueue(p.lhs); enqueue(p.rhs); }
      else if constexpr (std::is_same_v<T, AssignExpr>) { enqueue(p.lhs); enqueue(p.rhs); }
      else if constexpr (std::is_same_v<T, CastExpr>) { enqueue(p.expr); }
      else if constexpr (std::is_same_v<T, CallExpr>) {
        enqueue(p.callee);
        for (const auto& a : p.args) enqueue(a);
      }
      else if constexpr (std::is_same_v<T, IndexExpr>) { enqueue(p.base); enqueue(p.index); }
      else if constexpr (std::is_same_v<T, MemberExpr>) { enqueue(p.base); }
      else if constexpr (std::is_same_v<T, TernaryExpr>) {
        enqueue(p.cond); enqueue(p.then_expr); enqueue(p.else_expr);
      }
      else if constexpr (std::is_same_v<T, SizeofExpr>) { enqueue(p.expr); }
    }, e->payload);
  }
  return false;
}

/// Check if a callee's return type is void (no pointer indirection).
static bool is_void_return(const Function& fn) {
  return fn.return_type.spec.base == TB_VOID &&
         fn.return_type.spec.ptr_level == 0;
}

/// Create a DeclRef expression referencing a local variable.
static ExprId make_local_ref(Module& module, LocalId local, const std::string& name,
                             QualType type) {
  ExprId id = module.alloc_expr_id();
  Expr e;
  e.id = id;
  e.type = type;
  DeclRef ref;
  ref.name = name;
  ref.local = local;
  e.payload = ref;
  module.expr_pool.push_back(std::move(e));
  return id;
}

/// Create an AssignExpr (lhs = rhs) and return the wrapping ExprId.
static ExprId make_assign(Module& module, ExprId lhs, ExprId rhs, QualType type) {
  ExprId id = module.alloc_expr_id();
  Expr e;
  e.id = id;
  e.type = type;
  AssignExpr assign;
  assign.op = AssignOp::Set;
  assign.lhs = lhs;
  assign.rhs = rhs;
  e.payload = assign;
  module.expr_pool.push_back(std::move(e));
  return id;
}

/// Perform inline expansion for one candidate.
/// Returns true if the expansion was performed, false if skipped.
static bool expand_inline_site(Module& module, Function& caller,
                               const InlineCandidate& cand) {
  const Function& callee = *cand.callee;

  // Restrictions:
  // - no va_arg in callee (va_list copy semantics are ABI-sensitive)
  if (contains_va_arg(module, callee)) return false;

  bool is_void = is_void_return(callee);

  // Find caller block by linear scan (pointer into vector).
  Block* caller_block = nullptr;
  size_t caller_block_idx = 0;
  for (size_t i = 0; i < caller.blocks.size(); ++i) {
    if (caller.blocks[i].id.value == cand.caller_block.value) {
      caller_block = &caller.blocks[i];
      caller_block_idx = i;
      break;
    }
  }
  if (!caller_block) return false;
  if (cand.stmt_index >= caller_block->stmts.size()) return false;

  // Get the call expression.  Copy it to avoid iterator invalidation
  // (subsequent make_local_ref/make_assign calls push to expr_pool,
  // which may reallocate and invalidate pointers into it).
  const Expr* call_expr_ptr = module.find_expr(cand.call_expr_id);
  if (!call_expr_ptr) return false;
  const auto* call_ptr = std::get_if<CallExpr>(&call_expr_ptr->payload);
  if (!call_ptr) return false;
  CallExpr call_copy = *call_ptr;
  const CallExpr* call = &call_copy;

  // Determine the statement context of the call.
  const Stmt& call_stmt = caller_block->stmts[cand.stmt_index];
  enum class Ctx { ExprStmt, LocalDecl, Return };
  Ctx ctx_kind;
  if (std::holds_alternative<ExprStmt>(call_stmt.payload)) {
    ctx_kind = Ctx::ExprStmt;
  } else if (std::holds_alternative<LocalDecl>(call_stmt.payload)) {
    ctx_kind = Ctx::LocalDecl;
  } else if (std::holds_alternative<ReturnStmt>(call_stmt.payload)) {
    ctx_kind = Ctx::Return;
  } else {
    return false;  // unsupported call context
  }

  // --- 1. Create continuation block ---
  BlockId cont_id = module.alloc_block_id();
  Block cont_block;
  cont_block.id = cont_id;
  cont_block.has_explicit_terminator = caller_block->has_explicit_terminator;

  // Move stmts after stmt_index to continuation.
  for (size_t i = cand.stmt_index + 1; i < caller_block->stmts.size(); ++i) {
    cont_block.stmts.push_back(std::move(caller_block->stmts[i]));
  }

  // --- 2. Create result local if needed ---
  LocalId result_local = LocalId::invalid();
  std::string result_name;
  if (!is_void && ctx_kind != Ctx::ExprStmt) {
    result_local = module.alloc_local_id();
    result_name = "inl_" + callee.name + "_result";
  }

  // --- 3. Handle the call statement (rebuild in continuation) ---
  if (ctx_kind == Ctx::LocalDecl) {
    // Move LocalDecl to start of continuation, replacing init with result ref.
    LocalDecl decl = std::get<LocalDecl>(call_stmt.payload);
    QualType ref_type = callee.return_type;
    ref_type.category = ValueCategory::LValue;
    ExprId ref_id = make_local_ref(module, result_local, result_name, ref_type);
    decl.init = ref_id;
    Stmt s;
    s.payload = decl;
    s.span = call_stmt.span;
    cont_block.stmts.insert(cont_block.stmts.begin(), std::move(s));
  } else if (ctx_kind == Ctx::Return) {
    if (!is_void) {
      QualType ref_type = callee.return_type;
      ref_type.category = ValueCategory::LValue;
      ExprId ref_id = make_local_ref(module, result_local, result_name, ref_type);
      ReturnStmt ret;
      ret.expr = ref_id;
      Stmt s;
      s.payload = ret;
      s.span = call_stmt.span;
      cont_block.stmts.insert(cont_block.stmts.begin(), std::move(s));
    } else {
      ReturnStmt ret;
      Stmt s;
      s.payload = ret;
      s.span = call_stmt.span;
      cont_block.stmts.insert(cont_block.stmts.begin(), std::move(s));
    }
    cont_block.has_explicit_terminator = true;
  }
  // For ExprStmt: nothing needed in continuation (call result is discarded).

  // --- 4. Truncate caller block to [0, stmt_index) ---
  caller_block->stmts.resize(cand.stmt_index);

  // --- 5. Declare result local at end of pre-call block ---
  if (result_local.valid()) {
    LocalDecl result_decl;
    result_decl.id = result_local;
    result_decl.name = result_name;
    result_decl.type = callee.return_type;
    result_decl.storage = StorageClass::Auto;
    Stmt s;
    s.payload = result_decl;
    caller_block->stmts.push_back(std::move(s));
  }

  // --- 6. Set up clone context and bind parameters ---
  InlineCloneContext ctx;
  ctx.module = &module;
  ctx.debug_prefix = "inl_" + callee.name + "_";

  auto param_stmts = bind_callee_params(ctx, callee, *call);
  for (auto& s : param_stmts) {
    caller_block->stmts.push_back(std::move(s));
  }

  // --- 7. Clone callee blocks ---
  preallocate_block_ids(ctx, callee);

  std::vector<Block> cloned_blocks;
  for (const auto& bb : callee.blocks) {
    cloned_blocks.push_back(clone_block(ctx, bb));
  }

  // --- 8. Rewrite all ReturnStmt in cloned blocks ---
  for (auto& bb : cloned_blocks) {
    for (size_t i = 0; i < bb.stmts.size(); ++i) {
      auto* ret = std::get_if<ReturnStmt>(&bb.stmts[i].payload);
      if (!ret) continue;

      if (result_local.valid() && ret->expr) {
        // result_local = return_expr; goto continuation
        QualType lhs_type = callee.return_type;
        lhs_type.category = ValueCategory::LValue;
        ExprId lhs_id = make_local_ref(module, result_local, result_name, lhs_type);
        ExprId assign_id = make_assign(module, lhs_id, *ret->expr, callee.return_type);

        Stmt assign_stmt;
        ExprStmt es;
        es.expr = assign_id;
        assign_stmt.payload = es;

        GotoStmt gs;
        gs.target.resolved_block = cont_id;
        Stmt goto_stmt;
        goto_stmt.payload = gs;

        bb.stmts[i] = std::move(assign_stmt);
        bb.stmts.insert(bb.stmts.begin() + static_cast<long>(i) + 1,
                         std::move(goto_stmt));
        // Skip past the inserted goto
        ++i;
      } else {
        // Void or no expr: just goto continuation.
        GotoStmt gs;
        gs.target.resolved_block = cont_id;
        Stmt goto_stmt;
        goto_stmt.payload = gs;
        bb.stmts[i] = std::move(goto_stmt);
      }
      bb.has_explicit_terminator = true;
      // After rewriting a return, remaining stmts in this block are dead code.
      // Trim them for cleanliness.
      bb.stmts.resize(i + 1);
      break;
    }
  }

  // For cloned blocks that have no explicit terminator (e.g., void callee
  // with no return statement), add a goto to the continuation block.
  for (auto& bb : cloned_blocks) {
    if (!bb.has_explicit_terminator) {
      GotoStmt gs;
      gs.target.resolved_block = cont_id;
      Stmt goto_stmt;
      goto_stmt.payload = gs;
      bb.stmts.push_back(std::move(goto_stmt));
      bb.has_explicit_terminator = true;
    }
  }

  // --- 9. Add goto from pre-call block to callee entry ---
  BlockId callee_entry_clone = ctx.lookup_block(callee.entry);
  assert(callee_entry_clone.valid());
  GotoStmt entry_goto;
  entry_goto.target.resolved_block = callee_entry_clone;
  Stmt entry_goto_stmt;
  entry_goto_stmt.payload = entry_goto;
  caller_block->stmts.push_back(std::move(entry_goto_stmt));
  caller_block->has_explicit_terminator = true;

  // --- 10. Insert cloned blocks + continuation into caller ---
  // IMPORTANT: after inserting, caller_block pointer is invalidated.
  auto insert_pos = caller.blocks.begin() +
                    static_cast<long>(caller_block_idx) + 1;
  for (auto& cb : cloned_blocks) {
    insert_pos = caller.blocks.insert(insert_pos, std::move(cb));
    ++insert_pos;
  }
  caller.blocks.insert(insert_pos, std::move(cont_block));

  return true;
}

// ── Phase 6: Expression-context normalization ────────────────────────────────

/// Check if a CallExpr is eligible for hoisting (inline-eligible, non-void).
static bool is_hoistable_inline_call(const Module& module, ExprId eid) {
  const Expr* e = module.find_expr(eid);
  if (!e) return false;
  const auto* call = std::get_if<CallExpr>(&e->payload);
  if (!call) return false;

  const Function* callee = resolve_direct_callee(module, *call);
  if (!callee) return false;
  if (callee->blocks.empty()) return false;
  if (callee->attrs.variadic) return false;
  if (callee->attrs.no_inline) return false;
  if (!callee->attrs.always_inline) return false;
  if (is_void_return(*callee)) return false;

  return true;
}

/// Recursively walk an expression tree. If any sub-expression (not the root)
/// is an inline-eligible call, hoist it: create a temp LocalDecl initialized
/// with the call, and replace the sub-expression reference with a DeclRef
/// to the temp. Hoisted stmts are collected in `hoisted`.
/// Returns the (possibly new) ExprId for this position.
static ExprId hoist_nested_calls(Module& module, ExprId eid,
                                 std::vector<Stmt>& hoisted,
                                 bool is_root) {
  const Expr* e = module.find_expr(eid);
  if (!e) return eid;

  // If this is a non-root inline-eligible call, hoist it.
  if (!is_root && is_hoistable_inline_call(module, eid)) {
    const Function* callee = resolve_direct_callee(
        module, std::get<CallExpr>(e->payload));

    // First, recurse into this call's arguments to hoist nested calls there.
    // Copy args to avoid invalidation during recursion.
    auto args_copy = std::get<CallExpr>(e->payload).args;
    for (auto& arg : args_copy) {
      arg = hoist_nested_calls(module, arg, hoisted, false);
    }
    // Write updated args back (re-lookup after possible reallocation).
    Expr* me_updated = module.find_expr(eid);
    if (me_updated) std::get<CallExpr>(me_updated->payload).args = std::move(args_copy);

    // Now hoist this call itself.
    LocalId tmp_id = module.alloc_local_id();
    std::string tmp_name = "inl_hoist_" + callee->name;

    LocalDecl decl;
    decl.id = tmp_id;
    decl.name = tmp_name;
    decl.type = callee->return_type;
    decl.storage = StorageClass::Auto;
    decl.init = eid;

    Stmt s;
    s.payload = decl;
    hoisted.push_back(std::move(s));

    // Create DeclRef referencing the temp.
    QualType ref_type = callee->return_type;
    ref_type.category = ValueCategory::LValue;
    return make_local_ref(module, tmp_id, tmp_name, ref_type);
  }

  // Recurse into sub-expressions and update references.
  // IMPORTANT: We must copy the payload before recursing because recursive
  // calls may push to expr_pool, invalidating pointers into it.
  Expr* me = module.find_expr(eid);
  if (!me) return eid;

  // Copy payload by value to avoid invalidation during recursion.
  auto payload_copy = me->payload;

  std::visit(
      [&](auto& p) {
        using T = std::decay_t<decltype(p)>;
        if constexpr (std::is_same_v<T, UnaryExpr>) {
          p.operand = hoist_nested_calls(module, p.operand, hoisted, false);
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
          p.lhs = hoist_nested_calls(module, p.lhs, hoisted, false);
          p.rhs = hoist_nested_calls(module, p.rhs, hoisted, false);
        } else if constexpr (std::is_same_v<T, AssignExpr>) {
          // Don't hoist LHS (it's an lvalue), only RHS.
          p.rhs = hoist_nested_calls(module, p.rhs, hoisted, false);
        } else if constexpr (std::is_same_v<T, CastExpr>) {
          p.expr = hoist_nested_calls(module, p.expr, hoisted, false);
        } else if constexpr (std::is_same_v<T, CallExpr>) {
          // Recurse into arguments of calls (including non-inline calls).
          for (auto& arg : p.args) {
            arg = hoist_nested_calls(module, arg, hoisted, false);
          }
        } else if constexpr (std::is_same_v<T, IndexExpr>) {
          p.base = hoist_nested_calls(module, p.base, hoisted, false);
          p.index = hoist_nested_calls(module, p.index, hoisted, false);
        } else if constexpr (std::is_same_v<T, MemberExpr>) {
          p.base = hoist_nested_calls(module, p.base, hoisted, false);
        } else if constexpr (std::is_same_v<T, TernaryExpr>) {
          // Hoist calls out of ternary operands.
          p.cond = hoist_nested_calls(module, p.cond, hoisted, false);
          p.then_expr = hoist_nested_calls(module, p.then_expr, hoisted, false);
          p.else_expr = hoist_nested_calls(module, p.else_expr, hoisted, false);
        } else if constexpr (std::is_same_v<T, SizeofExpr>) {
          // sizeof operand is not evaluated, skip.
        }
        // Leaf types (IntLiteral, FloatLiteral, StringLiteral, CharLiteral,
        // DeclRef, LabelAddrExpr, SizeofTypeExpr, VaArgExpr): nothing to do.
      },
      payload_copy);

  // Write modified payload back (re-lookup in case expr_pool was reallocated).
  Expr* me2 = module.find_expr(eid);
  if (me2) me2->payload = std::move(payload_copy);

  return eid;
}

/// Normalize a single statement: hoist inline-eligible calls from
/// non-top-level expression positions into temp LocalDecls.
/// Returns the list of Stmts to insert before this statement.
static std::vector<Stmt> normalize_stmt_inline_calls(Module& module,
                                                      Stmt& stmt) {
  std::vector<Stmt> hoisted;

  std::visit(
      [&](auto& s) {
        using T = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<T, ExprStmt>) {
          if (s.expr) {
            // The root expression can be a call (handled by existing inliner).
            // But its sub-expressions may also contain calls.
            // Walk the expression tree, treating the top as root.
            s.expr = hoist_nested_calls(module, *s.expr, hoisted, true);
          }
        } else if constexpr (std::is_same_v<T, ReturnStmt>) {
          if (s.expr) {
            s.expr = hoist_nested_calls(module, *s.expr, hoisted, true);
          }
        } else if constexpr (std::is_same_v<T, LocalDecl>) {
          if (s.init) {
            s.init = hoist_nested_calls(module, *s.init, hoisted, true);
          }
        } else if constexpr (std::is_same_v<T, IfStmt>) {
          s.cond = hoist_nested_calls(module, s.cond, hoisted, false);
        } else if constexpr (std::is_same_v<T, WhileStmt>) {
          // Don't hoist from loop conditions — they're re-evaluated each iteration.
        } else if constexpr (std::is_same_v<T, ForStmt>) {
          // Don't hoist from for-loop parts — they may be re-evaluated.
        } else if constexpr (std::is_same_v<T, DoWhileStmt>) {
          // Don't hoist from do-while condition.
        } else if constexpr (std::is_same_v<T, SwitchStmt>) {
          s.cond = hoist_nested_calls(module, s.cond, hoisted, false);
        }
      },
      stmt.payload);

  return hoisted;
}

/// Run normalization across all functions: hoist nested inline-eligible calls
/// out of expression trees into temporary LocalDecls.
static void normalize_inline_calls(Module& module) {
  for (auto& fn : module.functions) {
    for (auto& bb : fn.blocks) {
      // Process statements, inserting hoisted decls before each stmt.
      // Iterate carefully since we're inserting into the vector.
      for (size_t i = 0; i < bb.stmts.size(); ++i) {
        auto hoisted = normalize_stmt_inline_calls(module, bb.stmts[i]);
        if (!hoisted.empty()) {
          bb.stmts.insert(bb.stmts.begin() + static_cast<long>(i),
                          std::make_move_iterator(hoisted.begin()),
                          std::make_move_iterator(hoisted.end()));
          i += hoisted.size();  // skip past inserted stmts
        }
      }
    }
  }
}

// ── run_inline_expansion ─────────────────────────────────────────────────────

/// Recursively walk an expression tree looking for CallExpr nodes that
/// reference always_inline functions. Used for diagnostics only.
static void find_always_inline_calls_in_expr(
    const Module& module,
    const Function& caller,
    ExprId eid,
    std::vector<InlineCandidate>& out,
    std::vector<bool>& visited) {
  const Expr* e = module.find_expr(eid);
  if (!e) return;

  // Avoid visiting the same expression twice.
  for (size_t i = 0; i < module.expr_pool.size(); ++i) {
    if (module.expr_pool[i].id.value == eid.value) {
      if (visited[i]) return;
      visited[i] = true;
      break;
    }
  }

  // If this is a CallExpr, check eligibility.
  if (const auto* call = std::get_if<CallExpr>(&e->payload)) {
    auto cand = check_inline_eligibility(
        module, caller, *call, eid, BlockId::invalid(), 0);
    // Only collect rejected always_inline calls.
    if (!cand.eligible() && cand.callee && cand.callee->attrs.always_inline &&
        cand.reject != InlineRejectReason::NotAlwaysInline) {
      out.push_back(cand);
    }
    // Recurse into args.
    for (const auto& arg : call->args) {
      find_always_inline_calls_in_expr(module, caller, arg, out, visited);
    }
    find_always_inline_calls_in_expr(module, caller, call->callee, out, visited);
    return;
  }

  // Recurse into sub-expressions.
  std::visit([&](const auto& p) {
    using T = std::decay_t<decltype(p)>;
    if constexpr (std::is_same_v<T, UnaryExpr>) {
      find_always_inline_calls_in_expr(module, caller, p.operand, out, visited);
    } else if constexpr (std::is_same_v<T, BinaryExpr>) {
      find_always_inline_calls_in_expr(module, caller, p.lhs, out, visited);
      find_always_inline_calls_in_expr(module, caller, p.rhs, out, visited);
    } else if constexpr (std::is_same_v<T, AssignExpr>) {
      find_always_inline_calls_in_expr(module, caller, p.lhs, out, visited);
      find_always_inline_calls_in_expr(module, caller, p.rhs, out, visited);
    } else if constexpr (std::is_same_v<T, CastExpr>) {
      find_always_inline_calls_in_expr(module, caller, p.expr, out, visited);
    } else if constexpr (std::is_same_v<T, IndexExpr>) {
      find_always_inline_calls_in_expr(module, caller, p.base, out, visited);
      find_always_inline_calls_in_expr(module, caller, p.index, out, visited);
    } else if constexpr (std::is_same_v<T, MemberExpr>) {
      find_always_inline_calls_in_expr(module, caller, p.base, out, visited);
    } else if constexpr (std::is_same_v<T, TernaryExpr>) {
      find_always_inline_calls_in_expr(module, caller, p.cond, out, visited);
      find_always_inline_calls_in_expr(module, caller, p.then_expr, out, visited);
      find_always_inline_calls_in_expr(module, caller, p.else_expr, out, visited);
    } else if constexpr (std::is_same_v<T, SizeofExpr>) {
      find_always_inline_calls_in_expr(module, caller, p.expr, out, visited);
    }
  }, e->payload);
}

// ── Phase 8: Backend Coordination ────────────────────────────────────────────

/// Scan all expressions in the module for CallExpr nodes referencing a function.
/// Returns true if at least one call site still references the given function.
static bool has_remaining_call_sites(const Module& module, const Function& target) {
  for (const auto& fn : module.functions) {
    for (const auto& bb : fn.blocks) {
      for (const auto& stmt : bb.stmts) {
        // Check expression roots from all statement types.
        auto check_expr = [&](ExprId eid, auto&& self) -> bool {
          const Expr* e = module.find_expr(eid);
          if (!e) return false;
          bool found = false;
          std::visit([&](const auto& p) {
            using T = std::decay_t<decltype(p)>;
            if constexpr (std::is_same_v<T, CallExpr>) {
              const Function* callee = resolve_direct_callee(module, p);
              if (callee && callee->id.value == target.id.value) {
                found = true;
                return;
              }
              for (const auto& arg : p.args) {
                if (self(arg, self)) { found = true; return; }
              }
            } else if constexpr (std::is_same_v<T, UnaryExpr>) {
              if (self(p.operand, self)) found = true;
            } else if constexpr (std::is_same_v<T, BinaryExpr>) {
              if (self(p.lhs, self) || self(p.rhs, self)) found = true;
            } else if constexpr (std::is_same_v<T, AssignExpr>) {
              if (self(p.lhs, self) || self(p.rhs, self)) found = true;
            } else if constexpr (std::is_same_v<T, CastExpr>) {
              if (self(p.expr, self)) found = true;
            } else if constexpr (std::is_same_v<T, IndexExpr>) {
              if (self(p.base, self) || self(p.index, self)) found = true;
            } else if constexpr (std::is_same_v<T, MemberExpr>) {
              if (self(p.base, self)) found = true;
            } else if constexpr (std::is_same_v<T, TernaryExpr>) {
              if (self(p.cond, self) || self(p.then_expr, self) || self(p.else_expr, self)) found = true;
            } else if constexpr (std::is_same_v<T, SizeofExpr>) {
              if (self(p.expr, self)) found = true;
            }
          }, e->payload);
          return found;
        };

        bool hit = false;
        std::visit([&](const auto& s) {
          using T = std::decay_t<decltype(s)>;
          if constexpr (std::is_same_v<T, ExprStmt>) {
            if (s.expr && check_expr(*s.expr, check_expr)) hit = true;
          } else if constexpr (std::is_same_v<T, ReturnStmt>) {
            if (s.expr && check_expr(*s.expr, check_expr)) hit = true;
          } else if constexpr (std::is_same_v<T, LocalDecl>) {
            if (s.init && check_expr(*s.init, check_expr)) hit = true;
          } else if constexpr (std::is_same_v<T, IfStmt>) {
            if (check_expr(s.cond, check_expr)) hit = true;
          } else if constexpr (std::is_same_v<T, WhileStmt>) {
            if (check_expr(s.cond, check_expr)) hit = true;
          } else if constexpr (std::is_same_v<T, ForStmt>) {
            if ((s.init && check_expr(*s.init, check_expr)) ||
                (s.cond && check_expr(*s.cond, check_expr)) ||
                (s.update && check_expr(*s.update, check_expr))) hit = true;
          } else if constexpr (std::is_same_v<T, DoWhileStmt>) {
            if (check_expr(s.cond, check_expr)) hit = true;
          } else if constexpr (std::is_same_v<T, SwitchStmt>) {
            if (check_expr(s.cond, check_expr)) hit = true;
          } else if constexpr (std::is_same_v<T, InlineAsmStmt>) {
            if (s.output && check_expr(*s.output, check_expr)) hit = true;
            for (const auto& inp : s.inputs) {
              if (check_expr(inp, check_expr)) hit = true;
            }
          }
        }, stmt.payload);
        if (hit) return true;
      }
    }
  }
  return false;
}

/// Post-expansion attribute reconciliation.
/// - For always_inline functions where ALL call sites were expanded: strip
///   the always_inline flag so LLVM IR won't carry a redundant alwaysinline
///   attribute that could confuse downstream passes.
/// - For non-inline functions (default policy, no always_inline): add noinline
///   to prevent accidental backend inlining if the IR is later optimized.
static void reconcile_backend_attrs(Module& module) {
  for (auto& fn : module.functions) {
    if (fn.attrs.always_inline && !fn.blocks.empty()) {
      if (!has_remaining_call_sites(module, fn)) {
        // All call sites were successfully expanded — strip alwaysinline.
        fn.attrs.always_inline = false;
      }
    }
  }
}

/// Emit a warning for always_inline callees that could not be expanded.
/// Walks all expressions in all functions to find remaining always_inline calls.
static void emit_inline_diagnostics(const Module& module) {
  std::vector<InlineCandidate> rejected;
  std::vector<bool> visited(module.expr_pool.size(), false);

  for (const auto& fn : module.functions) {
    for (const auto& bb : fn.blocks) {
      for (const auto& stmt : bb.stmts) {
        // Collect all expression roots from all statement types.
        auto scan_expr = [&](ExprId eid) {
          find_always_inline_calls_in_expr(module, fn, eid, rejected, visited);
        };
        std::visit([&](const auto& s) {
          using T = std::decay_t<decltype(s)>;
          if constexpr (std::is_same_v<T, ExprStmt>) {
            if (s.expr) scan_expr(*s.expr);
          } else if constexpr (std::is_same_v<T, ReturnStmt>) {
            if (s.expr) scan_expr(*s.expr);
          } else if constexpr (std::is_same_v<T, LocalDecl>) {
            if (s.init) scan_expr(*s.init);
          } else if constexpr (std::is_same_v<T, IfStmt>) {
            scan_expr(s.cond);
          } else if constexpr (std::is_same_v<T, WhileStmt>) {
            scan_expr(s.cond);
          } else if constexpr (std::is_same_v<T, ForStmt>) {
            if (s.init) scan_expr(*s.init);
            if (s.cond) scan_expr(*s.cond);
            if (s.update) scan_expr(*s.update);
          } else if constexpr (std::is_same_v<T, DoWhileStmt>) {
            scan_expr(s.cond);
          } else if constexpr (std::is_same_v<T, SwitchStmt>) {
            scan_expr(s.cond);
          } else if constexpr (std::is_same_v<T, InlineAsmStmt>) {
            if (s.output) scan_expr(*s.output);
            for (const auto& inp : s.inputs) scan_expr(inp);
          }
        }, stmt.payload);
      }
    }
  }

  // Deduplicate by callee name + caller name + reason.
  std::vector<std::tuple<std::string, std::string, InlineRejectReason>> seen;
  for (const auto& cand : rejected) {
    auto key = std::make_tuple(
        cand.callee->name,
        cand.caller ? cand.caller->name : std::string(),
        cand.reject);
    bool dup = false;
    for (const auto& s : seen) {
      if (s == key) { dup = true; break; }
    }
    if (dup) continue;
    seen.push_back(key);

    fprintf(stderr, "warning: always_inline function '%s' could not be "
            "inlined into '%s': %s\n",
            cand.callee->name.c_str(),
            cand.caller ? cand.caller->name.c_str() : "<unknown>",
            inline_reject_reason_str(cand.reject));
  }
}

void run_inline_expansion(Module& module) {
  // Phase 6: normalize nested inline calls into statement position first.
  normalize_inline_calls(module);
  // Iteratively discover and expand one candidate at a time.
  // Re-discover after each expansion because block/stmt indices change.
  bool changed = true;
  int max_iterations = 256;  // guard against infinite expansion
  // Per-callee expansion count to detect mutual recursion chains.
  // Cap each callee at 16 expansions — enough for reasonable always_inline
  // depth but prevents runaway mutual recursion (A→B→A→B→...).
  constexpr int max_per_callee = 16;
  OptionalDenseIdMap<FunctionId, int> callee_expand_count;
  while (changed && max_iterations-- > 0) {
    changed = false;
    auto candidates = discover_inline_candidates(module);
    for (const auto& cand : candidates) {
      if (!cand.eligible()) continue;

      // Skip callees that have already been expanded too many times
      // (mutual recursion guard).
      FunctionId callee_id = cand.callee->id;
      if (const int* expand_count = callee_expand_count.find(callee_id);
          expand_count && *expand_count >= max_per_callee) {
        continue;
      }

      // Find the mutable caller function.
      Function* caller = nullptr;
      for (auto& fn : module.functions) {
        if (fn.id.value == cand.caller->id.value) {
          caller = &fn;
          break;
        }
      }
      if (!caller) continue;

      if (expand_inline_site(module, *caller, cand)) {
        ++callee_expand_count.get_or_create(callee_id, [] { return 0; });
        changed = true;
        break;  // re-discover after each expansion
      }
    }
  }

  // Phase 7: emit diagnostics for always_inline calls that remain unexpanded.
  // Final pass: any remaining always_inline call sites that weren't expanded
  // get a warning (permissive mode — not an error).
  emit_inline_diagnostics(module);

  // Phase 8: reconcile backend attributes after expansion.
  // Strip alwaysinline from fully-expanded functions so LLVM IR doesn't carry
  // redundant attributes. This ensures the backend does not re-inline what the
  // frontend already expanded.
  reconcile_backend_attrs(module);
}

}  // namespace c4c::hir
