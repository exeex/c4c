# Plan Execution State

## Baseline
- 1828/1828 tests pass (2026-03-16)

## Active Plan: HIR Inline Expansion (inline_plan.md)

### Completed
- [x] Phase 0: Attribute and Policy Plumbing
  - Added `is_noinline` and `is_always_inline` to TypeSpec (ast.hpp)
  - `parse_attributes()` now captures `noinline`/`__noinline__` and `always_inline`/`__always_inline__`
  - Replaced `skip_attributes()` with `parse_attributes()` at function-definition sites in declarations.cpp
  - Propagated to HIR `FnAttr.no_inline` and `FnAttr.always_inline` in ast_to_hir.cpp
  - LLVM IR emission emits `noinline`/`alwaysinline` function attributes in hir_emitter.cpp
  - Added `inline_attrs.c` test case validating attribute parsing and IR emission
  - Precedence: noinline and always_inline stored independently; noinline wins at inline-expansion time (Phase 1+)

### Next
- [ ] Phase 1: Call-site discovery and eligibility
  - Scan function bodies for CallExpr, resolve direct callees
  - Determine eligibility (has body, inline-always, not noinline, non-variadic, non-recursive)

### Previous Plan (Consteval) — Complete
- [x] Phase 1–4 of consteval plan fully implemented

## Blockers
- None
