Status: Active
Source Idea: ideas/open/03_lir_to_backend_ir_refactor.md
Activated from: ideas/open/03_lir_to_backend_ir_refactor.md

# LIR To Backend IR Refactor Runbook

## Purpose

Resume the structural backend-boundary refactor now that the backend regression-recovery plan has been closed.

## Goal

Turn the current `lir_adapter` boundary into a staged backend-lowering path that preserves behavior while moving decode/parsing responsibilities out of target emitters.

## Core Rule

Refactor one narrow lowering seam at a time, keep backend behavior stable, and prove each slice with backend-scoped tests before broadening the migration.

## Read First

- `src/backend/lir_adapter.hpp`
- `src/backend/lir_adapter.cpp`
- `src/backend/ir.hpp`
- `src/backend/aarch64/*`
- `src/backend/x86_64/*`
- `tests/backend/*`
- `tests/c/internal/backend_*`

## Scope

- Production backend lowering boundary between LIR and backend IR.
- Transitional renaming from "adapter" toward explicit lowering terminology.
- Isolation of LIR text-surface decoding into a narrower lowering layer.
- Backend-only validation scope during the migration.

## Non-Goals

- No frontend semantic changes.
- No broad redesign of target assembly emission.
- No one-shot replacement of the whole backend path.
- No unrelated backend correctness work unless it blocks the current refactor slice.

## Working Model

The migration should proceed in four behavior-preserving stages:

1. introduce an explicit lowering entrypoint and file ownership
2. move syntax-decoding helpers behind the lowering layer
3. reshape backend IR toward structured backend semantics
4. let target emitters consume backend semantics without parsing LLVM-shaped text

## Execution Rules

- Keep the existing backend path working throughout the transition.
- Prefer compatibility shims over large flag-day rewrites.
- Do not mix BIR scaffold work into this plan.
- When a new issue is outside this refactor scope, record it in `ideas/open/` instead of expanding the runbook.
- Validate with backend-focused tests after each slice and use broader backend sweeps before closing major steps.

## Step 1: Re-establish the lowering boundary

- Inspect the current production flow around `adapt_minimal_module()`.
- Introduce a behavior-preserving lowering entrypoint with backend-lowering naming.
- Split file ownership so the main production path is described as lowering rather than an adapter.
- Keep existing callers working through compatibility shims if needed.

Completion check:
- [ ] A production lowering entrypoint exists with explicit backend-lowering intent.
- [ ] The main production path is no longer described primarily as a "minimal adapter".
- [ ] Backend-scoped tests covering the touched path pass.

## Step 2: Isolate LIR syntax decoding

- Identify the helper surfaces that parse LLVM-shaped text in backend code.
- Move or wrap decode/parsing helpers so they live behind the lowering layer.
- Keep target-facing APIs behaviorally equivalent while reducing direct syntax parsing in emitters.

Completion check:
- [ ] LIR decode responsibility is isolated to a narrow backend-lowering layer.
- [ ] Target emitters do not gain new syntax-parsing dependencies during the slice.
- [ ] Targeted backend tests for touched decode/call paths pass.

## Step 3: Make backend IR more backend-native

- Replace the highest-value stringly-typed backend IR surfaces with structured forms.
- Start with operands/calls/addressing shapes that currently force parser-style helpers.
- Update validation/printing/helpers only as much as needed for the migrated slice.

Completion check:
- [ ] `src/backend/ir.hpp` is more backend-native in the migrated slice.
- [ ] The migrated slice no longer depends on parsing call/type structure from raw text.
- [ ] Backend-scoped regressions remain stable.

## Step 4: Shift target codegen onto backend semantics

- Update the AArch64 and x86 backends to consume the structured backend IR surfaces introduced in Step 3.
- Remove now-redundant target-side parsing from the migrated slice.
- Keep compatibility shims only where they materially reduce migration risk.

Completion check:
- [ ] Target-specific codegen in the migrated slice consumes backend semantics rather than syntax-decoding helpers.
- [ ] No new adapter-style parsing is required in target emitters for the migrated slice.
- [ ] Backend tests for the migrated paths pass on both supported targets in current coverage.

## Step 5: Validate positioning for later BIR work

- Confirm the refactor leaves a clean foundation for the later `backend_ir -> bir` scaffold.
- Record any remaining adapter-era surfaces that must stay as transitional shims.
- Update the source idea with completed slices, follow-ups, and residual risks.

Completion check:
- [ ] The result is clearly a foundation for later BIR migration, not a competing architecture.
- [ ] Remaining transitional shims are documented.
- [ ] Backend validation remains stable at the end of the active slice.
