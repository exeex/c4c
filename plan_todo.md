# Plan Execution State

## Baseline
- 1818/1818 tests pass (2026-03-16)

## Current Phase: Phase 2 — Immediate-function interpretation

### Completed
- [x] Phase 1 (all tasks complete — see previous commits)
- [x] Phase 2, Task 1: Minimal function-body interpreter for consteval functions
  - Added `evaluate_consteval_call()` to consteval.cpp/hpp
  - Interpreter supports: parameter refs, local const decls, arithmetic, return, if/else, chained consteval calls
  - Lowerer collects consteval function defs in Phase 1.5 of `lower_program`
  - NK_CALL handler in `lower_expr` intercepts consteval calls with constant args → folds to IntLiteral
  - consteval_interp.cpp test covers: basic call, chained calls, local binding, if/else branching
  - Depth limit (64) prevents infinite recursion
- [x] Phase 2, Task 2: Mutable locals, loops, break/continue, assignments
  - Interpreter now supports: NK_ASSIGN, NK_FOR, NK_WHILE, NK_DO_WHILE, NK_BREAK, NK_CONTINUE, NK_EXPR_STMT, NK_COMMA_EXPR
  - Mutable locals: NK_DECL without initializer defaults to 0; NK_ASSIGN mutates locals map
  - Loop step counter (1M limit) prevents infinite loops
  - consteval_mutable.cpp test covers: for loop, while loop, do-while, assignment, break, continue
- [x] Phase 2, Task 3: Define failure modes / diagnostics
  - Added `error` field to `ConstEvalResult` with descriptive failure messages
  - Added `error` field to `StmtResult` for statement-level failure propagation
  - Every failure point in eval_impl, interp_expr, interp_stmt, evaluate_consteval_call now carries a message
  - Categories: unsupported expr kind, read of non-constant variable, call to non-consteval function, step/depth limit exceeded, missing return value, non-constant return expression
  - Currently diagnostics are hard errors (throw std::runtime_error)
- [x] Phase 2, Task 4: Hook call resolution
  - Failed consteval calls are now hard errors (throw std::runtime_error) — no runtime fallback
  - Consteval function bodies are skipped during HIR lowering (not emitted as runtime code)
  - interp_expr now handles NK_BINOP, NK_UNARY, NK_CAST, NK_TERNARY directly (not via eval_impl) so nested consteval calls in expressions are properly resolved
  - consteval_call_resolution.cpp test covers: factorial, fibonacci, chained consteval (choose), arithmetic over consteval results

### Not Started
- Phase 3: Enforce consteval rules (diagnose non-immediate calls, forbid &consteval, tighten decl rules)
- Phase 4: Integrate with if constexpr, builtins, templates

## Next Intended Slice
- Phase 3, Task 1: Diagnose non-immediate calls (consteval call with non-constant args is already a hard error; add negative tests)

## Blockers
- None
