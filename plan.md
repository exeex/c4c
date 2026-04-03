Status: Active
Source Idea: ideas/open/03_lir_to_backend_ir_refactor.md
Activated from: ideas/open/03_lir_to_backend_ir_refactor.md

# LIR To Backend IR Refactor Runbook

## Purpose

Finish the LIR-to-backend lowering cleanup by making the active refactor slices converge on structured backend semantics instead of open-ended compatibility widening.

## Goal

Turn the current `lir_adapter` boundary into an explicit backend-lowering path that:

- keeps syntax decoding inside lowering-owned code
- moves high-value backend IR seams to structured forms
- lets target emitters consume backend semantics instead of LLVM-shaped text

## Core Rule

Prefer convergence over coverage farming. When a family of cases differs only by bounded local-slot count or other size-only variation, replace per-`N` widening with one structural lowering rule and representative coverage.

## Read First

- `src/backend/lowering/lir_to_backend_ir.hpp`
- `src/backend/lowering/lir_to_backend_ir.cpp`
- `src/backend/lowering/call_decode.*`
- `src/backend/ir.hpp`
- `src/backend/aarch64/*`
- `src/backend/x86_64/*`
- `tests/backend/*`

## Scope

- Production lowering between LIR and backend IR
- Backend IR structure needed by current lowering and emitter slices
- Removal of emitter-local syntax-parsing dependencies in the migrated slices
- Backend-scoped validation for each migrated structural family

## Non-Goals

- No frontend semantic changes
- No broad backend redesign outside the active lowering seam
- No BIR scaffold work in this plan
- No unbounded `N -> N+1` matcher expansion when a shape is semantically the same

## Execution Rules

- Keep the existing backend path working throughout the transition.
- Prefer compatibility shims over flag-day rewrites when they reduce migration risk.
- Record genuinely separate initiatives in `ideas/open/` instead of silently broadening this plan.
- Close a step only when its exit criteria are structural, testable, and finite.

## Step 1: Re-establish the lowering boundary

- Keep the explicit lowering entrypoint and compatibility shim in place.
- Treat `src/backend/lowering/*` as the production ownership boundary.

Completion check:
- [x] A production lowering entrypoint exists with explicit backend-lowering intent.
- [x] The main production path is no longer described primarily as a "minimal adapter".
- [x] Backend-scoped tests covering the touched path pass.

## Step 2: Isolate LIR syntax decoding

- Keep syntax-decoding helpers behind lowering-owned code.
- Avoid adding new target-local parsing for migrated slices.

Completion check:
- [x] LIR decode responsibility is isolated to a narrow backend-lowering layer.
- [x] Target emitters do not gain new syntax-parsing dependencies during the slice.
- [x] Targeted backend tests for touched decode/call paths pass.

## Step 3: Make backend IR more backend-native

- Convert the active fallback-heavy lowering families from bounded compatibility matchers into structured backend lowering rules.
- Treat the branch-only constant-return local-slot family as structurally converged with respect to symbol naming, unused fixed parameters, and local-slot count.
- Focus remaining work on real semantic boundaries such as typed scalar coverage and backend-owned structure, not on entrypoint naming or arbitrary size caps.

Remaining work in this step is intentionally narrow:

1. generalize the current `i32`-only local-slot/value model only if that unlocks a real backend semantic family such as additional scalar widths or pointer-typed local state
2. reduce repetitive slot-count fixtures into representative coverage that documents the semantic boundary instead of enumerating arbitrary sizes

The following are explicitly out of scope for further Step 3 churn:

- adding new `N -> N+1` local-slot widening commits
- reintroducing gating by function name, fixed-parameter count, or local-slot count
- treating linker/runtime entrypoint concerns as backend-lowering constraints
- expanding this matcher into a generic direct-LIR interpreter

Exit criteria:
- [x] The branch-only constant-return family no longer relies on a bounded one-to-`N` local-slot matcher.
- [x] The migrated family no longer depends on the function being named `main` or on having zero fixed parameters when the parameters are outside the matched local-slot state.
- [ ] The remaining matcher boundaries track real value semantics rather than arbitrary naming or size-only constraints.
- [ ] Any further generalization is driven by typed-scalar/backend-semantic needs, not by size-only fixture growth.
- [ ] Tests cover the family with representative shapes rather than one fixture per slot count.
- [ ] Backend validation remains stable for the migrated family.

## Step 4: Shift target codegen onto backend semantics

- Keep removing emitter-local compatibility reshaping where lowering-owned structured views already exist.
- For the active migrated families, require direct and lowered emitter paths to converge on the same backend semantics.

Exit criteria:
- [ ] The active migrated family is emitted from structured backend semantics on both AArch64 and x86 paths in current coverage.
- [ ] No new target-local parsing or fake reshaping layers are introduced for the migrated family.
- [ ] Backend tests for the migrated paths pass on both supported targets in current coverage.

## Step 5: Validate positioning for later BIR work

- Confirm the resulting boundary is a clean precursor to `backend_ir -> bir`, not another transitional dead end.
- Record any remaining transitional shims that are intentionally left behind.
- Fold durable conclusions back into the source idea before closeout.

Exit criteria:
- [ ] The resulting structure is a clear foundation for later BIR work.
- [ ] Remaining transitional shims are documented and bounded.
- [ ] Backend validation is stable at the end of the active slice.
