# Sema / IR Split Migration Plan (Rebased)

Last updated: 2026-03-08

## Goal

Refactor pipeline from:

- `Parser -> IRBuilder (semantic + LLVM lowering mixed)`

to:

- `Parser -> Sema (validation + typed info) -> HIR -> Backend`

with these hard rules:

- `next` pipeline must not depend on legacy semantic behavior.
- `codegen` must not own semantic rejection logic.
- legacy path stays available only as comparison/fallback binary during migration.

## Why Rebase Now

Current state is effectively "Phase 3 half-done":

- HIR data model and AST->HIR lowering exist.
- Native HIR->LLVM emitter exists for a subset.
- But semantic rejection still depends on behavior historically living in legacy IRBuilder.

This causes architectural drift: failures in `next` are currently dominated by missing sema checks, not just missing codegen coverage.

## Target Architecture

```txt
parse
  -> sema_validate (hard errors + diagnostics)
  -> lower_ast_to_hir
  -> hir_to_llvm_native
  -> (optional) future hir_to_dag / native backend
```

Legacy components are allowed only for:

- parity diff tooling
- emergency debug path (`--pipeline=legacy`)

They are not allowed as required runtime dependencies of `--pipeline=hir`.

## Phase Plan

### Phase 0: Parallel Entry (completed)

- `tiny-c2ll-next` binary introduced.
- legacy binary remains test default during migration.

### Phase 1: Sema Helper Extraction (completed)

- `sema_driver` helper utilities extracted from IRBuilder.
- No behavior change target achieved.

### Phase 2: Minimal HIR (completed)

- `ir.hpp`, `ast_to_hir`, HIR summary/dump integrated.

### Phase 3A: Independent Sema Validation Gate (now)

Deliverables:

- Add explicit sema validation module in `src/frontend/sema/` (new pass API).
- `tiny-c2ll-next --pipeline=hir` flow becomes:
  - parse -> sema_validate -> lower_hir -> emit_native
- Remove any mandatory call path to legacy IRBuilder from `pipeline=hir`.

Initial validation scope (must block compile with non-zero exit):

- control-flow misuse: `break/continue/case/default` outside valid context
- symbol resolution: undeclared/undefined usage
- call arity mismatch for known prototypes
- const correctness:
  - assignment to const lvalue
  - passing const pointer to mutable parameter
  - dropping const in implicit pointer assignment
  - const object inc/dec
- invalid casts:
  - cast to incomplete struct/union object type
  - cast to unknown type name

Acceptance:

- All `negative_tests_*` pass under `tiny-c2ll-next` without legacy dependency.
- Failing sema diagnostics come from sema module, not codegen exceptions.

### Phase 3B: Native HIR->LLVM Coverage Expansion

Deliverables:

- Complete native emission for currently incomplete high-impact features:
  - struct/union member access
  - string/global initializers
  - switch/case dispatch completion
  - variadic call lowering path
- Keep sema and codegen responsibilities separated.

Acceptance:

- `tiny_c2ll_tests`, `ccc_review_*`, `c_testsuite_*` converge on parity for agreed subset.
- No semantic regressions introduced by codegen-only changes.

### Phase 3C: Bridge Retirement in `next`

Deliverables:

- Remove runtime dependency on bridge in default `next` path.
- Keep legacy invocation only in explicit comparison mode/tooling.

Acceptance:

- `--pipeline=hir` is self-contained.
- CI gate for non-torture suites no longer requires legacy bridge.

### Phase 4: DAG Layer Activation

Deliverables:

- Add `hir_to_dag` lowering skeleton + `--dump-dag` output.
- Keep LLVM backend functional while DAG grows.

Acceptance:

- representative files produce inspectable DAG output.

### Phase 5: Cutover Prep

Deliverables:

- Stabilize `next` on non-torture suites.
- Add parity diff tooling (legacy vs next outputs/diagnostics).

Acceptance:

- agreed suite parity and documented perf/memory baseline.

### Phase 6: Default Switch + Legacy Decommission

Deliverables:

- switch default compiler path to `next`
- remove legacy monolith after sustained green runs

Acceptance:

- team sign-off and transition window complete.

## Test Strategy (Rebased)

Execution order (mandatory):

1. Run aggregate gate first:
   - `cmake --build build --target ctest_core`
2. If `ctest_core` fails, fix in this practical priority:
   - `tiny_c2ll_tests` first
   - then `ccc_review_*`
   - then `c_testsuite_*`

Within `tiny_c2ll_tests`, `negative_tests` remain the first-fix subset
(semantic correctness gate).

Core aggregate gate:

- `cmake --build build --target ctest_core`

Focused rerun commands:

- `ctest -R tiny_c2ll_tests`
- `ctest -R '^negative_tests'`
- `ctest -R ccc_review`
- `ctest -R c_testsuite`

Exclusion policy in migration loop:

- `llvm_gcc_c_torture_*` excluded from default gate until Phase 5+

Timeout policy:

- default per-test timeout: 30s
- any >30s test runtime treated as suspicious regression unless explicitly documented

## Current Status Snapshot (2026-03-08)

- Phase 0/1/2: done.
- Phase 3: partially done.
  - native HIR emitter exists but sema validation is incomplete.
  - major failure mode is semantic rejection gaps, not parser stability.
- `next` architecture correction required now:
  - move hard semantic checks into sema pass before codegen.

## Immediate Next 3 PRs

1. PR-A: Introduce `sema_validate` pass and wire it into `tiny-c2ll-next` before HIR/codegen.
2. PR-B: Implement first-wave hard checks (negative suite core: control-flow misuse + const/cast/call arity).
3. PR-C: Remove mandatory legacy bridge calls from `--pipeline=hir`; keep bridge behind explicit legacy mode only.

## Guardrails

- No new semantic checks in `hir_to_llvm.cpp` unless they are IR structural invariants.
- Semantic user-facing diagnostics must originate from sema module.
- Any temporary fallback to legacy must be feature-flagged, off by default, and tracked with removal issue.
